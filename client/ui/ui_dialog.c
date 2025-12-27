#include "../graphics/graphics.h"
#include "ui.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define UI_CORNER_RADIUS 8 

void render_create_room_dialog(SDL_Renderer *renderer, TTF_Font *font,
                                InputField *room_name, InputField *access_code,
                                Button *create_btn, Button *cancel_btn) {
    extern int selected_game_mode;  // Global from main.c
    int win_w, win_h;
    SDL_GetRendererOutputSize(renderer, &win_w, &win_h);
    
    // Darken background
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_Rect full_screen = {0, 0, win_w, win_h};
    SDL_RenderFillRect(renderer, &full_screen);
    
    // LARGER Dialog box - 700x650 (increased height for game mode buttons)
    int dialog_w = 700;
    int dialog_h = 650;  // Increased from 550 to fit mode buttons
    int dialog_x = (win_w - dialog_w) / 2;
    int dialog_y = (win_h - dialog_h) / 2;
    
    SDL_Rect dialog = {dialog_x, dialog_y, dialog_w, dialog_h};
    draw_rounded_rect(renderer, dialog, CLR_INPUT_BG, 12);
    draw_rounded_border(renderer, dialog, CLR_PRIMARY, 12, 2);
    
    // Title - LARGER
    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface *title = TTF_RenderText_Blended(font, "Create Room", CLR_ACCENT);
    if (title) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, title);
        SDL_Rect rect = {dialog_x + (dialog_w - title->w)/2, dialog_y + 30, title->w, title->h};
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(title);
    }
    
    // Input fields - positions already set in main.c, but need to be relative to dialog
    // Room name input - LARGER 550x60
    room_name->rect.x = dialog_x + 75;
    room_name->rect.y = dialog_y + 120;
    room_name->rect.w = dialog_w - 150;
    room_name->rect.h = 60;
    
    draw_input_field(renderer, font, room_name);
    
    // Access code field - LARGER
    if (access_code) {
        access_code->rect.x = dialog_x + 75;
        access_code->rect.y = dialog_y + 230;
        access_code->rect.w = dialog_w - 300;  // Make room for random button
        access_code->rect.h = 60;
        
        draw_input_field(renderer, font, access_code);
        
        // Helper text - smaller font would be better but we'll use existing
        SDL_Surface *helper_surf = TTF_RenderText_Blended(font, "6-digit code (leave empty for public)", 
                                                          CLR_GRAY);
        if (helper_surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, helper_surf);
            SDL_Rect rect = {dialog_x + 75, dialog_y + 300, helper_surf->w, helper_surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(helper_surf);
        }
        
        // Random button - LARGER 120x60
        SDL_Rect btn_random = {access_code->rect.x + access_code->rect.w + 15, 
                               access_code->rect.y, 120, 60};
       
        // Shadow
        draw_layered_shadow(renderer, btn_random, UI_CORNER_RADIUS, 3);
        
        // Button gradient
        draw_vertical_gradient(renderer, btn_random, CLR_ACCENT, CLR_ACCENT_DK);
        draw_rounded_border(renderer, btn_random, CLR_ACCENT_DK, UI_CORNER_RADIUS, 1);
        
        // Button text
        SDL_Surface *btn_surf = TTF_RenderText_Blended(font, "Random", white);
        if (btn_surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, btn_surf);
            SDL_Rect text_rect = {
                btn_random.x + (btn_random.w - btn_surf->w) / 2,
                btn_random.y + (btn_random.h - btn_surf->h) / 2,
                btn_surf->w, btn_surf->h
            };
            SDL_RenderCopy(renderer, tex, NULL, &text_rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(btn_surf);
        }
    }
    
    // === GAME MODE SELECTION ===
    SDL_Surface *mode_label = TTF_RenderText_Blended(font, "Game Mode:", CLR_WHITE);
    if (mode_label) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, mode_label);
        SDL_Rect rect = {dialog_x + 75, dialog_y + 350, mode_label->w, mode_label->h};
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(mode_label);
    }
    
    // Three mode buttons: Classic, Sudden Death, Fog of War
    const char *mode_names[] = {"Classic", "Sudden Death", "Fog of War"};
    int mode_y = dialog_y + 390;
    int btn_width = 180;
    int btn_spacing = 20;
    
    for (int i = 0; i < 3; i++) {
        int btn_x = dialog_x + 75 + i * (btn_width + btn_spacing);
        SDL_Rect mode_btn = {btn_x, mode_y, btn_width, 50};
        
        // Selected mode gets accent color, others get gray
        SDL_Color btn_bg = (i == selected_game_mode) ? CLR_ACCENT : CLR_INPUT_BG;
        SDL_Color btn_border = (i == selected_game_mode) ? CLR_ACCENT : CLR_GRAY;
        
        draw_layered_shadow(renderer, mode_btn, UI_CORNER_RADIUS, 3);
        draw_rounded_rect(renderer, mode_btn, btn_bg, UI_CORNER_RADIUS);
        draw_rounded_border(renderer, mode_btn, btn_border, UI_CORNER_RADIUS, (i == selected_game_mode) ? 2 : 1);
        
        // Checkmark for selected mode
        if (i == selected_game_mode) {
            SDL_Surface *check_surf = TTF_RenderText_Blended(font, "*", CLR_SUCCESS);
            if (check_surf) {
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, check_surf);
                SDL_Rect check_rect = {btn_x + 10, mode_y + 5 / 2, 
                                       check_surf->w, check_surf->h};
                SDL_RenderCopy(renderer, tex, NULL, &check_rect);
                SDL_DestroyTexture(tex);
                SDL_FreeSurface(check_surf);
            }
        }
        
        // Mode name
        SDL_Surface *name_surf = TTF_RenderText_Blended(font, mode_names[i], CLR_WHITE);
        if (name_surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, name_surf);
            SDL_Rect name_rect = {
                btn_x + (btn_width - name_surf->w) / 2,
                mode_y + (50 - name_surf->h) / 2,
                name_surf->w, name_surf->h
            };
            SDL_RenderCopy(renderer, tex, NULL, &name_rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(name_surf);
        }
    }
    
    // Buttons - LARGER 220x60 (was 180x50) - ONLY SET POSITION, don't draw here
    create_btn->rect = (SDL_Rect){dialog_x + 120, dialog_y + dialog_h - 100, 220, 60};
    cancel_btn->rect = (SDL_Rect){dialog_x + dialog_w - 340, dialog_y + dialog_h - 100, 220, 60};
    
    strcpy(create_btn->text, "Create");
    strcpy(cancel_btn->text, "Cancel");
    
    // NOTE: Buttons and input fields are drawn by main.c after this function returns
}