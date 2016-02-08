#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include "config.h"

struct client_socket_info {
    int connfd;
    struct sockaddr_in client_addr;
    int bytes_received;
};

struct peer {
    char name[256];
    struct sockaddr_in s_addr;
};

struct in_addr find_eth0_ip_address();
void build_sockaddr(char* ip_str, struct sockaddr_in* address);
int find_self_id(struct peer* peer_list);
int build_peer_list(struct peer* peer_list);
