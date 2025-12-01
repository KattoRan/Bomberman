/* client/ui_screens.c */
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string.h>
#include "../common/protocol.h"
#include "ui.h" // <--- THÊM DÒNG NÀY

// --- ĐÃ XÓA struct Button và InputField ở đây vì đã có trong ui.h ---

// Vẽ button
void draw_button(SDL_Renderer *renderer, TTF_Font *font, Button *btn) {
    SDL_Color bg_color = btn->is_hovered ? 
        (SDL_Color){100, 150, 255, 255} : 
        (SDL_Color){70, 120, 200, 255};
    
    SDL_SetRenderDrawColor(renderer, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
    SDL_RenderFillRect(renderer, &btn->rect);
    
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &btn->rect);
    
    if (font) {
        SDL_Color white = {255, 255, 255, 255};
        SDL_Surface *text_surface = TTF_RenderText_Solid(font, btn->text, white);
        if (text_surface) {
            SDL_Texture *text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
            SDL_Rect text_rect = {
                btn->rect.x + (btn->rect.w - text_surface->w) / 2,
                btn->rect.y + (btn->rect.h - text_surface->h) / 2,
                text_surface->w,
                text_surface->h
            };
            SDL_RenderCopy(renderer, text_texture, NULL, &text_rect);
            SDL_DestroyTexture(text_texture);
            SDL_FreeSurface(text_surface);
        }
    }
}

// Vẽ input field
void draw_input_field(SDL_Renderer *renderer, TTF_Font *font, InputField *field) {
    // Vẽ label
    if (font) {
        SDL_Color white = {255, 255, 255, 255};
        SDL_Surface *label_surface = TTF_RenderText_Solid(font, field->label, white);
        if (label_surface) {
            SDL_Texture *label_texture = SDL_CreateTextureFromSurface(renderer, label_surface);
            SDL_Rect label_rect = {
                field->rect.x,
                field->rect.y - 30,
                label_surface->w,
                label_surface->h
            };
            SDL_RenderCopy(renderer, label_texture, NULL, &label_rect);
            SDL_DestroyTexture(label_texture);
            SDL_FreeSurface(label_surface);
        }
    }
    
    // Vẽ input box
    SDL_Color box_color = field->is_active ? 
        (SDL_Color){255, 255, 255, 255} : 
        (SDL_Color){200, 200, 200, 255};
    
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_RenderFillRect(renderer, &field->rect);
    
    SDL_SetRenderDrawColor(renderer, box_color.r, box_color.g, box_color.b, box_color.a);
    SDL_RenderDrawRect(renderer, &field->rect);
    
    // Vẽ text
    if (font && strlen(field->text) > 0) {
        SDL_Color text_color = {255, 255, 255, 255};
        
        // Ẩn password
        char display_text[128];
        if (strcmp(field->label, "Password:") == 0) {
            int len = strlen(field->text);
            for (int i = 0; i < len; i++) {
                display_text[i] = '*';
            }
            display_text[len] = '\0';
        } else {
            strcpy(display_text, field->text);
        }
        
        SDL_Surface *text_surface = TTF_RenderText_Solid(font, display_text, text_color);
        if (text_surface) {
            SDL_Texture *text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
            SDL_Rect text_rect = {
                field->rect.x + 10,
                field->rect.y + (field->rect.h - text_surface->h) / 2,
                text_surface->w,
                text_surface->h
            };
            SDL_RenderCopy(renderer, text_texture, NULL, &text_rect);
            SDL_DestroyTexture(text_texture);
            SDL_FreeSurface(text_surface);
        }
    }
    
    // Vẽ cursor nếu active
    if (field->is_active) {
        static int cursor_blink = 0;
        cursor_blink = (cursor_blink + 1) % 60;
        
        if (cursor_blink < 30) {
            int cursor_x = field->rect.x + 10;
            if (font && strlen(field->text) > 0) {
                int w, h;
                TTF_SizeText(font, field->text, &w, &h);
                cursor_x += w;
            }
            
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawLine(renderer, cursor_x, field->rect.y + 5, 
                             cursor_x, field->rect.y + field->rect.h - 5);
        }
    }
}

// Màn hình đăng nhập
void render_login_screen(SDL_Renderer *renderer, TTF_Font *font, 
                        InputField *username, InputField *password,
                        Button *login_btn, Button *register_btn, 
                        const char *message) {
    SDL_SetRenderDrawColor(renderer, 30, 30, 50, 255);
    SDL_RenderClear(renderer);
    
    // Title
    if (font) {
        SDL_Color yellow = {255, 255, 0, 255};
        SDL_Surface *title_surface = TTF_RenderText_Solid(font, "BOMBERMAN - LOGIN", yellow);
        if (title_surface) {
            SDL_Texture *title_texture = SDL_CreateTextureFromSurface(renderer, title_surface);
            SDL_Rect title_rect = {
                400 - title_surface->w / 2,
                100,
                title_surface->w,
                title_surface->h
            };
            SDL_RenderCopy(renderer, title_texture, NULL, &title_rect);
            SDL_DestroyTexture(title_texture);
            SDL_FreeSurface(title_surface);
        }
    }
    
    draw_input_field(renderer, font, username);
    draw_input_field(renderer, font, password);
    draw_button(renderer, font, login_btn);
    draw_button(renderer, font, register_btn);
    
    if (font && strlen(message) > 0) {
        SDL_Color white = {255, 255, 255, 255};
        SDL_Surface *msg_surface = TTF_RenderText_Solid(font, message, white);
        if (msg_surface) {
            SDL_Texture *msg_texture = SDL_CreateTextureFromSurface(renderer, msg_surface);
            SDL_Rect msg_rect = {
                400 - msg_surface->w / 2,
                500,
                msg_surface->w,
                msg_surface->h
            };
            SDL_RenderCopy(renderer, msg_texture, NULL, &msg_rect);
            SDL_DestroyTexture(msg_texture);
            SDL_FreeSurface(msg_surface);
        }
    }
    SDL_RenderPresent(renderer);
}

// Màn hình danh sách lobby
void render_lobby_list_screen(SDL_Renderer *renderer, TTF_Font *font,
                              Lobby *lobbies, int lobby_count,
                              Button *create_btn, Button *refresh_btn,
                              int selected_lobby) {
    SDL_SetRenderDrawColor(renderer, 30, 30, 50, 255);
    SDL_RenderClear(renderer);
    
    if (font) {
        SDL_Color yellow = {255, 255, 0, 255};
        SDL_Surface *title = TTF_RenderText_Solid(font, "GAME LOBBIES", yellow);
        if (title) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, title);
            SDL_Rect rect = {400 - title->w/2, 20, title->w, title->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(title);
        }
    }
    
    int y = 80;
    for (int i = 0; i < lobby_count; i++) {
        SDL_Color bg = (i == selected_lobby) ? 
            (SDL_Color){80, 120, 180, 255} : 
            (SDL_Color){50, 50, 70, 255};
        
        SDL_Rect lobby_rect = {50, y, 700, 60};
        SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, bg.a);
        SDL_RenderFillRect(renderer, &lobby_rect);
        SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
        SDL_RenderDrawRect(renderer, &lobby_rect);
        
        if (font) {
            char info[256];
            snprintf(info, sizeof(info), "%s - Players: %d/4 - Status: %s",
                    lobbies[i].name,
                    lobbies[i].num_players,
                    lobbies[i].status == LOBBY_WAITING ? "Waiting" : "Playing");
            
            SDL_Color white = {255, 255, 255, 255};
            SDL_Surface *surf = TTF_RenderText_Solid(font, info, white);
            if (surf) {
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
                SDL_Rect rect = {60, y + 20, surf->w, surf->h};
                SDL_RenderCopy(renderer, tex, NULL, &rect);
                SDL_DestroyTexture(tex);
                SDL_FreeSurface(surf);
            }
        }
        y += 70;
    }
    
    draw_button(renderer, font, create_btn);
    draw_button(renderer, font, refresh_btn);
    SDL_RenderPresent(renderer);
}

// Màn hình phòng chờ
void render_lobby_room_screen(SDL_Renderer *renderer, TTF_Font *font,
                              Lobby *lobby, int my_player_id,
                              Button *ready_btn, Button *start_btn, 
                              Button *leave_btn) {
    SDL_SetRenderDrawColor(renderer, 30, 30, 50, 255);
    SDL_RenderClear(renderer);
    
    if (font) {
        SDL_Color yellow = {255, 255, 0, 255};
        SDL_Surface *title = TTF_RenderText_Solid(font, lobby->name, yellow);
        if (title) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, title);
            SDL_Rect rect = {400 - title->w/2, 20, title->w, title->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(title);
        }
    }
    
    int y = 100;
    for (int i = 0; i < lobby->num_players; i++) {
        Player *p = &lobby->players[i];
        
        SDL_Color bg = (i == lobby->host_id) ? 
            (SDL_Color){100, 50, 150, 255} : 
            (SDL_Color){50, 50, 70, 255};
        
        SDL_Rect player_rect = {100, y, 600, 50};
        SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, bg.a);
        SDL_RenderFillRect(renderer, &player_rect);
        
        if (font) {
            char info[256];
            snprintf(info, sizeof(info), "%s %s %s",
                    p->username,
                    (i == lobby->host_id) ? "[HOST]" : "",
                    p->is_ready ? "[READY]" : "");
            
            SDL_Color color = p->is_ready ? 
                (SDL_Color){0, 255, 0, 255} : 
                (SDL_Color){255, 255, 255, 255};
            
            SDL_Surface *surf = TTF_RenderText_Solid(font, info, color);
            if (surf) {
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
                SDL_Rect rect = {120, y + 15, surf->w, surf->h};
                SDL_RenderCopy(renderer, tex, NULL, &rect);
                SDL_DestroyTexture(tex);
                SDL_FreeSurface(surf);
            }
        }
        y += 60;
    }
    
    if (ready_btn) draw_button(renderer, font, ready_btn);
    if (start_btn) draw_button(renderer, font, start_btn);
    draw_button(renderer, font, leave_btn);
    SDL_RenderPresent(renderer);
}