/* client/handlers/event_handler.c */
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string.h>
#include "../common/protocol.h"
#include "../ui/ui.h"
#include "../state/client_state.h"
#include "../network/network.h"
#include "../graphics/graphics.h"

int all_players_ready(Lobby *lobby) {
    for (int i = 0; i < lobby->num_players; i++) {
        if (!lobby->players[i].is_ready) return 0;
    }
    return 1;
}
void handle_events(SDL_Event *e, int mx, int my, SDL_Renderer *rend) {
    if (e->type == SDL_QUIT) {
        extern int running;
        running = 0;
        return;
    }

    if (current_invite.is_active && e->type == SDL_MOUSEBUTTONDOWN) {
        // Global Invite Handler - Priority 1
        // Calculate expected button positions dynamically
        int win_w, win_h;
        SDL_GetRendererOutputSize(rend, &win_w, &win_h);
        int box_w = 400;
        int box_h = 250;
        int box_x = (win_w - box_w) / 2;
        int box_y = (win_h - box_h) / 2;

        btn_invite_accept.rect.x = box_x + 40;
        btn_invite_accept.rect.y = box_y + 180;
        btn_invite_decline.rect.x = box_x + 210;
        btn_invite_decline.rect.y = box_y + 180;

        printf("[DEBUG] Click at %d,%d. Accept Rect: %d,%d %dx%d. Decline Rect: %d,%d %dx%d\n",
                mx, my, 
                btn_invite_accept.rect.x, btn_invite_accept.rect.y, btn_invite_accept.rect.w, btn_invite_accept.rect.h,
                btn_invite_decline.rect.x, btn_invite_decline.rect.y, btn_invite_decline.rect.w, btn_invite_decline.rect.h);

        if (is_mouse_inside(btn_invite_accept.rect, mx, my)) {
            printf("[DEBUG] Accepted!\n");
            if (my_player_id != -1) {
                send_packet(MSG_LEAVE_LOBBY, 0);
            }
            
            ClientPacket pkt;
            memset(&pkt, 0, sizeof(pkt));
            pkt.type = MSG_JOIN_LOBBY;
            pkt.lobby_id = current_invite.lobby_id;
            strncpy(pkt.access_code, current_invite.access_code, 7);
            send(sock, &pkt, sizeof(pkt), 0);
            
            current_invite.is_active = 0;
            show_invite_overlay = 0;
        }
        else if (is_mouse_inside(btn_invite_decline.rect, mx, my)) {
            printf("[DEBUG] Declined!\n");
            current_invite.is_active = 0;
        }
        // Consume the event!
        return;
    }

    switch (current_screen) {
        case SCREEN_LOGIN:
            if (e->type == SDL_MOUSEBUTTONDOWN) {
                inp_user.is_active = is_mouse_inside(inp_user.rect, mx, my);
                inp_pass.is_active = is_mouse_inside(inp_pass.rect, mx, my);
                
                if (is_mouse_inside(btn_login.rect, mx, my)) {
                    if (strlen(inp_user.text) > 0 && strlen(inp_pass.text) > 0) {
                        ClientPacket pkt;
                        memset(&pkt, 0, sizeof(pkt));
                        pkt.type = MSG_LOGIN;
                        strcpy(pkt.username, inp_user.text);
                        strcpy(pkt.password, inp_pass.text);
                        send(sock, &pkt, sizeof(pkt), 0);
                    } else {
                        strncpy(status_message, "Please enter username and password", sizeof(status_message));
                    }
                }
                
                if (is_mouse_inside(btn_reg.rect, mx, my)) {
                    // Switch to registration screen
                    current_screen = SCREEN_REGISTER;
                    inp_user.text[0] = '\0';
                    inp_email.text[0] = '\0';
                    inp_pass.text[0] = '\0';
                    inp_user.is_active = 1;
                    inp_email.is_active = 0;
                    inp_pass.is_active = 0;
                    status_message[0] = '\0';
                }
            }
            
            if (e->type == SDL_TEXTINPUT) {
                if (inp_user.is_active) handle_text_input(&inp_user, e->text.text[0]);
                if (inp_pass.is_active) handle_text_input(&inp_pass, e->text.text[0]);
            }

            if (e->type == SDL_KEYDOWN) {
                if (e->key.keysym.sym == SDLK_BACKSPACE) {
                    if (inp_user.is_active) handle_text_input(&inp_user, '\b');
                    if (inp_pass.is_active) handle_text_input(&inp_pass, '\b');
                }
                if (e->key.keysym.sym == SDLK_RETURN || e->key.keysym.sym == SDLK_KP_ENTER) {
                    if (strlen(inp_user.text) > 0 && strlen(inp_pass.text) > 0) {
                        ClientPacket pkt;
                        memset(&pkt, 0, sizeof(pkt));
                        pkt.type = MSG_LOGIN;
                        strcpy(pkt.username, inp_user.text);
                        strcpy(pkt.password, inp_pass.text);
                        send(sock, &pkt, sizeof(pkt), 0);
                    }
                }
                if (e->key.keysym.sym == SDLK_TAB) {
                    if (inp_user.is_active) {
                        inp_user.is_active = 0;
                        inp_pass.is_active = 1;
                    } else if (inp_pass.is_active) {
                        inp_pass.is_active = 0;
                        inp_user.is_active = 1;
                    } else {
                        inp_user.is_active = 1;
                    }
                }
            }
            break;
            
        case SCREEN_REGISTER:
            if (e->type == SDL_MOUSEBUTTONDOWN) {
                inp_user.is_active = is_mouse_inside(inp_user.rect, mx, my);
                inp_email.is_active = is_mouse_inside(inp_email.rect, mx, my);
                inp_pass.is_active = is_mouse_inside(inp_pass.rect, mx, my);
                
                if (is_mouse_inside(btn_reg.rect, mx, my)) {
                    if (strlen(inp_user.text) > 0 && strlen(inp_email.text) > 0 && strlen(inp_pass.text) > 0) {
                        ClientPacket pkt;
                        memset(&pkt, 0, sizeof(pkt));
                        pkt.type = MSG_REGISTER;
                        strcpy(pkt.username, inp_user.text);
                        strcpy(pkt.email, inp_email.text);
                        strcpy(pkt.password, inp_pass.text);
                        send(sock, &pkt, sizeof(pkt), 0);
                    } else {
                        strncpy(status_message, "Please fill all fields", sizeof(status_message));
                    }
                }
                
                if (is_mouse_inside(btn_login.rect, mx, my)) {
                    // Back to login screen
                    current_screen = SCREEN_LOGIN;
                    inp_user.text[0] = '\0';
                    inp_email.text[0] = '\0';
                    inp_pass.text[0] = '\0';
                    inp_user.is_active = 1;
                    inp_email.is_active = 0;
                    inp_pass.is_active = 0;
                    status_message[0] = '\0';
                }
            }
            
            if (e->type == SDL_TEXTINPUT) {
                if (inp_user.is_active) handle_text_input(&inp_user, e->text.text[0]);
                if (inp_email.is_active) handle_text_input(&inp_email, e->text.text[0]);
                if (inp_pass.is_active) handle_text_input(&inp_pass, e->text.text[0]);
            }

            if (e->type == SDL_KEYDOWN) {
                if (e->key.keysym.sym == SDLK_BACKSPACE) {
                    if (inp_user.is_active) handle_text_input(&inp_user, '\b');
                    if (inp_email.is_active) handle_text_input(&inp_email, '\b');
                    if (inp_pass.is_active) handle_text_input(&inp_pass, '\b');
                }

                if (e->key.keysym.sym == SDLK_TAB) {
                    if (inp_user.is_active) {
                        inp_user.is_active = 0;
                        inp_email.is_active = 1;
                    } else if (inp_email.is_active) {
                        inp_email.is_active = 0;
                        inp_pass.is_active = 1;
                    } else if (inp_pass.is_active) {
                        inp_pass.is_active = 0;
                        inp_user.is_active = 1;
                    } else {
                        inp_user.is_active = 1;
                    }
                }
            }
            break;
            
        case SCREEN_LOBBY_LIST:
            // --- 1. DIALOG HANDLING (Priority) ---
            if (show_create_room_dialog) {
                if (e->type == SDL_MOUSEBUTTONDOWN) {
                    inp_room_name.is_active = is_mouse_inside(inp_room_name.rect, mx, my);
                    inp_access_code.is_active = is_mouse_inside(inp_access_code.rect, mx, my);
                    
                    // Game mode buttons
                    int win_w, win_h;
                    SDL_GetRendererOutputSize(rend, &win_w, &win_h);
                    int dialog_w = 700;
                    int dialog_h = 650;
                    int dialog_x = (win_w - dialog_w) / 2;
                    int dialog_y = (win_h - dialog_h) / 2;
                    int mode_y = dialog_y + 390;
                    int btn_width = 180;
                    int btn_spacing = 20;
                    
                    for (int i = 0; i < 3; i++) {
                        int btn_x = dialog_x + 75 + i * (btn_width + btn_spacing);
                        SDL_Rect mode_btn = {btn_x, mode_y, btn_width, 50};
                        if (is_mouse_inside(mode_btn, mx, my)) {
                            selected_game_mode = i;
                            printf("[CLIENT] Selected game mode: %d\n", selected_game_mode);
                            break;
                        }
                    }
                    
                    // Random button
                    SDL_Rect btn_random = {inp_access_code.rect.x + inp_access_code.rect.w + 10, 
                                            inp_access_code.rect.y, 80, inp_access_code.rect.h};
                    if (is_mouse_inside(btn_random, mx, my)) {
                        int random_code = 100000 + (rand() % 900000);
                        snprintf(inp_access_code.text, sizeof(inp_access_code.text), "%d", random_code);
                    }
                    
                    if (is_mouse_inside(btn_create_confirm.rect, mx, my)) {
                        int code_len = strlen(inp_access_code.text);
                        int is_valid = 1;
                        char error_msg[128] = "";
                        
                        if (code_len > 0) {
                            if (code_len != 6) {
                                is_valid = 0;
                                snprintf(error_msg, sizeof(error_msg), 
                                        "Access code must be exactly 6 digits! (Currently: %d)", code_len);
                            } else {
                                for (int i = 0; i < code_len; i++) {
                                    if (inp_access_code.text[i] < '0' || inp_access_code.text[i] > '9') {
                                        is_valid = 0;
                                        strcpy(error_msg, "Access code must contain only numbers!");
                                        break;
                                    }
                                }
                            }
                        }
                        
                        if (is_valid) {
                            ClientPacket pkt;
                            memset(&pkt, 0, sizeof(pkt));
                            pkt.type = MSG_CREATE_LOBBY;
                            strncpy(pkt.room_name, inp_room_name.text, MAX_ROOM_NAME - 1);
                            pkt.is_private = (code_len == 6) ? 1 : 0;
                            pkt.game_mode = selected_game_mode;
                            if (pkt.is_private) {
                                strncpy(pkt.access_code, inp_access_code.text, 7);
                            }
                            send(sock, &pkt, sizeof(pkt), 0);
                            show_create_room_dialog = 0;
                        } else {
                            snprintf(notification_message, sizeof(notification_message), "%s", error_msg);
                            notification_time = SDL_GetTicks();
                        }
                    }
                    if (is_mouse_inside(btn_cancel.rect, mx, my)) {
                        show_create_room_dialog = 0;
                    }
                }
                
                if (e->type == SDL_TEXTINPUT) {
                    if (inp_room_name.is_active) handle_text_input(&inp_room_name, e->text.text[0]);
                    if (inp_access_code.is_active && e->text.text[0] >= '0' && e->text.text[0] <= '9') {
                        handle_text_input(&inp_access_code, e->text.text[0]);
                    }
                }

                if (e->type == SDL_KEYDOWN) {
                    if (e->key.keysym.sym == SDLK_BACKSPACE) {
                        if (inp_room_name.is_active) handle_text_input(&inp_room_name, '\b');
                        if (inp_access_code.is_active) handle_text_input(&inp_access_code, '\b');
                    }
                    if (e->key.keysym.sym == SDLK_ESCAPE) show_create_room_dialog = 0;
                }
                // Consume all events when dialog is open
                break;
            }

            if (show_join_code_dialog) {
                if (e->type == SDL_MOUSEBUTTONDOWN) {
                    inp_join_code.is_active = is_mouse_inside(inp_join_code.rect, mx, my);
                    
                    if (is_mouse_inside(btn_create_confirm.rect, mx, my)) {
                        if (strlen(inp_join_code.text) == 6) {
                            ClientPacket pkt;
                            memset(&pkt, 0, sizeof(pkt));
                            pkt.type = MSG_JOIN_LOBBY;
                            pkt.lobby_id = selected_private_lobby_id;
                            strncpy(pkt.access_code, inp_join_code.text, 7);
                            send(sock, &pkt, sizeof(pkt), 0);
                            show_join_code_dialog = 0;
                        }
                    }
                    if (is_mouse_inside(btn_cancel.rect, mx, my)) {
                        show_join_code_dialog = 0;
                    }
                }

                if (e->type == SDL_TEXTINPUT && inp_join_code.is_active) {
                    if (e->text.text[0] >= '0' && e->text.text[0] <= '9') {
                        handle_text_input(&inp_join_code, e->text.text[0]);
                    }
                }

                if (e->type == SDL_KEYDOWN) {
                    if (e->key.keysym.sym == SDLK_BACKSPACE && inp_join_code.is_active) {
                        handle_text_input(&inp_join_code, '\b');
                    }
                    if (e->key.keysym.sym == SDLK_ESCAPE) show_join_code_dialog = 0;
                }
                // Consume all events when dialog is open
                break;
            }

            // --- 2. MAIN LOBBY LIST HANDLING (Only if no dialogs) ---
            if (e->type == SDL_MOUSEBUTTONDOWN) {
                if (is_mouse_inside(btn_create.rect, mx, my)) {
                    // Show create room dialog
                    show_create_room_dialog = 1;
                    snprintf(inp_room_name.text, sizeof(inp_room_name.text), "Room %d", lobby_count + 1);
                    inp_access_code.text[0] = '\0';
                    inp_room_name.is_active = 1;
                    inp_access_code.is_active = 0;
                }
                if (is_mouse_inside(btn_refresh.rect, mx, my)) {
                    send_packet(MSG_LIST_LOBBIES, 0);
                }
                if (is_mouse_inside(btn_friends.rect, mx, my)) {
                    send_packet(MSG_FRIEND_LIST, 0);
                    current_screen = SCREEN_FRIENDS;
                }
                if (is_mouse_inside(btn_profile.rect, mx, my)) {
                    send_packet(MSG_GET_PROFILE, 0);
                    current_screen = SCREEN_PROFILE;
                }
                if (is_mouse_inside(btn_leaderboard.rect, mx, my)) {
                    send_packet(MSG_GET_LEADERBOARD, 0);
                    current_screen = SCREEN_LEADERBOARD;
                }
                if (is_mouse_inside(btn_profile.rect, mx, my)) {
                    send_packet(MSG_GET_PROFILE, 0);
                    current_screen = SCREEN_PROFILE;
                }
                if (is_mouse_inside(btn_leaderboard.rect, mx, my)) {
                    send_packet(MSG_GET_LEADERBOARD, 0);
                    current_screen = SCREEN_LEADERBOARD;
                }
                // Quick Play removed
                // Settings removed
                if (is_mouse_inside(btn_logout.rect, mx, my)) {
                    clear_session_token();
                    current_screen = SCREEN_LOGIN;
                    status_message[0] = '\0';
                    inp_user.text[0] = '\0';
                    inp_pass.text[0] = '\0';
                }
                
                // Lobby click detection - Updated for new button layout
                int y = 120; // Fixed from top
                // FIXED lobby cards for 1120x720 (Must match ui_screens.c)
                int list_width = 740;  
                int card_height = 80;  
                int win_w;
                SDL_GetRendererOutputSize(rend, &win_w, NULL);
                int start_x = (win_w - list_width) / 2;
                
                for (int i = 0; i < lobby_count; i++) {
                    // Calculate button positions relative to card (must match ui_screens.c)
                    int btn_y = y + 20;
                    int btn_h = 40;
                    
                    // Join Button Rect
                    SDL_Rect join_rect = {start_x + list_width - 200, btn_y, 90, btn_h};
                    
                    // Spectate Button Rect
                    SDL_Rect spectate_rect = {start_x + list_width - 100, btn_y, 90, btn_h};

                    // JOIN CLICK
                    if (is_mouse_inside(join_rect, mx, my)) {
                        if (lobby_list[i].status == LOBBY_PLAYING) {
                            // Disabled - do nothing
                        } else {
                            // Join Logic
                            if (lobby_list[i].is_private) {
                                selected_private_lobby_id = lobby_list[i].id;
                                show_join_code_dialog = 1;
                                inp_join_code.text[0] = '\0';
                                inp_join_code.is_active = 1;
                            } else {
                                ClientPacket pkt;
                                memset(&pkt, 0, sizeof(pkt));
                                pkt.type = MSG_JOIN_LOBBY;
                                pkt.lobby_id = lobby_list[i].id;
                                pkt.access_code[0] = '\0';
                                send(sock, &pkt, sizeof(pkt), 0);
                            }
                        }
                        selected_lobby_idx = i; // Optionally select it
                        break; 
                    }
                    
                    // SPECTATE CLICK
                    if (is_mouse_inside(spectate_rect, mx, my)) {
                        ClientPacket pkt;
                        memset(&pkt, 0, sizeof(pkt));
                        pkt.type = MSG_SPECTATE;
                        pkt.lobby_id = lobby_list[i].id;
                        send(sock, &pkt, sizeof(pkt), 0);
                        
                        selected_lobby_idx = i;
                        break;
                    }
                    
                    // Keep card selection if clicking elsewhere on card?
                    // Optional: allows selecting without joining
                    SDL_Rect card_rect = {start_x, y, list_width, card_height};
                    if (is_mouse_inside(card_rect, mx, my)) {
                        selected_lobby_idx = i;
                    }
                    
                    y += card_height + 10; 
                }
            }
            break;
            
        case SCREEN_LOBBY_ROOM:
            if (e->type == SDL_MOUSEBUTTONDOWN) {
                // LAYER 2: Invite Friends Overlay
                if (show_invite_overlay) {
                    if (is_mouse_inside(btn_close_invite.rect, mx, my)) {
                        show_invite_overlay = 0;
                    }
                    
                    // Check clicks on friend list "Invite" buttons
                    // Calculated same way as render_invite_overlay
                    int win_w, win_h;
                    SDL_GetRendererOutputSize(rend, &win_w, &win_h);
                    int overlay_w = 500, overlay_h = 500;
                    int overlay_x = (win_w - overlay_w) / 2;
                    int overlay_y = 110;
                    int list_y = overlay_y + 80;
                    int btn_w = 100, btn_h = 40;
                    
                    // Iterate only online friends
                    int displayed_count = 0;
                    for (int i = 0; i < friends_count; i++) {
                        if (friends_list[i].is_online) {
                            if (displayed_count < 6) { // Page limit
                                SDL_Rect invite_btn = {
                                    overlay_x + overlay_w - btn_w - 40,
                                    list_y + (displayed_count * 60) + 10,
                                    btn_w, btn_h
                                };
                                
                                if (is_mouse_inside(invite_btn, mx, my)) {
                                    // Check if already invited
                                    int already_invited = 0;
                                    for(int k=0; k<invited_count; k++) {
                                        if(invited_user_ids[k] == friends_list[i].user_id) already_invited = 1;
                                    }
                                    
                                    if (!already_invited) {
                                        // Send Invite
                                        ClientPacket pkt;
                                        memset(&pkt, 0, sizeof(pkt));
                                        pkt.type = MSG_FRIEND_INVITE;
                                        strncpy(pkt.target_display_name, friends_list[i].display_name, MAX_DISPLAY_NAME-1);
                                        pkt.target_user_id = friends_list[i].user_id; // Use ID for lookup
                                        send(sock, &pkt, sizeof(pkt), 0);
                                        
                                        // Track invited user locally
                                        if(invited_count < 50) {
                                            invited_user_ids[invited_count++] = friends_list[i].user_id;
                                        }
                                    }
                                }
                                displayed_count++;
                            }
                        }
                    }
                    
                    // BLOCK underlying lobby inputs
                    break;
                }

                // LAYER 3: Normal Lobby Room Controls
                // Invite Button
                if (current_lobby.num_players < 4) {
                        if (is_mouse_inside(btn_open_invite.rect, mx, my)) {
                        show_invite_overlay = 1;
                        invited_count = 0; // Reset session tracking
                        send_packet(MSG_FRIEND_LIST, 0); // Refresh friends
                        break;
                        }
                }

                // Check Start/Ready buttons FIRST (highest priority)
                if (my_player_id == current_lobby.host_id) {
                    if (is_mouse_inside(btn_start.rect, mx, my)) {
                        if (current_lobby.num_players < 2) {
                            strncpy(lobby_error_message, "Need at least 2 players to start!", 
                                    sizeof(lobby_error_message));
                            error_message_time = SDL_GetTicks();
                        } else if (!all_players_ready(&current_lobby)) {
                            strncpy(lobby_error_message, "All players must be ready!", 
                                    sizeof(lobby_error_message));
                            error_message_time = SDL_GetTicks();
                        } else {
                            send_packet(MSG_START_GAME, 0);
                            lobby_error_message[0] = '\0';
                        }
                    }
                } else if (is_mouse_inside(btn_ready.rect, mx, my)) {
                    send_packet(MSG_READY, 0);
                }
                
                // Leave button
                if (is_mouse_inside(btn_leave.rect, mx, my)) {
                    send_packet(MSG_LEAVE_LOBBY, 0);
                    current_screen = SCREEN_LOBBY_LIST;
                    current_lobby.id = -1; // Reset lobby ID to prevent state pollution
                    send_packet(MSG_LIST_LOBBIES, 0);
                    lobby_error_message[0] = '\0';
                }
                // Leave button
                
                // Chat input field click (activate for typing)
                if (is_mouse_inside((SDL_Rect){610, 632, 340, 38}, mx, my)) {
                    inp_chat_message.is_active = 1;
                }
                
                // Chat send button click
                else if (is_mouse_inside((SDL_Rect){958, 632, 75, 38}, mx, my)) {
                    if (strlen(inp_chat_message.text) > 0) {
                        ClientPacket pkt;
                        memset(&pkt, 0, sizeof(pkt));
                        pkt.type = MSG_CHAT;
                        strncpy(pkt.chat_message, inp_chat_message.text, 199);
                        send(sock, &pkt, sizeof(pkt), 0);
                        inp_chat_message.text[0] = '\0';  // Clear input
                        inp_chat_message.is_active = 0;
                    }
                }
                // Kick buttons (host only, check for each player)
                else if (my_player_id == current_lobby.host_id) {
                    int card_y = 120;
                    int card_width = 440;
                    int start_x = 80;
                    
                    for (int i = 0; i < current_lobby.num_players; i++) {
                        if (i != current_lobby.host_id) {
                            SDL_Rect kick_btn = {start_x + 180, card_y + 26, 90, 28};
                            if (is_mouse_inside(kick_btn, mx, my)) {
                                // Kick player - show notification (backend not implemented)
                                snprintf(notification_message, sizeof(notification_message),
                                        "Kick player feature coming soon!");
                                notification_time = SDL_GetTicks();
                                break;
                            }
                        }
                        card_y += 95;
                    }
                }
            }
            
            // Chat text input handling
            if (e->type == SDL_TEXTINPUT && inp_chat_message.is_active) {
                handle_text_input(&inp_chat_message, e->text.text[0]);
            }
            
            // Chat keyboard handling  
            if (e->type == SDL_KEYDOWN && inp_chat_message.is_active) {
                if (e->key.keysym.sym == SDLK_BACKSPACE) {
                    handle_text_input(&inp_chat_message, '\b');
                }
                else if (e->key.keysym.sym == SDLK_RETURN && strlen(inp_chat_message.text) > 0) {
                    // Send chat message on Enter
                    ClientPacket pkt;
                    memset(&pkt, 0, sizeof(pkt));
                    pkt.type = MSG_CHAT;
                    strncpy(pkt.chat_message, inp_chat_message.text, 199);
                    send(sock, &pkt, sizeof(pkt), 0);
                    inp_chat_message.text[0] = '\0';  // Clear
                    inp_chat_message.is_active = 0;
                }
                else if (e->key.keysym.sym == SDLK_ESCAPE) {
                    inp_chat_message.is_active = 0;  // Cancel typing
                }
            }
            break;
            
        case SCREEN_GAME:
            if (e->type == SDL_MOUSEBUTTONDOWN) {
                SDL_Rect leave_btn = get_game_leave_button_rect();
                if (is_mouse_inside(leave_btn, mx, my)) {
                    send_packet(MSG_LEAVE_GAME, 0);
                    current_screen = SCREEN_LOBBY_LIST;
                    send_packet(MSG_LIST_LOBBIES, 0);
                    lobby_error_message[0] = '\0';
                    my_player_id = -1;
                    break;
                }
            }

            if (e->type == SDL_KEYDOWN) {
                if (e->key.keysym.sym == SDLK_ESCAPE) {
                    send_packet(MSG_LEAVE_GAME, 0);
                    current_screen = SCREEN_LOBBY_LIST;
                    send_packet(MSG_LIST_LOBBIES, 0);
                    lobby_error_message[0] = '\0';
                    my_player_id = -1;
                    break;
                }

                ClientPacket pkt;
                memset(&pkt, 0, sizeof(pkt));
                pkt.type = MSG_MOVE;
                pkt.data = -1;
                
                int key = e->key.keysym.sym;
                if (key == SDLK_w || key == SDLK_UP) pkt.data = MOVE_UP;
                else if (key == SDLK_s || key == SDLK_DOWN) pkt.data = MOVE_DOWN;
                else if (key == SDLK_a || key == SDLK_LEFT) pkt.data = MOVE_LEFT;
                else if (key == SDLK_d || key == SDLK_RIGHT) pkt.data = MOVE_RIGHT;
                else if (key == SDLK_SPACE) {
                    pkt.type = MSG_PLANT_BOMB;
                    pkt.data = 0;
                }
                
                if (pkt.data >= 0 || pkt.type == MSG_PLANT_BOMB) {
                    send(sock, &pkt, sizeof(pkt), 0);
                }
            }
            break;
        
        case SCREEN_POST_MATCH:
            if (e->type == SDL_MOUSEBUTTONDOWN) {
                // Post-match screen buttons - USE DYNAMIC RECTS
                if (is_mouse_inside(btn_rematch.rect, mx, my)) {
                    // Rematch - return to lobby room for another game
                    current_screen = SCREEN_LOBBY_ROOM;
                    post_match_shown = 0;
                }
                if (is_mouse_inside(btn_return_lobby.rect, mx, my)) {
                    if (my_player_id == -1) {
                        // Spectator: Leave lobby and return to list
                        send_packet(MSG_LEAVE_LOBBY, 0);
                        current_screen = SCREEN_LOBBY_LIST;
                        send_packet(MSG_LIST_LOBBIES, 0);
                        lobby_error_message[0] = '\0';
                    } else {
                        // Player: Back to Room - Actually we want to "Return to Lobby LIST" 
                        // because the user is "stuck" otherwise.
                        // ORIGINAL BUG: "Return to Lobby" kept user in room (SCREEN_LOBBY_ROOM). 
                        // FIX: Send LEAVE_LOBBY and go to SCREEN_LOBBY_LIST
                        send_packet(MSG_LEAVE_LOBBY, 0);
                        current_screen = SCREEN_LOBBY_LIST;
                        send_packet(MSG_LIST_LOBBIES, 0);
                    }
                    post_match_shown = 0;
                }
            }
            break;
            
        case SCREEN_FRIENDS: {
            /* ===== DELETE CONFIRM ===== */
            if (show_delete_confirm) {
                if (e->type == SDL_MOUSEBUTTONDOWN) {
                    SDL_Rect yes_btn = {300, 350, 100, 40};
                    SDL_Rect no_btn  = {420, 350, 100, 40};

                    if (is_mouse_inside(yes_btn, mx, my)) {
                        if (delete_friend_index >= 0 &&
                            delete_friend_index < friends_count) {

                            ClientPacket pkt;
                            memset(&pkt, 0, sizeof(pkt));
                            pkt.type = MSG_FRIEND_REMOVE;
                            pkt.target_user_id =
                                friends_list[delete_friend_index].user_id;
                            send(sock, &pkt, sizeof(pkt), 0);

                            send_packet(MSG_FRIEND_LIST, 0);

                            snprintf(notification_message,
                                    sizeof(notification_message),
                                    "Removed %s from friends",
                                    friends_list[delete_friend_index].display_name);
                            notification_time = SDL_GetTicks();
                        }
                        show_delete_confirm = 0;
                        delete_friend_index = -1;
                    }

                    if (is_mouse_inside(no_btn, mx, my)) {
                        show_delete_confirm = 0;
                        delete_friend_index = -1;
                    }
                }

                if (e->type == SDL_KEYDOWN &&
                    e->key.keysym.sym == SDLK_ESCAPE) {
                    show_delete_confirm = 0;
                    delete_friend_index = -1;
                }
                break;
            }

            /* ===== MOUSE CLICK ===== */
            if (e->type == SDL_MOUSEBUTTONDOWN) {

                inp_friend_request.is_active = is_mouse_inside(inp_friend_request.rect, mx, my);

                /* ===== FRIEND LIST ===== */
                int list_y = 134 + 46;
                int card_width = 360;

                for (int i = 0; i < friends_count && i < 5; i++) {
                    SDL_Rect card = {80, list_y, card_width, 100};
                    SDL_Rect remove_btn = {
                        card.x + card.w - 70,
                        list_y + 38,
                        50, 35
                    };

                    if (is_mouse_inside(remove_btn, mx, my)) {
                        show_delete_confirm = 1;
                        delete_friend_index = i;
                        break;
                    }

                    if (is_mouse_inside(card, mx, my)) {
                        ClientPacket pkt;
                        memset(&pkt, 0, sizeof(pkt));
                        pkt.type = MSG_GET_PROFILE;
                        pkt.target_user_id = friends_list[i].user_id;
                        send(sock, &pkt, sizeof(pkt), 0);
                        current_screen = SCREEN_PROFILE;
                        break;
                    }

                    list_y += 110;
                }

                /* ===== PENDING REQUESTS ===== */
                int pending_x = 480;
                int pending_y = 134 + 46;

                for (int i = 0; i < pending_count && i < 5; i++) {
                    SDL_Rect accept_btn  =
                        {pending_x + 40,  pending_y + 55, 90, 35};
                    SDL_Rect decline_btn =
                        {pending_x + 150, pending_y + 55, 90, 35};

                    if (is_mouse_inside(accept_btn, mx, my)) {
                        ClientPacket pkt;
                        memset(&pkt, 0, sizeof(pkt));
                        pkt.type = MSG_FRIEND_ACCEPT;
                        pkt.target_user_id = pending_requests[i].user_id;
                        send(sock, &pkt, sizeof(pkt), 0);

                        send_packet(MSG_FRIEND_LIST, 0);

                        snprintf(notification_message,
                                sizeof(notification_message),
                                "Accepted %s's friend request!",
                                pending_requests[i].display_name);
                        notification_time = SDL_GetTicks();
                        break;
                    }

                    if (is_mouse_inside(decline_btn, mx, my)) {
                        ClientPacket pkt;
                        memset(&pkt, 0, sizeof(pkt));
                        pkt.type = MSG_FRIEND_DECLINE;
                        pkt.target_user_id = pending_requests[i].user_id;
                        send(sock, &pkt, sizeof(pkt), 0);

                        send_packet(MSG_FRIEND_LIST, 0);

                        snprintf(notification_message,
                                sizeof(notification_message),
                                "Declined friend request");
                        notification_time = SDL_GetTicks();
                        break;
                    }

                    pending_y += 110;
                }

                /* ===== SEND FRIEND REQUEST ===== */
                if (is_mouse_inside(btn_send_friend_request.rect, mx, my) &&
                    strlen(inp_friend_request.text) > 0) {

                    ClientPacket pkt;
                    memset(&pkt, 0, sizeof(pkt));
                    pkt.type = MSG_FRIEND_REQUEST;
                    strncpy(pkt.target_display_name,
                            inp_friend_request.text,
                            MAX_DISPLAY_NAME - 1);
                    send(sock, &pkt, sizeof(pkt), 0);

                    snprintf(notification_message,
                            sizeof(notification_message),
                            "Friend request sent to %s!",
                            inp_friend_request.text);
                    notification_time = SDL_GetTicks();
                    inp_friend_request.text[0] = '\0';
                }

                if (is_mouse_inside((SDL_Rect){920, 40, 120, 40}, mx, my)) {
                    current_screen = SCREEN_LOBBY_LIST;
                    send_packet(MSG_LIST_LOBBIES, 0);
                }
            }

            /* ===== TEXT INPUT ===== */
            if (e->type == SDL_TEXTINPUT &&
                inp_friend_request.is_active) {
                handle_text_input(&inp_friend_request,
                                e->text.text[0]);
            }

            if (e->type == SDL_KEYDOWN &&
                inp_friend_request.is_active &&
                e->key.keysym.sym == SDLK_BACKSPACE) {
                handle_text_input(&inp_friend_request, '\b');
            }

            break;
        }

        case SCREEN_PROFILE: {
            if (e->type == SDL_MOUSEBUTTONDOWN) {
                SDL_Rect back_rect = {460, 650, 200, 60};
                if (is_mouse_inside(back_rect, mx, my)) {
                    current_screen = SCREEN_LOBBY_LIST;
                    send_packet(MSG_LIST_LOBBIES, 0);
                }
            }
            break;
        }

        case SCREEN_LEADERBOARD: {
            if (e->type == SDL_MOUSEBUTTONDOWN) {
                SDL_Rect back_rect = {460, 650, 200, 60};
                if (is_mouse_inside(back_rect, mx, my)) {
                    current_screen = SCREEN_LOBBY_LIST;
                    send_packet(MSG_LIST_LOBBIES, 0);
                }
            }
            break;
        }
        default:
            break;
    }
}
