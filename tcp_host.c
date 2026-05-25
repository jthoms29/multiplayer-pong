
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
#include <netinet/tcp.h>

char HOSTNAME[256];




// get requests from one client, right player
int get_client_info(int server_fd, int* client_fds) {

    char sendbuf[16];
    struct sockaddr_storage their_addr;
    socklen_t their_len;

    client_fds[1] = accept(server_fd, (struct sockaddr*)&their_addr, &their_len);
    if (client_fds[1] == -1) {
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
        return 1;
    }

    int client_fds[2];
    get_client_info(tcp_server_fd, client_fds);

    FILE* csv;
    if ((csv = create_file("./csv/tcp_player_serv_latency.csv")) == NULL) {
        printf("Error creating csv file\n");
        return 1;
    }

    // turn off nagle
    int opt = 1;
    setsockopt(client_fds[1], IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));

    game_loop_host(tcp_server_fd, send_update_tcp_server, recv_update_tcp_server, client_fds, csv);



    shutdown(client_fds[1], SHUT_RDWR);
    close(client_fds[1]);
    fclose(csv);
    return 0;
}
