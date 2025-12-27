#include "graphics.h"

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
