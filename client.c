//파일 하나를 요청하고, 서버로부터 해당 파일을 수신하는 프로그램
//명령행인자 : 접속할 서버의 ip주소, 포트번호, 요청할 파일명(경로는 생략하고 이름만)

//파일의 이름과 크기도 전송해야 한다는 말?
//파일이 없는 경우 어떻게 할지도 처리하기

//쓰레드 작업시 문제점
//1. 저장되는 파일 이름이 download_file로 같기 때문에 충돌 발생!
//2. 클라이언트 실행시 최대 10번 &로 묶어가며 작업하기 힘듦'
//   따라서 중간에 scanf로 통신을 몇 개 열건지, 어떤 파일 다운 받을건지 입력할 수 있음 좋을 것
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define BUFSIZE 1048576

int main(int argc, char* argv[]) {
    int c_sock;
    int data_size;
    struct sockaddr_in server_addr, client_addr;
    socklen_t c_addr_size;
    char buf[BUFSIZE] = {0};

    char *server_ip, *request_file_name, *down_file_name;
    int port_num;
    char sucess[] = "수신성공\n";
    char savefilename[100];
    
    int fd, i, write_data_size;
    char num[100];

    server_ip = argv[1];
    port_num = atoi(argv[2]);
    request_file_name = argv[3];

    if(argc != 4) {
    printf("< %s <접속할 서버의 ip주소> <포트번호> <파일명>꼴로 입력해주세요 >\n", __FILE__);
    return 1;
}

    c_sock = socket(AF_INET, SOCK_STREAM, 0);
    
    //memset처럼 0로 만듦
    bzero(&server_addr, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_num);
    //server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    if(connect(c_sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1){ //지정된 서버 주소로 접속
        perror("Can't connet a socket");
        exit(1);
    }

    //1. recv msg from server
    //리시브 호출시 패킷이 도착할 때까지 프로세스가 멈춰서 대기
    if(recv(c_sock, buf, BUFSIZE, 0) == -1){
    perror("Can't receive message");
    exit(1);
    }
    //서버와의 연결확인 메세지 출력
    printf("%s\n", buf);

    //서버에게 요청하는 파일 이름 전송
    if(send(c_sock, request_file_name, sizeof(request_file_name)+1, 0) == -1){
        perror("요청사항 보낼 수 없음");
        exit(1);
    }

    printf("서버에 <%s> 다운로드 요청\n\n", request_file_name);
    
    //파일에 수신 데이터 저장
    //3. 서버로부터 확인받은 파일 정보 수신 및 출력
    bzero(buf, BUFSIZE);

    if(recv(c_sock, buf, BUFSIZE, 0) == -1){
        perror("Can't receive filename");
        exit(1);
    }
    down_file_name = buf;
    printf("서버 측의 <%s> 파일 확인완료\n", down_file_name);

    //4. 서버에 파일명 수신 확인
    if(send(c_sock, sucess, sizeof(sucess)+1, 0) == -1){
        perror("요청사항 보낼 수 없음");
        exit(1);
    }

    //open해서 이미 파일 있을 경우 물어보기
    //중복되는 파일명으로 저장되는 경우 방지 #1 (hw12내 filetest.c에서 코드연습 진행 후 옮김)
    //초기에 밑의 #2방식으로만 코드를 짰으나 여러 프로세스가 동시에 작업되는 것을 표현하기에 부적절하여 #1의 방식을 기본으로 설정함
    for(i = 1; i < 10; i++) {
        memset(num, '\0', sizeof(num));
        sprintf(num, "%d", i);  //num에 정수를 문자열로 저장
        strcat(num, down_file_name);    //해당 정수를 저장하려는 파일명 앞에 붙이기
        strcpy(savefilename, num);  //직관적인 변수명으로 전환
        fd = open(savefilename, O_CREAT|O_EXCL|O_WRONLY, 0644);   //저장하려는 파일이 이미 존재하는지 확인
        if(fd > 0) { //open 성공시 (=중복되는 파일명 x)
            printf("<%s>이름으로 파일을 저장합니다\n", savefilename);
            break;
        }
    }

    //중복되는 파일명으로 저장되는 경우 방지 #2
    //1~9 + <파일명>으로도 중복되는 파일명이 저장되는 경우는 유저에게 저장하려는 이름 묻기
    if(fd == -1){ //파일 열기 실패시
        printf("<%s> 파일을 다운받아 저장할 영문 20자 미만 이름입력\n", down_file_name);
        scanf("%s", savefilename);
        fd = open(savefilename, O_CREAT|O_EXCL|O_WRONLY, 0644);   //저장하려는 파일이 이미 존재하는지 확인
        if(fd > 0) {
            printf("<%s>이름으로 파일을 저장합니다\n", savefilename);
        }
        else {
            while(1){
                printf("중복되는 이름의 파일명입니다.\n다른 이름을 입력해주세요\n");
                scanf("%s", savefilename);
                fd = open(savefilename, O_CREAT|O_EXCL|O_WRONLY, 0644);   //저장하려는 파일이 이미 존재하는지 확인
                if(fd > 0) {
                    printf("<%s>이름으로 파일을 저장합니다\n", savefilename);
                    break;
                }
            }
        }
    }         

    //5. 파일 수신
    data_size = recv(c_sock, buf, BUFSIZE, 0); //서버로부터 수신된 데이터의 크기 저장
    if(data_size == -1){
        perror("Can't receive file");
        close(fd);
        exit(1);
    }

    printf("수신 받은  파일 크기 : %dbyte\n", data_size);

    write_data_size = write(fd, buf, data_size);    //버퍼에 저장된 수신 데이터를 파일에 쓰기, 정상적으로 쓰여졌는지 확인하기 위해 반환값 저장
    printf("write 데이터 크기 : %dbyte\n", write_data_size);
    if(write_data_size > 0){
    printf("파일수신 성공!\n<%s>를 확인해보세요\n", savefilename);
    }
    else {
        perror("파일수신 실패\n");
        //fclose(fp);
        close(fd);
        exit(1);
    }

    close(fd);
    close(c_sock);

    return 0;
}