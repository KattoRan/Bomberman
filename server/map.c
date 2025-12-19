#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#include "../common/protocol.h"
#define NUM_PREDEFINED_MAPS 7

typedef struct {
    int x, y;
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

/* =========================
   SPAWN AREA HELPERS
   ========================= */

bool is_spawn_area(int x, int y) {
    return (x <= 2 && y <= 2) ||
           (x >= MAP_WIDTH - 3 && y <= 2) ||
           (x <= 2 && y >= MAP_HEIGHT - 3) ||
           (x >= MAP_WIDTH - 3 && y >= MAP_HEIGHT - 3);
}

void clear_spawn_hard_walls(GameState *state) {
    for (int y = 1; y < MAP_HEIGHT - 1; y++) {
        for (int x = 1; x < MAP_WIDTH - 1; x++) {
            if (is_spawn_area(x, y) && state->map[y][x] == WALL_HARD) {
                state->map[y][x] = EMPTY;
            }
        }
    }

    // mở đường 2 ô cạnh spawn theo phong cách bomberman
    // TOP-LEFT
    state->map[1][2] = EMPTY;
    state->map[2][1] = EMPTY;

    // TOP-RIGHT
    state->map[1][MAP_WIDTH - 3] = EMPTY;
    state->map[2][MAP_WIDTH - 2] = EMPTY;

    // BOTTOM-LEFT
    state->map[MAP_HEIGHT - 2][2] = EMPTY;
    state->map[MAP_HEIGHT - 3][1] = EMPTY;

    // BOTTOM-RIGHT
    state->map[MAP_HEIGHT - 2][MAP_WIDTH - 3] = EMPTY;
    state->map[MAP_HEIGHT - 3][MAP_WIDTH - 2] = EMPTY;
}

/* =========================
   CONNECTIVITY (HARD TOPOLOGY)
   - WALL_HARD là không đi qua
   - EMPTY và WALL_SOFT đều "passable" cho topology check
   ========================= */

static bool in_bounds(int x, int y) {
    return x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT;
}

static bool is_passable(GameState *state, int x, int y) {
    (void)state;
    return state->map[y][x] != WALL_HARD;
}

static int flood_count(GameState *state, int sx, int sy, bool visited[MAP_HEIGHT][MAP_WIDTH]) {
    Position q[MAP_WIDTH * MAP_HEIGHT];
    int head = 0, tail = 0;
    int count = 0;

    visited[sy][sx] = true;
    q[tail++] = (Position){sx, sy};

    while (head < tail) {
        Position p = q[head++];
        count++;

        const int dx[4] = {1, -1, 0, 0};
        const int dy[4] = {0, 0, 1, -1};

        for (int i = 0; i < 4; i++) {
            int nx = p.x + dx[i];
            int ny = p.y + dy[i];

            if (!in_bounds(nx, ny)) continue;
            if (visited[ny][nx]) continue;
            if (!is_passable(state, nx, ny)) continue;

            visited[ny][nx] = true;
            q[tail++] = (Position){nx, ny};
        }
    }

    return count;
}

// TRUE nếu tất cả ô != WALL_HARD thuộc 1 connected component
static bool map_is_connected(GameState *state) {
    bool visited[MAP_HEIGHT][MAP_WIDTH] = {0};

    int sx = -1, sy = -1;
    int total_passable = 0;

    for (int y = 1; y < MAP_HEIGHT - 1; y++) {
        for (int x = 1; x < MAP_WIDTH - 1; x++) {
            if (state->map[y][x] != WALL_HARD) {
                total_passable++;
                if (sx == -1) { sx = x; sy = y; }
            }
        }
    }

    if (sx == -1) return false; // map toàn hard -> lỗi

    int reachable = flood_count(state, sx, sy, visited);
    return reachable == total_passable;
}

/* =========================
   HARD WALL PLACEMENT SAFETY
   ========================= */

static int count_hard_walls(GameState *state) {
    int count = 0;
    for (int y = 1; y < MAP_HEIGHT - 1; y++) {
        for (int x = 1; x < MAP_WIDTH - 1; x++) {
            if (state->map[y][x] == WALL_HARD) count++;
        }
    }
    return count;
}

// kiểm tra hình học cơ bản (nhanh) + không đặt sát spawn
static bool can_place_hard_wall_geom(GameState *state, int x, int y) {
    if (!in_bounds(x, y)) return false;
    if (x <= 0 || x >= MAP_WIDTH - 1 || y <= 0 || y >= MAP_HEIGHT - 1) return false;
    if (state->map[y][x] != EMPTY) return false;
    if (is_spawn_area(x, y)) return false;

    // Không sát spawn (1 ô quanh spawn)
    if (abs(x - 1) <= 1 && abs(y - 1) <= 1) return false;
    if (abs(x - (MAP_WIDTH - 2)) <= 1 && abs(y - 1) <= 1) return false;
    if (abs(x - 1) <= 1 && abs(y - (MAP_HEIGHT - 2)) <= 1) return false;
    if (abs(x - (MAP_WIDTH - 2)) <= 1 && abs(y - (MAP_HEIGHT - 2)) <= 1) return false;

    // Tránh chặn hành lang 1 ô (horizontal)
    bool corridor_h =
        state->map[y][x - 1] == EMPTY &&
        state->map[y][x + 1] == EMPTY &&
        state->map[y - 1][x] != EMPTY &&
        state->map[y + 1][x] != EMPTY;

    // Tránh chặn hành lang 1 ô (vertical)
    bool corridor_v =
        state->map[y - 1][x] == EMPTY &&
        state->map[y + 1][x] == EMPTY &&
        state->map[y][x - 1] != EMPTY &&
        state->map[y][x + 1] != EMPTY;

    if (corridor_h || corridor_v) return false;

    return true;
}

// đặt WALL_HARD nhưng đảm bảo không tạo hard-pocket / split
static bool try_place_hard_wall_safe(GameState *state, int x, int y) {
    if (!can_place_hard_wall_geom(state, x, y)) return false;

    // thử đặt
    state->map[y][x] = WALL_HARD;

    // nếu làm map không connected -> rollback
    if (!map_is_connected(state)) {
        state->map[y][x] = EMPTY;
        return false;
    }

    return true;
}

static void ensure_min_hard_walls(GameState *state, int min_walls) {
    int count = count_hard_walls(state);
    if (count >= min_walls) return;

    for (int attempt = 0; attempt < 4000 && count < min_walls; attempt++) {
        int x = 2 + rand() % (MAP_WIDTH - 4);
        int y = 2 + rand() % (MAP_HEIGHT - 4);

        if (try_place_hard_wall_safe(state, x, y)) {
            count++;
        }
    }
}

/* =========================
   HARD WALL GENERATION (7 MAPS)
   - tạo theo pattern, sau đó clear spawn,
   - cuối cùng "bọc an toàn" bằng ensure_min_hard_walls_safe
   ========================= */

void generate_hard_walls(GameState *state, MapType map_type) {
    // 1. Khởi tạo toàn bộ map là EMPTY
    memset(state->map, EMPTY, sizeof(state->map));

    // 2. Border
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
            for (int y = 2; y <= MAP_HEIGHT - 2; y += 2) {
                for (int x = 2; x <= MAP_WIDTH - 2; x += 2) {
                    // pattern classic vốn an toàn, cứ đặt thẳng
                    state->map[y][x] = WALL_HARD;
                }
            }
            break;

        case MAP_CROSS:
            for (int y = 2; y < MAP_HEIGHT - 2; y++) {
                for (int x = 2; x < MAP_WIDTH - 2; x++) {
                    if (abs(x - mid_x) > 1 && abs(y - mid_y) > 1) {
                        if (x % 2 == 0 && y % 2 == 0) {
                            state->map[y][x] = WALL_HARD;
                        }
                    }
                }
            }
            break;

        case MAP_DIAMOND:
            for (int y = 1; y < MAP_HEIGHT - 1; y++) {
                for (int x = 1; x < MAP_WIDTH - 1; x++) {
                    int dist = abs(x - mid_x) + abs(y - mid_y);
                    if ((dist == 4 || dist == 8) && !is_spawn_area(x, y)) {
                        state->map[y][x] = WALL_HARD;
                    }
                }
            }
            break;

        case MAP_CHECKERBOARD:
            for (int y = 1; y < MAP_HEIGHT - 1; y++) {
                for (int x = 1; x < MAP_WIDTH - 1; x++) {
                    if ((x + y) % 4 == 0 && !is_spawn_area(x, y)) {
                        state->map[y][x] = WALL_HARD;
                    }
                }
            }
            break;

        case MAP_SPIRAL:
            for (int y = 2; y < MAP_HEIGHT - 2; y++) {
                for (int x = 2; x < MAP_WIDTH - 2; x++) {
                    if (y == 2 && x < MAP_WIDTH - 3) state->map[y][x] = WALL_HARD;
                    else if (x == MAP_WIDTH - 3 && y < MAP_HEIGHT - 3) state->map[y][x] = WALL_HARD;
                    else if (y == MAP_HEIGHT - 3 && x > 3) state->map[y][x] = WALL_HARD;
                    else if (x == 3 && y > 4 && y < MAP_HEIGHT - 4) state->map[y][x] = WALL_HARD;
                }
            }
            break;

        case MAP_ARENA:
            for (int y = mid_y - 3; y <= mid_y + 3; y++) {
                for (int x = mid_x - 3; x <= mid_x + 3; x++) {
                    if ((x == mid_x - 3 || x == mid_x + 3) &&
                        (y == mid_y - 3 || y == mid_y + 3)) {
                        state->map[y][x] = WALL_HARD;
                    }
                }
            }
            break;

        case MAP_MAZE:
            for (int y = 2; y < MAP_HEIGHT - 2; y += 2) {
                for (int x = 2; x < MAP_WIDTH - 2; x += 2) {
                    if (!is_spawn_area(x, y)) {
                        state->map[y][x] = WALL_HARD;
                        int dir = rand() % 4;
                        if (dir == 0 && !is_spawn_area(x + 1, y)) state->map[y][x + 1] = WALL_HARD;
                        else if (dir == 1 && !is_spawn_area(x, y + 1)) state->map[y + 1][x] = WALL_HARD;
                    }
                }
            }
            break;
    }

    // 3. clear spawn (bắt buộc)
    clear_spawn_hard_walls(state);

    // 4. nếu pattern tạo pocket => ta sẽ “phá pocket” bằng cách remove hard walls
    //    cách đơn giản: nếu disconnected, thử gỡ bớt một số hard walls ngẫu nhiên cho đến khi connected
    if (!map_is_connected(state)) {
        for (int attempt = 0; attempt < 5000 && !map_is_connected(state); attempt++) {
            int x = 2 + rand() % (MAP_WIDTH - 4);
            int y = 2 + rand() % (MAP_HEIGHT - 4);

            if (state->map[y][x] == WALL_HARD && !is_spawn_area(x, y)) {
                state->map[y][x] = EMPTY;
            }
        }
    }

    // 5. đảm bảo cuối cùng connected (nếu vẫn fail thì fallback classic)
    if (!map_is_connected(state)) {
        // fallback: classic border + cột chẵn (đảm bảo connected)
        memset(state->map, EMPTY, sizeof(state->map));
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                if (x == 0 || x == MAP_WIDTH - 1 || y == 0 || y == MAP_HEIGHT - 1) {
                    state->map[y][x] = WALL_HARD;
                }
            }
        }
        for (int y = 2; y < MAP_HEIGHT - 2; y += 2) {
            for (int x = 2; x < MAP_WIDTH - 2; x += 2) {
                if (!is_spawn_area(x, y)) state->map[y][x] = WALL_HARD;
            }
        }
        clear_spawn_hard_walls(state);
    }

    // 6. thêm hard walls ngẫu nhiên nhưng PHẢI an toàn
    ensure_min_hard_walls(state, 30);
}

/* =========================
   SMART SOFT WALLS
   - soft có thể phá nên không cần check connected theo hard,
   - giữ logic bạn, chỉ chỉnh vài guard nhỏ
   ========================= */

void generate_smart_soft_walls(GameState *state) {
    int total_empty = 0;
    int placed = 0;

    for (int y = 1; y < MAP_HEIGHT - 1; y++) {
        for (int x = 1; x < MAP_WIDTH - 1; x++) {
            if (state->map[y][x] == EMPTY && !is_spawn_area(x, y)) {
                total_empty++;
            }
        }
    }

    int target_soft_walls = total_empty * 40 / 100;

    for (int attempt = 0; attempt < 5 && placed < target_soft_walls; attempt++) {
        for (int y = 1; y < MAP_HEIGHT - 1; y++) {
            for (int x = 1; x < MAP_WIDTH - 1; x++) {
                if (state->map[y][x] != EMPTY) continue;
                if (is_spawn_area(x, y)) continue;
                if (placed >= target_soft_walls) break;

                int min_spawn_dist = 100;
                int spawn_dists[4] = {
                    abs(x - 1) + abs(y - 1),
                    abs(x - (MAP_WIDTH - 2)) + abs(y - 1),
                    abs(x - 1) + abs(y - (MAP_HEIGHT - 2)),
                    abs(x - (MAP_WIDTH - 2)) + abs(y - (MAP_HEIGHT - 2))
                };

                for (int i = 0; i < 4; i++) {
                    if (spawn_dists[i] < min_spawn_dist) min_spawn_dist = spawn_dists[i];
                }

                int probability;
                if (min_spawn_dist <= 3) probability = 15;
                else if (min_spawn_dist <= 5) probability = 30;
                else probability = 45;

                // tránh soft wall tạo “kẹt ngay lập tức” (chỉ là quality, không phải topology)
                int empty_neighbors = 0;
                if (x > 1 && state->map[y][x - 1] == EMPTY) empty_neighbors++;
                if (x < MAP_WIDTH - 2 && state->map[y][x + 1] == EMPTY) empty_neighbors++;
                if (y > 1 && state->map[y - 1][x] == EMPTY) empty_neighbors++;
                if (y < MAP_HEIGHT - 2 && state->map[y + 1][x] == EMPTY) empty_neighbors++;

                if (empty_neighbors <= 1) continue;

                if (rand() % 100 < probability) {
                    state->map[y][x] = WALL_SOFT;
                    placed++;
                }
            }
        }
    }
}

/* =========================
   MAP REASONABLE CHECK (FIXED)
   - không còn “thoát spawn area”
   - thay bằng: spawn không bị hard + toàn map connected
   ========================= */

bool is_map_reasonable(GameState *state) {
    // spawn area không có hard
    for (int y = 1; y < MAP_HEIGHT - 1; y++) {
        for (int x = 1; x < MAP_WIDTH - 1; x++) {
            if (is_spawn_area(x, y) && state->map[y][x] == WALL_HARD) {
                return false;
            }
        }
    }

    // 4 spawn tiles phải passable
    Position spawns[4] = {
        {1, 1},
        {MAP_WIDTH - 2, 1},
        {1, MAP_HEIGHT - 2},
        {MAP_WIDTH - 2, MAP_HEIGHT - 2}
    };

    for (int i = 0; i < 4; i++) {
        if (!is_passable(state, spawns[i].x, spawns[i].y)) {
            printf("Spawn %d is on HARD wall\n", i + 1);
            return false;
        }
    }

    // quan trọng nhất: không được có hard-pocket / region kín
    if (!map_is_connected(state)) {
        printf("Map has HARD-ENCLOSED pocket / disconnected region\n");
        return false;
    }

    return true;
}

/* =========================
   DEBUG PRINT
   ========================= */

void print_map_state(GameState *state) {
    printf("\n=== MAP DEBUG ===\n");
    int hard_count = 0, soft_count = 0, empty_count = 0;

    for (int y = 0; y < MAP_HEIGHT; y++) {
        printf("%2d: ", y);
        for (int x = 0; x < MAP_WIDTH; x++) {
            char c;
            switch (state->map[y][x]) {
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

/* =========================
   INIT MAP
   ========================= */

void init_map(GameState *state) {
    printf("=== INITIALIZING MAP ===\n");

    int attempts = 0;
    const int MAX_ATTEMPTS = 20;

    do {
        attempts++;
        printf("\n--- Attempt %d ---\n", attempts);

        // MapType selected_map = rand() % NUM_PREDEFINED_MAPS;
        MapType selected_map = 3;
        printf("Selected map type: %d\n", selected_map);

        generate_hard_walls(state, selected_map);
        generate_smart_soft_walls(state);

        print_map_state(state);

        bool reasonable = is_map_reasonable(state);
        printf("Map reasonable: %s\n", reasonable ? "YES" : "NO");

        if (reasonable) break;

    } while (attempts < MAX_ATTEMPTS);

    if (attempts >= MAX_ATTEMPTS) {
        printf("\n!!! USING FALLBACK MAP !!!\n");

        memset(state->map, EMPTY, sizeof(state->map));

        // Border
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                if (x == 0 || x == MAP_WIDTH - 1 ||
                    y == 0 || y == MAP_HEIGHT - 1) {
                    state->map[y][x] = WALL_HARD;
                }
            }
        }

        // Classic pillars
        for (int y = 2; y < MAP_HEIGHT - 2; y += 2) {
            for (int x = 2; x < MAP_WIDTH - 2; x += 2) {
                if (!is_spawn_area(x, y)) state->map[y][x] = WALL_HARD;
            }
        }

        clear_spawn_hard_walls(state);

        // Soft walls random
        for (int y = 1; y < MAP_HEIGHT - 1; y++) {
            for (int x = 1; x < MAP_WIDTH - 1; x++) {
                if (state->map[y][x] == EMPTY && !is_spawn_area(x, y)) {
                    if (rand() % 100 < 40) state->map[y][x] = WALL_SOFT;
                }
            }
        }

        print_map_state(state);

        // nếu fallback mà vẫn lỗi (hầu như không) thì in cảnh báo
        if (!is_map_reasonable(state)) {
            printf("WARNING: fallback map still not reasonable (unexpected)\n");
        }
    }

    printf("=== MAP INITIALIZATION COMPLETE ===\n\n");
}
