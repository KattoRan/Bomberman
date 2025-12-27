#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#include "../common/protocol.h"

#define NUM_PREDEFINED_MAPS 10

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

    // mở 2 ô thoát theo phong cách Bomberman
    state->map[1][2] = EMPTY;
    state->map[2][1] = EMPTY;

    state->map[1][MAP_WIDTH - 3] = EMPTY;
    state->map[2][MAP_WIDTH - 2] = EMPTY;

    state->map[MAP_HEIGHT - 2][2] = EMPTY;
    state->map[MAP_HEIGHT - 3][1] = EMPTY;

    state->map[MAP_HEIGHT - 2][MAP_WIDTH - 3] = EMPTY;
    state->map[MAP_HEIGHT - 3][MAP_WIDTH - 2] = EMPTY;
}

static const char PREDEFINED_MAPS[NUM_PREDEFINED_MAPS][MAP_HEIGHT][MAP_WIDTH + 1] = {
    {
        "###############",
        "#..%#%#%#%#%..#",
        "#.#.#.#.#.#.#.#",
        "#%.%.%...%.%.%#",
        "#.#.#.%.%.#.#.#",
        "#%...%#%#%...%#",
        "#%#%#%.%.%#%#%#",
        "#%...%#%#%...%#",
        "#.#.#.%.%.#.#.#",
        "#%.%.%...%.%.%#",
        "#.#.#.#.#.#.#.#",
        "#..%#%#%#%#%..#",
        "###############",
    },
    {
        "###############",
        "#..%.......%..#",
        "#.###%###%###.#",
        "#.%...%#%...%.#",
        "#.#%#%###%#%#.#",
        "#.%.........%.#",
        "###.###%###.###",
        "#.%.........%.#",
        "#.#%#%###%#%#.#",
        "#.%...%#%...%.#",
        "#.###%###%###.#",
        "#..%.......%..#",
        "###############",
    },
    {
        "###############",
        "#..%...%...%..#",
        "#.#%#######%#.#",
        "#.%.........%.#",
        "#.###.%#%.###.#",
        "#.%...%#%...%.#",
        "#%#.#%###%#.#%#",
        "#.%...%#%...%.#",
        "#.###.%#%.###.#",
        "#.%.........%.#",
        "#.#%#######%#.#",
        "#..%...%...%..#",
        "###############",
    },
    {
        "###############",
        "#..%...%...%..#",
        "#.###%#.#%###.#",
        "#%..%..%..%..%#",
        "#.#%#.###.#%#.#",
        "#.%....%....%.#",
        "###.%##.##%.###",
        "#.%....%....%.#",
        "#.#%#%###%#%#.#",
        "#%..%..%..%..%#",
        "#.###%###%###.#",
        "#..%...%...%..#",
        "###############",
    },
    {
        "###############",
        "#..%..#...#%..#",
        "#..#.#%#.#%#..#",
        "#%#...%#%...#%#",
        "#.#.##.#%#.##.#",
        "#.%..#.....#%.#",
        "#.#.#%#%#%#.#.#",
        "#.%#.....#..%.#",
        "#.#.##.#.#.##.#",
        "#%#...%#%...#%#",
        "#..#%#.#%#.#..#",
        "#..%#...#..%..#",
        "###############",
    },
    {
        "###############",
        "#..#...%...#..#",
        "#%#.#.#.#.#.#%#",
        "#.%...%#%...%.#",
        "#%#.#%#.#%#.#%#",
        "#.%....%....%.#",
        "#%#.#%#%#%#.#%#",
        "#.%....%....%.#",
        "#%#.#%#.#%#.#%#",
        "#.%...%#%...%.#",
        "#%#.#.#.#.#.#%#",
        "#..#...%...#..#",
        "###############",
    },
    {
        "###############",
        "#..%...%...%..#",
        "#.#%#.#.#.#%#.#",
        "#.%...%#%...%.#",
        "###.#######.###",
        "#.%....%....%.#",
        "#.#.###%###.#.#",
        "#.%....%....%.#",
        "###.#######.###",
        "#.%...%#%...%.#",
        "#.#%#.#.#.#%#.#",
        "#..%...%...%..#",
        "###############",
    },
    {
        "###############",
        "#..%.%.%.%.%..#",
        "#.#.#.#.#.#.#.#",
        "#%#%#%...%#%#%#",
        "#.#.#.#.#.#.#.#",
        "#%...%#%#%...%#",
        "#%#%#%%.%%#%#%#",
        "#%...%#%#%...%#",
        "#.#.#.#.#.#.#.#",
        "#%#%#%...%#%#%#",
        "#.#.#.#.#.#.#.#",
        "#..%.%.%.%.%..#",
        "###############",
    },
    {
        "###############",
        "#..%#%#%#%#%..#",
        "#.#.#.#.#.#.#.#",
        "#%#.........%%#",
        "#.#.#%#.#%#.#.#",
        "#%#%#..%..#%#%#",
        "#...%..#..%...#",
        "#%#%#..%..#%#%#",
        "#.#.#.#%#.#.#.#",
        "#%#.........%%#",
        "#.#.#.#.#.#.#.#",
        "#..%#%#%#%#%..#",
        "###############",
    },
    {
        "###############",
        "#..%..#.%..%..#",
        "#.#.#.#.#.#.#.#",
        "#%..%..%..%..%#",
        "#.#.#.###.#.#.#",
        "#..%..%.%..%..#",
        "#.#.#%.%.%#.#.#",
        "#..%..%.%..%..#",
        "#.#.#.###.#.#.#",
        "#%..%..%..%..%#",
        "#.#.#.#.#.#.#.#",
        "#..%..%.#..%..#",
        "###############",
    }
};

static int last_map_index = -1;

static int select_random_map_index(void) {
    int index;
    do {
        index = rand() % NUM_PREDEFINED_MAPS;
    } while (index == last_map_index);

    last_map_index = index;
    return index;
}

static void load_predefined_map(GameState *state, int map_index) {
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            char tile = PREDEFINED_MAPS[map_index][y][x];
            switch (tile) {
                case '#': state->map[y][x] = WALL_HARD; break;
                case '%': state->map[y][x] = WALL_SOFT; break;
                default:  state->map[y][x] = EMPTY; break;
            }
        }
    }
}

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

    int target = total_empty * 40 / 100;

    for (int pass = 0; pass < 5 && placed < target; pass++) {
        for (int y = 1; y < MAP_HEIGHT - 1; y++) {
            for (int x = 1; x < MAP_WIDTH - 1; x++) {
                if (state->map[y][x] != EMPTY) continue;
                if (is_spawn_area(x, y)) continue;
                if (placed >= target) break;

                int min_dist = 100;
                int dists[4] = {
                    abs(x - 1) + abs(y - 1),
                    abs(x - (MAP_WIDTH - 2)) + abs(y - 1),
                    abs(x - 1) + abs(y - (MAP_HEIGHT - 2)),
                    abs(x - (MAP_WIDTH - 2)) + abs(y - (MAP_HEIGHT - 2))
                };

                for (int i = 0; i < 4; i++)
                    if (dists[i] < min_dist) min_dist = dists[i];

                int probability = (min_dist <= 3) ? 15 :
                                  (min_dist <= 5) ? 30 : 45;

                int empty_neighbors = 0;
                if (state->map[y][x - 1] == EMPTY) empty_neighbors++;
                if (state->map[y][x + 1] == EMPTY) empty_neighbors++;
                if (state->map[y - 1][x] == EMPTY) empty_neighbors++;
                if (state->map[y + 1][x] == EMPTY) empty_neighbors++;

                if (empty_neighbors <= 1) continue;

                if (rand() % 100 < probability) {
                    state->map[y][x] = WALL_SOFT;
                    placed++;
                }
            }
        }
    }
}

void init_map(GameState *state) {
    printf("=== INITIALIZING MAP ===\n");

    int map_index = select_random_map_index();
    printf("Selected map: %d\n", map_index + 1);

    load_predefined_map(state, map_index);

    clear_spawn_hard_walls(state);

    generate_smart_soft_walls(state);

    printf("=== MAP INITIALIZATION COMPLETE ===\n\n");
}
