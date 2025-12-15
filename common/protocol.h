#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#define PORT 8081
#define MAX_CLIENTS 4
#define MAX_LOBBIES 10
#define MAX_USERNAME 32
#define MAX_PASSWORD 128
#define MAX_ROOM_NAME 64
#define MAX_EMAIL 128
#define MAX_DISPLAY_NAME 64

// Map config
#define MAP_WIDTH 15
#define MAP_HEIGHT 13

// Tile types
#define EMPTY 0
#define WALL_HARD 1
#define WALL_SOFT 2
#define BOMB 3
#define EXPLOSION 4
#define POWERUP_BOMB 5      // Tăng số bom
#define POWERUP_FIRE 6      // Tăng tầm nổ
#define POWERUP_SPEED 7     // Tăng tốc độ (dự phòng)

// Message types - Client to Server
#define MSG_REGISTER 1
#define MSG_LOGIN 2
#define MSG_CREATE_LOBBY 3
#define MSG_JOIN_LOBBY 4
#define MSG_LEAVE_LOBBY 5
#define MSG_LIST_LOBBIES 6
#define MSG_READY 7
#define MSG_START_GAME 8
#define MSG_MOVE 10
#define MSG_PLANT_BOMB 11
#define MSG_FRIEND_REQUEST 12
#define MSG_FRIEND_RESPONSE 13
#define MSG_FRIEND_LIST 14
#define MSG_GET_PROFILE 15
#define MSG_UPDATE_PROFILE 16
#define MSG_GET_LEADERBOARD 17
#define MSG_KICK_PLAYER 18
#define MSG_SET_ROOM_PRIVATE 19

// Message types - Server to Client
#define MSG_AUTH_RESPONSE 20
#define MSG_LOBBY_LIST 21
#define MSG_LOBBY_UPDATE 22
#define MSG_GAME_STATE 23
#define MSG_ERROR 24
#define MSG_FRIEND_LIST_RESPONSE 25
#define MSG_PROFILE_RESPONSE 26
#define MSG_LEADERBOARD_RESPONSE 27
#define MSG_NOTIFICATION 28

// Movement
#define MOVE_UP 0
#define MOVE_DOWN 1
#define MOVE_LEFT 2
#define MOVE_RIGHT 3

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

// Error codes
#define ERR_LOBBY_NOT_FOUND -1
#define ERR_LOBBY_FULL -2
#define ERR_LOBBY_GAME_IN_PROGRESS -3
#define ERR_LOBBY_LOCKED -4
#define ERR_LOBBY_WRONG_ACCESS_CODE -5
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
    int status;                  // LOBBY_WAITING or LOBBY_PLAYING
    int is_private;              // 0 = public, 1 = private
    char access_code[8];         // 6-digit code (plus null terminator)
    int is_locked;               // 0 = unlocked, 1 = locked (no new joins)
} Lobby;

// Game state
typedef struct {
    int map[MAP_HEIGHT][MAP_WIDTH];
    Player players[MAX_CLIENTS];
    int num_players;
    int game_status;
    int winner_id;
    long long end_game_time;
} GameState;

// Client packet - ENHANCED
typedef struct {
    int type;
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
    char email[MAX_EMAIL];             // For registration
    char room_name[MAX_ROOM_NAME];
    int lobby_id;
    int data;                          // Multi-purpose: direction, player_id, etc.
    char access_code[8];               // For joining private rooms
    int is_private;                    // For creating private rooms
    char target_display_name[MAX_DISPLAY_NAME]; // For friend requests
    int target_player_id;              // For kick, spectator view
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
        } auth;
        struct {
            Lobby lobbies[MAX_LOBBIES];
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
    } payload;
} ServerPacket;

#endif