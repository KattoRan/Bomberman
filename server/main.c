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
#include "server.h"

// --- Structures & Globals ---
typedef struct {
    int socket_fd;
    int user_id;                      // Database user ID
    char username[MAX_USERNAME];
    char display_name[MAX_DISPLAY_NAME];
    int is_authenticated;
    int lobby_id;
    int player_id_in_game; 
} ClientInfo;

ClientInfo clients[MAX_CLIENTS * MAX_LOBBIES];
int num_clients = 0;

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
    ServerPacket packet;
    packet.type = MSG_GAME_STATE;
    packet.payload.game_state = active_games[lobby_id];

    for (int i = 0; i < num_clients; i++) {
        if (clients[i].lobby_id == lobby_id && clients[i].is_authenticated) {
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

// Mark a player as forfeited during an active game, and end game if only one remains
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
        printf("[GAME] %s forfeited in lobby %d\n", username, lobby_id);
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

// --- Packet Handling ---

void handle_client_packet(int socket_fd, ClientPacket *pkt) {
    ClientInfo *client = find_client_by_socket(socket_fd);
    ServerPacket response;
    memset(&response, 0, sizeof(ServerPacket));
    
    if (!client) return;

    // Logic xử lý Game Input (Move, Bomb)
    if (pkt->type == MSG_MOVE || pkt->type == MSG_PLANT_BOMB) {
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
                    ServerPacket notif;
                    memset(&notif, 0, sizeof(ServerPacket));
                    
                    if (pkt->type == MSG_MOVE) {
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
                    } else if (pkt->type == MSG_PLANT_BOMB) {
                        plant_bomb(gs, p_id);
                    }
                    
                    // IMPORTANT: Broadcast ngay khi có input để responsive
                    broadcast_game_state(client->lobby_id);
                }
            }
        }
        return; 
    }

    // Logic xử lý System Input
    switch (pkt->type) {
        case MSG_REGISTER:
            response.type = MSG_AUTH_RESPONSE;
            response.code = db_register_user(pkt->username, pkt->email, pkt->password);
            if (response.code == AUTH_SUCCESS) {
                strcpy(response.message, "Registration successful");
                printf("[AUTH] Registration: %s (email: %s)\n", pkt->username, pkt->email);
            } else {
                strcpy(response.message, "Registration failed");
            }
            send_response(socket_fd, &response);
            break;
            
        case MSG_LOGIN:
            {
                User user;
                response.type = MSG_AUTH_RESPONSE;
                response.code = db_login_user(pkt->username, pkt->password, &user);
                if (response.code == AUTH_SUCCESS) {
                    client->user_id = user.id;
                    strncpy(client->username, user.username, MAX_USERNAME - 1);
                    strncpy(client->display_name, user.display_name, MAX_DISPLAY_NAME - 1);
                    client->is_authenticated = 1;
                    
                    // Send back user info
                    response.payload.auth.user_id = user.id;
                    strncpy(response.payload.auth.username, user.username, MAX_USERNAME - 1);
                    strncpy(response.payload.auth.display_name, user.display_name, MAX_DISPLAY_NAME - 1);
                    response.payload.auth.elo_rating = user.elo_rating;
                    strcpy(response.message, "Login successful");
                    
                    printf("[AUTH] Login: %s (ID: %d, ELO: %d)\n", 
                           user.username, user.id, user.elo_rating);
                } else {
                    strcpy(response.message, "Invalid credentials");
                }
                send_response(socket_fd, &response);
            }
            break;

        case MSG_CREATE_LOBBY:
            if (!client->is_authenticated) break;
            int lid = create_lobby(pkt->room_name, client->username, pkt->is_private, pkt->access_code);
            if (lid >= 0) {
                client->lobby_id = lid;
                response.type = MSG_LOBBY_UPDATE;
                response.payload.lobby = *find_lobby(lid);
                send_response(socket_fd, &response);
                
                if (pkt->is_private) {
                    printf("[LOBBY] Private room created with code: %s\n", pkt->access_code);
                }

                // Notify everyone to refresh lobby list
                broadcast_lobby_list();
            }
            break;

        case MSG_JOIN_LOBBY:
            if (!client->is_authenticated) break;
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
            break;
        
        case MSG_LEAVE_LOBBY:
            if (client->lobby_id != -1) {
                int old_lid = client->lobby_id;
                leave_lobby(old_lid, client->username);
                client->lobby_id = -1;
                broadcast_lobby_update(old_lid);
                broadcast_lobby_list();
                
                response.type = MSG_LOBBY_LIST;
                response.payload.lobby_list.count = get_lobby_list(response.payload.lobby_list.lobbies);
                send_response(socket_fd, &response);
            }
            break;

        case MSG_LIST_LOBBIES:
            response.type = MSG_LOBBY_LIST;
            response.payload.lobby_list.count = get_lobby_list(response.payload.lobby_list.lobbies);
            send_response(socket_fd, &response);
            break;

        case MSG_LEAVE_GAME:
            if (client->lobby_id != -1) {
                int lid = client->lobby_id;
                forfeit_player_from_game(lid, client->username);
                leave_lobby(lid, client->username);
                client->lobby_id = -1;
                broadcast_lobby_list();

                response.type = MSG_LOBBY_LIST;
                response.payload.lobby_list.count = get_lobby_list(response.payload.lobby_list.lobbies);
                send_response(socket_fd, &response);
            }
            break;

        case MSG_READY:
            if (client->lobby_id != -1) {
                toggle_ready(client->lobby_id, client->username);
                broadcast_lobby_update(client->lobby_id);
            }
            break;

        case MSG_START_GAME:
            if (client->lobby_id != -1) {
                int start_res = start_game(client->lobby_id, client->username);
                if (start_res == 0) {
                    Lobby *lb = find_lobby(client->lobby_id);
                    init_game(&active_games[client->lobby_id], lb);
                    
                    // THÊM: Initialize game update timer
                    last_game_update[client->lobby_id] = get_current_time_ms();
                    
                    broadcast_lobby_update(client->lobby_id);
                    broadcast_game_state(client->lobby_id);
                }
            }
            break;
            
        case MSG_FRIEND_REQUEST:
            if (!client->is_authenticated) break;
            {
                int result = friend_send_request(client->user_id, pkt->target_display_name);
                response.type = MSG_NOTIFICATION;
                response.code = result;
                if (result == 0) {
                    snprintf(response.message, sizeof(response.message), 
                             "Friend request sent to %s", pkt->target_display_name);
                } else {
                    strcpy(response.message, "Friend request failed");
                }
               send_response(socket_fd, &response);
            break;
        }    
        
        case MSG_FRIEND_LIST: {
            if (!client->is_authenticated) break;
            response.type = MSG_FRIEND_LIST_RESPONSE;
            
            // Get accepted friends
            int friend_count = friend_get_list(client->user_id, 
                                              response.payload.friend_list.friends, 50);
            response.payload.friend_list.count = friend_count;
            
            // Get pending requests (incoming)
            FriendInfo pending[50];
            int pending_count = friend_get_pending_requests(client->user_id, pending, 50);
            
            // Get sent requests (outgoing)
            FriendInfo sent[50];
            int sent_count = friend_get_sent_requests(client->user_id, sent, 50);
            
            // Append pending to the friends array if there's space
            if (friend_count + pending_count + sent_count <= 50) {
                memcpy(&response.payload.friend_list.friends[friend_count], 
                       pending, sizeof(FriendInfo) * pending_count);
                memcpy(&response.payload.friend_list.friends[friend_count + pending_count],
                       sent, sizeof(FriendInfo) * sent_count);
                response.payload.friend_list.count += pending_count + sent_count;
            }
            
            // Use 'code' field: low byte = pending_count, high byte = sent_count
            response.code = pending_count | (sent_count << 8);
            
            send_response(socket_fd, &response);
            break;
        }    
        
        case MSG_FRIEND_ACCEPT:
            if (!client->is_authenticated) break;
            {
                int result = friend_accept_request(client->user_id, pkt->target_user_id);
                response.type = MSG_NOTIFICATION;
                response.code = result;
                if (result == 0) {
                    strcpy(response.message, "Friend request accepted");
                } else {
                    strcpy(response.message, "Failed to accept request");
                }
                send_response(socket_fd, &response);
                
                // Refresh friend list
                if (result == 0) {
                    response.type = MSG_FRIEND_LIST_RESPONSE;
                    int friend_count = friend_get_list(client->user_id, 
                                                      response.payload.friend_list.friends, 50);
                    response.payload.friend_list.count = friend_count;
                    
                    FriendInfo pending[50];
                    int pending_count = friend_get_pending_requests(client->user_id, pending, 50);
                    
                    if (friend_count + pending_count <= 50) {
                        memcpy(&response.payload.friend_list.friends[friend_count], 
                               pending, sizeof(FriendInfo) * pending_count);
                        response.payload.friend_list.count += pending_count;
                    }
                    
                    response.code = pending_count;
                    send_response(socket_fd, &response);
                }
            }
            break;
            
        case MSG_FRIEND_DECLINE:
            if (!client->is_authenticated) break;
            {
                int result = friend_decline_request(client->user_id, pkt->target_user_id);
                response.type = MSG_NOTIFICATION;
                response.code = result;
                if (result == 0) {
                    strcpy(response.message, "Friend request declined");
                } else {
                    strcpy(response.message, "Failed to decline request");
                }
                send_response(socket_fd, &response);
                
                // Refresh friend list
                if (result == 0) {
                    response.type = MSG_FRIEND_LIST_RESPONSE;
                    int friend_count = friend_get_list(client->user_id, 
                                                      response.payload.friend_list.friends, 50);
                    response.payload.friend_list.count = friend_count;
                    
                    FriendInfo pending[50];
                    int pending_count = friend_get_pending_requests(client->user_id, pending, 50);
                    
                    if (friend_count + pending_count <= 50) {
                        memcpy(&response.payload.friend_list.friends[friend_count], 
                               pending, sizeof(FriendInfo) * pending_count);
                        response.payload.friend_list.count += pending_count;
                    }
                    
                    response.code = pending_count;
                    send_response(socket_fd, &response);
                }
            }
            break;
        
        case MSG_FRIEND_REMOVE:
            if (!client->is_authenticated) break;
            {
                int result = friend_remove(client->user_id, pkt->target_user_id);
                response.type = MSG_NOTIFICATION;
                response.code = result;
                if (result == 0) {
                    strcpy(response.message, "Friend removed");
                } else {
                    strcpy(response.message, "Failed to remove friend");
                }
                send_response(socket_fd, &response);
                
                // Refresh friend list
                if (result == 0) {
                    response.type = MSG_FRIEND_LIST_RESPONSE;
                    int friend_count = friend_get_list(client->user_id, 
                                                      response.payload.friend_list.friends, 50);
                    response.payload.friend_list.count = friend_count;
                    
                    FriendInfo pending[50];
                    int pending_count = friend_get_pending_requests(client->user_id, pending, 50);
                    
                    if (friend_count + pending_count <= 50) {
                        memcpy(&response.payload.friend_list.friends[friend_count], 
                               pending, sizeof(FriendInfo) * pending_count);
                        response.payload.friend_list.count += pending_count;
                    }
                    
                    response.code = pending_count;
                    send_response(socket_fd, &response);
                }
            }
            break;
        
        case MSG_GET_PROFILE:
            if (!client->is_authenticated) break;
            {
                response.type = MSG_PROFILE_RESPONSE;
                int target_id = (pkt->data > 0) ? pkt->data : client->user_id;
                if (stats_get_profile(target_id, &response.payload.profile) == 0) {
                    response.code = 0;
                } else {
                    response.code = -1;
                    strcpy(response.message, "Profile not found");
                }
                send_response(socket_fd, &response);
            }
            break;
            
        case MSG_GET_LEADERBOARD:
            if (!client->is_authenticated) break;
            {
                response.type = MSG_LEADERBOARD_RESPONSE;
                response.payload.leaderboard.count = 
                    stats_get_leaderboard(response.payload.leaderboard.entries, 100);
                send_response(socket_fd, &response);
            }
            break;
    }
}

// --- Main Server Loop ---

int main() {
    printf("╔════════════════════════════════════╗\n");
    printf("║  Bomberman Server v4.0 (SQLite3)  ║\n");
    printf("║  Game tick rate: 20 Hz (50ms)     ║\n");
    printf("╚════════════════════════════════════╝\n\n");
    
    // Initialize SQLite database
    if (db_init() != 0) {
        fprintf(stderr, "Failed to initialize database\n");
        return 1;
    }
    
    // Initialize random number generator ONCE at startup
    srand(time(NULL));
    
    init_lobbies();
    int server_fd = init_server_socket();
    
    // Initialize game update timers
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
                cl->is_authenticated = 0;
                cl->username[0] = '\0';
                printf("[CONNECTION] Client %d connected\n", new_sock);
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
                    printf("[DISCONNECT] Client %d (%s)\n", sd, 
                           clients[i].username[0] ? clients[i].username : "unknown");

                    if (clients[i].lobby_id != -1) {
                         // If leaving during a game, mark forfeit
                         forfeit_player_from_game(clients[i].lobby_id, clients[i].username);
                         leave_lobby(clients[i].lobby_id, clients[i].username);
                         broadcast_lobby_update(clients[i].lobby_id);
                         broadcast_lobby_list();
                    }
                    // No longer using logout_user() since it doesn't exist
                    // Database tracks online status differently now
                    
                    
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
                    
                    // Broadcast state to all players
                    broadcast_game_state(i);
                    
                    // Update timer
                    last_game_update[i] = now;

                    // Check game over
                    if (active_games[i].game_status == GAME_ENDED) {
                        printf("[GAME] Lobby %d ended. Winner: %d\n", 
                               i, active_games[i].winner_id);
                        
                        // === NEW: ELO AND STATS CALCULATION ===
                        GameState *gs = &active_games[i];
                        int player_ids[MAX_CLIENTS];
                        int placements[MAX_CLIENTS];
                        int kills[MAX_CLIENTS] = {0};  // TODO: Track kills during gameplay
                        
                        // Map player usernames to user_ids
                        for (int j = 0; j < gs->num_players; j++) {
                            // Find the client with this username
                            int found_user_id = -1;
                            for (int k = 0; k < num_clients; k++) {
                                if (clients[k].is_authenticated && 
                                    strcmp(clients[k].username, gs->players[j].username) == 0) {
                                    found_user_id = clients[k].user_id;
                                    break;
                                }
                            }
                            
                            player_ids[j] = found_user_id;
                            
                            // Determine placement: winner = 1, others = 2
                            if (j == gs->winner_id) {
                                placements[j] = 1;  // Winner
                            } else {
                                placements[j] = 2;  // Loser
                            }
                            
                            printf("[ELO] Player %s (user_id: %d) -> Placement: %d\n", 
                                   gs->players[j].username, player_ids[j], placements[j]);
                        }
                        
                        // Calculate match duration (for now, use 0 as placeholder)
                        int duration_seconds = 0;  // TODO: Track actual match duration
                        
                        // Update ELO ratings
                        if (elo_update_after_match(player_ids, placements, gs->num_players) == 0) {
                            printf("[ELO] Successfully updated ELO ratings\n");
                        } else {
                            printf("[ELO] ERROR: Failed to update ELO ratings\n");
                        }
                        
                        // Record match statistics
                        int match_id = stats_record_match(player_ids, placements, kills, 
                                                         gs->num_players, gs->winner_id, 
                                                         duration_seconds);
                        if (match_id >= 0) {
                            printf("[STATS] Match recorded with ID: %d\n", match_id);
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
                        // === END ELO AND STATS ===
                        
                        lb->status = LOBBY_WAITING;
                        
                        // Reset all players to not ready
                        for (int j = 0; j < lb->num_players; j++) {
                            if (j != lb->host_id) {
                                lb->players[j].is_ready = 0;
                            }
                        }
                        
                        broadcast_lobby_update(i);
                        
                        // Broadcast final game state
                        broadcast_game_state(i);
                    }
                }
            }
        }
    }
    
    return 0;
}
