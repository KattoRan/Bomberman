#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../common/protocol.h"

#define MAX_BOMBS 50
#define BOMB_TIMER 3000
#define EXPLOSION_TIMER 500
#define BOMB_RANGE 2

typedef struct {
    int x, y;
    long long plant_time;
    int is_active;
} Bomb;

typedef struct {
    int x, y;
    long long start_time;
    int is_active;
} Explosion;

Bomb bombs[MAX_BOMBS];
Explosion explosions[MAX_BOMBS * 4];

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
    }
    
    state->game_status = GAME_RUNNING;
    state->winner_id = -1;
    
    printf("[GAME] Initialized with %d players\n", state->num_players);
}

int can_move_to(GameState *state, int x, int y) {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) return 0;
    int tile = state->map[y][x];
    return (tile == EMPTY || tile == EXPLOSION);
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
        return 1;
    }
    return 0;
}

int plant_bomb(GameState *state, int player_id) {
    if (player_id < 0 || player_id >= state->num_players) return 0;
    
    Player *p = &state->players[player_id];
    if (!p->is_alive || state->game_status != GAME_RUNNING) return 0;
    if (state->map[p->y][p->x] == BOMB) return 0;
    
    for (int i = 0; i < MAX_BOMBS; i++) {
        if (!bombs[i].is_active) {
            bombs[i].x = p->x;
            bombs[i].y = p->y;
            bombs[i].plant_time = get_time_ms();
            bombs[i].is_active = 1;
            state->map[p->y][p->x] = BOMB;
            return 1;
        }
    }
    return 0;
}

void create_explosion_line(GameState *state, int sx, int sy, int dx, int dy) {
    for (int i = 0; i <= BOMB_RANGE; i++) {
        int x = sx + dx * i;
        int y = sy + dy * i;
        
        if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) break;
        
        int tile = state->map[y][x];
        if (tile == WALL_HARD) break;
        
        for (int j = 0; j < MAX_BOMBS * 4; j++) {
            if (!explosions[j].is_active) {
                explosions[j].x = x;
                explosions[j].y = y;
                explosions[j].start_time = get_time_ms();
                explosions[j].is_active = 1;
                break;
            }
        }
        
        state->map[y][x] = EXPLOSION;
        if (tile == WALL_SOFT || tile == BOMB) break;
    }
}

void update_game(GameState *state) {
    long long now = get_time_ms();
    
    // Bomb explosions
    for (int i = 0; i < MAX_BOMBS; i++) {
        if (bombs[i].is_active && now - bombs[i].plant_time >= BOMB_TIMER) {
            create_explosion_line(state, bombs[i].x, bombs[i].y, 0, -1);
            create_explosion_line(state, bombs[i].x, bombs[i].y, 0, 1);
            create_explosion_line(state, bombs[i].x, bombs[i].y, -1, 0);
            create_explosion_line(state, bombs[i].x, bombs[i].y, 1, 0);
            bombs[i].is_active = 0;
        }
    }
    
    // Clear explosions
    for (int i = 0; i < MAX_BOMBS * 4; i++) {
        if (explosions[i].is_active && 
            now - explosions[i].start_time >= EXPLOSION_TIMER) {
            state->map[explosions[i].y][explosions[i].x] = EMPTY;
            explosions[i].is_active = 0;
        }
    }
    
    // Check player deaths
    int alive = 0;
    int last_alive = -1;
    for (int i = 0; i < state->num_players; i++) {
        Player *p = &state->players[i];
        if (p->is_alive) {
            if (state->map[p->y][p->x] == EXPLOSION) {
                p->is_alive = 0;
                printf("[GAME] Player %d died!\n", i);
            } else {
                alive++;
                last_alive = i;
            }
        }
    }
    
    // Check game over
    if (state->game_status == GAME_RUNNING && alive <= 1) {
        state->game_status = GAME_ENDED;
        state->winner_id = (alive == 1) ? last_alive : -1;
        printf("[GAME] Game ended. Winner: %d\n", state->winner_id);
    }
}