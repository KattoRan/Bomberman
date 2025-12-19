/* client/main.c - With Notifications System */
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "../common/protocol.h"
#include "ui.h" 

extern void render_game(SDL_Renderer*, TTF_Font*, int, int, int);
extern TTF_Font* init_font();
extern void add_notification(const char *text, SDL_Color color);

// State
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

ScreenState current_screen = SCREEN_LOGIN;

int sock;
int my_player_id = -1;
char my_username[MAX_USERNAME];
char status_message[256] = "";
char lobby_error_message[256] = "";
Uint32 error_message_time = 0;

// Data Store
Lobby lobby_list[MAX_LOBBIES];
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

// UI Components - UPDATED SIZES AND POSITIONS
// Login/Register inputs - centered, larger
InputField inp_user = {{685, 350, 550, 65}, "", "Username:", 0, 30};
InputField inp_email = {{685, 450, 550, 65}, "", "Email:", 0, 127};
InputField inp_pass = {{685, 550, 550, 65}, "", "Password:", 0, 30};
Button btn_login = {{760, 680, 180, 60}, "Login", 0};
Button btn_reg = {{970, 680, 180, 60}, "Register", 0};

// Lobby list buttons - BOTTOM CENTER, larger
Button btn_create = {{500, 940, 280, 70}, "Create Room", 0};
Button btn_refresh = {{820, 940, 280, 70}, "Refresh", 0};
Button btn_friends = {{1140, 940, 280, 70}, "Friends", 0};
Button btn_quick_play = {{1460, 940, 280, 70}, "Quick Play", 0};  // NEW
Button btn_profile = {{1620, 20, 130, 55}, "Profile", 0};
Button btn_leaderboard = {{1765, 20, 130, 55}, "Top", 0};

// Lobby room buttons - RAISED for better positioning (Y: 940->910)
Button btn_ready = {{540, 910, 280, 70}, "Ready", 0};
Button btn_start = {{430, 910, 340, 70}, "Start Game", 0};
Button btn_leave = {{860, 910, 280, 70}, "Leave", 0};

// Lobby room - host-only buttons
Button btn_lock_room = {{1620, 100, 180, 50}, "Lock Room", 0};
Button btn_chat = {{1620, 170, 180, 50}, "Chat", 0};
// Kick buttons will be positioned per-player dynamically

// Game mode selection
int selected_game_mode = 0;  // 0=Classic, 1=Sudden Death, 2=Fog of War

// Room creation UI - larger
InputField inp_room_name = {{685, 380, 550, 60}, "", "Room Name:", 0, 63};
InputField inp_access_code = {{685, 480, 450, 60}, "", "Access Code (6 digits, optional):", 0, 6};
int show_create_room_dialog = 0;
int creating_private_room = 0;
Button btn_create_confirm = {{0, 0, 220, 60}, "Create", 0};
Button btn_cancel = {{0, 0, 220, 60}, "Cancel", 0};

// Notification system
char notification_message[256] = "";
Uint32 notification_time = 0;
const int NOTIFICATION_DURATION = 3000; // 3 seconds

// Access code prompt for joining private rooms - larger
InputField inp_join_code = {{685, 450, 550, 60}, "", "Enter 6-digit access code:", 0, 6};
int show_join_code_dialog = 0;
int selected_private_lobby_id = -1;

// Friend request UI - fixed spacing and button width
InputField inp_friend_request = {{685, 870, 350, 60}, "", "Enter display name...", 0, 31};
Button btn_send_friend_request = {{1060, 870, 220, 60}, "Send Request", 0};  // W: 200->220 to fit text

// Delete friend confirmation
int show_delete_confirm = 0;
int delete_friend_index = -1;

// Settings screen state
int settings_active_tab = 0;  // 0=Graphics, 1=Controls, 2=Account
Button btn_settings = {{20, 940, 200, 60}, "Settings", 0};
Button btn_settings_apply = {{0, 0, 200, 60}, "Apply", 0};

// Post-match screen state  
int post_match_winner_id = -1;
int post_match_elo_changes[4] = {0, 0, 0, 0};
int post_match_kills[4] = {0, 0, 0, 0};
int post_match_duration = 0;  // Match duration in seconds
int post_match_shown = 0;  // Prevent showing multiple times
Button btn_rematch = {{0, 0, 250, 60}, "Rematch", 0};
Button btn_return_lobby = {{0, 0, 250, 60}, "Return", 0};

// Game timer tracking
Uint32 game_start_time = 0;  // SDL ticks when game started

// --- Helper Functions ---

int is_mouse_inside(SDL_Rect rect, int mx, int my) {
    return (mx >= rect.x && mx <= rect.x + rect.w && my >= rect.y && my <= rect.y + rect.h);
}

void handle_text_input(InputField *field, char c) {
    if (!field->is_active) return;
    int len = strlen(field->text);
    if (c == '\b') {
        if (len > 0) field->text[len-1] = '\0';
    } else if (len < field->max_length) {
        field->text[len] = c;
        field->text[len+1] = '\0';
    }
}

int find_my_player_id(Lobby *lobby, const char *username) {
    for (int i = 0; i < lobby->num_players; i++) {
        if (strcmp(lobby->players[i].username, username) == 0) {
            return i;
        }
    }
    return -1;
}

int all_players_ready(Lobby *lobby) {
    for (int i = 0; i < lobby->num_players; i++) {
        if (!lobby->players[i].is_ready) return 0;
    }
    return 1;
}

// Kiểm tra sự thay đổi trong game state
void check_game_changes() {
    // Kiểm tra người chơi chết
    for (int i = 0; i < current_state.num_players; i++) {
        if (previous_state.players[i].is_alive && !current_state.players[i].is_alive) {
            char msg[128];
            snprintf(msg, sizeof(msg), "%s has been defeated!", 
                    current_state.players[i].username);
            add_notification(msg, (SDL_Color){255, 68, 68, 255});
        }
        
        // Kiểm tra power-up
        if (current_state.players[i].max_bombs > previous_state.players[i].max_bombs) {
            if (i == my_player_id) { // Chỉ thông báo cho người chơi hiện tại
                add_notification("Picked up BOMB power-up! +1 Bomb", 
                               (SDL_Color){255, 215, 0, 255});
            }
        }
        
        if (current_state.players[i].bomb_range > previous_state.players[i].bomb_range) {
            if (i == my_player_id) {
                add_notification("Picked up FIRE power-up! +1 Blast Range", 
                               (SDL_Color){255, 69, 0, 255});
            }
        }
    }
    
    // Cập nhật previous state
    memcpy(&previous_state, &current_state, sizeof(GameState));
}

// --- Network packet receiving ---
int receive_server_packet(ServerPacket *out_packet) {
    static char buffer[sizeof(ServerPacket)];
    static int bytes_received = 0;
    
    int n = recv(sock, buffer + bytes_received, 
                 sizeof(ServerPacket) - bytes_received, MSG_DONTWAIT);
    
    if (n > 0) {
        bytes_received += n;
        
        if (bytes_received == sizeof(ServerPacket)) {
            memcpy(out_packet, buffer, sizeof(ServerPacket));
            bytes_received = 0;
            return 1;
        }
    } else if (n == 0) {
        return -1;
    }
    
    return 0;
}

// --- Network Functions ---
void send_packet(int type, int data) {
    ClientPacket pkt;
    memset(&pkt, 0, sizeof(pkt));
    pkt.type = type;
    pkt.data = data;
    strncpy(pkt.username, my_username, MAX_USERNAME);
    send(sock, &pkt, sizeof(pkt), 0);
}

void process_server_packet(ServerPacket *pkt) {
    switch (pkt->type) {
        case MSG_AUTH_RESPONSE:
            if (pkt->code == AUTH_SUCCESS) {
                current_screen = SCREEN_LOBBY_LIST;
                send_packet(MSG_LIST_LOBBIES, 0); 
                strncpy(my_username, inp_user.text, MAX_USERNAME);
                status_message[0] = '\0';
                
                // Show welcome notification
                snprintf(notification_message, sizeof(notification_message), "Welcome, %s!", my_username);
                notification_time = SDL_GetTicks();
                
                printf("[CLIENT] Authenticated as: %s\n", my_username);
            } else {
                strncpy(status_message, pkt->message, sizeof(status_message));
            }
            break;

        case MSG_LOBBY_LIST:
            lobby_count = pkt->payload.lobby_list.count;
            memcpy(lobby_list, pkt->payload.lobby_list.lobbies, sizeof(lobby_list));
            break;

        case MSG_LOBBY_UPDATE:
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
                    add_notification("Game đã bắt đầu!", (SDL_Color){0, 255, 0, 255});
                    game_start_time = SDL_GetTicks();  // Start the timer
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
            break;

        case MSG_GAME_STATE:
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
                        snprintf(msg, sizeof(msg), "🎉 Chúc mừng! Bạn đã chiến thắng! 🎉");
                        add_notification(msg, (SDL_Color){0, 255, 0, 255});
                    } else {
                        snprintf(msg, sizeof(msg), "%s đã chiến thắng!", 
                                current_state.players[current_state.winner_id].username);
                        add_notification(msg, (SDL_Color){255, 215, 0, 255});
                    }
                } else {
                    printf("║      Draw!                 ║\n");
                    add_notification("Trận đấu hòa!", (SDL_Color){200, 200, 200, 255});
                }
                printf("╚═══════════════════════╝\n\n");
                
                // Switch to post-match screen (only once)
                if (current_screen == SCREEN_GAME && !post_match_shown) {
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
            break;
            
        case MSG_ERROR:
            strncpy(status_message, pkt->message, sizeof(status_message));
            strncpy(lobby_error_message, pkt->message, sizeof(lobby_error_message));
            break;
            
        case MSG_FRIEND_LIST_RESPONSE:
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
            break;
        
        case MSG_FRIEND_RESPONSE:
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
            break;
            
        case MSG_PROFILE_RESPONSE:
            my_profile = pkt->payload.profile;
            printf("[CLIENT] Received profile: ELO %d, Matches %d\n", 
                   my_profile.elo_rating, my_profile.total_matches);
            break;
            
        case MSG_LEADERBOARD_RESPONSE:
            leaderboard_count = pkt->payload.leaderboard.count;
            if (leaderboard_count > 100) leaderboard_count = 100;
            memcpy(leaderboard, pkt->payload.leaderboard.entries,
                   sizeof(LeaderboardEntry) * leaderboard_count);
            printf("[CLIENT] Received %d leaderboard entries\n", leaderboard_count);
            break;
            
        case MSG_NOTIFICATION:
            // Handle notifications (including power-up caps during game)
            if (pkt->message[0] != '\0') {
                snprintf(notification_message, sizeof(notification_message), "%s", pkt->message);
                notification_time = SDL_GetTicks();
                printf("[CLIENT] Notification: %s\n", pkt->message);
            }
            break;
    }
}

// --- Main Client ---

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    // 1. Setup Network
    sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
    
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Cannot connect to server\n");
        return -1;
    }
    
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    
    printf("Connected to server!\n");

    // 2. Setup SDL - FULLSCREEN 1920x1080
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    SDL_Window *win = SDL_CreateWindow("Bomberman", 
                                       SDL_WINDOWPOS_CENTERED, 
                                       SDL_WINDOWPOS_CENTERED, 
                                       1920, 1080, 
                                       SDL_WINDOW_SHOWN);
    SDL_Renderer *rend = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!win) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return -1;
    }

    if (!rend) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return -1;
    }
    // LARGER FONTS for better readability
    TTF_Font *font_large = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 64);
    if (!font_large) {
        font_large = TTF_OpenFont("C:\\Windows\\Fonts\\arialbd.ttf", 64);
    }
    
    // font_small now uses 26pt (updated in init_font)
    TTF_Font *font_small = init_font();
    
    if (!font_small) {
        printf("Failed to load font!\n");
        return -1;
    }

    SDL_StartTextInput();
    int running = 1;
    int tick = 0;

    while (running) {
        SDL_Event e;
        int mx, my;
        SDL_GetMouseState(&mx, &my);

        // --- Event Handling ---
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;

            switch (current_screen) {
                case SCREEN_LOGIN:
                    if (e.type == SDL_MOUSEBUTTONDOWN) {
                        inp_user.is_active = is_mouse_inside(inp_user.rect, mx, my);
                        inp_pass.is_active = is_mouse_inside(inp_pass.rect, mx, my);
                        
                        if (is_mouse_inside(btn_login.rect, mx, my)) {
                            if (strlen(inp_user.text) > 0 && strlen(inp_pass.text) > 0) {
                                ClientPacket pkt;
                                memset(&pkt, 0, sizeof(pkt));
                                pkt.type = MSG_LOGIN;
                                strcpy(pkt.username, inp_user.text);
                                strcpy(pkt.password, inp_pass.text);
                                send(sock, &pkt, sizeof(pkt), 0);
                            } else {
                                strncpy(status_message, "Please enter username and password", sizeof(status_message));
                            }
                        }
                        
                        if (is_mouse_inside(btn_reg.rect, mx, my)) {
                            // Switch to registration screen
                            current_screen = SCREEN_REGISTER;
                            inp_user.text[0] = '\0';
                            inp_email.text[0] = '\0';
                            inp_pass.text[0] = '\0';
                            inp_user.is_active = 1;
                            inp_email.is_active = 0;
                            inp_pass.is_active = 0;
                            status_message[0] = '\0';
                        }
                    }
                    
                    if (e.type == SDL_TEXTINPUT) {
                        if (inp_user.is_active) handle_text_input(&inp_user, e.text.text[0]);
                        if (inp_pass.is_active) handle_text_input(&inp_pass, e.text.text[0]);
                    }
                    
                    if (e.type == SDL_KEYDOWN) {
                        if (e.key.keysym.sym == SDLK_BACKSPACE) {
                            if (inp_user.is_active) handle_text_input(&inp_user, '\b');
                            if (inp_pass.is_active) handle_text_input(&inp_pass, '\b');
                        }
                        if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER) {
                            if (strlen(inp_user.text) > 0 && strlen(inp_pass.text) > 0) {
                                ClientPacket pkt;
                                memset(&pkt, 0, sizeof(pkt));
                                pkt.type = MSG_LOGIN;
                                strcpy(pkt.username, inp_user.text);
                                strcpy(pkt.password, inp_pass.text);
                                send(sock, &pkt, sizeof(pkt), 0);
                            }
                        }
                        if (e.key.keysym.sym == SDLK_TAB) {
                            if (inp_user.is_active) {
                                inp_user.is_active = 0;
                                inp_pass.is_active = 1;
                            } else if (inp_pass.is_active) {
                                inp_pass.is_active = 0;
                                inp_user.is_active = 1;
                            } else {
                                inp_user.is_active = 1;
                            }
                        }
                    }
                    break;
                    
                case SCREEN_REGISTER:
                    if (e.type == SDL_MOUSEBUTTONDOWN) {
                        inp_user.is_active = is_mouse_inside(inp_user.rect, mx, my);
                        inp_email.is_active = is_mouse_inside(inp_email.rect, mx, my);
                        inp_pass.is_active = is_mouse_inside(inp_pass.rect, mx, my);
                        
                        if (is_mouse_inside(btn_reg.rect, mx, my)) {
                            if (strlen(inp_user.text) > 0 && strlen(inp_email.text) > 0 && strlen(inp_pass.text) > 0) {
                                ClientPacket pkt;
                                memset(&pkt, 0, sizeof(pkt));
                                pkt.type = MSG_REGISTER;
                                strcpy(pkt.username, inp_user.text);
                                strcpy(pkt.email, inp_email.text);
                                strcpy(pkt.password, inp_pass.text);
                                send(sock, &pkt, sizeof(pkt), 0);
                            } else {
                                strncpy(status_message, "Please fill all fields", sizeof(status_message));
                            }
                        }
                        
                        if (is_mouse_inside(btn_login.rect, mx, my)) {
                            // Back to login screen
                            current_screen = SCREEN_LOGIN;
                            inp_user.text[0] = '\0';
                            inp_email.text[0] = '\0';
                            inp_pass.text[0] = '\0';
                            inp_user.is_active = 1;
                            inp_email.is_active = 0;
                            inp_pass.is_active = 0;
                            status_message[0] = '\0';
                        }
                    }
                    
                    if (e.type == SDL_TEXTINPUT) {
                        if (inp_user.is_active) handle_text_input(&inp_user, e.text.text[0]);
                        if (inp_email.is_active) handle_text_input(&inp_email, e.text.text[0]);
                        if (inp_pass.is_active) handle_text_input(&inp_pass, e.text.text[0]);
                    }
                    
                    if (e.type == SDL_KEYDOWN) {
                        if (e.key.keysym.sym == SDLK_BACKSPACE) {
                            if (inp_user.is_active) handle_text_input(&inp_user, '\b');
                            if (inp_email.is_active) handle_text_input(&inp_email, '\b');
                            if (inp_pass.is_active) handle_text_input(&inp_pass, '\b');
                        }
                        
                        if (e.key.keysym.sym == SDLK_TAB) {
                            if (inp_user.is_active) {
                                inp_user.is_active = 0;
                                inp_email.is_active = 1;
                            } else if (inp_email.is_active) {
                                inp_email.is_active = 0;
                                inp_pass.is_active = 1;
                            } else if (inp_pass.is_active) {
                                inp_pass.is_active = 0;
                                inp_user.is_active = 1;
                            } else {
                                inp_user.is_active = 1;
                            }
                        }
                    }
                    break;
                    
                case SCREEN_LOBBY_LIST:
                    if (e.type == SDL_MOUSEBUTTONDOWN) {
                        if (is_mouse_inside(btn_create.rect, mx, my)) {
                            // Show create room dialog
                            show_create_room_dialog = 1;
                            snprintf(inp_room_name.text, sizeof(inp_room_name.text), "Room %d", lobby_count + 1);
                            inp_access_code.text[0] = '\0';
                            inp_room_name.is_active = 1;
                            inp_access_code.is_active = 0;
                        }
                        if (is_mouse_inside(btn_refresh.rect, mx, my)) {
                            send_packet(MSG_LIST_LOBBIES, 0);
                        }
                        if (is_mouse_inside(btn_friends.rect, mx, my)) {
                            // Request friends list from server
                            send_packet(MSG_FRIEND_LIST, 0);
                            current_screen = SCREEN_FRIENDS;
                        }
                        if (is_mouse_inside(btn_profile.rect, mx, my)) {
                            // Request own profile
                            send_packet(MSG_GET_PROFILE, 0);
                            current_screen = SCREEN_PROFILE;
                        }
                        if (is_mouse_inside(btn_leaderboard.rect, mx, my)) {
                            // Request leaderboard
                            send_packet(MSG_GET_LEADERBOARD, 0);
                            current_screen = SCREEN_LEADERBOARD;
                        }
                        if (is_mouse_inside(btn_quick_play.rect, mx, my)) {
                            // Quick Play - show notification (backend not implemented)
                            snprintf(notification_message, sizeof(notification_message),
                                    "Quick Play matchmaking coming soon!");
                            notification_time = SDL_GetTicks();
                        }
                        if (is_mouse_inside(btn_settings.rect, mx, my)) {
                            // Open settings screen
                            current_screen = SCREEN_SETTINGS;
                            settings_active_tab = 0;  // Default to Graphics tab
                        }
                        
                        // Lobby click detection - MUST MATCH ui_screens.c coordinates
                        int list_width = 1056;  // Fixed width from ui_screens.c
                        int start_x = 432;      // Centered: (1920-1056)/2
                        int y = 162;            // Fixed from top (matches rendering)
                        int card_height = 75;   // Card height from ui_screens.c
                        
                        for (int i = 0; i < lobby_count; i++) {
                            SDL_Rect r = {start_x, y, list_width, card_height};
                            if (is_mouse_inside(r, mx, my)) {
                                // Check if private room
                                if (lobby_list[i].is_private) {
                                    // Show access code prompt
                                    selected_private_lobby_id = lobby_list[i].id;
                                    show_join_code_dialog = 1;
                                    inp_join_code.text[0] = '\0';
                                    inp_join_code.is_active = 1;
                                } else {
                                    // Join public room directly
                                    ClientPacket pkt;
                                    memset(&pkt, 0, sizeof(pkt));
                                    pkt.type = MSG_JOIN_LOBBY;
                                    pkt.lobby_id = lobby_list[i].id;
                                    pkt.access_code[0] = '\0';
                                    send(sock, &pkt, sizeof(pkt), 0);
                                }
                                selected_lobby_idx = i;
                                break;
                            }
                            y += 85;  // Card spacing (75 + 10)
                        }
                    }
                    
                    // Dialog event handling
                    if (show_create_room_dialog) {
                        if (e.type == SDL_MOUSEBUTTONDOWN) {
                            inp_room_name.is_active = is_mouse_inside(inp_room_name.rect, mx, my);
 inp_access_code.is_active = is_mouse_inside(inp_access_code.rect, mx, my);
                            
                            // Game mode buttons - calculate positions from render_create_room_dialog
                            int win_w, win_h;
                            SDL_GetRendererOutputSize(rend, &win_w, &win_h);
                            int dialog_w = 700;
                            int dialog_h = 650;
                            int dialog_x = (win_w - dialog_w) / 2;
                            int dialog_y = (win_h - dialog_h) / 2;
                            int mode_y = dialog_y + 390;
                            int btn_width = 180;
                            int btn_spacing = 20;
                            
                            // Check each game mode button
                            for (int i = 0; i < 3; i++) {
                                int btn_x = dialog_x + 75 + i * (btn_width + btn_spacing);
                                SDL_Rect mode_btn = {btn_x, mode_y, btn_width, 50};
                                if (is_mouse_inside(mode_btn, mx, my)) {
                                    selected_game_mode = i;  // 0=Classic, 1=Sudden Death, 2=Fog of War
                                    printf("[CLIENT] Selected game mode: %d\n", selected_game_mode);
                                    break;
                                }
                            }
                            
                            // Random button - generates 6-digit code
                            SDL_Rect btn_random = {inp_access_code.rect.x + inp_access_code.rect.w + 10, 
                                                  inp_access_code.rect.y, 80, inp_access_code.rect.h};
                            if (is_mouse_inside(btn_random, mx, my)) {
                                // Generate random 6-digit code (100000-999999)
                                int random_code = 100000 + (rand() % 900000);
                                snprintf(inp_access_code.text, sizeof(inp_access_code.text), "%d", random_code);
                            }
                            
                            if (is_mouse_inside(btn_create_confirm.rect, mx, my)) {
                                // Validate access code if provided
                                int code_len = strlen(inp_access_code.text);
                                int is_valid = 1;
                                char error_msg[128] = "";
                                
                                if (code_len > 0) {
                                    // Check if exactly 6 digits
                                    if (code_len != 6) {
                                        is_valid = 0;
                                        snprintf(error_msg, sizeof(error_msg), 
                                                "Access code must be exactly 6 digits! (Currently: %d)", code_len);
                                    } else {
                                        // Check if all characters are digits
                                        for (int i = 0; i < code_len; i++) {
                                            if (inp_access_code.text[i] < '0' || inp_access_code.text[i] > '9') {
                                                is_valid = 0;
                                                strcpy(error_msg, "Access code must contain only numbers!");
                                                break;
                                            }
                                        }
                                    }
                                }
                                
                                if (is_valid) {
                                    ClientPacket pkt;
                                    memset(&pkt, 0, sizeof(pkt));
                                    pkt.type = MSG_CREATE_LOBBY;
                                    strncpy(pkt.room_name, inp_room_name.text, MAX_ROOM_NAME - 1);
                                    pkt.is_private = (code_len == 6) ? 1 : 0;
                                    pkt.game_mode = selected_game_mode;  // Send selected game mode
                                    if (pkt.is_private) {
                                        strncpy(pkt.access_code, inp_access_code.text, 7);
                                    }
                                    send(sock, &pkt, sizeof(pkt), 0);
                                    show_create_room_dialog = 0;
                                } else {
                                    // Show error notification
                                    snprintf(notification_message, sizeof(notification_message), "%s", error_msg);
                                    notification_time = SDL_GetTicks();
                                }
                            }
                            if (is_mouse_inside(btn_cancel.rect, mx, my)) {
                                show_create_room_dialog = 0;
                            }
                        }
                        
                        if (e.type == SDL_TEXTINPUT) {
                            if (inp_room_name.is_active) handle_text_input(&inp_room_name, e.text.text[0]);
                            if (inp_access_code.is_active && e.text.text[0] >= '0' && e.text.text[0] <= '9') {
                                handle_text_input(&inp_access_code, e.text.text[0]);
                            }
                        }
                        
                        if (e.type == SDL_KEYDOWN) {
                            if (e.key.keysym.sym == SDLK_BACKSPACE) {
                                if (inp_room_name.is_active) handle_text_input(&inp_room_name, '\b');
                                if (inp_access_code.is_active) handle_text_input(&inp_access_code, '\b');
                            }
                            if (e.key.keysym.sym == SDLK_ESCAPE) show_create_room_dialog = 0;
                        }
                    }
                    
                    // Join code dialog handling
                    if (show_join_code_dialog && e.type == SDL_MOUSEBUTTONDOWN) {
                        inp_join_code.is_active = is_mouse_inside(inp_join_code.rect, mx, my);
                        
                        if (is_mouse_inside(btn_create_confirm.rect, mx, my)) {
                            if (strlen(inp_join_code.text) == 6) {
                                ClientPacket pkt;
                                memset(&pkt, 0, sizeof(pkt));
                                pkt.type = MSG_JOIN_LOBBY;
                                pkt.lobby_id = selected_private_lobby_id;
                                strncpy(pkt.access_code, inp_join_code.text, 7);
                                send(sock, &pkt, sizeof(pkt), 0);
                                show_join_code_dialog = 0;
                            }
                        }
                        if (is_mouse_inside(btn_cancel.rect, mx, my)) {
                            show_join_code_dialog = 0;
                        }
                    }
                    
                    if (show_join_code_dialog && e.type == SDL_TEXTINPUT && inp_join_code.is_active) {
                        if (e.text.text[0] >= '0' && e.text.text[0] <= '9') {
                            handle_text_input(&inp_join_code, e.text.text[0]);
                        }
                    }
                    
                    if (show_join_code_dialog && e.type == SDL_KEYDOWN) {
                        if (e.key.keysym.sym == SDLK_BACKSPACE && inp_join_code.is_active) {
                            handle_text_input(&inp_join_code, '\b');
                        }
                        if (e.key.keysym.sym == SDLK_ESCAPE) show_join_code_dialog = 0;
                    }
                    break;
                    
                case SCREEN_LOBBY_ROOM:
                    if (e.type == SDL_MOUSEBUTTONDOWN) {
                        // Check Start/Ready buttons FIRST (highest priority)
                        if (my_player_id == current_lobby.host_id) {
                            if (is_mouse_inside(btn_start.rect, mx, my)) {
                                if (current_lobby.num_players < 2) {
                                    strncpy(lobby_error_message, "Need at least 2 players to start!", 
                                           sizeof(lobby_error_message));
                                    error_message_time = SDL_GetTicks();
                                } else if (!all_players_ready(&current_lobby)) {
                                    strncpy(lobby_error_message, "All players must be ready!", 
                                           sizeof(lobby_error_message));
                                    error_message_time = SDL_GetTicks();
                                } else {
                                    send_packet(MSG_START_GAME, 0);
                                    lobby_error_message[0] = '\0';
                                }
                            }
                        } else if (is_mouse_inside(btn_ready.rect, mx, my)) {
                            send_packet(MSG_READY, 0);
                        }
                        
                        // Leave button
                        if (is_mouse_inside(btn_leave.rect, mx, my)) {
                            send_packet(MSG_LEAVE_LOBBY, 0);
                            current_screen = SCREEN_LOBBY_LIST;
                            send_packet(MSG_LIST_LOBBIES, 0);
                            lobby_error_message[0] = '\0';
                        }
                        // Lock Room button (host only)
                        else if (my_player_id == current_lobby.host_id && 
                                is_mouse_inside((SDL_Rect){1620, 100, 180, 50}, mx, my)) {
                            // Toggle lock - show notification (backend not implemented)
                            snprintf(notification_message, sizeof(notification_message),
                                    "Room locking coming soon!");
                            notification_time = SDL_GetTicks();
                        }
                        // Chat button
                        else if (is_mouse_inside((SDL_Rect){1620, 170, 180, 50}, mx, my)) {
                            // Open chat - show notification (backend not implemented)
                            snprintf(notification_message, sizeof(notification_message),
                                    "Chat system coming soon!");
                            notification_time = SDL_GetTicks();
                        }
                        // Kick buttons (host only, check for each player)
                        else if (my_player_id == current_lobby.host_id) {
                            int card_y = 200;
                            int card_width = 850;
                            int start_x = (1920 - card_width) / 2;
                            
                            for (int i = 0; i < current_lobby.num_players; i++) {
                                if (i != current_lobby.host_id) {
                                    SDL_Rect kick_btn = {start_x + card_width - 230, card_y + 35, 90, 40};
                                    if (is_mouse_inside(kick_btn, mx, my)) {
                                        // Kick player - show notification (backend not implemented)
                                        snprintf(notification_message, sizeof(notification_message),
                                                "Kick player feature coming soon!");
                                        notification_time = SDL_GetTicks();
                                        break;
                                    }
                                }
                                card_y += 130;
                            }
                        }
                    }
                    break;
                    
                case SCREEN_GAME:
                    if (e.type == SDL_KEYDOWN) {
                        ClientPacket pkt;
                        memset(&pkt, 0, sizeof(pkt));
                        pkt.type = MSG_MOVE;
                        pkt.data = -1;
                        
                        int key = e.key.keysym.sym;
                        if (key == SDLK_w || key == SDLK_UP) pkt.data = MOVE_UP;
                        else if (key == SDLK_s || key == SDLK_DOWN) pkt.data = MOVE_DOWN;
                        else if (key == SDLK_a || key == SDLK_LEFT) pkt.data = MOVE_LEFT;
                        else if (key == SDLK_d || key == SDLK_RIGHT) pkt.data = MOVE_RIGHT;
                        else if (key == SDLK_SPACE) {
                            pkt.type = MSG_PLANT_BOMB;
                            pkt.data = 0;
                        }
                        
                        if (pkt.data >= 0 || pkt.type == MSG_PLANT_BOMB) {
                            send(sock, &pkt, sizeof(pkt), 0);
                        }
                    }
                    break;
                
                case SCREEN_POST_MATCH:
                    if (e.type == SDL_MOUSEBUTTONDOWN) {
                        // Post-match screen buttons
                        printf("[CLIENT] Post-match click at (%d, %d)\n", mx, my);
                        if (is_mouse_inside((SDL_Rect){660, 850, 250, 60}, mx, my)) {
                            // Rematch - return to lobby room for another game
                            printf("[CLIENT] Rematch button clicked - returning to lobby room\n");
                            current_screen = SCREEN_LOBBY_ROOM;
                            post_match_shown = 0;  // Reset for next match
                            // Note: Players are still in the lobby, host can start another game
                        }
                        if (is_mouse_inside((SDL_Rect){930, 850, 250, 60}, mx, my)) {
                            // Return to lobby list - LEAVE CURRENT LOBBY FIRST
                            printf("[CLIENT] Return to lobby button clicked - leaving lobby and switching to LOBBY_LIST\n");
                            send_packet(MSG_LEAVE_LOBBY, 0);  // Leave current lobby
                            current_screen = SCREEN_LOBBY_LIST;
                            send_packet(MSG_LIST_LOBBIES, 0);  // Request fresh lobby list
                            post_match_shown = 0;  // Reset for next match
                        }
                    }
                    break;
                    
                case SCREEN_FRIENDS:
                case SCREEN_PROFILE:
                case SCREEN_LEADERBOARD:
                    // Handle delete friend confirmation dialog first
                    if (current_screen == SCREEN_FRIENDS && show_delete_confirm) {
                        if (e.type == SDL_MOUSEBUTTONDOWN) {
                            SDL_Rect yes_btn = {300, 350, 100, 40};
                            SDL_Rect no_btn = {420, 350, 100, 40};
                            
                            if (is_mouse_inside(yes_btn, mx, my)) {
                                // Confirm delete
                                if (delete_friend_index >= 0 && delete_friend_index < friends_count) {
                                    ClientPacket pkt;
                                    memset(&pkt, 0, sizeof(pkt));
                                    pkt.type = MSG_FRIEND_REMOVE;
                                    pkt.target_user_id = friends_list[delete_friend_index].user_id;
                                    send(sock, &pkt, sizeof(pkt), 0);
                                    
                                    snprintf(notification_message, sizeof(notification_message),
                                            "Removed %s from friends", friends_list[delete_friend_index].display_name);
                                    notification_time = SDL_GetTicks();
                                }
                                show_delete_confirm = 0;
                                delete_friend_index = -1;
                            }
                            
                            if (is_mouse_inside(no_btn, mx, my)) {
                                show_delete_confirm = 0;
                                delete_friend_index = -1;
                            }
                        }
                        
                        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                            show_delete_confirm = 0;
                            delete_friend_index = -1;
                        }
                        break;
                    }
                    
                    if (e.type == SDL_MOUSEBUTTONDOWN) {
                        // Handle friend request sending
                        if (current_screen == SCREEN_FRIENDS) {
                            inp_friend_request.is_active = is_mouse_inside(inp_friend_request.rect, mx, my);
                            
                            // Check Remove button for friends
                            int list_y = 140;  // Match rendering position
                            for (int i = 0; i < friends_count && i < 5; i++) {
                                SDL_Rect remove_btn = {360, list_y + 18, 30, 24};
                                SDL_Rect card = {50, list_y, 350, 60};
                                
                                if (is_mouse_inside(remove_btn, mx, my)) {
                                    // Show delete confirmation dialog
                                    show_delete_confirm = 1;
                                    delete_friend_index = i;
                                    break;
                                }
                                
                                // Click card to view profile
                                if (is_mouse_inside(card, mx, my) && !is_mouse_inside(remove_btn, mx, my)) {
                                    // Request friend's profile
                                    ClientPacket pkt;
                                    memset(&pkt, 0, sizeof(pkt));
                                    pkt.type = MSG_GET_PROFILE;
                                    pkt.target_user_id = friends_list[i].user_id;
                                    send(sock, &pkt, sizeof(pkt), 0);
                                    current_screen = SCREEN_PROFILE;
                                    break;
                                }
                                
                                list_y += 70;
                            }
                            
                            // Check Accept/Decline buttons for pending requests - MATCH ui_new_screens.c
                            int pending_y = 140;  // Starting Y position (matches ui_new_screens.c line 155)
                            int pending_x = 900;  // Right column X position (matches ui_new_screens.c line 156)
                            pending_y += 50;  // Skip title offset
                            
                            for (int i = 0; i < pending_count && i < 5; i++) {
                                // MATCH rendering coordinates from ui_new_screens.c lines 197, 210
                                SDL_Rect accept_btn = {pending_x + 20, pending_y + 60, 90, 35};
                                SDL_Rect decline_btn = {pending_x + 125, pending_y + 60, 90, 35};
                                
                                if (is_mouse_inside(accept_btn, mx, my)) {
                                    // Accept friend request
                                    ClientPacket pkt;
                                    memset(&pkt, 0, sizeof(pkt));
                                    pkt.type = MSG_FRIEND_ACCEPT;
                                    pkt.target_user_id = pending_requests[i].user_id;
                                    send(sock, &pkt, sizeof(pkt), 0);
                                    
                                    snprintf(notification_message, sizeof(notification_message),
                                            "Accepted %s's friend request!", pending_requests[i].display_name);
                                    notification_time = SDL_GetTicks();
                                    break;
                                }
                                
                                if (is_mouse_inside(decline_btn, mx, my)) {
                                    // Decline friend request
                                    ClientPacket pkt;
                                    memset(&pkt, 0, sizeof(pkt));
                                    pkt.type = MSG_FRIEND_DECLINE;
                                    pkt.target_user_id = pending_requests[i].user_id;
                                    send(sock, &pkt, sizeof(pkt), 0);
                                    
                                    snprintf(notification_message, sizeof(notification_message),
                                            "Declined friend request");
                                    notification_time = SDL_GetTicks();
                                    break;
                                }
                                
                                pending_y += 125;  // Card spacing (matches ui_new_screens.c line 222)
                            }
                            
                            if (is_mouse_inside(btn_send_friend_request.rect, mx, my)) {
                                if (strlen(inp_friend_request.text) > 0) {
                                    ClientPacket pkt;
                                    memset(&pkt, 0, sizeof(pkt));
                                    pkt.type = MSG_FRIEND_REQUEST;
                                    strncpy(pkt.target_display_name, inp_friend_request.text, MAX_DISPLAY_NAME - 1);
                                    send(sock, &pkt, sizeof(pkt), 0);
                                    
                                    snprintf(notification_message, sizeof(notification_message), 
                                            "Friend request sent to %s!", inp_friend_request.text);
                                    notification_time = SDL_GetTicks();
                                    inp_friend_request.text[0] = '\0';
                                }
                            }
                        }
                        
                        // Back button for all screens
                        int win_w, win_h;
                        SDL_GetRendererOutputSize(rend, &win_w, &win_h);
                        SDL_Rect back_rect = {win_w/2 - 75, win_h - 100, 150, 50};
                        
                        if (current_screen == SCREEN_SETTINGS) {
                            // Settings screen back button
                            if (is_mouse_inside((SDL_Rect){980, 850, 200, 60}, mx, my)) {
                                current_screen = SCREEN_LOBBY_LIST;
                                send_packet(MSG_LIST_LOBBIES, 0);
                            }
                            // Tab switching
                            int tab_y = 140;
                            int tab_width = 180;
                            int tab_height = 50;
                            int tab_spacing = 20;
                            int tabs_start_x = (win_w - (tab_width * 3 + tab_spacing * 2)) / 2;
                            for (int i = 0; i < 3; i++) {
                                SDL_Rect tab_rect = {tabs_start_x + i * (tab_width + tab_spacing), tab_y, tab_width, tab_height};
                                if (is_mouse_inside(tab_rect, mx, my)) {
                                    settings_active_tab = i;
                                    break;
                                }
                            }
                        } else if (is_mouse_inside(back_rect, mx, my)) {
                            current_screen = SCREEN_LOBBY_LIST;
                            send_packet(MSG_LIST_LOBBIES, 0);
                        }
                    }
                    
                    // Text input for friend requests
                    if (current_screen == SCREEN_FRIENDS && e.type == SDL_TEXTINPUT && inp_friend_request.is_active) {
                        handle_text_input(&inp_friend_request, e.text.text[0]);
                    }
                    
                    if (current_screen == SCREEN_FRIENDS && e.type == SDL_KEYDOWN && inp_friend_request.is_active) {
                        if (e.key.keysym.sym == SDLK_BACKSPACE) {
                            handle_text_input(&inp_friend_request, '\b');
                        }
                    }
                    break;
                    
                default:
                    break;
            }
        }

        // --- Network Receiving ---
        ServerPacket spkt;
        int recv_result;
        
        int packets_received = 0;
        while ((recv_result = receive_server_packet(&spkt)) == 1 && packets_received < 10) {
            process_server_packet(&spkt);
            packets_received++;
        }
        
        if (recv_result < 0) {
            printf("Server disconnected!\n");
            running = 0;
        }

        // --- Rendering ---
        if (lobby_error_message[0] != '\0' && SDL_GetTicks() - error_message_time > 3000) {
            lobby_error_message[0] = '\0';
        }
        
        switch (current_screen) {
            case SCREEN_LOGIN: {
                btn_login.is_hovered = is_mouse_inside(btn_login.rect, mx, my);
                btn_reg.is_hovered = is_mouse_inside(btn_reg.rect, mx, my);

                // Login screen: only show username and password
                render_login_screen(
                    rend, font_large, font_small,
                    &inp_user, NULL, &inp_pass,
                    &btn_login, &btn_reg, status_message
                );
                break;
            }
            
            case SCREEN_REGISTER: {
                btn_login.is_hovered = is_mouse_inside(btn_login.rect, mx, my);
                btn_reg.is_hovered = is_mouse_inside(btn_reg.rect, mx, my);

                // Register screen: show all three fields
                render_login_screen(
                    rend, font_large, font_small,
                    &inp_user, &inp_email, &inp_pass,
                    &btn_login, &btn_reg, status_message
                );
                break;
            }

            case SCREEN_LOBBY_LIST: {
                // Update hover states ONCE before rendering
                btn_create.is_hovered = is_mouse_inside(btn_create.rect, mx, my);
                btn_refresh.is_hovered = is_mouse_inside(btn_refresh.rect, mx, my);
                btn_friends.is_hovered = is_mouse_inside(btn_friends.rect, mx, my);
                btn_quick_play.is_hovered = is_mouse_inside(btn_quick_play.rect, mx, my);
                btn_settings.is_hovered = is_mouse_inside(btn_settings.rect, mx, my);
                btn_profile.is_hovered = is_mouse_inside(btn_profile.rect, mx, my);
                btn_leaderboard.is_hovered = is_mouse_inside(btn_leaderboard.rect, mx, my);
                btn_create_confirm.is_hovered = is_mouse_inside(btn_create_confirm.rect, mx, my);
                btn_cancel.is_hovered = is_mouse_inside(btn_cancel.rect, mx, my);

                render_lobby_list_screen(
                    rend, font_small,
                    lobby_list, lobby_count,
                    &btn_create, &btn_refresh,
                    selected_lobby_idx
                );
                
                // Draw additional nav buttons - MODERNIZED with rounded corners
                // Friends button
                draw_button(rend, font_small, &btn_friends);
                
                // Quick Play button
                draw_button(rend, font_small, &btn_quick_play);
                
                // Settings button (bottom left)
                draw_button(rend, font_small, &btn_settings);
                
                // Profile & Leaderboard buttons (top right)
                draw_button(rend, font_small, &btn_profile);
                draw_button(rend, font_small, &btn_leaderboard);
                
                //SDL_RenderPresent(rend);
                
                // Render dialog overlay if showing
                if (show_create_room_dialog) {
                    render_create_room_dialog(rend, font_small, &inp_room_name, &inp_access_code,
                                             &btn_create_confirm, &btn_cancel);
                    // Draw input fields
                    draw_input_field(rend, font_small, &inp_room_name);
                    draw_input_field(rend, font_small, &inp_access_code);
                    // Draw buttons
                    draw_button(rend, font_small, &btn_create_confirm);
                    draw_button(rend, font_small, &btn_cancel);
                    //SDL_RenderPresent(rend);
                }
                
                // Render join code dialog if showing
                if (show_join_code_dialog) {
                    render_create_room_dialog(rend, font_small, &inp_join_code, NULL,
                                             &btn_create_confirm, &btn_cancel);
                    // Change button text
                    strcpy(btn_create_confirm.text, "Join");
                    draw_input_field(rend, font_small, &inp_join_code);
                    draw_button(rend, font_small, &btn_create_confirm);
                    draw_button(rend, font_small, &btn_cancel);
                    //SDL_RenderPresent(rend);
                }
                break;
            }

            case SCREEN_LOBBY_ROOM: {
                if (my_player_id == current_lobby.host_id) {
                    btn_start.is_hovered = is_mouse_inside(btn_start.rect, mx, my);
                    btn_leave.is_hovered = is_mouse_inside(btn_leave.rect, mx, my);
                    btn_ready.is_hovered = 0;
                } else {
                    btn_ready.is_hovered = is_mouse_inside(btn_ready.rect, mx, my);
                    btn_leave.is_hovered = is_mouse_inside(btn_leave.rect, mx, my);
                    btn_start.is_hovered = 0;
                }

                Button *ready_ptr = (my_player_id != current_lobby.host_id) ? &btn_ready : NULL;
                Button *start_ptr = (my_player_id == current_lobby.host_id) ? &btn_start : NULL;

                render_lobby_room_screen(
                    rend, font_small,
                    &current_lobby, my_player_id,
                    ready_ptr, start_ptr, &btn_leave
                );
                break;
            }

            case SCREEN_GAME: {
                // Calculate elapsed game time
                int elapsed_seconds = 0;
                if (game_start_time > 0) {
                    Uint32 elapsed_ms = SDL_GetTicks() - game_start_time;
                    elapsed_seconds = elapsed_ms / 1000;
                }
                render_game(rend, font_small, tick++, my_player_id, elapsed_seconds);
                break;
            }
            
            case SCREEN_FRIENDS: {
                Button back_btn;
                back_btn.is_hovered = is_mouse_inside(back_btn.rect, mx, my);
                btn_send_friend_request.is_hovered = is_mouse_inside(btn_send_friend_request.rect, mx, my);
                
                render_friends_screen(rend, font_small, friends_list, friends_count,
                                     pending_requests, pending_count,
                                     sent_requests, sent_count, &back_btn);
                
                // Draw friend request input and button
                draw_input_field(rend, font_small, &inp_friend_request);
                draw_button(rend, font_small, &btn_send_friend_request);
                
                // Draw delete confirmation dialog if active - MODERNIZED
                if (show_delete_confirm && delete_friend_index >= 0 && delete_friend_index < friends_count) {
                    int win_w, win_h;
                    SDL_GetRendererOutputSize(rend, &win_w, &win_h);
                    
                    // Semi-transparent overlay
                    SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);
                    SDL_SetRenderDrawColor(rend, 0, 0, 0, 200);
                    SDL_Rect overlay = {0, 0, win_w, win_h};
                    SDL_RenderFillRect(rend, &overlay);
                    
                    // Modern dialog box with rounded corners
                    SDL_Rect dialog = {250, 250, 300, 150};
                    
                    // Layered shadow
                    draw_layered_shadow(rend, dialog, 12, 6);
                    
                    // Dialog background
                    SDL_Color dialog_bg = {30, 41, 59, 255};
                    draw_rounded_rect(rend, dialog, dialog_bg, 12);
                    
                    // Dialog border
                    SDL_Color border_col = {59, 130, 246, 255};
                    draw_rounded_border(rend, dialog, border_col, 12, 2);
                    
                    // Title
                    SDL_Surface *surf = TTF_RenderText_Blended(font_small, "Delete Friend?", (SDL_Color){255, 255, 255, 255});
                    if (surf) {
                        SDL_Texture *tex = SDL_CreateTextureFromSurface(rend, surf);
                        SDL_Rect r = {dialog.x + (dialog.w - surf->w)/2, dialog.y + 20, surf->w, surf->h};
                        SDL_RenderCopy(rend, tex, NULL, &r);
                        SDL_DestroyTexture(tex);
                        SDL_FreeSurface(surf);
                    }
                    
                    // Friend name
                    char msg[128];
                    snprintf(msg, sizeof(msg), "Remove %s?", friends_list[delete_friend_index].display_name);
                    surf = TTF_RenderText_Blended(font_small, msg, (SDL_Color){200, 200, 200, 255});
                    if (surf) {
                        SDL_Texture *tex = SDL_CreateTextureFromSurface(rend, surf);
                        SDL_Rect r = {dialog.x + (dialog.w - surf->w)/2, dialog.y + 60, surf->w, surf->h};
                        SDL_RenderCopy(rend, tex, NULL, &r);
                        SDL_DestroyTexture(tex);
                        SDL_FreeSurface(surf);
                    }
                    
                    // Modern Yes button (rounded, red with shadow)
                    SDL_Rect yes_btn = {300, 350, 100, 40};
                    draw_layered_shadow(rend, yes_btn, 6, 3);
                    SDL_Color yes_bg = {239, 68, 68, 255};
                    draw_rounded_rect(rend, yes_btn, yes_bg, 6);
                    
                    surf = TTF_RenderText_Blended(font_small, "Yes", (SDL_Color){255, 255, 255, 255});
                    if (surf) {
                        SDL_Texture *tex = SDL_CreateTextureFromSurface(rend, surf);
                        SDL_Rect r = {yes_btn.x + (yes_btn.w - surf->w)/2, yes_btn.y + (yes_btn.h - surf->h)/2, surf->w, surf->h};
                        SDL_RenderCopy(rend, tex, NULL, &r);
                        SDL_DestroyTexture(tex);
                        SDL_FreeSurface(surf);
                    }
                    
                    // Modern No button (rounded, gray with shadow)
                    SDL_Rect no_btn = {420, 350, 100, 40};
                    draw_layered_shadow(rend, no_btn, 6, 3);
                    SDL_Color no_bg = {100, 116, 139, 255};
                    draw_rounded_rect(rend, no_btn, no_bg, 6);
                    
                    surf = TTF_RenderText_Blended(font_small, "No", (SDL_Color){255, 255, 255, 255});
                    if (surf) {
                        SDL_Texture *tex = SDL_CreateTextureFromSurface(rend, surf);
                        SDL_Rect r = {no_btn.x + (no_btn.w - surf->w)/2, no_btn.y + (no_btn.h - surf->h)/2, surf->w, surf->h};
                        SDL_RenderCopy(rend, tex, NULL, &r);
                        SDL_DestroyTexture(tex);
                        SDL_FreeSurface(surf);
                    }
                    
                    SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_NONE);
                }
                break;
            }
            
            case SCREEN_PROFILE: {
                Button back_btn;
                back_btn.is_hovered = is_mouse_inside(back_btn.rect, mx, my);
                render_profile_screen(rend, font_large, font_small, &my_profile, &back_btn);
                break;
            }
            
            case SCREEN_LEADERBOARD: {
                Button back_btn;
                back_btn.is_hovered = is_mouse_inside(back_btn.rect, mx, my);
                render_leaderboard_screen(rend, font_large, font_small, leaderboard, leaderboard_count, &back_btn);
                break;
            }
            
            case SCREEN_SETTINGS: {
                // Update hover states
                btn_settings_apply.is_hovered = is_mouse_inside((SDL_Rect){760, 850, 200, 60}, mx, my);
                Button back_btn_temp = {{980, 850, 200, 60}, "Back", 0};
                back_btn_temp.is_hovered = is_mouse_inside(back_btn_temp.rect, mx, my);
                
                render_settings_screen(rend, font_large, font_small,
                                      settings_active_tab, &back_btn_temp, &btn_settings_apply);
                break;
            }
            
            case SCREEN_POST_MATCH: {
                // Update hover states
                btn_rematch.is_hovered = is_mouse_inside((SDL_Rect){660, 850, 250, 60}, mx, my);
                btn_return_lobby.is_hovered = is_mouse_inside((SDL_Rect){930, 850, 250, 60}, mx, my);
                
                render_post_match_screen(rend, font_large, font_small,
                                        post_match_winner_id, post_match_elo_changes,
                                        post_match_kills, post_match_duration,
                                        &btn_rematch, &btn_return_lobby,
                                        &current_state);
                break;
            }

            default:
                break;
        }
        
        // Render notifications at top center - MODERNIZED
        if (notification_message[0] != '\0' && SDL_GetTicks() - notification_time < NOTIFICATION_DURATION) {
            int win_w, win_h;
            SDL_GetRendererOutputSize(rend, &win_w, &win_h);
            
            SDL_Surface *notif = TTF_RenderText_Blended(font_small, notification_message, (SDL_Color){255, 255, 255, 255});
            if (notif) {
                int box_w = notif->w + 40;
                int box_h = notif->h + 20;
                int box_x = (win_w - box_w) / 2;
                int box_y = 20;
                
                SDL_Rect bg = {box_x, box_y, box_w, box_h};
                
                // Layered shadow for depth
                draw_layered_shadow(rend, bg, 8, 4);
                
                // Background with rounded corners
                SDL_Color notif_bg = {59, 130, 246, 230};
                SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);
                draw_rounded_rect(rend, bg, notif_bg, 8);
                
                // Border with accent color
                SDL_Color notif_border = {96, 165, 250, 255};
                draw_rounded_border(rend, bg, notif_border, 8, 2);
                SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_NONE);
                
                // Text
                SDL_Texture *tex = SDL_CreateTextureFromSurface(rend, notif);
                SDL_Rect text_rect = {box_x + 20, box_y + 10, notif->w, notif->h};
                SDL_RenderCopy(rend, tex, NULL, &text_rect);
                SDL_DestroyTexture(tex);
                SDL_FreeSurface(notif);
            }
        } else if (SDL_GetTicks() - notification_time >= NOTIFICATION_DURATION) {
            notification_message[0] = '\0'; // Clear after duration
        }

        SDL_RenderPresent(rend); // ~60 FPS
    }

    close(sock);
    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(win);
    if (font_large) TTF_CloseFont(font_large);
    if (font_small) TTF_CloseFont(font_small);
    TTF_Quit();
    SDL_Quit();
    return 0;
}