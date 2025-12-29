#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "../common/protocol.h"

#define TILE_SIZE 50
#define WINDOW_WIDTH (MAP_WIDTH * TILE_SIZE)
#define WINDOW_HEIGHT (MAP_HEIGHT * TILE_SIZE + 70)
#define MAX_NOTIFICATIONS 5
#define MAX_PARTICLES 200

extern GameState current_state;
extern Lobby current_lobby;
extern char my_username[MAX_USERNAME];

// effects
void init_particles();
void add_particle(float, float, float, float, SDL_Color, int, float);
void update_particles();
void render_particles(SDL_Renderer*);
float ease_in_out_cubic(float);
float ease_out_bounce(float);
void draw_vertical_gradient(SDL_Renderer*, SDL_Rect, SDL_Color, SDL_Color);
void draw_horizontal_gradient(SDL_Renderer*, SDL_Rect, SDL_Color, SDL_Color);
void draw_glow_circle(SDL_Renderer*, int, int, int, SDL_Color, int);

// map
void draw_tile(SDL_Renderer*, int, int, SDL_Color, int);

// entity
void draw_bomb(SDL_Renderer*, int, int, int);
void draw_explosion(SDL_Renderer*, int, int, int);
void draw_powerup(SDL_Renderer*, int, int, int, int);
void draw_player(SDL_Renderer*, Player*, SDL_Color);

// hud
void add_notification(const char*, SDL_Color);
void draw_notifications(SDL_Renderer*, TTF_Font*);
void draw_status_bar(SDL_Renderer*, TTF_Font*, int);
SDL_Rect get_game_leave_button_rect();

// overlay
void draw_fog_overlay(SDL_Renderer*, GameState*, int);

// main render
void render_game(SDL_Renderer*, TTF_Font*, int, int, int);

// font
TTF_Font* init_font();
