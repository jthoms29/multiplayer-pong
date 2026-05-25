//John Thoms, gvr812, 11357558

#include <ncurses.h>
#include <pong.h>


#define DRAW_PIXEL(w,x,y) mvwaddch(w, y, x, ' ' | COLOR_PAIR(1))

WINDOW* init_view() {
    // ncurses init
    initscr();

    // set up mousewheel input
    mousemask(ALL_MOUSE_EVENTS, NULL);
    mouseinterval(0);
    keypad(stdscr, TRUE);
    // don't block on getch();
    timeout(0);
    // make cursor invisible
    curs_set(0);
    // make it so black square characters can be drawn to screen
    start_color();
    use_default_colors();
    init_pair(1, COLOR_BLACK, COLOR_WHITE);

    return newwin(F_HEIGHT+1, F_WIDTH+1, 0, 0);
}


void draw_ball(WINDOW* w, ball* b) {
    int y = b->top;
    int x = b->left;
    DRAW_PIXEL(w, x, y);
}

void draw_paddle(WINDOW* w, paddle* p) {

    int x = p->left;
    for (int y = p->top; y < p->top+p->height; y++) {
        DRAW_PIXEL(w, x, y);
    }
}

void display_field(WINDOW* w, game_info* obj) {
    werase(w);
    box(w, 0, 0);
    draw_ball(w, obj->ball);
    draw_paddle(w, obj->r_paddle);
    draw_paddle(w, obj->l_paddle);
    wnoutrefresh(w);
    mvprintw(F_HEIGHT+1 , 0, "Left: %d  Right: %d", obj->l_score, obj->r_score);
    wnoutrefresh(stdscr);
    
    // update both window and stdscr at same time, avoids flicker
    doupdate();
}