#include "packet_handlers.h"
#include "ui.h"
#include "../common/protocol.h"
#include <SDL2/SDL.h>
#include <string.h>
#include <stdio.h>

// Forward declaration of screen state enum from main.c
typedef enum {
    SCREEN_LOGIN,
    SCREEN_REGISTER,
    SCREEN_LOBBY_LIST,
    SCREEN_LOBBY_ROOM,
    SCREEN_GAME,
    SCREEN_FRIENDS,
    SCREEN_PROFILE,
    SCREEN_LEADERBOARD,
    SCREEN_SETTINGS,
    SCREEN_POST_MATCH
} ScreenState;

// Chat system types from main.c
#define MAX_CHAT_MESSAGES 50
typedef struct {
    char sender[MAX_USERNAME];
    char message[200];
    Uint32 timestamp;
    int player_id;
    int is_current_user;
} ChatMessage;

// External global variables from main.c
extern ScreenState current_screen;
extern int sock;
extern int my_player_id;
extern char my_username[MAX_USERNAME];
extern char status_message[256];
extern char lobby_error_message[256];
extern Uint32 error_message_time;

extern Lobby lobby_list[MAX_LOBBIES];
extern int lobby_count;
extern Lobby current_lobby;

extern GameState current_state;
extern GameState previous_state;

extern FriendInfo friends_list[50];
extern int friends_count;
extern FriendInfo pending_requests[50];
extern int pending_count;
extern FriendInfo sent_requests[50];
extern int sent_count;
extern ProfileData my_profile;
extern LeaderboardEntry leaderboard[100];
extern int leaderboard_count;

extern char notification_message[256];
extern Uint32 notification_time;

extern Uint32 game_start_time;
extern int post_match_shown;
extern int post_match_winner_id;
extern int post_match_elo_changes[4];
extern int post_match_kills[4];
extern int post_match_duration;

extern ChatMessage chat_history[MAX_CHAT_MESSAGES];
extern int chat_count;

extern IncomingInvite current_invite;

// External functions from main.c
extern void send_packet(int type, int data);
extern void save_session_token(const char *token);
extern void clear_session_token(void);
extern void check_game_changes(void);
extern void add_notification(const char *text, SDL_Color color);

// Strategy pattern: function pointer array indexed by message type
static PacketHandler handlers[64] = {NULL};

void init_client_packet_handlers(void) {
    // Map message types to handlers
    handlers[MSG_AUTH_RESPONSE] = handle_auth_response;
    handlers[MSG_LOBBY_LIST] = handle_lobby_list;
    handlers[MSG_LOBBY_UPDATE] = handle_lobby_update;
    handlers[MSG_GAME_STATE] = handle_game_state;
    handlers[MSG_ERROR] = handle_error;
    handlers[MSG_FRIEND_LIST_RESPONSE] = handle_friend_list_response;
    handlers[MSG_FRIEND_RESPONSE] = handle_friend_response;
    handlers[MSG_PROFILE_RESPONSE] = handle_profile_response;
    handlers[MSG_LEADERBOARD_RESPONSE] = handle_leaderboard_response;
    handlers[MSG_NOTIFICATION] = handle_notification;
    handlers[MSG_CHAT] = handle_chat;
    handlers[MSG_INVITE_RECEIVED] = handle_invite_received;
}

void handle_server_packet(ServerPacket *pkt) {
    if (pkt->type >= 0 && pkt->type < 64 && handlers[pkt->type] != NULL) {
        handlers[pkt->type](pkt);
    } else {
        printf("[CLIENT] Unknown packet type: %d\n", pkt->type);
    }
}

void handle_auth_response(ServerPacket *pkt) {
    if (pkt->code == AUTH_SUCCESS) {
        if (current_screen == SCREEN_LOGIN || current_screen == SCREEN_REGISTER) {
            current_screen = SCREEN_LOBBY_LIST;
        }
        send_packet(MSG_LIST_LOBBIES, 0); 
        
        // Save session token
        if (pkt->payload.auth.session_token[0] != '\0') {
            save_session_token(pkt->payload.auth.session_token);
        }
        
        strncpy(my_username, pkt->payload.auth.username, MAX_USERNAME); // Use server provided username
        status_message[0] = '\0';
        
        // Show welcome notification
        snprintf(notification_message, sizeof(notification_message), "Welcome, %s!", my_username);
        notification_time = SDL_GetTicks();
        
        printf("[CLIENT] Authenticated as: %s\n", my_username);
    } else {
        if (current_screen == SCREEN_LOGIN) { 
            // Only show errors if we are actually ON the login screen
            // (prevents auto-login failure from showing alert, it just stays on login)
            if (pkt->code == AUTH_FAIL && pkt->message[0] == '\0') {
                 strcpy(status_message, "Session expired");
            } else {
                 strncpy(status_message, pkt->message, sizeof(status_message));
            }
        } else {
            // Auto-login failed implicitly -> force to login screen
            current_screen = SCREEN_LOGIN; 
            clear_session_token();
        }
        if (pkt->code == AUTH_USER_EXISTS) {
            strncpy(status_message,
                    "Email and username are taken. Try another.",
                    sizeof(status_message));
        } else if (pkt->code == AUTH_USERNAME_EXISTS) {
            strncpy(status_message,
                    "Username is taken. Try another.",
                    sizeof(status_message));
        } else if (pkt->code == AUTH_EMAIL_EXISTS) {
            strncpy(status_message,
                    "Email is taken. Try another.",
                    sizeof(status_message));
        } else {
            strncpy(status_message, pkt->message, sizeof(status_message));
        }
    }
}

void handle_lobby_list(ServerPacket *pkt) {
    lobby_count = pkt->payload.lobby_list.count;
    memcpy(lobby_list, pkt->payload.lobby_list.lobbies, sizeof(lobby_list));
}

void handle_lobby_update(ServerPacket *pkt) {
    current_lobby = pkt->payload.lobby;
    
    // Show access code notification if we just created a private room
    static int last_lobby_id = -1;
    if (current_lobby.is_private && current_lobby.id != last_lobby_id && 
        strcmp(current_lobby.host_username, my_username) == 0) {
        snprintf(notification_message, sizeof(notification_message), 
                "Private room created! Code: %s", current_lobby.access_code);
        notification_time = SDL_GetTicks();
    }
    last_lobby_id = current_lobby.id;
    
    // Find my player ID
    my_player_id = -1;
    for (int i = 0; i < current_lobby.num_players; i++) {
        if (strcmp(current_lobby.players[i].username, my_username) == 0) {
            my_player_id = i;
            break;
        }
    }
    
    printf("[CLIENT] My player_id: %d, Host_id: %d\n", my_player_id, current_lobby.host_id);
    
    if (current_lobby.status == LOBBY_PLAYING) {
        if (current_screen != SCREEN_GAME) {
            add_notification("Game started!", (SDL_Color){0, 255, 0, 255});
            game_start_time = SDL_GetTicks();  // Start the timer
            post_match_shown = 0;  // Reset flag for new match
        }
        current_screen = SCREEN_GAME;
        memset(&current_state, 0, sizeof(GameState));
        memset(&previous_state, 0, sizeof(GameState));
        lobby_error_message[0] = '\0';
    } else {
        // Don't switch to lobby room if showing post-match screen
        if (current_screen != SCREEN_POST_MATCH) {
            current_screen = SCREEN_LOBBY_ROOM;
        }
    }
}

void handle_game_state(ServerPacket *pkt) {
    current_state = pkt->payload.game_state;
    check_game_changes();
    
    if (current_state.game_status == GAME_ENDED) {
        printf("\n╔═══════════════════════╗\n");
        printf("║      GAME ENDED!           ║\n");
        if (current_state.winner_id >= 0) {
            printf("║  Winner: %s\n", 
                   current_state.players[current_state.winner_id].username);
            
            char msg[128];
            if (current_state.winner_id == 0) {
                snprintf(msg, sizeof(msg), "Congratulations! You Win!");
                add_notification(msg, (SDL_Color){0, 255, 0, 255});
            } else {
                snprintf(msg, sizeof(msg), "%s has won!", 
                        current_state.players[current_state.winner_id].username);
                add_notification(msg, (SDL_Color){255, 215, 0, 255});
            }
        } else {
            printf("║      Draw!                 ║\n");
            add_notification("Match draw!", (SDL_Color){200, 200, 200, 255});
        }
        printf("╚═══════════════════════╝\n\n");
        
        // Switch to post-match screen (only once)
        if ((current_screen == SCREEN_GAME || current_screen == SCREEN_LOBBY_ROOM) && !post_match_shown) {
            current_screen = SCREEN_POST_MATCH;
            post_match_shown = 1;  // Mark as shown
            
            // Populate post-match data with REAL values
            post_match_winner_id = current_state.winner_id;
            post_match_duration = current_state.match_duration_seconds;
            printf("[CLIENT] Post-match data from server:\n");
            for (int i = 0; i < current_state.num_players && i < MAX_CLIENTS; i++) {
                post_match_elo_changes[i] = current_state.elo_changes[i];  // Real ELO changes!
                post_match_kills[i] = current_state.kills[i];  // Real kills!
                printf("[CLIENT]   Player %d: ELO change = %d, Kills = %d\n", 
                       i, post_match_elo_changes[i], post_match_kills[i]);
            }
            printf("[CLIENT]   Match duration: %d seconds\n", post_match_duration);
            
            printf("[CLIENT] Switched to post-match screen\n");
        }
    }
}

void handle_error(ServerPacket *pkt) {
    strncpy(status_message, pkt->message, sizeof(status_message));
    strncpy(lobby_error_message, pkt->message, sizeof(lobby_error_message));
}

void handle_friend_list_response(ServerPacket *pkt) {
    // Server sends: total count in payload.friend_list.count
    // code field bit-packed: low byte = pending_count, high byte = sent_count
    pending_count = pkt->code & 0xFF;  // Low byte
    sent_count = (pkt->code >> 8) & 0xFF;  // High byte
    friends_count = pkt->payload.friend_list.count - pending_count - sent_count;
    
    // Copy accepted friends (first part of array)
    if (friends_count > 0 && friends_count <= 50) {
        memcpy(friends_list, pkt->payload.friend_list.friends, 
               sizeof(FriendInfo) * friends_count);
    }
    
    // Copy pending requests (second part)
    if (pending_count > 0 && pending_count <= 50) {
        memcpy(pending_requests, &pkt->payload.friend_list.friends[friends_count], 
               sizeof(FriendInfo) * pending_count);
        
        // Show notification for new pending requests
        if (pending_count > 0) {
            snprintf(notification_message, sizeof(notification_message),
                    "You have %d pending friend request%s!", pending_count, pending_count > 1 ? "s" : "");
            notification_time = SDL_GetTicks();
        }
    }
    
    // Copy sent requests (third part)
    if (sent_count > 0 && sent_count <= 50) {
        memcpy(sent_requests, &pkt->payload.friend_list.friends[friends_count + pending_count],
                sizeof(FriendInfo) * sent_count);
    }
    
    printf("[CLIENT] Received %d friends, %d pending, %d sent \n", friends_count, pending_count, sent_count);
}

void handle_friend_response(ServerPacket *pkt) {
    if (pkt->code == 0) {
        // Success - show notification
        snprintf(notification_message, sizeof(notification_message),
                "Friend request sent!");
        notification_time = SDL_GetTicks();
        // Refresh friends list
        send_packet(MSG_FRIEND_LIST, 0);
    } else {
        // Error
        snprintf(notification_message, sizeof(notification_message),
                "Request failed");
        notification_time = SDL_GetTicks();
    }
}

void handle_profile_response(ServerPacket *pkt) {
    my_profile = pkt->payload.profile;
    printf("[CLIENT] Received profile: ELO %d, Matches %d\n", 
           my_profile.elo_rating, my_profile.total_matches);
}

void handle_leaderboard_response(ServerPacket *pkt) {
    leaderboard_count = pkt->payload.leaderboard.count;
    if (leaderboard_count > 100) leaderboard_count = 100;
    memcpy(leaderboard, pkt->payload.leaderboard.entries,
           sizeof(LeaderboardEntry) * leaderboard_count);
    printf("[CLIENT] Received %d leaderboard entries\n", leaderboard_count);
}

void handle_notification(ServerPacket *pkt) {
    // Handle notifications (including power-up caps during game)
    if (pkt->message[0] != '\0') {
        snprintf(notification_message, sizeof(notification_message), "%s", pkt->message);
        notification_time = SDL_GetTicks();
        printf("[CLIENT] Notification: %s\n", pkt->message);
    }
}

void handle_chat(ServerPacket *pkt) {
    // Store incoming chat message
    if (chat_count < MAX_CHAT_MESSAGES) {
        ChatMessage* msg = &chat_history[chat_count];
        strncpy(msg->sender, pkt->payload.chat_msg.sender_username, MAX_USERNAME - 1);
        msg->sender[MAX_USERNAME - 1] = '\0';
        strncpy(msg->message, pkt->payload.chat_msg.message, 199);
        msg->message[199] = '\0';
        msg->timestamp = SDL_GetTicks();
        msg->player_id = pkt->payload.chat_msg.player_id;
        msg->is_current_user = (strcmp(msg->sender, my_username) == 0) ? 1 : 0;
        chat_count++;
    } else {
        // Shift array and add new message (FIFO)
        for (int i = 0; i < MAX_CHAT_MESSAGES - 1; i++) {
            chat_history[i] = chat_history[i + 1];
        }
        ChatMessage* msg = &chat_history[MAX_CHAT_MESSAGES - 1];
        strncpy(msg->sender, pkt->payload.chat_msg.sender_username, MAX_USERNAME - 1);
        msg->sender[MAX_USERNAME - 1] = '\0';
        strncpy(msg->message, pkt->payload.chat_msg.message, 199);
        msg->message[199] = '\0';
        msg->timestamp = SDL_GetTicks();
        msg->player_id = pkt->payload.chat_msg.player_id;
        msg->is_current_user = (strcmp(msg->sender, my_username) == 0) ? 1 : 0;
    }
    
    printf("[CLIENT] Chat - %s: %s\n", pkt->payload.chat_msg.sender_username, pkt->payload.chat_msg.message);
}

void handle_invite_received(ServerPacket *pkt) {
    current_invite.lobby_id = pkt->payload.invite.lobby_id;
    strncpy(current_invite.room_name, pkt->payload.invite.room_name, 63);
    strncpy(current_invite.host_name, pkt->payload.invite.host_name, 31);
    strncpy(current_invite.access_code, pkt->payload.invite.access_code, 7);
    current_invite.game_mode = pkt->payload.invite.game_mode;
    current_invite.is_active = 1;

    snprintf(notification_message, sizeof(notification_message), 
            "Game Invite from %s!", current_invite.host_name);
    notification_time = SDL_GetTicks();
    printf("[CLIENT] Received invite to Lobby %d from %s (Code: %s)\n", 
           current_invite.lobby_id, current_invite.host_name, current_invite.access_code);
}
