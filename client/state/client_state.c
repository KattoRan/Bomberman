#include "client_state.h"
#include "../ui/ui.h"

ScreenState current_screen = SCREEN_LOGIN;

int sock;
int my_player_id = -1;
char my_username[MAX_USERNAME];
char status_message[256] = "";
char lobby_error_message[256] = "";
Uint32 error_message_time = 0;

// Data Store
LobbySummary lobby_list[MAX_LOBBIES];
int lobby_count = 0;
int selected_lobby_idx = -1;
Lobby current_lobby;

// Game State (shared with graphics.c)
GameState current_state;
GameState previous_state;  // Để theo dõi thay đổi

// Friends, Profile, Leaderboard Data
FriendInfo friends_list[50];
int friends_count = 0;
FriendInfo pending_requests[50];
int pending_count = 0;
FriendInfo sent_requests[50];  // Outgoing requests
int sent_count = 0;
ProfileData my_profile = {0};  // Initialize to zero
LeaderboardEntry leaderboard[100];
int leaderboard_count = 0;

// Login/Register inputs
InputField inp_user  = {{335, 240, 450, 65}, "", "Username:", 0, 30};
InputField inp_email = {{335, 340, 450, 65}, "", "Email:",    0, 127};
InputField inp_pass  = {{335, 440, 450, 65}, "", "Password:", 0, 30};
Button btn_login = {{335, 560, 180, 60}, "Login",    0, BTN_PRIMARY};
Button btn_reg   = {{605, 560, 180, 60}, "Register", 0, BTN_PRIMARY};

// Lobby list buttons
Button btn_create     = {{250, 620, 200, 60}, "Create Room", 0 , BTN_PRIMARY};
Button btn_refresh    = {{460, 620, 200, 60}, "Refresh", 0 , BTN_PRIMARY};
Button btn_friends    = {{670, 620, 200, 60}, "Friends", 0, BTN_PRIMARY};
Button btn_profile     = {{850, 20, 130, 50}, "Profile", 0, BTN_PRIMARY};
Button btn_leaderboard = {{1000, 20, 80, 50}, "Top", 0, BTN_OUTLINE};
Button btn_logout        = {{40, 20, 120, 50}, "Logout", 0, BTN_DANGER};

// Lobby room buttons
Button btn_ready = {{80, 630, 200, 50}, "Ready", 0, BTN_PRIMARY};
Button btn_start = {{80, 630, 200, 50}, "Start Game", 0, BTN_PRIMARY};
Button btn_leave = {{320, 630, 200, 50}, "Leave", 0, BTN_DANGER};

// Game mode selection
int selected_game_mode = 0;  // 0=Classic, 1=Sudden Death, 2=Fog of War

// Room creation UI - adjusted for 1120x720
InputField inp_room_name   = {{335, 260, 450, 60}, "", "Room Name:", 0, 63};
InputField inp_access_code = {{335, 350, 350, 60}, "", "Access Code (6 digits, optional):", 0, 6};
int show_create_room_dialog = 0;
int creating_private_room = 0;
Button btn_create_confirm = {{370, 440, 220, 60}, "Create", 0, BTN_PRIMARY};
Button btn_cancel         = {{620, 440, 220, 60}, "Cancel", 0, BTN_DANGER};

// Notification system
char notification_message[256] = "";
Uint32 notification_time = 0;
const int NOTIFICATION_DURATION = 3000; // 3 seconds

// Access code prompt for joining private rooms - adjusted for 1120x720
InputField inp_join_code = {{335, 320, 450, 60}, "", "Enter 6-digit access code:", 0, 6};
int show_join_code_dialog = 0;
int selected_private_lobby_id = -1;

// Friend request UI - adjusted for 1120x720
InputField inp_friend_request = {{400, 620, 360, 60}, "", "Enter display name...", 0, 31};
Button btn_send_friend_request = {{800, 620, 220, 60}, "Send Request", 0};

// Delete friend confirmation
int show_delete_confirm = 0;
int delete_friend_index = -1;

// Post-match screen state  
int post_match_winner_id = -1;
int post_match_elo_changes[4] = {0, 0, 0, 0};
int post_match_kills[4] = {0, 0, 0, 0};
int post_match_duration = 0;  // Match duration in seconds
int post_match_shown = 0;  // Prevent showing multiple times
Button btn_rematch       = {{360, 800, 250, 60}, "Rematch", 0};
Button btn_return_lobby  = {{610, 800, 250, 60}, "Back to Room", 0};

// Game timer tracking
Uint32 game_start_time = 0;  // SDL ticks when game started

ChatMessage chat_history[MAX_CHAT_MESSAGES];
int chat_count = 0;
int chat_panel_open = 0;  // Toggle for gameplay (0=mini, 1=full)
InputField inp_chat_message = {{0, 0, 600, 40}, "", "", 0, 199};  // Max 199 chars + null

// --- Invite System State ---
IncomingInvite current_invite = {0};
int show_invite_overlay = 0;
int invited_user_ids[50];
int invited_count = 0;

// Buttons for Invite System
Button btn_open_invite = {{910, 15, 180, 50}, "Invite Friend", 0, BTN_PRIMARY};
Button btn_close_invite = {{0, 0, 40, 40}, "X", 0, BTN_DANGER}; // Positioned dynamically
Button btn_invite_accept = {{0, 0, 150, 50}, "Accept", 0, BTN_PRIMARY};
Button btn_invite_decline = {{0, 0, 150, 50}, "Decline", 0, BTN_DANGER};

void init_client_state() {
    current_screen = SCREEN_LOGIN;
    my_player_id = -1;
    memset(my_username, 0, sizeof(my_username));
    memset(status_message, 0, sizeof(status_message));
    memset(lobby_error_message, 0, sizeof(lobby_error_message));
    lobby_count = 0;
    selected_lobby_idx = -1;
    friends_count = 0;
    pending_count = 0;
    sent_count = 0;
    leaderboard_count = 0;
    chat_count = 0;
    invited_count = 0;
    post_match_shown = 0;
}

void reset_client_state() {
    init_client_state();
}