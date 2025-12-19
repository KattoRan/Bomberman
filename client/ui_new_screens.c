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
extern const SDL_Color CLR_INPUT_BG;
extern const SDL_Color CLR_WARNING;
extern const SDL_Color CLR_DANGER;
// UI Constants and Colors
#define UI_CORNER_RADIUS 8
#define UI_SHADOW_OFFSET 4

// FIXED LAYOUT FOR 1920x1080 - removed all responsive code

extern int get_button_width(int screen_w);
extern int get_button_height(int screen_h);
extern int get_spacing(int screen_h);

// External helper functions from ui_screens.c
extern void draw_rounded_rect(SDL_Renderer *renderer, SDL_Rect rect, SDL_Color color, int radius);
extern void draw_rounded_border(SDL_Renderer *renderer, SDL_Rect rect, SDL_Color color, int radius, int thickness);
extern void draw_layered_shadow(SDL_Renderer *renderer, SDL_Rect rect, int radius, int offset);
extern void draw_background_grid(SDL_Renderer *renderer, int w, int h);
extern void draw_vertical_gradient(SDL_Renderer *renderer, SDL_Rect rect, SDL_Color top_color, SDL_Color bottom_color);
extern void truncate_text_to_fit(char *text, size_t text_size, TTF_Font *font, int max_width);

// Placeholder friends screen (shows "Under Development")
void render_friends_screen(SDL_Renderer *renderer, TTF_Font *font,
                           FriendInfo *friends, int friend_count,
                           FriendInfo *pending, int pending_count,
                           FriendInfo *sent, int sent_count,
                           Button *back_btn) {
    int win_w, win_h;
    SDL_GetRendererOutputSize(renderer, &win_w, &win_h);
    
    SDL_SetRenderDrawColor(renderer, 15, 23, 42, 255);
    SDL_RenderClear(renderer);
    
    SDL_Color white = {255, 255, 255, 255};
    
    // Title - Larger
    SDL_Surface *surf = TTF_RenderText_Blended(font, "Friends", CLR_ACCENT);
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect rect = {50, 40, surf->w, surf->h};
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }
    
    // LARGER cards - 750px wide (was 518px)
    int list_y = 140;
    int card_width = 750;
    
    // Your Friends section
    char title[64];
    snprintf(title, sizeof(title), "Your Friends (%d)", friend_count);
    surf = TTF_RenderText_Blended(font, title, white);
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect rect = {90, list_y, surf->w, surf->h};
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }
    
    // Friend cards - LARGER 110px tall (was 80px)
    list_y += 50;
    for (int i = 0; i < friend_count && i < 5; i++) {
        SDL_Rect card = {90, list_y, card_width, 110};
        
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
            SDL_Rect rect = {135, list_y + 20, surf->w, surf->h};
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
            SDL_Rect rect = {135, list_y + 60, surf->w, surf->h};
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
        
        list_y += 125;  // More spacing (was 75)
    }
    
    // Pending Requests section - right column
    int pending_y = 140;
    int pending_x = 900;  // More spacing from left column
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
    pending_y += 50;
    for (int i = 0; i < pending_count && i < 5; i++) {
        SDL_Rect card = {pending_x, pending_y, card_width, 110};  // INCREASED from 90
        
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
            SDL_Rect rect = {pending_x + 20, pending_y + 15, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        // Accept button - positioned LOWER to avoid overlap (Y: 48->60)
        SDL_Rect accept_btn = {pending_x + 20, pending_y + 60, 90, 35};
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
        SDL_Rect decline_btn = {pending_x + 125, pending_y + 60, 90, 35};
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
        
        pending_y += 125;  // More spacing (was 105)
    }
    
    // Sent Requests section - bottom left
    int sent_y = list_y + 40;
    snprintf(title, sizeof(title), "Sent Requests (%d)", sent_count);
    surf = TTF_RenderText_Blended(font, title, white);
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect rect = {90, sent_y, surf->w, surf->h};
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }
    
    sent_y += 50;
    for (int i = 0; i < sent_count && i < 3; i++) {
        SDL_Rect card = {90, sent_y, card_width, 80};
        
        draw_layered_shadow(renderer, card, UI_CORNER_RADIUS, 4);
        draw_vertical_gradient(renderer, card, CLR_INPUT_BG, CLR_BG);
        draw_rounded_border(renderer, card, CLR_GRAY, UI_CORNER_RADIUS, 2);
        
        // Name
        surf = TTF_RenderText_Blended(font, sent[i].display_name, CLR_GRAY);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {110, sent_y + 15, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        // Cancel button - LARGER
        SDL_Rect cancel_btn = {card.x + card.w - 100, sent_y + 20, 80, 35};
        draw_rounded_rect(renderer, cancel_btn, CLR_DANGER, 6);
        surf = TTF_RenderText_Blended(font, "Cancel", white);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {cancel_btn.x + (80 - surf->w)/2, cancel_btn.y + (35 - surf->h)/2, 
                            surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        sent_y += 95;
    }
    
    // Send friend request section
    surf = TTF_RenderText_Blended(font, "Send Friend Request", white);
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect rect = {50, win_h - 210, surf->w, surf->h};  // Changed from -150 to -210
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }
    
    // Note: Input field and button rendered separately in main.c
    
    // LARGER back button - bottom center
    back_btn->rect = (SDL_Rect){860, 950, 200, 60};  // Increased size
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
        // ELO card - TALLER to fit large number (was 100, now 130)
        int elo_y = 194;
        int elo_card_w = 480;
        int elo_card_h = 130;  // INCREASED from 100
        SDL_Rect elo_card = {720, elo_y, elo_card_w, elo_card_h};
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
        surf = TTF_RenderText_Blended(font_large, elo_text, CLR_ACCENT);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {elo_card.x + (elo_card.w - surf->w)/2, elo_y + 55, surf->w, surf->h};  // Y: 45->55
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        // 2x2 stat grid - EVEN TALLER cards with old color scheme
        int grid_y = elo_y + elo_card_h + 40;
        int card_w = 326;
        int card_h = 125;  // INCREASED from 100 to 125
        int spacing = 20;
        int start_x = 636;
        
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
            surf = TTF_RenderText_Blended(font_large, value_text, CLR_WHITE);
            if (surf) {
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
                SDL_Rect rect = {x + (card_w - surf->w)/2, y + 65, surf->w, surf->h};  // More bottom space
                SDL_RenderCopy(renderer, tex, NULL, &rect);
                SDL_DestroyTexture(tex);
                SDL_FreeSurface(surf);
            }
        }
    }
    
    // LARGER back button - bottom center
    back_btn->rect = (SDL_Rect){860, 950, 200, 60};
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
    
    // FIXED leaderboard for 1920x1080
    int y = 173;  // 16% of 1080
    int max_entries = entry_count < 10 ? entry_count : 10;
    int row_width = 1056;  // 55% of 1920
    int row_height = 65;  // Fixed height
    int start_x = 432;  // Centered
    
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
            SDL_Rect rect = {start_x + badge_size + 25, y + 18, surf->w, surf->h};  // Y: 12->18
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
            SDL_Rect rect = {start_x + badge_size + 25, y + 43, surf->w, surf->h};  // Y: was row_height-20, now 43 (safely in middle)
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        y += row_height + get_spacing(win_h);
    }
    
    // LARGER back button - bottom center
    back_btn->rect = (SDL_Rect){860, 950, 200, 60};
    strcpy(back_btn->text, "Back");
    draw_button(renderer, font_small, back_btn);
}
// Settings screen - 3 tabs: Graphics, Controls, Account
void render_settings_screen(SDL_Renderer *renderer, TTF_Font *font_large, TTF_Font *font_small,
                            int active_tab, Button *back_btn, Button *apply_btn) {
    int win_w, win_h;
    SDL_GetRendererOutputSize(renderer, &win_w, &win_h);
    
    draw_background_grid(renderer, win_w, win_h);
    
    // Title
    const char *title_text = "SETTINGS";
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
    
    // Tab buttons
    const char *tabs[3] = {"Graphics", "Controls", "Account"};
    int tab_y = 140;
    int tab_width = 180;
    int tab_height = 50;
    int tab_spacing = 20;
    int tabs_start_x = (win_w - (tab_width * 3 + tab_spacing * 2)) / 2;
    
    for (int i = 0; i < 3; i++) {
        SDL_Rect tab_rect = {tabs_start_x + i * (tab_width + tab_spacing), tab_y, tab_width, tab_height};
        
        SDL_Color tab_bg = (i == active_tab) ? CLR_PRIMARY : CLR_INPUT_BG;
        SDL_Color tab_border = (i == active_tab) ? CLR_ACCENT : CLR_GRAY;
        
        draw_layered_shadow(renderer, tab_rect, UI_CORNER_RADIUS, 3);
        draw_rounded_rect(renderer, tab_rect, tab_bg, UI_CORNER_RADIUS);
        draw_rounded_border(renderer, tab_rect, tab_border, UI_CORNER_RADIUS, 2);
        
        surf = TTF_RenderText_Blended(font_small, tabs[i], CLR_WHITE);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {tab_rect.x + (tab_width - surf->w) / 2,
                            tab_rect.y + (tab_height - surf->h) / 2,
                            surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
    }
    
    // Content area
    SDL_Rect content_area = {360, 240, 1200, 550};
    draw_layered_shadow(renderer, content_area, UI_CORNER_RADIUS, 5);
    draw_rounded_rect(renderer, content_area, CLR_INPUT_BG, UI_CORNER_RADIUS);
    draw_rounded_border(renderer, content_area, CLR_PRIMARY, UI_CORNER_RADIUS, 2);
    
    int content_x = content_area.x + 40;
    int content_y = content_area.y + 40;
    
    // Tab content
    if (active_tab == 0) {
        // Graphics tab
        surf = TTF_RenderText_Blended(font_small, "Graphics Settings", CLR_ACCENT);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {content_x, content_y, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        content_y += 60;
        
        // Fullscreen checkbox (placeholder)
        surf = TTF_RenderText_Blended(font_small, "[ ] Fullscreen", CLR_WHITE);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {content_x, content_y, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        content_y += 50;
        
        // VSync checkbox
        surf = TTF_RenderText_Blended(font_small, "[ ] VSync", CLR_WHITE);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {content_x, content_y, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        content_y += 50;
        
        // Resolution
        surf = TTF_RenderText_Blended(font_small, "Resolution: 1920x1080", CLR_WHITE);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {content_x, content_y, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
    } else if (active_tab == 1) {
        // Controls tab
        surf = TTF_RenderText_Blended(font_small, "Controls", CLR_ACCENT);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {content_x, content_y, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        content_y += 60;
        
        const char *controls[] = {
            "W / Up Arrow    - Move Up",
            "S / Down Arrow  - Move Down",
            "A / Left Arrow  - Move Left",
            "D / Right Arrow - Move Right",
            "SPACE           - Plant Bomb"
        };
        
        for (int i = 0; i < 5; i++) {
            surf = TTF_RenderText_Blended(font_small, controls[i], CLR_WHITE);
            if (surf) {
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
                SDL_Rect rect = {content_x, content_y, surf->w, surf->h};
                SDL_RenderCopy(renderer, tex, NULL, &rect);
                SDL_DestroyTexture(tex);
                SDL_FreeSurface(surf);
            }
            content_y += 45;
        }
        
    } else if (active_tab == 2) {
        // Account tab
        surf = TTF_RenderText_Blended(font_small, "Account Settings", CLR_ACCENT);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {content_x, content_y, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        content_y += 60;
        
        surf = TTF_RenderText_Blended(font_small, "Change Display Name:", CLR_GRAY);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {content_x, content_y, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        content_y += 40;
        
        // Display name input box (placeholder)
        SDL_Rect input_box = {content_x, content_y, 400, 50};
        draw_rounded_rect(renderer, input_box, CLR_BG, UI_CORNER_RADIUS);
        draw_rounded_border(renderer, input_box, CLR_PRIMARY, UI_CORNER_RADIUS, 1);
        
        content_y += 80;
        
        surf = TTF_RenderText_Blended(font_small, "Change Password:", CLR_GRAY);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {content_x, content_y, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        content_y += 40;
        
        // Password input box (placeholder)
        SDL_Rect pass_box = {content_x, content_y, 400, 50};
        draw_rounded_rect(renderer, pass_box, CLR_BG, UI_CORNER_RADIUS);
        draw_rounded_border(renderer, pass_box, CLR_PRIMARY, UI_CORNER_RADIUS, 1);
    }
    
    // Buttons at bottom
    apply_btn->rect = (SDL_Rect){760, 850, 200, 60};
    strcpy(apply_btn->text, "Apply");
    draw_button(renderer, font_small, apply_btn);
    
    back_btn->rect = (SDL_Rect){980, 850, 200, 60};
    strcpy(back_btn->text, "Back");
    draw_button(renderer, font_small, back_btn);
}

// Post-match results screen
void render_post_match_screen(SDL_Renderer *renderer, TTF_Font *font_large, TTF_Font *font_small,
                               int winner_id, int *elo_changes, int *kills, int duration_seconds,
                               Button *rematch_btn, Button *lobby_btn, GameState *game_state) {
    int win_w, win_h;
    SDL_GetRendererOutputSize(renderer, &win_w, &win_h);
    
    draw_background_grid(renderer, win_w, win_h);
    
    // Winner announcement with actual username
    SDL_Color winner_color = {255, 215, 0, 255};  // Gold
    char winner_text[128] = "VICTORY!";
    if (game_state && winner_id >= 0 && winner_id < game_state->num_players) {
        snprintf(winner_text, sizeof(winner_text), "%s Wins!", 
                 game_state->players[winner_id].username);
    }
    
    SDL_Surface *surf = TTF_RenderText_Blended(font_large, winner_text, winner_color);
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect r = {(win_w - surf->w) / 2, 50, surf->w, surf->h};
        SDL_RenderCopy(renderer, tex, NULL, &r);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }
    
    // Player cards - ONLY for actual players in the match
    int card_width = 450;
    int card_height = 80;
    int card_spacing = 20;
    int start_y = 130;
    int num_players = game_state ? game_state->num_players : 2;
    int last_card_y = start_y;
    
    for (int i = 0; i < num_players; i++) {
        int card_x = (win_w - card_width) / 2;
        int card_y = start_y + i * (card_height + card_spacing);
        last_card_y = card_y;
        
        SDL_Rect card = {card_x, card_y, card_width, card_height};
        
        // Winner card gets blue background, others get dark gray
        SDL_Color card_bg = (i == winner_id) ? 
            (SDL_Color){59, 130, 246, 255} : (SDL_Color){45, 55, 72, 255};
        
        draw_rounded_rect(renderer, card, card_bg, 8);
        
        // Winner badge
        if (i == winner_id) {
            SDL_Color badge_color = {255, 215, 0, 255};
            surf = TTF_RenderText_Blended(font_small, "WINNER", badge_color);
            if (surf) {
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
                SDL_Rect r = {card_x + 20, card_y + 10, surf->w, surf->h};
                SDL_RenderCopy(renderer, tex, NULL, &r);
                SDL_DestroyTexture(tex);
                SDL_FreeSurface(surf);
            }
        }
        
        // Player name (actual username!)
        SDL_Color name_color = {255, 255, 255, 255};
        const char *player_name = (game_state && i < num_players) ?
            game_state->players[i].username : "Unknown";
        surf = TTF_RenderText_Blended(font_small, player_name, name_color);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect r = {card_x + 20, card_y + (i == winner_id ? 40 : 20), surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &r);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        // ELO change
        char elo_text[64];
        int elo_change = (elo_changes && i < MAX_CLIENTS) ? elo_changes[i] : 0;
        SDL_Color elo_color = (elo_change >= 0) ? 
            (SDL_Color){34, 197, 94, 255} : (SDL_Color){239, 68, 68, 255};
        
        snprintf(elo_text, sizeof(elo_text), "%+d ELO", elo_change);
        surf = TTF_RenderText_Blended(font_small, elo_text, elo_color);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect r = {card_x + 250, card_y + 30, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &r);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
        
        // Kills
        char kills_text[64];
        int player_kills = (kills && i < MAX_CLIENTS) ? kills[i] : 0;
        snprintf(kills_text, sizeof(kills_text), "Kills: %d", player_kills);
        surf = TTF_RenderText_Blended(font_small, kills_text, (SDL_Color){200, 200, 200, 255});
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect r = {card_x + 360, card_y + 30, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &r);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
    }
    
    // Match duration - REAL TIME formatted as MM:SS
    int minutes = duration_seconds / 60;
    int seconds = duration_seconds % 60;
    char duration_text[64];
    snprintf(duration_text, sizeof(duration_text), "Match Duration: %d:%02d", minutes, seconds);
    surf = TTF_RenderText_Blended(font_small, duration_text, CLR_GRAY);
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect rect = {(win_w - surf->w) / 2, last_card_y + card_height + 40, surf->w, surf->h};
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }
    
    // Buttons
    rematch_btn->rect = (SDL_Rect){660, 850, 250, 60};
    strcpy(rematch_btn->text, "Rematch");
    draw_button(renderer, font_small, rematch_btn);
    
    lobby_btn->rect = (SDL_Rect){930, 850, 250, 60};
    strcpy(lobby_btn->text, "Return to Lobby");
    draw_button(renderer, font_small, lobby_btn);
}
