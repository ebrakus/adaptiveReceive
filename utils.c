#include "utils.h"

struct in_addr find_eth0_ip_address() {
    int fd;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);
    ioctl(fd, SIOCGIFADDR, &ifr);
    close(fd);
    //printf("%s\n", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
    return ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;
}

void build_sockaddr(char* ip_str, struct sockaddr_in* address) {
    int ip;
    int rc = inet_pton(AF_INET, ip_str, &ip);
    if(rc != 1) {
        address = NULL;
        return;
    }

    address->sin_family = AF_INET;
    address->sin_addr.s_addr = htonl(ip);
    address->sin_port = htons(BASE_SERVER_PORT);
}

int find_self_id(struct peer* peer_list){
    struct in_addr p = find_eth0_ip_address();
    int self_addr = htonl(p.s_addr);
    int i = 0;

    for(i = 0; i < MESH_SIZE; i++) {
        printf("%d %d\n", peer_list[i].s_addr.sin_addr.s_addr, self_addr);
        if(peer_list[i].s_addr.sin_addr.s_addr == self_addr) {
            return i;
        }
    }
    return -1;
}

int build_peer_list(struct peer* peer_list) {
    FILE *ifp = fopen(IN_FILENAME, "r");
    char hostname[DOMAIN_NAME_SIZE];
    char ip_str[INET_ADDRSTRLEN];
    
    if (ifp == NULL) {
        fprintf(stderr, "Can't open input file\n");
        exit(1);
    }

    int count = 0;
    while(fscanf(ifp, "%s %s", hostname, ip_str) != EOF) {
        struct sockaddr_in* temp = malloc(sizeof(struct sockaddr_in));
        build_sockaddr(ip_str, temp);
        if(temp == NULL) {
            return -1;
        }
        memcpy(peer_list[count].name, hostname, DOMAIN_NAME_SIZE);
        memcpy(&peer_list[count].s_addr, temp, sizeof(struct sockaddr_in));
        printf("%d %s %d\n", count, peer_list[count].name, peer_list[count].s_addr.sin_addr.s_addr);
        count++;
    }

    fclose(ifp);
    return 0;
}
