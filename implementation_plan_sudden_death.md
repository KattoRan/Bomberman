# Sudden Death Mode - Implementation Plan

**Status:** ✅ Complete
**Timeline:** 90s match, walls shrink every 15s

---

## ✅ Done
- Protocol updates (timer and shrink zone fields)
- Server initialization in init_game()
- apply_sudden_death_shrinking() function
- Integration into game loop
- Client-side death zone overlay (pulsing red)
- Color-coded countdown timer


---

## 🔧 TODO

### Protocol (`common/protocol.h`)
Added to `GameState`:
```c
int sudden_death_timer;       // 1800 ticks (90s)
int shrink_zone_left;         // Safe zone bounds
int shrink_zone_right;
int shrink_zone_top;
int shrink_zone_bottom;
```

### 1. Server Init (`server/game_logic.c`)
In `init_game()` after line 82:
```c
// Sudden Death init
if (lobby->game_mode == GAME_MODE_SUDDEN_DEATH) {
    state->sudden_death_timer = 1800;
    state->shrink_zone_left = 0;
    state->shrink_zone_right = MAP_WIDTH - 1;
    state->shrink_zone_top = 0;
    state->shrink_zone_bottom = MAP_HEIGHT - 1;
}
```

### 2. Server Shrinking Logic
Add before `update_game()`:
```c
void apply_sudden_death_shrinking(GameState *state) {
    if (state->game_mode != GAME_MODE_SUDDEN_DEATH) return;
    
    state->sudden_death_timer--;
    int elapsed = 1800 - state->sudden_death_timer;
    
    // Shrink every 300 ticks (15s)
    if (elapsed > 0 && elapsed % 300 == 0) {
        state->shrink_zone_left++;
        state->shrink_zone_right--;
        state->shrink_zone_top++;
        state->shrink_zone_bottom--;
        
        // Kill players in death zone
        for (int i = 0; i < state->num_players; i++) {
            if (state->players[i].is_alive) {
                int px = state->players[i].x;
                int py = state->players[i].y;
                if (px < state->shrink_zone_left || px > state->shrink_zone_right ||
                    py < state->shrink_zone_top || py > state->shrink_zone_bottom) {
                    state->players[i].is_alive = 0;
                }
            }
        }
    }
    
    // Timer expired
    if (state->sudden_death_timer <= 0) {
        state->game_status = GAME_ENDED;
        // Find winner by kills
    }
}
```

Call in `update_game()` at line 280:
```c
apply_sudden_death_shrinking(state);
```

### 3. Client Rendering (`client/graphics.c`)
Add two functions and call in `render_game()`:
- Red overlay for death zones
- Timer countdown (top center)

---

## Quick Reference

**Shrink Schedule:**
- 15s, 30s, 45s, 60s, 75s → walls advance 1 tile
- 90s → game ends, winner = most kills

**Testing:**
1. Create Sudden Death game
2. Timer counts down from 90
3. Walls shrink every 15s
4. Players in red zone die

---

## Note
Previous implementation attempts accidentally deleted code. 
**Be extra careful with line numbers and target content!**
