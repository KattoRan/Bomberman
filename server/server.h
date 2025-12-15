/* server/server.h */
#ifndef SERVER_H
#define SERVER_H

#include "../common/protocol.h"

#define MAX_USERS 10000
#define MAX_EMAIL 128
#define MAX_DISPLAY_NAME 64

// --- Enhanced User Struct (Database) ---
typedef struct {
    int id;                                  // Primary key from database
    char username[MAX_USERNAME];             // Immutable unique identifier
    char display_name[MAX_DISPLAY_NAME];     // Mutable display name
    char email[MAX_EMAIL];                   // For login and verification
    int elo_rating;                          // ELO ranking
    int is_online;                           // Current online status
    int lobby_id;                            // Current lobby (-1 if none)
} User;

// --- Database Functions (SQLite3) ---
int db_init();                               // Initialize SQLite database
void db_close();                             // Close database connection
int db_register_user(const char *username, const char *email, const char *password);
int db_login_user(const char *identifier, const char *password, User *out_user);
int db_update_display_name(int user_id, const char *new_display_name);
int db_get_user_by_id(int user_id, User *out_user);
int db_find_user_by_display_name(const char *display_name, User *out_user);
int db_update_elo(int user_id, int new_elo);

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

// --- Friend System Functions ---
int friend_send_request(int sender_id, const char *target_display_name);
int friend_accept_request(int user_id, int requester_id);
int friend_decline_request(int user_id, int requester_id);
int friend_remove(int user_id, int friend_id);
int friend_get_list(int user_id, FriendInfo *out_friends, int max_count);
int friend_get_pending_requests(int user_id, FriendInfo *out_requests, int max_count);

// --- ELO System Functions ---
int get_k_factor(int matches_played);
int calculate_elo_change(int player_rating, int avg_opponent_rating, int placement, int matches_played);
int elo_update_after_match(int *player_ids, int *placements, int num_players);
int get_tier(int elo_rating);
const char* get_tier_name(int tier);

// --- Statistics Functions ---
int stats_record_match(int *player_ids, int *placements, int *kills, int num_players, int winner_id, int duration_seconds);
int stats_get_profile(int user_id, ProfileData *out_profile);
int stats_get_leaderboard(LeaderboardEntry *out_entries, int max_count);
void stats_increment_bombs(int user_id);
void stats_increment_walls(int user_id, int count);

// --- Network Functions ---
int init_server_socket();

#endif