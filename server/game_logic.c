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
    
    srand(time(NULL));
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
    
    printf("[GAME] Initialized with %d players\n", state->num_players);
}

int can_move_to(GameState *state, int x, int y) {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) return 0;
    int tile = state->map[y][x];
    return (tile == EMPTY || tile == EXPLOSION || 
            tile == POWERUP_BOMB || tile == POWERUP_FIRE || tile == POWERUP_SPEED);
}

void pickup_powerup(GameState *state, Player *p, int x, int y) {
    int tile = state->map[y][x];
    
    switch (tile) {
        case POWERUP_BOMB:
            p->max_bombs++;
            printf("[GAME] Player %s picked up BOMB power-up! Max bombs: %d\n", 
                   p->username, p->max_bombs);
            state->map[y][x] = EMPTY;
            break;
            
        case POWERUP_FIRE:
            p->bomb_range++;
            printf("[GAME] Player %s picked up FIRE power-up! Range: %d\n", 
                   p->username, p->bomb_range);
            state->map[y][x] = EMPTY;
            break;
            
        case POWERUP_SPEED:
            printf("[GAME] Player %s picked up SPEED power-up!\n", p->username);
            state->map[y][x] = EMPTY;
            break;
    }
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
        pickup_powerup(state, p, nx, ny);
        return 1;
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
            state->map[y][x] = EXPLOSION;
            // SỬA Ở ĐÂY: Chỉ dừng nếu gặp bom KHÁC (i > 0).
            // Nếu i == 0 (tâm bom) thì không break để lửa lan tiếp ra các hướng.
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
        }
    }
    
    int alive = 0;
    int last_alive = -1;
    for (int i = 0; i < state->num_players; i++) {
        Player *p = &state->players[i];
        if (p->is_alive) {
            if (state->map[p->y][p->x] == EXPLOSION) {
                p->is_alive = 0;
                printf("[GAME] Player %s died at (%d,%d)!\n", p->username, p->x, p->y);
            } else {
                alive++;
                last_alive = i;
            }
        }
    }
    
    if (state->game_status == GAME_RUNNING && alive <= 1) {
        state->game_status = GAME_ENDED;
        state->winner_id = (alive == 1) ? last_alive : -1;
        
        if (state->winner_id >= 0) {
            printf("[GAME] Game ended. Winner: %s\n", 
                   state->players[state->winner_id].username);
        } else {
            printf("[GAME] Game ended. Draw!\n");
        }
    }
}