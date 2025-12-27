#include "graphics.h"
#include <math.h>
#include <stdlib.h>

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

