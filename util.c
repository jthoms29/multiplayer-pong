#include <time.h>
#include <stdio.h>
#include <pong.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>

/* Get current time in milliseconds */
long cur_ms() {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts)) {
        perror("clock_gettime");
    }
    return ts.tv_sec * 1000 + ts.tv_nsec/1000000;
}
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

int8_t get_player_input() {
    MEVENT event;
    static int8_t continuous_input_frames = 0;
    static int8_t acceleration_countdown = 0;
    static int8_t prev_input_direction = 0;

    if (getch() == KEY_MOUSE && getmouse(&event) == OK) {

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

void move_paddle(paddle* paddle, int direction) {

    // new vertical position of paddle
    int new_top = paddle->top + direction;

    // restrict movement, paddle can't exit playing field
    if (new_top < 1 || (new_top + paddle->height) > F_HEIGHT) {
        return;
    }

    paddle->top = new_top;
}

void reset_ball(ball* b, int out) {
    b->left = F_WIDTH/2;
    b->top = F_HEIGHT/2;
    b->y_dir = 1;
    // If ball exited from the left side, it should now move to right
    // If it exited from the right side, it should now move towards the left
    b->x_dir = (out == 0) ? 1 : -1;
    b->speed = 1;
}

void update_ball_position(ball* ball) {
    // check preconds
    assert(ball);
    ball->left += ball->x_dir;
    ball->top += ball->y_dir;
}

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

void game_loop_server(int server_fd, int (*send_update)(int, int, game_info*), int (*receive_update)(int, int, game_info*)) {

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


    int counter;

    // indicates that this object has been updated in a given frame
    bool ball_updated = false;
    bool l_updated = false;
    bool r_updated = false;
    for(;;) {
        start_time = cur_ms(); 

        if (counter % -(ball.speed-4) == 0) {
            update_ball_position(&ball);
            handle_ball_collision(&g);
            ball_updated = true;
        }


        l_updated = receive_update(server_fd, LPLAYER, &g);
        r_updated = receive_update(server_fd, RPLAYER, &g);
          
        // only send update if state has changed this frame
        if (l_updated || r_updated || ball_updated) {
                send_update(server_fd, LPLAYER, &g);
                send_update(server_fd, RPLAYER, &g);
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


void game_loop_client(int sockfd, int (*send_update)(int, int8_t), int (*receive_update)(int, game_info*)) {
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
    for(;;) {
        start_time = cur_ms();

        input_direction = get_player_input();

        if (input_direction) {
            send_update(sockfd, input_direction);
        }

        receive_update(sockfd, &g);

        display_field(win, &g);

        // game should run at 60 fps
        wait_time = FRAMETIME - (cur_ms() - start_time);
        if (wait_time < 0) {
            wait_time = 0;
        }
        usleep(wait_time*1000);
    }

}