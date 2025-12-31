# Sơ Đồ Giao Tiếp - Bomberman Game Architecture

## 📋 Tổng Quan Kiến Trúc

Đây là một ứng dụng Bomberman client-server sử dụng **Socket TCP/IP** cho giao tiếp mạng và **SQLite** cho lưu trữ dữ liệu.

```
┌──────────────────┐                    ┌──────────────────┐
│   CLIENT SIDE    │◄──────────────────►│  SERVER SIDE     │
│   (SDL2 UI)      │   Socket TCP/IP    │  (Linux/C)       │
└──────────────────┘       Port 8081    └──────────────────┘
         │                                      │
         ├─ Graphics (SDL2)                    ├─ Game Logic
         ├─ Event Handler                      ├─ Database (SQLite)
         ├─ State Management                   ├─ Lobby Manager
         ├─ Network Handler                    ├─ Friend System
         └─ Session Manager                    ├─ ELO System
                                               └─ Statistics
```

---

## 🔌 Mô Hình Giao Tiếp

### Layer 1: Transport Layer

- **Protocol**: TCP/IP Socket
- **Port**: 8081
- **Connection Type**: Non-blocking socket (Client), Multi-client select() (Server)
- **Packet Size**: Fixed size packets (ClientPacket, ServerPacket)

### Layer 2: Application Layer

```
CLIENT ◄──────────────────────────► SERVER
  │                                   │
  ├─ ClientPacket (Send)             ├─ ServerPacket (Response)
  │  {                                │  {
  │   int type;                       │   int type;
  │   username, password, email       │   int code;
  │   display_name                    │   char message[256];
  │   lobby_id, game_mode             │   payload (union)
  │   chat_message, etc               │  }
  │  }                                │
  │                                   │
```

---

## 📊 Message Types (Giao Thức)

### Authentication Messages

```
CLIENT → SERVER              SERVER → CLIENT
MSG_REGISTER (1)    ────┐
                        ├──► MSG_AUTH_RESPONSE (33)
MSG_LOGIN (2)       ────┘
MSG_LOGIN_WITH_TOKEN (35)     MSG_ERROR (28)
```

### Lobby Management

```
CLIENT → SERVER              SERVER → CLIENT
MSG_CREATE_LOBBY (3) ──┐
MSG_JOIN_LOBBY (4)    ├──► MSG_LOBBY_UPDATE (19)
MSG_LIST_LOBBIES (5)  ├──► MSG_LOBBY_LIST (20)
MSG_LEAVE_LOBBY (6)   ├──► MSG_NOTIFICATION (27)
MSG_READY (21)        │
MSG_KICK_PLAYER (38)  ┘
```

### Game Messages

```
CLIENT → SERVER              SERVER → CLIENT
MSG_START_GAME (7)  ──┐
MSG_MOVE (9)        ├──► MSG_GAME_STATE (8)
MSG_PLANT_BOMB (10) ├──► MSG_GAME_STATE (8)
MSG_LEAVE_GAME (11) ┘
```

### Social Features

```
CLIENT → SERVER              SERVER → CLIENT
MSG_FRIEND_REQUEST (12)    ──┐
MSG_FRIEND_ACCEPT (13)       ├──► MSG_FRIEND_RESPONSE (18)
MSG_FRIEND_DECLINE (14)      ├──► MSG_FRIEND_LIST_RESPONSE (17)
MSG_FRIEND_REMOVE (15)       ├──► MSG_FRIEND_INVITE (30)
MSG_FRIEND_LIST (16)         ├──► MSG_INVITE_RECEIVED (31)
MSG_INVITE_RESPONSE (32)   ──┘
```

### Chat & Profile

```
CLIENT → SERVER              SERVER → CLIENT
MSG_CHAT (22)              ──► MSG_CHAT (22)
MSG_GET_PROFILE (23)       ──► MSG_PROFILE_RESPONSE (24)
MSG_GET_LEADERBOARD (25)   ──► MSG_LEADERBOARD_RESPONSE (26)
```

---

## 🏗️ Kiến Trúc Chi Tiết

### CLIENT SIDE ARCHITECTURE

```
┌─────────────────────────────────────────────────┐
│         CLIENT/MAIN.C (Entry Point)             │
├─────────────────────────────────────────────────┤
│                                                 │
│  ┌──────────────────────────────────────┐      │
│  │   Initialization                      │      │
│  ├──────────────────────────────────────┤      │
│  │ • init_sdl_window()  ────► SDL2 UI   │      │
│  │ • load_fonts()       ────► TTF       │      │
│  │ • setup_network...() ────► Connect   │      │
│  └──────────────────────────────────────┘      │
│                                                 │
│  ┌──────────────────────────────────────┐      │
│  │   Main Loop                           │      │
│  ├──────────────────────────────────────┤      │
│  │ while (running) {                     │      │
│  │   • handle_events()                   │      │
│  │   • process_network_packets()         │      │
│  │   • render_screen()                   │      │
│  │   • update_game_state()               │      │
│  │ }                                     │      │
│  └──────────────────────────────────────┘      │
│                                                 │
└─────────────────────────────────────────────────┘
         │               │               │
         │               │               │
    ┌────▼────┐   ┌─────▼──────┐  ┌────▼───────┐
    │Graphics │   │  Network   │  │  Handlers  │
    │(SDL2)   │   │(Socket I/O)│  │            │
    ├─────────┤   ├────────────┤  ├────────────┤
    │render   │   │connect_to  │  │handle_     │
    │entities │   │_server()   │  │events()    │
    │render   │   │            │  │            │
    │map()    │   │send_       │  │game_       │
    │render   │   │packet()    │  │handler()   │
    │game()   │   │            │  │            │
    │HUD,UI   │   │receive_    │  │session_    │
    │         │   │_packet()   │  │handler()   │
    │effects  │   │            │  │            │
    │         │   │process_    │  │event_      │
    │colors   │   │_server_    │  │handler()   │
    │         │   │packet()    │  │            │
    └─────────┘   └────────────┘  └────────────┘
         │               │               │
         └───────┬───────┴───────┬───────┘
                 │               │
            ┌────▼───────────────▼────┐
            │   CLIENT STATE          │
            ├─────────────────────────┤
            │ • my_username           │
            │ • my_user_id            │
            │ • current_lobby_id      │
            │ • current_game_state    │
            │ • friends_list          │
            │ • session_token         │
            │ • player_position       │
            │ • my_player_id          │
            └─────────────────────────┘
```

### CLIENT MODULES

```
CLIENT/
├── main.c              ► Entry point, event loop
├── state/
│   └── client_state.c  ► Local state management
├── network/
│   ├── network.h
│   └── network.c       ► Socket operations, packet I/O
├── handlers/
│   ├── event_handler.c ► SDL2 event processing
│   ├── game.c          ► Game logic (move, bomb)
│   ├── session.c       ► Login, token management
│   └── ui_handlers.c   ► UI interaction handlers
├── graphics/           ► SDL2 rendering
│   ├── render_game.c
│   ├── render_entity.c
│   ├── render_map.c
│   ├── HUD, effects, colors
│   └── overlay.c
└── ui/                 ► UI screens
    ├── ui_login.c
    ├── ui_lobby.c
    ├── ui_game.c
    ├── ui_chat.c
    ├── ui_friend.c
    ├── ui_social.c
    └── ui_dialog.c
```

---

### SERVER SIDE ARCHITECTURE

```
┌─────────────────────────────────────────────────┐
│         SERVER/MAIN.C (Entry Point)             │
├─────────────────────────────────────────────────┤
│                                                 │
│  ┌──────────────────────────────────────┐      │
│  │   Server Initialization              │      │
│  ├──────────────────────────────────────┤      │
│  │ • init_server_socket()               │      │
│  │ • db_init()                          │      │
│  │ • init_lobbies()                     │      │
│  └──────────────────────────────────────┘      │
│                                                 │
│  ┌──────────────────────────────────────┐      │
│  │   Server Main Loop                   │      │
│  ├──────────────────────────────────────┤      │
│  │ while (running) {                     │      │
│  │   • select() - check fd activity      │      │
│  │   • accept() - new connections        │      │
│  │   • recv() - read client packets      │      │
│  │   • route to handlers                 │      │
│  │   • send responses                    │      │
│  │   • update game states                │      │
│  │   • broadcast updates                 │      │
│  │ }                                     │      │
│  └──────────────────────────────────────┘      │
│                                                 │
└─────────────────────────────────────────────────┘
       │              │              │
       │              │              │
   ┌───▼──┐   ┌──────▼────┐   ┌────▼───────┐
   │Auth  │   │  Handlers │   │   Core     │
   │(DB)  │   │           │   │ Systems    │
   ├──────┤   ├───────────┤   ├────────────┤
   │      │   │handle_    │   │Lobby       │
   │auth  │   │login()    │   │Manager     │
   │      │   │           │   │            │
   │db_   │   │handle_    │   │Game Logic  │
   │login │   │create_    │   │            │
   │      │   │lobby()    │   │Friend Sys  │
   │db_   │   │           │   │            │
   │regis │   │handle_    │   │ELO System  │
   │ter   │   │game_move()│   │            │
   │      │   │           │   │Statistics  │
   │db_   │   │handle_    │   │            │
   │auth  │   │chat()     │   │            │
   │      │   │           │   │            │
   └──────┘   │handle_    │   └────────────┘
              │friend_    │
              │request()  │
              │           │
              │broadcast_ │
              │functions  │
              │           │
              └───────────┘
       │              │              │
       └──────┬───────┴───────┬──────┘
              │               │
         ┌────▼───────────────▼────┐
         │  GLOBAL STATE            │
         ├─────────────────────────┤
         │ • ClientInfo clients[]   │
         │ • Lobby lobbies[]        │
         │ • GameState games[]      │
         │ • LobbyChat chats[]      │
         │ • Friends links[]        │
         └─────────────────────────┘
              │
              ▼
         ┌─────────────────────────┐
         │   SQLite Database       │
         ├─────────────────────────┤
         │ • Users (auth, stats)   │
         │ • Friends relationships │
         │ • Match history         │
         │ • Leaderboard data      │
         │ • ELO ratings           │
         └─────────────────────────┘
```

### SERVER MODULES

```
SERVER/
├── main.c              ► Server loop, socket accept
├── server.h            ► Data structures, declarations
├── database.c          ► SQLite operations
├── network.c           ► Socket handling (server-side)
├── game_logic.c        ► Game state updates
├── lobby_manager.c     ► Lobby CRUD operations
├── map.c               ► Map generation, tile management
├── elo_system.c        ► ELO calculations
├── friend_system.c     ► Friend relationships
├── statistics.c        ► Match records, leaderboard
├── schema.sql          ► Database schema
├── handlers/
│   ├── auth.c          ► Registration, login
│   ├── lobby.c         ► Lobby creation/join
│   ├── game.c          ► Game move handling
│   ├── chat.c          ► Chat messages
│   └── social.c        ► Friend requests
└── test_elo_sim.c      ► ELO testing utility
```

---

## Chi Tiết Luồng Giao Tiếp

### 1.Authentication Flow

```
CLIENT                                    SERVER
   │                                        │
   ├─ MSG_LOGIN (username, password) ──────►│
   │                                        ├─ Verify credentials
   │                                        ├─ Query SQLite users table
   │                                        ├─ Generate session token
   │                                        │
   │                    ◄──────────────────┤│ MSG_AUTH_RESPONSE
   │                                        │  (user_id, token, elo)
   │
   ├─ Save token to local file             │
   │
```

### 2.Lobby Creation & Join Flow

```
CLIENT                              SERVER
   │                                  │
   ├─ MSG_CREATE_LOBBY ──────────────►│
   │  (room_name, game_mode,          ├─ Create lobby structure
   │   is_private, access_code)       ├─ Store in lobbies[]
   │                                  ├─ Add creator to lobby
   │                                  │
   │  ◄────────────────────────────── MSG_LOBBY_UPDATE (Lobby struct)
   │
   │  ◄────────────────────────────── MSG_LOBBY_LIST (broadcast to all)
   │
```

### 3.Game Start & Gameplay Flow

```
CLIENT                              SERVER
   │                                  │
   ├─ MSG_READY ─────────────────────►│
   │  (player marks ready)             ├─ Update player ready status
   │                                  │
   │  ◄────────────────────────────── MSG_LOBBY_UPDATE
   │
   (when all players ready)           │
   ├─ MSG_START_GAME ────────────────►│
   │                                  ├─ Generate game map
   │                                  ├─ Initialize GameState
   │                                  ├─ Create game in active_games[]
   │                                  │
   │  ◄────────────────────────────── MSG_GAME_STATE (full state)
   │
   ├─ MSG_MOVE (direction) ──────────►│
   │  (continuously)                   ├─ Update player position
   │                                  ├─ Check collisions
   │                                  ├─ Broadcast updated state
   │                                  │
   │  ◄────────────────────────────── MSG_GAME_STATE (20 Hz)
   │                                  │
   ├─ MSG_PLANT_BOMB ────────────────►│
   │                                  ├─ Place bomb on map
   │                                  ├─ Start bomb timer
   │                                  ├─ Broadcast explosion
   │                                  │
   │  ◄────────────────────────────── MSG_GAME_STATE
   │
   │ (game ends)                      │
   │  ◄────────────────────────────── MSG_GAME_STATE
   │                                  │  (winner_id, end_game_time)
   │                                  ├─ Update ELO ratings
   │                                  ├─ Record statistics
   │
```

### 4.Friend System Flow

```
CLIENT                              SERVER
   │                                  │
   ├─ MSG_FRIEND_REQUEST ────────────►│
   │  (target_display_name)           ├─ Find target user by name
   │                                  ├─ Create pending request
   │                                  ├─ Store in database
   │                                  │
   │  ◄────────────────────────────── MSG_FRIEND_RESPONSE
   │                                  │  (success/error)
   │
   (target client)                    │
   │  ◄────────────────────────────── MSG_INVITE_RECEIVED
   │                                  │  (sender_display_name)
   │
   ├─ MSG_FRIEND_ACCEPT ─────────────►│
   │  (requester_id)                  ├─ Create friend relationship
   │                                  ├─ Update database
   │                                  │
   │  ◄────────────────────────────── MSG_FRIEND_RESPONSE
   │
```

### 5.Chat Flow

```
CLIENT                              SERVER
   │                                  │
   ├─ MSG_CHAT ──────────────────────►│
   │  (message, lobby_id)             ├─ Store in LobbyChat struct
   │                                  ├─ Broadcast to all in lobby
   │                                  │
   │  ◄────────────────────────────── MSG_CHAT
   │                                  │  (to all in lobby)
   │
```

---

## Data Structures

### Client-Server Communication Structures

```c
// CLIENT SENDS
typedef struct {
    int type;                           // Message type (MSG_LOGIN, etc)
    char username[MAX_USERNAME];        // User identifier
    char password[MAX_PASSWORD];        // Auth credential
    char email[MAX_EMAIL];              // For registration
    char display_name[MAX_DISPLAY_NAME];// Mutable name

    // Lobby/Game operations
    int lobby_id;                       // Target lobby
    char room_name[MAX_ROOM_NAME];      // For creating lobby
    int data;                           // Multi-purpose: direction, player_id
    char access_code[8];                // For private lobby
    int is_private;                     // Lobby privacy flag
    int game_mode;                      // Game mode selection

    // Social
    char target_display_name[MAX_DISPLAY_NAME];
    int target_user_id;

    // Chat
    char chat_message[200];             // Chat text

    // Session
    char session_token[64];             // For auto-login
} ClientPacket;

// SERVER RESPONDS
typedef struct {
    int type;                           // Response type
    int code;                           // Status code (auth result, error)
    char message[256];                  // Error message or info

    // Union for different payload types
    union {
        // Authentication payload
        struct {
            int user_id;
            char username[MAX_USERNAME];
            char display_name[MAX_DISPLAY_NAME];
            int elo_rating;
            char session_token[64];
        } auth;

        // Lobby payload
        Lobby lobby;                    // Full lobby data

        // Game state payload
        GameState game_state;           // Full game state (or filtered)

        // Lists
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

        // Other payloads
        ProfileData profile;

        struct {
            char sender_username[MAX_USERNAME];
            char message[200];
            uint32_t timestamp;
            int player_id;              // For color coding
        } chat_msg;
    } payload;
} ServerPacket;
```

---

## Key Design Patterns

### 1. **Request-Response Pattern**

```
Client ─────► Request ─────► Server
         Packet (type, data)

Client ◄───── Response ◄───── Server
         Packet (type, code, payload)
```

### 2. **Broadcast Pattern** (Server → Multiple Clients)

```
Server broadcasts to all clients in lobby:
┌─────────────┐
│   Server    │
└──────┬──────┘
       ├──► Client 1
       ├──► Client 2
       ├──► Client 3
       └──► Client 4
```

### 3. **State Management Pattern**

```
CLIENT:  Local state ← Receive from server → Update UI
SERVER:  Authoritative state ← Process packets → Broadcast changes
```

### 4. **Session Token Pattern**

```
Login
  ↓
Receive token
  ↓
Save to disk
  ↓
On next launch: MSG_LOGIN_WITH_TOKEN
  ↓
Authenticate & reconnect
```

---

## 🗄️ Database Schema (SQLite)

```sql
-- Users table
CREATE TABLE users (
    id INTEGER PRIMARY KEY,
    username TEXT UNIQUE,
    display_name TEXT,
    email TEXT UNIQUE,
    password_hash TEXT,
    elo_rating INTEGER DEFAULT 1000,
    is_online INTEGER DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Friends table
CREATE TABLE friends (
    user_id INTEGER,
    friend_id INTEGER,
    status INTEGER,  -- 0: pending, 1: accepted
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (user_id, friend_id)
);

-- Match history
CREATE TABLE matches (
    id INTEGER PRIMARY KEY,
    player_ids TEXT,     -- Comma-separated player IDs
    placements TEXT,     -- Rank of each player
    kills TEXT,          -- Kill count for each player
    winner_id INTEGER,
    duration_seconds INTEGER,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Statistics (calculated from matches)
-- Leaderboard (calculated from ELO ratings)
```

---

## ⚡ Network Performance Considerations

### **Update Frequency**

- **Game State**: 20 Hz (50ms) - sent to all game players
- **Lobby List**: On-demand or periodic (5-10 sec)
- **Friend Status**: On-demand or periodic
- **Chat**: Immediate on send

### **Packet Loss Handling**

- Non-blocking socket with timeouts
- Resend mechanisms for critical packets (game state)
- Session token for reconnection

### **Scalability Limits**

- Max 4 clients per lobby
- Max 10 concurrent lobbies
- Max 4000 concurrent users (limited by system FDs and memory)

---

## Summary

| Component        | Technology                         | Purpose                    |
| ---------------- | ---------------------------------- | -------------------------- |
| **Transport**    | TCP/IP Socket                      | Reliable, ordered delivery |
| **Protocol**     | Custom (ClientPacket/ServerPacket) | Type-based message routing |
| **Client UI**    | SDL2 + TTF                         | Graphics, input, rendering |
| **Server Logic** | C with select()                    | Non-blocking multi-client  |
| **Storage**      | SQLite                             | Persistent user/match data |
| **Game Sync**    | 20 Hz broadcast                    | Real-time gameplay state   |
| **Auth**         | Token-based sessions               | Stateless reconnection     |
| **Social**       | Friend relationships               | In-database friend graph   |
| **Ranking**      | ELO system                         | Competitive rating         |

---

## Ghi Chú

- **Non-blocking I/O**: Server sử dụng `select()` cho nhiều client
- **Packet-based**: Tất cả giao tiếp sử dụng fixed-size packets
- **Stateless Design**: Server có thể khôi phục client via token
- **Broadcast Mechanism**: Server gửi cập nhật đến tất cả affected clients
