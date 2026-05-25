//John Thoms, gvr812, 11357558

#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <pong.h>
#include <poll.h>
#include <assert.h>

#define BACKLOG 20


void sigchld_handler(int s) {
    (void)s;

    //waitpid may overwrite errno, save/restore
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}

// get sockaddr, IPv4 or 6
void* get_in_addr(struct sockaddr *sa) {

    //ipv4
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    //ipv6
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}



int udp_server_init(int* sockfd, char* port) {
    struct addrinfo hints, *servinfo, *p;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // or set to AF_INET6 to use IPv6
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((*sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }

        if (bind(*sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(*sockfd);
            perror("listener: bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servinfo);
    return 0;
}




struct addrinfo* udp_client_init(int* sockfd, char* hostname, char* udp_port) {
    struct addrinfo hints, *servinfo, *p;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // set to AF_INET6 to use IPv6
    hints.ai_socktype = SOCK_DGRAM;

    rv = getaddrinfo(hostname, udp_port, &hints, &servinfo);
    if (rv != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return NULL;
    }

    // loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((*sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("talker: socket");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "talker: failed to create socket\n");
        return NULL;
    }

    return p;
}



int tcp_server_init(int* sockfd, char* port) {
    struct addrinfo hints, *servinfo, *p;
    struct sigaction sa;
    int yes=1;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((*sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(*sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(*sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(*sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    return 0;
}

int tcp_client_init(int* sockfd, char* hostname, char* port) {
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];


    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((*sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        inet_ntop(p->ai_family,
            get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);
        printf("client: attempting connection to %s\n", s);


        if (connect(*sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("client: connect");
            close(*sockfd);
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family,
            get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);
    printf("client: connected to %s\n", s);

    freeaddrinfo(servinfo); // all done with this structure
    return 0;
}
/*
 * This function ensures that total expected amount of data is sent
 */
int send_all(int fd, void* data, size_t size) {

	// first of all, send the size of the incoming data as a 32 bit int. Reciever
	// is expecting exactly 4 bytes

	uint32_t b32_size = (uint32_t) size;
	uint32_t network_size = htonl(b32_size);


	// uint32_t to bytes, allows me to send remainder if the whole thing doesn't
	// go through
	uint8_t size_buf[4];
	memcpy(size_buf, &network_size, 4);

	size_t sent, total_sent = 0;
	do {
	 	sent = send(fd, size_buf+total_sent, sizeof(uint32_t) - total_sent, 0);
		if (sent == -1 || sent == 0) {
			//perror("send");
            return -1;
		}
		total_sent += sent;
	} while (total_sent != sizeof(uint32_t));


	
	// now that the reciever knows how many bytes to expect, send the actual data
	// Anything that needs to be converted to network order should be converted beforehand, this
	// function doesn't know anything about the actual data
	sent = 0, total_sent = 0;

	unsigned char* data_buf = (unsigned char*) data;
	do  {
		sent = send(fd, data_buf+total_sent, size-total_sent, 0);
		if (sent == -1 || sent == 0) {
			//perror("send");
			return -1;
		}
		total_sent += sent;
	} while (total_sent != size);

	return 0;
}




/*
 * Ensures proper amount of data is received.
 */
int recv_all(int fd, void* recv_buf, size_t max_size) {

	//first, recieve intital 32 bit data size from sender. This way, the exact amount
	//of data that should be received is known
	uint32_t expected_size;
	char size_buf[4];
	size_t recvd, total_recvd = 0;
	do {
		recvd = recv(fd, size_buf+total_recvd, sizeof(uint32_t)-total_recvd, 0);
		if (recvd == -1) {
			//perror("recv");
			return -1;
		}
        if (recvd == 0) {
            return -1;
        }
		total_recvd+=recvd;
	} while (total_recvd != sizeof(uint32_t));

	memcpy(&expected_size, size_buf, 4);
	expected_size = ntohl(expected_size);




	// supplied buffer can't handle incoming data
	if (expected_size > max_size) {
		return 0;
	}




	// now, the size of the incoming data is known. We can recieve it
	total_recvd = 0;
	unsigned char* network_buf = (unsigned char*) recv_buf;
	do {
		recvd = recv(fd, network_buf+total_recvd, expected_size-total_recvd, 0);
		if (recvd == -1) {
			perror("recv");
            return -1;
		}
        if (recvd == 0) {
             return -1;
        }
		total_recvd+=recvd;

	} while (total_recvd != expected_size);
	
	return total_recvd;
}


int send_update_udp_server(int server_fd, int player, game_info* g, void* client_info) {

    static char sendbuf[GAMESTATESIZE];

    assert(player == 0 || player == 1);
    // their info
    sendbuf[0] = g->ball->left;
    sendbuf[1] = g->ball->top;

    sendbuf[2] = g->l_paddle->left;
    sendbuf[3] = g->l_paddle->top;

    sendbuf[4] = g->r_paddle->left;
    sendbuf[5] = g->r_paddle->top;

    sendbuf[6] = g->l_score;
    sendbuf[7] = g->r_score;

    uint32_t ts = wall_ms();
    ts = htonl(ts);
    memcpy(sendbuf+8, &ts, sizeof(uint32_t));

    struct sockaddr_storage *addrs = (struct sockaddr_storage*) client_info;

    socklen_t addr_len = sizeof(addrs[player]);
    if (sendto(server_fd, sendbuf, GAMESTATESIZE, 0, (struct sockaddr*)&addrs[player], addr_len) < 0) {
        perror("sendto");
        return -1;
    }
    return 0;
}


int recv_update_udp_server(int server_fd, int player, game_info* g, int* client_fds, FILE* csv) {
    static char recvbuf[POSUPDATESIZE];
    // most recent timestamps for left and right player
    static uint32_t timestamps[2];

    struct pollfd pfd;
    pfd.fd = server_fd;
    pfd.events = POLLIN | POLLHUP | POLLERR;

    int updated = 0;
    for(;;) {
        int ret = poll(&pfd, 1, 0);
            // err
        if (ret < 0) { return -1; }
        // nothing to recv

        if (ret == 0 || !(pfd.revents & POLLIN)) { break;}
        
        struct sockaddr_storage their_addr;
        socklen_t their_len;
        their_len =  sizeof their_addr;
        ssize_t result = recvfrom(server_fd, recvbuf, POSUPDATESIZE, 0, (struct sockaddr*)&their_addr, &their_len);
        if (result < 0) {
            printf("dropped\n");
            return 0;
        }

        int8_t from = recvbuf[0];
        int8_t dir = recvbuf[1];

        uint32_t tstamp;
        memcpy(&tstamp, recvbuf+2, sizeof(uint32_t));
        tstamp = ntohl(tstamp);

        uint32_t cur_time = wall_ms();
        uint32_t latency = cur_time - tstamp;
        write_line(csv, tstamp, cur_time, latency);


        // if timestamp greater than currently stored timestamp for this player, use the input
        if (from == LPLAYER) {
            if (tstamp > timestamps[LPLAYER]) {
                move_paddle(g->l_paddle, dir);
                timestamps[LPLAYER] = tstamp;
                updated = 1;
            }
        }
        else if (from == RPLAYER) {
            if (tstamp > timestamps[RPLAYER]) {
                move_paddle(g->r_paddle, dir);
                timestamps[RPLAYER] = tstamp;
                updated = 1;
            }
        }
    }
    return updated;
}


int send_update_tcp_server(int server_fd, int player, game_info* g, void* client_info) {

    static char sendbuf[GAMESTATESIZE];
    assert(player == 0 || player == 1);

    // their info
    sendbuf[0] = g->ball->left;
    sendbuf[1] = g->ball->top;

    sendbuf[2] = g->l_paddle->left;
    sendbuf[3] = g->l_paddle->top;

    sendbuf[4] = g->r_paddle->left;
    sendbuf[5] = g->r_paddle->top;

    sendbuf[6] = g->l_score;
    sendbuf[7] = g->r_score;

    uint32_t ts = wall_ms();
    ts = ntohl(ts);
    memcpy(sendbuf+8, &ts, sizeof(uint32_t));

    int* fds = (int*) client_info;

    if (send_all(fds[player], sendbuf, GAMESTATESIZE) < 0) {
        return -1;
    }
    return 0;
}


int recv_update_tcp_server(int server_fd, int player, game_info* g, int* client_fds, FILE* csv) {
    static char recvbuf[POSUPDATESIZE];

    assert(client_fds != NULL);
    struct pollfd pfd;
    pfd.fd = client_fds[player];
    pfd.events = POLLIN | POLLHUP | POLLERR;


    int updated = 0;
    for (;;) {
        int ret = poll(&pfd, 1, 0);
        // err
        if (ret < 0) { return -1; }
        // nothing to recv
        if (!(pfd.revents & POLLIN)) { break;}

        ssize_t result = recv_all(client_fds[player], recvbuf, POSUPDATESIZE);
        if (result < 0) {
            return -1;
        }
        int8_t from = recvbuf[0];
        int8_t dir = recvbuf[1];

        uint32_t tstamp;
        memcpy(&tstamp, recvbuf+2, sizeof(uint32_t));
        tstamp = ntohl(tstamp);

        uint32_t cur_time = wall_ms();
        uint32_t latency = cur_time - tstamp;
        write_line(csv, tstamp, cur_time, latency);

        // no timestamp check needed with tcp
        if (from == LPLAYER) {
            move_paddle(g->l_paddle, dir);
            updated = 1;
        }
        else if (from == RPLAYER) {
            move_paddle(g->r_paddle, dir);
            updated = 1;
        }
    }
    return updated;
}


int send_update_udp_client(int sockfd, int8_t d, int8_t player, struct addrinfo *serv_info) {
    static char sendbuf[POSUPDATESIZE];

    // send which player, direction of input, and timestamp
    sendbuf[0] = player;
    sendbuf[1] = d;
    uint32_t tstamp = wall_ms();
    tstamp = htonl(tstamp);
    memcpy(sendbuf+2, &tstamp, sizeof(uint32_t));

    if (sendto(sockfd, sendbuf, POSUPDATESIZE, 0,
        serv_info->ai_addr, serv_info->ai_addrlen) < 0) {
        return 1;
    }
    return 0;
}



int recv_update_udp_client(int sockfd, game_info* g, FILE* csv) {
    static char recvbuf[GAMESTATESIZE];
    // most recent timestamp received from server. Will update only if more recent than current
    static uint32_t rec_timestamp;

    struct pollfd pfd;
    pfd.fd = sockfd;
    pfd.events = POLLIN | POLLHUP | POLLERR;

    int updated = 0;
    for (;;) {
        int ret = poll(&pfd, 1, 0);
        // err
        if (ret < 0) { return -1; }
        // nothing to recv
        if (ret == 0 || !(pfd.revents & POLLIN)) { break;}

        struct sockaddr_storage their_addr;
        socklen_t addr_len = sizeof their_addr;
        if (recvfrom(sockfd, recvbuf, GAMESTATESIZE, 0,
            (struct sockaddr*)&their_addr, &addr_len) < 0) {
            return 1;
        }

        uint32_t received_ts;
        memcpy(&received_ts, recvbuf+8, sizeof(uint32_t));
        received_ts = ntohl(received_ts);

        uint32_t cur_time = wall_ms();
        uint32_t latency = cur_time - received_ts;

        write_line(csv, received_ts, cur_time, latency);

        // update only if newer than current update
        if (received_ts >= rec_timestamp) {
            g->ball->left = recvbuf[0];
            g->ball->top = recvbuf[1];

            g->l_paddle->left = recvbuf[2];
            g->l_paddle->top = recvbuf[3];

            g->r_paddle->left = recvbuf[4];
            g->r_paddle->top = recvbuf[5];

            g->l_score = recvbuf[6];
            g->r_score = recvbuf[7];

            rec_timestamp = received_ts;
            updated = 1;
        }

    }
    return updated;
}

int send_update_tcp_client(int sockfd, int8_t d, int8_t player, struct addrinfo *serv_info) {

    static char sendbuf[POSUPDATESIZE];
    // send which player, direction of input, and timestamp
    sendbuf[0] = player;
    sendbuf[1] = d;
    uint32_t tstamp = wall_ms();
    uint32_t net_tstamp = htonl(tstamp);
    memcpy(sendbuf+2, &net_tstamp, sizeof(uint32_t));

    if (send_all(sockfd, sendbuf, POSUPDATESIZE) < 0) {
        printf("what\n");
        return -1;
    }
    return 0;
}

int recv_update_tcp_client(int sockfd, game_info* g, FILE* csv) {
    static char recvbuf[GAMESTATESIZE];

    struct pollfd pfd;
    pfd.fd = sockfd;
    pfd.events = POLLIN | POLLHUP | POLLERR;

    int received = 0;
    for (;;) {
        int ret = poll(&pfd, 1, 0);
        // err
        if (ret < 0) { return -1; }
        // nothing to recv
        if (!(pfd.revents & POLLIN)) { break;}


        if (recv_all(sockfd, recvbuf, GAMESTATESIZE) <= 0) {
            return -1;
        }
        received = 1;

        g->ball->left = recvbuf[0];
        g->ball->top = recvbuf[1];

        g->l_paddle->left = recvbuf[2];
        g->l_paddle->top = recvbuf[3];

        g->r_paddle->left = recvbuf[4];
        g->r_paddle->top = recvbuf[5];

        g->l_score = recvbuf[6];
        g->r_score = recvbuf[7];

        uint32_t received_ts;
        memcpy(&received_ts, recvbuf+8, sizeof(uint32_t));
        received_ts = ntohl(received_ts);

        uint32_t cur_time = wall_ms();
        uint32_t latency = cur_time - received_ts;
        write_line(csv, received_ts, cur_time, latency);
    }

    return received;
}
