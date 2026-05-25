
//John Thoms, gvr812, 11357558

#include <pong.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <stdint.h>
#include <unistd.h>
#include <netdb.h>
#include <poll.h>
#include <netinet/tcp.h>



int main(int argc, char** argv) {
    if (argc != 3) {
        printf("Usage: ./client <hostname> <port>");
        return 1;
    }

    // our info
    int sockfd;

    char recvbuf[GAMESTATESIZE];
    int8_t player;

    if ((tcp_client_init(&sockfd, argv[1], argv[2]))) {
        printf("Failed to intialize client\n");
        return 1;
    }


    if (recv_all(sockfd, recvbuf, sizeof(int8_t)) <= 0) {
            perror("recvfrom");
            return 1;
    }
    player = (int8_t) recvbuf[0];


    FILE* csv;
    char filename_buf[64];
    sprintf(filename_buf, "./csv/tcp_serv_to_cli_%d.csv", player);
    if ((csv = create_file(filename_buf)) == NULL) {
        printf("Error creating csv file\n");
        return 1;
    }

    // turn off nagle
    int opt = 1;
    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));

    game_loop_client(sockfd, send_update_tcp_client, recv_update_tcp_client, player, NULL, csv);

    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);
    fclose(csv);
    return 0;
}
