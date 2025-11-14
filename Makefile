CC = gcc
CFLAGS = -Wall -g

BUILD_DIR = build
BIN_DIR = bin

# Tous les fichiers source
SERVER_SRCS = $(wildcard Serveur/**/*.c) $(wildcard watchers/*.c) $(wildcard game/*.c) $(wildcard parsing/*.c) $(wildcard utils/*.c)
CLIENT_SRCS = $(wildcard Client/*.c)

# Objets correspondants
SERVER_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SERVER_SRCS))
CLIENT_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(CLIENT_SRCS))

# Binaires finaux
SERVER_BIN = $(BIN_DIR)/server
CLIENT_BIN = $(BIN_DIR)/client

all: $(SERVER_BIN) $(CLIENT_BIN)

# Compilation des .o (pattern rule)
$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Liens
$(SERVER_BIN): $(SERVER_OBJS)
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

$(CLIENT_BIN): $(CLIENT_OBJS)
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

.PHONY: all clean
