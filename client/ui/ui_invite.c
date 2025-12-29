#include "../state/client_state.h"
#include "ui.h"
#include <SDL2/SDL.h> 
#include <SDL2/SDL_ttf.h>
void render_invite_overlay(SDL_Renderer *renderer, TTF_Font *font, 
                          FriendInfo *online_friends, int friend_count, 
                          int *invited_user_ids, int invited_count,
                          Button *close_btn) {
    int win_w, win_h;
    SDL_GetRendererOutputSize(renderer, &win_w, &win_h);
    
    // 1. DIm Background
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200); // Darker dim
    SDL_Rect overlay_bg = {0, 0, win_w, win_h};
    SDL_RenderFillRect(renderer, &overlay_bg);
    
    // 2. Main Container
    int container_w = 500;
    int container_h = 500;
    int x = (win_w - container_w) / 2;
    int y = 110; // Top offset
    SDL_Rect container = {x, y, container_w, container_h};
    
    // Background
    draw_layered_shadow(renderer, container, 12, 8);
    draw_rounded_rect(renderer, container, CLR_BG, 12);
    draw_rounded_border(renderer, container, CLR_PRIMARY, 12, 2);
    
    // Header
    SDL_Surface *title = TTF_RenderText_Blended(font, "INVITE FRIENDS", CLR_WHITE);
    if(title) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, title);
        SDL_Rect r = {x + (container_w - title->w)/2, y + 15, title->w, title->h};
        SDL_RenderCopy(renderer, tex, NULL, &r);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(title);
    }
    
    // Update Close Button Position
    close_btn->rect.x = x + container_w - 50;
    close_btn->rect.y = y + 10;
    draw_button(renderer, font, close_btn);
    
    // 3. Friend List
    int list_y = y + 80;
    int displayed_count = 0;
    
    for(int i=0; i<friend_count; i++) {
        if(online_friends[i].is_online) {
            if(displayed_count >= 6) break; // Max 6 items
            
            SDL_Rect row = {x + 20, list_y + (displayed_count * 60), container_w - 40, 50};
            draw_rounded_rect(renderer, row, CLR_GRID, 8);
            
            // Name
            SDL_Surface *name = TTF_RenderText_Blended(font, online_friends[i].display_name, CLR_WHITE);
            if(name) {
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, name);
                SDL_Rect r = {row.x + 15, row.y + (row.h - name->h)/2, name->w, name->h};
                SDL_RenderCopy(renderer, tex, NULL, &r);
                SDL_DestroyTexture(tex);
                SDL_FreeSurface(name);
            }
            
            // Status Dot (Green)
            SDL_Rect dot = {row.x + container_w - 180, row.y + 20, 10, 10};
            draw_rounded_rect(renderer, dot, CLR_SUCCESS, 5);
            
            // Invite Button
            int is_invited = 0;
            for(int k=0; k<invited_count; k++) {
                if(invited_user_ids[k] == online_friends[i].user_id) is_invited = 1;
            }
            
            Button btn_inv = {{row.x + row.w - 110, row.y + 5, 100, 40}, "", 0, is_invited ? BTN_OUTLINE : BTN_PRIMARY};
            strcpy(btn_inv.text, is_invited ? "Sent" : "Invite");
            
            // Pseudo-draw (interactivity handled in main.c)
            draw_button(renderer, font, &btn_inv);
            
            displayed_count++;
        }
    }
    
    if(displayed_count == 0) {
       SDL_Surface *msg = TTF_RenderText_Blended(font, "No friends online", CLR_GRAY);
       if(msg) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, msg);
            SDL_Rect r = {x + (container_w - msg->w)/2, y + 200, msg->w, msg->h};
            SDL_RenderCopy(renderer, tex, NULL, &r);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(msg);
       } 
    }
}

void render_invitation_popup(SDL_Renderer *renderer, TTF_Font *font,
                            IncomingInvite *invite,
                            Button *accept_btn, Button *decline_btn) {
    int win_w, win_h;
    SDL_GetRendererOutputSize(renderer, &win_w, &win_h);
    
    // 1. Overlay
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
    SDL_Rect overlay = {0, 0, win_w, win_h};
    SDL_RenderFillRect(renderer, &overlay);
    
    // 2. Dialog Box
    int box_w = 400;
    int box_h = 250;
    int x = (win_w - box_w) / 2;
    int y = (win_h - box_h) / 2;
    SDL_Rect box = {x, y, box_w, box_h};
    
    draw_layered_shadow(renderer, box, 12, 12);
    draw_rounded_rect(renderer, box, CLR_BG, 12);
    // Pulsing border effect (simple version)
    draw_rounded_border(renderer, box, CLR_ACCENT, 12, 3);
    
    // 3. Content
    // Title
    SDL_Surface *title = TTF_RenderText_Blended(font, "GAME INVITATION", CLR_ACCENT);
    if(title) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, title);
        SDL_Rect r = {x + (box_w - title->w)/2, y + 20, title->w, title->h};
        SDL_RenderCopy(renderer, tex, NULL, &r);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(title);
    }
    
    // Host Name
    char msg[128];
    snprintf(msg, sizeof(msg), "%s invited you to play!", invite->host_name);
    SDL_Surface *text = TTF_RenderText_Blended(font, msg, CLR_WHITE);
    if(text) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, text);
        SDL_Rect r = {x + (box_w - text->w)/2, y + 70, text->w, text->h};
        SDL_RenderCopy(renderer, tex, NULL, &r);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(text);
    }
    
    // Room Name
    snprintf(msg, sizeof(msg), "Room: %s", invite->room_name);
    text = TTF_RenderText_Blended(font, msg, CLR_GRAY);
    if(text) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, text);
        SDL_Rect r = {x + (box_w - text->w)/2, y + 110, text->w, text->h};
        SDL_RenderCopy(renderer, tex, NULL, &r);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(text);
    }
    
    // Buttons
    accept_btn->rect.x = x + 40;
    accept_btn->rect.y = y + 180;
    decline_btn->rect.x = x + 210;
    decline_btn->rect.y = y + 180;
    
    draw_button(renderer, font, accept_btn);
    draw_button(renderer, font, decline_btn);
}