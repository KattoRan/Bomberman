#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "../common/protocol.h"

#define TILE_SIZE 40
#define WINDOW_WIDTH (MAP_WIDTH * TILE_SIZE)
#define WINDOW_HEIGHT (MAP_HEIGHT * TILE_SIZE + 50) // Thêm 50px cho status bar

extern GameState current_state;

// Màu sắc định nghĩa
const SDL_Color COLOR_BACKGROUND = {34, 139, 34, 255};      // Xanh lá đậm
const SDL_Color COLOR_WALL_HARD = {64, 64, 64, 255};        // Xám đậm
const SDL_Color COLOR_WALL_SOFT = {139, 69, 19, 255};       // Nâu
const SDL_Color COLOR_BOMB = {255, 0, 0, 255};              // Đỏ
const SDL_Color COLOR_EXPLOSION = {255, 165, 0, 255};       // Cam
const SDL_Color COLOR_PLAYER1 = {0, 0, 255, 255};           // Xanh dương
const SDL_Color COLOR_PLAYER2 = {255, 255, 0, 255};         // Vàng
const SDL_Color COLOR_PLAYER3 = {255, 0, 255, 255};         // Hồng
const SDL_Color COLOR_PLAYER4 = {0, 255, 255, 255};         // Cyan

// Vẽ một ô vuông với viền
void draw_tile(SDL_Renderer *renderer, int x, int y, SDL_Color color, int draw_border) {
    SDL_Rect rect = {x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE};
    
    // Vẽ ô chính
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);
    
    // Vẽ viền nếu cần
    if (draw_border) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &rect);
    }
}

// Vẽ bom với hiệu ứng nhấp nháy
void draw_bomb(SDL_Renderer *renderer, int x, int y, int tick) {
    SDL_Rect bomb_rect = {x * TILE_SIZE + 5, y * TILE_SIZE + 5, 
                          TILE_SIZE - 10, TILE_SIZE - 10};
    
    // Nhấp nháy dựa vào tick
    if ((tick / 10) % 2 == 0) {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 200, 0, 0, 255);
    }
    SDL_RenderFillRect(renderer, &bomb_rect);
    
    // Vẽ dây chì
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_Rect fuse = {x * TILE_SIZE + TILE_SIZE/2 - 2, 
                     y * TILE_SIZE + 2, 4, 8};
    SDL_RenderFillRect(renderer, &fuse);
}

// Vẽ vụ nổ với hiệu ứng
void draw_explosion(SDL_Renderer *renderer, int x, int y, int tick) {
    int pulse = (tick % 20) - 10;
    int size = TILE_SIZE - abs(pulse);
    int offset = (TILE_SIZE - size) / 2;
    
    SDL_Rect exp_rect = {x * TILE_SIZE + offset, y * TILE_SIZE + offset, 
                         size, size};
    
    SDL_SetRenderDrawColor(renderer, 255, 165, 0, 200);
    SDL_RenderFillRect(renderer, &exp_rect);
    
    // Viền sáng
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
    SDL_RenderDrawRect(renderer, &exp_rect);
}

// Vẽ người chơi
void draw_player(SDL_Renderer *renderer, Player *p, SDL_Color color) {
    if (!p->is_alive) return;
    
    // Thân
    SDL_Rect body = {p->x * TILE_SIZE + 8, p->y * TILE_SIZE + 8, 
                     TILE_SIZE - 16, TILE_SIZE - 16};
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &body);
    
    // Đầu (hình tròn giản đơn - dùng hình vuông bo góc)
    SDL_Rect head = {p->x * TILE_SIZE + 12, p->y * TILE_SIZE + 4, 
                     TILE_SIZE - 24, TILE_SIZE - 28};
    SDL_RenderFillRect(renderer, &head);
    
    // Viền đen
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(renderer, &body);
    SDL_RenderDrawRect(renderer, &head);
    
    // Mắt
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Rect eye1 = {p->x * TILE_SIZE + 14, p->y * TILE_SIZE + 10, 4, 4};
    SDL_Rect eye2 = {p->x * TILE_SIZE + 22, p->y * TILE_SIZE + 10, 4, 4};
    SDL_RenderFillRect(renderer, &eye1);
    SDL_RenderFillRect(renderer, &eye2);
}

// Vẽ status bar
void draw_status_bar(SDL_Renderer *renderer, TTF_Font *font) {
    int bar_y = MAP_HEIGHT * TILE_SIZE;
    
    // Nền status bar
    SDL_Rect status_bg = {0, bar_y, WINDOW_WIDTH, 50};
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderFillRect(renderer, &status_bg);
    
    // Vẽ thông tin người chơi còn sống
    if (font) {
        char status_text[256];
        int alive_count = 0;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (current_state.players[i].is_alive) alive_count++;
        }
        
        // Hiển thị trạng thái game
        const char *status_str = "UNKNOWN";
        SDL_Color status_color = {255, 255, 255, 255};
        
        if (current_state.game_status == GAME_WAITING) {
            status_str = "WAITING";
            status_color = (SDL_Color){255, 255, 0, 255}; // Vàng
        } else if (current_state.game_status == GAME_RUNNING) {
            status_str = "PLAYING";
            status_color = (SDL_Color){0, 255, 0, 255}; // Xanh
        } else if (current_state.game_status == GAME_ENDED) {
            status_str = "ENDED";
            status_color = (SDL_Color){255, 0, 0, 255}; // Đỏ
        }
        
        snprintf(status_text, sizeof(status_text), 
                "%s | Alive: %d", status_str, alive_count);
        
        SDL_Surface *text_surface = TTF_RenderText_Solid(font, status_text, status_color);
        if (text_surface) {
            SDL_Texture *text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
            SDL_Rect text_rect = {10, bar_y + 15, text_surface->w, text_surface->h};
            SDL_RenderCopy(renderer, text_texture, NULL, &text_rect);
            SDL_DestroyTexture(text_texture);
            SDL_FreeSurface(text_surface);
        }
    }
}

// Hàm render chính
void render_game(SDL_Renderer *renderer, TTF_Font *font, int tick) {
    // Xóa màn hình
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Vẽ nền
    SDL_SetRenderDrawColor(renderer, COLOR_BACKGROUND.r, COLOR_BACKGROUND.g, 
                          COLOR_BACKGROUND.b, COLOR_BACKGROUND.a);
    SDL_Rect bg = {0, 0, WINDOW_WIDTH, MAP_HEIGHT * TILE_SIZE};
    SDL_RenderFillRect(renderer, &bg);

    // Vẽ bản đồ
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
                case EMPTY:
                    // Vẽ lưới nhẹ
                    SDL_SetRenderDrawColor(renderer, 40, 120, 40, 100);
                    SDL_Rect grid = {x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE};
                    SDL_RenderDrawRect(renderer, &grid);
                    break;
            }
        }
    }

    // Vẽ người chơi
    SDL_Color player_colors[] = {COLOR_PLAYER1, COLOR_PLAYER2, 
                                 COLOR_PLAYER3, COLOR_PLAYER4};
    for (int i = 0; i < current_state.num_players; i++) {
        draw_player(renderer, &current_state.players[i], 
                   player_colors[i % 4]);
    }

    // Vẽ status bar
    draw_status_bar(renderer, font);

    // Hiển thị
    SDL_RenderPresent(renderer);
}

// Khởi tạo font (gọi một lần khi init)
TTF_Font* init_font() {
    if (TTF_Init() == -1) {
        fprintf(stderr, "TTF_Init error: %s\n", TTF_GetError());
        return NULL;
    }
    
    // Thử load font hệ thống (thay đổi path phù hợp với OS của bạn)
    TTF_Font *font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 18);
    if (!font) {
        // Fallback cho Windows
        font = TTF_OpenFont("C:\\Windows\\Fonts\\arial.ttf", 18);
    }
    
    if (!font) {
        fprintf(stderr, "TTF_OpenFont error: %s\n", TTF_GetError());
    }
    
    return font;
}