#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <math.h>
#include <string.h>
#include "../common/protocol.h"

#define TILE_SIZE 40
#define WINDOW_WIDTH (MAP_WIDTH * TILE_SIZE)
#define WINDOW_HEIGHT (MAP_HEIGHT * TILE_SIZE + 50)
#define MAX_NOTIFICATIONS 5
#define MAX_EVENTS 3

// Sidebar layout
static const int SIDEBAR_PADDING = 16;
static const int LINE_HEIGHT = 18;
static const int LINE_GAP = 5;
static const int SECTION_GAP_SMALL = 16; // after profile
static const int SECTION_GAP = 18;       // between main sections
static const int FEED_LIMIT = 3;

extern GameState current_state;
extern Lobby current_lobby;
extern char my_username[MAX_USERNAME];

// Màu sắc
const SDL_Color COLOR_BACKGROUND = {34, 139, 34, 255};
const SDL_Color COLOR_WALL_HARD = {64, 64, 64, 255};
const SDL_Color COLOR_WALL_SOFT = {139, 69, 19, 255};
const SDL_Color COLOR_BOMB = {255, 0, 0, 255};
const SDL_Color COLOR_EXPLOSION = {255, 165, 0, 255};
const SDL_Color COLOR_PLAYER1 = {0, 0, 255, 255};
const SDL_Color COLOR_PLAYER2 = {255, 255, 0, 255};
const SDL_Color COLOR_PLAYER3 = {255, 0, 255, 255};
const SDL_Color COLOR_PLAYER4 = {0, 255, 255, 255};
const SDL_Color COLOR_POWERUP_BOMB = {255, 215, 0, 255};
const SDL_Color COLOR_POWERUP_FIRE = {255, 69, 0, 255};
const SDL_Color COLOR_POWERUP_SPEED = {50, 205, 50, 255};

// Text palette for sidebar
static const SDL_Color COLOR_TEXT_PRIMARY = {237, 237, 237, 255}; // #EDEDED
static const SDL_Color COLOR_TEXT_MUTED   = {168, 168, 168, 255}; // #A8A8A8
static const SDL_Color COLOR_TEXT_ACCENT  = {205, 187, 138, 255}; // #CDBB8A
static const SDL_Color COLOR_TEXT_OK      = {61, 220, 151, 255};  // #3DDC97
static const SDL_Color COLOR_TEXT_BAD     = {220, 60, 60, 255};
static const SDL_Color COLOR_DIVIDER      = {42, 42, 42, 255};    // #2A2A2A
static const SDL_Color COLOR_BADGE_PRIVATE_BG = {34, 34, 34, 255}; // subtle badge
static const SDL_Color COLOR_BADGE_PRIVATE_TX = {168, 168, 168, 255};

// Leave button placement in game HUD
static SDL_Rect game_leave_btn = {MAP_WIDTH * TILE_SIZE - 150, MAP_HEIGHT * TILE_SIZE + 10, 140, 30};

// Hệ thống thông báo
typedef struct {
    char text[128];
    Uint32 start_time;
    int is_active;
    SDL_Color color;
} Notification;

Notification notifications[MAX_NOTIFICATIONS];

// Simple event log (latest first)
static char event_log[MAX_EVENTS][128];
static int event_count = 0;

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

void add_event_log(const char *text) {
    if (!text || !text[0]) return;
    char trimmed[128];
    size_t len = strlen(text);
    if (len >= sizeof(trimmed)) len = sizeof(trimmed) - 1;
    strncpy(trimmed, text, len);
    trimmed[len] = '\0';
    if (len > 60) {
        // Truncate long lines
        trimmed[57] = '.';
        trimmed[58] = '.';
        trimmed[59] = '.';
        trimmed[60] = '\0';
    }
    // Shift down to make room for newest at index 0
    for (int i = MAX_EVENTS - 1; i > 0; i--) {
        strncpy(event_log[i], event_log[i - 1], sizeof(event_log[i]) - 1);
        event_log[i][sizeof(event_log[i]) - 1] = '\0';
    }
    strncpy(event_log[0], trimmed, sizeof(event_log[0]) - 1);
    event_log[0][sizeof(event_log[0]) - 1] = '\0';
    if (event_count < MAX_EVENTS) event_count++;
}

// Expose leave button rect for input handling
SDL_Rect get_game_leave_button_rect() {
    return game_leave_btn;
}

// Draw text helper
static void draw_text(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, SDL_Color color) {
    if (!font || !text) return;
    SDL_Surface *surf = TTF_RenderText_Blended(font, text, color);
    if (!surf) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_Rect rect = {x, y, surf->w, surf->h};
    SDL_RenderCopy(renderer, tex, NULL, &rect);
    SDL_DestroyTexture(tex);
    SDL_FreeSurface(surf);
}

static void draw_divider(SDL_Renderer *renderer, int x, int y, int w) {
    SDL_SetRenderDrawColor(renderer, COLOR_DIVIDER.r, COLOR_DIVIDER.g, COLOR_DIVIDER.b, COLOR_DIVIDER.a);
    SDL_Rect line = {x, y, w, 1};
    SDL_RenderFillRect(renderer, &line);
}

void draw_tile(SDL_Renderer *renderer, int x, int y, SDL_Color color, int draw_border) {
    SDL_Rect rect = {x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE};
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);
    
    if (draw_border) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &rect);
    }
}

void draw_bomb(SDL_Renderer *renderer, int x, int y, int tick) {
    SDL_Rect bomb_rect = {x * TILE_SIZE + 5, y * TILE_SIZE + 5, 
                          TILE_SIZE - 10, TILE_SIZE - 10};
    
    if ((tick / 10) % 2 == 0) {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 200, 0, 0, 255);
    }
    SDL_RenderFillRect(renderer, &bomb_rect);
    
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_Rect fuse = {x * TILE_SIZE + TILE_SIZE/2 - 2, 
                     y * TILE_SIZE + 2, 4, 8};
    SDL_RenderFillRect(renderer, &fuse);
}

void draw_explosion(SDL_Renderer *renderer, int x, int y, int tick) {
    int pulse = (tick % 20) - 10;
    int size = TILE_SIZE - abs(pulse);
    int offset = (TILE_SIZE - size) / 2;
    
    SDL_Rect exp_rect = {x * TILE_SIZE + offset, y * TILE_SIZE + offset, 
                         size, size};
    
    SDL_SetRenderDrawColor(renderer, 255, 165, 0, 200);
    SDL_RenderFillRect(renderer, &exp_rect);
    
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
    SDL_RenderDrawRect(renderer, &exp_rect);
}

void draw_powerup(SDL_Renderer *renderer, int x, int y, int type, int tick) {
    int alpha = 200 + (int)(55 * sin(tick * 0.1));
    
    SDL_Rect powerup_rect = {x * TILE_SIZE + 8, y * TILE_SIZE + 8,
                             TILE_SIZE - 16, TILE_SIZE - 16};
    
    SDL_Color color;
    switch (type) {
        case POWERUP_BOMB:
            color = COLOR_POWERUP_BOMB;
            break;
        case POWERUP_FIRE:
            color = COLOR_POWERUP_FIRE;
            break;
        case POWERUP_SPEED:
            color = COLOR_POWERUP_SPEED;
            break;
        default:
            return;
    }
    
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, alpha);
    for (int i = 0; i < 3; i++) {
        SDL_Rect border = {
            powerup_rect.x - i, 
            powerup_rect.y - i,
            powerup_rect.w + 2*i, 
            powerup_rect.h + 2*i
        };
        SDL_RenderDrawRect(renderer, &border);
    }
    
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, alpha);
    SDL_RenderFillRect(renderer, &powerup_rect);
    
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    int cx = x * TILE_SIZE + TILE_SIZE/2;
    int cy = y * TILE_SIZE + TILE_SIZE/2;
    
    if (type == POWERUP_BOMB) {
        SDL_Rect b1 = {cx - 6, cy - 8, 3, 16};
        SDL_Rect b2 = {cx - 3, cy - 8, 9, 3};
        SDL_Rect b3 = {cx - 3, cy - 2, 9, 3};
        SDL_Rect b4 = {cx - 3, cy + 5, 9, 3};
        SDL_RenderFillRect(renderer, &b1);
        SDL_RenderFillRect(renderer, &b2);
        SDL_RenderFillRect(renderer, &b3);
        SDL_RenderFillRect(renderer, &b4);
    } else if (type == POWERUP_FIRE) {
        SDL_Rect f1 = {cx - 6, cy - 8, 3, 16};
        SDL_Rect f2 = {cx - 3, cy - 8, 9, 3};
        SDL_Rect f3 = {cx - 3, cy - 2, 7, 3};
        SDL_RenderFillRect(renderer, &f1);
        SDL_RenderFillRect(renderer, &f2);
        SDL_RenderFillRect(renderer, &f3);
    }
}

void draw_player(SDL_Renderer *renderer, Player *p, SDL_Color color) {
    if (!p->is_alive) return;
    
    SDL_Rect body = {p->x * TILE_SIZE + 8, p->y * TILE_SIZE + 8, 
                     TILE_SIZE - 16, TILE_SIZE - 16};
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &body);
    
    SDL_Rect head = {p->x * TILE_SIZE + 12, p->y * TILE_SIZE + 4, 
                     TILE_SIZE - 24, TILE_SIZE - 28};
    SDL_RenderFillRect(renderer, &head);
    
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(renderer, &body);
    SDL_RenderDrawRect(renderer, &head);
    
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Rect eye1 = {p->x * TILE_SIZE + 14, p->y * TILE_SIZE + 10, 4, 4};
    SDL_Rect eye2 = {p->x * TILE_SIZE + 22, p->y * TILE_SIZE + 10, 4, 4};
    SDL_RenderFillRect(renderer, &eye1);
    SDL_RenderFillRect(renderer, &eye2);
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
    
    SDL_Rect status_bg = {0, bar_y, WINDOW_WIDTH, 50};
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderFillRect(renderer, &status_bg);
    
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
            status_color = (SDL_Color){0, 255, 0, 255};
        } else if (current_state.game_status == GAME_ENDED) {
            status_str = "ENDED";
            status_color = (SDL_Color){255, 0, 0, 255};
        }
        
        snprintf(status_text, sizeof(status_text), 
                "%s | Alive: %d", status_str, alive_count);
        
        SDL_Surface *text_surface = TTF_RenderText_Solid(font, status_text, status_color);
        if (text_surface) {
            SDL_Texture *text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
            SDL_Rect text_rect = {10, bar_y + 15, text_surface->w, text_surface->h};
            SDL_RenderCopy(renderer, text_texture, NULL, &text_rect);
            SDL_DestroyTexture(text_texture);
            SDL_FreeSurface(text_surface);
        }
        
        if (current_state.num_players > 0) {
            Player *p = NULL;
            if (my_player_id >= 0 && my_player_id < MAX_CLIENTS) {
                p = &current_state.players[my_player_id];
            } else {
                p = &current_state.players[0]; // Fallback
            }
            char powerup_text[128];
            snprintf(powerup_text, sizeof(powerup_text), 
                    "Bombs: %d/%d | Range: %d", 
                    p->current_bombs, p->max_bombs, p->bomb_range);
            
            SDL_Surface *pu_surface = TTF_RenderText_Solid(font, powerup_text, 
                                      (SDL_Color){255, 215, 0, 255});
            if (pu_surface) {
                SDL_Texture *pu_texture = SDL_CreateTextureFromSurface(renderer, pu_surface);
                // Shift left a bit to leave space for the leave button
                SDL_Rect pu_rect = {WINDOW_WIDTH - pu_surface->w - 170, 
                                   bar_y + 15, pu_surface->w, pu_surface->h};
                SDL_RenderCopy(renderer, pu_texture, NULL, &pu_rect);
                SDL_DestroyTexture(pu_texture);
                SDL_FreeSurface(pu_surface);
            }
        }
    }
}

static void draw_sidebar(SDL_Renderer *renderer, TTF_Font *font, int my_player_id, int window_w, int window_h) {
    int sidebar_x = MAP_WIDTH * TILE_SIZE + SIDEBAR_PADDING;
    int sidebar_w = window_w - sidebar_x - SIDEBAR_PADDING;
    if (sidebar_w < 140) return;

    SDL_Rect bg = {sidebar_x - SIDEBAR_PADDING, 0, sidebar_w + SIDEBAR_PADDING, window_h};
    SDL_SetRenderDrawColor(renderer, 18, 18, 18, 255);
    SDL_RenderFillRect(renderer, &bg);

    int y = SIDEBAR_PADDING;
    SDL_Color player_colors[] = {COLOR_PLAYER1, COLOR_PLAYER2, COLOR_PLAYER3, COLOR_PLAYER4};

    // --- Profile ---
    draw_text(renderer, font, my_username, sidebar_x, y, COLOR_TEXT_PRIMARY);
    y += LINE_HEIGHT + LINE_GAP;
    // Derive tier from ELO if available
    const char *tier_name = "Bronze";
    int my_elo = 0;
    if (my_player_id >= 0 && my_player_id < current_state.num_players) {
        my_elo = current_state.players[my_player_id].elo_rating;
    }
    if (my_elo >= 2000) tier_name = "Diamond";
    else if (my_elo >= 1500) tier_name = "Gold";
    else if (my_elo >= 1000) tier_name = "Silver";
    char profile_line[128];
    snprintf(profile_line, sizeof(profile_line), "Rank: %s", tier_name);
    draw_text(renderer, font, profile_line, sidebar_x, y, COLOR_TEXT_MUTED);
    y += LINE_HEIGHT;
    snprintf(profile_line, sizeof(profile_line), "ELO: %d", my_elo);
    draw_text(renderer, font, profile_line, sidebar_x, y, COLOR_TEXT_MUTED);
    y += LINE_HEIGHT + SECTION_GAP_SMALL;
    draw_divider(renderer, sidebar_x, y, sidebar_w - SIDEBAR_PADDING);
    y += LINE_GAP;

    // --- Room summary ---
    draw_text(renderer, font, "Room", sidebar_x, y, COLOR_TEXT_ACCENT);
    y += LINE_HEIGHT;
    if (current_lobby.id >= 0) {
        char line[160];
        snprintf(line, sizeof(line), "%s", current_lobby.name);
        draw_text(renderer, font, line, sidebar_x, y, COLOR_TEXT_PRIMARY);
        y += LINE_HEIGHT;
        if (current_lobby.is_private) {
            draw_text(renderer, font, "Private", sidebar_x, y, COLOR_TEXT_MUTED);
            y += LINE_HEIGHT;
        } else {
            draw_text(renderer, font, "Public", sidebar_x, y, COLOR_TEXT_MUTED);
            y += LINE_HEIGHT;
        }
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

    // --- Match ---
    extern Uint32 match_start_time;
    Uint32 now = SDL_GetTicks();
    int elapsed = match_start_time ? (int)((now - match_start_time) / 1000) : 0;
    int alive = 0;
    for (int i = 0; i < current_state.num_players; i++) if (current_state.players[i].is_alive) alive++;

    draw_text(renderer, font, "Match", sidebar_x, y, COLOR_TEXT_ACCENT);
    y += LINE_HEIGHT;
    char info[128];
    snprintf(info, sizeof(info), "Time: %02d:%02d", elapsed/60, elapsed%60);
    draw_text(renderer, font, info, sidebar_x, y, COLOR_TEXT_MUTED); y += LINE_HEIGHT + LINE_GAP;
    snprintf(info, sizeof(info), "Alive: %d/%d", alive, current_state.num_players);
    draw_text(renderer, font, info, sidebar_x, y, alive > 1 ? COLOR_TEXT_OK : COLOR_TEXT_BAD); y += LINE_HEIGHT;
    y += SECTION_GAP;
    draw_divider(renderer, sidebar_x, y, sidebar_w - SIDEBAR_PADDING);
    y += LINE_GAP;

    // --- Players ---
    draw_text(renderer, font, "Players", sidebar_x, y, COLOR_TEXT_ACCENT);
    y += LINE_HEIGHT;
    for (int i = 0; i < current_state.num_players; i++) {
        Player *p = &current_state.players[i];
        SDL_Color c = p->is_alive ? player_colors[i % 4] : COLOR_TEXT_MUTED;
        SDL_Rect swatch = {sidebar_x, y + 4, 10, 10};
        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
        SDL_RenderFillRect(renderer, &swatch);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &swatch);

        char line[120];
        snprintf(line, sizeof(line), " %s", p->username);
        draw_text(renderer, font, line, sidebar_x + 14, y, c);
        const char *status = p->is_alive ? "A" : "X";
        SDL_Color status_color = p->is_alive ? COLOR_TEXT_OK : COLOR_TEXT_BAD;
        int status_x = sidebar_x + sidebar_w - 20;
        draw_text(renderer, font, status, status_x, y, status_color);
        y += LINE_HEIGHT;
    }
    y += SECTION_GAP;

    // --- Recent events ---
    // Recent feed removed per latest UX request
}

void render_game(SDL_Renderer *renderer, TTF_Font *font, int tick, int my_player_id) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, COLOR_BACKGROUND.r, COLOR_BACKGROUND.g, 
                          COLOR_BACKGROUND.b, COLOR_BACKGROUND.a);
    SDL_Rect bg = {0, 0, WINDOW_WIDTH, MAP_HEIGHT * TILE_SIZE};
    SDL_RenderFillRect(renderer, &bg);

    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            switch (current_state.map[y][x]) {
                case WALL_HARD:
                    draw_tile(renderer, x, y, COLOR_WALL_HARD, 1);
                    break;
                case WALL_SOFT:
                    draw_tile(renderer, x, y, COLOR_WALL_SOFT, 1);
                    break;
                case BOMB:
                    draw_bomb(renderer, x, y, tick);
                    break;
                case EXPLOSION:
                    draw_explosion(renderer, x, y, tick);
                    break;
                case POWERUP_BOMB:
                case POWERUP_FIRE:
                case POWERUP_SPEED:
                    draw_powerup(renderer, x, y, current_state.map[y][x], tick);
                    break;
                case EMPTY:
                    SDL_SetRenderDrawColor(renderer, 40, 120, 40, 100);
                    SDL_Rect grid = {x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE};
                    SDL_RenderDrawRect(renderer, &grid);
                    break;
            }
        }
    }

    SDL_Color player_colors[] = {COLOR_PLAYER1, COLOR_PLAYER2, 
                                 COLOR_PLAYER3, COLOR_PLAYER4};
    for (int i = 0; i < current_state.num_players; i++) {
        draw_player(renderer, &current_state.players[i], 
                   player_colors[i % 4]);
    }

    draw_status_bar(renderer, font, my_player_id);

    // Draw Leave button
    SDL_SetRenderDrawColor(renderer, 200, 50, 50, 200);
    SDL_RenderFillRect(renderer, &game_leave_btn);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &game_leave_btn);
    if (font) {
        SDL_Surface *surf = TTF_RenderText_Blended(font, "Leave Match", (SDL_Color){255,255,255,255});
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {game_leave_btn.x + (game_leave_btn.w - surf->w)/2,
                             game_leave_btn.y + (game_leave_btn.h - surf->h)/2,
                             surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
    }

    draw_notifications(renderer, font);

    // Sidebar on the right (uses actual window width)
    int win_w, win_h;
    SDL_GetRendererOutputSize(renderer, &win_w, &win_h);
    draw_sidebar(renderer, font, my_player_id, win_w, win_h);
    //SDL_RenderPresent(renderer);
}

TTF_Font* init_font() {
    if (TTF_Init() == -1) {
        fprintf(stderr, "TTF_Init error: %s\n", TTF_GetError());
        return NULL;
    }
    
    TTF_Font *font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 18);
    if (!font) {
        font = TTF_OpenFont("C:\\Windows\\Fonts\\arial.ttf", 18);
    }
    
    if (!font) {
        fprintf(stderr, "TTF_OpenFont error: %s\n", TTF_GetError());
    }
    
    return font;
}
