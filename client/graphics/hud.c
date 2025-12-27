#include "graphics.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "color.h"

// ================= LAYOUT =================
static const int SIDEBAR_PADDING = 16;
static const int LINE_HEIGHT = 22;
static const int LINE_GAP = 6;
static const int SECTION_GAP_SMALL = 14;
static const int SECTION_GAP = 18;

// ================= LEAVE BUTTON =================
static SDL_Rect game_leave_btn = {WINDOW_WIDTH - 170, MAP_HEIGHT * TILE_SIZE + 18, 160, 40};

SDL_Rect get_game_leave_button_rect() {
    return game_leave_btn;
}

// ================= TEXT HELPERS =================
void draw_text(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, SDL_Color color) {
    if (!font || !text) return;
    SDL_Surface *surf = TTF_RenderText_Blended(font, text, color);
    if (!surf) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_Rect rect = {x, y, surf->w, surf->h};
    SDL_RenderCopy(renderer, tex, NULL, &rect);
    SDL_DestroyTexture(tex);
    SDL_FreeSurface(surf);
}

void draw_divider(SDL_Renderer *renderer, int x, int y, int w) {
    SDL_SetRenderDrawColor(renderer, COLOR_DIVIDER.r, COLOR_DIVIDER.g, COLOR_DIVIDER.b, COLOR_DIVIDER.a);
    SDL_Rect line = {x, y, w, 1};
    SDL_RenderFillRect(renderer, &line);
}


// ================= NOTIFICATION =================
// Hệ thống thông báo
typedef struct {
    char text[128];
    Uint32 start_time;
    int is_active;
    SDL_Color color;
} Notification;

Notification notifications[MAX_NOTIFICATIONS];

void add_notification(const char *text, SDL_Color color) {
    for (int i = 0; i < MAX_NOTIFICATIONS; i++) {
        if (!notifications[i].is_active) {
            strncpy(notifications[i].text, text, 127);
            notifications[i].text[127] = '\0';
            notifications[i].start_time = SDL_GetTicks();
            notifications[i].is_active = 1;
            notifications[i].color = color;
            break;
        }
    }
}

void draw_notifications(SDL_Renderer *renderer, TTF_Font *font) {
    if (!font) return;
    
    Uint32 current_time = SDL_GetTicks();
    int y_offset = 60;
    
    for (int i = 0; i < MAX_NOTIFICATIONS; i++) {
        if (notifications[i].is_active) {
            Uint32 elapsed = current_time - notifications[i].start_time;
            
            if (elapsed > 3000) {
                notifications[i].is_active = 0;
                continue;
            }
            
            int alpha = 255;
            if (elapsed > 2500) {
                alpha = 255 - ((elapsed - 2500) * 255 / 500);
            }
            
            SDL_Color text_color = notifications[i].color;
            text_color.a = alpha;
            
            SDL_Surface *surf = TTF_RenderText_Blended(font, notifications[i].text, text_color);
            if (surf) {
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
                SDL_SetTextureAlphaMod(tex, alpha);
                
                int msg_width = surf->w + 30;
                int msg_height = surf->h + 15;
                int msg_x = (WINDOW_WIDTH - msg_width) / 2;
                int msg_y = y_offset;
                
                SDL_Rect shadow = {msg_x + 2, msg_y + 2, msg_width, msg_height};
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, alpha / 2);
                SDL_RenderFillRect(renderer, &shadow);
                
                SDL_Rect bg = {msg_x, msg_y, msg_width, msg_height};
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, alpha * 3 / 4);
                SDL_RenderFillRect(renderer, &bg);
                
                SDL_SetRenderDrawColor(renderer, text_color.r, text_color.g, text_color.b, alpha);
                SDL_RenderDrawRect(renderer, &bg);
                
                SDL_Rect text_rect = {msg_x + 15, msg_y + 7, surf->w, surf->h};
                SDL_RenderCopy(renderer, tex, NULL, &text_rect);
                
                SDL_DestroyTexture(tex);
                SDL_FreeSurface(surf);
                
                y_offset += msg_height + 10;
            }
        }
    }
}


void draw_status_bar(SDL_Renderer *renderer, TTF_Font *font, int my_player_id) {
    int bar_y = MAP_HEIGHT * TILE_SIZE;
    
    SDL_Rect status_bg = {0, bar_y, WINDOW_WIDTH, 75};
    // Gradient background for status bar
    draw_vertical_gradient(renderer, status_bg, (SDL_Color){40, 40, 50, 255}, (SDL_Color){25, 25, 35, 255});
    
    // Top border glow
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 100, 150, 255, 100);
    SDL_RenderDrawLine(renderer, 0, bar_y, WINDOW_WIDTH, bar_y);
    SDL_RenderDrawLine(renderer, 0, bar_y + 1, WINDOW_WIDTH, bar_y + 1);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    
    if (font) {
        char status_text[256];
        int alive_count = 0;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (current_state.players[i].is_alive) alive_count++;
        }
        
        const char *status_str = "UNKNOWN";
        SDL_Color status_color = {255, 255, 255, 255};
        
        if (current_state.game_status == GAME_WAITING) {
            status_str = "WAITING";
            status_color = (SDL_Color){255, 255, 0, 255};
        } else if (current_state.game_status == GAME_RUNNING) {
            status_str = "PLAYING";
            status_color = (SDL_Color){0, 255, 100, 255};
        } else if (current_state.game_status == GAME_ENDED) {
            status_str = "ENDED";
            status_color = (SDL_Color){255, 100, 100, 255};
        }
        
        snprintf(status_text, sizeof(status_text), 
                "%s | Alive: %d", status_str, alive_count);
        
        SDL_Surface *text_surface = TTF_RenderText_Solid(font, status_text, status_color);
        if (text_surface) {
            SDL_Texture *text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
            SDL_Rect text_rect = {10, bar_y + 18, text_surface->w, text_surface->h};
            SDL_RenderCopy(renderer, text_texture, NULL, &text_rect);
            SDL_DestroyTexture(text_texture);
            SDL_FreeSurface(text_surface);
        }
        
        if (current_state.num_players > 0) {
            Player *p = NULL;
            // Spectator check
            if (my_player_id == -1) {
                // Show spectator mode indicator
                SDL_Surface *spec_surface = TTF_RenderText_Solid(font, "SPECTATOR MODE", 
                                          (SDL_Color){200, 200, 200, 255});
                if (spec_surface) {
                    SDL_Texture *spec_texture = SDL_CreateTextureFromSurface(renderer, spec_surface);
                    int right_padding = game_leave_btn.w + 20;
                    SDL_Rect spec_rect = {WINDOW_WIDTH - spec_surface->w - right_padding, 
                                       bar_y + 15, spec_surface->w, spec_surface->h};
                    SDL_RenderCopy(renderer, spec_texture, NULL, &spec_rect);
                    SDL_DestroyTexture(spec_texture);
                    SDL_FreeSurface(spec_surface);
                }
            } else if (my_player_id >= 0 && my_player_id < MAX_CLIENTS) {
                p = &current_state.players[my_player_id];
                char powerup_text[128];
                snprintf(powerup_text, sizeof(powerup_text), 
                        "Bomb %d/%d | Fire %d", 
                        p->current_bombs, p->max_bombs, p->bomb_range);
                
                SDL_Surface *pu_surface = TTF_RenderText_Solid(font, powerup_text, 
                                          (SDL_Color){255, 215, 0, 255});
                if (pu_surface) {
                    SDL_Texture *pu_texture = SDL_CreateTextureFromSurface(renderer, pu_surface);
                    int right_padding = game_leave_btn.w + 20;
                    SDL_Rect pu_rect = {WINDOW_WIDTH - pu_surface->w - right_padding, 
                                       bar_y + 18, pu_surface->w, pu_surface->h};
                    SDL_RenderCopy(renderer, pu_texture, NULL, &pu_rect);
                    SDL_DestroyTexture(pu_texture);
                    SDL_FreeSurface(pu_surface);
                }
            }
        }
    }
}

void draw_sidebar(SDL_Renderer *renderer, TTF_Font *font, int my_player_id, int elapsed_seconds) {
    if (!font) return;

    int win_w, win_h;
    SDL_GetRendererOutputSize(renderer, &win_w, &win_h);
    int sidebar_x = WINDOW_WIDTH + SIDEBAR_PADDING;
    int sidebar_w = win_w - sidebar_x - SIDEBAR_PADDING;
    if (sidebar_w < 140) return;

    SDL_Rect bg = {WINDOW_WIDTH, 0, win_w - WINDOW_WIDTH, win_h};
    SDL_SetRenderDrawColor(renderer, 18, 18, 18, 255);
    SDL_RenderFillRect(renderer, &bg);

    int y = SIDEBAR_PADDING;
    SDL_Color player_colors[] = {COLOR_PLAYER1, COLOR_PLAYER2, COLOR_PLAYER3, COLOR_PLAYER4};

    draw_text(renderer, font, my_username, sidebar_x, y, COLOR_TEXT_PRIMARY);
    y += LINE_HEIGHT + LINE_GAP;

    const char *tier_name = "Bronze";
    int my_elo = 0;
    if (my_player_id >= 0 && my_player_id < current_state.num_players) {
        my_elo = current_state.players[my_player_id].elo_rating;
    }
    if (my_elo >= 2000) tier_name = "Diamond";
    else if (my_elo >= 1500) tier_name = "Gold";
    else if (my_elo >= 1000) tier_name = "Silver";

    {
        char profile_line[128];
        snprintf(profile_line, sizeof(profile_line), "Rank: %s", tier_name);
        draw_text(renderer, font, profile_line, sidebar_x, y, COLOR_TEXT_MUTED);
        y += LINE_HEIGHT;
        snprintf(profile_line, sizeof(profile_line), "ELO: %d", my_elo);
        draw_text(renderer, font, profile_line, sidebar_x, y, COLOR_TEXT_MUTED);
        y += LINE_HEIGHT + SECTION_GAP_SMALL;
    }

    draw_divider(renderer, sidebar_x, y, sidebar_w - SIDEBAR_PADDING);
    y += LINE_GAP;

    draw_text(renderer, font, "Room", sidebar_x, y, COLOR_TEXT_ACCENT);
    y += LINE_HEIGHT;
    if (current_lobby.id >= 0) {
        char line[160];
        snprintf(line, sizeof(line), "%s", current_lobby.name);
        draw_text(renderer, font, line, sidebar_x, y, COLOR_TEXT_PRIMARY);
        y += LINE_HEIGHT;
        draw_text(renderer, font, current_lobby.is_private ? "Private" : "Public",
                  sidebar_x, y, COLOR_TEXT_MUTED);
        y += LINE_HEIGHT;
        if (current_lobby.is_private && current_lobby.access_code[0]) {
            snprintf(line, sizeof(line), "Code: %s", current_lobby.access_code);
            draw_text(renderer, font, line, sidebar_x, y, COLOR_TEXT_MUTED);
            y += LINE_HEIGHT;
        }
    } else {
        draw_text(renderer, font, "No room info", sidebar_x, y, COLOR_TEXT_MUTED);
        y += LINE_HEIGHT;
    }
    y += SECTION_GAP;

    draw_divider(renderer, sidebar_x, y, sidebar_w - SIDEBAR_PADDING);
    y += LINE_GAP;

    draw_text(renderer, font, "Match", sidebar_x, y, COLOR_TEXT_ACCENT);
    y += LINE_HEIGHT;
    {
        int alive = 0;
        for (int i = 0; i < current_state.num_players; i++) {
            if (current_state.players[i].is_alive) alive++;
        }
        char info[128];
        snprintf(info, sizeof(info), "Time: %02d:%02d", elapsed_seconds / 60, elapsed_seconds % 60);
        draw_text(renderer, font, info, sidebar_x, y, COLOR_TEXT_MUTED);
        y += LINE_HEIGHT + LINE_GAP;
        snprintf(info, sizeof(info), "Alive: %d/%d", alive, current_state.num_players);
        draw_text(renderer, font, info, sidebar_x, y,
                  alive > 1 ? COLOR_TEXT_OK : COLOR_TEXT_BAD);
        y += LINE_HEIGHT;
    }
    y += SECTION_GAP;

    draw_divider(renderer, sidebar_x, y, sidebar_w - SIDEBAR_PADDING);
    y += LINE_GAP;

    draw_text(renderer, font, "Players", sidebar_x, y, COLOR_TEXT_ACCENT);
    y += LINE_HEIGHT;
    for (int i = 0; i < current_state.num_players; i++) {
        Player *p = &current_state.players[i];
        SDL_Color c = p->is_alive ? player_colors[i % 4] : COLOR_TEXT_MUTED;
        SDL_Rect swatch = {sidebar_x, y + 5, 10, 10};
        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
        SDL_RenderFillRect(renderer, &swatch);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &swatch);

        {
            char line[120];
            snprintf(line, sizeof(line), " %s", p->username);
            draw_text(renderer, font, line, sidebar_x + 14, y, c);
        }

        {
            const char *status = p->is_alive ? "A" : "X";
            SDL_Color status_color = p->is_alive ? COLOR_TEXT_OK : COLOR_TEXT_BAD;
            int status_x = sidebar_x + sidebar_w - 20;
            draw_text(renderer, font, status, status_x, y, status_color);
        }
        y += LINE_HEIGHT;
    }
}

// ================= FONT INITIALIZATION =================

TTF_Font* init_font() {
    if (TTF_Init() == -1) {
        fprintf(stderr, "TTF_Init error: %s\n", TTF_GetError());
        return NULL;
    }
    
    // LARGER FONT: 26pt (was 18pt)
    TTF_Font *font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 26);
    if (!font) {
        font = TTF_OpenFont("C:\\Windows\\Fonts\\arial.ttf", 26);
    }
    
    if (!font) {
        fprintf(stderr, "TTF_OpenFont error: %s\n", TTF_GetError());
    }
    
    return font;
}
