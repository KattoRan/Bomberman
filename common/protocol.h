#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#define PORT 8081
#define MAX_CLIENTS 4
#define MAX_LOBBIES 10
#define MAX_USERNAME 32
#define MAX_PASSWORD 128
#define MAX_ROOM_NAME 64

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

// Message types - Server to Client
#define MSG_AUTH_RESPONSE 20
#define MSG_LOBBY_LIST 21
#define MSG_LOBBY_UPDATE 22
#define MSG_GAME_STATE 23
#define MSG_ERROR 24

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
#define AUTH_FAILED 1
#define AUTH_USER_EXISTS 2
#define AUTH_INVALID 3

// Error codes
#define ERR_LOBBY_FULL 1
#define ERR_LOBBY_NOT_FOUND 2
#define ERR_NOT_HOST 3
#define ERR_NOT_ENOUGH_PLAYERS 4
#define ERR_ALREADY_IN_LOBBY 5

// Player structure - VỚI POWER-UPS
typedef struct {
    int id;
    int x, y;
    int is_alive;
    int is_ready;
    char username[MAX_USERNAME];
    int max_bombs;        // Số bom tối đa có thể đặt
    int bomb_range;       // Tầm nổ
    int current_bombs;    // Số bom đang đặt
} Player;

// Lobby structure
typedef struct {
    int id;
    char name[MAX_ROOM_NAME];
    int host_id;
    Player players[MAX_CLIENTS];
    int num_players;
    int status;
} Lobby;

// Game state
typedef struct {
    int map[MAP_HEIGHT][MAP_WIDTH];
    Player players[MAX_CLIENTS];
    int num_players;
    int game_status;
    int winner_id;
} GameState;

// Client packet
typedef struct {
    int type;
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
    char room_name[MAX_ROOM_NAME];
    int lobby_id;
    int data;
} ClientPacket;

// Server packet
typedef struct {
    int type;
    int code;
    char message[256];
    union {
        struct {
            char username[MAX_USERNAME];
        } auth;
        struct {
            Lobby lobbies[MAX_LOBBIES];
            int count;
        } lobby_list;
        Lobby lobby;
        GameState game_state;
    } payload;
} ServerPacket;

#endif