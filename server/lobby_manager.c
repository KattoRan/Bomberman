#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../common/protocol.h"

Lobby lobbies[MAX_LOBBIES];

void init_lobbies() {
    for (int i = 0; i < MAX_LOBBIES; i++) {
        lobbies[i].id = -1;
        lobbies[i].num_players = 0;
        lobbies[i].status = LOBBY_WAITING;
    }
    printf("[LOBBY] System initialized\n");
}

Lobby* find_lobby(int lobby_id) {
    for (int i = 0; i < MAX_LOBBIES; i++) {
        if (lobbies[i].id == lobby_id) {
            return &lobbies[i];
        }
    }
    return NULL;
}

// Create a new lobby
int create_lobby(const char *room_name, const char *host_username, int is_private, const char *access_code) {
    int slot = -1;
    for (int i = 0; i < MAX_LOBBIES; i++) {
        if (lobbies[i].id == -1) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        printf("[LOBBY] No slots available\n");
        return -1;
    }
    
    Lobby *lobby = &lobbies[slot];
    lobby->id = slot;  // Use slot index as ID for consistency
    strncpy(lobby->name, room_name, MAX_ROOM_NAME - 1);
    lobby->name[MAX_ROOM_NAME - 1] = '\0';
    lobby->num_players = 1;
    lobby->host_id = 0;
    lobby->status = LOBBY_WAITING;
    lobby->is_private = is_private;
    lobby->is_locked = 0;
    
    // Set access code for private rooms
    if (is_private && access_code) {
        strncpy(lobby->access_code, access_code, 7);
        lobby->access_code[7] = '\0';
    } else {
        lobby->access_code[0] = '\0';
    }
    
    // Store host username for display
    strncpy(lobby->host_username, host_username, MAX_USERNAME - 1);
    lobby->host_username[MAX_USERNAME - 1] = '\0';
    
    Player *host = &lobby->players[0];
    host->id = 0;
    strncpy(host->username, host_username, MAX_USERNAME - 1);
    host->username[MAX_USERNAME - 1] = '\0';
    host->is_ready = 1;
    host->is_alive = 0;
    
    printf("[LOBBY] Created: '%s' (ID:%d) by %s\n", 
           room_name, lobby->id, host_username);
    return lobby->id;
}

// Join an existing lobby with optional access code
int join_lobby_with_code(int lobby_id, const char *username, const char *access_code) {
    Lobby *lobby = find_lobby(lobby_id);
    
    if (!lobby) return ERR_LOBBY_NOT_FOUND;
    if (lobby->num_players >= MAX_CLIENTS) return ERR_LOBBY_FULL;
    if (lobby->status != LOBBY_WAITING) return ERR_LOBBY_GAME_IN_PROGRESS; // Game in progress
    if (lobby->is_locked) return ERR_LOBBY_LOCKED; // Room is locked
    
    // Check access code for private rooms
    if (lobby->is_private) {
        if (!access_code || strcmp(lobby->access_code, access_code) != 0) {
            printf("[LOBBY] Access denied to private room %d (wrong code)\n", lobby_id);
            return ERR_LOBBY_WRONG_ACCESS_CODE; // Wrong access code
        }
    }
    
    Player *player = &lobby->players[lobby->num_players];
    player->id = lobby->num_players;
    strncpy(player->username, username, MAX_USERNAME - 1);
    player->username[MAX_USERNAME - 1] = '\0';
    player->is_ready = 0;
    player->is_alive = 0;
    
    lobby->num_players++;
    printf("[LOBBY] %s joined lobby %d (%d/%d)\n", 
           username, lobby_id, lobby->num_players, MAX_CLIENTS);
    return 0;
}

// Wrapper for backwards compatibility
int join_lobby(int lobby_id, const char *username) {
    return join_lobby_with_code(lobby_id, username, NULL);
}

int leave_lobby(int lobby_id, const char *username) {
    Lobby *lobby = find_lobby(lobby_id);
    if (!lobby) return ERR_LOBBY_NOT_FOUND;
    
    int player_idx = -1;
    for (int i = 0; i < lobby->num_players; i++) {
        if (strcmp(lobby->players[i].username, username) == 0) {
            player_idx = i;
            break;
        }
    }
    
    if (player_idx == -1) return ERR_LOBBY_NOT_FOUND;
    
    // Shift array
    for (int i = player_idx; i < lobby->num_players - 1; i++) {
        lobby->players[i] = lobby->players[i + 1];
        lobby->players[i].id = i;
    }
    lobby->num_players--;
    
    printf("[LOBBY] %s left lobby %d\n", username, lobby_id);
    
    // Transfer host
    if (player_idx == lobby->host_id && lobby->num_players > 0) {
        lobby->host_id = 0;
        lobby->players[0].is_ready = 1;
        printf("[LOBBY] New host: %s\n", lobby->players[0].username);
    }
    
    // Delete if empty
    if (lobby->num_players == 0) {
        lobby->id = -1;
        printf("[LOBBY] Deleted lobby %d (empty)\n", lobby_id);
    }
    
    return 0;
}

int toggle_ready(int lobby_id, const char *username) {
    Lobby *lobby = find_lobby(lobby_id);
    if (!lobby) return ERR_LOBBY_NOT_FOUND;
    
    for (int i = 0; i < lobby->num_players; i++) {
        if (strcmp(lobby->players[i].username, username) == 0) {
            if (i != lobby->host_id) {
                lobby->players[i].is_ready = !lobby->players[i].is_ready;
                printf("[LOBBY] %s ready: %d\n", 
                       username, lobby->players[i].is_ready);
            }
            return 0;
        }
    }
    return ERR_LOBBY_NOT_FOUND;
}

int can_start_game(int lobby_id) {
    Lobby *lobby = find_lobby(lobby_id);
    if (!lobby) return 0;
    if (lobby->num_players < 2) return 0;
    
    for (int i = 0; i < lobby->num_players; i++) {
        if (!lobby->players[i].is_ready) return 0;
    }
    return 1;
}

int start_game(int lobby_id, const char *username) {
    Lobby *lobby = find_lobby(lobby_id);
    if (!lobby) return ERR_LOBBY_NOT_FOUND;
    
    // Check host
    if (strcmp(lobby->players[lobby->host_id].username, username) != 0) {
        return ERR_NOT_HOST;
    }
    
    if (!can_start_game(lobby_id)) {
        return ERR_NOT_ENOUGH_PLAYERS;
    }
    
    lobby->status = LOBBY_PLAYING;
    printf("[LOBBY] Game started in lobby %d\n", lobby_id);
    return 0;
}

int get_lobby_list(Lobby *out_lobbies) {
    int count = 0;
    for (int i = 0; i < MAX_LOBBIES; i++) {
        if (lobbies[i].id != -1 && lobbies[i].status != LOBBY_PLAYING) {
            out_lobbies[count++] = lobbies[i];
        }
    }
    return count;
}

int find_user_lobby(const char *username) {
    for (int i = 0; i < MAX_LOBBIES; i++) {
        if (lobbies[i].id == -1) continue;
        for (int j = 0; j < lobbies[i].num_players; j++) {
            if (strcmp(lobbies[i].players[j].username, username) == 0) {
                return lobbies[i].id;
            }
        }
    }
    return -1;
}