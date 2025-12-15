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
    
    // Simple background
    SDL_SetRenderDrawColor(renderer, 15, 23, 42, 255);
    SDL_RenderClear(renderer);
    
    // Title
    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface *surf = TTF_RenderText_Blended(font, "Friends Screen - Under Development", white);
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect rect = {(win_w - surf->w) / 2, 100, surf->w, surf->h};
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }
    
    // Show count
    char msg[128];
    snprintf(msg, sizeof(msg), "Friends: %d | Pending Requests: %d", friend_count, pending_count);
    surf = TTF_RenderText_Blended(font, msg, white);
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect rect = {(win_w - surf->w) / 2, 200, surf->w, surf->h};
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }
    
    // Back button
    back_btn->rect = (SDL_Rect){win_w/2 - 75, win_h - 100, 150, 50};
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
    
    SDL_RenderPresent(renderer);
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
    SDL_Surface *surf = TTF_RenderText_Blended(font_large, "Profile Screen - Under Development", white);
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect rect = {(win_w - surf->w) / 2, 100, surf->w, surf->h};
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }
    
    char msg[256];
    snprintf(msg, sizeof(msg), "ELO: %d | Matches: %d | Wins: %d", 
             profile->elo_rating, profile->total_matches, profile->wins);
    surf = TTF_RenderText_Blended(font_small, msg, white);
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect rect = {(win_w - surf->w) / 2, 200, surf->w, surf->h};
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
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
    
    SDL_RenderPresent(renderer);
}

// Placeholder leaderboard screen
void render_leaderboard_screen(SDL_Renderer *renderer, TTF_Font *font,
                               LeaderboardEntry *entries, int entry_count,
                               Button *back_btn) {
    int win_w, win_h;
    SDL_GetRendererOutputSize(renderer, &win_w, &win_h);
    
    SDL_SetRenderDrawColor(renderer, 15, 23, 42, 255);
    SDL_RenderClear(renderer);
    
    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface *surf = TTF_RenderText_Blended(font, "Leaderboard - Under Development", white);
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect rect = {(win_w - surf->w) / 2, 100, surf->w, surf->h};
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }
    
    char msg[128];
    snprintf(msg, sizeof(msg), "Top %d Players", entry_count);
    surf = TTF_RenderText_Blended(font, msg, white);
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect rect = {(win_w - surf->w) / 2, 200, surf->w, surf->h};
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }
    
    back_btn->rect = (SDL_Rect){win_w/2 - 75, win_h - 100, 150, 50};
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
    
    SDL_RenderPresent(renderer);
}
