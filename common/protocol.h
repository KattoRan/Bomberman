#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#define PORT 8081
#define MAX_CLIENTS 4
#define MAX_LOBBIES 10
#define MAX_USERNAME 32
#define MAX_SPECTATORS 4
#define MAX_PASSWORD 128
#define MAX_ROOM_NAME 64
#define MAX_EMAIL 128
#define MAX_DISPLAY_NAME 64

// Map config
#define MAP_WIDTH 15
#define MAP_HEIGHT 13

// Game modes
#define GAME_MODE_CLASSIC 0
#define GAME_MODE_SUDDEN_DEATH 1
#define GAME_MODE_FOG_OF_WAR 2

// Tile types
#define EMPTY 0
#define WALL_HARD 1
#define WALL_SOFT 2
#define BOMB 3
#define EXPLOSION 4
#define POWERUP_BOMB 5      // Tăng số bom
#define POWERUP_FIRE 6      // Tăng tầm nổ
#define POWERUP_FIRE 6      // Tăng tầm nổ

#define MOVE_UP 0
#define MOVE_DOWN 1
#define MOVE_LEFT 2
#define MOVE_RIGHT 3

// Message types
#define MSG_REGISTER 1
#define MSG_LOGIN 2
#define MSG_CREATE_LOBBY 3
#define MSG_JOIN_LOBBY 4
#define MSG_LIST_LOBBIES 5
#define MSG_LEAVE_LOBBY 6
#define MSG_START_GAME 7
#define MSG_GAME_STATE 8
#define MSG_MOVE 9
#define MSG_PLANT_BOMB 10
#define MSG_LEAVE_GAME 11
#define MSG_FRIEND_REQUEST 12
#define MSG_FRIEND_ACCEPT 13
#define MSG_FRIEND_DECLINE 14
#define MSG_FRIEND_REMOVE 15
#define MSG_FRIEND_LIST 16
#define MSG_FRIEND_LIST_RESPONSE 17
#define MSG_FRIEND_RESPONSE 18
#define MSG_LOBBY_UPDATE 19
#define MSG_LOBBY_LIST 20
#define MSG_READY 21
#define MSG_CHAT 22
#define MSG_GET_PROFILE 23
#define MSG_PROFILE_RESPONSE 24
#define MSG_GET_LEADERBOARD 25
#define MSG_LEADERBOARD_RESPONSE 26
#define MSG_NOTIFICATION 27
#define MSG_ERROR 28
#define MSG_FRIEND_INVITE 30
#define MSG_INVITE_RECEIVED 31
#define MSG_INVITE_RESPONSE 32
#define MSG_AUTH_RESPONSE 33
#define MSG_RECONNECT 34
#define MSG_LOGIN_WITH_TOKEN 35
#define MSG_SPECTATE 36
#define MSG_UPDATE_PROFILE 37
#define MSG_KICK_PLAYER 38
#define MSG_SET_ROOM_PRIVATE 39

// Lobby status
#define LOBBY_WAITING 0
#define LOBBY_PLAYING 1

// Game status
#define GAME_WAITING 0
#define GAME_RUNNING 1
#define GAME_ENDED 2

// Response codes
#define AUTH_SUCCESS 0
#define AUTH_FAIL 1
#define AUTH_FAILED 1           // Alias for backward compatibility
#define AUTH_USER_EXISTS 2
#define AUTH_INVALID 3
#define AUTH_USERNAME_EXISTS 4
#define AUTH_EMAIL_EXISTS 5

// Server response codes (errors)
#define ERR_LOBBY_NOT_FOUND -2
#define ERR_LOBBY_GAME_IN_PROGRESS -3
#define ERR_LOBBY_FULL -4
#define ERR_LOBBY_WRONG_ACCESS_CODE -5
#define ERR_LOBBY_LOCKED -6
#define ERR_LOBBY_DUPLICATE_USER -7  // User already in lobby
#define ERR_NOT_HOST 3
#define ERR_NOT_ENOUGH_PLAYERS 4
#define ERR_ALREADY_IN_LOBBY 5

// Player structure - WITH POWER-UPS AND ELO
typedef struct {
    int id;
    int x, y;
    int is_alive;
    int is_ready;
    char username[MAX_USERNAME];
    char display_name[MAX_DISPLAY_NAME];  // Mutable display name
    int elo_rating;                       // ELO ranking
    int max_bombs;                        // Max bombs can place
    int bomb_range;                       // Explosion range
    int current_bombs;                    // Current bombs placed
} Player;

// Friend info structure
typedef struct {
    int user_id;
    char display_name[MAX_DISPLAY_NAME];
    int elo_rating;
    int is_online;  // 0=offline, 1=online, 2=in-game
} FriendInfo;

// Profile/Statistics structure
typedef struct {
    char username[MAX_USERNAME];
    char display_name[MAX_DISPLAY_NAME];
    int elo_rating;
    int tier;  // 0=Bronze, 1=Silver, 2=Gold, 3=Diamond
    int total_matches;
    int wins;
    int total_kills;
    int deaths;
} ProfileData;

// Leaderboard entry
typedef struct {
    int rank;
    char display_name[MAX_DISPLAY_NAME];
    int elo_rating;
    int wins;
} LeaderboardEntry;

// Lobby structure
typedef struct {
    int id;
    char name[64];
    char host_username[MAX_USERNAME];
    int host_id;
    Player players[MAX_CLIENTS];
    int num_players;
    int spectator_count;
    char spectators[MAX_SPECTATORS][MAX_USERNAME];
    int status;                  // LOBBY_WAITING or LOBBY_PLAYING
    int is_private;              // 0 = public, 1 = private
    char access_code[8];         // 6-digit code (plus null terminator)
    int is_locked;               // 0 = unlocked, 1 = locked (no new joins)
    int game_mode;               // Game mode: 0=Classic, 1=Sudden Death, 2=Fog of War
} Lobby;

// Lightweight Lobby Summary for lists
typedef struct {
    int id;
    char name[MAX_ROOM_NAME];
    int num_players;
    int max_players;
    int spectator_count;
    int game_mode;
    int status;
    int is_private;
    int is_locked;
} LobbySummary;

// Game state
typedef struct {
    int map[MAP_HEIGHT][MAP_WIDTH];
    Player players[MAX_CLIENTS];
    int num_players;
    int game_status;
    int winner_id;
    int kills[MAX_CLIENTS];  // Kill count per player this match
    int elo_changes[MAX_CLIENTS];  // ELO change for each player (+/-)
    long long match_start_time;  // Unix timestamp when match started
    int match_duration_seconds;  // Match duration in seconds
    long long end_game_time;
    int game_mode;               // Active game mode (0=Classic, 1=Sudden Death, 2=Fog of War)
    int fog_radius;              // Fog of war visibility radius in tiles (5 = default)
    int sudden_death_timer;      // 1800 ticks (90 seconds at 20 ticks/sec)
    int shrink_zone_left;        // Safe zone left boundary
    int shrink_zone_right;       // Safe zone right boundary
    int shrink_zone_top;         // Safe zone top boundary
    int shrink_zone_bottom;      // Safe zone bottom boundary
    long long start_game_time;   // Server timestamp when game started
} GameState;

// Client packet - ENHANCED
typedef struct {
    int type;
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
    char email[MAX_EMAIL];             // For registration
    char display_name[MAX_DISPLAY_NAME]; // For profile updates, etc.
    
    // For friend operations
    char target_display_name[MAX_DISPLAY_NAME]; // For friend requests
    int target_user_id;  // For accept/decline/remove friend
    
    // For lobby
    int lobby_id;
    char room_name[MAX_ROOM_NAME];     // For creating lobby
    int data;                          // Multi-purpose: direction, player_id, etc.
    char access_code[8];               // For joining private rooms
    int is_private;                    // For creating private rooms
    int target_player_id;              // For kick, spectator view
    int game_mode;                     // For room creation: game mode selection
    char chat_message[200];            // For chat messages
    char session_token[64];            // For reconnection and auto-login
} ClientPacket;

// Server packet - ENHANCED
typedef struct {
    int type;
    int code;
    char message[256];
    union {
        struct {
            int user_id;
            char username[MAX_USERNAME];
            char display_name[MAX_DISPLAY_NAME];
            int elo_rating;
            char session_token[64];
        } auth;
        struct {
            LobbySummary lobbies[MAX_LOBBIES];
            int count;
        } lobby_list;
        struct {
            FriendInfo friends[50];
            int count;
        } friend_list;
        struct {
            LeaderboardEntry entries[100];
            int count;
        } leaderboard;
        Lobby lobby;
        GameState game_state;
        ProfileData profile;
        struct {
            char sender_username[MAX_USERNAME];
            char message[200];
            uint32_t timestamp;
            int player_id;  // For color coding
        } chat_msg;
        struct {
            int lobby_id;
            char room_name[MAX_ROOM_NAME];
            char host_name[MAX_USERNAME];
            char access_code[8];  // For password bypass
            int game_mode;
        } invite;
    } payload;
} ServerPacket;

#endif
