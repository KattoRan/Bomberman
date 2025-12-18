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

// Modern UI helper functions
void draw_rounded_rect(SDL_Renderer *renderer, SDL_Rect rect, SDL_Color color, int radius);
void draw_rounded_border(SDL_Renderer *renderer, SDL_Rect rect, SDL_Color color, int radius, int thickness);
void draw_layered_shadow(SDL_Renderer *renderer, SDL_Rect rect, int radius, int offset);

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
                           FriendInfo *sent, int sent_count,
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

void render_settings_screen(SDL_Renderer *renderer, TTF_Font *font_large, TTF_Font *font_small,
                            int active_tab, Button *back_btn, Button *apply_btn);

void render_post_match_screen(SDL_Renderer *renderer, TTF_Font *font_large, TTF_Font *font_small,
                              int winner_id, int *elo_changes, int *kills, int duration_seconds,
                              Button *rematch_btn, Button *lobby_btn, GameState *game_state);

#endif