/* server/server.h */
#ifndef SERVER_H
#define SERVER_H

#include "../common/protocol.h"

#define MAX_USERS 1000

// --- Định nghĩa Struct User (Database) ---
typedef struct {
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
    int is_online;
    int lobby_id;
} User;

// --- Database Functions ---
void load_users();
void save_users();
int register_user(const char *username, const char *password);
int login_user(const char *username, const char *password);
void logout_user(const char *username);
User* find_user(const char *username);

// --- Lobby Functions ---
void init_lobbies();
int create_lobby(const char *room_name, const char *host_username);
int join_lobby(int lobby_id, const char *username);
int leave_lobby(int lobby_id, const char *username);
int toggle_ready(int lobby_id, const char *username);
int start_game(int lobby_id, const char *username);
int get_lobby_list(Lobby *out_lobbies);
Lobby* find_lobby(int lobby_id);
int find_user_lobby(const char *username);

// --- Game Logic Functions ---
void init_game(GameState *state, Lobby *lobby);
void update_game(GameState *state);
int handle_move(GameState *state, int player_id, int direction);
int plant_bomb(GameState *state, int player_id);

// --- Network Functions ---
int init_server_socket();

#endif