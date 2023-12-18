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
#define FILE_BUF_SIZE 256  // 추가: 파일 전송 버퍼 크기
#define MYPORT 3502
#define MAX_CLNT 100

void *handle_clnt(void *arg);
void send_msg(char *msg, int len);
void error_handling(char *msg);
char *serverState(int count);
void send_file(int clnt_sock);  // 추가: 파일 전송 함수

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

    snprintf(msg, BUF_SIZE, "[%s] is Join", name);
    send_msg(msg, strlen(msg));

    while ((str_len = read(clnt_sock, msg, sizeof(msg))) != 0) {
        // 추가: 파일 전송 처리
        if (strncmp(msg, "/upload", strlen("/upload")) == 0) {
            send_file(clnt_sock);
        } else {
            snprintf(msg + str_len, BUF_SIZE - str_len, " [%s]", name);
            send_msg(msg, str_len + strlen(name) + 3);
        }

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

    snprintf(msg, BUF_SIZE, "[%s] is Left", name);
    send_msg(msg, strlen(msg));

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
        strcpy(stateMsg, "Connect Success\n");
    else
        strcpy(stateMsg, "Connect Fail");

    return stateMsg;
}

void send_file(int clnt_sock) {
    char file_name[BUF_SIZE];
    long file_size;

    // 파일명 수신
    if (read(clnt_sock, file_name, sizeof(file_name)) <= 0) {
        perror("read file name error");
        return;
    }

    // 파일 크기 수신
    if (read(clnt_sock, &file_size, sizeof(file_size)) <= 0) {
        perror("read file size error");
        return;
    }

    // 변경된 부분: 파일 저장 경로 설정
    char upload_path[BUF_SIZE];
    snprintf(upload_path, sizeof(upload_path), "sendfile/%s", file_name);
    FILE *file = fopen(upload_path, "wb");
    if (file == NULL) {
        perror("file open error");
        return;
    }

    // 파일 내용 수신 및 저장
    char file_buffer[FILE_BUF_SIZE];
    size_t bytesRead;
    while (file_size > 0) {
        bytesRead = read(clnt_sock, file_buffer, sizeof(file_buffer));
        if (bytesRead <= 0) {
            perror("read file content error");
            break;
        }

        fwrite(file_buffer, 1, bytesRead, file);
        file_size -= bytesRead;
    }

    fclose(file);
    printf("File received: %s\n", upload_path);
}

