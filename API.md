# API Reference

Quick reference for network protocol and database functions.

## Network Protocol

### Message Type Constants

#### Client → Server
```c
#define MSG_REGISTER        1   // Register new account
#define MSG_LOGIN           2   // Login to account
#define MSG_CREATE_LOBBY    3   // Create new lobby
#define MSG_JOIN_LOBBY      4   // Join existing lobby
#define MSG_LEAVE_LOBBY     5   // Leave current lobby
#define MSG_LIST_LOBBIES    6   // Request lobby list
#define MSG_READY           7   // Toggle ready status
#define MSG_START_GAME      8   // Start game (host only)
#define MSG_MOVE            10  // Player movement
#define MSG_PLANT_BOMB      11  // Place bomb
#define MSG_FRIEND_REQUEST  12  // Send friend request
#define MSG_FRIEND_ACCEPT   13  // Accept friend request
#define MSG_FRIEND_DECLINE  14  // Decline friend request
#define MSG_FRIEND_REMOVE   15  // Remove friend
#define MSG_FRIEND_LIST     17  // Request friend list
#define MSG_GET_PROFILE     18  // Request profile data
#define MSG_GET_LEADERBOARD 20  // Request leaderboard
#define MSG_KICK_PLAYER     21  // Kick player (host only)
#define MSG_SET_ROOM_PRIVATE 22 // Toggle room privacy
```

#### Server → Client
```c
#define MSG_AUTH_RESPONSE         20  // Login/register result
#define MSG_LOBBY_LIST            21  // List of lobbies
#define MSG_LOBBY_UPDATE          22  // Lobby state changed
#define MSG_GAME_STATE            23  // Game state update
#define MSG_ERROR                 24  // Error message
#define MSG_FRIEND_LIST_RESPONSE  25  // Friend list data
#define MSG_PROFILE_RESPONSE      26  // Profile/stats data
#define MSG_LEADERBOARD_RESPONSE  27  // Leaderboard data
#define MSG_NOTIFICATION          28  // General notification
```

### Packet Structures

#### ClientPacket
```c
typedef struct {
    int type;                              // Message type
    char username[32];                     // Username
    char password[128];                    // Password
    char email[128];                       // Email (registration)
    char display_name[64];                 // Display name
    char target_display_name[64];          // Target for friend ops
    int target_user_id;                    // Target user ID
    int lobby_id;                          // Lobby ID
    char room_name[64];                    // Room name (create lobby)
    int data;                              // Multi-purpose data
    char access_code[8];                   // Private room code
    int is_private;                        // Private room flag
    int target_player_id;                  // Target player (kick)
} ClientPacket;
```

#### ServerPacket
```c
typedef struct {
    int type;                              // Message type
    int code;                              // Response code
    char message[256];                     // Message text
    union {
        struct {                           // AUTH_RESPONSE
            int user_id;
            char username[32];
            char display_name[64];
            int elo_rating;
        } auth;
        struct {                           // LOBBY_LIST
            Lobby lobbies[10];
            int count;
        } lobby_list;
        struct {                           // FRIEND_LIST
            FriendInfo friends[50];
            int count;
        } friend_list;
        struct {                           // LEADERBOARD
            LeaderboardEntry entries[100];
            int count;
        } leaderboard;
        Lobby lobby;                       // LOBBY_UPDATE
        GameState game_state;              // GAME_STATE
        ProfileData profile;               // PROFILE_RESPONSE
    } payload;
} ServerPacket;
```

### Error Codes
```c
#define AUTH_SUCCESS              0   // Authentication succeeded
#define AUTH_FAIL                 1   // Authentication failed
#define AUTH_USER_EXISTS          2   // Username already taken
#define ERR_LOBBY_NOT_FOUND      -1   // Lobby doesn't exist
#define ERR_LOBBY_FULL           -2   // Lobby at max capacity
#define ERR_LOBBY_GAME_IN_PROGRESS -3 // Game already started
#define ERR_LOBBY_LOCKED         -4   // Room is locked
#define ERR_LOBBY_WRONG_ACCESS_CODE -5 // Incorrect access code
#define ERR_NOT_HOST              3   // Not the host
#define ERR_NOT_ENOUGH_PLAYERS    4   // Need 2+ players
```

---

## Database API

### User Management

#### `db_register_user()`
```c
int db_register_user(const char *username, const char *email, const char *password);
```
- **Purpose**: Create new user account
- **Returns**: 0 on success, -1 on failure, -2 if username exists
- **Side Effects**: Creates Statistics entry, hashes password with salt

#### `db_login_user()`
```c
int db_login_user(const char *identifier, const char *password, User *out_user);
```
- **Purpose**: Authenticate user and load profile
- **Params**: 
  - `identifier`: Username or email
  - `password`: Plain text password
  - `out_user`: Output buffer for user data
- **Returns**: 0 on success, -1 on failure
- **Side Effects**: Updates last_login timestamp

#### `db_get_user_by_id()`
```c
int db_get_user_by_id(int user_id, User *out_user);
```
- **Purpose**: Load user data by ID
- **Returns**: 0 on success, -1 if not found

#### `db_find_user_by_display_name()`
```c
int db_find_user_by_display_name(const char *display_name, User *out_user);
```
- **Purpose**: Search user by display name (for friend requests)
- **Returns**: 0 on success, -1 if not found

#### `db_update_elo()`
```c
int db_update_elo(int user_id, int new_elo);
```
- **Purpose**: Update user's ELO rating
- **Returns**: 0 on success, -1 on failure

---

### Friend System

#### `friend_send_request()`
```c
int friend_send_request(int sender_id, const char *target_display_name);
```
- **Returns**: 
  - 0: Success
  - -1: User not found
  - -2: Cannot friend yourself
  - -4: Already friends
  - -5: Request already pending

#### `friend_accept_request()`
```c
int friend_accept_request(int user_id, int requester_id);
```
- **Purpose**: Accept pending friend request
- **Returns**: 0 on success, -1 on failure

#### `friend_decline_request()`
```c
int friend_decline_request(int user_id, int requester_id);
```
- **Purpose**: Decline/remove pending request
- **Returns**: 0 on success, -1 on failure

#### `friend_remove()`
```c
int friend_remove(int user_id, int friend_id);
```
- **Purpose**: Remove accepted friendship
- **Returns**: 0 on success, -1 on failure

#### `friend_get_list()`
```c
int friend_get_list(int user_id, FriendInfo *out_friends, int max_count);
```
- **Purpose**: Get accepted friends with online status
- **Returns**: Number of friends found
- **Notes**: Includes online status (0=offline, 1=online, 2=in-game)

#### `friend_get_pending_requests()`
```c
int friend_get_pending_requests(int user_id, FriendInfo *out_requests, int max_count);
```
- **Purpose**: Get incoming friend requests
- **Returns**: Number of requests found

#### `friend_get_sent_requests()`
```c
int friend_get_sent_requests(int user_id, FriendInfo *out_requests, int max_count);
```
- **Purpose**: Get outgoing friend requests
- **Returns**: Number of requests found

---

### ELO System

#### `calculate_elo_change()`
```c
int calculate_elo_change(int player_rating, int avg_opponent_rating, 
                         int placement, int matches_played);
```
- **Purpose**: Calculate ELO points gained/lost
- **Params**:
  - `placement`: 1 = winner, 2+ = loser
  - `matches_played`: Used for K-factor
- **Returns**: ELO change (positive or negative)

#### `elo_update_after_match()`
```c
int elo_update_after_match(int *player_ids, int *placements, int num_players);
```
- **Purpose**: Update all players' ELO ratings after match
- **Params**:
  - `player_ids[]`: Array of user IDs
  - `placements[]`: 1=1st, 2=2nd, etc.
- **Returns**: 0 on success, -1 on failure
- **Side Effects**: Updates Users.elo_rating in database

#### `get_tier()`
```c
int get_tier(int elo_rating);
```
- **Returns**: 0=Bronze, 1=Silver, 2=Gold, 3=Diamond

---

### Statistics

#### `stats_record_match()`
```c
int stats_record_match(int *player_ids, int *placements, int *kills,
                       int num_players, int winner_id, int duration_seconds);
```
- **Purpose**: Record match completion and update all statistics
- **Returns**: Match ID on success, -1 on failure
- **Side Effects**: 
  - Inserts into MatchHistory
  - Updates Statistics table for all players
  - Inserts into MatchPlayers

#### `stats_get_profile()`
```c
int stats_get_profile(int user_id, ProfileData *out_profile);
```
- **Purpose**: Load player statistics for display
- **Returns**: 0 on success, -1 on failure
- **Populates**: total_matches, wins, kills, deaths, ELO, tier

#### `stats_get_leaderboard()`
```c
int stats_get_leaderboard(LeaderboardEntry *out_entries, int max_count);
```
- **Purpose**: Get top players by ELO
- **Returns**: Number of entries found
- **Sorted**: By elo_rating DESC

#### `stats_increment_bombs()`
```c
void stats_increment_bombs(int user_id);
```
- **Purpose**: Track bombs planted statistic

#### `stats_increment_walls()`
```c
void stats_increment_walls(int user_id, int count);
```
- **Purpose**: Track walls destroyed statistic

---

### Lobby Management

#### `create_lobby()`
```c
int create_lobby(const char *room_name, const char *host_username, 
                 int is_private, const char *access_code);
```
- **Returns**: Lobby ID on success, -1 if no slots available

#### `join_lobby_with_code()`
```c
int join_lobby_with_code(int lobby_id, const char *username, const char *access_code);
```
- **Returns**: 
  - 0: Success
  - ERR_LOBBY_NOT_FOUND
  - ERR_LOBBY_FULL
  - ERR_LOBBY_GAME_IN_PROGRESS
  - ERR_LOBBY_LOCKED
  - ERR_LOBBY_WRONG_ACCESS_CODE

#### `start_game()`
```c
int start_game(int lobby_id, const char *username);
```
- **Returns**:
  - 0: Success
  - ERR_NOT_HOST
  - ERR_NOT_ENOUGH_PLAYERS

---

### Game Logic

#### `init_game()`
```c
void init_game(GameState *state, Lobby *lobby);
```
- **Purpose**: Initialize map and spawn players
- **Side Effects**: Generates random map, resets power-ups

#### `update_game()`
```c
void update_game(GameState *state);
```
- **Purpose**: Called every tick (60Hz) to update game state
- **Does**: Process bombs, explosions, deaths, win condition

#### `handle_move()`
```c
int handle_move(GameState *state, int player_id, int direction);
```
- **Params**: direction = MOVE_UP (0), MOVE_DOWN (1), MOVE_LEFT (2), MOVE_RIGHT (3)
- **Returns**: 
  - 0: Cannot move
  - 1: Moved successfully
  - 11: Moved + picked up power-up
  - 12: Moved + already at max power-up

#### `plant_bomb()`
```c
int plant_bomb(GameState *state, int player_id);
```
- **Returns**: 1 on success, 0 if cannot plant
- **Limits**: Respects player's max_bombs capacity

---

## Quick Reference

### Common Workflows

**User Login:**
```
Client → MSG_LOGIN → Server
Server → db_login_user() → Database
Server → MSG_AUTH_RESPONSE → Client
```

**Create Room:**
```
Client → MSG_CREATE_LOBBY → Server
Server → create_lobby() → lobbies[]
Server → broadcast_lobby_update() → All Clients
```

**Plant Bomb:**
```
Client → MSG_PLANT_BOMB → Server
Server → plant_bomb() → bombs[]
Server (60Hz) → update_game() → detect explosion
Server → broadcast_game_state() → All Clients
```

**Add Friend:**
```
Client → MSG_FRIEND_REQUEST → Server
Server → friend_send_request() → Friendships table
Server → MSG_NOTIFICATION → Target Client (if online)
```

### Database Schema Quick View
```sql
Users(id, username, display_name, email, password_hash, salt, elo_rating)
Friendships(id, user_id_1, user_id_2, status)  -- PENDING or ACCEPTED
Statistics(user_id, total_matches, wins, total_kills, deaths, bombs_planted, walls_destroyed)
MatchHistory(id, match_date, winner_id, duration_seconds, num_players)
MatchPlayers(id, match_id, user_id, placement, kills, deaths, elo_change)
```
