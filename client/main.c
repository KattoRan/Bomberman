/* client/main.c */
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include "../common/protocol.h"
#include "ui.h" 

extern void render_game(SDL_Renderer*, TTF_Font*, int);
extern TTF_Font* init_font();

// State
typedef enum {
    SCREEN_LOGIN,
    SCREEN_REGISTER,
    SCREEN_LOBBY_LIST,
    SCREEN_LOBBY_ROOM,
    SCREEN_GAME
} ScreenState;

ScreenState current_screen = SCREEN_LOGIN;
int sock;
int my_player_id = -1;
char my_username[MAX_USERNAME];
char status_message[256] = "";

// Data Store
Lobby lobby_list[MAX_LOBBIES];
int lobby_count = 0;
int selected_lobby_idx = -1;
Lobby current_lobby;

// --- SỬA ĐỔI QUAN TRỌNG: Đặt tên biến trùng với graphics.c ---
GameState current_state; 
// -------------------------------------------------------------

// UI Components Definitions
InputField inp_user = {{350, 200, 300, 40}, "", "Username:", 0, 30};
InputField inp_pass = {{350, 300, 300, 40}, "", "Password:", 0, 30};
Button btn_login = {{350, 400, 140, 50}, "Login", 0};
Button btn_reg = {{510, 400, 140, 50}, "Register", 0};

Button btn_create = {{50, 500, 200, 50}, "Create Room", 0};
Button btn_refresh = {{270, 500, 200, 50}, "Refresh", 0};

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
    if (c == '\b') { // Backspace
        if (len > 0) field->text[len-1] = '\0';
    } else if (len < field->max_length) {
        field->text[len] = c;
        field->text[len+1] = '\0';
    }
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
            }
            strncpy(status_message, pkt->message, sizeof(status_message));
            break;

        case MSG_LOBBY_LIST:
            lobby_count = pkt->payload.lobby_list.count;
            memcpy(lobby_list, pkt->payload.lobby_list.lobbies, sizeof(lobby_list));
            break;

        case MSG_LOBBY_UPDATE:
            current_lobby = pkt->payload.lobby;
            if (current_lobby.status == LOBBY_PLAYING) {
                current_screen = SCREEN_GAME;
            } else {
                current_screen = SCREEN_LOBBY_ROOM;
            }
            break;

        case MSG_GAME_STATE:
            // --- SỬA ĐỔI: Cập nhật trực tiếp vào current_state ---
            current_state = pkt->payload.game_state;
            break;
            
        case MSG_ERROR:
            strncpy(status_message, pkt->message, sizeof(status_message));
            break;
    }
}

// --- Main Client ---

int main(int argc, char *argv[]) {
    // Suppress unused parameter warnings
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

    // 2. Setup SDL
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    SDL_Window *win = SDL_CreateWindow("Bomberman", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN);
    SDL_Renderer *rend = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    TTF_Font *font = init_font();

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

            if (current_screen == SCREEN_LOGIN) {
                if (e.type == SDL_MOUSEBUTTONDOWN) {
                    inp_user.is_active = is_mouse_inside(inp_user.rect, mx, my);
                    inp_pass.is_active = is_mouse_inside(inp_pass.rect, mx, my);
                    
                    if (is_mouse_inside(btn_login.rect, mx, my)) {
                        ClientPacket pkt;
                        memset(&pkt, 0, sizeof(pkt));
                        pkt.type = MSG_LOGIN;
                        strcpy(pkt.username, inp_user.text);
                        strcpy(pkt.password, inp_pass.text);
                        send(sock, &pkt, sizeof(pkt), 0);
                    }
                    if (is_mouse_inside(btn_reg.rect, mx, my)) {
                         ClientPacket pkt;
                        memset(&pkt, 0, sizeof(pkt));
                        pkt.type = MSG_REGISTER;
                        strcpy(pkt.username, inp_user.text);
                        strcpy(pkt.password, inp_pass.text);
                        send(sock, &pkt, sizeof(pkt), 0);
                    }
                }
                if (e.type == SDL_TEXTINPUT) {
                    if (inp_user.is_active) handle_text_input(&inp_user, e.text.text[0]);
                    if (inp_pass.is_active) handle_text_input(&inp_pass, e.text.text[0]);
                }
                if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_BACKSPACE) {
                     if (inp_user.is_active) handle_text_input(&inp_user, '\b');
                     if (inp_pass.is_active) handle_text_input(&inp_pass, '\b');
                }
            }
            else if (current_screen == SCREEN_LOBBY_LIST) {
                if (e.type == SDL_MOUSEBUTTONDOWN) {
                    if (is_mouse_inside(btn_create.rect, mx, my)) {
                        ClientPacket pkt;
                        memset(&pkt, 0, sizeof(pkt));
                        pkt.type = MSG_CREATE_LOBBY;
                        strcpy(pkt.room_name, "New Room");
                        send(sock, &pkt, sizeof(pkt), 0);
                    }
                    if (is_mouse_inside(btn_refresh.rect, mx, my)) {
                        send_packet(MSG_LIST_LOBBIES, 0);
                    }
                    int y = 80;
                    for (int i=0; i<lobby_count; i++) {
                        SDL_Rect r = {50, y, 700, 60};
                        if (is_mouse_inside(r, mx, my)) {
                            ClientPacket pkt;
                            pkt.type = MSG_JOIN_LOBBY;
                            pkt.lobby_id = lobby_list[i].id;
                            send(sock, &pkt, sizeof(pkt), 0);
                        }
                        y += 70;
                    }
                }
            }
            else if (current_screen == SCREEN_LOBBY_ROOM) {
                if (e.type == SDL_MOUSEBUTTONDOWN) {
                    if (is_mouse_inside(btn_ready.rect, mx, my)) send_packet(MSG_READY, 0);
                    if (is_mouse_inside(btn_start.rect, mx, my)) send_packet(MSG_START_GAME, 0);
                    if (is_mouse_inside(btn_leave.rect, mx, my)) {
                        send_packet(MSG_LEAVE_LOBBY, 0);
                        current_screen = SCREEN_LOBBY_LIST;
                        send_packet(MSG_LIST_LOBBIES, 0);
                    }
                }
            }
            else if (current_screen == SCREEN_GAME) {
                if (e.type == SDL_KEYDOWN) {
                    ClientPacket pkt;
                    memset(&pkt, 0, sizeof(pkt));
                    pkt.type = MSG_MOVE;
                    int key = e.key.keysym.sym;
                    if (key == SDLK_w) pkt.data = MOVE_UP;
                    else if (key == SDLK_s) pkt.data = MOVE_DOWN;
                    else if (key == SDLK_a) pkt.data = MOVE_LEFT;
                    else if (key == SDLK_d) pkt.data = MOVE_RIGHT;
                    else if (key == SDLK_SPACE) pkt.type = MSG_PLANT_BOMB;
                    
                    if (pkt.data >= 0 || pkt.type == MSG_PLANT_BOMB)
                        send(sock, &pkt, sizeof(pkt), 0);
                }
            }
        }

        // --- Network Receiving ---
        ServerPacket spkt;
        int n = recv(sock, &spkt, sizeof(spkt), MSG_DONTWAIT);
        if (n > 0) {
            process_server_packet(&spkt);
        }

        // --- Rendering ---
        if (current_screen == SCREEN_LOGIN) {
            btn_login.is_hovered = is_mouse_inside(btn_login.rect, mx, my);
            btn_reg.is_hovered = is_mouse_inside(btn_reg.rect, mx, my);
            render_login_screen(rend, font, &inp_user, &inp_pass, &btn_login, &btn_reg, status_message);
        }
        else if (current_screen == SCREEN_LOBBY_LIST) {
            btn_create.is_hovered = is_mouse_inside(btn_create.rect, mx, my);
            btn_refresh.is_hovered = is_mouse_inside(btn_refresh.rect, mx, my);
            render_lobby_list_screen(rend, font, lobby_list, lobby_count, &btn_create, &btn_refresh, selected_lobby_idx);
        }
        else if (current_screen == SCREEN_LOBBY_ROOM) {
            btn_ready.is_hovered = is_mouse_inside(btn_ready.rect, mx, my);
            btn_start.is_hovered = is_mouse_inside(btn_start.rect, mx, my);
            btn_leave.is_hovered = is_mouse_inside(btn_leave.rect, mx, my);
            render_lobby_room_screen(rend, font, &current_lobby, 0, &btn_ready, &btn_start, &btn_leave);
        }
        else if (current_screen == SCREEN_GAME) {
            render_game(rend, font, tick++);
        }

        SDL_Delay(16);
    }

    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(win);
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();
    return 0;
}