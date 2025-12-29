CC = gcc
CFLAGS = -Wall -Wextra -Icommon -Iclient 		

# SDL2 (client dùng)
SDL_CFLAGS = `sdl2-config --cflags`
SDL_LIBS = `sdl2-config --libs` -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lm

# ---- DIRECTORIES ----

# CLIENT SOURCES
CLIENT_SRC := \
    $(wildcard client/*.c) \
    $(wildcard client/graphics/*.c) \
    $(wildcard client/ui/*.c) \
    $(wildcard client/state/*.c) \
    $(wildcard client/handlers/*.c) \
    $(wildcard client/network/*.c)

# SERVER SOURCES
SERVER_SRC = $(filter-out server/test_elo_sim.c, $(wildcard server/*.c))
SERVER_HANDLERS = $(wildcard server/handlers/*.c)

# OBJECTS
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)
SERVER_OBJ = $(SERVER_SRC:.c=.o) $(SERVER_HANDLERS:.c=.o)

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
	$(CC) -o $@ $^ -lsqlite3 -lm

server/%.o: server/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# ---- CLEAN ----
clean:
	rm -f \
		client/*.o \
		client/graphics/*.o \
		client/ui/*.o \
		client/state/*.o \
		client/handlers/*.o \
		client/network/*.o \
		server/*.o \
		server/handlers/*.o \
		$(CLIENT_BIN) \
		$(SERVER_BIN)

# ---- RUN ----
run-client: $(CLIENT_BIN)
	./$(CLIENT_BIN)

run-server: $(SERVER_BIN)
	./$(SERVER_BIN)

.PHONY: all clean run-client run-server
