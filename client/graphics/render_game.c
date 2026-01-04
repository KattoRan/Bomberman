#include "graphics.h"
#include "color.h"

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
        snprintf(timer_text, sizeof(timer_text), "Countdown: %d:%02d", minutes, seconds);
        
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

    // Leave button
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 200, 50, 50, 220);
    SDL_Rect btn = get_game_leave_button_rect();
    SDL_RenderFillRect(renderer, &btn);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &btn);
    if (font) {
        SDL_Surface *surf = TTF_RenderText_Blended(font, "Leave Match", (SDL_Color){255, 255, 255, 255});
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {
                btn.x + (btn.w - surf->w) / 2,
                btn.y + (btn.h - surf->h) / 2,
                surf->w,
                surf->h
            };
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    draw_notifications(renderer, font);

    draw_sidebar(renderer, font, my_player_id, elapsed_seconds);
}
