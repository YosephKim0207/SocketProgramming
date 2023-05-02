#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <pthread.h>
#include <fcntl.h>

#define BUFSIZE 1048576
#define SERVERPORT 7799
#define MAX_THREADS (10)

char hello[] = "서버와 연결 성공\n";
char nofile[] = "서버에 없는 파일입니다. 다시 요청해주세요\n";
pthread_t tid[MAX_THREADS+1];

void GetMyIpAddr(char *ip_buffer)
{
    int fd;
    struct ifreq ifr;
 
    fd = socket(AF_INET, SOCK_DGRAM, 0);
     
    strncpy(ifr.ifr_name, "ens3", IFNAMSIZ);
    
    ioctl(fd, SIOCGIFADDR, &ifr);
    close(fd);
     
    sprintf(ip_buffer, "%s", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
}

//참고: https://0x616b616d61.tistory.com/entry/Linux-시스템서버-IP주소-얻기 [AweSome Life ; )]
//참고: https://pencil1031.tistory.com/67
//https://yjm9205040.tistory.com/entry/ioctl-%ED%95%A8%EC%88%98%EC%99%80-%EC%9D%B8%ED%84%B0%ED%8E%98%EC%9D%B4%EC%8A%A4
//ifconfig 결과 네트워크 인터페이스의 이름이 "ens3" < 해당 부분에 맞추어 함수 일부 수정
//네트워크 인터페이스의 정보를 가져와야 하므로 네트워크 인터페이스의 정보가 담겨있는 ifreq 구조체를 사용
//ifreq 구조체에 얻고자하는 네트워크 인터페이스의 이름인 "ens3"을 입력
//단순히 네트워크 인터페이서의 ip를 가져오는데 있어서 필요 없는 내용 제거
//io장치에 접근하여 장치의 정보를 가져오기 위해 ioctl()에 주소를 확인하는 시스템콜인 SIOCGIFADDR를 이용, ip 주소를 ifr 구조체에 저장

void* filetrans (void *arg) {
    int c_sock = *((int *)arg);
    int *ret, indent = *((int *)arg);
    char filebuf[BUFSIZE] = {0};
    char buf[BUFSIZE] = {0};
    int send_data_size, send_file_size;
    char *send_file_name;
    int fd;

    //1. 클라이언트와 연결수립되면 접속된 클라이언트에게 메세지 전송
    if(send(c_sock, hello, sizeof(hello)+1, 0) == -1){
        perror("Can't send message");
        exit(1);
    }

    printf("클라이언트와 연결 성공\n");

    //2. recv 클라이언트가 요청하는 파일명
    bzero(buf, sizeof(buf));

    if(recv(c_sock, buf, BUFSIZE, 0) == -1) {
        perror("Can't receive request filename");
        exit(1);
    }

    //서버에 클라이언트가 요구하는 파일 유무 확인하기 위한 파일명 변수
    send_file_name = buf;
    printf("클라이언트가 요청한 파일명: %s\n",send_file_name);
    
    //동시 10개 쓰레드 작업시 클라이언트에서fopen시 파일이 덧씌워지는 것을 방지하기 위해 open으로 변경(O_EXCL사용을 위해)
    fd = open(send_file_name, O_RDONLY);

    //해당 파일이 디렉토리에 없는 경우
    if(fd == -1) {
        printf("요청받은 파일은 디렉토리에 없는 파일입니다\n");
      
        if(send(c_sock, nofile, sizeof(nofile), 0) == -1){
            perror("Can't send message");
            close(fd);
            exit(1);
        }
    }
    //파일 전송
    else{
        //3. 전송할 파일 이름 보내주기
        send(c_sock, send_file_name, sizeof(send_file_name), 0);

        //4. 클라이언트로부터 파일 이름 수신여부 확인
        if(recv(c_sock, buf, BUFSIZE, 0) == -1){
            perror("Can't receive file name confirm");
            close(fd);
            exit(1);
        }

        //파일 전송을 위해 읽어드리는 데이터의 크기 확인
        send_data_size = read(fd, filebuf, BUFSIZE);
        printf("read file size : %dbyte\n", send_data_size);
        
        if(send_data_size == -1){
            perror("fread error");
            close(fd);
            exit(1);
        }
        else if (send_data_size == 0) {
            perror("파일 읽는 중 문제 발생\n");
            close(fd);
            exit(1);
        }

        //5. 파일 전송
        if((send_file_size = send(c_sock, filebuf, send_data_size, 0)) == -1){
            perror("send file error");
            close(fd);
            exit(1);
        }
        else if (send_file_size == 0){
            perror("send의 반환값이 0. 정상적으로 파일이 전송되지 않음\n");
            close(fd);
            exit(1);
        }
    }
    //최종 전송된 파일의 데이터 크기 확인
    printf("send %dbyte\n", send_file_size);

    close(fd);
    close(c_sock);
    ret = (int *)malloc(sizeof(int));
    *ret = indent;
    pthread_exit(ret);
}


int main(void) {
    int i, s_sock, c_sock[MAX_THREADS+1], *status;
    struct sockaddr_in server_addr, client_addr;
    socklen_t c_addr_size;
    char server_ip[20] = {0,};
    int t_count = 1;

    //해당 프로그램이 작동되는 서버 ip 받아올 것!
    GetMyIpAddr(server_ip);

    //소켓생성, 리턴값은 소켓의 파일디스크립터
    s_sock = socket(AF_INET, SOCK_STREAM, 0);

    int option = 1;
    setsockopt(s_sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    //memset과 같은 기능인대 0으로 초기화
    //소켓의 주소 구조체를 0으로 초기화
    bzero(&server_addr, sizeof(server_addr));

    //해당 소켓은 네트워크를 이용하는 인터넷 소켓이다
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVERPORT);
    server_addr.sin_addr.s_addr = inet_addr(server_ip); //jcloud에 할당된 ip 받아와서 넣기

    if(bind(s_sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1){
        perror("Can't bind a socket");
        exit(1);
    }

    while(1){

        //동시에 최대 10개의 통신
        listen(s_sock, 10);

        c_addr_size = sizeof(struct sockaddr);

        //c_sock[]에 각 클라이언트의 fd저장 > filetrans함수로 전달해서 사용
        //server를 계속 돌리면서 10개 중 일부가 끝났으면 다른 client가 접근 가능하게 만들건가
        //아니면 10개가 다 끝나면 새롭게 server를 시작하게 만들건가?
        c_sock[t_count] = accept(s_sock, (struct sockaddr *) &client_addr, &c_addr_size);
        if(c_sock[t_count] == -1){
            perror("Can't accept a connection");
            exit(1);
        }

        printf("** Check: s_sock = %d, c_sock = %d\n", s_sock, c_sock[t_count]);
        printf("** Check: client IP addr=%s port=%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        if(pthread_create(&tid[t_count], NULL, &filetrans, &c_sock[t_count])){
            perror("Failed to create thread");
            goto exit;
        }
        printf("서버에 접속한 클라이언트의 누적숫자 : %d\n", t_count);
        t_count++;

        //동시에 10개의 쓰레드만 작업 가능할 경우(11개부터 서버 종료, 10개 이하의 경우 서버 계속 run)
        if(t_count == MAX_THREADS+1){
            printf("서버에 누적 10개의 쓰레드가 작업하였습니다.\n");
            t_count = 1;
        }



    }
    exit:
        for(i=0; i<t_count; i++){
            pthread_join(tid[t_count], (void **) &status);
            printf("Thread no.%d ends: %d\n", t_count, *status);
        }

    close(s_sock);

    return 0;
}