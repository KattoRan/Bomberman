#include "ui.h"
#include "../graphics/graphics.h"
#include <string.h>
#include <math.h>
#include <stdio.h>

#define UI_GRID_SIZE 40
#define UI_CORNER_RADIUS 8      
#define UI_SHADOW_OFFSET 4      

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

/**
 * Check if mouse coordinates are inside a rectangle
 */
int is_mouse_inside(SDL_Rect rect, int mx, int my) {
    return (mx >= rect.x && mx <= rect.x + rect.w && my >= rect.y && my <= rect.y + rect.h);
}

/**
 * Handle text input for an input field (character input and backspace)
 */
void handle_text_input(InputField *field, char c) {
    if (!field->is_active) return;
    
    int len = strlen(field->text);
    
    if (c == '\b') {  // Backspace
        if (len > 0) {
            field->text[len - 1] = '\0';
        }
    } else if (len < field->max_length) {  // Regular character
        field->text[len] = c;
        field->text[len + 1] = '\0';
    }
}

/**
 * Render friend delete confirmation dialog
 * Uses global state: delete_friend_index, friends_list, friends_count
 * Declared in client_state.h
 */
extern int delete_friend_index;
extern FriendInfo friends_list[];
extern int friends_count;

void render_friend_delete_dialog(SDL_Renderer *renderer, TTF_Font *font_small) {
    if (delete_friend_index < 0 || delete_friend_index >= friends_count) {
        return;
    }
    
    int win_w, win_h;
    SDL_GetRendererOutputSize(renderer, &win_w, &win_h);
    
    // Semi-transparent overlay
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_Rect overlay = {0, 0, win_w, win_h};
    SDL_RenderFillRect(renderer, &overlay);
    
    // Modern dialog box with rounded corners
    SDL_Rect dialog = {250, 250, 300, 150};
    
    // Layered shadow
    draw_layered_shadow(renderer, dialog, 12, 6);
    
    // Dialog background
    SDL_Color dialog_bg = {30, 41, 59, 255};
    draw_rounded_rect(renderer, dialog, dialog_bg, 12);
    
    // Dialog border
    SDL_Color border_col = {59, 130, 246, 255};
    draw_rounded_border(renderer, dialog, border_col, 12, 2);
    
    // Title
    SDL_Surface *surf = TTF_RenderText_Blended(font_small, "Delete Friend?", (SDL_Color){255, 255, 255, 255});
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect r = {dialog.x + (dialog.w - surf->w)/2, dialog.y + 20, surf->w, surf->h};
        SDL_RenderCopy(renderer, tex, NULL, &r);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }
    
    // Friend name
    char msg[128];
    snprintf(msg, sizeof(msg), "Remove %s?", friends_list[delete_friend_index].display_name);
    surf = TTF_RenderText_Blended(font_small, msg, (SDL_Color){200, 200, 200, 255});
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect r = {dialog.x + (dialog.w - surf->w)/2, dialog.y + 60, surf->w, surf->h};
        SDL_RenderCopy(renderer, tex, NULL, &r);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }
    
    // Modern Yes button (rounded, red with shadow)
    SDL_Rect yes_btn = {300, 350, 100, 40};
    draw_layered_shadow(renderer, yes_btn, 6, 3);
    SDL_Color yes_bg = {239, 68, 68, 255};
    draw_rounded_rect(renderer, yes_btn, yes_bg, 6);
    
    surf = TTF_RenderText_Blended(font_small, "Yes", (SDL_Color){255, 255, 255, 255});
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect r = {yes_btn.x + (yes_btn.w - surf->w)/2, yes_btn.y + (yes_btn.h - surf->h)/2, surf->w, surf->h};
        SDL_RenderCopy(renderer, tex, NULL, &r);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }
    
    // Modern No button (rounded, gray with shadow)
    SDL_Rect no_btn = {420, 350, 100, 40};
    draw_layered_shadow(renderer, no_btn, 6, 3);
    SDL_Color no_bg = {100, 116, 139, 255};
    draw_rounded_rect(renderer, no_btn, no_bg, 6);
    
    surf = TTF_RenderText_Blended(font_small, "No", (SDL_Color){255, 255, 255, 255});
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect r = {no_btn.x + (no_btn.w - surf->w)/2, no_btn.y + (no_btn.h - surf->h)/2, surf->w, surf->h};
        SDL_RenderCopy(renderer, tex, NULL, &r);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }
    
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}
// Vẽ Button với gradient, rounded corners và shadow
void draw_button_text(SDL_Renderer *renderer, TTF_Font *font, Button *btn, SDL_Color color) {
    if (!font) return;

    char text[64];
    strncpy(text, btn->text, sizeof(text) - 1);
    text[sizeof(text) - 1] = '\0';

    truncate_text_to_fit(text, sizeof(text), font, btn->rect.w - 20);

    // Shadow
    SDL_Surface *shadow = TTF_RenderText_Blended(font, text, (SDL_Color){0,0,0,150});
    SDL_Surface *surf   = TTF_RenderText_Blended(font, text, color);

    if (shadow && surf) {
        SDL_Texture *shadow_tex = SDL_CreateTextureFromSurface(renderer, shadow);
        SDL_Texture *tex        = SDL_CreateTextureFromSurface(renderer, surf);

        SDL_Rect dst = {
            btn->rect.x + (btn->rect.w - surf->w) / 2,
            btn->rect.y + (btn->rect.h - surf->h) / 2,
            surf->w, surf->h
        };

        SDL_Rect shadow_dst = dst;
        shadow_dst.x += 1;
        shadow_dst.y += 1;

        SDL_RenderCopy(renderer, shadow_tex, NULL, &shadow_dst);
        SDL_RenderCopy(renderer, tex, NULL, &dst);

        SDL_DestroyTexture(shadow_tex);
        SDL_DestroyTexture(tex);
    }

    SDL_FreeSurface(shadow);
    SDL_FreeSurface(surf);
}

void draw_button_primary(SDL_Renderer *renderer, TTF_Font *font, Button *btn) {
    SDL_Color top, bottom;

    if (btn->is_hovered) {
        top = CLR_PRIMARY_LIGHT;
        bottom = CLR_BTN_HOVER;
    } else {
        top = CLR_BTN_NORM;
        bottom = CLR_PRIMARY_DK;
    }

    draw_layered_shadow(renderer, btn->rect, UI_CORNER_RADIUS, UI_SHADOW_OFFSET);
    draw_vertical_gradient(renderer, btn->rect, top, bottom);
    draw_rounded_border(renderer, btn->rect, bottom, UI_CORNER_RADIUS, 1);

    draw_button_text(renderer, font, btn, CLR_WHITE);
}
void draw_button_danger(SDL_Renderer *renderer, TTF_Font *font, Button *btn) {
    SDL_Color top, bottom;

    if (btn->is_hovered) {
        top = (SDL_Color){255, 120, 120, 255};
        bottom = CLR_DANGER;
    } else {
        top = CLR_DANGER;
        bottom = (SDL_Color){185, 28, 28, 255};
    }

    draw_layered_shadow(renderer, btn->rect, UI_CORNER_RADIUS, UI_SHADOW_OFFSET);
    draw_vertical_gradient(renderer, btn->rect, top, bottom);
    draw_rounded_border(renderer, btn->rect, bottom, UI_CORNER_RADIUS, 1);

    // Glow đỏ khi hover
    if (btn->is_hovered) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        for (int i = 1; i <= 3; i++) {
            SDL_Rect glow = {
                btn->rect.x - i,
                btn->rect.y - i,
                btn->rect.w + i * 2,
                btn->rect.h + i * 2
            };
            SDL_Color glow_color = {255, 80, 80, 60 / i};
            draw_rounded_border(renderer, glow, glow_color, UI_CORNER_RADIUS + i, 1);
        }
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }

    draw_button_text(renderer, font, btn, CLR_WHITE);
}
void draw_button_outline(SDL_Renderer *renderer, TTF_Font *font, Button *btn) {
    SDL_Color border = btn->is_hovered ? CLR_PRIMARY : CLR_GRAY;
    SDL_Color text   = btn->is_hovered ? CLR_WHITE : CLR_GRAY_LIGHT;

    // Shadow nhẹ
    draw_layered_shadow(renderer, btn->rect, UI_CORNER_RADIUS, 1);

    // Fill khi hover
    if (btn->is_hovered) {
        draw_vertical_gradient(
            renderer,
            btn->rect,
            CLR_PRIMARY_LIGHT,
            CLR_PRIMARY_DK
        );
    }

    // Border
    draw_rounded_border(renderer, btn->rect, border, UI_CORNER_RADIUS, 2);

    draw_button_text(renderer, font, btn, text);
}

void draw_button(SDL_Renderer *renderer, TTF_Font *font, Button *btn) {
    switch (btn->type) {
        case BTN_DANGER:
            draw_button_danger(renderer, font, btn);
            break;
        case BTN_OUTLINE:
            draw_button_outline(renderer, font, btn);
            break;
        default:
            draw_button_primary(renderer, font, btn);
            break;
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
    
    SDL_SetRenderDrawColor(renderer, CLR_ACCENT_LIGHT.r, CLR_ACCENT_LIGHT.g, CLR_ACCENT_LIGHT.b, grid_alpha);
    for (int i = 0; i < w; i += UI_GRID_SIZE) {
        SDL_RenderDrawLine(renderer, i, 0, i, h);
    }
    for (int i = 0; i < h; i += UI_GRID_SIZE) {
        SDL_RenderDrawLine(renderer, 0, i, w, i);
    }
    
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}
