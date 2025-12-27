#include "../graphics/graphics.h"
#include "ui.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#define UI_CORNER_RADIUS 8
void render_friends_screen(SDL_Renderer *renderer, TTF_Font *font,
                           FriendInfo *friends, int friend_count,
                           FriendInfo *pending, int pending_count,
                           FriendInfo *sent, int sent_count,
                           Button *back_btn) {
    int win_w, win_h;
    SDL_GetRendererOutputSize(renderer, &win_w, &win_h);
    draw_background_grid(renderer, win_w, win_h);
    
    SDL_Color white = {255, 255, 255, 255};
    
    // Title - Larger
    SDL_Surface *surf = TTF_RenderText_Blended(font, "Friends", CLR_ACCENT);
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect rect = {40, 80-surf->h, surf->w, surf->h};
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }
    
    // LARGER cards - 750px wide (was 518px)
    int list_y = 134;
    // Your Friends section
    char title[64];
    snprintf(title, sizeof(title), "Your Friends (%d)", friend_count);
    surf = TTF_RenderText_Blended(font, title, white);
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect rect = {80, list_y, surf->w, surf->h};
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }
    
    // Friend cards - LARGER 110px tall (was 80px)
    list_y += 46;
    int card_width = 360;
    for (int i = 0; i < friend_count && i < 5; i++) {
        SDL_Rect card = {80, list_y, card_width, 100};
        
        // Layered shadow
        draw_layered_shadow(renderer, card, UI_CORNER_RADIUS, 4);
        
        // Background with gradient
        draw_vertical_gradient(renderer, card, CLR_INPUT_BG, CLR_BG);
        
        // Border
        draw_rounded_border(renderer, card, CLR_PRIMARY, UI_CORNER_RADIUS, 2);
        
        // Online status indicator (larger pulsing dot)
        if (friends[i].is_online > 0) {
            float pulse = (sinf(SDL_GetTicks() / 600.0f) + 1.0f) / 2.0f;
            int dot_size = 12 + (int)(pulse * 4);  // Larger (was 8+3)
            SDL_Rect status_dot = {110, list_y + 20, dot_size, dot_size};
            SDL_Color dot_color = CLR_SUCCESS;
            dot_color.a = 200 + (int)(pulse * 55);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            draw_rounded_rect(renderer, status_dot, dot_color, dot_size/2);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        }
        
        // Name - larger text, truncated to fit
        char friend_name[64];
        strncpy(friend_name, friends[i].display_name, sizeof(friend_name) - 1);
        friend_name[sizeof(friend_name) - 1] = '\0';
        truncate_text_to_fit(friend_name, sizeof(friend_name), font, 600);
        
        surf = TTF_RenderText_Blended(font, friend_name, CLR_WHITE);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {100, list_y + 19, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        // Status and ELO - truncated
        char info[64];
        const char *status = friends[i].is_online == 1 ? "Online" : (friends[i].is_online == 2 ? "In Game" : "Offline");
        snprintf(info, sizeof(info), "%s | ELO: %d", status, friends[i].elo_rating);
        truncate_text_to_fit(info, sizeof(info), font, 580);
        
        SDL_Color status_color = friends[i].is_online > 0 ? CLR_SUCCESS : CLR_GRAY;
        surf = TTF_RenderText_Blended(font, info, status_color);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {100, list_y + 55, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        // Remove button - LARGER 50x35 (was 30x26)
        SDL_Rect remove_btn = {card.x + card.w - 70, list_y + 38, 50, 35};
        draw_rounded_rect(renderer, remove_btn, CLR_DANGER, 6);
        surf = TTF_RenderText_Blended(font, "X", CLR_WHITE);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {remove_btn.x + (50 - surf->w)/2, remove_btn.y + (35 - surf->h)/2, 
                            surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        list_y += 110;  // More spacing (was 75)
    }
    
    // Pending Requests section - right column
    int pending_y = 134;
    int pending_x = 480;  // More spacing from left column
    snprintf(title, sizeof(title), "Pending Requests (%d)", pending_count);
    surf = TTF_RenderText_Blended(font, title, white);
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect rect = {pending_x, pending_y, surf->w, surf->h};
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }
    
    // Pending request cards - TALLER to fit buttons (110px instead of 90px)
    pending_y += 46;
    for (int i = 0; i < pending_count && i < 5; i++) {
        SDL_Rect card = {pending_x, pending_y, card_width-80, 100};  // INCREASED from 90
        
        // Layered shadow
        draw_layered_shadow(renderer, card, UI_CORNER_RADIUS, 4);
        
        // Background
        draw_vertical_gradient(renderer, card, CLR_INPUT_BG, CLR_BG);
        
        // Yellow border for pending - thicker
        draw_rounded_border(renderer, card, CLR_WARNING, UI_CORNER_RADIUS, 2);
        
        // Name - truncated to fit
        char pending_name[64];
        strncpy(pending_name, pending[i].display_name, sizeof(pending_name) - 1);
        pending_name[sizeof(pending_name) - 1] = '\0';
        truncate_text_to_fit(pending_name, sizeof(pending_name), font, 680);
        
        surf = TTF_RenderText_Blended(font, pending_name, white);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {pending_x + 40, pending_y + 19, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        // Accept button - positioned LOWER to avoid overlap (Y: 48->60)
        SDL_Rect accept_btn = {pending_x + 40, pending_y + 55, 90, 35};
        draw_rounded_rect(renderer, accept_btn, CLR_SUCCESS, 6);
        surf = TTF_RenderText_Blended(font, "Accept", white);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {accept_btn.x + (90 - surf->w)/2, accept_btn.y + (35 - surf->h)/2, 
                            surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        // Decline button - positioned LOWER (Y: 48->60)
        SDL_Rect decline_btn = {pending_x + 150, pending_y + 55, 90, 35};
        draw_rounded_rect(renderer, decline_btn, CLR_DANGER, 6);
        surf = TTF_RenderText_Blended(font, "Decline", white);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {decline_btn.x + (90 - surf->w)/2, decline_btn.y + (35 - surf->h)/2, 
                            surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        pending_y += 110;  // More spacing (was 105)
    }
    
    // Sent Requests section - bottom left
    int sent_y = 134;
    int sent_x = pending_x + 320;
    snprintf(title, sizeof(title), "Sent Requests (%d)", sent_count);
    surf = TTF_RenderText_Blended(font, title, white);
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect rect = {sent_x, sent_y, surf->w, surf->h};
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }
    
    sent_y += 46;
    for (int i = 0; i < sent_count && i < 3; i++) {
        SDL_Rect card = {sent_x, sent_y, 240, 45};
        
        draw_layered_shadow(renderer, card, UI_CORNER_RADIUS, 4);
        draw_vertical_gradient(renderer, card, CLR_INPUT_BG, CLR_BG);
        draw_rounded_border(renderer, card, CLR_GRAY, UI_CORNER_RADIUS, 2);
        
        // Name
        surf = TTF_RenderText_Blended(font, sent[i].display_name, CLR_GRAY);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {sent_x + 20, sent_y + 7, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        // Cancel button - LARGER
        SDL_Rect cancel_btn = {card.x + card.w - 110, sent_y + 5, 90, 35};
        draw_rounded_rect(renderer, cancel_btn, CLR_DANGER, 6);
        surf = TTF_RenderText_Blended(font, "Cancel", white);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {cancel_btn.x + (90 - surf->w)/2, cancel_btn.y + (35 - surf->h)/2,
                            surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        sent_y += 55;
    }
    
    // Send friend request section
    surf = TTF_RenderText_Blended(font, "Send Friend Request", white);
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect rect = {80, 637, surf->w, surf->h};  // Changed from -150 to -210
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }
    
    // Note: Input field and button rendered separately in main.c
    
    // LARGER back button - bottom center, adjusted for 1120x720
    back_btn->rect = (SDL_Rect){920, 40, 120, 40};  // Increased size
    strcpy(back_btn->text, "Back");
    draw_button(renderer, font, back_btn);
}

