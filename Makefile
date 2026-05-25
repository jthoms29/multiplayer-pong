
#John Thoms, gvr812, 11357558
CC = gcc
CFLAGS = -g -std=gnu11
CPPFLAGS = -std=gnu11 -Wall -pedantic


OBJ_DIR=build/obj
LIB_DIR=build/lib
BIN_DIR=build/bin

# name of file with main function
BINARY = udp_server udp_host udp_client tcp_server tcp_host tcp_client test

# get name of object files from source files
SRCS = $(wildcard *.c)
OBJS = $(SRCS:%.c=$(OBJ_DIR)/%.o)



all: $(BINARY) csv

clean:
	rm -rf build $(BINARY) csv


# directories
$(OBJ_DIR) $(LIB_DIR) $(BIN_DIR):
	mkdir -p $@


# build every obj file
$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -I. $< -o $@

# link 
$(BIN_DIR)/udp_server: $(OBJ_DIR)/udp_server.o $(OBJ_DIR)/game_view.o $(OBJ_DIR)/network_functions.o $(OBJ_DIR)/game.o | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -lm -lncurses -o $@

$(BIN_DIR)/udp_host: $(OBJ_DIR)/udp_host.o $(OBJ_DIR)/game_view.o $(OBJ_DIR)/network_functions.o $(OBJ_DIR)/game.o | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -lm -lncurses -o $@

$(BIN_DIR)/udp_client: $(OBJ_DIR)/udp_client.o $(OBJ_DIR)/game_view.o $(OBJ_DIR)/network_functions.o $(OBJ_DIR)/game.o | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -lm -lncurses -o $@

$(BIN_DIR)/tcp_server: $(OBJ_DIR)/tcp_server.o $(OBJ_DIR)/game_view.o $(OBJ_DIR)/network_functions.o $(OBJ_DIR)/game.o | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -lm -lncurses -o $@

$(BIN_DIR)/tcp_host: $(OBJ_DIR)/tcp_host.o $(OBJ_DIR)/game_view.o $(OBJ_DIR)/network_functions.o $(OBJ_DIR)/game.o | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -lm -lncurses -o $@

$(BIN_DIR)/tcp_client: $(OBJ_DIR)/tcp_client.o $(OBJ_DIR)/game_view.o $(OBJ_DIR)/network_functions.o $(OBJ_DIR)/game.o | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -lm -lncurses -o $@

$(BIN_DIR)/test: $(OBJ_DIR)/test.o $(OBJ_DIR)/game.o $(OBJ_DIR)/game_view.o | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -lm -lncurses -o $@


udp_server: $(BIN_DIR)/udp_server
	ln -sf $< $@

udp_host: $(BIN_DIR)/udp_host
	ln -sf $< $@


udp_client: $(BIN_DIR)/udp_client
	ln -sf $< $@

tcp_server: $(BIN_DIR)/tcp_server
	ln -sf $< $@

tcp_host: $(BIN_DIR)/tcp_host
	ln -sf $< $@

tcp_client: $(BIN_DIR)/tcp_client
	ln -sf $< $@

test: $(BIN_DIR)/test
	ln -sf $< $@

csv: 
	mkdir ./csv