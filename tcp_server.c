
//John Thoms, gvr812, 11357558
#include <complex.h>
#include <pong.h>
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <ncurses.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <poll.h>

char HOSTNAME[256];




// get requests from two clients, left and right player
int get_client_info(int server_fd, int* client_fds) {

    char sendbuf[16];
    struct sockaddr_storage their_addr;
    socklen_t their_len;

    // request from left player
    client_fds[0] = accept(server_fd, (struct sockaddr*)&their_addr, &their_len);
    if (client_fds[0] == -1) {
        //==TODO==
        return 1;
    }
    client_fds[1] = accept(server_fd, (struct sockaddr*)&their_addr, &their_len);
    if (client_fds[1] == -1) {
        //==TODO==
        return 1;
    }

    sendbuf[0] = LPLAYER;
    if (send_all(client_fds[0], sendbuf, 1)) {
        printf("error\n");
        return 1;
    }
    sendbuf[0] = RPLAYER;
    if (send_all(client_fds[1], sendbuf, 1)) {
        printf("error\n");
        return 1;
    }
    return 0;
}


int main(int argc, char** argv) {

    if (gethostname(HOSTNAME, 127)) {
        perror("gethostname");
        return 1;
    }

    int tcp_server_fd;
    if (tcp_server_init(&tcp_server_fd, argv[1])) {
        printf("uh oh\n");
        return 1;
    }

    int client_fds[2];
    get_client_info(tcp_server_fd, client_fds);

   
    FILE* csv;
    if ((csv = create_file("./csv/tcp_cli_to_serv_latency.csv")) == NULL) {
        printf("Error creating csv file\n");
        return 1;
    }

    game_loop_server(tcp_server_fd, send_update_tcp_server, recv_update_tcp_server, client_fds, csv);

    
    shutdown(client_fds[0], SHUT_RDWR);
    close(client_fds[0]);
    
    shutdown(client_fds[1], SHUT_RDWR);
    close(client_fds[1]);
    fclose(csv);
    return 0;
}
