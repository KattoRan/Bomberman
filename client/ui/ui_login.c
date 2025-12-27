#include "ui.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string.h>

void render_login_screen(SDL_Renderer *renderer, TTF_Font *font_large, TTF_Font *font_small, 
                        InputField *username, InputField *email, InputField *password,
                        Button *login_btn, Button *register_btn, 
                        const char *message) {
    
    int win_w, win_h;
    SDL_GetRendererOutputSize(renderer, &win_w, &win_h);

    draw_background_grid(renderer, win_w, win_h);

    // Tính toán vị trí trung tâm cho tất cả elements
    int center_x = win_w / 2;
    int content_width = 400;
    int start_x = center_x - content_width / 2;

    // Tiêu đề "BOMBERMAN" với hiệu ứng đẹp
    if (font_large) {
        const char *title = "BOMBERMAN";
        
        // Bóng đổ
        SDL_Surface *surf_shadow = TTF_RenderText_Blended(font_large, title, 
                                    (SDL_Color){0, 0, 0, 120});
        if (surf_shadow) {
            SDL_Texture *tex_shadow = SDL_CreateTextureFromSurface(renderer, surf_shadow);
            SDL_Rect rect_shadow = { 
                center_x - surf_shadow->w / 2 + 4, 
                64, 
                surf_shadow->w, 
                surf_shadow->h 
            };
            SDL_RenderCopy(renderer, tex_shadow, NULL, &rect_shadow);
            SDL_DestroyTexture(tex_shadow);
            SDL_FreeSurface(surf_shadow);
        }
        
        // Chữ chính
        SDL_Surface *surf = TTF_RenderText_Blended(font_large, title, CLR_ACCENT);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = { 
                center_x - surf->w / 2, 
                60, 
                surf->w, 
                surf->h 
            };
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
    }
    // Just draw them
    int is_registration = (email != NULL);
    
    draw_input_field(renderer, font_small, username);
    if (is_registration) {
        // Adjust password Y for registration mode
        password->rect.y = 440;  // More space for email field
        draw_input_field(renderer, font_small, email);
    } else {
        password->rect.y = 360;  // Standard login spacing
    }
    draw_input_field(renderer, font_small, password);
    // Set button labels based on mode
    if (is_registration) {
        strcpy(login_btn->text, "Back");
        strcpy(register_btn->text, "Register");
    } else {
        strcpy(login_btn->text, "Login");
        strcpy(register_btn->text, "Register");
    }

    draw_button(renderer, font_small, login_btn);
    draw_button(renderer, font_small, register_btn);
    
    // Vẽ thông báo lỗi (căn giữa)
    if (font_small && message && strlen(message) > 0) {
        SDL_Surface *surf = TTF_RenderText_Blended(font_small, message, CLR_DANGER);
        if (surf) {
            if (is_registration) {
                // Position below register button
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
                SDL_Rect rect = {
                    center_x - surf->w / 2,
                    160,
                    surf->w, 
                    surf->h
                };
                SDL_RenderCopy(renderer, tex, NULL, &rect);
                SDL_DestroyTexture(tex);
                SDL_FreeSurface(surf);
            } else {
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
                SDL_Rect rect = {
                    center_x - surf->w / 2,
                    460,
                    surf->w, 
                    surf->h
                };
                SDL_RenderCopy(renderer, tex, NULL, &rect);
                SDL_DestroyTexture(tex);
                SDL_FreeSurface(surf);
            }
        }
    }
}