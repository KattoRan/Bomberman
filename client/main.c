/* client/main.c - Clean Application Entry Point */
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include "../common/protocol.h"
#include "ui/ui.h"
#include "graphics/graphics.h"
#include "state/client_state.h"
#include "network/network.h"
#include "handlers/session.h"
#include "handlers/game.h"
#include "handlers/event_handler.h"

// Global flag for main loop
int running = 1;

/* ===== INITIALIZATION FUNCTIONS ===== */

SDL_Window* init_sdl_window(SDL_Renderer** rend) {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    SDL_Window *win = SDL_CreateWindow("Bomberman", 
                                       SDL_WINDOWPOS_CENTERED, 
                                       SDL_WINDOWPOS_CENTERED, 
                                       1120, 720, 
                                       SDL_WINDOW_SHOWN);
    *rend = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    
    if (!win) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return NULL;
    }
    if (!*rend) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return NULL;
    }
    return win;
}

TTF_Font* load_fonts(TTF_Font** font_large, TTF_Font** font_medium) {
    *font_large = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 64);
    if (!*font_large) {
        *font_large = TTF_OpenFont("C:\\Windows\\Fonts\\arialbd.ttf", 64);
    }

    *font_medium = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 40);
    if (!*font_medium) {
        *font_medium = TTF_OpenFont("C:\\Windows\\Fonts\\arialbd.ttf", 40);
    }

    TTF_Font *font_small = init_font();
    if (!font_small) {
        printf("Failed to load font!\n");
        return NULL;
    }
    return font_small;
}

void setup_network_and_login(const char *server_ip) {
    if (connect_to_server(server_ip, PORT) != 0) {
        fprintf(stderr, "Failed to connect to server\n");
        running = 0;
        return;
    }
    
    char saved_token[64];
    if (load_session_token(saved_token)) {
        ClientPacket pkt;
        memset(&pkt, 0, sizeof(pkt));
        pkt.type = MSG_LOGIN_WITH_TOKEN;
        strncpy(pkt.session_token, saved_token, 63);
        send(sock, &pkt, sizeof(pkt), 0);
    }
}

void process_network_packets() {
    ServerPacket spkt;
    int recv_result;
    
    int packets_received = 0;
    while ((recv_result = receive_server_packet(&spkt)) == 1 && packets_received < 10) {
        process_server_packet(&spkt);
        packets_received++;
    }
    
    if (recv_result < 0) {
        printf("Server disconnected!\n");
        running = 0;
    }
}

void update_error_messages() {
    if (lobby_error_message[0] != '\0' && SDL_GetTicks() - error_message_time > 3000) {
        lobby_error_message[0] = '\0';
    }
}

void render_current_screen(SDL_Renderer *rend, TTF_Font *font_large, TTF_Font *font_medium, 
                           TTF_Font *font_small, int mx, int my, int tick) {
    switch (current_screen) {
        case SCREEN_LOGIN: {
            btn_login.is_hovered = is_mouse_inside(btn_login.rect, mx, my);
            btn_reg.is_hovered = is_mouse_inside(btn_reg.rect, mx, my);

            // Login screen: only show username and password
            render_login_screen(
                rend, font_large, font_small,
                &inp_user, NULL, &inp_pass,
                &btn_login, &btn_reg, status_message
            );
            break;
        }
        
        case SCREEN_REGISTER: {
            btn_login.is_hovered = is_mouse_inside(btn_login.rect, mx, my);
            btn_reg.is_hovered = is_mouse_inside(btn_reg.rect, mx, my);

            // Register screen: show all three fields
            render_login_screen(
                rend, font_large, font_small,
                &inp_user, &inp_email, &inp_pass,
                &btn_login, &btn_reg, status_message
            );
            break;
        }

        case SCREEN_LOBBY_LIST: {
            // Update hover states ONCE before rendering
            btn_create.is_hovered = is_mouse_inside(btn_create.rect, mx, my);
            btn_refresh.is_hovered = is_mouse_inside(btn_refresh.rect, mx, my);
            btn_friends.is_hovered = is_mouse_inside(btn_friends.rect, mx, my);
            // btn_quick_play removed
            // btn_settings removed
            btn_profile.is_hovered = is_mouse_inside(btn_profile.rect, mx, my);
            btn_leaderboard.is_hovered = is_mouse_inside(btn_leaderboard.rect, mx, my);
            btn_logout.is_hovered = is_mouse_inside(btn_logout.rect, mx, my);
            btn_create_confirm.is_hovered = is_mouse_inside(btn_create_confirm.rect, mx, my);
            btn_cancel.is_hovered = is_mouse_inside(btn_cancel.rect, mx, my);

            render_lobby_list_screen(
                rend, font_small,
                lobby_list, lobby_count,
                &btn_create, &btn_refresh,
                selected_lobby_idx
            );
            
            // Draw additional nav buttons - MODERNIZED with rounded corners
            // Logout button (top left)
            draw_button(rend, font_small, &btn_logout);

            // Friends button
            draw_button(rend, font_small, &btn_friends);
            
            // Quick Play removed
            
            // Settings button removed
            
            // Profile & Leaderboard buttons (top right)
            draw_button(rend, font_small, &btn_profile);
            draw_button(rend, font_small, &btn_leaderboard);
            
            //SDL_RenderPresent(rend);
            
            // Render dialog overlay if showing
            if (show_create_room_dialog) {
                render_create_room_dialog(rend, font_small, &inp_room_name, &inp_access_code,
                                            &btn_create_confirm, &btn_cancel);
                // Draw input fields
                draw_input_field(rend, font_small, &inp_room_name);
                draw_input_field(rend, font_small, &inp_access_code);
                // Draw buttons
                draw_button(rend, font_small, &btn_create_confirm);
                draw_button(rend, font_small, &btn_cancel);
                //SDL_RenderPresent(rend);
            }
            
            // Render join code dialog if showing
            if (show_join_code_dialog) {
                render_create_room_dialog(rend, font_small, &inp_join_code, NULL,
                                            &btn_create_confirm, &btn_cancel);
                // Change button text
                strcpy(btn_create_confirm.text, "Join");
                draw_input_field(rend, font_small, &inp_join_code);
                draw_button(rend, font_small, &btn_create_confirm);
                draw_button(rend, font_small, &btn_cancel);
                //SDL_RenderPresent(rend);
            }
            break;
        }

        case SCREEN_LOBBY_ROOM: {
            if (my_player_id == current_lobby.host_id) {
                btn_start.is_hovered = is_mouse_inside(btn_start.rect, mx, my);
                btn_leave.is_hovered = is_mouse_inside(btn_leave.rect, mx, my);
                btn_ready.is_hovered = 0;
            } else {
                btn_ready.is_hovered = is_mouse_inside(btn_ready.rect, mx, my);
                btn_leave.is_hovered = is_mouse_inside(btn_leave.rect, mx, my);
                btn_start.is_hovered = 0;
            }

            Button *ready_ptr = (my_player_id != current_lobby.host_id) ? &btn_ready : NULL;
            Button *start_ptr = (my_player_id == current_lobby.host_id) ? &btn_start : NULL;

            render_lobby_room_screen(
                rend, font_small,
                &current_lobby, my_player_id,
                ready_ptr, start_ptr, &btn_leave
            );
            
            // Render chat panel
            render_chat_panel_room(rend, font_small, chat_history, &inp_chat_message, chat_count);
            
            // Invite button
            if (current_lobby.num_players < 4) {
                btn_open_invite.is_hovered = is_mouse_inside(btn_open_invite.rect, mx, my);
                draw_button(rend, font_small, &btn_open_invite);
            }
            
            if (show_invite_overlay) {
                render_invite_overlay(rend, font_small, friends_list, friends_count, 
                                        invited_user_ids, invited_count, &btn_close_invite);
            }
            break;
        }

        case SCREEN_GAME: {
            // Calculate elapsed game time
            int elapsed_seconds = 0;
            if (game_start_time > 0) {
                Uint32 elapsed_ms = SDL_GetTicks() - game_start_time;
                elapsed_seconds = elapsed_ms / 1000;
            }
            render_game(rend, font_small, tick++, my_player_id, elapsed_seconds);
            break;
        }
        
        case SCREEN_FRIENDS: {
            Button back_btn;
            back_btn.is_hovered = is_mouse_inside(back_btn.rect, mx, my);
            btn_send_friend_request.is_hovered = is_mouse_inside(btn_send_friend_request.rect, mx, my);
            
            render_friends_screen(rend, font_small, friends_list, friends_count,
                                    pending_requests, pending_count,
                                    sent_requests, sent_count, &back_btn);
            
            // Draw friend request input and button
            draw_input_field(rend, font_small, &inp_friend_request);
            draw_button(rend, font_small, &btn_send_friend_request);
            
            // Draw delete confirmation dialog if active - MODERNIZED
            if (show_delete_confirm && delete_friend_index >= 0 && delete_friend_index < friends_count) {
                int win_w, win_h;
                SDL_GetRendererOutputSize(rend, &win_w, &win_h);
                
                // Semi-transparent overlay
                SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(rend, 0, 0, 0, 200);
                SDL_Rect overlay = {0, 0, win_w, win_h};
                SDL_RenderFillRect(rend, &overlay);
                
                // Modern dialog box with rounded corners
                SDL_Rect dialog = {250, 250, 300, 150};
                
                // Layered shadow
                draw_layered_shadow(rend, dialog, 12, 6);
                
                // Dialog background
                SDL_Color dialog_bg = {30, 41, 59, 255};
                draw_rounded_rect(rend, dialog, dialog_bg, 12);
                
                // Dialog border
                SDL_Color border_col = {59, 130, 246, 255};
                draw_rounded_border(rend, dialog, border_col, 12, 2);
                
                // Title
                SDL_Surface *surf = TTF_RenderText_Blended(font_small, "Delete Friend?", (SDL_Color){255, 255, 255, 255});
                if (surf) {
                    SDL_Texture *tex = SDL_CreateTextureFromSurface(rend, surf);
                    SDL_Rect r = {dialog.x + (dialog.w - surf->w)/2, dialog.y + 20, surf->w, surf->h};
                    SDL_RenderCopy(rend, tex, NULL, &r);
                    SDL_DestroyTexture(tex);
                    SDL_FreeSurface(surf);
                }
                
                // Friend name
                char msg[128];
                snprintf(msg, sizeof(msg), "Remove %s?", friends_list[delete_friend_index].display_name);
                surf = TTF_RenderText_Blended(font_small, msg, (SDL_Color){200, 200, 200, 255});
                if (surf) {
                    SDL_Texture *tex = SDL_CreateTextureFromSurface(rend, surf);
                    SDL_Rect r = {dialog.x + (dialog.w - surf->w)/2, dialog.y + 60, surf->w, surf->h};
                    SDL_RenderCopy(rend, tex, NULL, &r);
                    SDL_DestroyTexture(tex);
                    SDL_FreeSurface(surf);
                }
                
                // Modern Yes button (rounded, red with shadow)
                SDL_Rect yes_btn = {300, 350, 100, 40};
                draw_layered_shadow(rend, yes_btn, 6, 3);
                SDL_Color yes_bg = {239, 68, 68, 255};
                draw_rounded_rect(rend, yes_btn, yes_bg, 6);
                
                surf = TTF_RenderText_Blended(font_small, "Yes", (SDL_Color){255, 255, 255, 255});
                if (surf) {
                    SDL_Texture *tex = SDL_CreateTextureFromSurface(rend, surf);
                    SDL_Rect r = {yes_btn.x + (yes_btn.w - surf->w)/2, yes_btn.y + (yes_btn.h - surf->h)/2, surf->w, surf->h};
                    SDL_RenderCopy(rend, tex, NULL, &r);
                    SDL_DestroyTexture(tex);
                    SDL_FreeSurface(surf);
                }
                
                // Modern No button (rounded, gray with shadow)
                SDL_Rect no_btn = {420, 350, 100, 40};
                draw_layered_shadow(rend, no_btn, 6, 3);
                SDL_Color no_bg = {100, 116, 139, 255};
                draw_rounded_rect(rend, no_btn, no_bg, 6);
                
                surf = TTF_RenderText_Blended(font_small, "No", (SDL_Color){255, 255, 255, 255});
                if (surf) {
                    SDL_Texture *tex = SDL_CreateTextureFromSurface(rend, surf);
                    SDL_Rect r = {no_btn.x + (no_btn.w - surf->w)/2, no_btn.y + (no_btn.h - surf->h)/2, surf->w, surf->h};
                    SDL_RenderCopy(rend, tex, NULL, &r);
                    SDL_DestroyTexture(tex);
                    SDL_FreeSurface(surf);
                }
                
                SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_NONE);
            }
            break;
        }
        
        case SCREEN_PROFILE: {
            Button back_btn;
            back_btn.is_hovered = is_mouse_inside(back_btn.rect, mx, my);
            // Determine title based on whether it's our profile or someone else's
            char p_title[128] = "";
            if (strcmp(my_profile.username, my_username) != 0 && strlen(my_profile.username) > 0) {
                    snprintf(p_title, sizeof(p_title), "PROFILE: %s", my_profile.display_name);
            }
            render_profile_screen(rend, font_medium, font_small, &my_profile, &back_btn, p_title);
            break;
        }
        
        case SCREEN_LEADERBOARD: {
            Button back_btn;
            back_btn.is_hovered = is_mouse_inside(back_btn.rect, mx, my);
            render_leaderboard_screen(rend, font_large, font_small, leaderboard, leaderboard_count, &back_btn);
            break;
        }
        
        // SCREEN_SETTINGS case removed
        
        case SCREEN_POST_MATCH: {
            // Update hover states
            btn_rematch.is_hovered = is_mouse_inside((SDL_Rect){660, 850, 250, 60}, mx, my);
            btn_return_lobby.is_hovered = is_mouse_inside((SDL_Rect){930, 850, 250, 60}, mx, my);
            
            render_post_match_screen(rend, font_large, font_small,
                                    post_match_winner_id, post_match_elo_changes,
                                    post_match_kills, post_match_duration,
                                    &btn_rematch, &btn_return_lobby,
                                    &current_state, my_player_id);
            break;
        }

        default:
            break;
    }
}

void render_ui_overlays(SDL_Renderer *rend, TTF_Font *font_small, int mx, int my) {
    // Render notifications
    if (current_screen != SCREEN_LOGIN &&
        current_screen != SCREEN_REGISTER &&
        notification_message[0] != '\0' &&
        SDL_GetTicks() - notification_time < NOTIFICATION_DURATION) {
        int win_w, win_h;
        SDL_GetRendererOutputSize(rend, &win_w, &win_h);
        
        SDL_Surface *notif = TTF_RenderText_Blended(font_small, notification_message, (SDL_Color){255, 255, 255, 255});
        if (notif) {
            int box_w = notif->w + 40;
            int box_h = notif->h + 20;
            int box_x = (win_w - box_w) / 2;
            int box_y = 20;
            
            SDL_Rect bg = {box_x, box_y, box_w, box_h};
            
            draw_layered_shadow(rend, bg, 8, 4);
            
            SDL_Color notif_bg = {59, 130, 246, 230};
            SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);
            draw_rounded_rect(rend, bg, notif_bg, 8);
            
            SDL_Color notif_border = {96, 165, 250, 255};
            draw_rounded_border(rend, bg, notif_border, 8, 2);
            SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_NONE);
            
            SDL_Texture *tex = SDL_CreateTextureFromSurface(rend, notif);
            SDL_Rect text_rect = {box_x + 20, box_y + 10, notif->w, notif->h};
            SDL_RenderCopy(rend, tex, NULL, &text_rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(notif);
        }
    } else if (SDL_GetTicks() - notification_time >= NOTIFICATION_DURATION) {
        notification_message[0] = '\0';
    }

    // Global Invite Popup
    if (current_invite.is_active) {
        btn_invite_accept.is_hovered = is_mouse_inside(btn_invite_accept.rect, mx, my);
        btn_invite_decline.is_hovered = is_mouse_inside(btn_invite_decline.rect, mx, my);
        render_invitation_popup(rend, font_small, &current_invite, &btn_invite_accept, &btn_invite_decline);
    }
}

void cleanup_resources(SDL_Window *win, SDL_Renderer *rend, TTF_Font *font_large, 
                       TTF_Font *font_medium, TTF_Font *font_small) {
    close(sock);
    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(win);
    if (font_large) TTF_CloseFont(font_large);
    if (font_medium) TTF_CloseFont(font_medium);
    if (font_small) TTF_CloseFont(font_small);
    TTF_Quit();
    SDL_Quit();
}

/* ===== MAIN ENTRY POINT ===== */

int main(int argc, char *argv[]) {
    // Setup
    determine_session_path(argc, argv);
    const char *server_ip = (argc > 1) ? argv[1] : "127.0.0.1";
    
    setup_network_and_login(server_ip);
    if (!running) return -1;
    
    SDL_Renderer *rend = NULL;
    SDL_Window *win = init_sdl_window(&rend);
    if (!win) return -1;
    
    TTF_Font *font_large = NULL;
    TTF_Font *font_medium = NULL;
    TTF_Font *font_small = load_fonts(&font_large, &font_medium);
    if (!font_small) return -1;
    
    SDL_StartTextInput();
    int tick = 0;

    // Main loop
    while (running) {
        SDL_Event e;
        int mx, my;
        SDL_GetMouseState(&mx, &my);

        // 1. Handle events
        while (SDL_PollEvent(&e)) {
            handle_events(&e, mx, my, rend);
        }
        
        // 2. Process network
        process_network_packets();
        
        // 3. Update state
        update_error_messages();
        
        // 4. Render
        render_current_screen(rend, font_large, font_medium, font_small, mx, my, tick);
        render_ui_overlays(rend, font_small, mx, my);
        SDL_RenderPresent(rend);
        
        tick++;
    }

    // Cleanup
    cleanup_resources(win, rend, font_large, font_medium, font_small);
    return 0;
}
