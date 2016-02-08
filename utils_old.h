#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <sys/socket.h>
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

long double find_sec_elapsed(struct timeval end, struct timeval start);
struct in_addr find_eth0_ip_address();
char* build_domain_name(int serial_no);
int find_sockaddr(struct sockaddr_in* address, char* domain_name, int port);
int find_id_from_hostname(char* hostname);
int find_sockaddr_temp(struct sockaddr_in* address, int num, int port);
struct sockaddr_in* build_sockaddr(char* ip_str);
int find_self_id();
