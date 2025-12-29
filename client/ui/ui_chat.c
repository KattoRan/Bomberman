#include "../state/client_state.h"
#include "ui.h"
#include "../graphics/graphics.h"
#include <SDL2/SDL.h> 
#include <SDL2/SDL_ttf.h>
#define UI_CORNER_RADIUS 8

const SDL_Color CHAT_PLAYER_COLORS[4] = {
    {0, 0, 255, 255},     // Player 1: Blue
    {255, 255, 0, 255},   // Player 2: Yellow
    {255, 0, 255, 255},   // Player 3: Magenta
    {0, 255, 255, 255}    // Player 4: Cyan
};

void render_chat_message_block(SDL_Renderer *renderer, TTF_Font *font,
                               const char *sender, const char *message,
                               int player_id, int is_current_user,
                               int x, int y, int width) {
    int card_height = 70;  // Compact height for 2-line message
    SDL_Rect card_rect = {x, y, width, card_height};
    
    // Background color based on current user
    SDL_Color bg_color;
    if (is_current_user) {
        bg_color = (SDL_Color){20, 30, 45, 255};  // Darker for current user
    } else {
        bg_color = CLR_INPUT_BG;  // {30, 41, 59, 255}
    }
    
    // Shadow
    SDL_Rect shadow = {card_rect.x + 2, card_rect.y + 2, card_rect.w, card_rect.h};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    draw_rounded_rect(renderer, shadow, (SDL_Color){0, 0, 0, 40}, UI_CORNER_RADIUS);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    
    // Background
    draw_rounded_rect(renderer, card_rect, bg_color, UI_CORNER_RADIUS);
    
    // Border (accent for current user, gray for others)
    SDL_Color border_color;
    int border_thickness;
    if (is_current_user) {
        border_color = CLR_ACCENT;  // Orange border
        border_thickness = 2;
    } else {
        border_color = CLR_GRAY;
        border_thickness = 1;
    }
    draw_rounded_border(renderer, card_rect, border_color, UI_CORNER_RADIUS, border_thickness);
    
    if (!font) return;
    
    // Line 1: Username (colored by player ID)
    SDL_Color username_color = CHAT_PLAYER_COLORS[player_id % 4];
    
    char username_display[40];
    snprintf(username_display, sizeof(username_display), "%s:", sender);
    
    SDL_Surface *username_surf = TTF_RenderText_Blended(font, username_display, username_color);
    if (username_surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, username_surf);
        SDL_Rect username_rect = {x + 10, y + 6, username_surf->w, username_surf->h};
        SDL_RenderCopy(renderer, tex, NULL, &username_rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(username_surf);
    }
    
    // Line 2: Message text (white, truncated if too long)
    char msg_display[80];
    strncpy(msg_display, message, sizeof(msg_display) - 1);
    msg_display[sizeof(msg_display) - 1] = '\0';
    truncate_text_to_fit(msg_display, sizeof(msg_display), font, width - 25);
    
    SDL_Surface *msg_surf = TTF_RenderText_Blended(font, msg_display, CLR_WHITE);
    if (msg_surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, msg_surf);
        SDL_Rect msg_rect = {x + 10, y + 28, msg_surf->w, msg_surf->h};
        SDL_RenderCopy(renderer, tex, NULL, &msg_rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(msg_surf);
    }
}

// Render chat panel for room waiting screen
void render_chat_panel_room(SDL_Renderer *renderer, TTF_Font *font,
                            void *chat_messages, void *input_field,
                            int chat_count) {
    ChatMessage *messages = (ChatMessage*)chat_messages;
    InputField *input = (InputField*)input_field;
    
    int panel_x = 600;
    int panel_y = 120;  // Moved up from 720
    int panel_w = 440;
    int panel_h = 560;  // Increased from 200
    
    // Panel background
    SDL_Rect panel_bg = {panel_x, panel_y, panel_w, panel_h};
    
    // Shadow
    SDL_Rect shadow = {panel_bg.x + 3, panel_bg.y + 3, panel_bg.w, panel_bg.h};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    draw_rounded_rect(renderer, shadow, (SDL_Color){0, 0, 0, 60}, UI_CORNER_RADIUS);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    
    draw_rounded_rect(renderer, panel_bg, CLR_BG, UI_CORNER_RADIUS);
    draw_rounded_border(renderer, panel_bg, CLR_PRIMARY, UI_CORNER_RADIUS, 2);
    
    // Title
    if (font) {
        SDL_Surface *title_surf = TTF_RenderText_Blended(font, "Room Chat", CLR_ACCENT);
        if (title_surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, title_surf);
            SDL_Rect title_rect = {panel_x + 15, panel_y + 8, title_surf->w, title_surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &title_rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(title_surf);
        }
    }
    
    // Messages area (show last 3 messages)
    int msg_area_y = panel_y + 42;
    
    int start_idx = (chat_count > 6) ? (chat_count - 6) : 0;
    int msg_y = msg_area_y;
    
    for (int i = start_idx; i < chat_count && i < start_idx + 6; i++) {
        render_chat_message_block(renderer, font,
                                 messages[i].sender,
                                 messages[i].message,
                                 messages[i].player_id,
                                 messages[i].is_current_user,
                                 panel_x + 10, msg_y, panel_w - 20);
        msg_y += 75;  // Message height + spacing
    }
    
    // Input field at bottom
    input->rect.x = panel_x + 10;
    input->rect.y = panel_y + panel_h - 48;
    input->rect.w = panel_w - 100;
    input->rect.h = 38;
    draw_input_field(renderer, font, input);
    
    // Send button
    SDL_Rect send_btn_rect = {input->rect.x + input->rect.w + 8, input->rect.y, 75, 38};
    
    // Shadow
    SDL_Rect btn_shadow = {send_btn_rect.x + 2, send_btn_rect.y + 2, send_btn_rect.w, send_btn_rect.h};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    draw_rounded_rect(renderer, btn_shadow, (SDL_Color){0, 0, 0, 50}, UI_CORNER_RADIUS);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    
    draw_vertical_gradient(renderer, send_btn_rect, CLR_BTN_NORM, CLR_PRIMARY_DK);
    draw_rounded_border(renderer, send_btn_rect, CLR_PRIMARY_DK, UI_CORNER_RADIUS, 1);
    
    if (font) {
        SDL_Surface *btn_surf = TTF_RenderText_Blended(font, "Send", CLR_WHITE);
        if (btn_surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, btn_surf);
            SDL_Rect btn_text_rect = {
                send_btn_rect.x + (send_btn_rect.w - btn_surf->w) / 2,
                send_btn_rect.y + (send_btn_rect.h - btn_surf->h) / 2,
                btn_surf->w, btn_surf->h
            };
            SDL_RenderCopy(renderer, tex, NULL, &btn_text_rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(btn_surf);
        }
    }
}