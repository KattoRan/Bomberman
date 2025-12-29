// Lobby list screen + lobby room screen 
#include "../graphics/graphics.h"
#include "ui.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define UI_CORNER_RADIUS 8

void render_lobby_list_screen(SDL_Renderer *renderer, TTF_Font *font,
                              LobbySummary *lobbies, int lobby_count,
                              Button *create_btn, Button *refresh_btn,
                              int selected_lobby) {
    int win_w, win_h;
    SDL_GetRendererOutputSize(renderer, &win_w, &win_h);

    draw_background_grid(renderer, win_w, win_h);
    
    // Tiêu đề với gradient effect
    if (font) {
        const char *title_text = "GAME LOBBIES";
        
        // Shadow
        SDL_Surface *title_shadow = TTF_RenderText_Blended(font, title_text, (SDL_Color){0, 0, 0, 120});
        if (title_shadow) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, title_shadow);
            SDL_Rect rect = {(win_w - title_shadow->w) / 2 + 3, 30, title_shadow->w, title_shadow->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(title_shadow);
        }
        
        SDL_Surface *title = TTF_RenderText_Blended(font, title_text, CLR_ACCENT);
        if (title) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, title);
            SDL_Rect rect = {(win_w - title->w) / 2, 27, title->w, title->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(title);
        }
    }
    
    // FIXED lobby cards for 1120x720
    int y = 120;  // Fixed from top
    int list_width = 740;  // Widen card to fit buttons
    int card_height = 80;  // Fixed height
    int start_x = (win_w - list_width) / 2;  // Centered

    for (int i = 0; i < lobby_count; i++) {
        SDL_Rect lobby_rect = {start_x, y, list_width, card_height};
        
        // Layered shadow (stronger for selected)
        int shadow_offset = (i == selected_lobby) ? 6 : 4;
        draw_layered_shadow(renderer, lobby_rect, UI_CORNER_RADIUS, shadow_offset);
        
        // Background
        draw_rounded_rect(renderer, lobby_rect, CLR_BG_DARK, UI_CORNER_RADIUS);

        // Border (accent for selected, gray for others)
        SDL_Color border = (i == selected_lobby) ? CLR_ACCENT_DK : CLR_GRAY;
        draw_rounded_border(renderer, lobby_rect, border, UI_CORNER_RADIUS, 2);
        
        // Glow effect for selected card
        if (i == selected_lobby) {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_Rect glow = {lobby_rect.x - 2, lobby_rect.y - 2, lobby_rect.w + 4, lobby_rect.h + 4};
            draw_rounded_border(renderer, glow, CLR_ACCENT, UI_CORNER_RADIUS + 2, 1);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        }
        
        if (font) {
            // Tên lobby với private badge
            char display_name[80];
            if (lobbies[i].is_private) {
                snprintf(display_name, sizeof(display_name), "[PRIVATE] %.40s (ID:%d)", lobbies[i].name, lobbies[i].id);
            } else {
                snprintf(display_name, sizeof(display_name), "%s (ID:%d)", lobbies[i].name, lobbies[i].id);
            }
            
            // Truncate to fit in card (leave space for player count)
            truncate_text_to_fit(display_name, sizeof(display_name), font, 800);
            
            SDL_Surface *name_surf = TTF_RenderText_Blended(font, display_name, CLR_WHITE);
            if (name_surf) {
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, name_surf);
                SDL_Rect rect = {start_x + 20, y + 15, name_surf->w, name_surf->h};
                SDL_RenderCopy(renderer, tex, NULL, &rect);
                SDL_DestroyTexture(tex);
                SDL_FreeSurface(name_surf);
            }
            
            // Player count badge (circular)
            char player_text[32];
            snprintf(player_text, sizeof(player_text), "Ply:%d | Spc:%d", 
                     lobbies[i].num_players, lobbies[i].spectator_count);
            // Move badge left to make room for buttons
            SDL_Rect player_badge = {start_x + 310, y + 22, 200, 35};
            
            SDL_Color badge_bg = lobbies[i].num_players >= 4 ? CLR_DANGER : 
                               lobbies[i].num_players >= 2 ? CLR_WARNING : CLR_SUCCESS;
            draw_rounded_rect(renderer, player_badge, badge_bg, 6);
            
            SDL_Surface *player_surf = TTF_RenderText_Blended(font, player_text, CLR_WHITE);
            if (player_surf) {
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, player_surf);
                SDL_Rect rect = {
                    player_badge.x + (player_badge.w - player_surf->w) / 2,
                    player_badge.y + (player_badge.h - player_surf->h) / 2,
                    player_surf->w, player_surf->h
                };
                SDL_RenderCopy(renderer, tex, NULL, &rect);
                SDL_DestroyTexture(tex);
                SDL_FreeSurface(player_surf);
            }
            
            // Status indicator with pulsing animation for waiting rooms
            const char *status_text = lobbies[i].status == LOBBY_WAITING ? "* Waiting" : "* Playing";
            SDL_Color status_color = lobbies[i].status == LOBBY_WAITING ? CLR_SUCCESS : CLR_WARNING;
            
            // Pulsing effect for waiting rooms
            if (lobbies[i].status == LOBBY_WAITING) {
                float pulse = (sinf(SDL_GetTicks() / 500.0f) + 1.0f) / 2.0f;
                status_color.a = 150 + (int)(pulse * 105);  // 150-255
            }
            
            SDL_Surface *info_surf = TTF_RenderText_Blended(font, status_text, status_color);
            if (info_surf) {
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, info_surf);
                SDL_Rect rect = {start_x + 20, y + 45, info_surf->w, info_surf->h};
                SDL_RenderCopy(renderer, tex, NULL, &rect);
                SDL_DestroyTexture(tex);
                SDL_FreeSurface(info_surf);
            }
            
            // --- ACTION BUTTONS ---
            int btn_y = y + 20;
            int btn_h = 40;

            // JOIN Button
            Button btn_join = {
                .rect = {start_x + list_width - 220, btn_y, 90, btn_h},
                .type = BTN_PRIMARY,
                .is_hovered = 0
            };
            strcpy(btn_join.text, "Join");
            
            if (lobbies[i].status == LOBBY_PLAYING) {
                draw_rounded_rect(renderer, btn_join.rect, CLR_GRAY, UI_CORNER_RADIUS);
                draw_rounded_border(renderer, btn_join.rect, CLR_GRAY_LIGHT, UI_CORNER_RADIUS, 1);
                draw_button_text(renderer, font, &btn_join, CLR_GRAY_LIGHT);
            } else {
                draw_button(renderer, font, &btn_join);
            }

            // SPECTATE Button
            Button btn_spectate = {
                .rect = {start_x + list_width - 120, btn_y, 110, btn_h},
                .type = BTN_OUTLINE,
                .is_hovered = 0
            };
            strcpy(btn_spectate.text, "Watch");
            
            draw_button(renderer, font, &btn_spectate);
        }
        y += card_height + 10;
    }
    
    draw_button(renderer, font, create_btn);
    draw_button(renderer, font, refresh_btn);
    //SDL_RenderPresent(renderer);
}

void render_lobby_room_screen(SDL_Renderer *renderer, TTF_Font *font,
                              Lobby *lobby, int my_player_id,
                              Button *ready_btn, Button *start_btn, 
                              Button *leave_btn) {
    int win_w, win_h;
    SDL_GetRendererOutputSize(renderer, &win_w, &win_h);

    draw_rounded_rect(renderer, (SDL_Rect){0, 0, win_w, win_h}, CLR_BG_DARK, UI_CORNER_RADIUS);
    
    // Title with room name and ID for private rooms
    if (font) {
        char title_text[128];
        if (lobby->is_private) {
            snprintf(title_text, sizeof(title_text), "ROOM: %s (ID: %s)", lobby->name, lobby->access_code);
        } else {
            snprintf(title_text, sizeof(title_text), "ROOM: %s", lobby->name);
        }
        
        // Shadow for title
        SDL_Surface *title_shadow = TTF_RenderText_Blended(font, title_text, (SDL_Color){0, 0, 0, 120});
        if (title_shadow) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, title_shadow);
            SDL_Rect rect = {(win_w - title_shadow->w) / 2 + 3, 30, title_shadow->w, title_shadow->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(title_shadow);
        }
        
        SDL_Surface *title = TTF_RenderText_Blended(font, title_text, CLR_ACCENT);
        if (title) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, title);
            SDL_Rect rect = {(win_w - title->w) / 2, 27, title->w, title->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(title);
        }
        
    }
    
    // Player list with LARGER cards
    int y = 120;  // Start a bit lower after title
    int card_width = 440;  
    int start_x = 80;

    for (int i = 0; i < lobby->num_players; i++) {
        Player *p = &lobby->players[i];
        
        int card_height = 75; 
        SDL_Rect card_rect = {start_x, y, card_width, card_height};
        
        // Enhanced shadow
        draw_layered_shadow(renderer, card_rect, UI_CORNER_RADIUS, 5);
        
        // Card background with gradient
        SDL_Color card_bg_top, card_bg_bottom;
        if (i == lobby->host_id) {
            card_bg_top = (SDL_Color){109, 40, 217, 255};
            card_bg_bottom = (SDL_Color){90, 30, 180, 255};
        } else {
            card_bg_top = (SDL_Color){51, 65, 85, 255};
            card_bg_bottom = (SDL_Color){40, 50, 70, 255};
        }
        draw_vertical_gradient(renderer, card_rect, card_bg_top, card_bg_bottom);
        
        // Border
        SDL_SetRenderDrawColor(renderer, 71, 85, 105, 255);
        draw_rounded_border(renderer, card_rect, (SDL_Color){71, 85, 105, 255}, UI_CORNER_RADIUS, 1);
        
        // Accent bar on left (wider)
        SDL_Rect accent_bar = {start_x, y, 8, card_height};  // 8px wide (was 6)
        SDL_Color accent_color;
        if (i == lobby->host_id) {
            accent_color = (SDL_Color){167, 139, 250, 255}; 
        } else if (p->is_ready) {
            accent_color = CLR_SUCCESS;
        } else {
            accent_color = CLR_WARNING;
        }
        draw_rounded_rect(renderer, accent_bar, accent_color, 4);

        if (font) {
            // Number circle - vertically centered in card
            SDL_Rect number_circle = {start_x + 20, y + (card_height - 40) / 2, 40, 40};
            draw_rounded_rect(renderer, number_circle, (SDL_Color){30, 41, 59, 200}, 20);
            draw_rounded_border(renderer, number_circle, accent_color, 20, 2);
            
            char num_str[16];
            snprintf(num_str, sizeof(num_str), "%d", i + 1);
            SDL_Surface *num_surf = TTF_RenderText_Blended(font, num_str, CLR_WHITE);
            if (num_surf) {
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, num_surf);
                SDL_Rect num_rect = {
                    number_circle.x + (40 - num_surf->w) / 2,
                    number_circle.y + (40 - num_surf->h) / 2,
                    num_surf->w, num_surf->h
                };
                SDL_RenderCopy(renderer, tex, NULL, &num_rect);
                SDL_DestroyTexture(tex);
                SDL_FreeSurface(num_surf);
            }
            
            // Username - aligned with left content area
            SDL_Color username_color = (i == lobby->host_id) ? 
                (SDL_Color){233, 213, 255, 255} : CLR_WHITE;
            
            // Truncate username to fit in card
            char username_display[MAX_USERNAME + 1];
            strncpy(username_display, p->username, MAX_USERNAME);
            username_display[MAX_USERNAME] = '\0';
            truncate_text_to_fit(username_display, sizeof(username_display), font, 650);
            
            SDL_Surface *name_surf = TTF_RenderText_Blended(font, username_display, username_color);
            if (name_surf) {
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, name_surf);
                SDL_Rect name_rect = {start_x + 75, y + 20, name_surf->w, name_surf->h};
                SDL_RenderCopy(renderer, tex, NULL, &name_rect);
                SDL_DestroyTexture(tex);
                SDL_FreeSurface(name_surf);
            }
            
            // Role badge - aligned below username
            if (i == lobby->host_id) {
                SDL_Rect host_badge = {start_x + 320, y + 26, 80, 28};
                draw_rounded_rect(renderer, host_badge, (SDL_Color){167, 139, 250, 255}, 4);
                
                SDL_Surface *host_surf = TTF_RenderText_Blended(font, "HOST", 
                    (SDL_Color){30, 27, 75, 255});
                if (host_surf) {
                    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, host_surf);
                    SDL_Rect rect = {
                        host_badge.x + (host_badge.w - host_surf->w) / 2,
                        host_badge.y + (host_badge.h - host_surf->h) / 2,
                        host_surf->w, host_surf->h
                    };
                    SDL_RenderCopy(renderer, tex, NULL, &rect);
                    SDL_DestroyTexture(tex);
                    SDL_FreeSurface(host_surf);
                }
            }
            
            // Status badge on right - LARGER
            if (i != lobby->host_id) {
                SDL_Rect status_badge = {start_x + 295, y + 26, 130, 28};
                SDL_Color badge_color = p->is_ready ? CLR_SUCCESS : 
                    (SDL_Color){71, 85, 105, 255};
                draw_rounded_rect(renderer, status_badge, badge_color, 6);
                draw_rounded_border(renderer, status_badge, 
                    (SDL_Color){badge_color.r - 20, badge_color.g - 20, badge_color.b - 20, 255}, 6, 1);
                
                const char *status_text = p->is_ready ? "READY" : "WAITING";
                SDL_Surface *status_surf = TTF_RenderText_Blended(font, status_text, CLR_WHITE);
                if (status_surf) {
                    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, status_surf);
                    SDL_Rect rect = {
                        status_badge.x + (status_badge.w - status_surf->w) / 2,
                        status_badge.y + (status_badge.h - status_surf->h) / 2,
                        status_surf->w, status_surf->h
                    };
                    SDL_RenderCopy(renderer, tex, NULL, &rect);
                    SDL_DestroyTexture(tex);
                    SDL_FreeSurface(status_surf);
                }
                
                // Kick button for host
                if (my_player_id == lobby->host_id) {

                    Button btn_kick = {
                        .rect = {start_x + 180, y + 26, 90, 28},
                        .is_hovered = 0,
                        .type = BTN_DANGER
                    };

                    strncpy(btn_kick.text, "Kick", sizeof(btn_kick.text) - 1);
                    btn_kick.text[sizeof(btn_kick.text) - 1] = '\0';
                    draw_button(renderer, font, &btn_kick);
                }
            }
        }
        y += card_height + 20;  // 20px spacing (was 15)
    }
    
    int button_y = win_h - 90;
    // Cập nhật vị trí nút Leave (luôn hiện)
    leave_btn->rect.x = 320;
    leave_btn->rect.y = button_y;
    leave_btn->rect.w = 200;
    leave_btn->rect.h = 50;
    
    // Vẽ nút tùy theo vai trò
    if (my_player_id == lobby->host_id) {
        // Host chỉ thấy Start Game và Leave
        if (start_btn) {
            start_btn->rect.x = 80;
            start_btn->rect.y = button_y;
            start_btn->rect.w = 200;
            start_btn->rect.h = 50;
            draw_button(renderer, font, start_btn);
        }
    } else {
        // Người chơi thường chỉ thấy Ready và Leave
        if (ready_btn) {
            ready_btn->rect.x = 80;
            ready_btn->rect.y = button_y;
            ready_btn->rect.w = 200;
            ready_btn->rect.h = 50;
            draw_button(renderer, font, ready_btn);
        }
    }
    
    draw_button(renderer, font, leave_btn);
    
    // Hiển thị thông báo lỗi nếu có
    extern char lobby_error_message[256];
    if (font && strlen(lobby_error_message) > 0) {
        // Background cho message
        SDL_Surface *msg_surf = TTF_RenderText_Blended(font, lobby_error_message, CLR_DANGER);
        if (msg_surf) {
            int msg_width = msg_surf->w + 40;
            int msg_height = msg_surf->h + 20;
            int msg_x = (win_w - msg_width) / 2;
            int msg_y = 15;
            
            // Shadow
            SDL_Rect shadow = {msg_x + 2, msg_y + 2, msg_width, msg_height};
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 100);
            SDL_RenderFillRect(renderer, &shadow);
            
            // Background
            SDL_Rect bg = {msg_x, msg_y, msg_width, msg_height};
            SDL_SetRenderDrawColor(renderer, 185, 28, 28, 230);
            SDL_RenderFillRect(renderer, &bg);
            
            // Border
            SDL_SetRenderDrawColor(renderer, CLR_DANGER.r, CLR_DANGER.g, CLR_DANGER.b, 255);
            SDL_RenderDrawRect(renderer, &bg);
            
            // Text
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, msg_surf);
            SDL_Rect text_rect = {
                msg_x + 20,
                msg_y + 10,
                msg_surf->w,
                msg_surf->h
            };
            SDL_RenderCopy(renderer, tex, NULL, &text_rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(msg_surf);
        }
    }
    
    //SDL_RenderPresent(renderer);
}