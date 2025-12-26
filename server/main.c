#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <time.h>
#include "../common/protocol.h"
#include <stdarg.h>
#include "server.h"

// --- Logging Helper ---
void log_event(const char *category, const char *format, ...) {
    time_t now;
    time(&now);
    char buf[20]; // YYYY-MM-DD HH:MM:SS
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    printf("[%s] [%s] ", buf, category);
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    printf("\n");
    fflush(stdout); // Ensure immediate output
}

// --- Structures & Globals ---


ClientInfo clients[MAX_CLIENTS * MAX_LOBBIES];
int num_clients = 0;

// Chat history storage (server-side only)



LobbyChat lobby_chats[MAX_LOBBIES];

// Helper function to check if a user is online
int is_user_online(int user_id) {
    for (int i = 0; i < num_clients; i++) {
        if (clients[i].is_authenticated && clients[i].user_id == user_id) {
            return 1;  // Online
        }
    }
    return 0;  // Offline
}

GameState active_games[MAX_LOBBIES];

// --- THÊM: Tracking game update timing ---
long long last_game_update[MAX_LOBBIES];

ClientInfo* find_client_by_socket(int socket_fd) {
    for (int i = 0; i < num_clients; i++) {
        if (clients[i].socket_fd == socket_fd) return &clients[i];
    }
    return NULL;
}

void send_response(int socket_fd, ServerPacket *packet) {
    send(socket_fd, packet, sizeof(ServerPacket), 0);
}

// Broadcast full lobby list to all authenticated clients
void broadcast_lobby_list() {
    ServerPacket packet;
    packet.type = MSG_LOBBY_LIST;
    packet.payload.lobby_list.count = get_lobby_list(packet.payload.lobby_list.lobbies);

    for (int i = 0; i < num_clients; i++) {
        if (clients[i].is_authenticated) {
            send_response(clients[i].socket_fd, &packet);
        }
    }
}

void broadcast_lobby_update(int lobby_id) {
    Lobby *lobby = find_lobby(lobby_id);
    if (!lobby) return;
    
    ServerPacket packet;
    packet.type = MSG_LOBBY_UPDATE;
    packet.code = 0;
    packet.payload.lobby = *lobby;
    
    for (int i = 0; i < num_clients; i++) {
        if (clients[i].lobby_id == lobby_id) {
            send_response(clients[i].socket_fd, &packet);
        }
    }
}

void broadcast_game_state(int lobby_id) {
    GameState *full_state = &active_games[lobby_id];
    
    // Send per-player filtered state in fog of war mode
    for (int i = 0; i < num_clients; i++) {
        if (clients[i].lobby_id == lobby_id && clients[i].is_authenticated) {
            ServerPacket packet;
            memset(&packet, 0, sizeof(ServerPacket));
            packet.type = MSG_GAME_STATE;
            
            if (full_state->game_mode == GAME_MODE_FOG_OF_WAR) {
                // Filter state for this specific player
                GameState filtered_state;
                filter_game_state(full_state, clients[i].player_id_in_game, &filtered_state);
                packet.payload.game_state = filtered_state;
            } else {
                // Send full state for classic mode
                packet.payload.game_state = *full_state;
            }
            
            send_response(clients[i].socket_fd, &packet);
        }
    }
}

// --- THÊM: Get current time in milliseconds ---
long long get_current_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

// Forfeit Logic moved to handlers/game.c

// Generate a random session token
void generate_session_token(char *buffer, size_t length) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    if (length > 0) {
        for (size_t i = 0; i < length - 1; i++) {
            int key = rand() % (int)(sizeof(charset) - 1);
            buffer[i] = charset[key];
        }
        buffer[length - 1] = '\0';
    }
}

// --- Packet Handling ---

void handle_client_packet(int socket_fd, ClientPacket *pkt) {
    ClientInfo *client = find_client_by_socket(socket_fd);
    ServerPacket response;
    memset(&response, 0, sizeof(ServerPacket));
    
    if (!client) return;

    // Logic xử lý Game Input (Move, Bomb)
    // Logic xử lý Game Input (Move, Bomb)
    if (pkt->type == MSG_MOVE) {
        handle_game_move(socket_fd, pkt);
        return;
    } else if (pkt->type == MSG_PLANT_BOMB) {
        handle_plant_bomb(socket_fd, pkt);
        return;
    }

    // Logic xử lý System Input
    switch (pkt->type) {
        case MSG_REGISTER:
            handle_register(socket_fd, pkt);
            break;
            
        case MSG_LOGIN:
            handle_login(socket_fd, pkt);
            break;
            
        case MSG_LOGIN_WITH_TOKEN:
            handle_login_with_token(socket_fd, pkt);
            break;
            
        case MSG_RECONNECT:
            // Explicit reconnect request (usually used if socket drops mid-game)
            // Logic similar to Login with Token but might be stricter about existing session
            break;

        case MSG_CREATE_LOBBY:
            handle_create_lobby(socket_fd, pkt);
            break;

        case MSG_SPECTATE:
            handle_spectate(socket_fd, pkt);
            break;

        case MSG_JOIN_LOBBY:
            handle_join_lobby(socket_fd, pkt);
            break;
        
        case MSG_LEAVE_LOBBY:
            handle_leave_lobby(socket_fd, pkt);
            break;

        case MSG_LIST_LOBBIES:
            handle_list_lobbies(socket_fd, pkt);
            break;

        case MSG_LEAVE_GAME:
            handle_leave_game(socket_fd, pkt);
            break;

        case MSG_READY:
            handle_ready(socket_fd, pkt);
            break;

        case MSG_START_GAME:
            handle_start_game(socket_fd, pkt);
            break;
            
        case MSG_FRIEND_REQUEST:
            handle_friend_request(socket_fd, pkt);
            break;
        case MSG_FRIEND_ACCEPT:
            handle_friend_accept(socket_fd, pkt);
            break;
        case MSG_FRIEND_DECLINE:
            handle_friend_reject(socket_fd, pkt);
            break;
        case MSG_FRIEND_LIST:
            handle_friend_list(socket_fd, pkt);
            break;
        case MSG_GET_PROFILE:
            handle_get_profile(socket_fd, pkt);
            break;
    }
}

int main() {
    printf("╔════════════════════════════════════╗\n");
    printf("║  Bomberman Server v4.0 (SQLite3)  ║\n");
    printf("║  Game tick rate: 20 Hz (50ms)     ║\n");
    printf("╚════════════════════════════════════╝\n\n");
    
    if (db_init() != 0) {
        fprintf(stderr, "Failed to initialize database\n");
        return 1;
    }
    
    srand(time(NULL));
    init_lobbies();
    int server_fd = init_server_socket();
    
    for (int i = 0; i < MAX_LOBBIES; i++) {
        last_game_update[i] = 0;
    }
    
    fd_set readfds;
    struct timeval tv;
    int max_fd;

    printf("SERVER STARTED on PORT %d\n\n", PORT);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_fd = server_fd;

        for (int i = 0; i < num_clients; i++) {
            if (clients[i].socket_fd > 0) {
                FD_SET(clients[i].socket_fd, &readfds);
                if (clients[i].socket_fd > max_fd) max_fd = clients[i].socket_fd;
            }
        }

        // THAY ĐÔI: Giảm timeout để game loop chạy mượt hơn
        tv.tv_sec = 0;
        tv.tv_usec = 10000; // 10ms instead of 15ms

        int activity = select(max_fd + 1, &readfds, NULL, NULL, &tv);
        
        if (activity < 0) {
            // Error handling (có thể log nếu cần)
        }

        // 1. New Connections
        if (FD_ISSET(server_fd, &readfds)) {
            struct sockaddr_in addr;
            socklen_t len = sizeof(addr);
            int new_sock = accept(server_fd, (struct sockaddr*)&addr, &len);
            if (new_sock >= 0) {
                ClientInfo *cl = &clients[num_clients++];
                cl->socket_fd = new_sock;
                cl->lobby_id = -1;
                cl->player_id_in_game = -1;
                cl->is_authenticated = 0;
                cl->username[0] = '\0';
                log_event("CONNECTION", "Client %d connected", new_sock);
            }
        }

        // 2. Client Data
        for (int i = 0; i < num_clients; i++) {
            int sd = clients[i].socket_fd;
            if (FD_ISSET(sd, &readfds)) {
                ClientPacket pkt;
                int n = recv(sd, &pkt, sizeof(pkt), 0);
                if (n <= 0) {
                    // Client disconnected
                    log_event("DISCONNECT", "Client %d (%s)", sd, 
                           clients[i].username[0] ? clients[i].username : "unknown");

                    // CHECK: Is this user in an active game?
                    int handled = 0;
                    if (clients[i].lobby_id != -1 && clients[i].is_authenticated) {
                        Lobby *lb = find_lobby(clients[i].lobby_id);
                        if (lb && lb->status == LOBBY_PLAYING) {
                            // DO NOT FORFEIT YET! Just close the socket.
                            // The player object in GameState remains "alive" but un-controlled.
                            // Ideally, mark them as "DISCONNECTED" in GameState struct if you have a flag, 
                            // or just let them stand still.
                            printf("[SESSION] User %s preserved in lobby %d for reconnect.\n", clients[i].username, clients[i].lobby_id);
                            
                            // Remove ClientInfo from active socket list, BUT we rely on Database/GameState 
                            // to persist the "Player". 
                            // Issue: `clients[]` array is the only link between socket and game.
                            // If we remove `clients[i]`, we lose the map socket -> player.
                            // New Logic: When RECONNECT comes in, we scan Active Games to find the player.
                            handled = 1; 
                        }
                    }

                    if (!handled && clients[i].lobby_id != -1) {
                         // Normal leave (not in game, or game waiting)
                         leave_lobby(clients[i].lobby_id, clients[i].username);
                         broadcast_lobby_update(clients[i].lobby_id);
                         broadcast_lobby_list();
                    }
                    
                    
                    close(sd);
                    clients[i] = clients[num_clients-1];
                    num_clients--;
                    i--;
                } else {
                    handle_client_packet(sd, &pkt);
                }
            }
        }

        // 3. *** CRITICAL: REALTIME GAME LOOP với TIMING CONTROL ***
        long long now = get_current_time_ms();
        const long long GAME_TICK_INTERVAL = 50; // 50ms = 20 ticks/second
        
        for (int i = 0; i < MAX_LOBBIES; i++) {
            Lobby *lb = find_lobby(i);
            if (lb && lb->status == LOBBY_PLAYING) {
                
                // KIỂM TRA: Chỉ update khi đủ thời gian
                if (now - last_game_update[i] >= GAME_TICK_INTERVAL) {
                    
                    // Update game logic (bombs, explosions, deaths)
                    update_game(&active_games[i]);
                    
                    // Check if game just ended and calculate ELO BEFORE broadcasting
                    if (active_games[i].game_status == GAME_ENDED) {
                        GameState *gs = &active_games[i];
                        
                        // Only calculate ELO once (check if not already calculated)
                        int already_calculated = 0;
                        for (int p = 0; p < gs->num_players; p++) {
                            if (gs->elo_changes[p] != 0) {
                                already_calculated = 1;
                                break;
                            }
                        }
                        
                        if (!already_calculated) {
                            log_event("GAME", "Lobby %d ended. Winner: %d", i, gs->winner_id);
                            
                            // Prepare data for stats recording
                            int player_ids[MAX_CLIENTS];
                            int placements[MAX_CLIENTS];
                            int kills[MAX_CLIENTS];
                            
                            // Get actual player IDs and populate kills
                            for (int p = 0; p < gs->num_players; p++) {
                                // Find user_id by username
                                int found_user_id = -1;
                                for (int k = 0; k < num_clients; k++) {
                                    if (clients[k].is_authenticated && 
                                        strcmp(clients[k].username, gs->players[p].username) == 0) {
                                        found_user_id = clients[k].user_id;
                                        break;
                                    }
                                }
                                
                                player_ids[p] = found_user_id;
                                placements[p] = (p == gs->winner_id) ? 1 : 2;
                                kills[p] = gs->kills[p];
                                
                                log_event("ELO", "Player %s (user_id: %d) -> Placement: %d, Kills: %d", 
                                       gs->players[p].username, player_ids[p], placements[p], kills[p]);
                            }
                            
                            // Update ELO ratings and get changes BEFORE broadcasting
                            int elo_changes_temp[MAX_CLIENTS] = {0};
                            if (elo_update_after_match(player_ids, placements, gs->num_players, elo_changes_temp) == 0) {
                                printf("[ELO] Successfully updated ELO ratings\n");
                                
                                // Store ELO changes in game state for client display
                                printf("[ELO] Storing ELO changes in game state:\n");
                                for (int p = 0; p < gs->num_players; p++) {
                                    gs->elo_changes[p] = elo_changes_temp[p];
                                    printf("[ELO]   Player %d: elo_changes[%d] = %d\n", p, p, gs->elo_changes[p]);
                                }
                            } else {
                                printf("[ELO] ERROR: Failed to update ELO ratings\n");
                            }
                            
                            // Record match statistics with actual duration
                            int duration_seconds = gs->match_duration_seconds;
                            int match_id = stats_record_match(player_ids, placements, kills, 
                                                             gs->num_players, gs->winner_id, 
                                                             duration_seconds);
                            
                            if (match_id >= 0) {
                                log_event("STATS", "Match recorded with ID: %d (Duration: %d seconds)", 
                                       match_id, duration_seconds);
                            } else {
                                printf("[STATS] ERROR: Failed to record match\n");
                            }
                            
                            // Send notification to players about ELO changes
                            for (int j = 0; j < num_clients; j++) {
                                if (clients[j].lobby_id == i && clients[j].is_authenticated) {
                                    ServerPacket notif;
                                    memset(&notif, 0, sizeof(ServerPacket));
                                    notif.type = MSG_NOTIFICATION;
                                    notif.code = 0;
                                    
                                    if (gs->winner_id >= 0 && 
                                        strcmp(clients[j].username, gs->players[gs->winner_id].username) == 0) {
                                        sprintf(notif.message, "Victory! ELO updated.");
                                    } else {
                                        sprintf(notif.message, "Match ended. ELO updated.");
                                    }
                                    
                                    send_response(clients[j].socket_fd, &notif);
                                }
                            }
                            
                            lb->status = LOBBY_WAITING;
                            broadcast_lobby_update(i);
                        }
                    }
                    
                    // Broadcast state to all players (NOW with ELO changes populated!)
                    broadcast_game_state(i);
                    
                    // Update timer
                    last_game_update[i] = now;
                }
            }
        }
    }
    
    return 0;
}
