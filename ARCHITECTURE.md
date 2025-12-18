# Architecture Documentation

## System Overview

Bomberman is a client-server multiplayer game with clear separation between backend logic and frontend presentation.

## Component Responsibilities

### Server (`server/`)

**Purpose**: Authoritative game state, authentication, persistence

#### `main.c` (661 lines)
- **Role**: Main server loop, network handling, packet routing
- **Key Functions**:
  - `main()` - Server initialization and select() loop
  - `handle_client_packet()` - Routes incoming packets to handlers
  - `broadcast_lobby_update()` - Sends lobby state to all clients
  - `broadcast_game_state()` - Sends game updates (60Hz)
- **Responsibilities**: Accept connections, manage client sockets, route messages

#### `database.c` (12KB)
- **Role**: SQLite database abstraction layer
- **Key Functions**:
  - `db_init()` - Initialize database and create tables
  - `db_register_user()` - Create new user with hashed password
  - `db_login_user()` - Authenticate and return user data
  - `db_update_elo()` - Update player ELO rating
  - `db_find_user_by_display_name()` - Search by display name
- **Responsibilities**: All database operations, schema management

#### `game_logic.c` (344 lines)
- **Role**: Core gameplay mechanics
- **Key Functions**:
  - `init_game()` - Initialize map and spawn players
  - `update_game()` - Game loop (bombs, explosions, win conditions)
  - `handle_move()` - Validate and process player movement
  - `plant_bomb()` - Place bomb with owner tracking
  - `create_explosion_line()` - Explosion propagation with chain reactions
  - `pickup_powerup()` - Power-up collection and limits
- **Data Structures**:
  - `Bomb[50]` - Active bombs with timers
  - `Explosion[500]` - Active explosion tiles
- **Responsibilities**: Map generation, movement, bombs, power-ups, win detection

#### `lobby_manager.c` (215 lines)
- **Role**: Room/lobby lifecycle management
- **Key Functions**:
  - `create_lobby()` - Create room with access code support
  - `join_lobby_with_code()` - Join with validation
  - `leave_lobby()` - Handle player leave and host transfer
  - `start_game()` - Start game with readiness check
  - `toggle_ready()` - Ready/unready toggle
- **Data**: `Lobby[10]` - Up to 10 concurrent lobbies
- **Responsibilities**: Room creation, player join/leave, host privileges

#### `friend_system.c` (270 lines)
- **Role**: Social features
- **Key Functions**:
  - `friend_send_request()` - Send by display name
  - `friend_accept_request()` - Accept pending request
  - `friend_get_list()` - Get friends with online status
  - `friend_get_pending_requests()` - Incoming requests
  - `friend_get_sent_requests()` - Outgoing requests
- **Responsibilities**: Friend management, online status

#### `elo_system.c` (117 lines)
- **Role**: Competitive ranking
- **Key Functions**:
  - `calculate_elo_change()` - Standard ELO formula
  - `elo_update_after_match()` - Update all players' ELO
  - `get_k_factor()` - Dynamic K based on experience
  - `get_tier()` - Calculate tier badge
- **Algorithm**: Zero-sum ELO with dynamic K-factor
- **Responsibilities**: Rating calculations, tier badges

#### `statistics.c` (191 lines)
- **Role**: Player performance tracking
- **Key Functions**:
  - `stats_record_match()` - Persist match to database
  - `stats_get_profile()` - Retrieve player stats
  - `stats_get_leaderboard()` - Top 100 players
- **Responsibilities**: Match history, statistics updates, leaderboards

---

### Client (`client/`)

**Purpose**: User interface, input handling, server communication

#### `main.c` (1,234 lines)
- **Role**: Main client loop, networking, state management
- **Key Sections**:
  - Lines 1-100: Includes, globals, state variables
  - Lines 101-366: Network handling (`receive_server_packet()`, `process_server_packet()`)
  - Lines 367-1234: Main loop with event handling, rendering dispatch
- **State Management**:
  - `current_screen` - Which UI to show (login, lobby list, room, game, friends, profile)
  - `current_lobby` - Active lobby data
  - `game_state` - Current game state
  - `friends[], pending_requests[], sent_requests[]` - Social data
- **Responsibilities**: SDL initialization, input events, screen switching, packet send/receive

#### `ui_screens.c` (948 lines)
- **Role**: UI rendering for all screens
- **Key Functions**:
  - `render_login_screen()` - Login/register form
  - `render_lobby_list_screen()` - Room browser
  - `render_lobby_room_screen()` - Room lobby with players
  - `render_create_room_dialog()` - Room creation popup
  - `draw_button()`, `draw_input_field()` - UI components
  - `draw_background_grid()` - Animated background
- **UI System**:
  - Modern dark theme (slate colors)
  - Rounded corners, shadows, hover effects
  - Responsive sizing helpers
- **Responsibilities**: All UI rendering except game screen

#### `ui_new_screens.c` (22KB)
- **Role**: Additional UI components
- **Key Functions**:
  - Friends screen rendering
  - Profile/statistics display
  - Leaderboard rendering
- **Responsibilities**: Social and stats UI

#### `graphics.c` (13KB)
- **Role**: Game screen rendering
- **Key Functions**:
  - Map tile rendering (walls, bombs, explosions, power-ups)
  - Player sprite rendering
  - HUD overlay
- **Responsibilities**: In-game graphics

---

### Common (`common/`)

#### `protocol.h` (212 lines)
- **Role**: Shared data structures and constants
- **Defines**:
  - Message types (30+ packet types)
  - Error codes (lobby full, wrong code, etc.)
  - Game constants (map size, tile types, power-up types)
- **Structures**:
  - `Player` - Player state with power-ups
  - `Lobby` - Room state
  - `GameState` - Complete game state
  - `ClientPacket` - Client requests
  - `ServerPacket` - Server responses (union for different payloads)
- **Responsibilities**: Protocol definition, type safety

---

## Data Flow

### 1. User Registration
```
[Client] User fills form
   ↓ MSG_REGISTER
[Server] Receives packet
   ↓ handle_client_packet()
[Server] database.c → db_register_user()
   ↓ Hash password, INSERT INTO Users
[Database] Create user record
   ↓ Success/Failure
[Server] Send MSG_AUTH_RESPONSE
   ↓
[Client] Show success or error message
```

### 2. Lobby Join
```
[Client] User clicks "Join" on room
   ↓ MSG_JOIN_LOBBY (with access code if private)
[Server] Receives packet
   ↓ handle_client_packet()
[Server] lobby_manager.c → join_lobby_with_code()
   ↓ Validate: room exists, not full, correct code
[Server] Add player to lobby.players[]
   ↓ broadcast_lobby_update()
[All Clients in Lobby] Receive MSG_LOBBY_UPDATE
   ↓
[All Clients] Update UI with new player
```

### 3. Game Loop (60Hz)
```
[Server] Game running, timer fires (every 16ms)
   ↓ update_game()
[Server] game_logic.c processes:
   - Bomb timers → explosions
   - Explosion timers → clear tiles
   - Player in explosion → kill
   - Check win condition
   ↓ broadcast_game_state()
[All Clients] Receive MSG_GAME_STATE
   ↓ process_server_packet()
[Client] graphics.c → render_game_screen()
   ↓ SDL_RenderPresent()
[Display] Updated frame shown to user
```

### 4. Friend Request
```
[Client] User enters display name, clicks "Send Request"
   ↓ MSG_FRIEND_REQUEST
[Server] friend_system.c → friend_send_request()
   ↓ db_find_user_by_display_name()
[Database] Find target user
   ↓ INSERT INTO Friendships (status='PENDING')
[Database] Store pending request
   ↓ MSG_NOTIFICATION (if target online)
[Target Client] "You have a friend request from X"
```

---

## Threading Model

**Current**: Single-threaded server with `select()` for I/O multiplexing

**Future** (Requirement 11):
- **Main Thread**: Game loop at 60Hz
- **Network Thread**: `epoll/IOCP` with packet queue
- **Database Thread**: Async SQLite writes

---

## Memory Management

**Server**:
- Static allocation for lobbies, bombs, explosions
- Minimal `malloc()` in game loop
- Database handles managed by SQLite

**Client**:
- SDL manages textures/surfaces
- Static arrays for friends, lobbies, game state
- Font loading at startup

---

## Performance Considerations

**Server**:
- 60Hz tick rate for game updates
- Delta compression (only changed entities)
- Pre-allocated game state arrays

**Client**:
- 60 FPS rendering with VSync
- Texture caching (fonts loaded once)
- Efficient rendering (only dirty regions)

**Network**:
- Binary protocol (packed structs)
- No JSON overhead
- Typical packet size: 50-500 bytes

---

## Extension Points

### Adding a New Feature

1. **Define Protocol** (`common/protocol.h`)
   - Add message type constant
   - Add fields to `ClientPacket` or `ServerPacket`

2. **Server Handler** (`server/main.c`)
   - Add case in `handle_client_packet()`
   - Call appropriate module function

3. **Server Logic** (appropriate module)
   - Implement feature logic
   - Update database if needed

4. **Client UI** (`client/ui_screens.c`)
   - Add UI elements
   - Send packet on user action

5. **Client Handler** (`client/main.c`)
   - Add case in `process_server_packet()`
   - Update client state

### Example: Adding Chat System

1. Protocol: `MSG_CHAT`, `chat_message[256]` field
2. Server: Broadcast to all in same lobby
3. Server: No logic needed (just relay)
4. Client: Input field + message list in `ui_screens.c`
5. Client: Display in room screen

---

## Key Design Decisions

### Why SQLite?
- No external database server needed
- ACID transactions
- Simple setup for educational project
- Good enough for ~100 concurrent users

### Why SDL2?
- Cross-platform
- Hardware-accelerated rendering
- Active community and documentation
- Good compromise between low-level and high-level

### Why Binary Protocol?
- Reduced bandwidth (vs JSON)
- Type safety with structs
- Predictable packet sizes
- Fast serialization

### Why Static Arrays?
- Predictable memory usage
- No fragmentation
- Cache-friendly
- Simpler than dynamic allocation in C

---

## Common Workflows

### Reading Code Flow

1. **Start with `protocol.h`** - Understand message types
2. **Server entry**: `server/main.c → handle_client_packet()`
3. **Follow handler**: See which module function is called
4. **Module logic**: Read the specific feature implementation
5. **Database**: Check `database.c` if data is persisted
6. **Client rendering**: See `ui_screens.c` for UI

### Debugging a Feature

1. **Server logs**: Check printf statements with `[MODULE]` prefix
2. **Client logs**: Check console output
3. **Database**: `sqlite3 bomberman.db` to inspect tables
4. **Network**: Trace packet types being sent/received

### Adding Tests

1. **Unit test**: Test module functions in isolation
2. **Integration test**: Spawn server + client, simulate interactions
3. **Manual test**: Use client UI to verify behavior
