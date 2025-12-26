/* client/ui.h */
#ifndef UI_H
#define UI_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "../common/protocol.h"

// Định nghĩa Struct Button
typedef enum {
    BTN_PRIMARY,
    BTN_DANGER,
    BTN_OUTLINE
} ButtonType;

typedef struct Button {
    SDL_Rect rect;
    char text[64];
    int is_hovered;
    ButtonType type;
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
void draw_background_grid(SDL_Renderer *renderer, int w, int h);

// Khai báo các hàm UI (Prototype)
void render_login_screen(SDL_Renderer *renderer, TTF_Font *font_large, TTF_Font *font_small, 
                        InputField *username, InputField *email, InputField *password,
                        Button *login_btn, Button *register_btn, 
                        const char *message);

void render_lobby_list_screen(SDL_Renderer *renderer, TTF_Font *font,
                              LobbySummary *lobbies, int lobby_count,
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

void render_profile_screen(SDL_Renderer *renderer, TTF_Font *font_medium, TTF_Font *font_small,
                           ProfileData *profile,
                           Button *back_btn, const char *title_override);

void render_leaderboard_screen(SDL_Renderer *renderer, TTF_Font *font_medium, TTF_Font *font_small,
                               LeaderboardEntry *entries, int entry_count,
                               Button *back_btn);

void render_create_room_dialog(SDL_Renderer *renderer, TTF_Font *font,
                               InputField *room_name, InputField *access_code,
                               Button *create_btn, Button *cancel_btn);

// render_settings_screen removed

void render_post_match_screen(SDL_Renderer *renderer, TTF_Font *font_large, TTF_Font *font_small,
                              int winner_id, int *elo_changes, int *kills, int duration_seconds,
                              Button *rematch_btn, Button *lobby_btn, GameState *game_state, int my_player_id);

// Chat rendering functions
void render_chat_message_block(SDL_Renderer *renderer, TTF_Font *font,
                               const char *sender, const char *message,
                               int player_id, int is_current_user,
                               int x, int y, int width);

void render_chat_panel_room(SDL_Renderer *renderer, TTF_Font *font,
                            void *chat_messages, void *input_field,
                            int chat_count);


// Invite system structures
typedef struct {
    int lobby_id;
    char room_name[64];
    char host_name[32];
    char access_code[8];
    int game_mode;
    int is_active;
} IncomingInvite;

void render_invite_overlay(SDL_Renderer *renderer, TTF_Font *font, 
                          FriendInfo *online_friends, int friend_count, 
                          int *invited_user_ids, int invited_count,
                          Button *close_btn);

void render_invitation_popup(SDL_Renderer *renderer, TTF_Font *font,
                            IncomingInvite *invite,
                            Button *accept_btn, Button *decline_btn);

#endif