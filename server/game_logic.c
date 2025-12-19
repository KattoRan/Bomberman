#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "../common/protocol.h"

#define MAX_BOMBS 50
#define BOMB_TIMER 3000
#define EXPLOSION_TIMER 500
#define POWERUP_CHANCE 30  // 30% cơ hội xuất hiện power-up
#define FOG_RADIUS 5       // Fog of war visibility radius (5 tiles)

// Power-up limits - ADJUSTED
#define MAX_BOMB_CAPACITY 3   // User requested: max 3 bombs
#define MAX_BOMB_RANGE 4      // User requested: max 4 range
#define MAX_MOVE_SPEED 2.0f
#define BASE_MOVE_SPEED 1.0f

typedef struct {
    int x, y;
    long long plant_time;
    int is_active;
    int owner_id;      // ID người đặt bom
    int range;         // Tầm nổ của bom này
} Bomb;

typedef struct {
    int x, y;
    long long start_time;
    int is_active;
} Explosion;

Bomb bombs[MAX_BOMBS];
Explosion explosions[MAX_BOMBS * 10];

long long get_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

void init_map(GameState *state) {
    memset(state->map, EMPTY, sizeof(state->map));
    
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (x == 0 || x == MAP_WIDTH - 1 || 
                y == 0 || y == MAP_HEIGHT - 1) {
                state->map[y][x] = WALL_HARD;
            }
            else if (x % 2 == 0 && y % 2 == 0) {
                state->map[y][x] = WALL_HARD;
            }
        }
    }
    
    // srand() moved to server startup - removed from here for better randomness
    for (int y = 1; y < MAP_HEIGHT - 1; y++) {
        for (int x = 1; x < MAP_WIDTH - 1; x++) {
            if (state->map[y][x] == EMPTY) {
                int is_spawn = (x <= 2 && y <= 2) ||
                              (x >= MAP_WIDTH - 3 && y <= 2) ||
                              (x <= 2 && y >= MAP_HEIGHT - 3) ||
                              (x >= MAP_WIDTH - 3 && y >= MAP_HEIGHT - 3);
                
                if (!is_spawn && rand() % 100 < 40) {
                    state->map[y][x] = WALL_SOFT;
                }
            }
        }
    }
}

void init_game(GameState *state, Lobby *lobby) {
    memset(state, 0, sizeof(GameState));
    memset(bombs, 0, sizeof(bombs));
    memset(explosions, 0, sizeof(explosions));
    
    init_map(state);
    
    int spawn_pos[4][2] = {
        {1, 1},
        {MAP_WIDTH - 2, 1},
        {1, MAP_HEIGHT - 2},
        {MAP_WIDTH - 2, MAP_HEIGHT - 2}
    };
    
    state->num_players = lobby->num_players;
    for (int i = 0; i < lobby->num_players; i++) {
        state->players[i] = lobby->players[i];
        state->players[i].x = spawn_pos[i][0];
        state->players[i].y = spawn_pos[i][1];
        state->players[i].is_alive = 1;
        state->players[i].max_bombs = 1;
        state->players[i].bomb_range = 2;
        state->players[i].current_bombs = 0;
    }
    
    state->game_status = GAME_RUNNING;
    state->winner_id = -1;
    state->end_game_time = 0;
    
    // Initialize game mode and fog settings from lobby
    state->game_mode = lobby->game_mode;
    state->fog_radius = (lobby->game_mode == GAME_MODE_FOG_OF_WAR) ? FOG_RADIUS : 0;
    
    // Initialize timer
    state->match_start_time = time(NULL);  // Record start time
    state->match_duration_seconds = 0;
    
    // Initialize kill tracking
    for (int i = 0; i < MAX_CLIENTS; i++) {
        state->kills[i] = 0;
        state->elo_changes[i] = 0;  // Initialize ELO changes
    }
    
    printf("[GAME] Initialized with %d players\n", state->num_players);
}

int can_move_to(GameState *state, int x, int y) {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) return 0;
    int tile = state->map[y][x];
    // FIXED: Can't walk through bombs or explosions anymore!
    return (tile == EMPTY || 
            tile == POWERUP_BOMB || tile == POWERUP_FIRE || tile == POWERUP_SPEED);
}

// Returns: 0=nothing, 1=picked up, 2=already at max
int pickup_powerup(GameState *state, Player *p, int x, int y) {
    int tile = state->map[y][x];
    
    switch (tile) {
        case POWERUP_BOMB:
            if (p->max_bombs < MAX_BOMB_CAPACITY) {
                p->max_bombs++;
                printf("[GAME] Player %s picked up BOMB power-up! Max bombs: %d/%d\n", 
                       p->username, p->max_bombs, MAX_BOMB_CAPACITY);
                state->map[y][x] = EMPTY;
                return 1;  // Picked up
            } else {
                printf("[GAME] Player %s already at max bombs (%d)\n", 
                       p->username, MAX_BOMB_CAPACITY);
                state->map[y][x] = EMPTY;  // Still consume it
                return 2;  // At max
            }
            break;
            
        case POWERUP_FIRE:
            if (p->bomb_range < MAX_BOMB_RANGE) {
                p->bomb_range++;
                printf("[GAME] Player %s picked up FIRE power-up! Range: %d/%d\n", 
                       p->username, p->bomb_range, MAX_BOMB_RANGE);
                state->map[y][x] = EMPTY;
                return 1;  // Picked up
            } else {
                printf("[GAME] Player %s already at max range (%d)\n", 
                       p->username, MAX_BOMB_RANGE);
                state->map[y][x] = EMPTY;  // Still consume it
                return 2;  // At max
            }
            break;
            
        case POWERUP_SPEED:
            // Speed power-up not implemented
            printf("[GAME] Player %s picked up SPEED power-up! (Speed boost not implemented)\n", 
                   p->username);
            state->map[y][x] = EMPTY;
            return 1;  // Picked up
    }
    return 0;  // Nothing happened
}

int handle_move(GameState *state, int player_id, int direction) {
    if (player_id < 0 || player_id >= state->num_players) return 0;
    
    Player *p = &state->players[player_id];
    if (!p->is_alive || state->game_status != GAME_RUNNING) return 0;
    
    int nx = p->x, ny = p->y;
    
    switch (direction) {
        case MOVE_UP: ny--; break;
        case MOVE_DOWN: ny++; break;
        case MOVE_LEFT: nx--; break;
        case MOVE_RIGHT: nx++; break;
        default: return 0;
    }
    
    if (can_move_to(state, nx, ny)) {
        p->x = nx;
        p->y = ny;
        int pickup_status = pickup_powerup(state, p, nx, ny);
        return (pickup_status == 0) ? 1 : (pickup_status + 10); // 1=moved, 11=picked up, 12=at max
    }
    return 0;
}

int plant_bomb(GameState *state, int player_id) {
    if (player_id < 0 || player_id >= state->num_players) return 0;
    
    Player *p = &state->players[player_id];
    if (!p->is_alive || state->game_status != GAME_RUNNING) return 0;
    
    if (p->current_bombs >= p->max_bombs) {
        return 0;
    }
    
    if (state->map[p->y][p->x] == BOMB) return 0;
    
    for (int i = 0; i < MAX_BOMBS; i++) {
        if (!bombs[i].is_active) {
            bombs[i].x = p->x;
            bombs[i].y = p->y;
            bombs[i].plant_time = get_time_ms();
            bombs[i].is_active = 1;
            bombs[i].owner_id = player_id;
            bombs[i].range = p->bomb_range;
            state->map[p->y][p->x] = BOMB;
            p->current_bombs++;
            
            printf("[GAME] Player %s planted bomb at (%d,%d), Range: %d, Count: %d/%d\n",
                   p->username, p->x, p->y, bombs[i].range, p->current_bombs, p->max_bombs);
            return 1;
        }
    }
    return 0;
}

void spawn_powerup(GameState *state, int x, int y) {
    int roll = rand() % 100;
    
    if (roll < POWERUP_CHANCE) {
        int type_roll = rand() % 100;
        if (type_roll < 50) {
            state->map[y][x] = POWERUP_BOMB;
            printf("[GAME] Spawned BOMB power-up at (%d, %d)\n", x, y);
        } else {
            state->map[y][x] = POWERUP_FIRE;
            printf("[GAME] Spawned FIRE power-up at (%d, %d)\n", x, y);
        }
    } else {
        state->map[y][x] = EMPTY;
    }
}

void create_explosion_line(GameState *state, int sx, int sy, int dx, int dy, int range) {
    for (int i = 0; i <= range; i++) {
        int x = sx + dx * i;
        int y = sy + dy * i;
        
        if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) break;
        
        int tile = state->map[y][x];
        
        if (tile == WALL_HARD) break;
        
        for (int j = 0; j < MAX_BOMBS * 10; j++) {
            if (!explosions[j].is_active) {
                explosions[j].x = x;
                explosions[j].y = y;
                explosions[j].start_time = get_time_ms();
                explosions[j].is_active = 1;
                break;
            }
        }
        
        if (tile == WALL_SOFT) {
            spawn_powerup(state, x, y);
            break;
        } else if (tile == BOMB) {
            // FIXED: Need to trigger this bomb immediately!
            // Find and detonate the bomb at this position
            for (int b = 0; b < MAX_BOMBS; b++) {
                if (bombs[b].is_active && bombs[b].x == x && bombs[b].y == y) {
                    // Force immediate detonation by setting plant_time to past
                    bombs[b].plant_time = get_time_ms() - BOMB_TIMER - 1;
                    printf("[GAME] Chain reaction! Bomb at (%d,%d) triggered!\n", x, y);
                    break;
                }
            }
            state->map[y][x] = EXPLOSION;
            // Stop explosion propagation after hitting another bomb
            if (i > 0) break;
        } else {
            state->map[y][x] = EXPLOSION;
        }
    }
}

void update_game(GameState *state) {
    long long now = get_time_ms();
    
    for (int i = 0; i < MAX_BOMBS; i++) {
        if (bombs[i].is_active && now - bombs[i].plant_time >= BOMB_TIMER) {
            int x = bombs[i].x;
            int y = bombs[i].y;
            int owner_id = bombs[i].owner_id;
            int range = bombs[i].range;
            
            printf("[GAME] Bomb at (%d,%d) exploding with range %d\n", x, y, range);
            
            create_explosion_line(state, x, y,  0, -1, range);
            create_explosion_line(state, x, y,  0,  1, range);
            create_explosion_line(state, x, y, -1,  0, range);
            create_explosion_line(state, x, y,  1,  0, range);
            
            if (owner_id >= 0 && owner_id < state->num_players) {
                state->players[owner_id].current_bombs--;
                if (state->players[owner_id].current_bombs < 0) {
                    state->players[owner_id].current_bombs = 0;
                }
            }
            
            bombs[i].is_active = 0;
        }
    }
    
    for (int i = 0; i < MAX_BOMBS * 10; i++) {
        if (explosions[i].is_active && 
            now - explosions[i].start_time >= EXPLOSION_TIMER) {
            int x = explosions[i].x;
            int y = explosions[i].y;
            
            if (state->map[y][x] == EXPLOSION) {
                state->map[y][x] = EMPTY;
            }
            explosions[i].is_active = 0;

            // Check if any player is hit by this explosion tile
            for (int p = 0; p < state->num_players; p++) {
                if (state->players[p].is_alive &&
                    state->players[p].x == x && state->players[p].y == y) {
                    
                    state->players[p].is_alive = 0;
                    
                    // Try to find which bomb caused this explosion
                    // Since we don't track explosion source, check recently detonated bombs
                    int killer_id = -1;
                    
                    // Look for a bomb that detonated recently at a position that could reach this explosion
                    for (int b = 0; b < MAX_BOMBS; b++) {
                        if (!bombs[b].is_active) { // Check inactive bombs (recently exploded)
                            // Check if this bomb could have caused explosion at (x, y)
                            int dist_x = abs(bombs[b].x - x);
                            int dist_y = abs(bombs[b].y - y);
                            
                            // If explosion is in line with bomb and within range
                            if ((dist_x == 0 && dist_y <= bombs[b].range) ||
                                (dist_y == 0 && dist_x <= bombs[b].range)) {
                                killer_id = bombs[b].owner_id;
                                break;
                            }
                        }
                    }
                    
                    // Attribute kill (don't count suicide)
                    if (killer_id >= 0 && killer_id != p && killer_id < state->num_players) {
                        state->kills[killer_id]++;
                        printf("[GAME] Player %s killed %s! (Total kills: %d)\n",
                               state->players[killer_id].username,
                               state->players[p].username,
                               state->kills[killer_id]);
                    } else if (killer_id == p) {
                        printf("[GAME] Player %s died from own bomb (suicide)\n",
                               state->players[p].username);
                    } else {
                        printf("[GAME] Player %s died at (%d,%d)! (killer unknown)\n",
                               state->players[p].username, x, y);
                    }
                }
            }
        }
    }
    
    int alive = 0;
    int last_alive = -1;
    for (int i = 0; i < state->num_players; i++) {
        Player *p = &state->players[i];
        if (p->is_alive) {
            // The player death check is now handled when explosions are cleared
            // This block only counts alive players for game end condition
            alive++;
            last_alive = i;
        }
    }
    
    if (state->game_status == GAME_RUNNING && alive <= 1) {
        state->game_status = GAME_ENDED;
        state->winner_id = (alive == 1) ? last_alive : -1;
        
        // Calculate match duration
        long long end_time = time(NULL);
        state->match_duration_seconds = (int)(end_time - state->match_start_time);
        
        if (state->winner_id >= 0) {
            printf("[GAME] Game ended. Winner: %s (Duration: %d seconds)\n", 
                   state->players[state->winner_id].username, state->match_duration_seconds);
        } else {
            printf("[GAME] Game ended. Draw! (Duration: %d seconds)\n", 
                   state->match_duration_seconds);
        }
    }
}

// === FOG OF WAR FUNCTIONS ===

// Check if a tile is visible to a specific player (7x7 square centered on player)
int is_tile_visible(GameState *state, int player_id, int tile_x, int tile_y) {
    // No fog in non-fog-of-war modes
    if (state->game_mode != GAME_MODE_FOG_OF_WAR) return 1;
    
    // Safety check
    if (player_id < 0 || player_id >= state->num_players) return 0;
    
    Player *p = &state->players[player_id];
    
    // Dead players see everything (spectator view)
    if (!p->is_alive) return 1;
    
    // 7x7 square: player at center, so 3 tiles in each direction
    int dist_x = abs(p->x - tile_x);
    int dist_y = abs(p->y - tile_y);
    
    // Visible if within 3 tiles in both x and y directions (creates 7x7 square)
    return (dist_x <= 3 && dist_y <= 3);
}

// Filter game state for a specific player (create per-player view with fog)
void filter_game_state(GameState *full_state, int player_id, GameState *out_filtered) {
    // Start with full copy
    memcpy(out_filtered, full_state, sizeof(GameState));
    
    // No filtering needed in non-fog modes
    if (full_state->game_mode != GAME_MODE_FOG_OF_WAR) return;
    
    // Filter map tiles
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (!is_tile_visible(full_state, player_id, x, y)) {
                // Hide unseen tiles (set to empty or keep hard walls for structure)
                if (full_state->map[y][x] != WALL_HARD) {
                    out_filtered->map[y][x] = EMPTY;
                }
            }
        }
    }
    
    // Filter other players - MOVE THEM OFF-MAP instead of marking dead
    for (int i = 0; i < full_state->num_players; i++) {
        if (i != player_id) {  // Don't hide yourself
            Player *p = &full_state->players[i];
            if (p->is_alive && !is_tile_visible(full_state, player_id, p->x, p->y)) {
                // Hide this player by moving them off-map (don't change is_alive!)
                out_filtered->players[i].x = -100;  // Off-map position
                out_filtered->players[i].y = -100;
            }
        }
    }
    
    printf("[FOG] Filtered game state for player %d (mode=%d, radius=7x7)\n", 
           player_id, full_state->game_mode);
}
