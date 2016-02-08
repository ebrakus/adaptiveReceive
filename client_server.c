#define _GNU_SOURCE
#include "utils.h"
#include "client_server.h"

int self_id = 0;

void* run_server_thread(void* client) {
    struct client_socket_info client_info = *(struct client_socket_info*)client;
    fprintf(stderr, "%d, %d\n", client_info.connfd, ntohs(client_info.client_addr.sin_port));

    int n;
    char recvBuff[BATCH_SIZE];
    unsigned long long int sum=0;
    struct timeval start, end;

    /* Take the starting measurement */
    gettimeofday(&start, NULL);
    while((n = read(client_info.connfd, recvBuff, BATCH_SIZE)) > 0) {
        sum+=n;
        pthread_yield();
        /* End measurement */
    }
    gettimeofday(&end, NULL);
    long double diff = find_sec_elapsed(end, start);
    double bw = ((sum*8)/diff)/(1024*1024);
    printf("\n\n(Time elapsed = %Le) - Bandwidth[%d: %d]:%f Mbps\n", diff, self_id, ntohl(client_info.client_addr.sin_addr.s_addr) - BASE_IP, bw);
    
    close(client_info.connfd);  //Closing the connection
    sleep(1);
    return NULL;
}

void* run_server() {
    int listenfd = 0;
    socklen_t len = sizeof(struct sockaddr_in);
    struct sockaddr_in serv_addr;
    pthread_t server_threads[MAX_CLIENT_SUPPORT];
    struct client_socket_info client[MAX_CLIENT_SUPPORT];

    /* Create a TCP socket */
    listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(BASE_SERVER_PORT + self_id); 

    /* Binding the socket to the appropriate IP and port */
    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(struct sockaddr_in)); 
    listen(listenfd, 10); 

    /* Waiting for the client connection */
    int count = 0;
    while(1) {
        client[count].connfd = accept(listenfd, (struct sockaddr*)&(client[count].client_addr), &len);
        fprintf(stderr, "Wait over %d\n", count);
        pthread_create(&server_threads[count], NULL, run_server_thread, (void*)&client[count]);
        count = (count+1)%MAX_CLIENT_SUPPORT;
    }

    return NULL;
}

void* run_client_thread(void* num) {
    int j = 0;
    int rc = 0;
    int fd;
    struct sockaddr_in echoServAddr; 
    char echoString[BATCH_SIZE];
    int serial_no = *(int*)num;

    //rc = find_sockaddr(&echoServAddr, build_domain_name(serial_no), BASE_CLIENT_PORT + serial_no);
    rc = find_sockaddr_temp(&echoServAddr, serial_no, BASE_CLIENT_PORT + serial_no);
    if(rc != 0) {
        fprintf(stderr, "Unable to retrieve sokaddr %d\n", rc);
        return NULL;
    }

    /*
     * Create a 1MB of data to be sent to the server 
     */
    for(j = 0; j< BATCH_SIZE-1; j++){
        echoString[j] = 'h';
    }
    echoString[BATCH_SIZE-1] = 0;

    /* Create a TCP socket */
    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);  
    if(fd == -1){
        fprintf(stderr, "Can't open socket\n");
        return NULL;
    }

    /* Establish the connection to the echo server */
    while (connect(fd, (struct sockaddr *)&echoServAddr, sizeof(struct sockaddr_in)) < 0){
        //fprintf(stderr, "Connect failed\n");
        sleep(1);
    }
    fprintf(stderr, "Connected\n");

    /*
     * Keep sending 1MB of data to the server continuously
     */
    int i = 0;
    while(i < ITERATIONS){
        j = send(fd, echoString, BATCH_SIZE, 0);
        i++;
        pthread_yield();
    }
    /* Gracefully close the connection */
    shutdown(fd, 1);
    return NULL;
}

int main(int argc, char* argv[])
{
    int rc = 0, i;
    void *res;
    pthread_t client_thread[MESH_SIZE], server_thread;
    int serial_no[MESH_SIZE];
    /*char self_hostname[DOMAIN_NAME_SIZE];

    gethostname(self_hostname, DOMAIN_NAME_SIZE - 1);
    self_id = find_id_from_hostname(self_hostname);
    printf("%d\n", self_id);
    if(self_id == 0) {
        fprintf(stderr, "Unexpected hostname: %s\n", self_hostname);
        return 0;
    }*/

    struct in_addr p = find_eth0_ip_address();

    self_id = ntohl(p.s_addr) - BASE_IP;
    printf("Self-id %d\n", self_id);

    for(i = 0; i < MESH_SIZE; i++) {
        if(i == self_id) {
            continue;
        }
        serial_no[i] = i; 
        rc = pthread_create(&client_thread[i], NULL, run_client_thread, (void*)&serial_no[i]);
        if(rc != 0) {
            fprintf(stderr, "Failed to create client thread\n");
            return 0;
        }
    }

    rc = pthread_create(&server_thread, NULL, run_server, (void*)0);
    if(rc != 0) {
        fprintf(stderr, "Failed to create server thread\n");
        return 0;
    }

    pthread_join(server_thread, &res);
    for(i = 0; i < MESH_SIZE; i++) {
        if(i == self_id) {
            continue;
        }
        pthread_join(client_thread[i], &res);
    }
    return 0;
}
