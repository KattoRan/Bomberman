#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <math.h>
#include <string.h>
#include "../common/protocol.h"

#define TILE_SIZE 40
#define WINDOW_WIDTH (MAP_WIDTH * TILE_SIZE)
#define WINDOW_HEIGHT (MAP_HEIGHT * TILE_SIZE + 50)
#define MAX_NOTIFICATIONS 5

extern GameState current_state;

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

void draw_status_bar(SDL_Renderer *renderer, TTF_Font *font) {
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
            Player *p = &current_state.players[0];
            char powerup_text[128];
            snprintf(powerup_text, sizeof(powerup_text), 
                    "Bombs: %d/%d | Range: %d", 
                    p->current_bombs, p->max_bombs, p->bomb_range);
            
            SDL_Surface *pu_surface = TTF_RenderText_Solid(font, powerup_text, 
                                      (SDL_Color){255, 215, 0, 255});
            if (pu_surface) {
                SDL_Texture *pu_texture = SDL_CreateTextureFromSurface(renderer, pu_surface);
                SDL_Rect pu_rect = {WINDOW_WIDTH - pu_surface->w - 10, 
                                   bar_y + 15, pu_surface->w, pu_surface->h};
                SDL_RenderCopy(renderer, pu_texture, NULL, &pu_rect);
                SDL_DestroyTexture(pu_texture);
                SDL_FreeSurface(pu_surface);
            }
        }
    }
}

void render_game(SDL_Renderer *renderer, TTF_Font *font, int tick) {
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

    draw_status_bar(renderer, font);
    draw_notifications(renderer, font);
    SDL_RenderPresent(renderer);
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