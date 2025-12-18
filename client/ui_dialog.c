/* Room creation dialog rendering - add to ui_screens.c */

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
    
    // Buttons
    create_btn->rect = (SDL_Rect){dialog_x + 50, dialog_y + dialog_h - 80, 180, 50};
    cancel_btn->rect = (SDL_Rect){dialog_x + dialog_w - 230, dialog_y + dialog_h - 80, 180, 50};
    
    strcpy(create_btn->text, "Create");
    strcpy(cancel_btn->text, "Cancel");
}
