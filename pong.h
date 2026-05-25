// John Thoms, gvr812, 11357558

#ifndef PONG_H
#define PONG_H

#include <stdbool.h>
#include <ncurses.h>
#include <stdint.h>
#include <netdb.h>

#define F_WIDTH 100
#define F_HEIGHT 30
#define P_SPEED 10

// each frame should take a 60th of a second
#define FRAMETIME 1000/60

#define LPLAYER 0
#define RPLAYER 1

#define UP -1
#define DOWN 1

#define POSUPDATESIZE 2 + sizeof(uint32_t)
#define GAMESTATESIZE 8 + sizeof(uint32_t)



/*
 * Holds info related to the ball
 */
typedef struct ball {

    int8_t left;
    int8_t top;

    int8_t x_dir;
    int8_t y_dir;

    int8_t speed;
    
} ball;

/*
 * Holds info relating to each paddle
 */
typedef struct paddle {
    bool is_l;

    int8_t height;

    int8_t left;
    int8_t top;

} paddle;


/* Holds info about current game */
typedef struct game_info {
    int l_score;
    int r_score;

    paddle* l_paddle;
    paddle* r_paddle;
    ball* ball;
} game_info;


// GAME LOGIC FUNCTIONS (game.c) ####################################################
void init_game(game_info* obj);
int8_t get_player_input();
void move_paddle(paddle* paddle, int direction);
void update_ball_position(ball* ball);
void reset_ball(ball* b, int out);
void handle_ball_collision(game_info* g);
void game_loop_server(int server_fd, int (*send_update)(int, int, game_info*, void*), int (*receive_update)(int, int, game_info*, int*, FILE*), void*, FILE* csv);
void game_loop_client(int sockfd, int (*send_update)(int, int8_t, int8_t, struct addrinfo*), int (*receive_update)(int, game_info*, FILE*), int8_t player, struct addrinfo *serv_info, FILE* csv);
void game_loop_host(int server_fd, int (*send_update)(int, int, game_info*, void* client_info), int (*receive_update)(int, int, game_info*, int*, FILE*), void* client_info, FILE* csv);
uint32_t cur_ms();
uint32_t wall_ms();

int send_update_udp_server(int server_fd, int player, game_info* g, void* client_info);
int recv_update_udp_server(int server_fd, int player, game_info* g, int* client_fds, FILE* csv);

int send_update_tcp_server(int server_fd, int player, game_info* g, void* client_info);
int recv_update_tcp_server(int server_fd, int player, game_info* g, int* client_fds, FILE* csv);

int send_update_udp_client(int sockfd, int8_t d, int8_t player, struct addrinfo* serv_info);
int recv_update_udp_client(int sockfd, game_info* g, FILE* csv);

int send_update_tcp_client(int sockfd, int8_t d, int8_t player, struct addrinfo *serv_info);
int recv_update_tcp_client(int sockfd, game_info* g, FILE* csv);

FILE* create_file(char* filename);
int write_line(FILE* fpt, uint32_t received_tstamp, uint32_t cur_time, uint32_t latency);

// VIEW FUNCTIONS (game_view.c) ###########################################################
WINDOW* init_view();
void draw_ball(WINDOW* w, ball* b);
void draw_paddle(WINDOW* w, paddle* p);
void display_field(WINDOW* w, game_info* obj);


// NETWORK FUNCTIONS ###################################################
int udp_server_init(int* sockfd, char* port);
struct addrinfo* udp_client_init(int* sockfd, char* hostname, char* udp_port);
int tcp_server_init(int* sockfd, char* port);
int tcp_client_init(int* sockfd, char* hostname, char* port);
int send_all(int fd, void* data, size_t size);
int recv_all(int fd, void* recv_buf, size_t max_size);
#endif