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
    int c_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t c_addr_size;
    char buf[BUFFSIZE] = {0};
    char hello[] = "Hi~ I am Client!!\n";

    c_sock = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&server_addr, sizeof(server_addr)); //memset처럼 0로 만듦

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVERPORT);
    //server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_addr.s_addr = inet_addr("10.0.0.172");

    printf("[C] Connecting...\n");

    if(connect(c_sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1){ //지정된 서버 주소로 접속
        perror("[C] Can't connet a Server");
        exit(1);
    }

    printf("[C] Connected!\n");

    //1. recv msg from server (maybe it's "hello") 서버가 말할걸 기대하고 리시브부터 호출, 대기 버프를 통해 데이터를 받음
    //리시브 호출시 패킷이 도착할 때까지 프로세스가 멈춰서 대기
    
    if(recv(c_sock, buf, BUFFSIZE, 0) == -1){
        perror("[C] Can't receive message");
        exit(1);
    }
    
    printf("[C] Server says: %s\n", buf);
    
    //2. say hi to server 클라이언트가 서버에게 클라와 동일한 방식으로 샌드
    if(send(c_sock, hello, sizeof(hello)+1, 0) == -1){
        perror("[C] Can't send message");
        exit(1);
    }
    printf("[C] I said Hi to Server!\n");

    //printf("[C] I am going to sleep...\n");
    //sleep(10);

    close(c_sock);

    return 0;
}