
//John Thoms, gvr812, 11357558
#include <bits/time.h>
#include <time.h>
#include <stdio.h>
#include <pong.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <stdint.h>
/*
 Create csv file to record latency between server and client
 */
FILE* create_file(char* filename) {
    FILE* fpt = fopen(filename, "w");
    if (fpt == NULL) {
        return NULL;
    }

    fprintf(fpt, "received_tstamp,cur_time,latency\n");
    return fpt;
}

int write_line(FILE* fpt, uint32_t received_tstamp, uint32_t cur_time, uint32_t latency) {
    assert (received_tstamp != 0);
    fflush(fpt);
    fprintf(fpt, "%u,%u,%u\n", received_tstamp, cur_time, latency);
    return 0;
}

/* Get 32 bit timestamp from current millisecond value
- this will give about 50 days of unique timestamps 
*/
uint32_t cur_ms() {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts)) {
        perror("clock_gettime");
    }
    uint64_t big_ms = (uint64_t) ts.tv_sec * 1000 + ts.tv_nsec/1000000;
    return (uint32_t) big_ms;
}

uint32_t wall_ms() {
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts)) {
        perror("clock_gettime");
    }
    uint64_t big_ms = (uint64_t) ts.tv_sec * 1000 + ts.tv_nsec/1000000;
    return (uint32_t) big_ms;
}

/* Initialize game object with default values */
void init_game(game_info* obj) {

    paddle* l = obj->l_paddle;
    paddle* r = obj->r_paddle;
    ball* ball = obj->ball;

    obj->l_paddle->is_l = true;
    obj->r_paddle->is_l = false;

    l->left = 1;
    l->top = 8;
    l->height = 4;

    r->left = F_WIDTH-1;
    r->top = 8;
    r->height = 4;

    ball->top=F_HEIGHT/2;
    ball->left = F_WIDTH/2;
    ball->x_dir = 1;
    ball->y_dir = 1;
    ball->speed = 1;

    obj->l_score = 0;
    obj->r_score = 0;
}

/*
 * Get player input from mouse wheel. If there is continuous input for more than
 * a few frames, the last input will continue to be registered for a few extra frames
 */
int8_t get_player_input(bool* exit_flag) {
    MEVENT event;
    static int8_t continuous_input_frames = 0;
    static int8_t acceleration_countdown = 0;
    static int8_t prev_input_direction = 0;



    int ch = getch();

    if (ch == 'q') {
        *exit_flag = true;
        return 0;
    }

    if (ch == KEY_MOUSE && getmouse(&event) == OK) {

        // mouse wheel scroll up
        if (event.bstate & BUTTON4_PRESSED) {
            continuous_input_frames++;
            acceleration_countdown = 0;
            return (prev_input_direction = -1);
        }
        // mouse wheel scroll down
        if (event.bstate & BUTTON5_PRESSED) {
            continuous_input_frames++;
            acceleration_countdown = 0;
            return (prev_input_direction = 1);
        }
    }

    // if the player has given input on a few consecutive frames, that input
    // should continue to be read for a few more frames if they stop giveing input
    // - this makes control feel a bit better
    if (continuous_input_frames > 2) {
        acceleration_countdown = 6;
    }
    continuous_input_frames = 0;
    // continue giving previous direction for a few frames, or until input is recieved again
    if (acceleration_countdown) {
        acceleration_countdown--;
        return prev_input_direction;
    }
    
    return 0;
}

/*
 * Moves the paddle the given direction
 * - If the paddle's current position + direction would put it out of bounds, nothing changes
 */
void move_paddle(paddle* paddle, int direction) {
    // new vertical position of paddle
    int new_top = paddle->top + direction;
    // restrict movement, paddle can't exit playing field
    if (new_top < 1 || (new_top + paddle->height) > F_HEIGHT) {
        return;
    }
    paddle->top = new_top;
}

/*
 * Reset ball position after it exits the playing field
 */
void reset_ball(ball* b, int out) {
    b->left = F_WIDTH/2;
    b->top = F_HEIGHT/2;
    b->y_dir = 1;
    // If ball exited from the left side, it should now move to right
    // If it exited from the right side, it should now move towards the left
    b->x_dir = (out == 0) ? 1 : -1;
    b->speed = 1;
}

/*
 * Update the current position of the ball based on its direction values
 */
void update_ball_position(ball* ball) {
    ball->left += ball->x_dir;
    ball->top += ball->y_dir;
}

/* 
 * Checks if a ball has collided with either
 * - The top and bottom boundaries of the playing field
 * - The provided paddle reference
 * This function will be called every frame
 */
void handle_ball_collision(game_info* g) {
    // check pre-conds
    ball* ball = g->ball;
    paddle* l = g->l_paddle;
    paddle* r = g->r_paddle;

    assert(ball);
    assert(l);
    assert(r);
    assert(l->is_l);
    assert(!r->is_l);

    // check if ball has collided with top or bottom of playing field
    // reverse y direction if so
    if (ball->top <= 0 || (ball->top) >= F_HEIGHT) {
        ball->y_dir = -ball->y_dir;
    }

    // check if ball has collided with left paddle
    if (ball->left == l->left && ball->top >= l->top && ball->top <= (l->top+l->height)) {
        ball->x_dir = -ball->x_dir; 
        ball->left+=1;

        if (ball->top == l->top || ball->top == l->top+l->height) {
            // if the ball touches the very edge of the paddle, it should speed up
            ball->y_dir = (ball->top == l->top) ? -1 : 1;
            ball->speed = 2;
        }
        else {
            ball->speed = 1;
        }
    }

    // check if ball has collided with right paddle
    else if (ball->left == r->left && ball->top >= r->top && ball->top <= (r->top+r->height)) {
        ball->x_dir = -ball->x_dir;
        ball->left--;

        if (ball->top == r->top || ball->top == r->top+r->height) {
            ball->y_dir = (ball->top == r->top) ? -1 : 1;
            ball->speed = 2;
        }
        else {
            ball->speed = 1;
        }
    }

    // check if ball has exited field from left (right player score)
    else if (ball->left <= 0) {
        g->r_score++;
        reset_ball(ball, 0);

    }

    // check if ball has exited field from right (left player scores)
    else if (ball->left >= F_WIDTH) {
        g->l_score++;
        reset_ball(ball, 1);
    }
}

/*
 * Game loop for the server, runs at 60 fps. Provide functions for sending and receiving as
 * arguments - this means any protocol can be used with this function
 */

void game_loop_server(int server_fd, int (*send_update)(int, int, game_info*, void*), int (*receive_update)(int, int, game_info*, int*, FILE*), void* client_info, FILE* csv) {
    double start_time, wait_time;

    // set up gamestate object
    ball ball;
    paddle l;
    paddle r;
    game_info g;
    g.ball = &ball;
    g.l_paddle = &l;
    g.r_paddle = &r;
    init_game(&g);


    // used to count frames
    int counter = 0;

    // indicates that this object has been updated in a given frame
    bool ball_updated = false;
    int l_updated = 0;
    int r_updated = 0;


    // set up poll on stdin for exit flag
    struct pollfd pfd;
    pfd.fd = 0;
    pfd.events = POLLIN;

    char buf[8];

    for (;;) {
        start_time = cur_ms(); 

        // exit loop by typing 'q'
        int ret = poll(&pfd, 1, 0);
        if (ret < 0) {
            perror("poll");
            break;
        }
        if (pfd.revents & POLLIN) {
            if (fgets(buf, sizeof(buf), stdin) && buf[0] == 'q') {
                break;
            }
        }

        // depending on speed, ball will be updated every three frames or every two frames
        if (counter % -(ball.speed-4) == 0) {
            update_ball_position(&ball);
            handle_ball_collision(&g);
            ball_updated = true;
        }


        l_updated = receive_update(server_fd, LPLAYER, &g, client_info, csv);
        r_updated = receive_update(server_fd, RPLAYER, &g, client_info, csv);

        // disconnected
        if (l_updated < 0 || r_updated < 0) {
            break;
        }
         
        // only send update if state has changed this frame
        if (l_updated || r_updated || ball_updated) {
                if (send_update(server_fd, LPLAYER, &g, client_info) < 0) { break;}
                if (send_update(server_fd, RPLAYER, &g, client_info) < 0) {break;}
                l_updated = false;
                r_updated = false;
                ball_updated = false;
        }

        // game should run at 60 fps
        wait_time = FRAMETIME - (cur_ms() - start_time);
        if (wait_time < 0) {
            wait_time = 0;
        }
        counter++;
        usleep(wait_time*1000);
    }
}


/* 
 * Game loop for client, any protocol can be used for network functions, passed in as args
 */
void game_loop_client(int sockfd, int (*send_update)(int, int8_t, int8_t, struct addrinfo*), int (*receive_update)(int, game_info*, FILE*), int8_t player, struct addrinfo *serv_info, FILE* csv) {
    // intialize local game object
    game_info g;
    ball ball;
    paddle l;
    paddle r;
    g.ball = &ball;
    g.l_paddle = &l;
    g.r_paddle = &r;
    init_game(&g);

    WINDOW* win = init_view();


    int8_t input_direction;
    double start_time, wait_time;


    bool exit_flag = false;
    while (!exit_flag) {
        start_time = cur_ms();

        input_direction = get_player_input(&exit_flag);

        if (input_direction) {
            if (send_update(sockfd, input_direction, player, serv_info) < 0) {break; }
        }

        if (receive_update(sockfd, &g, csv) < 0) {break; }

        display_field(win, &g);

        // game should run at 60 fps
        wait_time = FRAMETIME - (cur_ms() - start_time);
        if (wait_time < 0) {
            wait_time = 0;
        }
        usleep(wait_time*1000);
    }
    endwin();
}

void game_loop_host(int server_fd, int (*send_update)(int, int, game_info*, void* client_info), int (*receive_update)(int, int, game_info*, int*, FILE*), void* client_info, FILE* csv) {
    double start_time, wait_time;

    // set up gamestate object
    ball ball;
    paddle l;
    paddle r;
    game_info g;
    g.ball = &ball;
    g.l_paddle = &l;
    g.r_paddle = &r;
    init_game(&g);

    WINDOW* win = init_view();

    // used to count frames
    int counter = 0;

    // indicates that this object has been updated in a given frame
    bool ball_updated = false;
    int r_updated = 0;


    int8_t input_direction = 0;

    bool exit_flag = false;


    // the player server also creates a second CSV, records latency between inputs and handling of those inputs
    // - this will likely always be instantaneous
    FILE* self_csv = create_file("./csv/host_player_latency.csv");

    uint32_t tstamp = 1, second_tstamp, self_latency = 0;
    while (!exit_flag) {
        start_time = cur_ms(); 


        if (counter % -(ball.speed-4) == 0) {
            update_ball_position(&ball);
            handle_ball_collision(&g);
            ball_updated = true;
        }


        // move own paddle
        tstamp = cur_ms();

        input_direction = get_player_input(&exit_flag);
        move_paddle(&l, input_direction);

        second_tstamp = cur_ms();
        self_latency = second_tstamp - tstamp;
        write_line(self_csv, tstamp, second_tstamp, self_latency);



        r_updated = receive_update(server_fd, RPLAYER, &g, client_info, csv);

        // disconnected
        if (r_updated < 0) {
            break;
        }
         
        // only send update if state has changed this frame
        if (input_direction || r_updated || ball_updated) {
                if (send_update(server_fd, RPLAYER, &g, client_info) < 0) {break;}
                r_updated = false;
                ball_updated = false;
        }

        display_field(win, &g);

        // game should run at 60 fps
        wait_time = FRAMETIME - (cur_ms() - start_time);
        if (wait_time < 0) {
            wait_time = 0;
        }
        counter++;
        usleep(wait_time*1000);
    }
    endwin();
}
