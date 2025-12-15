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

extern void render_game(SDL_Renderer*, TTF_Font*, int, int);
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
    SCREEN_LEADERBOARD
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
ProfileData my_profile;
LeaderboardEntry leaderboard[100];
int leaderboard_count = 0;

// UI Components
InputField inp_user = {{350, 150, 300, 40}, "", "Username:", 0, 30};
InputField inp_email = {{350, 220, 300, 40}, "", "Email:", 0, 127};
InputField inp_pass = {{350, 290, 300, 40}, "", "Password:", 0, 30};
Button btn_login = {{350, 380, 140, 50}, "Login", 0};
Button btn_reg = {{510, 380, 140, 50}, "Register", 0};

Button btn_create = {{50, 500, 200, 50}, "Create Room", 0};
Button btn_refresh = {{270, 500, 200, 50}, "Refresh", 0};
Button btn_friends = {{490, 500, 150, 50}, "Friends", 0};
Button btn_profile = {{650, 10, 70, 35}, "Profile", 0};
Button btn_leaderboard = {{730, 10, 60, 35}, "Top", 0};

Button btn_ready = {{50, 500, 200, 50}, "Ready", 0};
Button btn_start = {{270, 500, 200, 50}, "Start Game", 0};
Button btn_leave = {{600, 500, 150, 50}, "Leave", 0};

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
                // Both login and registration success lead to lobby list
                current_screen = SCREEN_LOBBY_LIST;
                send_packet(MSG_LIST_LOBBIES, 0); 
                
                // Store username from the input field
                strncpy(my_username, inp_user.text, MAX_USERNAME);
                status_message[0] = '\0';
                
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
                }
                current_screen = SCREEN_GAME;
                memset(&current_state, 0, sizeof(GameState));
                memset(&previous_state, 0, sizeof(GameState));
                lobby_error_message[0] = '\0';
            } else {
                current_screen = SCREEN_LOBBY_ROOM;
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
                
                SDL_Delay(3000);
                current_screen = SCREEN_LOBBY_ROOM;
            }
            break;
            
        case MSG_ERROR:
            strncpy(status_message, pkt->message, sizeof(status_message));
            strncpy(lobby_error_message, pkt->message, sizeof(lobby_error_message));
            break;
            
        case MSG_FRIEND_LIST_RESPONSE:
            friends_count = pkt->payload.friend_list.count;
            if (friends_count > 50) friends_count = 50;
            memcpy(friends_list, pkt->payload.friend_list.friends, 
                   sizeof(FriendInfo) * friends_count);
            printf("[CLIENT] Received %d friends\n", friends_count);
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

    // 2. Setup SDL
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    SDL_Window *win = SDL_CreateWindow("Bomberman", 
                                       SDL_WINDOWPOS_CENTERED, 
                                       SDL_WINDOWPOS_CENTERED, 
                                       800, 600, 
                                       SDL_WINDOW_SHOWN);
    SDL_Renderer *rend = SDL_CreateRenderer(win, -1, 0);
    if (!win) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return -1;
    }

    if (!rend) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return -1;
    }
    TTF_Font *font_large = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 48);
    if (!font_large) {
        font_large = TTF_OpenFont("C:\\Windows\\Fonts\\arialbd.ttf", 48);
    }
    
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
                            ClientPacket pkt;
                            memset(&pkt, 0, sizeof(pkt));
                            pkt.type = MSG_CREATE_LOBBY;
                            snprintf(pkt.room_name, sizeof(pkt.room_name), "Room %d", lobby_count + 1);
                            send(sock, &pkt, sizeof(pkt), 0);
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
                        
                        int win_w, win_h;
                        SDL_GetRendererOutputSize(rend, &win_w, &win_h);
                        int list_width = 700;
                        int start_x = (win_w - list_width) / 2;
                        int y = 100;
                        
                        for (int i = 0; i < lobby_count; i++) {
                            SDL_Rect r = {start_x, y, list_width, 70};
                            if (is_mouse_inside(r, mx, my)) {
                                ClientPacket pkt;
                                memset(&pkt, 0, sizeof(pkt));
                                pkt.type = MSG_JOIN_LOBBY;
                                pkt.lobby_id = lobby_list[i].id;
                                send(sock, &pkt, sizeof(pkt), 0);
                                selected_lobby_idx = i;
                                break;
                            }
                            y += 85;
                        }
                    }
                    break;
                    
                case SCREEN_LOBBY_ROOM:
                    if (e.type == SDL_MOUSEBUTTONDOWN) {
                        if (is_mouse_inside(btn_leave.rect, mx, my)) {
                            send_packet(MSG_LEAVE_LOBBY, 0);
                            current_screen = SCREEN_LOBBY_LIST;
                            send_packet(MSG_LIST_LOBBIES, 0);
                            lobby_error_message[0] = '\0';
                        }
                        else if (my_player_id != current_lobby.host_id) {
                            if (is_mouse_inside(btn_ready.rect, mx, my)) {
                                send_packet(MSG_READY, 0);
                                lobby_error_message[0] = '\0';
                            }
                        }
                        else if (my_player_id == current_lobby.host_id) {
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
                    
                case SCREEN_FRIENDS:
                case SCREEN_PROFILE:
                case SCREEN_LEADERBOARD:
                    if (e.type == SDL_MOUSEBUTTONDOWN) {
                        Button back_btn;
                        if (is_mouse_inside(back_btn.rect, mx, my)) {
                            current_screen = SCREEN_LOBBY_LIST;
                            send_packet(MSG_LIST_LOBBIES, 0);
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
                btn_create.is_hovered = is_mouse_inside(btn_create.rect, mx, my);
                btn_refresh.is_hovered = is_mouse_inside(btn_refresh.rect, mx, my);
                btn_friends.is_hovered = is_mouse_inside(btn_friends.rect, mx, my);
                btn_profile.is_hovered = is_mouse_inside(btn_profile.rect, mx, my);
                btn_leaderboard.is_hovered = is_mouse_inside(btn_leaderboard.rect, mx, my);

                render_lobby_list_screen(
                    rend, font_small,
                    lobby_list, lobby_count,
                    &btn_create, &btn_refresh,
                    selected_lobby_idx
                );
                
                // Draw additional nav buttons
                // Friends button
                SDL_Color friends_color = btn_friends.is_hovered ? (SDL_Color){96, 165, 250, 255} : (SDL_Color){59, 130, 246, 255};
                SDL_SetRenderDrawColor(rend, friends_color.r, friends_color.g, friends_color.b, 255);
                SDL_RenderFillRect(rend, &btn_friends.rect);
                SDL_Surface *surf = TTF_RenderText_Blended(font_small, btn_friends.text, (SDL_Color){255, 255, 255, 255});
                if (surf) {
                    SDL_Texture *tex = SDL_CreateTextureFromSurface(rend, surf);
                    SDL_Rect r = {btn_friends.rect.x + (btn_friends.rect.w - surf->w)/2, btn_friends.rect.y + (btn_friends.rect.h - surf->h)/2, surf->w, surf->h};
                    SDL_RenderCopy(rend, tex, NULL, &r);
                    SDL_DestroyTexture(tex);
                    SDL_FreeSurface(surf);
                }
                
                // Profile & Leaderboard buttons (top right)
                SDL_Color prof_color = btn_profile.is_hovered ? (SDL_Color){96, 165, 250, 255} : (SDL_Color){59, 130, 246, 255};
                SDL_SetRenderDrawColor(rend, prof_color.r, prof_color.g, prof_color.b, 255);
                SDL_RenderFillRect(rend, &btn_profile.rect);
                surf = TTF_RenderText_Blended(font_small, btn_profile.text, (SDL_Color){255, 255, 255, 255});
                if (surf) {
                    SDL_Texture *tex = SDL_CreateTextureFromSurface(rend, surf);
                    SDL_Rect r = {btn_profile.rect.x + (btn_profile.rect.w - surf->w)/2, btn_profile.rect.y + (btn_profile.rect.h - surf->h)/2, surf->w, surf->h};
                    SDL_RenderCopy(rend, tex, NULL, &r);
                    SDL_DestroyTexture(tex);
                    SDL_FreeSurface(surf);
                }
                
                SDL_Color lead_color = btn_leaderboard.is_hovered ? (SDL_Color){96, 165, 250, 255} : (SDL_Color){59, 130, 246, 255};
                SDL_SetRenderDrawColor(rend, lead_color.r, lead_color.g, lead_color.b, 255);
                SDL_RenderFillRect(rend, &btn_leaderboard.rect);
                surf = TTF_RenderText_Blended(font_small, btn_leaderboard.text, (SDL_Color){255, 255, 255, 255});
                if (surf) {
                    SDL_Texture *tex = SDL_CreateTextureFromSurface(rend, surf);
                    SDL_Rect r = {btn_leaderboard.rect.x + (btn_leaderboard.rect.w - surf->w)/2, btn_leaderboard.rect.y + (btn_leaderboard.rect.h - surf->h)/2, surf->w, surf->h};
                    SDL_RenderCopy(rend, tex, NULL, &r);
                    SDL_DestroyTexture(tex);
                    SDL_FreeSurface(surf);
                }
                
                SDL_RenderPresent(rend);
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
                render_game(rend, font_small, tick++, my_player_id);
                break;
            }
            
            case SCREEN_FRIENDS: {
                Button back_btn;
                back_btn.is_hovered = is_mouse_inside(back_btn.rect, mx, my);
                render_friends_screen(rend, font_small, friends_list, friends_count,
                                     pending_requests, pending_count, &back_btn);
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
                render_leaderboard_screen(rend, font_small, leaderboard, leaderboard_count, &back_btn);
                break;
            }

            default:
                break;
        }

        SDL_Delay(16); // ~60 FPS
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