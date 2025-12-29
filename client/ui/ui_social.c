#include "../graphics/graphics.h"
#include "ui.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#define UI_CORNER_RADIUS 8
// Enhanced profile screen with card-based stats layout
void render_profile_screen(SDL_Renderer *renderer, TTF_Font *font_medium, TTF_Font *font_small,
                           ProfileData *profile,
                           Button *back_btn, const char *title_override) {
    int win_w, win_h;
    SDL_GetRendererOutputSize(renderer, &win_w, &win_h);
    
    draw_background_grid(renderer, win_w, win_h);
    
    // Title with shadow
    const char *title_text = (title_override && strlen(title_override) > 0) ? title_override : "YOUR PROFILE";
    SDL_Surface *title_shadow = TTF_RenderText_Blended(font_medium, title_text, (SDL_Color){0, 0, 0, 120});
    if (title_shadow) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, title_shadow);
        SDL_Rect rect = {(win_w - title_shadow->w) / 2 + 3, 30, title_shadow->w, title_shadow->h};
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(title_shadow);
    }
    
    SDL_Surface *surf = TTF_RenderText_Blended(font_medium, title_text, CLR_ACCENT);
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect rect = {(win_w - surf->w) / 2, 27, surf->w, surf->h};
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }
    
    if (profile) {
        // ELO card - TALLER to fit large number (was 100, now 130)
        int elo_y = 160;
        int elo_card_w = 480;
        int elo_card_h = 120;  // INCREASED from 100
        SDL_Rect elo_card = {320, elo_y, elo_card_w, elo_card_h};
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
        
        // Large ELO number - positioned lower to fit
        char elo_text[16];
        snprintf(elo_text, sizeof(elo_text), "%d", profile->elo_rating);
        surf = TTF_RenderText_Blended(font_medium, elo_text, CLR_ACCENT);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {elo_card.x + (elo_card.w - surf->w)/2, elo_y + 55, surf->w, surf->h};  // Y: 45->55
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        // 2x2 stat grid - EVEN TALLER cards with old color scheme
        int grid_y = elo_y + elo_card_h + 40;
        int card_w = 300;
        int card_h = 125;  // INCREASED from 100 to 125
        int spacing = 20;
        int start_x = 240;
        
        typedef struct {
            const char *label;
            int value;
            SDL_Color color;
        } StatCard;
        
        StatCard stats[4] = {
            {"MATCHES", profile->total_matches, CLR_INPUT_BG},  // Grey background
            {"WINS", profile->wins, CLR_INPUT_BG},             // Grey background
            {"KILLS", profile->total_kills, CLR_INPUT_BG},     // Grey background
            {"DEATHS", profile->deaths, CLR_INPUT_BG}          // Grey background
        };
        
        // Border colors for each stat
        SDL_Color border_colors[4] = {
            CLR_PRIMARY,   // Matches - blue
            CLR_SUCCESS,   // Wins - green
            CLR_ACCENT,    // Kills - orange
            CLR_DANGER     // Deaths - red
        };
        
        for (int i = 0; i < 4; i++) {
            int row = i / 2;
            int col = i % 2;
            int x = start_x + col * (card_w + spacing);
            int y = grid_y + row * (card_h + spacing);
            
            SDL_Rect card = {x, y, card_w, card_h};
            draw_layered_shadow(renderer, card, UI_CORNER_RADIUS, 4);
            draw_rounded_rect(renderer, card, stats[i].color, UI_CORNER_RADIUS);
            draw_rounded_border(renderer, card, border_colors[i], UI_CORNER_RADIUS, 2);  // Colored border
            
            // Label at TOP
            surf = TTF_RenderText_Blended(font_small, stats[i].label, CLR_GRAY);
            if (surf) {
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
                SDL_Rect rect = {x + (card_w - surf->w)/2, y + 20, surf->w, surf->h};  // More top padding
                SDL_RenderCopy(renderer, tex, NULL, &rect);
                SDL_DestroyTexture(tex);
                SDL_FreeSurface(surf);
            }
            
            // Large value number at BOTTOM - clear separation
            char value_text[16];
            snprintf(value_text, sizeof(value_text), "%d", stats[i].value);
            surf = TTF_RenderText_Blended(font_medium, value_text, CLR_WHITE);
            if (surf) {
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
                SDL_Rect rect = {x + (card_w - surf->w)/2, y + 65, surf->w, surf->h};  // More bottom space
                SDL_RenderCopy(renderer, tex, NULL, &rect);
                SDL_DestroyTexture(tex);
                SDL_FreeSurface(surf);
            }
        }
    }
    
    // LARGER back button - bottom center, adjusted for 1120x720
    back_btn->rect = (SDL_Rect){460, 650, 200, 60};
    strcpy(back_btn->text, "Back");
    draw_button(renderer, font_small, back_btn);
}

// Enhanced leaderboard screen with visual rank badges
void render_leaderboard_screen(SDL_Renderer *renderer, TTF_Font *font_medium, TTF_Font *font_small,
                               LeaderboardEntry *entries, int entry_count,
                               Button *back_btn) {
    int win_w, win_h;
    SDL_GetRendererOutputSize(renderer, &win_w, &win_h);
    
    draw_background_grid(renderer, win_w, win_h);
    
    // Title with shadow
    const char *title_text = "LEADERBOARD - TOP PLAYERS";
    SDL_Surface *title_shadow = TTF_RenderText_Blended(font_medium, title_text, (SDL_Color){0, 0, 0, 120});
    if (title_shadow) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, title_shadow);
        SDL_Rect rect = {(win_w - title_shadow->w) / 2 + 3, 30, title_shadow->w, title_shadow->h};
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(title_shadow);
    }
    
    SDL_Surface *surf = TTF_RenderText_Blended(font_medium, title_text, CLR_ACCENT);
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect rect = {(win_w - surf->w) / 2, 27, surf->w, surf->h};
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }
    
    // FIXED leaderboard for 1120x720
    int y = 120;  
    int max_entries = entry_count < 10 ? entry_count : 10;
    int row_width = 616;  // 55% of 1120
    int row_height = 65;  // Fixed height
    int start_x = 252;  // Centered
    
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
        
        // Rank badge - better vertical centering  
        int badge_size = 45;
        SDL_Rect rank_badge = {start_x + 15, y + 13, badge_size, badge_size};  // Y: 10->13 for centering
        SDL_Color badge_color = (i < 3) ? medal_colors[i] : CLR_PRIMARY;
        draw_rounded_rect(renderer, rank_badge, badge_color, badge_size/2);
        
        // Rank number - vertically centered
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
        
        // Player name - positioned HIGHER to avoid overlap
        char name_display[64];
        strncpy(name_display, entries[i].display_name, sizeof(name_display) - 1);
        name_display[sizeof(name_display) - 1] = '\0';
        truncate_text_to_fit(name_display, sizeof(name_display), font_small, 700);
        
        SDL_Color name_color = (i < 3) ? CLR_ACCENT : CLR_WHITE;
        surf = TTF_RenderText_Blended(font_small, name_display, name_color);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {start_x + badge_size + 25, y + 8, surf->w, surf->h};  // Y: 12->18
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        // ELO and Wins info - positioned LOWER with better contrast
        char info[128];
        snprintf(info, sizeof(info), "ELO: %d  |  Wins: %d", entries[i].elo_rating, entries[i].wins);
        truncate_text_to_fit(info, sizeof(info), font_small, 700);
        
        // BRIGHTER grey for better contrast
        SDL_Color info_color = {200, 200, 200, 255};  // Brighter than CLR_GRAY
        surf = TTF_RenderText_Blended(font_small, info, info_color);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {start_x + badge_size + 25, y + 33, surf->w, surf->h};  // Y: was row_height-20, now 43 (safely in middle)
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        y += row_height + 16;
    }
    
    // LARGER back button - bottom center
    back_btn->rect = (SDL_Rect){460, 650, 200, 60};
    strcpy(back_btn->text, "Back");
    draw_button(renderer, font_small, back_btn);
}