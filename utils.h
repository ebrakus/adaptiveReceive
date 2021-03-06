#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
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
    long long bytes_received;
};

struct peer {
    char name[DOMAIN_NAME_SIZE];
    struct sockaddr_in s_addr;
};

struct in_addr find_eth0_ip_address();
void build_sockaddr(char* ip_str, struct sockaddr_in* address);
int find_self_id(struct peer* peer_list);
int build_peer_list(struct peer* peer_list);
bool is_max_min_far(struct client_socket_info* client, int max, int min, int max_delta);
void find_min_max(struct client_socket_info* client, int* max, int* min);
double time_diff(struct timeval t1, struct timeval t2);
