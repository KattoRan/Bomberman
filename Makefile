CC = gcc
CFLAGS = -Wall -Wextra -Icommon

# SDL2 (client dùng)
SDL_CFLAGS = `sdl2-config --cflags`
SDL_LIBS = `sdl2-config --libs` -lSDL2_image -lSDL2_ttf -lSDL2_mixer

# ---- DIRECTORIES ----
CLIENT_SRC = $(wildcard client/*.c)
SERVER_SRC = $(wildcard server/*.c)

CLIENT_OBJ = $(CLIENT_SRC:.c=.o)
SERVER_OBJ = $(SERVER_SRC:.c=.o)

CLIENT_BIN = client_bin
SERVER_BIN = server_bin

# ---- DEFAULT ----
all: $(CLIENT_BIN) $(SERVER_BIN)

# ---- CLIENT BUILD ----
$(CLIENT_BIN): $(CLIENT_OBJ)
	$(CC) -o $@ $^ $(SDL_LIBS)

client/%.o: client/%.c
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@


# ---- SERVER BUILD ----
$(SERVER_BIN): $(SERVER_OBJ)
	$(CC) -o $@ $^

server/%.o: server/%.c
	$(CC) $(CFLAGS) -c $< -o $@


# ---- CLEAN ----
clean:
	rm -f client/*.o server/*.o $(CLIENT_BIN) $(SERVER_BIN)

# ---- RUN ----
run-client: $(CLIENT_BIN)
	./$(CLIENT_BIN)

run-server: $(SERVER_BIN)
	./$(SERVER_BIN)

.PHONY: all clean run-client run-server
