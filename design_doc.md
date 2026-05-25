John Thoms, gvr812, 11357558

## `game.c`
This file contains the bulk of the implementation for the actual pong game.
The game contains 3 entities, the left paddle, the right paddle, and the ball. These are represented with
- `struct paddle`
- `struct ball`
These structs contain basic info about their respective entities, position (denoted by left (x) and top (y)), dimensions, and in the case
of the ball variables for speed and direction.
There is one final struct, `struct game_info`. This holds references to all of the entities, as well as the scores for the left and right
players.

From here, I will give a basic summary of the major functions that are executed within the game, explaining some of my major design decisions.

- `void init_game(game_info* obj)`
This function takes a game_info object and sets all of its values to their defaults, e.g. scores of 0, starting positions for all entities


- `void update_ball_position(ball* ball)`
This takes a reference to a ball object and updates its current position (left, top) based on it's direction variables. The current
position will only be incremented by one each time this function is called. I will explain why this is later.
This function is called every few frames - will explain later



 - `void handle_ball_collision(game_info* g)`
 This function is called everytime one of the entities is updated. It checks if the ball has either collided with the bounds of the playing
 field, or one of the paddles, and modifies its values accordingly - reversing x or y direction if it collided with another object, or reseting
 it if it exited the playing field.
 If the ball collides with the very edge of one of the paddles, its speed value is increased. This is to make the game a bit more dynamic. Ideally
 I could have separate speeds for the ball's x and y movement, but I couldn't get this to work for reasons I'll explain later, so the speed is
 just constant in both directions. 
 If the ball collides with the middle portion of one of the paddles, its speed is set to normal.


- `int8_t get_player_input(bool* exit_flag)`
This function is called every frame on the client side. It reads the input of the mousewheel on the current frame, returning the directional
value of the input (-1 for up, 1 for down)
I had wanted to use W and S to get player input, but couldn't get this to work. ncurses does not offer any kind of direct input polling,
meaning all you can do is read characters from the terminal. This means that when you hold down a key, after the initial input is registered there
is a short delay before constant input of that key is registered. This feels really bad when used as an input for a real time game, so I decided
against key inputs. The mousewheel is not subject to this same repeated input delay, and is luckily a very natural input method for a pong paddle,
so I went with that instead.
To try and make input feel a bit better, I have it so input continues being registered after the player stops giving input for a few frames if
they had given input for multiple frames beforehand. This is called 'acceleration' in the function, but it's more like a very simple attempt at
implementing deceleration. In effect it makes it so if you flick the mousewheel hard enough the paddle will travel a longer distance, which feels
natural.


- `void move_paddle(paddle* paddle, int direction)`
This function takes the input direction from the previous function, and applies it to one of the paddles. This is separated from the input
function, as the input comes from across the network in most circumstances.
This function also makes sure the paddle cannot be moved out of bounds.


There are three game loop functions, one for the client side, one for the server side, and one for a client that also acts as a server. These
are closely tied to the networking aspect of this project, so even though they are also defined int `game.c` I want to talk about the view
functions first


## `game_view.c`
To display the game, I opted to use ncurses. My reasoning for doing this was that I though it would make things simpler so I could mainly focus
on the networking portion - the major focus of this project. What instead happened was I spent most of my time trying to work around the strict
limitations imposed by displaying the game with ncurses, so it was probably a bad idea. In hindsight I think SDL would have been a better choice.


- `WINDOW* init_view()`
This function sets up the ncurses window where the game is displayed. It also sets up mouse input, and the color pair which I used to draw
'pixels' to the screen (whitespace with a white colored background). ncurses does not use a pixel buffer, it uses a character buffer. Characters are much larger than pixels, and they are
taller than they are wide. This lead to the game having a very strange internal resolution of 100x30. I wanted it to fit on a terminal window taking up
half the screen of a Spinks monitor at standard scale.

- `void draw_ball(WINDOW* w, ball* b)`, `void draw_paddle(WINDOW* w, paddle* p)`
Self explanatory. These draw the ball and given paddle at their x,y positions within the window

- `void display_field(WINDOW* w, game_info* obj)`
This function is called every frame. It refershes the screen and draws each entity in the game_info object at their current position.
It also displays the current score below the main game window.


## `game.c` again

First I'll talk about the function which handles the game loop on the server side, as this is the most complicated - it handles the game logic
and communicates with the client players.

### `void game_loop_server(int server_fd, int (*send_update)(int, int, game_info*, void*), int (*receive_update)(int, int, game_info*, int*, FILE*), void* client_info, FILE* csv)`
I wanted to make this function as general as possible - so it could be used with both UDP and TCP clients without any changes. I'll walk through the arguments 

- `int server_fd` - This is the file descriptor for the server itself - udp server uses it to communicate with clients
- `int (*send_update)` - This is the pointer to a function that handles sending updates to the clients - will go into more later
- `int (*receive_update)` - Pointer to function that handles receiving updates from clients
- `void* client_info*` - This is a len 2 array which either holds the socket addresses of the clients (udp) or fds for the clients (tcp)
- `FILE* csv` - This is a pointer to an open csv file, used to record latency values from client to server.


To start, this function sets up the game object, send and receive buffers, and a pollfd - this makes it so the user can cleanly close the server
by typing 'q'.

The first major thing that happens in the main game loop is this.
```c
if (counter % -(ball.speed-4) == 0) {
    update_ball_position(&ball);
    handle_ball_collision(&g);
    ball_updated = true;
}
```
The ball's speed value will only ever be 1 or 2. If it's speed is 1, it will be updated every 3 frames. If it's speed is two, it will updated every two frames.

I did it this way because updating the ball every frame made it move much too fast (60fps), and using float positions to make it slower made it super jittery since the
internal resolution of the screen is so low. The best solution I could come up with was to just increase the framerate of the ball to speed it up.

Movement of the ball is the same in both directions for a similar reason. Since the resolution is so low, having it move by different x and y
values created a very obvious staircase pattern. Having constant movement in both directions makes it look relatively smooth.

After this, updates are received from the left and right player using either protocol, the paddles being moved accordingly.

If any entities were updated on the current frame, the current game state is sent to both clients - the scores and positions of every entity.

To conclude, the time at the start of the current loop iteration is subtracted from the current time. This is subtracted from the frametime,
(1000/60 ms). The server then sleeps for this amount of time. This keeps the game running at 60 frames per second.


### `void game_loop_client(int sockfd, int (*send_update)(int, int8_t, int8_t, struct addrinfo*), int (*receive_update)(int, game_info*, FILE*), int8_t player, struct addrinfo *serv_info, FILE* csv)`
The general gameloop function for the client side again attempts to be as general as possible
- `int sockfd`- This is the socket file descriptor that the client uses to communicate with the server
- `int (*send_update)` - pointer to a function that sends updates to the server using either protocol
- `int (*receive_update)` - pointer to a function that receives updates from the server using either protocol
- `int8_t player` - The player that this client is - 0 for left, 1 for right
- `struct addrinfo *serv_info` - addrinfo for the server - udp client needs this to send
- `FILE* csv` - This is a pointer to an open csv file, used to record latency values from server to client.


This function first initializes the ncurses view, then goes into an infite loop, similar to the server
- Input is polled directly from the player - mousewheel direction, or 'q', which closes the game
- If any input was registered on the current frame, and update is sent to the server
- Then, an update is received from the server, which modifies the current game state - all game logic is handled on the server side
- The current game state is displayed in ncurses
- The client sleeps for a short time in order to keep the game running at 60 frames per second.


### `void game_loop_host(int server_fd, int (*send_update)(int, int, game_info*, void* client_info), int (*receive_update)(int, int, game_info*, int*, FILE*), void* client_info, FILE* csv)`
This function is for a server that also acts as a client - acts as the left paddle player in the game. It works exactly the same as the server function, except the gamestate is displayed with ncurses and instead of receiving updates from the left player, input is polled directly from
the user.
 

Next, I will go into the send and receive functions implemented by the clients and servers (player server uses the same functions as the regular
server)

# Networking functions

The UDP and TCP communication are both set up using the general client-server functionality in 'Beej's Guide to Network Programming'. Both use IPv4, as I couldn't get communication
to work between the lab computers with IPv6.
- note: tcp uses wrapper functions to make sure the entire message is sent. Basic mechanism as described in lectures - send/recv lenght,
loop until message is fully sent/recvd. `send_all` and `recv_all` in `network_functions.c`
## Server

### Server send
`int (*send_update)(int, int, game_info*, void*)`
This is the general function pointer for sending updates used within the server. In order, the arguments are
- `int server_fd` - the fd used to the udp server to communicate with clients
- `int player` - the player the update will be sent to - 0 for left, 1 for right
- `game_info* g` - the object holding the current game state
- `void* client_info*` - This is a len 2 array which either holds the socket addresses of the clients (udp) or fds for the clients (tcp)

The specific TCP and UDP implementations for this function pointer are defined in `network_functions.c`. They both work mostly the same, so I
will just give a high level walkthrough of how they work.

- Position of each entity position in the gameobject is put into a char buffer
- 32 bit ms timestamp (REAL TIME RATHER THAN MONOTONIC TIME - MUST BE CONSISTENT BETWEEN MACHINES) is appended on end
- This is sent to the client

### server recv
`int (*receive_update)(int, int, game_info*, int*, FILE*)`
General function pointer for receiving updates within the server. Args are:
- `int server_fd` - the fd used to the udp server to communicate with clients
- `int player` - the player the update is received from - for use with TCP, udp can't know beforehand
- `game_info* g` - the object holding the current game state
- `int* client_fds` - len 2 array of file descriptors for tcp clients
- `FILE* csv` - ref to open csv file, used to log latency of client to server

Since udp doesn't need separate file descriptors for receiving, NULL is passed in place of file descriptors. Again, the specific implementations
are defined in `network_functions.c`, here's a general overview

First, whichever fd that is being received from is polled - if there is nothing, the function returns. None of the file descriptors are non-blocking, so this needs to be done to keep the game running smoothly.

The client is the then received from. The timestamp sent from the client in the message is compared to the current time, the latency
between the two being recorded in the csv file. - Receives are attempted until poll indicates there are no more messages. This ensures that each frame, the server has processed
the most up to date message from the clients.

If the timestamp from the clients message is newer than the most recent timestamp from them the server has, their paddle is updated with the
corresponding direction from their message - this makes it so out of order udp messages can be ignored, only the most recent will affect the game.

### client send
`int (*send_update)(int, int8_t, int8_t, struct addrinfo*)`
General function pointer for sending updates within the client.
- `int sockfd` - socket file descriptor client uses to send to server
- `int8_t d` - a direction value to send to the server. -1 for up, 1 for down. Indicates how the client wants to move the paddle
- `int8_t player` - indicates which client is sending the message. 0 for left paddle, 1 for right
- `struct addrinfo* serv_info` - used by udp client to send message to server. TCP client would pass NULL here, and it would go unused in specific function implementation


Both implementations work similarily, here's a general overview
- player, direction, and 32 bit walltime timestamp added to sending buffer
- This is sent to the server

## client recv
`int (*receive_update) (int, game_info*, FILE*)`
- `int sockfd` - socket fd uses to communicate with server
- `game_info* g` - reference to main game_info struct on client side - server will update this
- `FILE* csv` - ref to open csv file, used to log latency of server to client

The client polls the file descriptor (nonblocking, have to do this). If there is something to receive, it is received as a byte string.
Specific indices of the received buffer correspond to the positions of game objects, so these are updated in the game state struct. The last 4
bytes of the buffer are a walltime timestamp sent from the server. The this is subtracted from the current time to record the latency in the csv

- Receives are attempted until poll indicates there are no more messages. This ensures that each frame, the client has the most up to date info about the current
gamestate.


### Main server programs
Both the udp and tcp implementations work the same, so I'll give a general overview.
- Hostname retreived, don't need to add as arg
- portnum taken from console arg
- Server functionality set up, copied from Beej.
- connections are made between two client processes. The first one to connect is made the left player, the second the right player
- the server gameloop function explained earlier is run
- When the loop exits, cleanup is done and the program exits


### Main client programs
- Hostname and port of server taken from console args
- Client functionality set up - Beej
- Connection made with server, who sends back an int8_t corresponding to what player this client is
- main client gameloop function runs
- When loop exits, cleanup is done and program exits.


### Main PlayerServer programs
These work similarly to the server ones, but only one connection is received, which is designated as the right paddle. The main
playerserver gameloop function is run. When this exits, cleanup is done and the program exits.


# Testing
`test.c` contains basic unit testing of the game logic functions. Initialization values, collisions, and paddle movement are checked.
I extensively tested the game manually during developement, using all combinations of server and clients, and am fairly confident there are no major bugs.
