CC = gcc
CFLAGS = -Wall -g

# Dossiers
BUILD_DIR = build
BIN_DIR = bin

# Sources serveur
SERVER_SRCS = \
	Serveur/server.c \
	Serveur/clients/client.c \
	Serveur/watchers/watchers.c \
	Serveur/game/game.c \
	Serveur/parsing/parsing.c \
	Serveur/utils/utils.c

# Sources client
CLIENT_SRCS = Client/client.c

# Objets compilés  
SERVER_OBJS = $(SERVER_SRCS:%.c=$(BUILD_DIR)/%.o)
CLIENT_OBJS = $(CLIENT_SRCS:%.c=$(BUILD_DIR)/%.o)

# Binaires
SERVER_BIN = $(BIN_DIR)/server
CLIENT_BIN = $(BIN_DIR)/client

# Règle principale
all: $(SERVER_BIN) $(CLIENT_BIN)

# Compilation des .o
$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Link serveur
$(SERVER_BIN): $(SERVER_OBJS)
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

# Link client
$(CLIENT_BIN): $(CLIENT_OBJS)
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

.PHONY: all clean
