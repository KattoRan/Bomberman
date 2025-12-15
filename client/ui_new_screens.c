/* Temporary placeholder UI screens - will be enhanced */
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string.h>
#include <stdio.h>
#include "../common/protocol.h"
#include "ui.h"

// Placeholder friends screen (shows "Under Development")
void render_friends_screen(SDL_Renderer *renderer, TTF_Font *font,
                           FriendInfo *friends, int friend_count,
                           FriendInfo *pending, int pending_count,
                           Button *back_btn) {
    int win_w, win_h;
    SDL_GetRendererOutputSize(renderer, &win_w, &win_h);
    
    SDL_SetRenderDrawColor(renderer, 15, 23, 42, 255);
    SDL_RenderClear(renderer);
    
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color gray = {148, 163, 184, 255};
    SDL_Color green = {34, 197, 94, 255};
    SDL_Color yellow = {234, 179, 8, 255};
    
    // Title
    SDL_Surface *surf = TTF_RenderText_Blended(font, "Friends", white);
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect rect = {50, 30, surf->w, surf->h};
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }
    
    // Friends list section
    int list_y = 100;
    char title[64];
    snprintf(title, sizeof(title), "Your Friends (%d)", friend_count);
    surf = TTF_RenderText_Blended(font, title, white);
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect rect = {50, list_y, surf->w, surf->h};
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }
    
    // Friend cards
    list_y += 40;
    for (int i = 0; i < friend_count && i < 5; i++) {
        SDL_Rect card = {50, list_y, 350, 60};
        SDL_SetRenderDrawColor(renderer, 30, 41, 59, 255);
        SDL_RenderFillRect(renderer, &card);
        SDL_SetRenderDrawColor(renderer, 59, 130, 246, 255);
        SDL_RenderDrawRect(renderer, &card);
        
        // Name
        surf = TTF_RenderText_Blended(font, friends[i].display_name, white);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {60, list_y + 10, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        // Status and ELO
        char info[64];
        const char *status = friends[i].is_online == 1 ? "Online" : (friends[i].is_online == 2 ? "In Game" : "Offline");
        snprintf(info, sizeof(info), "%s | ELO: %d", status, friends[i].elo_rating);
        SDL_Color status_color = friends[i].is_online > 0 ? green : gray;
        surf = TTF_RenderText_Blended(font, info, status_color);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {60, list_y + 35, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        // Remove button (red X)
        SDL_Rect remove_btn = {360, list_y + 18, 30, 24};
        SDL_SetRenderDrawColor(renderer, 239, 68, 68, 255);  // Red
        SDL_RenderFillRect(renderer, &remove_btn);
        surf = TTF_RenderText_Blended(font, "X", white);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {remove_btn.x + 8, remove_btn.y + 3, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        list_y += 70;
    }
    
    // Pending requests section
    int pending_y = 100;
    snprintf(title, sizeof(title), "Pending Requests (%d)", pending_count);
    surf = TTF_RenderText_Blended(font, title, white);
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect rect = {450, pending_y, surf->w, surf->h};
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }
    
    // Pending request cards
    pending_y += 40;
    for (int i = 0; i < pending_count && i < 5; i++) {
        SDL_Rect card = {450, pending_y, 350, 60};
        SDL_SetRenderDrawColor(renderer, 30, 41, 59, 255);
        SDL_RenderFillRect(renderer, &card);
        SDL_SetRenderDrawColor(renderer, 234, 179, 8, 255);
        SDL_RenderDrawRect(renderer, &card);
        
        // Name
        surf = TTF_RenderText_Blended(font, pending[i].display_name, white);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {460, pending_y + 8, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        // Accept button (green)
        SDL_Rect accept_btn = {460, pending_y + 32, 80, 22};
        SDL_SetRenderDrawColor(renderer, 34, 197, 94, 255);  // Green
        SDL_RenderFillRect(renderer, &accept_btn);
        surf = TTF_RenderText_Blended(font, "Accept", white);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {accept_btn.x + 10, accept_btn.y + 3, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        // Decline button (red)
        SDL_Rect decline_btn = {550, pending_y + 32, 80, 22};
        SDL_SetRenderDrawColor(renderer, 239, 68, 68, 255);  // Red
        SDL_RenderFillRect(renderer, &decline_btn);
        surf = TTF_RenderText_Blended(font, "Decline", white);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {decline_btn.x + 8, decline_btn.y + 3, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        pending_y += 70;
    }
    
    // Sent requests section (outgoing)
    extern FriendInfo sent_requests[];
    extern int sent_count;
    
    int sent_y = 100;
    snprintf(title, sizeof(title), "Sent Requests (%d)", sent_count);
    surf = TTF_RenderText_Blended(font, title, white);
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect rect = {450, sent_y + 320, surf->w, surf->h};
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }
    
    // Sent request cards
    sent_y += 360;
    for (int i = 0; i < sent_count && i < 3; i++) {
        SDL_Rect card = {450, sent_y, 350, 50};
        SDL_SetRenderDrawColor(renderer, 30, 41, 59, 255);
        SDL_RenderFillRect(renderer, &card);
        SDL_SetRenderDrawColor(renderer, 148, 163, 184, 255);
        SDL_RenderDrawRect(renderer, &card);
        
        // Name
        surf = TTF_RenderText_Blended(font, sent_requests[i].display_name, gray);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {460, sent_y + 8, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        // "Pending..." text
        surf = TTF_RenderText_Blended(font, "Pending...", gray);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {460, sent_y + 28, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        // Cancel button (small red X)
        SDL_Rect cancel_btn = {760, sent_y + 13, 30, 24};
        SDL_SetRenderDrawColor(renderer, 239, 68, 68, 255);
        SDL_RenderFillRect(renderer, &cancel_btn);
        surf = TTF_RenderText_Blended(font, "X", white);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {cancel_btn.x + 8, cancel_btn.y + 3, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        sent_y += 60;
    }
    
    // Send friend request section
    surf = TTF_RenderText_Blended(font, "Send Friend Request", white);
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect rect = {50, win_h - 150, surf->w, surf->h};
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }
    
    // Note: Input field and button rendered separately in main.c
    
    // Back button
    back_btn->rect = (SDL_Rect){win_w/2 - 75, win_h - 60, 150, 50};
    strcpy(back_btn->text, "Back");
    
    SDL_Color btn_color = back_btn->is_hovered ? (SDL_Color){96, 165, 250, 255} : (SDL_Color){59, 130, 246, 255};
    SDL_SetRenderDrawColor(renderer, btn_color.r, btn_color.g, btn_color.b, 255);
    SDL_RenderFillRect(renderer, &back_btn->rect);
    
    surf = TTF_RenderText_Blended(font, back_btn->text, white);
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect rect = {
            back_btn->rect.x + (back_btn->rect.w - surf->w) / 2,
            back_btn->rect.y + (back_btn->rect.h - surf->h) / 2,
            surf->w, surf->h
        };
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }
}

// Placeholder profile screen
void render_profile_screen(SDL_Renderer *renderer, TTF_Font *font_large, TTF_Font *font_small,
                           ProfileData *profile,
                           Button *back_btn) {
    int win_w, win_h;
    SDL_GetRendererOutputSize(renderer, &win_w, &win_h);
    
    SDL_SetRenderDrawColor(renderer, 15, 23, 42, 255);
    SDL_RenderClear(renderer);
    
    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface *surf = TTF_RenderText_Blended(font_large, "Your Profile", white);
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect rect = {(win_w - surf->w) / 2, 100, surf->w, surf->h};
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }
    
    if (profile) {
        char msg[256];
        snprintf(msg, sizeof(msg), "ELO Rating: %d | Matches: %d | Wins: %d | Kills: %d", 
                 profile->elo_rating, profile->total_matches, profile->wins, profile->total_kills);
        surf = TTF_RenderText_Blended(font_small, msg, white);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {(win_w - surf->w) / 2, 200, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
    }
    
    back_btn->rect = (SDL_Rect){win_w/2 - 75, win_h - 100, 150, 50};
    strcpy(back_btn->text, "Back");
    
    SDL_Color btn_color = back_btn->is_hovered ? (SDL_Color){96, 165, 250, 255} : (SDL_Color){59, 130, 246, 255};
    SDL_SetRenderDrawColor(renderer, btn_color.r, btn_color.g, btn_color.b, 255);
    SDL_RenderFillRect(renderer, &back_btn->rect);
    
    surf = TTF_RenderText_Blended(font_small, back_btn->text, white);
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect rect = {
            back_btn->rect.x + (back_btn->rect.w - surf->w) / 2,
            back_btn->rect.y + (back_btn->rect.h - surf->h) / 2,
            surf->w, surf->h
        };
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }
}

// Leaderboard screen with ranking table
void render_leaderboard_screen(SDL_Renderer *renderer, TTF_Font *font_large, TTF_Font *font_small,
                               LeaderboardEntry *entries, int entry_count,
                               Button *back_btn) {
    int win_w, win_h;
    SDL_GetRendererOutputSize(renderer, &win_w, &win_h);
    
    SDL_SetRenderDrawColor(renderer, 15, 23, 42, 255);
    SDL_RenderClear(renderer);
    
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color gold = {234, 179, 8, 255};
    
    // Title
    SDL_Surface *surf = TTF_RenderText_Blended(font_large, "Leaderboard - Top Players", white);
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect rect = {(win_w - surf->w) / 2, 30, surf->w, surf->h};
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }
    
    // Table rendering
    int y = 120;
    int max_entries = entry_count < 10 ? entry_count : 10;
    
    for (int i = 0; i < max_entries; i++) {
        char line[256];
        snprintf(line, sizeof(line), "#%d  %s  -  ELO: %d  |  Wins: %d", 
                i+1, entries[i].display_name, entries[i].elo_rating, entries[i].wins);
        
        SDL_Color color = (i < 3) ? gold : white;
        surf = TTF_RenderText_Blended(font_small, line, color);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {100, y, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        y += 40;
    }
    
    // Back button
    back_btn->rect = (SDL_Rect){win_w/2 - 75, win_h - 80, 150, 50};
    strcpy(back_btn->text, "Back");
    
    SDL_Color btn_color = back_btn->is_hovered ? (SDL_Color){96, 165, 250, 255} : (SDL_Color){59, 130, 246, 255};
    SDL_SetRenderDrawColor(renderer, btn_color.r, btn_color.g, btn_color.b, 255);
    SDL_RenderFillRect(renderer, &back_btn->rect);
    
    surf = TTF_RenderText_Blended(font_small, back_btn->text, white);
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect rect = {
            back_btn->rect.x + (back_btn->rect.w - surf->w) / 2,
            back_btn->rect.y + (back_btn->rect.h - surf->h) / 2,
            surf->w, surf->h
        };
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }
}

