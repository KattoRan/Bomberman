/* client/handlers/game.c */
#include "../state/client_state.h"
#include "../graphics/graphics.h" // for add_notification

void check_game_changes() {
    // Kiểm tra người chơi chết
    for (int i = 0; i < current_state.num_players; i++) {
        if (previous_state.players[i].is_alive && !current_state.players[i].is_alive) {
            char msg[128];
            snprintf(msg, sizeof(msg), "%s has been defeated!", 
                    current_state.players[i].username);
            add_notification(msg, (SDL_Color){255, 68, 68, 255});
        }
        
        // Kiểm tra power-up
        if (current_state.players[i].max_bombs > previous_state.players[i].max_bombs) {
            if (i == my_player_id) { // Chỉ thông báo cho người chơi hiện tại
                add_notification("Picked up BOMB power-up! +1 Bomb", 
                               (SDL_Color){255, 215, 0, 255});
            }
        }
        
        if (current_state.players[i].bomb_range > previous_state.players[i].bomb_range) {
            if (i == my_player_id) {
                add_notification("Picked up FIRE power-up! +1 Blast Range", 
                               (SDL_Color){255, 69, 0, 255});
            }
        }
    }
    
    // Cập nhật previous state
    memcpy(&previous_state, &current_state, sizeof(GameState));
}