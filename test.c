
//John Thoms, gvr812, 11357558
#include <assert.h>
#include <pong.h>
#include <stdio.h>

void init_test(void) {


    paddle l;
    paddle r;
    ball b;

    game_info g;
    g.ball = &b;
    g.l_paddle = &l;
    g.r_paddle = &r;


    // check proper default values set
    init_game(&g);
    assert(g.ball->left == F_WIDTH/2);
    assert(g.ball->top == F_HEIGHT/2);

    assert(g.ball->speed == 1);

    assert(g.l_paddle->left == 1);
    assert(g.r_paddle->left == F_WIDTH-1);

    assert(g.l_score == 0);
    assert(g.r_score == 0);
}


void move_paddle_test() {
    paddle l;
    paddle r;
    ball b;

    game_info g;
    g.ball = &b;
    g.l_paddle = &l;
    g.r_paddle = &r;

    init_game(&g);


    int prev_top;

    // ensure left paddle can't move upwards out of bounds
    while (l.top != 1) {
        prev_top = l.top;
        move_paddle(&l, -1);

        assert(l.top == prev_top-1);
        assert(l.left == 1); // x position should never change
    }
    // now, position shouldn't change when attempting to move up more
    prev_top = l.top;
    move_paddle(&l, -1);
    assert(l.top == prev_top);
    assert(l.left == 1); // x position should never change


    // now downwards
    // ensure left paddle can't move upwards out of bounds
    while (l.top != F_HEIGHT-l.height) {
        prev_top = l.top;
        move_paddle(&l, 1);

        assert(l.top == prev_top + 1);
        assert(l.left == 1); // x position should never change
    }
    // now, position shouldn't change when attempting to move down more
    prev_top = l.top;
    move_paddle(&l, 1);
    assert(l.top == prev_top);
    assert(l.left == 1); // x position should never change



    /* Now do same for right paddle*/
    // ensure left paddle can't move upwards out of bounds
    while (r.top != 1) {
        prev_top = r.top;
        move_paddle(&r, -1);

        assert(r.top == prev_top-1);
        assert(r.left == F_WIDTH-1); // x position should never change
    }
    // now, position shouldn't change when attempting to move up more
    prev_top = r.top;
    move_paddle(&r, -1);
    assert(r.top == prev_top);
    assert(r.left == F_WIDTH-1); // x position should never change


    // now downwards
    // ensure right paddle can't move upwards out of bounds
    while (r.top != F_HEIGHT-l.height) {
        prev_top = r.top;
        move_paddle(&r, 1);

        assert(r.top == prev_top + 1);
        assert(r.left == F_WIDTH-1); // x position should never change
    }
    // now, position shouldn't change when attempting to move down more
    prev_top = r.top;
    move_paddle(&r, 1);
    assert(r.top == prev_top);
    assert(r.left == F_WIDTH-1); // x position should never change
}


void ball_test(void) {
    paddle l;
    paddle r;
    ball b;

    game_info g;
    g.ball = &b;
    g.l_paddle = &l;
    g.r_paddle = &r;

    init_game(&g);


    // ball will exit field from right side if no paddle input
    int prev_left, prev_top;
    int prev_x_dir, prev_y_dir;
    int prev_l_score = 0, prev_y_score = 0;
    while (b.left != F_WIDTH) {
        prev_left = b.left;
        prev_top = b.top;
        prev_x_dir = b.x_dir;
        prev_y_dir = b.y_dir;
        
        handle_ball_collision(&g);
        update_ball_position(&b);


        assert(b.x_dir == prev_x_dir);
        assert(b.left == prev_left + prev_x_dir);
        // if the previous y position wasn't upper or lower boundary, ball should continue moving in same direction
        if (prev_top != F_HEIGHT && prev_top != 0) {
            assert(b.y_dir == prev_y_dir);
            assert(b.top == prev_top + prev_y_dir);
        }
        // if previous y position was upper or lower boundary, y direction should reverse
        else {
            assert(b.y_dir == -prev_y_dir);
            assert(b.top == prev_top - prev_y_dir);
        }
    }
    handle_ball_collision(&g);

    // after exiting field, position should reset, left player's score should increment
    assert(g.ball->left == F_WIDTH/2);
    assert(g.ball->top == F_HEIGHT/2);
    assert(g.ball->speed == 1);
    assert(g.l_score == prev_l_score+1);

    // do same check, now for left side since ball will now move in opposite direction
    while (b.left != 0) {
        prev_left = b.left;
        prev_top = b.top;
        prev_x_dir = b.x_dir;
        prev_y_dir = b.y_dir;
        
        handle_ball_collision(&g);
        update_ball_position(&b);


        assert(b.x_dir == prev_x_dir);
        assert(b.left == prev_left + prev_x_dir);
        // if the previous y position wasn't upper or lower boundary, ball should continue moving in same direction
        if (prev_top != F_HEIGHT && prev_top != 0) {
            assert(b.y_dir == prev_y_dir);
            assert(b.top == prev_top + prev_y_dir);
        }
        // if previous y position was upper or lower boundary, y direction should reverse
        else {
            assert(b.y_dir == -prev_y_dir);
            assert(b.top == prev_top - prev_y_dir);
        }
    }
    handle_ball_collision(&g);
    // after exiting field, position should reset, right player's score should increment
    assert(g.ball->left == F_WIDTH/2);
    assert(g.ball->top == F_HEIGHT/2);
    assert(g.ball->speed == 1);
    assert(g.r_score == prev_y_score+1);

}

int main(void) {
    // check that game object is initialized properly
    init_test();
    // make sure paddles never move out of bounds
    move_paddle_test();
    // make sure ball position is updated properly
    ball_test();

    printf("All tests passed :))))\n");
}