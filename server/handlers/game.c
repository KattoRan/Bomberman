#include <stdio.h>
#include <string.h>
#include "../server.h"

void handle_game_move(int socket_fd, ClientPacket *pkt) {
    ClientInfo *client = find_client_by_socket(socket_fd);
    if (!client) return;
    
    // Validate client is in a playing lobby
    if (client->lobby_id != -1) {
        Lobby *lobby = find_lobby(client->lobby_id);
        if (lobby && lobby->status == LOBBY_PLAYING) {
            GameState *gs = &active_games[client->lobby_id];
            
            // Find player index in game state
            int p_id = -1;
            for(int i=0; i<gs->num_players; i++) {
                if (strcmp(gs->players[i].username, client->username) == 0) {
                    p_id = i;
                    break;
                }
            }

            if (p_id != -1) {
                ServerPacket notif;
                memset(&notif, 0, sizeof(ServerPacket));
                
                int move_result = handle_move(gs, p_id, pkt->data);
                
                // Check if power-up was involved
                if (move_result == 11) {
                    // Picked up power-up
                    notif.type = MSG_NOTIFICATION;
                    notif.code = 0;
                    sprintf(notif.message, "Power-up collected!");
                    send_response(client->socket_fd, &notif);
                } else if (move_result == 12) {
                    // Already at max
                    notif.type = MSG_NOTIFICATION;
                    notif.code = 1;
                    sprintf(notif.message, "Already at maximum capacity!");
                    send_response(client->socket_fd, &notif);
                }
                
                // IMPORTANT: Broadcast immediately for responsiveness
                broadcast_game_state(client->lobby_id);
            }
        }
    }
}

void handle_plant_bomb(int socket_fd, ClientPacket *pkt) {
    (void)pkt;
    ClientInfo *client = find_client_by_socket(socket_fd);
    if (!client) return;

    if (client->lobby_id != -1) {
        Lobby *lobby = find_lobby(client->lobby_id);
        if (lobby && lobby->status == LOBBY_PLAYING) {
            GameState *gs = &active_games[client->lobby_id];
            
            int p_id = -1;
            for(int i=0; i<gs->num_players; i++) {
                if (strcmp(gs->players[i].username, client->username) == 0) {
                    p_id = i;
                    break;
                }
            }

            if (p_id != -1) {
                plant_bomb(gs, p_id);
                broadcast_game_state(client->lobby_id);
            }
        }
    }
}

// Forward declaration of forfeit function to use existing logic in main.c? 
// Ideally "forfeit_player_from_game" should be in game_logic.c or here.
// For now, let's look at where forfeit_player_from_game is defined.
// It is currently in main.c. We should probably move it to handlers/game.c or game_logic.c
// But to keep it simple, we will declare it as extern from main.c if we don't move it yet.
// However, the plan said "Cleanup server/main.c". So moving it to game_logic.c or here is better.
// Let's assume for this step we will move it HERE (handlers/game.c) or make it available.
// Actually, `forfeit_player_from_game` fits perfectly in `server/game_logic.c`. 
// But since I am only allowed to create new files or edit existing ones, let's keep it simple.
// I will RE-IMPLEMENT `forfeit_player_from_game` here as a static helper or move it.
// To avoid duplication/complexity, I will declare it extern if I leave it in main, 
// OR better yet, let's put it in `handlers/game.c` and remove from `main.c`.

void forfeit_player_from_game(int lobby_id, const char *username) {
    Lobby *lb = find_lobby(lobby_id);
    if (!lb || lb->status != LOBBY_PLAYING) return;

    GameState *gs = &active_games[lobby_id];
    int p_idx = -1;
    for (int i = 0; i < gs->num_players; i++) {
        if (strcmp(gs->players[i].username, username) == 0) {
            p_idx = i;
            break;
        }
    }

    if (p_idx != -1) {
        gs->players[p_idx].is_alive = 0;
        log_event("GAME", "%s forfeited in lobby %d", username, lobby_id);
    }

    // Check if game should end after forfeit
    int alive = 0;
    int last_alive = -1;
    for (int i = 0; i < gs->num_players; i++) {
        if (gs->players[i].is_alive) {
            alive++;
            last_alive = i;
        }
    }

    if (gs->game_status == GAME_RUNNING && alive <= 1) {
        gs->game_status = GAME_ENDED;
        gs->winner_id = (alive == 1) ? last_alive : -1;
    }

    broadcast_game_state(lobby_id);
}

void handle_leave_game(int socket_fd, ClientPacket *pkt) {
    (void)pkt;
    ClientInfo *client = find_client_by_socket(socket_fd);
    if (!client) return;

    if (client->lobby_id != -1) {
        int lid = client->lobby_id;
        forfeit_player_from_game(lid, client->username);
        leave_lobby(lid, client->username);
        client->lobby_id = -1;
        broadcast_lobby_list();

        ServerPacket response;
        memset(&response, 0, sizeof(ServerPacket));
        response.type = MSG_LOBBY_LIST;
        response.payload.lobby_list.count = get_lobby_list(response.payload.lobby_list.lobbies);
        send_response(socket_fd, &response);
    }
}
