#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>

#define BUF_SIZE 100
#define MYPORT 3428
#define MAX_CLNT 100

void *handle_clnt(void *arg);
void send_msg(char *msg, int len);
void error_handling(char *msg);
char *serverState(int count);

int clnt_cnt = 0;
int clnt_socks[MAX_CLNT];
pthread_mutex_t mutx;

int main(int argc, char *argv[]) {
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    int clnt_adr_sz;
    pthread_t t_id;

    pthread_mutex_init(&mutx, NULL);
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);

    if (serv_sock == -1)
        error_handling("socket() error");

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(MYPORT);

    if (bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("bind() error");

    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    printf("Server is running\n");

    while (1) {
        time_t timer = time(NULL);
        struct tm *t = localtime(&timer);
        clnt_adr_sz = sizeof(clnt_adr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);

        pthread_mutex_lock(&mutx);
        clnt_socks[clnt_cnt++] = clnt_sock;
        pthread_mutex_unlock(&mutx);

        pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock);
        pthread_detach(t_id);
        printf(" Connected client IP : %s ", inet_ntoa(clnt_adr.sin_addr));
        printf("(%d-%d-%d %d:%d)\n", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
               t->tm_hour, t->tm_min);
        printf(" chatter (%d/100)\n", clnt_cnt);

        char *stateMsg = serverState(clnt_cnt);
        write(clnt_sock, stateMsg, strlen(stateMsg));
        free(stateMsg);
    }

    close(serv_sock);
    return 0;
}

void *handle_clnt(void *arg) {
    int clnt_sock = *((int*)arg);
    int str_len = 0;
    char name[BUF_SIZE];
    char msg[BUF_SIZE];

    if ((str_len = read(clnt_sock, name, sizeof(name))) == 0) {
        perror("read name error");
        close(clnt_sock);
        return NULL;
    }
    name[str_len] = '\0';

    while ((str_len = read(clnt_sock, msg, sizeof(msg))) != 0) {
        snprintf(msg + str_len, BUF_SIZE - str_len, " [%s]", name);
        send_msg(msg, str_len + strlen(name) + 3);

        memset(msg, 0, sizeof(msg));
    }

    pthread_mutex_lock(&mutx);
    for (int i = 0; i < clnt_cnt; i++) {
        if (clnt_sock == clnt_socks[i]) {
            while (i++ < clnt_cnt - 1)
                clnt_socks[i] = clnt_socks[i + 1];
            break;
        }
    }
    clnt_cnt--;
    pthread_mutex_unlock(&mutx);
    close(clnt_sock);
    return NULL;
}

void send_msg(char *msg, int len) {
    int i;
    pthread_mutex_lock(&mutx);
    for (i = 0; i < clnt_cnt; i++)
        write(clnt_socks[i], msg, len);
    pthread_mutex_unlock(&mutx);
}

void error_handling(char *msg) {
    perror(msg);
    exit(1);
}

char *serverState(int count) {
    char *stateMsg = malloc(sizeof(char) * 20);
    if (stateMsg == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }
    strcpy(stateMsg, "None");

    if (count < 5)
        strcpy(stateMsg, "Good");
    else
        strcpy(stateMsg, "Bad");

    return stateMsg;
}

