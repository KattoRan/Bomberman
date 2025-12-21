#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "../common/protocol.h"

#define TILE_SIZE 50
#define WINDOW_WIDTH (MAP_WIDTH * TILE_SIZE)
#define WINDOW_HEIGHT (MAP_HEIGHT * TILE_SIZE + 70)
#define MAX_NOTIFICATIONS 5
#define MAX_PARTICLES 200

extern GameState current_state;

// ===== PARTICLE SYSTEM =====
typedef struct {
    float x, y;
    float vx, vy;
    SDL_Color color;
    int lifetime;
    int max_lifetime;
    float size;
    int is_active;
} Particle;

Particle particles[MAX_PARTICLES];

void init_particles() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        particles[i].is_active = 0;
    }
}

void add_particle(float x, float y, float vx, float vy, SDL_Color color, int lifetime, float size) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].is_active) {
            particles[i].x = x;
            particles[i].y = y;
            particles[i].vx = vx;
            particles[i].vy = vy;
            particles[i].color = color;
            particles[i].lifetime = lifetime;
            particles[i].max_lifetime = lifetime;
            particles[i].size = size;
            particles[i].is_active = 1;
            break;
        }
    }
}

void update_particles() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].is_active) {
            particles[i].x += particles[i].vx;
            particles[i].y += particles[i].vy;
            particles[i].lifetime--;
            
            // Gravity effect
            particles[i].vy += 0.1f;
            
            if (particles[i].lifetime <= 0) {
                particles[i].is_active = 0;
            }
        }
    }
}

void render_particles(SDL_Renderer *renderer) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].is_active) {
            float alpha_ratio = (float)particles[i].lifetime / particles[i].max_lifetime;
            int alpha = (int)(255 * alpha_ratio);
            
            SDL_Color c = particles[i].color;
            SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, alpha);
            
            int size = (int)(particles[i].size * (0.5f + alpha_ratio * 0.5f));
            SDL_Rect rect = {(int)particles[i].x - size/2, (int)particles[i].y - size/2, size, size};
            SDL_RenderFillRect(renderer, &rect);
        }
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

// ===== EASING FUNCTIONS =====
float ease_in_out_cubic(float t) {
    return t < 0.5f ? 4 * t * t * t : 1 - pow(-2 * t + 2, 3) / 2;
}

float ease_out_bounce(float t) {
    const float n1 = 7.5625f;
    const float d1 = 2.75f;
    
    if (t < 1 / d1) {
        return n1 * t * t;
    } else if (t < 2 / d1) {
        t -= 1.5f / d1;
        return n1 * t * t + 0.75f;
    } else if (t < 2.5 / d1) {
        t -= 2.25f / d1;
        return n1 * t * t + 0.9375f;
    } else {
        t -= 2.625f / d1;
        return n1 * t * t + 0.984375f;
    }
}

// ===== GRADIENT RENDERING =====
void draw_vertical_gradient(SDL_Renderer *renderer, SDL_Rect rect, SDL_Color top, SDL_Color bottom) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (int y = 0; y < rect.h; y++) {
        float ratio = (float)y / rect.h;
        SDL_Color color = {
            (Uint8)(top.r + (bottom.r - top.r) * ratio),
            (Uint8)(top.g + (bottom.g - top.g) * ratio),
            (Uint8)(top.b + (bottom.b - top.b) * ratio),
            (Uint8)(top.a + (bottom.a - top.a) * ratio)
        };
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_RenderDrawLine(renderer, rect.x, rect.y + y, rect.x + rect.w, rect.y + y);
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void draw_horizontal_gradient(SDL_Renderer *renderer, SDL_Rect rect, SDL_Color left, SDL_Color right) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (int x = 0; x < rect.w; x++) {
        float ratio = (float)x / rect.w;
        SDL_Color color = {
            (Uint8)(left.r + (right.r - left.r) * ratio),
            (Uint8)(left.g + (right.g - left.g) * ratio),
            (Uint8)(left.b + (right.b - left.b) * ratio),
            (Uint8)(left.a + (right.a - left.a) * ratio)
        };
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_RenderDrawLine(renderer, rect.x + x, rect.y, rect.x + x, rect.y + rect.h);
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void draw_glow_circle(SDL_Renderer *renderer, int cx, int cy, int radius, SDL_Color color, int max_alpha) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (int r = radius; r > 0; r--) {
        float ratio = (float)r / radius;
        int alpha = (int)(max_alpha * (1.0f - ratio));
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, alpha);
        
        // Draw circle approximation
        for (int angle = 0; angle < 360; angle += 5) {
            float rad = angle * 3.14159f / 180.0f;
            int x = cx + (int)(r * cos(rad));
            int y = cy + (int)(r * sin(rad));
            SDL_Rect pixel = {x, y, 2, 2};
            SDL_RenderFillRect(renderer, &pixel);
        }
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

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
    // Pulsing glow effect
    float pulse = (sinf(tick * 0.15f) + 1.0f) / 2.0f;  // 0.0 to 1.0
    int glow_radius = 18 + (int)(pulse * 8);
    SDL_Color glow_color = {255, 50, 50, 0};
    draw_glow_circle(renderer, x * TILE_SIZE + TILE_SIZE/2, y * TILE_SIZE + TILE_SIZE/2, 
                     glow_radius, glow_color, 60 + (int)(pulse * 40));
    
    // Breathing bomb body
    int breath = (int)(pulse * 4);
    SDL_Rect bomb_rect = {x * TILE_SIZE + 5 - breath, y * TILE_SIZE + 5 - breath, 
                          TILE_SIZE - 10 + breath*2, TILE_SIZE - 10 + breath*2};
    
    // Gradient fill for bomb (dark to bright red)
    SDL_Color dark_red = {150, 0, 0, 255};
    SDL_Color bright_red = {255, 30, 30, 255};
    SDL_Color bomb_color = ((tick / 10) % 2 == 0) ? bright_red : dark_red;
    
    // Vertical gradient
    draw_vertical_gradient(renderer, bomb_rect, bright_red, dark_red);
    
    // Shine highlight on top
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_Rect shine = {bomb_rect.x + 4, bomb_rect.y + 2, bomb_rect.w - 8, 6};
    SDL_SetRenderDrawColor(renderer, 255, 200, 200, 100);
    SDL_RenderFillRect(renderer, &shine);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    
    // Enhanced fuse with glow
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_Rect fuse = {x * TILE_SIZE + TILE_SIZE/2 - 2, 
                     y * TILE_SIZE + 2, 4, 8};
    SDL_RenderFillRect(renderer, &fuse);
    
    // Fuse spark
    if ((tick / 5) % 2 == 0) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 255, 200, 0, 200);
        SDL_Rect spark = {fuse.x - 1, fuse.y - 2, 6, 4};
        SDL_RenderFillRect(renderer, &spark);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
}

void draw_explosion(SDL_Renderer *renderer, int x, int y, int tick) {
    int cx = x * TILE_SIZE + TILE_SIZE/2;
    int cy = y * TILE_SIZE + TILE_SIZE/2;
    
    // Create particles on first tick
    static int last_explosion_tick[MAP_WIDTH][MAP_HEIGHT] = {0};
    if (tick != last_explosion_tick[x][y]) {
        last_explosion_tick[x][y] = tick;
        // Add explosion particles
        for (int i = 0; i < 15; i++) {
            float angle = (float)(rand() % 360) * 3.14159f / 180.0f;
            float speed = 1.0f + (float)(rand() % 100) / 50.0f;
            SDL_Color part_color = (rand() % 2 == 0) ? 
                (SDL_Color){255, 165, 0, 255} : (SDL_Color){255, 255, 0, 255};
            add_particle(cx, cy, cos(angle) * speed, sin(angle) * speed - 1.0f,
                        part_color, 15 + rand() % 15, 4 + rand() % 4);
        }
    }
    
    // Pulsing expansion
    int pulse = (tick % 20) - 10;
    int size = TILE_SIZE - abs(pulse);
    int offset = (TILE_SIZE - size) / 2;
    
    // Multiple explosion layers with different colors
    // Outer layer - orange
    SDL_Rect outer = {x * TILE_SIZE + offset - 2, y * TILE_SIZE + offset - 2, size + 4, size + 4};
    draw_vertical_gradient(renderer, outer, (SDL_Color){255, 100, 0, 200}, (SDL_Color){255, 69, 0, 150});
    
    // Middle layer - brighter orange
    SDL_Rect middle = {x * TILE_SIZE + offset, y * TILE_SIZE + offset, size, size};
    draw_vertical_gradient(renderer, middle, (SDL_Color){255, 200, 0, 220}, (SDL_Color){255, 140, 0, 180});
    
    // Inner core - yellow/white
    int core_size = size * 2 / 3;
    int core_offset = (TILE_SIZE - core_size) / 2;
    SDL_Rect core = {x * TILE_SIZE + core_offset, y * TILE_SIZE + core_offset, core_size, core_size};
    draw_vertical_gradient(renderer, core, (SDL_Color){255, 255, 200, 240}, (SDL_Color){255, 255, 100, 200});
    
    // Glow around explosion
    int glow_intensity = 20 - abs(pulse);
    draw_glow_circle(renderer, cx, cy, TILE_SIZE/2 + abs(pulse), 
                     (SDL_Color){255, 165, 0, 0}, glow_intensity * 8);
    
    // Border flash
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 150);
    SDL_RenderDrawRect(renderer, &middle);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void draw_powerup(SDL_Renderer *renderer, int x, int y, int type, int tick) {
    // Floating animation
    float float_offset = sinf(tick * 0.08f) * 3.0f;
    
    // Enhanced pulsing glow
    float pulse = (sinf(tick * 0.1f) + 1.0f) / 2.0f;  // 0.0 to 1.0
    int alpha = 200 + (int)(55 * pulse);
    
    int cx = x * TILE_SIZE + TILE_SIZE/2;
    int cy = y * TILE_SIZE + TILE_SIZE/2 + (int)float_offset;
    
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
    
    // Large rotating glow
    int glow_radius = 20 + (int)(pulse * 10);
    draw_glow_circle(renderer, cx, cy, glow_radius, color, 80 + (int)(pulse * 60));
    
    SDL_Rect powerup_rect = {x * TILE_SIZE + 8, y * TILE_SIZE + 8 + (int)float_offset,
                             TILE_SIZE - 16, TILE_SIZE - 16};
    
    // White border glow with enhanced pulsing
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (int i = 3; i >= 0; i--) {
        SDL_Rect border = {
            powerup_rect.x - i, 
            powerup_rect.y - i,
            powerup_rect.w + 2*i, 
            powerup_rect.h + 2*i
        };
        int border_alpha = alpha * (4 - i) / 4;
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, border_alpha);
        SDL_RenderDrawRect(renderer, &border);
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    
    // Gradient fill for powerup
    SDL_Color light_color = {
        (Uint8)(color.r + (255 - color.r) / 2),
        (Uint8)(color.g + (255 - color.g) / 2),
        (Uint8)(color.b + (255 - color.b) / 2),
        alpha
    };
    SDL_Color dark_color = color;
    dark_color.a = alpha;
    draw_vertical_gradient(renderer, powerup_rect, light_color, dark_color);
    
    // Shine highlight
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_Rect shine = {powerup_rect.x + 4, powerup_rect.y + 2, powerup_rect.w - 8, 6};
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 120);
    SDL_RenderFillRect(renderer, &shine);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    
    // Icon symbols
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    
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
    } else if (type == POWERUP_SPEED) {
        // Speed arrows
        SDL_Rect s1 = {cx - 8, cy - 6, 8, 3};
        SDL_Rect s2 = {cx - 4, cy - 2, 10, 3};
        SDL_Rect s3 = {cx, cy + 2, 8, 3};
        SDL_RenderFillRect(renderer, &s1);
        SDL_RenderFillRect(renderer, &s2);
        SDL_RenderFillRect(renderer, &s3);
    }
    
    // Sparkle effects
    if ((tick / 20) % 3 == 0) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        int sparkle_x = x * TILE_SIZE + 6 + (tick % 10);
        int sparkle_y = y * TILE_SIZE + 6 + ((tick + 10) % 10);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200);
        SDL_Rect sparkle = {sparkle_x, sparkle_y, 2, 2};
        SDL_RenderFillRect(renderer, &sparkle);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
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
                    "Bomb %d/%d | Fire %d", 
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

void render_game(SDL_Renderer *renderer, TTF_Font *font, int tick, int my_player_id, int elapsed_seconds) {
    // Update particles
    update_particles();
    
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Enhanced background with gradient
    SDL_Rect bg = {0, 0, WINDOW_WIDTH, MAP_HEIGHT * TILE_SIZE};
    draw_vertical_gradient(renderer, bg, (SDL_Color){20, 100, 20, 255}, (SDL_Color){34, 139, 34, 255});

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
    
    // Render particles on top of everything
    render_particles(renderer);
    
    // Render fog of war overlay (if in fog mode)
    extern void draw_fog_overlay(SDL_Renderer*, GameState*, int);
    draw_fog_overlay(renderer, &current_state, my_player_id);

    // Render sudden death death zones (if in sudden death mode)
    if (current_state.game_mode == GAME_MODE_SUDDEN_DEATH) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        
        // Draw red overlay for death zones
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                int in_death_zone = 0;
                if (x < current_state.shrink_zone_left || x > current_state.shrink_zone_right ||
                    y < current_state.shrink_zone_top || y > current_state.shrink_zone_bottom) {
                    in_death_zone = 1;
                }
                
                if (in_death_zone) {
                    SDL_Rect death_rect = {x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE};
                    // Pulsing red overlay
                    int pulse = (int)(sinf(tick * 0.1f) * 30 + 80);
                    SDL_SetRenderDrawColor(renderer, 255, 0, 0, pulse);
                    SDL_RenderFillRect(renderer, &death_rect);
                }
            }
        }
        
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }

    // === HUD - Match Timer (Top Center) ===
    char timer_text[32];
    SDL_Color timer_color = {255, 255, 255, 255};
    
    if (current_state.game_mode == GAME_MODE_SUDDEN_DEATH) {
        // Display sudden death countdown
        int remaining_ticks = current_state.sudden_death_timer;
        int remaining_seconds = remaining_ticks / 20;  // 20 ticks per second
        int minutes = remaining_seconds / 60;
        int seconds = remaining_seconds % 60;
        snprintf(timer_text, sizeof(timer_text), "⏱ %d:%02d", minutes, seconds);
        
        // Color code based on time remaining
        if (remaining_seconds <= 15) {
            timer_color = (SDL_Color){255, 0, 0, 255};  // Red - critical!
        } else if (remaining_seconds <= 30) {
            timer_color = (SDL_Color){255, 165, 0, 255};  // Orange - warning
        } else if (remaining_seconds <= 60) {
            timer_color = (SDL_Color){255, 255, 0, 255};  // Yellow - caution
        } else {
            timer_color = (SDL_Color){0, 255, 0, 255};  // Green - safe
        }
    } else {
        // Regular match timer
        int minutes = elapsed_seconds / 60;
        int seconds = elapsed_seconds % 60;
        snprintf(timer_text, sizeof(timer_text), "%d:%02d", minutes, seconds);
    }
    
    SDL_Surface *timer_surf = TTF_RenderText_Blended(font, timer_text, timer_color);
    if (timer_surf) {
        SDL_Texture *timer_tex = SDL_CreateTextureFromSurface(renderer, timer_surf);
        int timer_x = (WINDOW_WIDTH - timer_surf->w) / 2;  // Center horizontally
        int timer_y = 20;  // Top of screen
        SDL_Rect timer_rect = {timer_x, timer_y, timer_surf->w, timer_surf->h};
        SDL_RenderCopy(renderer, timer_tex, NULL, &timer_rect);
        SDL_DestroyTexture(timer_tex);
        SDL_FreeSurface(timer_surf);
    }

    draw_status_bar(renderer, font, my_player_id);
    draw_notifications(renderer, font);
    //SDL_RenderPresent(renderer);
}

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

// ===== FOG OF WAR OVERLAY =====
void draw_fog_overlay(SDL_Renderer *renderer, GameState *state, int my_player_id) {
    // No fog in non-fog-of-war modes
    if (state->game_mode != GAME_MODE_FOG_OF_WAR) return;
    
    // Dead players see everything
    if (my_player_id >= 0 && my_player_id < state->num_players) {
        Player *my_player = &state->players[my_player_id];
        if (!my_player->is_alive) return;  // Spectator view
    }
    
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            // Calculate if this tile should be visible (7x7 square)
            int visible = 1;
            if (my_player_id >= 0 && my_player_id < state->num_players) {
                Player *p = &state->players[my_player_id];
                int dist_x = abs(p->x - x);
                int dist_y = abs(p->y - y);
                // 7x7 square: 3 tiles in each direction from player
                visible = (dist_x <= 3 && dist_y <= 3);
            }
            
            if (!visible) {
                // Draw dark overlay for unseen tiles
                SDL_Rect fog_rect = {x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE};
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);  // Dark overlay
                SDL_RenderFillRect(renderer, &fog_rect);
            }
        }
    }
    
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}
