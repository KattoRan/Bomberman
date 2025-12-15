/* client/ui_screens.c - Enhanced Version with Improved Lobby Room */
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string.h>
#include "../common/protocol.h"
#include "ui.h" 

// --- BẢNG MÀU CẢI TIẾN (Modern Dark Theme) ---
const SDL_Color CLR_BG          = {15, 23, 42, 255};      // Nền xanh đậm sang trọng
const SDL_Color CLR_GRID        = {30, 41, 59, 255};      // Lưới nhẹ
const SDL_Color CLR_PRIMARY     = {59, 130, 246, 255};    // Xanh dương sáng
const SDL_Color CLR_PRIMARY_DK  = {37, 99, 235, 255};     // Xanh đậm hơn
const SDL_Color CLR_ACCENT      = {251, 146, 60, 255};    // Cam Bomberman
const SDL_Color CLR_ACCENT_DK   = {234, 88, 12, 255};     // Cam đậm
const SDL_Color CLR_WHITE       = {248, 250, 252, 255};   // Trắng nhẹ
const SDL_Color CLR_GRAY        = {148, 163, 184, 255};   // Xám trung bình
const SDL_Color CLR_GRAY_LIGHT  = {203, 213, 225, 255};   // Xám sáng
const SDL_Color CLR_INPUT_BG    = {30, 41, 59, 255};      // Nền input
const SDL_Color CLR_BTN_NORM    = {59, 130, 246, 255};    // Nút xanh
const SDL_Color CLR_BTN_HOVER   = {96, 165, 250, 255};    // Hover sáng hơn
const SDL_Color CLR_SUCCESS     = {40, 200, 40, 255};     // Xanh lá
const SDL_Color CLR_DANGER      = {239, 68, 68, 255};     // Đỏ
const SDL_Color CLR_WARNING     = {234, 179, 8, 255};     // Vàng
const SDL_Color CLR_PURPLE      = {139, 92, 246, 255};    // Tím (Host)
const SDL_Color CLR_PURPLE_DARK = {109, 40, 217, 255};    // Tím đậm

#define UI_GRID_SIZE 40

// --- HÀM HỖ TRỢ VẼ ---

// Vẽ Button với gradient và shadow
void draw_button(SDL_Renderer *renderer, TTF_Font *font, Button *btn) {
    SDL_Color bg_color = btn->is_hovered ? CLR_BTN_HOVER : CLR_BTN_NORM;
    
    // Shadow (bóng đổ)
    SDL_Rect shadow = {btn->rect.x + 2, btn->rect.y + 4, btn->rect.w, btn->rect.h};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 60);
    SDL_RenderFillRect(renderer, &shadow);
    
    // Nền chính
    SDL_SetRenderDrawColor(renderer, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
    SDL_RenderFillRect(renderer, &btn->rect);
    
    // Viền sáng (top-left) - tạo hiệu ứng 3D
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 40);
    SDL_RenderDrawLine(renderer, btn->rect.x, btn->rect.y, 
                      btn->rect.x + btn->rect.w - 1, btn->rect.y);
    SDL_RenderDrawLine(renderer, btn->rect.x, btn->rect.y, 
                      btn->rect.x, btn->rect.y + btn->rect.h - 1);
    
    // Viền tối (bottom-right)
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 60);
    SDL_RenderDrawLine(renderer, btn->rect.x + 1, btn->rect.y + btn->rect.h - 1, 
                      btn->rect.x + btn->rect.w - 1, btn->rect.y + btn->rect.h - 1);
    SDL_RenderDrawLine(renderer, btn->rect.x + btn->rect.w - 1, btn->rect.y + 1, 
                      btn->rect.x + btn->rect.w - 1, btn->rect.y + btn->rect.h - 1);

    // Text
    if (font) {
        SDL_Surface *surf = TTF_RenderText_Blended(font, btn->text, CLR_WHITE);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect dst = {
                btn->rect.x + (btn->rect.w - surf->w) / 2,
                btn->rect.y + (btn->rect.h - surf->h) / 2,
                surf->w, surf->h
            };
            SDL_RenderCopy(renderer, tex, NULL, &dst);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
    }
}

// Vẽ Input Field với style hiện đại
void draw_input_field(SDL_Renderer *renderer, TTF_Font *font, InputField *field) {
    // Label
    if (font) {
        SDL_Color label_col = field->is_active ? CLR_ACCENT : CLR_GRAY;
        SDL_Surface *surf = TTF_RenderText_Blended(font, field->label, label_col);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect dst = { field->rect.x, field->rect.y - 28, surf->w, surf->h };
            SDL_RenderCopy(renderer, tex, NULL, &dst);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
    }

    // Shadow
    SDL_Rect shadow = {field->rect.x + 2, field->rect.y + 2, field->rect.w, field->rect.h};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 40);
    SDL_RenderFillRect(renderer, &shadow);

    // Nền
    SDL_SetRenderDrawColor(renderer, CLR_INPUT_BG.r, CLR_INPUT_BG.g, 
                          CLR_INPUT_BG.b, CLR_INPUT_BG.a);
    SDL_RenderFillRect(renderer, &field->rect);

    // Viền với glow effect khi active
    SDL_Color border_col = field->is_active ? CLR_ACCENT : CLR_GRAY;
    SDL_SetRenderDrawColor(renderer, border_col.r, border_col.g, 
                          border_col.b, border_col.a);
    
    // Vẽ viền dày hơn nếu active
    for (int i = 0; i < (field->is_active ? 2 : 1); i++) {
        SDL_Rect border = {field->rect.x - i, field->rect.y - i, 
                          field->rect.w + 2*i, field->rect.h + 2*i};
        SDL_RenderDrawRect(renderer, &border);
    }

    // Xử lý ẩn password
    char display_text[128] = "";
    if (strcmp(field->label, "Password:") == 0) {
        int len = strlen(field->text);
        for(int i = 0; i < len && i < 127; i++) display_text[i] = '*';
        display_text[len < 127 ? len : 127] = '\0';
    } else {
        strncpy(display_text, field->text, 127);
        display_text[127] = '\0';
    }

    // Vẽ text với clipping
    int text_w = 0;
    if (font && strlen(display_text) > 0) {
        SDL_Surface *surf = TTF_RenderText_Blended(font, display_text, CLR_WHITE);
        if (surf) {
            text_w = surf->w;
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            
            SDL_Rect old_clip;
            SDL_RenderGetClipRect(renderer, &old_clip);
            
            SDL_Rect clip_rect = {field->rect.x + 2, field->rect.y + 2, 
                                 field->rect.w - 4, field->rect.h - 4};
            SDL_RenderSetClipRect(renderer, &clip_rect);

            SDL_Rect dst = {
                field->rect.x + 15,
                field->rect.y + (field->rect.h - surf->h) / 2,
                surf->w, surf->h
            };
            SDL_RenderCopy(renderer, tex, NULL, &dst);
            
            if (old_clip.w == 0 && old_clip.h == 0) 
                SDL_RenderSetClipRect(renderer, NULL);
            else 
                SDL_RenderSetClipRect(renderer, &old_clip);

            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
    }

    // Con trỏ nhấp nháy
    if (field->is_active && (SDL_GetTicks() / 500) % 2 == 0) {
        int cursor_x = field->rect.x + 15 + text_w;
        if (cursor_x > field->rect.x + field->rect.w - 10) 
            cursor_x = field->rect.x + field->rect.w - 10;
            
        SDL_SetRenderDrawColor(renderer, CLR_ACCENT.r, CLR_ACCENT.g, 
                              CLR_ACCENT.b, 255);
        SDL_RenderDrawLine(renderer, cursor_x, field->rect.y + 10, 
                          cursor_x, field->rect.y + field->rect.h - 10);
    }
}

// Vẽ background với gradient effect
void draw_background_grid(SDL_Renderer *renderer, int w, int h) {
    // Nền gradient (giả lập bằng cách vẽ nhiều lớp)
    SDL_SetRenderDrawColor(renderer, CLR_BG.r, CLR_BG.g, CLR_BG.b, CLR_BG.a);
    SDL_RenderClear(renderer);

    // Vẽ lưới mờ
    SDL_SetRenderDrawColor(renderer, CLR_GRID.r, CLR_GRID.g, CLR_GRID.b, 80);
    for (int i = 0; i < w; i += UI_GRID_SIZE) {
        SDL_RenderDrawLine(renderer, i, 0, i, h);
    }
    for (int i = 0; i < h; i += UI_GRID_SIZE) {
        SDL_RenderDrawLine(renderer, 0, i, w, i);
    }
}

// --- CÁC MÀN HÌNH ---

// 1. Màn hình Đăng nhập - CÂN ĐỐI HOÀN TOÀN
void render_login_screen(SDL_Renderer *renderer, TTF_Font *font_large, TTF_Font *font_small, 
                        InputField *username, InputField *email, InputField *password,
                        Button *login_btn, Button *register_btn, 
                        const char *message) {
    
    int win_w, win_h;
    SDL_GetRendererOutputSize(renderer, &win_w, &win_h);

    draw_background_grid(renderer, win_w, win_h);

    // Tính toán vị trí trung tâm cho tất cả elements
    int center_x = win_w / 2;
    int content_width = 400;
    int start_x = center_x - content_width / 2;

    // Tiêu đề "BOMBERMAN" với hiệu ứng đẹp
    if (font_large) {
        const char *title = "BOMBERMAN";
        
        // Bóng đổ
        SDL_Surface *surf_shadow = TTF_RenderText_Blended(font_large, title, 
                                    (SDL_Color){0, 0, 0, 120});
        if (surf_shadow) {
            SDL_Texture *tex_shadow = SDL_CreateTextureFromSurface(renderer, surf_shadow);
            SDL_Rect rect_shadow = { 
                center_x - surf_shadow->w / 2 + 4, 
                64, 
                surf_shadow->w, 
                surf_shadow->h 
            };
            SDL_RenderCopy(renderer, tex_shadow, NULL, &rect_shadow);
            SDL_DestroyTexture(tex_shadow);
            SDL_FreeSurface(surf_shadow);
        }
        
        // Chữ chính
        SDL_Surface *surf = TTF_RenderText_Blended(font_large, title, CLR_ACCENT);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = { 
                center_x - surf->w / 2, 
                60, 
                surf->w, 
                surf->h 
            };
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
    }

    // Cập nhật vị trí các input fields để căn giữa
    // If email is NULL, we're in login mode (2 fields), otherwise registration (3 fields)
    int is_registration = (email != NULL);
    
    username->rect.x = start_x;
    username->rect.y = is_registration ? 150 : 200;
    username->rect.w = content_width;
    username->rect.h = 45;

    if (is_registration) {
        email->rect.x = start_x;
        email->rect.y = 220;
        email->rect.w = content_width;
        email->rect.h = 45;
    }

    password->rect.x = start_x;
    password->rect.y = is_registration ? 290 : 280;
    password->rect.w = content_width;
    password->rect.h = 45;

    // Vẽ input fields
    draw_input_field(renderer, font_small, username);
    if (is_registration) {
        draw_input_field(renderer, font_small, email);
    }
    draw_input_field(renderer, font_small, password);

    // Cập nhật vị trí các buttons để căn giữa
    int button_width = 180;
    int button_spacing = 20;
    int total_button_width = button_width * 2 + button_spacing;
    int button_start_x = center_x - total_button_width / 2;
    int button_y = is_registration ? 380 : 380;

    login_btn->rect.x = button_start_x;
    login_btn->rect.y = button_y;
    login_btn->rect.w = button_width;
    login_btn->rect.h = 50;

    register_btn->rect.x = button_start_x + button_width + button_spacing;
    register_btn->rect.y = button_y;
    register_btn->rect.w = button_width;
    register_btn->rect.h = 50;
    
    // Set button labels based on mode
    if (is_registration) {
        strcpy(login_btn->text, "Back");
        strcpy(register_btn->text, "Register");
    } else {
        strcpy(login_btn->text, "Login");
        strcpy(register_btn->text, "Register");
    }

    draw_button(renderer, font_small, login_btn);
    draw_button(renderer, font_small, register_btn);
    
    // Vẽ thông báo lỗi (căn giữa)
    if (font_small && message && strlen(message) > 0) {
        SDL_Surface *surf = TTF_RenderText_Blended(font_small, message, CLR_DANGER);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {
                center_x - surf->w / 2,
                460,
                surf->w, 
                surf->h
            };
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
    }
    
    //SDL_RenderPresent(renderer);
}

// 2. Màn hình Danh sách phòng - Cải thiện màu sắc
void render_lobby_list_screen(SDL_Renderer *renderer, TTF_Font *font,
                              Lobby *lobbies, int lobby_count,
                              Button *create_btn, Button *refresh_btn,
                              int selected_lobby) {
    int win_w, win_h;
    SDL_GetRendererOutputSize(renderer, &win_w, &win_h);

    draw_background_grid(renderer, win_w, win_h);
    
    // Tiêu đề
    if (font) {
        SDL_Surface *title = TTF_RenderText_Blended(font, "GAME LOBBIES", CLR_ACCENT);
        if (title) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, title);
            SDL_Rect rect = {(win_w - title->w) / 2, 30, title->w, title->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(title);
        }
    }
    
    // Danh sách lobbies
    int y = 100;
    int list_width = 700;
    int start_x = (win_w - list_width) / 2;

    for (int i = 0; i < lobby_count; i++) {
        SDL_Rect lobby_rect = {start_x, y, list_width, 70};
        
        // Shadow
        SDL_Rect shadow = {start_x + 3, y + 3, list_width, 70};
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 50);
        SDL_RenderFillRect(renderer, &shadow);
        
        // Nền item
        SDL_Color bg = (i == selected_lobby) ? CLR_PRIMARY : CLR_INPUT_BG;
        SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, bg.a);
        SDL_RenderFillRect(renderer, &lobby_rect);
        
        // Viền
        SDL_Color border = (i == selected_lobby) ? CLR_ACCENT : CLR_GRAY;
        SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, 255);
        SDL_RenderDrawRect(renderer, &lobby_rect);
        
        if (font) {
            // Tên lobby với lock icon nếu private
            char display_name[80];
            if (lobbies[i].is_private) {
                snprintf(display_name, sizeof(display_name), "[PRIVATE] %s (ID:%d)", lobbies[i].name, lobbies[i].id);
            } else {
                snprintf(display_name, sizeof(display_name), "%s (ID:%d)", lobbies[i].name, lobbies[i].id);
            }
            
            SDL_Surface *name_surf = TTF_RenderText_Blended(font, display_name, CLR_WHITE);
            if (name_surf) {
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, name_surf);
                SDL_Rect rect = {start_x + 20, y + 12, name_surf->w, name_surf->h};
                SDL_RenderCopy(renderer, tex, NULL, &rect);
                SDL_DestroyTexture(tex);
                SDL_FreeSurface(name_surf);
            }
            
            // Thông tin players và status
            char info[128];
            const char *status_text = lobbies[i].status == LOBBY_WAITING ? "Waiting" : "Playing";
            if (lobbies[i].is_private) {
                snprintf(info, sizeof(info), "Players: %d/4 | %s | PRIVATE",
                        lobbies[i].num_players, status_text);
            } else {
                snprintf(info, sizeof(info), "Players: %d/4 | %s",
                        lobbies[i].num_players, status_text);
            }
            
            SDL_Color info_color = lobbies[i].status == LOBBY_WAITING ? CLR_SUCCESS : CLR_WARNING;
            SDL_Surface *info_surf = TTF_RenderText_Blended(font, info, info_color);
            if (info_surf) {
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, info_surf);
                SDL_Rect rect = {start_x + 20, y + 40, info_surf->w, info_surf->h};
                SDL_RenderCopy(renderer, tex, NULL, &rect);
                SDL_DestroyTexture(tex);
                SDL_FreeSurface(info_surf);
            }
        }
        y += 85;
    }
    
    draw_button(renderer, font, create_btn);
    draw_button(renderer, font, refresh_btn);
    //SDL_RenderPresent(renderer);
}

// 3. Màn hình Phòng chờ - CẢI THIỆN HOÀN TOÀN
void render_lobby_room_screen(SDL_Renderer *renderer, TTF_Font *font,
                              Lobby *lobby, int my_player_id,
                              Button *ready_btn, Button *start_btn, 
                              Button *leave_btn) {
    int win_w, win_h;
    SDL_GetRendererOutputSize(renderer, &win_w, &win_h);

    draw_background_grid(renderer, win_w, win_h);
    
    // Tên phòng với viền đẹp
    if (font) {
        char title_text[128];
        snprintf(title_text, sizeof(title_text), "ROOM: %s", lobby->name);
        
        // Shadow cho title
        SDL_Surface *title_shadow = TTF_RenderText_Blended(font, title_text, (SDL_Color){0, 0, 0, 120});
        if (title_shadow) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, title_shadow);
            SDL_Rect rect = {(win_w - title_shadow->w) / 2 + 2, 32, title_shadow->w, title_shadow->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(title_shadow);
        }
        
        SDL_Surface *title = TTF_RenderText_Blended(font, title_text, CLR_ACCENT);
        if (title) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, title);
            SDL_Rect rect = {(win_w - title->w) / 2, 30, title->w, title->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(title);
        }
    }
    
    // Danh sách players với màu sắc gradient
    int y = 120;
    int card_width = 600;
    int start_x = (win_w - card_width) / 2;

    for (int i = 0; i < lobby->num_players; i++) {
        Player *p = &lobby->players[i];
        
        int card_height = 75;
        SDL_Rect card_rect = {start_x, y, card_width, card_height};
        
        // Shadow Ä'áº¹p hÆ¡n
        SDL_Rect shadow = {start_x + 4, y + 4, card_width, card_height};
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 100);
        SDL_RenderFillRect(renderer, &shadow);
        
        // Ná»n card
        SDL_Color card_bg;
        if (i == lobby->host_id) {
            card_bg = (SDL_Color){109, 40, 217, 255};
        } else {
            card_bg = (SDL_Color){51, 65, 85, 255};
        }
        SDL_SetRenderDrawColor(renderer, card_bg.r, card_bg.g, card_bg.b, card_bg.a);
        SDL_RenderFillRect(renderer, &card_rect);
        
        // Accent bar bÃªn trÃ¡i
        SDL_Rect accent_bar = {start_x, y, 6, card_height};
        SDL_Color accent_color;
        if (i == lobby->host_id) {
            accent_color = (SDL_Color){167, 139, 250, 255}; 
        } else if (p->is_ready) {
            accent_color = CLR_SUCCESS;
        } else {
            accent_color = CLR_WARNING;
        }
        SDL_SetRenderDrawColor(renderer, accent_color.r, accent_color.g, accent_color.b, 255);
        SDL_RenderFillRect(renderer, &accent_bar);
        
        //view card
        SDL_SetRenderDrawColor(renderer, 71, 85, 105, 255);
        SDL_RenderDrawRect(renderer, &card_rect);

        if (font) {
            SDL_Rect number_circle = {start_x + 20, y + 22, 30, 30};
            SDL_SetRenderDrawColor(renderer, 30, 41, 59, 200);
            SDL_RenderFillRect(renderer, &number_circle);
            SDL_SetRenderDrawColor(renderer, accent_color.r, accent_color.g, accent_color.b, 255);
            SDL_RenderDrawRect(renderer, &number_circle);
            
            char num_str[16];
            snprintf(num_str, sizeof(num_str), "%d", i + 1);
            SDL_Surface *num_surf = TTF_RenderText_Blended(font, num_str, CLR_WHITE);
            if (num_surf) {
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, num_surf);
                SDL_Rect num_rect = {
                    number_circle.x + (30 - num_surf->w) / 2,
                    number_circle.y + (30 - num_surf->h) / 2,
                    num_surf->w, num_surf->h
                };
                SDL_RenderCopy(renderer, tex, NULL, &num_rect);
                SDL_DestroyTexture(tex);
                SDL_FreeSurface(num_surf);
            }
            
            // Username
            SDL_Color username_color = (i == lobby->host_id) ? 
                (SDL_Color){233, 213, 255, 255} : CLR_WHITE;
            SDL_Surface *name_surf = TTF_RenderText_Blended(font, p->username, username_color);
            if (name_surf) {
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, name_surf);
                SDL_Rect name_rect = {start_x + 65, y + 15, name_surf->w, name_surf->h};
                SDL_RenderCopy(renderer, tex, NULL, &name_rect);
                SDL_DestroyTexture(tex);
                SDL_FreeSurface(name_surf);
            }
            
            // Role badge (HOST hoáº·c trÃ¡ng thÃ¡i)
            if (i == lobby->host_id) {
                SDL_Rect host_badge = {start_x + 65, y + 43, 60, 22};
                SDL_SetRenderDrawColor(renderer, 167, 139, 250, 255);
                SDL_RenderFillRect(renderer, &host_badge);
                
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
            
            // Status badge bÃªn pháº£i
            if (i != lobby->host_id) {
                SDL_Rect status_badge = {start_x + card_width - 100, y + 22, 80, 30};
                SDL_Color badge_color = p->is_ready ? CLR_SUCCESS : 
                    (SDL_Color){71, 85, 105, 255};
                SDL_SetRenderDrawColor(renderer, badge_color.r, badge_color.g, 
                    badge_color.b, 255);
                SDL_RenderFillRect(renderer, &status_badge);
                
                // Viá»n badge
                SDL_SetRenderDrawColor(renderer, badge_color.r - 20, badge_color.g - 20,
                    badge_color.b - 20, 255);
                SDL_RenderDrawRect(renderer, &status_badge);
                
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
            } else {
                // // Crown icon cho host 
                // SDL_Rect crown_bg = {start_x + card_width - 60, y + 22, 40, 30};
                // SDL_SetRenderDrawColor(renderer, 251, 191, 36, 200);
                // SDL_RenderFillRect(renderer, &crown_bg);
                
                // SDL_Surface *crown_surf = TTF_RenderText_Blended(font, "♛", CLR_WHITE);
                // if (crown_surf) {
                //     SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, crown_surf);
                //     SDL_Rect rect = {
                //         crown_bg.x + (crown_bg.w - crown_surf->w) / 2,
                //         crown_bg.y + (crown_bg.h - crown_surf->h) / 2,
                //         crown_surf->w, crown_surf->h
                //     };
                //     SDL_RenderCopy(renderer, tex, NULL, &rect);
                //     SDL_DestroyTexture(tex);
                //     SDL_FreeSurface(crown_surf);
                // }
            }
        }
        y += card_height + 15;
    }
    
    // Vẽ các nút ở dưới cùng - CÂN GIỮA
    int button_y = win_h - 80;
    int center_x = win_w / 2;
    
    // Cập nhật vị trí nút Leave (luôn hiện)
    leave_btn->rect.x = center_x + 120;
    leave_btn->rect.y = button_y;
    leave_btn->rect.w = 150;
    leave_btn->rect.h = 50;
    
    // Vẽ nút tùy theo vai trò
    if (my_player_id == lobby->host_id) {
        // Host chỉ thấy Start Game và Leave
        if (start_btn) {
            start_btn->rect.x = center_x - 270;
            start_btn->rect.y = button_y;
            start_btn->rect.w = 230;
            start_btn->rect.h = 50;
            draw_button(renderer, font, start_btn);
        }
    } else {
        // Người chơi thường chỉ thấy Ready và Leave
        if (ready_btn) {
            ready_btn->rect.x = center_x - 270;
            ready_btn->rect.y = button_y;
            ready_btn->rect.w = 230;
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
            int msg_y = button_y - msg_height - 20;
            
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
}/* Room creation dialog rendering - add to ui_screens.c */

void render_create_room_dialog(SDL_Renderer *renderer, TTF_Font *font,
                                InputField *room_name, InputField *access_code,
                                Button *create_btn, Button *cancel_btn) {
    int win_w, win_h;
    SDL_GetRendererOutputSize(renderer, &win_w, &win_h);
    
    // Darken background
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_Rect full_screen = {0, 0, win_w, win_h};
    SDL_RenderFillRect(renderer, &full_screen);
    
    // Dialog box
    int dialog_w = 500;
    int dialog_h = 400;
    int dialog_x = (win_w - dialog_w) / 2;
    int dialog_y = (win_h - dialog_h) / 2;
    
    SDL_Rect dialog = {dialog_x, dialog_y, dialog_w, dialog_h};
    SDL_SetRenderDrawColor(renderer, 30, 41, 59, 255);
    SDL_RenderFillRect(renderer, &dialog);
    
    SDL_SetRenderDrawColor(renderer, 59, 130, 246, 255);
    SDL_RenderDrawRect(renderer, &dialog);
    
    // Title
    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface *title = TTF_RenderText_Blended(font, "Create Room", white);
    if (title) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, title);
        SDL_Rect rect = {dialog_x + (dialog_w - title->w)/2, dialog_y + 20, title->w, title->h};
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(title);
    }
    
    // Position input fields
    room_name->rect.x = dialog_x + 50;
    room_name->rect.y = dialog_y + 80;
    room_name->rect.w = dialog_w - 100;
    room_name->rect.h = 40;
    
    // Only show access code field if provided (for create room)
    if (access_code) {
        access_code->rect.x = dialog_x + 50;
        access_code->rect.y = dialog_y + 160;
        access_code->rect.w = dialog_w - 100;
        access_code->rect.h = 40;
        
        // Helper text
        const char *helper = "Leave access code empty for public room";
        SDL_Color gray = {148, 163, 184, 255};
        SDL_Surface *helper_surf = TTF_RenderText_Blended(font, helper, gray);
        if (helper_surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, helper_surf);
            SDL_Rect rect = {dialog_x + 50, dialog_y + 210, helper_surf->w, helper_surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(helper_surf);
        }
    }
    
    // Buttons
    create_btn->rect = (SDL_Rect){dialog_x + 50, dialog_y + dialog_h - 80, 180, 50};
    cancel_btn->rect = (SDL_Rect){dialog_x + dialog_w - 230, dialog_y + dialog_h - 80, 180, 50};
    
    strcpy(create_btn->text, "Create");
    strcpy(cancel_btn->text, "Cancel");
}
