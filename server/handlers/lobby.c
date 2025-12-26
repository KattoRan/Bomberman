#include <stdio.h>
#include <string.h>
#include "../server.h"

void handle_create_lobby(int socket_fd, ClientPacket *pkt) {
    ClientInfo *client = find_client_by_socket(socket_fd);
    if (!client || !client->is_authenticated) return;

    ServerPacket response;
    memset(&response, 0, sizeof(ServerPacket));

    int lid = create_lobby(pkt->room_name, client->username, pkt->is_private, pkt->access_code, pkt->game_mode);
    if (lid >= 0) {
        client->lobby_id = lid;
        response.type = MSG_LOBBY_UPDATE;
        response.payload.lobby = *find_lobby(lid);
        send_response(socket_fd, &response);
        
        if (pkt->is_private) {
            log_event("LOBBY", "Private room created with code: %s", pkt->access_code);
        }
    }
}

void handle_join_lobby(int socket_fd, ClientPacket *pkt) {
    ClientInfo *client = find_client_by_socket(socket_fd);
    if (!client || !client->is_authenticated) return;
    
    ServerPacket response;
    memset(&response, 0, sizeof(ServerPacket));

    // Use join_lobby_with_code to support private rooms
    int join_res = join_lobby_with_code(pkt->lobby_id, client->username, pkt->access_code);
    if (join_res == 0) {
        client->lobby_id = pkt->lobby_id;
        broadcast_lobby_update(pkt->lobby_id);
        broadcast_lobby_list(); // keep lobby list in sync for others
    } else {
        response.type = MSG_ERROR;
        response.code = join_res;
        // Better error messages
        if (join_res == ERR_LOBBY_WRONG_ACCESS_CODE) {
            strcpy(response.message, "Wrong access code");
        } else if (join_res == ERR_LOBBY_LOCKED) {
            strcpy(response.message, "Room is locked");
        } else if (join_res == ERR_LOBBY_GAME_IN_PROGRESS) {
            strcpy(response.message, "Game in progress");
        } else {
            strcpy(response.message, "Cannot join lobby");
        }
        send_response(socket_fd, &response);
    }
}

void handle_spectate(int socket_fd, ClientPacket *pkt) {
    ClientInfo *client = find_client_by_socket(socket_fd);
    if (!client || !client->is_authenticated) return;
    
    ServerPacket response;
    memset(&response, 0, sizeof(ServerPacket));
    
    int res = join_spectator(pkt->lobby_id, client->username);
    if (res == 0) {
        client->lobby_id = pkt->lobby_id;
        client->player_id_in_game = -1; // Mark as spectator
        
        Lobby *lb = find_lobby(pkt->lobby_id);
        
        // Send Lobby Update
        response.type = MSG_LOBBY_UPDATE;
        response.payload.lobby = *lb;
        send_response(socket_fd, &response);
        
        // If game is running, send initial state
        if (lb->status == LOBBY_PLAYING) {
             ServerPacket gs_pkt;
             memset(&gs_pkt, 0, sizeof(ServerPacket));
             gs_pkt.type = MSG_GAME_STATE;
             gs_pkt.payload.game_state = active_games[pkt->lobby_id];
             send_response(socket_fd, &gs_pkt);
        }
        
        // Notify everyone else
        broadcast_lobby_update(pkt->lobby_id);
    } else {
        response.type = MSG_ERROR;
        response.code = res;
        if (res == ERR_LOBBY_FULL) strcpy(response.message, "Room full (spectators)");
        else if (res == ERR_LOBBY_DUPLICATE_USER) strcpy(response.message, "Already joined");
        else strcpy(response.message, "Cannot spectate");
        send_response(socket_fd, &response);
    }
}

void handle_leave_lobby(int socket_fd, ClientPacket *pkt) {
    (void)pkt; // unused
    ClientInfo *client = find_client_by_socket(socket_fd);
    if (!client) return;

    if (client->lobby_id != -1) {
        int old_lid = client->lobby_id;
        leave_lobby(old_lid, client->username);
        client->lobby_id = -1;
        broadcast_lobby_update(old_lid);
        broadcast_lobby_list();
        
        ServerPacket response;
        memset(&response, 0, sizeof(ServerPacket));
        response.type = MSG_LOBBY_LIST;
        response.payload.lobby_list.count = get_lobby_list(response.payload.lobby_list.lobbies);
        send_response(socket_fd, &response);
    }
}

void handle_list_lobbies(int socket_fd, ClientPacket *pkt) {
    (void)pkt; // unused
    ServerPacket response;
    memset(&response, 0, sizeof(ServerPacket));
    response.type = MSG_LOBBY_LIST;
    response.payload.lobby_list.count = get_lobby_list(response.payload.lobby_list.lobbies);
    send_response(socket_fd, &response);
}

void handle_ready(int socket_fd, ClientPacket *pkt) {
    (void)pkt;
    ClientInfo *client = find_client_by_socket(socket_fd);
    if (!client) return;

    if (client->lobby_id != -1) {
        toggle_ready(client->lobby_id, client->username);
        broadcast_lobby_update(client->lobby_id);
    }
}

void handle_start_game(int socket_fd, ClientPacket *pkt) {
    (void)pkt;
    ClientInfo *client = find_client_by_socket(socket_fd);
    if (!client) return;
    
    if (client->lobby_id != -1) {
        int start_res = start_game(client->lobby_id, client->username);
        if (start_res == 0) {
            Lobby *lb = find_lobby(client->lobby_id);
            init_game(&active_games[client->lobby_id], lb);
            
            // Initialize game update timer
            last_game_update[client->lobby_id] = get_current_time_ms();
            
            // Set player_id_in_game for each client in this lobby for fog of war
            GameState *gs = &active_games[client->lobby_id];
            for (int i = 0; i < num_clients; i++) {
                if (clients[i].lobby_id == client->lobby_id) {
                    // Find this client's player ID in the game state
                    for (int p = 0; p < gs->num_players; p++) {
                        if (strcmp(clients[i].username, gs->players[p].username) == 0) {
                            clients[i].player_id_in_game = p;
                            printf("[FOG] Set player_id_in_game for %s: %d\n", 
                                   clients[i].username, p);
                            break;
                        }
                    }
                }
            }
            broadcast_lobby_update(client->lobby_id);
            broadcast_game_state(client->lobby_id);
        }
    }
}
