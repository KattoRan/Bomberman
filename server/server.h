/* server/server.h */
#ifndef SERVER_H
#define SERVER_H

#include <time.h>
#include "../common/protocol.h"

#define MAX_USERS 10000
#define MAX_EMAIL 128
#define MAX_DISPLAY_NAME 64

#define MAX_LOBBIES 10

// --- Structures ---
typedef struct {
    int socket_fd;
    int user_id;                      // Database user ID
    char username[MAX_USERNAME];
    char display_name[MAX_DISPLAY_NAME];
    int is_authenticated;
    int lobby_id;
    int player_id_in_game; 
    char session_token[64];
    time_t last_active;
} ClientInfo;

#define MAX_CHAT_HISTORY 50
typedef struct {
    char sender_username[MAX_USERNAME];
    char message[200];
    uint32_t timestamp;
    int player_id;
} ChatHistoryEntry;

typedef struct {
    ChatHistoryEntry messages[MAX_CHAT_HISTORY];
    int count;
} LobbyChat;

// --- Global State (Defined in main.c or specialized state file) ---
extern ClientInfo clients[MAX_CLIENTS * MAX_LOBBIES];
extern int num_clients;
extern LobbyChat lobby_chats[MAX_LOBBIES];
extern GameState active_games[MAX_LOBBIES];
extern long long last_game_update[MAX_LOBBIES];

// --- Helper Functions in main.c ---
ClientInfo* find_client_by_socket(int socket_fd);
void send_response(int socket_fd, ServerPacket *packet);
void broadcast_lobby_list();
void broadcast_lobby_update(int lobby_id);
void broadcast_game_state(int lobby_id);
void log_event(const char *category, const char *format, ...);
void generate_session_token(char *buffer, size_t length);
long long get_current_time_ms();

// --- Enhanced User Struct (Database) ---
typedef struct {
    int id;                                  // Primary key from database
    char username[MAX_USERNAME];             // Immutable unique identifier
    char display_name[MAX_DISPLAY_NAME];     // Mutable display name
    char email[MAX_EMAIL];                   // For login and verification
    int elo_rating;                          // ELO ranking
    char session_token[64];                  // Session token
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
int db_update_session_token(int user_id, const char *token);
int db_get_user_by_token(const char *token, User *out_user);

// --- Lobby Functions ---
void init_lobbies();
int create_lobby(const char *room_name, const char *host_username, int is_private, const char *access_code, int game_mode);
int join_lobby(int lobby_id, const char *username);
int join_lobby_with_code(int lobby_id, const char *username, const char *access_code);
int leave_lobby(int lobby_id, const char *username);
int toggle_ready(int lobby_id, const char *username);
int start_game(int lobby_id, const char *username);
int get_lobby_list(Lobby *out_lobbies);
Lobby* find_lobby(int lobby_id);
int find_user_lobby(const char *username);
int join_spectator(int lobby_id, const char *username);
int leave_spectator(int lobby_id, const char *username);

// --- Game Logic Functions ---
void init_game(GameState *state, Lobby *lobby);
void update_game(GameState *state);
int handle_move(GameState *state, int player_id, int direction);
int plant_bomb(GameState *state, int player_id);
int is_tile_visible(GameState *state, int player_id, int tile_x, int tile_y);
void filter_game_state(GameState *full_state, int player_id, GameState *out_filtered);

// --- Friend System Functions ---
int friend_send_request(int sender_id, const char *target_display_name);
int friend_accept_request(int user_id, int requester_id);
int friend_decline_request(int user_id, int requester_id);
int friend_remove(int user_id, int friend_id);
int friend_get_list(int user_id, FriendInfo *out_friends, int max_count);
int friend_get_pending_requests(int user_id, FriendInfo *out_requests, int max_count);
int friend_get_sent_requests(int user_id, FriendInfo *out_requests, int max_count);

// --- ELO System Functions ---
int get_k_factor(int matches_played);
int elo_calculate_change(int my_elo, int opp_elo, int win);
int elo_update_after_match(int *player_ids, int *placements, int num_players, int *out_elo_changes);
int get_tier(int elo_rating);
const char* get_tier_name(int tier);

// --- Statistics Functions ---
int stats_record_match(int *player_ids, int *placements, int *kills, int num_players, int winner_id, int duration_seconds);
int stats_get_profile(int user_id, ProfileData *out_profile);
int stats_get_leaderboard(LeaderboardEntry *out_entries, int max_count);
void stats_increment_bombs(int user_id);
void stats_increment_walls(int user_id, int count);

// --- Network Functions ---
// --- Network Functions ---
int init_server_socket();

// --- Packet Handlers ---
void handle_register(int socket_fd, ClientPacket *pkt);
void handle_login(int socket_fd, ClientPacket *pkt);
void handle_login_with_token(int socket_fd, ClientPacket *pkt);

void handle_create_lobby(int socket_fd, ClientPacket *pkt);
void handle_join_lobby(int socket_fd, ClientPacket *pkt);
void handle_leave_lobby(int socket_fd, ClientPacket *pkt);
void handle_list_lobbies(int socket_fd, ClientPacket *pkt);
void handle_spectate(int socket_fd, ClientPacket *pkt);
void handle_ready(int socket_fd, ClientPacket *pkt);
void handle_start_game(int socket_fd, ClientPacket *pkt);

void handle_game_move(int socket_fd, ClientPacket *pkt);
void handle_plant_bomb(int socket_fd, ClientPacket *pkt);
void handle_leave_game(int socket_fd, ClientPacket *pkt);
void forfeit_player_from_game(int lobby_id, const char *username);

void handle_chat(int socket_fd, ClientPacket *pkt);

void handle_friend_request(int socket_fd, ClientPacket *pkt);
void handle_friend_accept(int socket_fd, ClientPacket *pkt);
void handle_friend_reject(int socket_fd, ClientPacket *pkt);
void handle_friend_list(int socket_fd, ClientPacket *pkt);
void handle_get_profile(int socket_fd, ClientPacket *pkt);
void handle_get_leaderboard(int socket_fd, ClientPacket *pkt);
void handle_invite(int socket_fd, ClientPacket *pkt);

#endif