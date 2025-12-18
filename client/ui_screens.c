/* client/ui_screens.c - Enhanced Version with Improved Lobby Room */
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string.h>
#include <math.h>
#include "../common/protocol.h"
#include "ui.h" 

// External gradient rendering functions from graphics.c
extern void draw_vertical_gradient(SDL_Renderer *renderer, SDL_Rect rect, SDL_Color top, SDL_Color bottom);
extern void draw_horizontal_gradient(SDL_Renderer *renderer, SDL_Rect rect, SDL_Color left, SDL_Color right);

// Helper function to truncate text if it exceeds max width
void truncate_text_to_fit(char *text, size_t text_size, TTF_Font *font, int max_width) {
    if (!font || !text) return;
    
    int text_width, text_height;
    TTF_SizeText(font, text, &text_width, &text_height);
    
    // If text fits, no need to truncate
    if (text_width <= max_width) return;
    
    // Truncate and add ellipsis
    size_t len = strlen(text);
    char truncated[256];
    strncpy(truncated, text, sizeof(truncated) - 1);
    truncated[sizeof(truncated) - 1] = '\0';
    
    // Keep removing characters until it fits (with "...")
    while (len > 3) {
        len--;
        strcpy(truncated + len, "...");
        truncated[len + 3] = '\0';
        
        TTF_SizeText(font, truncated, &text_width, &text_height);
        if (text_width <= max_width) {
            strncpy(text, truncated, text_size);
            text[text_size - 1] = '\0';
            return;
        }
    }
    
    // Extreme case: just use "..."
    strncpy(text, "...", text_size);
} 

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

// --- GRADIENT & GLOW COLORS ---
const SDL_Color CLR_ACCENT_LIGHT = {255, 176, 102, 255};  // Cam sáng (gradient)
const SDL_Color CLR_PRIMARY_LIGHT = {96, 165, 250, 255};  // Xanh sáng hơn
const SDL_Color CLR_BG_DARK = {8, 15, 30, 255};           // Nền đậm nhất
const SDL_Color CLR_GLOW = {255, 255, 255, 60};           // Glow hover
const SDL_Color CLR_SHADOW_SOFT = {0, 0, 0, 30};           // Soft shadows
const SDL_Color CLR_SHADOW_HARD = {0, 0, 0, 80};           // Hard shadows

// --- FIXED LAYOUT FOR 1920x1080 FULLSCREEN ---
#define FIXED_BUTTON_WIDTH 200
#define FIXED_BUTTON_HEIGHT 50
#define FIXED_SPACING 20
#define FIXED_INPUT_WIDTH 400
#define FIXED_INPUT_HEIGHT 50

// FIXED button dimensions - no responsive
int get_button_width(int screen_w) {
    (void)screen_w;  // Unused
    return FIXED_BUTTON_WIDTH;
}

int get_button_height(int screen_h) {
    (void)screen_h;  // Unused
    return FIXED_BUTTON_HEIGHT;
}

int get_spacing(int screen_h) {
    (void)screen_h;  // Unused
    return FIXED_SPACING;
}

#define UI_GRID_SIZE 40
#define UI_CORNER_RADIUS 8      // Rounded corner radius
#define UI_SHADOW_OFFSET 4      // Shadow offset distance

// --- HÀM HỖ TRỢ VẼ ---

// Helper: Draw rounded rectangle (simulated with corner pixels)
void draw_rounded_rect(SDL_Renderer *renderer, SDL_Rect rect, SDL_Color color, int radius) {
    if (radius <= 0) {
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_RenderFillRect(renderer, &rect);
        return;
    }
    
    // Clamp radius to half of smallest dimension
    int max_radius = (rect.w < rect.h ? rect.w : rect.h) / 2;
    if (radius > max_radius) radius = max_radius;
    
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    
    // Draw main rectangles (center and sides without corners)
    SDL_Rect center = {rect.x + radius, rect.y, rect.w - 2*radius, rect.h};
    SDL_RenderFillRect(renderer, &center);
    
    SDL_Rect left = {rect.x, rect.y + radius, radius, rect.h - 2*radius};
    SDL_RenderFillRect(renderer, &left);
    
    SDL_Rect right = {rect.x + rect.w - radius, rect.y + radius, radius, rect.h - 2*radius};
    SDL_RenderFillRect(renderer, &right);
    
    // Draw corner circles (approximated with filled squares in circular pattern)
    for (int dy = 0; dy < radius; dy++) {
        for (int dx = 0; dx < radius; dx++) {
            // Check if point is inside circle
            int dist_sq = dx * dx + dy * dy;
            int r_sq = radius * radius;
            if (dist_sq <= r_sq) {
                // Top-left corner
                SDL_RenderDrawPoint(renderer, rect.x + radius - dx, rect.y + radius - dy);
                // Top-right corner
                SDL_RenderDrawPoint(renderer, rect.x + rect.w - radius + dx - 1, rect.y + radius - dy);
                // Bottom-left corner
                SDL_RenderDrawPoint(renderer, rect.x + radius - dx, rect.y + rect.h - radius + dy - 1);
                // Bottom-right corner
                SDL_RenderDrawPoint(renderer, rect.x + rect.w - radius + dx - 1, rect.y + rect.h - radius + dy - 1);
            }
        }
    }
}

// Helper: Draw rounded rectangle border
void draw_rounded_border(SDL_Renderer *renderer, SDL_Rect rect, SDL_Color color, int radius, int thickness) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    
    for (int t = 0; t < thickness; t++) {
        SDL_Rect outer = {rect.x + t, rect.y + t, rect.w - 2*t, rect.h - 2*t};
        
        if (radius <= 0) {
            SDL_RenderDrawRect(renderer, &outer);
        } else {
            // Draw straight lines
            SDL_RenderDrawLine(renderer, outer.x + radius, outer.y, outer.x + outer.w - radius, outer.y); // Top
            SDL_RenderDrawLine(renderer, outer.x + radius, outer.y + outer.h - 1, outer.x + outer.w - radius, outer.y + outer.h - 1); // Bottom
            SDL_RenderDrawLine(renderer, outer.x, outer.y + radius, outer.x, outer.y + outer.h - radius); // Left
            SDL_RenderDrawLine(renderer, outer.x + outer.w - 1, outer.y + radius, outer.x + outer.w - 1, outer.y + outer.h - radius); // Right
            
            // Draw corner arcs (simplified)
            for (int dy = 0; dy < radius; dy++) {
                for (int dx = 0; dx < radius; dx++) {
                    int dist_sq = dx * dx + dy * dy;
                    int r_outer_sq = radius * radius;
                    int r_inner_sq = (radius - 1) * (radius - 1);
                    
                    if (dist_sq <= r_outer_sq && dist_sq >= r_inner_sq) {
                        SDL_RenderDrawPoint(renderer, outer.x + radius - dx, outer.y + radius - dy);
                        SDL_RenderDrawPoint(renderer, outer.x + outer.w - radius + dx - 1, outer.y + radius - dy);
                        SDL_RenderDrawPoint(renderer, outer.x + radius - dx, outer.y + outer.h - radius + dy - 1);
                        SDL_RenderDrawPoint(renderer, outer.x + outer.w - radius + dx - 1, outer.y + outer.h - radius + dy - 1);
                    }
                }
            }
        }
    }
}

// Helper: Draw layered shadow for depth
void draw_layered_shadow(SDL_Renderer *renderer, SDL_Rect rect, int radius, int offset) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    
    // Layer 1: Softest, furthest
    SDL_Rect shadow1 = {rect.x + offset + 2, rect.y + offset + 2, rect.w, rect.h};
    draw_rounded_rect(renderer, shadow1, CLR_SHADOW_SOFT, radius);
    
    // Layer 2: Medium
    SDL_Rect shadow2 = {rect.x + offset, rect.y + offset, rect.w, rect.h};
    draw_rounded_rect(renderer, shadow2, (SDL_Color){0, 0, 0, 50}, radius);
    
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

// Vẽ Button với gradient, rounded corners và shadow
void draw_button(SDL_Renderer *renderer, TTF_Font *font, Button *btn) {
    SDL_Color bg_color_top, bg_color_bottom;
    
    if (btn->is_hovered) {
        bg_color_top = (SDL_Color){96, 165, 250, 255};   // Lighter blue
        bg_color_bottom = CLR_BTN_HOVER;
    } else {
        bg_color_top = CLR_BTN_NORM;
        bg_color_bottom = CLR_PRIMARY_DK;
    }
    
    // Layered shadow for depth
    draw_layered_shadow(renderer, btn->rect, UI_CORNER_RADIUS, UI_SHADOW_OFFSET);
    
    // Main button with vertical gradient
    draw_vertical_gradient(renderer, btn->rect, bg_color_top, bg_color_bottom);
    
    // Outer border
    draw_rounded_border(renderer, btn->rect, bg_color_bottom, UI_CORNER_RADIUS, 1);
    
    // Glow effect on hover (stronger and colored)
    if (btn->is_hovered) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        
        // Pulsing glow
        float pulse = (sinf(SDL_GetTicks() / 300.0f) + 1.0f) / 2.0f;
        int glow_alpha = 40 + (int)(pulse * 60);
        
        for (int i = 1; i <= 3; i++) {
            SDL_Rect glow = {btn->rect.x - i, btn->rect.y - i, btn->rect.w + i*2, btn->rect.h + i*2};
            SDL_Color glow_color = {150, 200, 255, glow_alpha / i};
            draw_rounded_border(renderer, glow, glow_color, UI_CORNER_RADIUS + i, 1);
        }
        
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
    
    // Subtle top highlight for 3D effect
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_Rect highlight = {btn->rect.x + UI_CORNER_RADIUS, btn->rect.y + 2, 
                          btn->rect.w - 2*UI_CORNER_RADIUS, 3};
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 50);
    SDL_RenderFillRect(renderer, &highlight);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    // Truncate button text to fit
    char button_text[64];
    strncpy(button_text, btn->text, sizeof(button_text) - 1);
    button_text[sizeof(button_text) - 1] = '\0';
    truncate_text_to_fit(button_text, sizeof(button_text), font, btn->rect.w - 20);

    // Text with shadow for depth
    if (font) {
        // Text shadow
        SDL_Surface *shadow_surf = TTF_RenderText_Blended(font, button_text, (SDL_Color){0, 0, 0, 150});
        if (shadow_surf) {
            SDL_Texture *shadow_tex = SDL_CreateTextureFromSurface(renderer, shadow_surf);
            SDL_Rect shadow_dst = {
                btn->rect.x + (btn->rect.w - shadow_surf->w) / 2 + 1,
                btn->rect.y + (btn->rect.h - shadow_surf->h) / 2 + 1,
                shadow_surf->w, shadow_surf->h
            };
            SDL_RenderCopy(renderer, shadow_tex, NULL, &shadow_dst);
            SDL_DestroyTexture(shadow_tex);
            SDL_FreeSurface(shadow_surf);
        }
        
        // Main text
        SDL_Surface *surf = TTF_RenderText_Blended(font, button_text, CLR_WHITE);
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


// Vẽ Input Field với style hiện đại và rounded corners
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

    // Layered shadow
    draw_layered_shadow(renderer, field->rect, UI_CORNER_RADIUS, 2);

    // Background with rounded corners
    draw_rounded_rect(renderer, field->rect, CLR_INPUT_BG, UI_CORNER_RADIUS);

    // Border with glow effect when active
    SDL_Color border_col = field->is_active ? CLR_ACCENT : CLR_GRAY;
    int border_thickness = field->is_active ? 2 : 1;
    draw_rounded_border(renderer, field->rect, border_col, UI_CORNER_RADIUS, border_thickness);
    
    // Glow on active
    if (field->is_active) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_Rect glow = {field->rect.x - 1, field->rect.y - 1, field->rect.w + 2, field->rect.h + 2};
        draw_rounded_border(renderer, glow, CLR_ACCENT, UI_CORNER_RADIUS + 1, 1);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
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


// Vẽ background với animated gradient grid
void draw_background_grid(SDL_Renderer *renderer, int w, int h) {
    // Dark gradient background (top darker, bottom lighter)
    SDL_SetRenderDrawColor(renderer, CLR_BG_DARK.r, CLR_BG_DARK.g, CLR_BG_DARK.b, 255);
    SDL_RenderClear(renderer);
    
    // Gradient effect (simulate by drawing horizontal lines)
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (int y = 0; y < h; y += 2) {
        int alpha = 20 + (y * 40 / h);  // 20 to 60 alpha
        SDL_SetRenderDrawColor(renderer, CLR_BG.r, CLR_BG.g, CLR_BG.b, alpha);
        SDL_RenderDrawLine(renderer, 0, y, w, y);
    }
    
    // Animated pulsing grid - VERY VISIBLE
    float pulse = (sinf(SDL_GetTicks() / 700.0f) + 1.0f) / 2.0f;  // 0.0 to 1.0, faster
    int grid_alpha = 20 + (int)(pulse * 140);  // 20 to 160 (much more dramatic!)
    
    SDL_SetRenderDrawColor(renderer, CLR_GRID.r, CLR_GRID.g, CLR_GRID.b, grid_alpha);
    for (int i = 0; i < w; i += UI_GRID_SIZE) {
        SDL_RenderDrawLine(renderer, i, 0, i, h);
    }
    for (int i = 0; i < h; i += UI_GRID_SIZE) {
        SDL_RenderDrawLine(renderer, 0, i, w, i);
    }
    
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
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

    // Input field positions already set in main.c (685, 350/450/550, 550x65)
    // Just draw them
    int is_registration = (email != NULL);
    
    draw_input_field(renderer, font_small, username);
    if (is_registration) {
        // Adjust password Y for registration mode
        password->rect.y = 650;  // More space for email field
        draw_input_field(renderer, font_small, email);
    } else {
        password->rect.y = 550;  // Standard login spacing
    }
    draw_input_field(renderer, font_small, password);

    // Button positions already set in main.c (760/970, 680, 180x60)
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
                800,
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

// 2. Màn hình Danh sách phòng - Modern Card Design
void render_lobby_list_screen(SDL_Renderer *renderer, TTF_Font *font,
                              Lobby *lobbies, int lobby_count,
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
            SDL_Rect rect = {(win_w - title_shadow->w) / 2 + 3, 33, title_shadow->w, title_shadow->h};
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
    
    // FIXED lobby cards for 1920x1080
    int y = 162;  // Fixed from top
    int list_width = 1056;  // Fixed width (55% of 1920)
    int card_height = 75;  // Fixed height
    int start_x = 432;  // Centered: (1920-1056)/2

    for (int i = 0; i < lobby_count; i++) {
        SDL_Rect lobby_rect = {start_x, y, list_width, card_height};
        
        // Layered shadow (stronger for selected)
        int shadow_offset = (i == selected_lobby) ? 6 : 4;
        draw_layered_shadow(renderer, lobby_rect, UI_CORNER_RADIUS, shadow_offset);
        
        // Background with rounded corners
        SDL_Color bg = (i == selected_lobby) ? CLR_PRIMARY : CLR_INPUT_BG;
        draw_rounded_rect(renderer, lobby_rect, bg, UI_CORNER_RADIUS);
        
        // Border (accent for selected, gray for others)
        SDL_Color border = (i == selected_lobby) ? CLR_ACCENT : CLR_GRAY;
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
                snprintf(display_name, sizeof(display_name), "[PRIVATE] %s (ID:%d)", lobbies[i].name, lobbies[i].id);
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
            char player_text[16];
            snprintf(player_text, sizeof(player_text), "%d/4", lobbies[i].num_players);
            SDL_Rect player_badge = {start_x + list_width - 70, y + 20, 50, 35};
            
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
        }
        y += card_height + 10;
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
        
        // Host-only buttons (Lock Room, Chat) - top right
        if (my_player_id == lobby->host_id) {
            // Lock Room button
            SDL_Rect lock_btn = {1620, 100, 180, 50};
            SDL_Color lock_bg = lobby->is_locked ? CLR_DANGER : CLR_PRIMARY;
            const char *lock_text = lobby->is_locked ? "Unlock" : "Lock Room";
            
            draw_layered_shadow(renderer, lock_btn, UI_CORNER_RADIUS, 3);
            draw_rounded_rect(renderer, lock_btn, lock_bg, UI_CORNER_RADIUS);
            draw_rounded_border(renderer, lock_btn, CLR_GRAY, UI_CORNER_RADIUS, 1);
            
            SDL_Surface *surf = TTF_RenderText_Blended(font, lock_text, CLR_WHITE);
            if (surf) {
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
                SDL_Rect rect = {lock_btn.x + (lock_btn.w - surf->w) / 2,
                                lock_btn.y + (lock_btn.h - surf->h) / 2,
                                surf->w, surf->h};
                SDL_RenderCopy(renderer, tex, NULL, &rect);
                SDL_DestroyTexture(tex);
                SDL_FreeSurface(surf);
            }
        }
        
        // Chat button (for everyone)
        SDL_Rect chat_btn = {1620, 170, 180, 50};
        draw_layered_shadow(renderer, chat_btn, UI_CORNER_RADIUS, 3);
        draw_rounded_rect(renderer, chat_btn, CLR_INPUT_BG, UI_CORNER_RADIUS);
        draw_rounded_border(renderer, chat_btn, CLR_PRIMARY, UI_CORNER_RADIUS, 1);
        
        SDL_Surface *surf = TTF_RenderText_Blended(font, "Chat", CLR_WHITE);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = {chat_btn.x + (chat_btn.w - surf->w) / 2,
                            chat_btn.y + (chat_btn.h - surf->h) / 2,
                            surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
    }
    
    // Player list with LARGER cards
    int y = 180;  // Start a bit lower after title
    int card_width = 750;  // LARGER (was 600)
    int start_x = (win_w - card_width) / 2;

    for (int i = 0; i < lobby->num_players; i++) {
        Player *p = &lobby->players[i];
        
        int card_height = 100;  // LARGER (was 75)
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
                SDL_Rect host_badge = {start_x + 75, y + 55, 80, 28};
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
                SDL_Rect status_badge = {start_x + card_width - 120, y + 35, 100, 40};  // 100x40 (was 80x30)
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
                    SDL_Rect kick_btn = {start_x + card_width - 230, y + 35, 90, 40};
                    draw_rounded_rect(renderer, kick_btn, CLR_DANGER, 6);
                    draw_rounded_border(renderer, kick_btn, (SDL_Color){200, 50, 50, 255}, 6, 1);
                    
                    SDL_Surface *kick_surf = TTF_RenderText_Blended(font, "Kick", CLR_WHITE);
                    if (kick_surf) {
                        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, kick_surf);
                        SDL_Rect rect = {
                            kick_btn.x + (kick_btn.w - kick_surf->w) / 2,
                            kick_btn.y + (kick_btn.h - kick_surf->h) / 2,
                            kick_surf->w, kick_surf->h
                        };
                        SDL_RenderCopy(renderer, tex, NULL, &rect);
                        SDL_DestroyTexture(tex);
                        SDL_FreeSurface(kick_surf);
                    }
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
        y += card_height + 20;  // 20px spacing (was 15)
    }
    
    // Vẽ các nút ở dưới cùng - CÂN GIỮA
    int button_y = win_h - 120;
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
    
    // LARGER Dialog box - 700x550 (was 500x400)
    int dialog_w = 700;
    int dialog_h = 550;
    int dialog_x = (win_w - dialog_w) / 2;
    int dialog_y = (win_h - dialog_h) / 2;
    
    SDL_Rect dialog = {dialog_x, dialog_y, dialog_w, dialog_h};
    draw_rounded_rect(renderer, dialog, CLR_INPUT_BG, 12);
    draw_rounded_border(renderer, dialog, CLR_PRIMARY, 12, 2);
    
    // Title - LARGER
    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface *title = TTF_RenderText_Blended(font, "Create Room", CLR_ACCENT);
    if (title) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, title);
        SDL_Rect rect = {dialog_x + (dialog_w - title->w)/2, dialog_y + 30, title->w, title->h};
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(title);
    }
    
    // Input fields - positions already set in main.c, but need to be relative to dialog
    // Room name input - LARGER 550x60
    room_name->rect.x = dialog_x + 75;
    room_name->rect.y = dialog_y + 120;
    room_name->rect.w = dialog_w - 150;
    room_name->rect.h = 60;
    
    draw_input_field(renderer, font, room_name);
    
    // Access code field - LARGER
    if (access_code) {
        access_code->rect.x = dialog_x + 75;
        access_code->rect.y = dialog_y + 230;
        access_code->rect.w = dialog_w - 300;  // Make room for random button
        access_code->rect.h = 60;
        
        draw_input_field(renderer, font, access_code);
        
        // Helper text - smaller font would be better but we'll use existing
        SDL_Surface *helper_surf = TTF_RenderText_Blended(font, "6-digit code (leave empty for public)", 
                                                          CLR_GRAY);
        if (helper_surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, helper_surf);
            SDL_Rect rect = {dialog_x + 75, dialog_y + 300, helper_surf->w, helper_surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(helper_surf);
        }
        
        // Random button - LARGER 120x60
        SDL_Rect btn_random = {access_code->rect.x + access_code->rect.w + 15, 
                               access_code->rect.y, 120, 60};
       
        // Shadow
        draw_layered_shadow(renderer, btn_random, UI_CORNER_RADIUS, 3);
        
        // Button gradient
        draw_vertical_gradient(renderer, btn_random, CLR_ACCENT, CLR_ACCENT_DK);
        draw_rounded_border(renderer, btn_random, CLR_ACCENT_DK, UI_CORNER_RADIUS, 1);
        
        // Button text
        SDL_Surface *btn_surf = TTF_RenderText_Blended(font, "Random", white);
        if (btn_surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, btn_surf);
            SDL_Rect text_rect = {
                btn_random.x + (btn_random.w - btn_surf->w) / 2,
                btn_random.y + (btn_random.h - btn_surf->h) / 2,
                btn_surf->w, btn_surf->h
            };
            SDL_RenderCopy(renderer, tex, NULL, &text_rect);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(btn_surf);
        }
    }
    
    // Buttons - LARGER 220x60 (was 180x50) - ONLY SET POSITION, don't draw here
    create_btn->rect = (SDL_Rect){dialog_x + 120, dialog_y + dialog_h - 100, 220, 60};
    cancel_btn->rect = (SDL_Rect){dialog_x + dialog_w - 340, dialog_y + dialog_h - 100, 220, 60};
    
    strcpy(create_btn->text, "Create");
    strcpy(cancel_btn->text, "Cancel");
    
    // NOTE: Buttons and input fields are drawn by main.c after this function returns
}
