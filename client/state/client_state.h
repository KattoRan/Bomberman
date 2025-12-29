/* client/state/client_state.h */
#ifndef CLIENT_STATE_H
#define CLIENT_STATE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "../common/protocol.h"
#include "../ui/ui.h"

// Screen states
typedef enum {
    SCREEN_LOGIN,
    SCREEN_REGISTER,
    SCREEN_LOBBY_LIST,
    SCREEN_LOBBY_ROOM,
    SCREEN_GAME,
    SCREEN_FRIENDS,
    SCREEN_PROFILE,
    SCREEN_LEADERBOARD,
    SCREEN_POST_MATCH
} ScreenState;

// External declarations for global state
extern ScreenState current_screen;
extern int sock;
extern int my_player_id;
extern char my_username[MAX_USERNAME];
extern char status_message[256];
extern char lobby_error_message[256];
extern Uint32 error_message_time;

// Data Store
extern LobbySummary lobby_list[MAX_LOBBIES];
extern int lobby_count;
extern int selected_lobby_idx;
extern Lobby current_lobby;

extern GameState current_state;
extern GameState previous_state;

// Friends, Profile, Leaderboard Data
extern FriendInfo friends_list[50];
extern int friends_count;
extern FriendInfo pending_requests[50];
extern int pending_count;
extern FriendInfo sent_requests[50];
extern int sent_count;
extern ProfileData my_profile;
extern LeaderboardEntry leaderboard[100];
extern int leaderboard_count;

// UI Components
extern InputField inp_user;
extern InputField inp_email;
extern InputField inp_pass;
extern Button btn_login;
extern Button btn_reg;

// Lobby buttons
extern Button btn_create;
extern Button btn_refresh;
extern Button btn_friends;
extern Button btn_profile;
extern Button btn_leaderboard;
extern Button btn_logout;

// Lobby room buttons
extern Button btn_ready;
extern Button btn_start;
extern Button btn_leave;

// Game mode selection
extern int selected_game_mode;

// Room creation UI
extern InputField inp_room_name;
extern InputField inp_access_code;
extern int show_create_room_dialog;
extern int creating_private_room;
extern Button btn_create_confirm;
extern Button btn_cancel;

// Notification system
extern char notification_message[256];
extern Uint32 notification_time;
extern const Uint32 NOTIFICATION_DURATION;

// Access code prompt
extern InputField inp_join_code;
extern int show_join_code_dialog;
extern int selected_private_lobby_id;

// Friend request UI
extern InputField inp_friend_request;
extern Button btn_send_friend_request;

// Delete friend confirmation
extern int show_delete_confirm;
extern int delete_friend_index;

// Post-match screen state
extern int post_match_winner_id;
extern int post_match_elo_changes[4];
extern int post_match_kills[4];
extern int post_match_duration;
extern int post_match_shown;
extern Button btn_rematch;
extern Button btn_return_lobby;

// Game timer
extern Uint32 game_start_time;

// Chat system
#define MAX_CHAT_MESSAGES 50
typedef struct {
    char sender[MAX_USERNAME];
    char message[200];
    Uint32 timestamp;
    int player_id;
    int is_current_user;
} ChatMessage;

extern ChatMessage chat_history[MAX_CHAT_MESSAGES];
extern int chat_count;
extern int chat_panel_open;
extern InputField inp_chat_message;

extern IncomingInvite current_invite;
extern int show_invite_overlay;
extern int invited_user_ids[50];
extern int invited_count;

extern Button btn_open_invite;
extern Button btn_close_invite;
extern Button btn_invite_accept;
extern Button btn_invite_decline;

// --- Session Persistence ---
extern char session_file_path[256];

// Functions
void init_client_state();
// Global running flag
extern int running;

void reset_client_state();

#endif