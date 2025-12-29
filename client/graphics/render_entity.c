#include "graphics.h"
#include <math.h>
#include <stdlib.h>
#include "color.h"

// ===== BOMB =====
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

// ===== EXPLOSION =====
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


// ===== POWERUP =====
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

// ===== PLAYER =====
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
