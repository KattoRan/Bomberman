# 🎮 Bomberman - Multiplayer Game

A real-time multiplayer Bomberman game built with C server and SDL2 client.

## 🚀 Quick Start

```bash
# Build everything
make

# Run server (in one terminal)
./server_bin

# Run client (in another terminal)
./client_bin
```

## 📁 Project Structure

```
Bomberman/
├── server/              # C server with TCP sockets
│   ├── main.c          # Server loop & packet handling
│   ├── database.c      # SQLite operations
│   ├── game_logic.c    # Core gameplay mechanics
│   ├── lobby_manager.c # Room/lobby management
│   ├── friend_system.c # Friend management
│   ├── elo_system.c    # ELO rating calculations
│   ├── statistics.c    # Player statistics
│   └── schema.sql      # Database schema
├── client/              # SDL2 graphical client
│   ├── main.c          # Client loop & networking
│   ├── ui_screens.c    # UI rendering (login, lobby, game)
│   ├── ui_new_screens.c # Additional UI components
│   └── graphics.c      # Graphics utilities
├── common/
│   └── protocol.h      # Shared network protocol
├── bomberman.db        # SQLite database (auto-created)
└── requirements.md     # Complete feature specifications
```

## 🎯 Features

### ✅ Implemented
- **Authentication**: Register/Login with secure password hashing
- **Lobbies**: Public/private rooms with 6-digit access codes
- **Gameplay**: 15x13 grid, bombs, explosions, power-ups
- **Friends**: Send/accept requests, online status, remove friends
- **ELO System**: Competitive ranking with tier badges
- **Statistics**: Match history, win rate, K/D ratio
- **UI**: Modern dark theme with smooth animations

### 🔨 In Progress
- Speed power-up implementation
- Match timer & sudden death mode
- ELO integration at match end
- Kill tracking during gameplay

## 🗄️ Database Schema

**5 Tables:**
- `Users` - Authentication & profiles
- `Friendships` - Friend relationships
- `Statistics` - Player performance
- `MatchHistory` - Completed matches
- `MatchPlayers` - Individual match participation

## 🌐 Network Protocol

**Client → Server:**
- Authentication: `MSG_REGISTER`, `MSG_LOGIN`
- Lobby: `MSG_CREATE_LOBBY`, `MSG_JOIN_LOBBY`, `MSG_LEAVE_LOBBY`, `MSG_READY`, `MSG_START_GAME`
- Gameplay: `MSG_MOVE`, `MSG_PLANT_BOMB`
- Social: `MSG_FRIEND_REQUEST`, `MSG_FRIEND_ACCEPT`, `MSG_FRIEND_REMOVE`

**Server → Client:**
- `MSG_AUTH_RESPONSE`, `MSG_LOBBY_LIST`, `MSG_LOBBY_UPDATE`
- `MSG_GAME_STATE`, `MSG_FRIEND_LIST_RESPONSE`
- `MSG_PROFILE_RESPONSE`, `MSG_LEADERBOARD_RESPONSE`

## 🎮 Gameplay

**Controls:**
- WASD / Arrow Keys - Movement
- Space - Plant bomb

**Power-ups:**
- 💣 BombUp - Increase max bombs (max 3)
- 🔥 FireUp - Increase explosion range (max 4)
- ⚡ SpeedUp - Increase movement speed (TODO)

**Win Condition:**
- Last player standing wins
- Simultaneous death = draw

## 🏗️ Architecture

```
┌──────────┐         TCP/IP          ┌──────────┐
│  Client  │◄─────────────────────►│  Server  │
│  (SDL2)  │   Binary Protocol      │   (C)    │
└──────────┘                         └────┬─────┘
                                          │
                                     ┌────▼─────┐
                                     │ SQLite   │
                                     │ Database │
                                     └──────────┘
```

## 📊 Progress Status

**Overall**: ~85% Complete

| Component | Status | Completion |
|-----------|--------|------------|
| Authentication | ✅ | 100% |
| Lobby System | ✅ | 100% |
| Core Gameplay | ⚠️ | 85% |
| Friends System | ✅ | 100% |
| ELO System | ⚠️ | 95% |
| Statistics | ⚠️ | 95% |
| UI/UX | ✅ | 95% |

## 🔧 Development

**Requirements:**
- GCC compiler
- SDL2 development libraries
- SDL2_ttf
- SQLite3
- Make

**Build Commands:**
```bash
make          # Build both server and client
make server   # Build server only
make client   # Build client only
make clean    # Clean build files
```

## 📝 Documentation

- `requirements.md` - Complete feature specifications (38 requirements)
- `ARCHITECTURE.md` - System design and code organization
- `API.md` - Network protocol and database API reference

## 🐛 Known Issues

1. Speed power-up not functional
2. ELO/stats not called at match end
3. Kill tracking not implemented
4. Match timer missing

See [walkthrough.md](/.gemini/antigravity/brain/149d733d-81e3-46ad-9aa6-ddd444f06d57/walkthrough.md) for detailed progress report.

## 📜 License

Educational project - feel free to use and modify!
