#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFSIZE 4096
#define SERVERPORT 7799

int main(void) {
    int i, s_sock, c_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t c_addr_size;
    char buf[BUFFSIZE] = {0};
    char hello[] = "Hello~ I am Server!\n";

    pid_t pid, waitfork;
    int proc_count = 0;
    int pidnum[4];
    int wstatus;

    s_sock = socket(AF_INET, SOCK_STREAM, 0);

    int option = 1;
    setsockopt(s_sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    bzero(&server_addr, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVERPORT);
    //server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); < 이건 로컬 ip 주소로 테스트한 것
    server_addr.sin_addr.s_addr = inet_addr("10.0.0.172"); //jcloud에 할당된 ip로 변경

    if(bind(s_sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1){
        perror("[S] Can't bind a socket");
        exit(1);
    }

    //동시에 세 클라이언트까지 접속 가능
    listen(s_sock,3);
    c_addr_size = sizeof(struct sockaddr);

        printf("[S] waiting for a client..\n");
        c_sock = accept(s_sock, (struct sockaddr *) &client_addr, &c_addr_size);
        if(c_sock == -1){
            perror("[S] Can't accept a connection [-1]");
            exit(1);
        }
        else if(c_sock == 0) {
            perror("[S] Can't accept a connetion [0]");
            exit(1);
        }

        //fork 3회 반복
        for(i = 1; i<4; i++){
            pidnum[i] = fork();

            if(pidnum[i] < 0) {   //fork 실패시
            perror("Fork Faild");
            return 1;
            }

            if(pidnum[i] == 0) { //fork 성공시 & child process
                printf("[S] Connected: [%02d번째]client IP addr=%s port=%d PID=%d\n", i, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), getpid());

                //1. say hello to client 연결수립되면 접속된 클라이언트에게 메세지 전송
                if(send(c_sock, hello, sizeof(hello)+1, 0) == -1){
                perror("[S] Can't send message");
                exit(1);
                }

                printf("[S] [%02d번째] PID:%d I said Hello to Client!\n", i, getpid());

                //2. recv msg from client 리시브 호출시 패킷이 도착할 때까지 프로세스가 멈춰서 대기
                if(recv(c_sock, buf, BUFFSIZE, 0) == -1){
                    perror("[S] Can't receive message");
                    exit(1);
                }

                printf("[S] [%02d번째] Client says: %s\n", i, buf);

                close(c_sock);
                exit(0);
            }

            if(i < 3){
                printf("[S] waiting for a client..\n");
                c_sock = accept(s_sock, (struct sockaddr *) &client_addr, &c_addr_size);
                
                if(c_sock == -1){
                    perror("[S] Can't accept a connection [-1]");
                    exit(1);
                }
                else if(c_sock == 0) {
                    perror("[S] Can't accept a connetion [0]");
                    exit(1);
                }
            }
        }    

            for(i=1; i<4; i++){
                waitfork = wait(&wstatus);
                printf("PID %d 종료\n", waitfork);
            }
            
    close(s_sock);

    return 0;
}