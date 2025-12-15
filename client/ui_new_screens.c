/* Enhanced UI screens with modern styling */
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string.h>
#include <stdio.h>
#include "../common/protocol.h"
#include "ui.h"

// External UI constants and colors from ui_screens.c
#define UI_CORNER_RADIUS 8
#define UI_SHADOW_OFFSET 4

extern const SDL_Color CLR_BG;
extern const SDL_Color CLR_BG_DARK;
extern const SDL_Color CLR_PRIMARY;
extern const SDL_Color CLR_ACCENT;
extern const SDL_Color CLR_WHITE;
extern const SDL_Color CLR_GRAY;
extern const SDL_Color CLR_SUCCESS;
extern const SDL_Color CLR_WARNING;
extern const SDL_Color CLR_DANGER;
extern const SDL_Color CLR_INPUT_BG;
extern const SDL_Color CLR_GLOW;

// External helper functions from ui_screens.c
extern void draw_rounded_rect(SDL_Renderer *renderer, SDL_Rect rect, SDL_Color color, int radius);
extern void draw_rounded_border(SDL_Renderer *renderer, SDL_Rect rect, SDL_Color color, int radius, int thickness);
extern void draw_layered_shadow(SDL_Renderer *renderer, SDL_Rect rect, int radius, int offset);
extern void draw_background_grid(SDL_Renderer *renderer, int w, int h);

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
    
    // Friend cards with modern styling
    list_y += 40;
    for (int i = 0; i < friend_count && i < 5; i++) {
        SDL_Rect card = {50, list_y, 350, 65};
        
        // Layered shadow
        draw_layered_shadow(renderer, card, UI_CORNER_RADIUS, 3);
        
        // Background
        draw_rounded_rect(renderer, card, CLR_INPUT_BG, UI_CORNER_RADIUS);
        
        // Border
        draw_rounded_border(renderer, card, CLR_PRIMARY, UI_CORNER_RADIUS, 1);
        
        // Online status indicator (pulsing dot)
        if (friends[i].is_online > 0) {
            float pulse = (sinf(SDL_GetTicks() / 600.0f) + 1.0f) / 2.0f;
            int dot_size = 8 + (int)(pulse * 3);
            SDL_Rect status_dot = {60, list_y + 15, dot_size, dot_size};
            SDL_Color dot_color = CLR_SUCCESS;
            dot_color.a = 200 + (int)(pulse * 55);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            draw_rounded_rect(renderer, status_dot, dot_color, dot_size/2);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        }
        
        // Name
        surf = TTF_RenderText_Blended(font, friends[i].display_name, CLR_WHITE);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {75, list_y + 10, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        // Status and ELO
        char info[64];
        const char *status = friends[i].is_online == 1 ? "Online" : (friends[i].is_online == 2 ? "In Game" : "Offline");
        snprintf(info, sizeof(info), "%s | ELO: %d", status, friends[i].elo_rating);
        SDL_Color status_color = friends[i].is_online > 0 ? CLR_SUCCESS : CLR_GRAY;
        surf = TTF_RenderText_Blended(font, info, status_color);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {75, list_y + 38, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        // Remove button (rounded red)
        SDL_Rect remove_btn = {360, list_y + 20, 30, 26};
        draw_rounded_rect(renderer, remove_btn, CLR_DANGER, 4);
        surf = TTF_RenderText_Blended(font, "X", CLR_WHITE);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {remove_btn.x + 8, remove_btn.y + 4, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        list_y += 75;
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
    
    // Pending request cards - MODERNIZED
    pending_y += 40;
    for (int i = 0; i < pending_count && i < 5; i++) {
        SDL_Rect card = {450, pending_y, 350, 60};
        
        // Layered shadow
        draw_layered_shadow(renderer, card, UI_CORNER_RADIUS, 3);
        
        // Background
        draw_rounded_rect(renderer, card, CLR_INPUT_BG, UI_CORNER_RADIUS);
        
        // Yellow border for pending
        draw_rounded_border(renderer, card, CLR_WARNING, UI_CORNER_RADIUS, 2);
        
        // Name
        surf = TTF_RenderText_Blended(font, pending[i].display_name, white);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {460, pending_y + 8, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        // Accept button (rounded green)
        SDL_Rect accept_btn = {460, pending_y + 32, 80, 24};
        draw_rounded_rect(renderer, accept_btn, CLR_SUCCESS, 4);
        surf = TTF_RenderText_Blended(font, "Accept", white);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {accept_btn.x + 10, accept_btn.y + 3, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        // Decline button (rounded red)
        SDL_Rect decline_btn = {550, pending_y + 32, 80, 24};
        draw_rounded_rect(renderer, decline_btn, CLR_DANGER, 4);
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
    
    // Sent request cards - MODERNIZED
    sent_y += 360;
    for (int i = 0; i < sent_count && i < 3; i++) {
        SDL_Rect card = {450, sent_y, 350, 50};
        
        // Layered shadow
        draw_layered_shadow(renderer, card, UI_CORNER_RADIUS, 3);
        
        // Background
        draw_rounded_rect(renderer, card, CLR_INPUT_BG, UI_CORNER_RADIUS);
        
        // Gray border for sent
        draw_rounded_border(renderer, card, CLR_GRAY, UI_CORNER_RADIUS, 1);
        
        // Name
        surf = TTF_RenderText_Blended(font, sent_requests[i].display_name, CLR_GRAY);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {460, sent_y + 8, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        // "Pending..." text
        surf = TTF_RenderText_Blended(font, "Pending...", CLR_GRAY);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {460, sent_y + 28, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        // Cancel button (rounded red X)
        SDL_Rect cancel_btn = {760, sent_y + 13, 30, 24};
        draw_rounded_rect(renderer, cancel_btn, CLR_DANGER, 4);
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
    
    // Back button - MODERNIZED (using draw_button helper)
    back_btn->rect = (SDL_Rect){win_w/2 - 75, win_h - 60, 150, 50};
    strcpy(back_btn->text, "Back");
    draw_button(renderer, font, back_btn);
}

// Enhanced profile screen with card-based stats layout
void render_profile_screen(SDL_Renderer *renderer, TTF_Font *font_large, TTF_Font *font_small,
                           ProfileData *profile,
                           Button *back_btn) {
    int win_w, win_h;
    SDL_GetRendererOutputSize(renderer, &win_w, &win_h);
    
    draw_background_grid(renderer, win_w, win_h);
    
    // Title with shadow
    const char *title_text = "YOUR PROFILE";
    SDL_Surface *title_shadow = TTF_RenderText_Blended(font_large, title_text, (SDL_Color){0, 0, 0, 120});
    if (title_shadow) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, title_shadow);
        SDL_Rect rect = {(win_w - title_shadow->w) / 2 + 3, 33, title_shadow->w, title_shadow->h};
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(title_shadow);
    }
    
    SDL_Surface *surf = TTF_RenderText_Blended(font_large, title_text, CLR_ACCENT);
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect rect = {(win_w - surf->w) / 2, 30, surf->w, surf->h};
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }
    
    if (profile) {
        // Large ELO rating card (center, prominent)
        int elo_y = 120;
        SDL_Rect elo_card = {win_w/2 - 150, elo_y, 300, 100};
        draw_layered_shadow(renderer, elo_card, UI_CORNER_RADIUS, 5);
        draw_rounded_rect(renderer, elo_card, CLR_PRIMARY, UI_CORNER_RADIUS);
        draw_rounded_border(renderer, elo_card, CLR_ACCENT, UI_CORNER_RADIUS, 2);
        
        // "ELO RATING" label
        surf = TTF_RenderText_Blended(font_small, "ELO RATING", CLR_GRAY);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {elo_card.x + (elo_card.w - surf->w)/2, elo_y + 15, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        // Large ELO number
        char elo_text[16];
        snprintf(elo_text, sizeof(elo_text), "%d", profile->elo_rating);
        surf = TTF_RenderText_Blended(font_large, elo_text, CLR_ACCENT);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {elo_card.x + (elo_card.w - surf->w)/2, elo_y + 45, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        // Stats cards in a 2x2 grid
        int card_y = 250;
        int card_w = 220;
        int card_h = 80;
        int spacing = 20;
        int start_x = win_w/2 - (card_w + spacing/2);
        
        typedef struct {
            const char *label;
            int value;
            SDL_Color color;
        } StatCard;
        
        StatCard stats[4] = {
            {"MATCHES", profile->total_matches, CLR_PRIMARY},
            {"WINS", profile->wins, CLR_SUCCESS},
            {"KILLS", profile->total_kills, CLR_ACCENT},
            {"DEATHS", profile->deaths, CLR_DANGER}
        };
        
        for (int i = 0; i < 4; i++) {
            int col = i % 2;
            int row = i / 2;
            int x = start_x + col * (card_w + spacing);
            int y = card_y + row * (card_h + spacing);
            
            SDL_Rect card = {x, y, card_w, card_h};
            draw_layered_shadow(renderer, card, UI_CORNER_RADIUS, 3);
            draw_rounded_rect(renderer, card, CLR_INPUT_BG, UI_CORNER_RADIUS);
            draw_rounded_border(renderer, card, stats[i].color, UI_CORNER_RADIUS, 1);
            
            // Label
            surf = TTF_RenderText_Blended(font_small, stats[i].label, CLR_GRAY);
            if (surf) {
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
                SDL_Rect rect = {x + 15, y + 12, surf->w, surf->h};
                SDL_RenderCopy(renderer, tex, NULL, &rect);
                SDL_DestroyTexture(tex);
                SDL_FreeSurface(surf);
            }
            
            // Value
            char value_text[32];
            snprintf(value_text, sizeof(value_text), "%d", stats[i].value);
            surf = TTF_RenderText_Blended(font_large, value_text, CLR_WHITE);
            if (surf) {
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
                SDL_Rect rect = {x + 15, y + 40, surf->w, surf->h};
                SDL_RenderCopy(renderer, tex, NULL, &rect);
                SDL_DestroyTexture(tex);
                SDL_FreeSurface(surf);
            }
        }
    }
    
    // Back button (now using draw_button)
    back_btn->rect = (SDL_Rect){win_w/2 - 75, win_h - 80, 150, 50};
    strcpy(back_btn->text, "Back");
    draw_button(renderer, font_small, back_btn);
}

// Enhanced leaderboard screen with visual rank badges
void render_leaderboard_screen(SDL_Renderer *renderer, TTF_Font *font_large, TTF_Font *font_small,
                               LeaderboardEntry *entries, int entry_count,
                               Button *back_btn) {
    int win_w, win_h;
    SDL_GetRendererOutputSize(renderer, &win_w, &win_h);
    
    draw_background_grid(renderer, win_w, win_h);
    
    // Title with shadow
    const char *title_text = "LEADERBOARD - TOP PLAYERS";
    SDL_Surface *title_shadow = TTF_RenderText_Blended(font_large, title_text, (SDL_Color){0, 0, 0, 120});
    if (title_shadow) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, title_shadow);
        SDL_Rect rect = {(win_w - title_shadow->w) / 2 + 3, 33, title_shadow->w, title_shadow->h};
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(title_shadow);
    }
    
    SDL_Surface *surf = TTF_RenderText_Blended(font_large, title_text, CLR_ACCENT);
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect rect = {(win_w - surf->w) / 2, 30, surf->w, surf->h};
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }
    
    // Leaderboard entries with modern card design
    int y = 110;
    int max_entries = entry_count < 10 ? entry_count : 10;
    int row_width = 700;
    int row_height = 60;
    int start_x = (win_w - row_width) / 2;
    
    // Medal colors for top 3
    SDL_Color medal_colors[3] = {
        {255, 215, 0, 255},   // Gold
        {192, 192, 192, 255}, // Silver
        {205, 127, 50, 255}   // Bronze
    };
    
    for (int i = 0; i < max_entries; i++) {
        SDL_Rect row = {start_x, y, row_width, row_height};
        
        // Shadow (stronger for top 3)
        int shadow_offset = (i < 3) ? 5 : 3;
        draw_layered_shadow(renderer, row, UI_CORNER_RADIUS, shadow_offset);
        
        // Background color (special for top 3)
        SDL_Color bg = (i < 3) ? (SDL_Color){45, 55, 75, 255} : CLR_INPUT_BG;
        draw_rounded_rect(renderer, row, bg, UI_CORNER_RADIUS);
        
        // Border (colored for top 3)
        SDL_Color border = (i < 3) ? medal_colors[i] : CLR_GRAY;
        draw_rounded_border(renderer, row, border, UI_CORNER_RADIUS, (i < 3) ? 2 : 1);
        
        // Rank badge (circular)
        int badge_size = 40;
        SDL_Rect rank_badge = {start_x + 15, y + (row_height - badge_size)/2, badge_size, badge_size};
        SDL_Color badge_color = (i < 3) ? medal_colors[i] : CLR_PRIMARY;
        draw_rounded_rect(renderer, rank_badge, badge_color, badge_size/2);
        
        // Rank number
        char rank_text[8];
        snprintf(rank_text, sizeof(rank_text), "%d", i + 1);
        SDL_Color rank_text_color = (i < 3) ? (SDL_Color){0, 0, 0, 255} : CLR_WHITE;
        surf = TTF_RenderText_Blended(font_small, rank_text, rank_text_color);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {
                rank_badge.x + (badge_size - surf->w)/2,
                rank_badge.y + (badge_size - surf->h)/2,
                surf->w, surf->h
            };
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        // Player name
        SDL_Color name_color = (i < 3) ? CLR_ACCENT : CLR_WHITE;
        surf = TTF_RenderText_Blended(font_small, entries[i].display_name, name_color);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {start_x + 70, y + 12, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        // ELO and Wins info
        char info[128];
        snprintf(info, sizeof(info), "ELO: %d  |  Wins: %d", entries[i].elo_rating, entries[i].wins);
        surf = TTF_RenderText_Blended(font_small, info, CLR_GRAY);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {start_x + 70, y + 35, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        y += row_height + 8;
    }
    
    // Back button (using draw_button)
    back_btn->rect = (SDL_Rect){win_w/2 - 75, win_h - 70, 150, 50};
    strcpy(back_btn->text, "Back");
    draw_button(renderer, font_small, back_btn);
}

