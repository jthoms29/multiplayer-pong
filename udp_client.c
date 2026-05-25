
//John Thoms, gvr812, 11357558
#include <pong.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <stdint.h>
#include <unistd.h>
#include <netdb.h>
#include <poll.h>




int main(int argc, char** argv) {
    if (argc != 3) {
        printf("Usage: ./client <hostname> <port>");
        return 1;
    }

    // our inf
    int sockfd;

    char sendbuf[POSUPDATESIZE];
    char recvbuf[GAMESTATESIZE];
    int8_t player;

    struct addrinfo* p;
    if ((p = udp_client_init(&sockfd, argv[1], argv[2])) == NULL) {
        printf("Failed to intialize client\n");
        return 1;
    }

    if (sendto(sockfd, sendbuf, 1, 0,
        p->ai_addr, p->ai_addrlen) < 0) {
            perror("sendto");
            return 1;
    }

    struct sockaddr_storage their_addr;
    socklen_t addr_len = sizeof their_addr;
    if (recvfrom(sockfd, recvbuf, sizeof(int8_t), 0,
        (struct sockaddr*)&their_addr, &addr_len) < 0) {
            perror("recvfrom");
            return 1;
    }

    player = (int8_t) recvbuf[0];
    
    
    FILE* csv;
    char filename_buf[64];
    sprintf(filename_buf, "./csv/udp_serv_to_cli_%d.csv", player);
    if ((csv = create_file(filename_buf)) == NULL) {
        printf("Error creating csv file\n");
        return 1;
    }

    game_loop_client(sockfd, send_update_udp_client, recv_update_udp_client, player, p, csv);

    close(sockfd);
    fclose(csv);
    return 0;
}

