
//John Thoms, gvr812, 11357558

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
int get_client_info(int server_fd, struct sockaddr_storage* client_addrs) {
    
    char sendbuf[16];
    char recvbuf[16];
    struct sockaddr_storage their_addr;
    socklen_t their_len;

    // request from left player
    their_len =  sizeof their_addr;
    ssize_t result = recvfrom(server_fd, recvbuf, 1, 0, (struct sockaddr*)&their_addr, &their_len);
    if (result <= 0) {
        return 1;
    }
    client_addrs[0] = their_addr;
   
    // request from right player
    their_len =  sizeof their_addr;
    result = recvfrom(server_fd, recvbuf, 1, 0, (struct sockaddr*)&their_addr, &their_len);
    if (result <= 0) {
        return 1;
    }
    client_addrs[1] = their_addr;

    // send confirmations to both
    sendbuf[0] = LPLAYER;
    their_len = sizeof(client_addrs[0]);
    if (sendto(server_fd, sendbuf, sizeof(int8_t), 0, (struct sockaddr*)&client_addrs[0], their_len) < 0) {
        return 1;
    }

    sendbuf[0] = RPLAYER;
    their_len = sizeof(client_addrs[1]);
    if (sendto(server_fd, sendbuf, sizeof(int8_t), 0, (struct sockaddr*)&client_addrs[1], their_len) < 0) {
        return 1;
    }
    return 0;
}


int main(int argc, char** argv) {

    if (gethostname(HOSTNAME, 127)) {
        perror("gethostname");
        return 1;
    }

    int udp_server_fd;
    if (udp_server_init(&udp_server_fd, argv[1])) {
        printf("uh oh\n");
        return 1;
    }

    struct sockaddr_storage client_addrs[2];
    get_client_info(udp_server_fd, client_addrs);


    FILE* csv;
    if ((csv = create_file("./csv/udp_cli_to_serv_latency.csv")) == NULL) {
        printf("Error creating csv file\n");
        return 1;
    }

    game_loop_server(udp_server_fd, send_update_udp_server, recv_update_udp_server, client_addrs, csv);

    close(udp_server_fd);
    fclose(csv);
    printf ("closed\n");
    return 0;
}
