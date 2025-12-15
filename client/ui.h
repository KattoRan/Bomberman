/* client/ui.h */
#ifndef UI_H
#define UI_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "../common/protocol.h"

// Định nghĩa Struct Button
typedef struct {
    SDL_Rect rect;
    char text[64];
    int is_hovered;
} Button;

// Định nghĩa Struct InputField
typedef struct {
    SDL_Rect rect;
    char text[128];
    char label[64];
    int is_active;
    int max_length;
} InputField;

// Biến toàn cục cho thông báo lỗi trong lobby room
extern char lobby_error_message[256];

// Helper drawing functions
void draw_button(SDL_Renderer *renderer, TTF_Font *font, Button *btn);
void draw_input_field(SDL_Renderer *renderer, TTF_Font *font, InputField *field);

// Khai báo các hàm UI (Prototype)
void render_login_screen(SDL_Renderer *renderer, TTF_Font *font_large, TTF_Font *font_small, 
                        InputField *username, InputField *email, InputField *password,
                        Button *login_btn, Button *register_btn, 
                        const char *message);

void render_lobby_list_screen(SDL_Renderer *renderer, TTF_Font *font,
                              Lobby *lobbies, int lobby_count,
                              Button *create_btn, Button *refresh_btn,
                              int selected_lobby);

void render_lobby_room_screen(SDL_Renderer *renderer, TTF_Font *font,
                              Lobby *lobby, int my_player_id,
                              Button *ready_btn, Button *start_btn, 
                              Button *leave_btn);

void render_friends_screen(SDL_Renderer *renderer, TTF_Font *font,
                           FriendInfo *friends, int friend_count,
                           FriendInfo *pending, int pending_count,
                           Button *back_btn);

void render_profile_screen(SDL_Renderer *renderer, TTF_Font *font_large, TTF_Font *font_small,
                           ProfileData *profile,
                           Button *back_btn);

void render_leaderboard_screen(SDL_Renderer *renderer, TTF_Font *font_large, TTF_Font *font_small,
                               LeaderboardEntry *entries, int entry_count,
                               Button *back_btn);

void render_create_room_dialog(SDL_Renderer *renderer, TTF_Font *font,
                               InputField *room_name, InputField *access_code,
                               Button *create_btn, Button *cancel_btn);

#endif