#define _GNU_SOURCE
#include "utils.h"

struct sockaddr_in self;
struct client_socket_info client[MESH_SIZE - 1];
int client_count = 0;
int self_id = -1;
long long send_count = 0;

void* run_server(void* map) {
    int listenfd = 0;
    socklen_t len = sizeof(struct sockaddr_in);

    /* Create a TCP socket */
    listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    /* Binding the socket to the appropriate IP and port */
    bind(listenfd, (struct sockaddr*)&self, sizeof(struct sockaddr_in)); 
    listen(listenfd, 10); 

    /* Waiting for the client connection */
    int count = 0;
    while(1) {
        struct sockaddr_in client_addr;
	printf("Listening... %d, %d\n", self.sin_addr.s_addr, self.sin_port);
        int connfd = accept(listenfd, &client_addr, &len);
        //int count = get_value(map, client_addr);
        client[count].connfd = connfd;
        memcpy(&client[count].client_addr, &client_addr, sizeof(struct sockaddr_in));
        count++;
        client_count++;
	printf("Received connection.. %d\n", client_count);
    }
    return NULL;
}

int open_client_connection(struct sockaddr_in echoServAddr) {
    int fd;

    /* Create a TCP socket */
    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);  
    if(fd == -1){
        fprintf(stderr, "Can't open socket\n");
        return fd;
    }

    /* Establish the connection to the echo server */
    printf("Trying to connect %d %d\n", echoServAddr.sin_addr.s_addr, echoServAddr.sin_port);
    while (connect(fd, (struct sockaddr *)&echoServAddr, sizeof(struct sockaddr_in)) < 0){
        //fprintf(stderr, "Connect failed\n");
        sleep(1);
    }

    char ipstr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &echoServAddr.sin_addr, ipstr, sizeof(ipstr));
    printf("Connected -- %s\n", ipstr);

    return fd;
}

void smart_reception_job1() {
    int i, n;
    int num = 1;
    int curr_job = 0;
    int done_clients = 0;
    int client_job[MESH_SIZE-1];
    char recvBuff[BATCH_SIZE];
    struct timeval t, t1;
    gettimeofday(&t, NULL);
    memset(client_job, 0, (sizeof(int))*(MESH_SIZE-1));
    int count = 0;

    //while(curr_job < 2) {
	done_clients = 0;
        /*gettimeofday(&t1, NULL);
	if(count%num == 0) {
	    printf("%f ", time_diff(t, t1));
	    count = 0;
	}*/
        for(i = 0; i < MESH_SIZE-1; i++) {
	    /*if(count == 0) {
                printf("%lld ", client[i].bytes_received);
	    }*/
            if(client_job[i] > curr_job) {
                continue;
            }
            n = read(client[i].connfd, recvBuff, BATCH_SIZE);
            client[i].bytes_received += n;
            if(client[i].bytes_received > JOB_SIZE*(curr_job+1)) {
	        gettimeofday(&t1, NULL);
		printf("Job %d done for client %d %f\n", curr_job, i, time_diff(t, t1));
                client_job[i]++;
            }
        }

	for(i = 0; i < MESH_SIZE-1; i++) {
	    if(client_job[i] > curr_job) {
		done_clients++;
	    }
	}
	if(done_clients == MESH_SIZE-1) {
	    curr_job++;
	}
        /*if(count == 0) {
            printf("\n");
	}
	count++;*/
    //}

   /* gettimeofday(&t1, NULL);
    for(i = 0; i < MESH_SIZE-1; i++) {
	printf("%lld ", client[i].bytes_received);
    }
    printf("\n %lld, %f\n", send_count, time_diff(t, t1));
    return NULL;*/
}

void* run_client(void* fd) {
    int peer_fd[MESH_SIZE];
    int i = 0, j = 0;
    char echoString[BATCH_SIZE];
    int iof = -1;
    int count = 0;

    memcpy(peer_fd, fd, 4*MESH_SIZE);

    /*
     * Create a 1MB of data to be sent to the server 
     */
    for(j = 0; j< BATCH_SIZE-1; j++){
        echoString[j] = 'h';
    }
    echoString[BATCH_SIZE-1] = 0;

    /*
     * Keep sending 1MB of data to each server continuously
     */
    //while(count < ITERATIONS){
    while(1){
        for(j = 0; j < MESH_SIZE; j++) {
	    if(j == self_id) {
     		continue;
	    }
	    int temp_fd = peer_fd[j];
	    //printf("%d\n", temp_fd);
	    if ((iof = fcntl(temp_fd, F_GETFL, 0)) != -1)
	        fcntl(temp_fd, F_SETFL, iof | O_NONBLOCK);
	    // receive
	    int k = send(temp_fd, echoString, BATCH_SIZE, 0);
	    // set as before
	    if (iof != -1)
	        fcntl(temp_fd, F_SETFL, iof);
	    if(k == -1) {
		//printf("Error in sending data to %d with error %d: peer_fd=%d\n", j, errno, temp_fd);
	    }else {
	        send_count += k;
		count++;
	    }
        }
	if(count == ITERATIONS) {
	    sleep(0.01);
	}
        pthread_yield();
    }

    /* Gracefully close the connection */
    for(j = 0; j < MESH_SIZE; j++) {
        shutdown(peer_fd[j], 1);
    }
    return NULL;
}

void* smart_reception() {
    /* Walk all the sockets to see if data is available */
    int i, result = 0;
    fd_set readset;
    struct timeval t;
    t.tv_sec = 0;
    t.tv_usec = 0;

    int maxfd = 0;
    FD_ZERO(&readset);
    for(i = 0; i < MESH_SIZE-1; i++) {
        int connfd = client[i].connfd;
        FD_SET(connfd, &readset);
        maxfd = (maxfd > connfd) ? maxfd : connfd;
    }

    //printf("Before calling select %d\n", readset);
    result = select(maxfd + 1, &readset, NULL, NULL, &t);      //select returns immediately
    if(result == -1) {
        printf("Error in select %d\n", errno);
    } else {
        /* Put selective reading logic */
        //printf("After calling select %d\n", readset);
        for(i = 0; i < MESH_SIZE; i++) {
            if(FD_ISSET(client[i].connfd, &readset)) {
                printf("%d.. is set\n", i);
            }
        }
    }
    return NULL;
}

void* normal_reception() {
    int i;
    int n = 0;
    char recvBuff[BATCH_SIZE];
    int count = 0;
    struct timeval t, t1;
    gettimeofday(&t, NULL);

    while(1) {
        gettimeofday(&t1, NULL);
	if(count%5 == 0) {
	    printf("%f ", time_diff(t, t1));
	    count = 0;
	}
        for(i = 0; i < MESH_SIZE-1; i++) {
            n = read(client[i].connfd, recvBuff, BATCH_SIZE);
            client[i].bytes_received += n;
	    if(count == 0) {
                printf("%lld ", client[i].bytes_received);
	    }
        }
        if(count == 0) {
            printf("\n");
	}
	count++;
    }
    return NULL;
}
 
void* smart_reception_ioctl() {
    int i;
    int count1[MESH_SIZE-1];
    int max = 0;
    int min = 0;
    int n = 0;
    char recvBuff[BATCH_SIZE];
    int data_to_read = BATCH_SIZE;
    int skip_max_min = 0;
    int count = 0;
    struct timeval t, t1;
    gettimeofday(&t, NULL);

    while(1) {
        skip_max_min = 0;
        gettimeofday(&t1, NULL);
	if(count%1000 == 0) {
	    printf("%f ", time_diff(t, t1));
	    count = 0;
	}
        for(i = 0; i < MESH_SIZE-1; i++) {
            ioctl(client[i].connfd, FIONREAD, &count1[i]);
	    if(count == 0) {
                printf("%lld ", client[i].bytes_received);
	    }
        }
        if(count == 0) {
            printf("\n");
	}

        if(is_max_min_far(client, max, min, MAX_DELTA)) {
            data_to_read = client[max].bytes_received;

            n = read(client[min].connfd, recvBuff, count1[min] > BATCH_SIZE ? BATCH_SIZE : count1[min]);
            client[min].bytes_received += n;
            if(client[min].bytes_received > data_to_read) {
                data_to_read = client[min].bytes_received;
            }
            skip_max_min = 1;
        }

        for(i = 0; i < MESH_SIZE-1; i++) {
            if((i == max || i == min) && skip_max_min == 1) {
                continue;
            }
            int temp_len = BATCH_SIZE;
            if(data_to_read - skip_max_min*client[i].bytes_received > count1[i]) {
                temp_len = count1[i];
            }else {
                temp_len = data_to_read - skip_max_min*client[i].bytes_received;
            }
            n = read(client[i].connfd, recvBuff, temp_len > BATCH_SIZE ? BATCH_SIZE : temp_len);
            client[i].bytes_received += n;
        }

        find_min_max(client, &max, &min);
	count++;
    }
    return NULL;
}

#if 0
void run_client_temp(char* echoString) {
    int j = 0;
    int iof = -1;

    /*
     * Keep sending 1MB of data to each server continuously
     */
    //while(i < ITERATIONS){
        for(j = 0; j < MESH_SIZE; j++) {
	    if(j == self_id) {
     		continue;
	    }
	    int temp_fd = peer_fd[j];
	    //printf("%d\n", temp_fd);
	    if ((iof = fcntl(temp_fd, F_GETFL, 0)) != -1)
	        fcntl(temp_fd, F_SETFL, iof | O_NONBLOCK);
	    // receive
	    int k = send(temp_fd, echoString, BATCH_SIZE, 0);
	    // set as before
	    if (iof != -1)
	        fcntl(temp_fd, F_SETFL, iof);
	    if(k == -1) {
		//printf("Error in sending data to %d with error %d: peer_fd=%d\n", j, errno, temp_fd);
	    }else {
	        send_count += k;
	    }
        }
}
#endif

void* smart_reception_step_job() {
    int i, n;
    int num = 10;
    int curr_job = 0;
    int done_clients = 0;
    int client_job[MESH_SIZE-1];
    char recvBuff[BATCH_SIZE];
    struct timeval t, t1;
    gettimeofday(&t, NULL);
    memset(client_job, 0, (sizeof(int))*(MESH_SIZE-1));
    int count = 0;

    int s = 5;
    int peak = JOB_SIZE;
    int curr_step = 0;

    while(curr_job < 10) {
	done_clients = 0;
        gettimeofday(&t1, NULL);
	if(count%num == 0) {
	    printf("%f ", time_diff(t, t1));
	    count = 0;
	}

        for(i = curr_step*s; i < (curr_step+1)*s-1; i++) {
	    if(count == 0) {
                printf("%lld ", client[i].bytes_received);
	    }
            if(client_job[i] > curr_job) {
                continue;
            }
            n = read(client[i].connfd, recvBuff, BATCH_SIZE);
            client[i].bytes_received += n;
            if(client[i].bytes_received > peak*(curr_job+1)) {
	        gettimeofday(&t1, NULL);
		//printf("Job %d done for client %d %f\n", curr_job, i, time_diff(t, t1));
                client_job[i]++;
            }
        }

        int temp = 0;
        for(i = curr_step*s; i < (curr_step+1)*s-1; i++) {
	    if(client_job[i] > curr_job) {
		temp++;
	    }
	}
        if(temp == s) {
            curr_step++;
        }

	for(i = 0; i < MESH_SIZE-1; i++) {
	    if(client_job[i] > curr_job) {
		done_clients++;
	    }
	}
	if(done_clients == MESH_SIZE-1) {
	    curr_job++;
            curr_step = 0;
	}
        if(count == 0) {
            printf("\n");
	}
	count++;
    }

    gettimeofday(&t1, NULL);
    for(i = 0; i < MESH_SIZE-1; i++) {
	printf("%lld ", client[i].bytes_received);
    }
    printf("\n %lld, %f\n", send_count, time_diff(t, t1));
    return NULL;
}

void* smart_reception_job() {
    int i, n;
    int num = 10;
    int curr_job = 0;
    int done_clients = 0;
    int client_job[MESH_SIZE-1];
    char recvBuff[BATCH_SIZE];
    struct timeval t, t1;
    gettimeofday(&t, NULL);
    memset(client_job, 0, (sizeof(int))*(MESH_SIZE-1));
    int count = 0;

    while(curr_job < 10) {
	done_clients = 0;
        gettimeofday(&t1, NULL);
	if(count%num == 0) {
	    printf("%f ", time_diff(t, t1));
	    count = 0;
	}
        for(i = 0; i < MESH_SIZE-1; i++) {
	    if(count == 0) {
                printf("%lld ", client[i].bytes_received);
	    }
            if(client_job[i] > curr_job) {
                continue;
            }
            n = read(client[i].connfd, recvBuff, BATCH_SIZE);
            client[i].bytes_received += n;
            if(client[i].bytes_received > JOB_SIZE*(curr_job+1)) {
	        gettimeofday(&t1, NULL);
		//printf("Job %d done for client %d %f\n", curr_job, i, time_diff(t, t1));
                client_job[i]++;
            }
        }

	for(i = 0; i < MESH_SIZE-1; i++) {
	    if(client_job[i] > curr_job) {
		done_clients++;
	    }
	}
	if(done_clients == MESH_SIZE-1) {
	    curr_job++;
	}
        if(count == 0) {
            printf("\n");
	}
	count++;
    }

    gettimeofday(&t1, NULL);
    for(i = 0; i < MESH_SIZE-1; i++) {
	printf("%lld ", client[i].bytes_received);
    }
    printf("\n %lld, %f\n", send_count, time_diff(t, t1));
    return NULL;
}
            


int main(int argc, char* argv[])
{
    int rc = 0, i;
    int res;
    struct peer peer_list[MESH_SIZE];
    pthread_t client_thread, server_thread, smart_reception_thread;
    int peer_fd[MESH_SIZE];

    memset(client, 0, sizeof(struct client_socket_info)*MESH_SIZE-1);
    rc = build_peer_list(peer_list);
    if(rc != 0) {
        fprintf(stderr, "Failed to build peer list\n");
        return 0;
    }

    self_id = find_self_id(peer_list);
    printf("Self-id %d\n", self_id);

    memcpy(&self, &peer_list[self_id].s_addr, sizeof(struct sockaddr_in));

    void* map = NULL;
    rc = pthread_create(&server_thread, NULL, run_server, (void*)map);
    if(rc != 0) {
        fprintf(stderr, "Failed to create server thread\n");
        return 0;
    }

    /* Open client connections to all the other servers */
    for(i = 0; i < MESH_SIZE; i++) {
        if(i == self_id) {
            continue;
        }
        peer_fd[i] = open_client_connection(peer_list[i].s_addr);
	printf("[%d]: %d\n", i, peer_fd[i]);
    }

    while(client_count < MESH_SIZE-1) {
    }
    printf("client count %d\n", client_count);

    rc = pthread_create(&client_thread, NULL, run_client, (void*)peer_fd);
    if(rc != 0) {
        fprintf(stderr, "Failed to create client thread\n");
        return 0;
    }

    rc = pthread_create(&smart_reception_thread, NULL, smart_reception_job, (void*)0);
    //rc = pthread_create(&smart_reception_thread, NULL, normal_reception, (void*)0);
    if(rc != 0) {
        fprintf(stderr, "Failed to create smart reception thread\n");
        return 0;
    }

    pthread_join(server_thread, (void*)&res);
    pthread_join(client_thread, &res);
    pthread_join(smart_reception_thread,(void*) &res);
    return 0;
}
