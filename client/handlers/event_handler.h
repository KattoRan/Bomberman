/* client/handlers/event_handler.h */
#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H

#include <SDL2/SDL.h>

// Function to handle SDL events based on current screen
void handle_events(SDL_Event *e, int mx, int my, SDL_Renderer *rend);

#endif