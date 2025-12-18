#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include "../common/protocol.h"

#define MAX_BOMBS 50
#define BOMB_TIMER 3000
#define EXPLOSION_TIMER 500
#define POWERUP_CHANCE 30  // 30% cơ hội xuất hiện power-up

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

#define NUM_PREDEFINED_MAPS 7

typedef struct {
    int x,y;
} Position;


typedef enum {
    MAP_CLASSIC,      // 0: Map cổ điển
    MAP_CROSS,        // 1: Hình chữ thập
    MAP_DIAMOND,      // 2: Hình kim cương  
    MAP_CHECKERBOARD, // 3: Bàn cờ
    MAP_SPIRAL,       // 4: Xoắn ốc
    MAP_ARENA,        // 5: Đấu trường
    MAP_MAZE          // 6: Mê cung
} MapType;

// Hàm kiểm tra spawn area 3x3 trống
bool is_spawn_area(int x, int y) {
    return (x <= 2 && y <= 2) ||
           (x >= MAP_WIDTH - 3 && y <= 2) ||
           (x <= 2 && y >= MAP_HEIGHT - 3) ||
           (x >= MAP_WIDTH - 3 && y >= MAP_HEIGHT - 3);
}

// Hàm xóa tường cứng trong spawn area
void clear_spawn_hard_walls(GameState *state) {
    for (int y = 1; y < MAP_HEIGHT - 1; y++) {
        for (int x = 1; x < MAP_WIDTH - 1; x++) {
            if (is_spawn_area(x, y)) {
                if (state->map[y][x] == WALL_HARD) {
                    state->map[y][x] = EMPTY;
                }
            }
        }
    }
}

void generate_hard_walls(GameState *state, MapType map_type) {
    // 1. Khởi tạo toàn bộ map là EMPTY
    memset(state->map, EMPTY, sizeof(state->map));
    
    // 2. Tạo viền tường cứng xung quanh (Border)
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (x == 0 || x == MAP_WIDTH - 1 || y == 0 || y == MAP_HEIGHT - 1) {
                state->map[y][x] = WALL_HARD;
            }
        }
    }
    
    int mid_x = MAP_WIDTH / 2;
    int mid_y = MAP_HEIGHT / 2;

    switch (map_type) {
        case MAP_CLASSIC: 
            // Kiểu truyền thống: Cứ mỗi ô trống là một cột tường
            for (int y = 2; y < MAP_HEIGHT - 2; y += 2) {
                for (int x = 2; x < MAP_WIDTH - 2; x += 2) {
                    state->map[y][x] = WALL_HARD;
                }
            }
            break;
            
        case MAP_CROSS: 
            // Tạo 4 khối tường lớn ở 4 góc, để lại đường đi hình chữ thập ở giữa
            for (int y = 2; y < MAP_HEIGHT - 2; y++) {
                for (int x = 2; x < MAP_WIDTH - 2; x++) {
                    // Nếu không nằm trên trục chữ thập chính giữa
                    if (abs(x - mid_x) > 1 && abs(y - mid_y) > 1) {
                        if (x % 2 == 0 && y % 2 == 0) {
                            state->map[y][x] = WALL_HARD;
                        }
                    }
                }
            }
            break;
            
        case MAP_DIAMOND: 
            // Tạo tường cứng theo hình thoi
            for (int y = 1; y < MAP_HEIGHT - 1; y++) {
                for (int x = 1; x < MAP_WIDTH - 1; x++) {
                    int dist = abs(x - mid_x) + abs(y - mid_y);
                    // Tạo các vòng kim cương cách nhau
                    if (dist == 4 || dist == 8) {
                        if (!is_spawn_area(x, y)) {
                            state->map[y][x] = WALL_HARD;
                        }
                    }
                }
            }
            break;
            
        case MAP_CHECKERBOARD: 
            // Bàn cờ mật độ cao nhưng vẫn đảm bảo di chuyển được
            for (int y = 1; y < MAP_HEIGHT - 1; y++) {
                for (int x = 1; x < MAP_WIDTH - 1; x++) {
                    if ((x + y) % 4 == 0 && !is_spawn_area(x, y)) {
                        state->map[y][x] = WALL_HARD;
                    }
                }
            }
            break;
            
        case MAP_SPIRAL: 
            // Tạo các bức tường dạng xoắn ốc nhưng không khép kín (để tránh kẹt)
            for (int y = 2; y < MAP_HEIGHT - 2; y++) {
                for (int x = 2; x < MAP_WIDTH - 2; x++) {
                    // Logic tạo các rãnh xoắn
                    if (y == 2 && x < MAP_WIDTH - 3) state->map[y][x] = WALL_HARD;
                    else if (x == MAP_WIDTH - 3 && y < MAP_HEIGHT - 3) state->map[y][x] = WALL_HARD;
                    else if (y == MAP_HEIGHT - 3 && x > 3) state->map[y][x] = WALL_HARD;
                    else if (x == 3 && y > 4 && y < MAP_HEIGHT - 4) state->map[y][x] = WALL_HARD;
                }
            }
            // Xóa bớt điểm đầu/cuối xoắn ốc để đảm bảo thông thoáng
            clear_spawn_hard_walls(state);
            break;
            
        case MAP_ARENA: 
            // Đấu trường mở: Chỉ có 4 cột lớn ở 4 khu vực trung tâm
            for (int y = mid_y - 3; y <= mid_y + 3; y++) {
                for (int x = mid_x - 3; x <= mid_x + 3; x++) {
                    if ((x == mid_x - 3 || x == mid_x + 3) && (y == mid_y - 3 || y == mid_y + 3)) {
                        state->map[y][x] = WALL_HARD;
                    }
                }
            }
            break;
            
        case MAP_MAZE: 
            // Mê cung logic: Sử dụng lưới 2x2 để tạo các hẻm nhỏ
            for (int y = 2; y < MAP_HEIGHT - 2; y += 2) {
                for (int x = 2; x < MAP_WIDTH - 2; x += 2) {
                    if (!is_spawn_area(x, y)) {
                        state->map[y][x] = WALL_HARD;
                        // Ngẫu nhiên nối tường sang bên cạnh để tạo cảm giác mê cung
                        int dir = rand() % 4;
                        if (dir == 0 && !is_spawn_area(x+1, y)) state->map[y][x+1] = WALL_HARD;
                        else if (dir == 1 && !is_spawn_area(x, y+1)) state->map[y+1][x] = WALL_HARD;
                    }
                }
            }
            break;
    }
    
    // Đảm bảo cuối cùng các khu vực xuất phát luôn trống tường cứng
    clear_spawn_hard_walls(state);
}

// Tạo tường mềm THÔNG MINH - đảm bảo đường đi
void generate_smart_soft_walls(GameState *state) {
    int total_empty = 0;
    int placed = 0;
    
    // Đếm số ô trống (không phải spawn area)
    for (int y = 1; y < MAP_HEIGHT - 1; y++) {
        for (int x = 1; x < MAP_WIDTH - 1; x++) {
            if (state->map[y][x] == EMPTY && !is_spawn_area(x, y)) {
                total_empty++;
            }
        }
    }
    
    // Mục tiêu: 35% ô trống là tường mềm
    int target_soft_walls = total_empty * 35 / 100;
    
    // Tạo danh sách các ô có thể đặt tường mềm
    for (int attempt = 0; attempt < 5 && placed < target_soft_walls; attempt++) {
        for (int y = 1; y < MAP_HEIGHT - 1; y++) {
            for (int x = 1; x < MAP_WIDTH - 1; x++) {
                if (state->map[y][x] == EMPTY && !is_spawn_area(x, y) && 
                    placed < target_soft_walls) {
                    
                    // Tính khoảng cách đến spawn gần nhất
                    int min_spawn_dist = 100;
                    int spawn_dists[4] = {
                        abs(x-1) + abs(y-1),                    // TL
                        abs(x-(MAP_WIDTH-2)) + abs(y-1),        // TR
                        abs(x-1) + abs(y-(MAP_HEIGHT-2)),       // BL
                        abs(x-(MAP_WIDTH-2)) + abs(y-(MAP_HEIGHT-2)) // BR
                    };
                    
                    for (int i = 0; i < 4; i++) {
                        if (spawn_dists[i] < min_spawn_dist) {
                            min_spawn_dist = spawn_dists[i];
                        }
                    }
                    
                    // Xa spawn thì tỉ lệ cao hơn
                    int probability;
                    if (min_spawn_dist <= 3) probability = 15;      // Gần spawn: 15%
                    else if (min_spawn_dist <= 5) probability = 30; // Vừa: 30%
                    else probability = 45;                         // Xa: 45%
                    
                    // Đảm bảo không chặn đường đi hoàn toàn
                    bool would_block = false;
                    
                    // Kiểm tra 4 hướng xem có ô trống không
                    int empty_neighbors = 0;
                    if (x > 1 && state->map[y][x-1] == EMPTY) empty_neighbors++;
                    if (x < MAP_WIDTH-2 && state->map[y][x+1] == EMPTY) empty_neighbors++;
                    if (y > 1 && state->map[y-1][x] == EMPTY) empty_neighbors++;
                    if (y < MAP_HEIGHT-2 && state->map[y+1][x] == EMPTY) empty_neighbors++;
                    
                    // Nếu chỉ có 1 lối đi duy nhất thì không chặn
                    if (empty_neighbors <= 1) would_block = true;
                    
                    if (!would_block && rand() % 100 < probability) {
                        state->map[y][x] = WALL_SOFT;
                        placed++;
                    }
                }
            }
        }
    }
}

// Kiểm tra map có hợp lý không
bool is_map_reasonable(GameState *state) {
    // Chỉ kiểm tra spawn areas trống
    for (int y = 1; y < MAP_HEIGHT - 1; y++) {
        for (int x = 1; x < MAP_WIDTH - 1; x++) {
            if (is_spawn_area(x, y) && state->map[y][x] != EMPTY) {
                return false;
            }
        }
    }
    
    // Thêm: kiểm tra có ít nhất một đường đi từ mỗi spawn ra ngoài
    Position spawns[4] = {{1,1}, {MAP_WIDTH-2,1}, {1,MAP_HEIGHT-2}, {MAP_WIDTH-2,MAP_HEIGHT-2}};
    
    for (int s = 0; s < 4; s++) {
        int sx = spawns[s].x;
        int sy = spawns[s].y;
        
        // Kiểm tra 4 hướng từ spawn
        bool can_exit = false;
        if (state->map[sy][sx+1] == EMPTY) can_exit = true;  // Phải
        if (state->map[sy][sx-1] == EMPTY) can_exit = true;  // Trái
        if (state->map[sy+1][sx] == EMPTY) can_exit = true;  // Xuống
        if (state->map[sy-1][sx] == EMPTY) can_exit = true;  // Lên
        
        if (!can_exit) {
            printf("Spawn %d at (%d,%d) is trapped!\n", s+1, sx, sy);
            return false;
        }
    }
    
    return true;
}

// Thêm hàm debug này
void print_map_state(GameState *state) {
    printf("\n=== MAP DEBUG ===\n");
    int hard_count = 0, soft_count = 0, empty_count = 0;
    
    for (int y = 0; y < MAP_HEIGHT; y++) {
        printf("%2d: ", y);
        for (int x = 0; x < MAP_WIDTH; x++) {
            char c;
            switch(state->map[y][x]) {
                case EMPTY: c = '.'; empty_count++; break;
                case WALL_HARD: c = '#'; hard_count++; break;
                case WALL_SOFT: c = '%'; soft_count++; break;
                default: c = '?';
            }
            printf("%c", c);
        }
        printf("\n");
    }
    
    printf("\nStatistics:\n");
    printf("Hard walls: %d\n", hard_count);
    printf("Soft walls: %d\n", soft_count);
    printf("Empty tiles: %d\n", empty_count);
}

// Sửa init_map để gọi debug
void init_map(GameState *state) {
    printf("=== INITIALIZING MAP ===\n");
    
    int attempts = 0;
    const int MAX_ATTEMPTS = 20;
    
    do {
        attempts++;
        
        printf("\n--- Attempt %d ---\n", attempts);
        
        // Chọn map ngẫu nhiên
        MapType selected_map = rand() % NUM_PREDEFINED_MAPS;
        printf("Selected map type: %d\n", selected_map);
        
        // Tạo tường cứng
        generate_hard_walls(state, selected_map);
        
        // Tạo tường mềm thông minh
        generate_smart_soft_walls(state);
        
        // In debug
        print_map_state(state);
        
        // Kiểm tra hợp lý
        bool reasonable = is_map_reasonable(state);
        printf("Map reasonable: %s\n", reasonable ? "YES" : "NO");
        
        if (reasonable) break;
        
    } while (attempts < MAX_ATTEMPTS);
    
    if (attempts >= MAX_ATTEMPTS) {
        printf("\n!!! USING FALLBACK MAP !!!\n");
        
        // Reset map
        memset(state->map, EMPTY, sizeof(state->map));
        
        // Tạo viền
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                if (x == 0 || x == MAP_WIDTH - 1 || 
                    y == 0 || y == MAP_HEIGHT - 1) {
                    state->map[y][x] = WALL_HARD;
                }
            }
        }
        
        // Tường cứng cổ điển
        for (int y = 1; y < MAP_HEIGHT - 1; y++) {
            for (int x = 1; x < MAP_WIDTH - 1; x++) {
                if (x % 2 == 0 && y % 2 == 0 && !is_spawn_area(x, y)) {
                    state->map[y][x] = WALL_HARD;
                }
            }
        }
        
        // Tường mềm ít hơn
        for (int y = 1; y < MAP_HEIGHT - 1; y++) {
            for (int x = 1; x < MAP_WIDTH - 1; x++) {
                if (state->map[y][x] == EMPTY && !is_spawn_area(x, y)) {
                    if (rand() % 100 < 25) { // Chỉ 25%
                        state->map[y][x] = WALL_SOFT;
                    }
                }
            }
        }
        
        print_map_state(state);
    }
    
    printf("=== MAP INITIALIZATION COMPLETE ===\n\n");
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
            // TODO: Implement speed in Player struct
            // For now, just acknowledge pickup
            printf("[GAME] Player %s picked up SPEED power-up! (Speed boost not yet implemented)\n", 
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