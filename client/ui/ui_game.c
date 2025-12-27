#include "../graphics/graphics.h"
#include "ui.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

void render_post_match_screen(SDL_Renderer *renderer, TTF_Font *font_large, TTF_Font *font_small,
                               int winner_id, int *elo_changes, int *kills, int duration_seconds,
                               Button *rematch_btn, Button *lobby_btn, GameState *game_state, int my_player_id) {
    int win_w, win_h;
    SDL_GetRendererOutputSize(renderer, &win_w, &win_h);
    
    draw_background_grid(renderer, win_w, win_h);
    
    // Winner announcement with actual username
    SDL_Color winner_color = {255, 215, 0, 255};  // Gold
    char winner_text[128] = "VICTORY!";
    
    // Spectator friendly title
    if (my_player_id == -1) {
        if (game_state && winner_id >= 0 && winner_id < game_state->num_players) {
            snprintf(winner_text, sizeof(winner_text), "%s Wins!", 
                     game_state->players[winner_id].username);
        } else {
            strncpy(winner_text, "Draw!", 64);
        }
    } else {
        if (game_state && winner_id >= 0 && winner_id < game_state->num_players) {
            if (winner_id == my_player_id) {
                strncpy(winner_text, "VICTORY!", 64);
            } else {
                 snprintf(winner_text, sizeof(winner_text), "%s Wins!", 
                     game_state->players[winner_id].username);
                winner_color = (SDL_Color){255, 100, 100, 255}; // Red for loss
            }
        }
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
    
    // Draw buttons
    // If spectator, only show "Back to Room" centered
    if (my_player_id == -1) {
        lobby_btn->rect = (SDL_Rect){(win_w - 250)/2, 600, 250, 60};
        strcpy(lobby_btn->text, "Leave Lobby");
        draw_button(renderer, font_small, lobby_btn);
    } else {
        // Player view - show Rematch and Back to Room
        rematch_btn->rect = (SDL_Rect){(win_w/2) - 270, 600, 250, 60};
        strcpy(rematch_btn->text, "Rematch");
        draw_button(renderer, font_small, rematch_btn);
        
        lobby_btn->rect = (SDL_Rect){(win_w/2) + 20, 600, 250, 60};
        strcpy(lobby_btn->text, "Return to Lobby");
        draw_button(renderer, font_small, lobby_btn);
    }
}