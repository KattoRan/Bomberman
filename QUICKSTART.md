# Quick Start Guide

## 🚀 Get Running in 5 Minutes

### 1. Build Everything
```bash
cd /home/ducbao/Bomberman
make
```

### 2. Start Server
```bash
./server_bin
```
You should see:
```
[DB] Database initialized successfully
[LOBBY] System initialized
[SERVER] Listening on port 8081...
```

### 3. Start Client (in a new terminal)
```bash
./client_bin
```

### 4. Register Account
1. Enter username (e.g., "player1")
2. Enter email
3. Enter password
4. Click **Register**

### 5. Create a Room
1. After login, click **Create Room**
2. Enter room name
3. (Optional) Set as private and generate access code
4. Click **Create**

### 6. Play!
- **WASD** or **Arrow Keys**: Move
- **Space**: Plant bomb
- Goal: Be the last player standing!

---

## 🎮 Testing with Multiple Players

### Terminal 1: Server
```bash
./server_bin
```

### Terminal 2: Client 1
```bash
./client_bin
```
- Register as "player1"
- Create room "My Room"

### Terminal 3: Client 2
```bash
./client_bin
```
- Register as "player2"
- Join "My Room"
- Click **Ready**

### Terminal 2 (Back to Client 1)
- As host, click **Start Game**

Now both players can move and plant bombs!

---

## 🛠️ Troubleshooting

### "Address already in use"
Server is already running. Kill it:
```bash
pkill server_bin
```

### Database errors
Delete and recreate:
```bash
rm bomberman.db
./server_bin  # Will recreate database
```

### Client won't connect
Check server is running on port 8081:
```bash
netstat -an | grep 8081
```

### Can't see game window
Install SDL2:
```bash
sudo apt-get install libsdl2-dev libsdl2-ttf-dev  # Ubuntu/Debian
```

---

## 📝 Common Tasks

### View Database
```bash
sqlite3 bomberman.db
sqlite> SELECT * FROM Users;
sqlite> SELECT * FROM Friendships;
sqlite> .quit
```

### Clean Build
```bash
make clean
make
```

### See Server Logs
Server prints to console:
- `[DB]` - Database operations
- `[LOBBY]` - Lobby events
- `[GAME]` - Gameplay events
- `[FRIEND]` - Friend system
- `[ELO]` - Rating changes
- `[STATS]` - Statistics updates

### Test Friends System
1. Register two accounts (different clients)
2. Player 1: Go to Friends tab → Enter Player 2's display name → Send Request
3. Player 2: See pending request → Accept
4. Both players now see each other in friends list with online status

---

## 🎯 Next Steps

- Read [README.md](README.md) for project overview
- Read [ARCHITECTURE.md](ARCHITECTURE.md) for code structure
- Read [API.md](API.md) for function reference
- Check [requirements.md](requirements.md) for complete features list

---

## 💡 Development Tips

### Adding a Feature?
1. Start with `common/protocol.h` - define message types
2. Add server handler in `server/main.c`
3. Implement logic in appropriate module
4. Add client UI in `client/ui_screens.c`
5. Handle server response in `client/main.c`

### Debugging?
- Server logs show all packet activity
- Use `printf()` liberally (already in place)
- Check database: `sqlite3 bomberman.db`
- Client console shows network events

### Getting Lost?
1. **Protocol first**: What messages exist? (`protocol.h`)
2. **Follow the flow**: Client → Server → Module → Database
3. **Check the logs**: Server prints everything with `[MODULE]` prefix
4. **Read ARCHITECTURE.md**: Understand component responsibilities

---

Happy coding! 🎮
