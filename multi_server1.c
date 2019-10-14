#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MYPORT 1234    // the port users will be connecting to

#define BACKLOG 5     // how many pending connections queue will hold

#define BUF_SIZE 200

#define MAX_CONTROLLER_NUM (3)

#define MAX_TOKENS  (10)

struct fd_array{
    int fd;
    int type;
};

struct conn {
    fd_set fdsr;
    int maxsock;
    int conn_amount;
    struct fd_array fd_A[MAX_CONTROLLER_NUM];
};    

void conn_init(struct conn *conn) {
    FD_ZERO(&(conn->fdsr));
    conn->maxsock = 0;
    conn->conn_amount = 0;
}

int connect_server(char *ip, int port) {

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;  //使用IPv4地址
    serv_addr.sin_addr.s_addr = inet_addr(ip);  //具体的IP地址
    serv_addr.sin_port = htons(port);  //端口

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0){
        printf("invalid socket! \n");
        return -1;
    }

    if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("connect error!\n");
        return -1;
    }

    return sock;
}


int conn_add_connect(struct conn *conn, char *ip, int port, int type) {

    int sock = connect_server(ip, port);
    if(sock > 0 && conn->conn_amount < MAX_CONTROLLER_NUM) {
        conn->fd_A[conn->conn_amount].fd = sock;
        conn->fd_A[conn->conn_amount].type = type;
        conn->conn_amount ++;
        // if(sock > conn->maxsock) conn->maxsock = sock;
        // FD_SET(sock, &(conn->fdsr));
        printf("add client sock :%d\n", sock);
       
    }else{
        printf("add connect error!\n");
        return -1;
    }
    return 0;

}


void process_main(struct conn *conn) {

    int ret, i, flag = 0;
    struct timeval tv;
    char buf[BUF_SIZE];
    char ip[16] = "127.0.0.1";
    char hello[20] = "hello";
    int port1 = 1111;
    int port2 = 2222;
    int port = 0;

    while(1) {
        
       //clear
        tv.tv_sec = 20;
        tv.tv_usec = 0;
        

        //  if(flag < 2){
        //         //connect first server
        //         if(flag == 0) port = port1;
        //         if(flag == 1) port = port2;

        //         if(conn_add_connect(conn, ip, port, 0) < 0){
        //             printf("connect error.\n");
        //         }else{
        //             printf("connect success\n");
        //         }
        //         flag ++;
        // }

        if(flag == 0) {
            if(conn_add_connect(conn, ip, port1, 0) < 0){
                    printf("connect error.\n");
                }else{
                    printf("connect success\n");
                }

        if(conn_add_connect(conn, ip, port2, 0) < 0){
                    printf("connect error.\n");
                }else{
                    printf("connect success\n");
                }
                flag = 1;
        }

        FD_ZERO(&(conn->fdsr));
        


        //添加已有连接到select中
        for (i = 0; i < conn->conn_amount; i++) {
            if(conn->fd_A[i].fd != 0){
                FD_SET(conn->fd_A[i].fd, &(conn->fdsr));
            } if(conn->maxsock < conn->fd_A[i].fd) {
                conn->maxsock = conn->fd_A[i].fd;
            }
        }

        //select
        ret = select(conn->maxsock + 1, &(conn->fdsr), NULL, NULL, &tv); //检查套接字集合里是否有套接字可读，设置了一个超时时间，30s
        if (ret < 0) {
            perror("select");
            exit(1);
        } else if (ret == 0) {
            // printf("timeout\n");
            continue;
        } else {
            //处理客户端的消息
            for (i = 0; i < conn->conn_amount; i++) {
                printf("i: %d\n", i);
                if(conn->fd_A[i].fd != 0) {
                    if (FD_ISSET(conn->fd_A[i].fd, &(conn->fdsr))) { //判断这个文件描述符是否有数据可读
                        ret = recv(conn->fd_A[i].fd, buf, sizeof(buf), 0); //收消息
                        if (ret <= 0) {        // client close
                            printf("client[%d] close\n", i); //连接错误，关掉连接，在文件描述符集合里清除这个fd
                            close(conn->fd_A[i].fd);
                            FD_CLR(conn->fd_A[i].fd, &(conn->fdsr));
                            conn->fd_A[i].fd = 0;
                        } else {        // receive data
                            if (ret < BUF_SIZE)
                                memset(&buf[ret], '\0', 1);
                            printf("client[%d] send:%s\n", i, buf);
                        }}}   
            }//end for
        } 

        for (i = 0; i < conn->conn_amount; i++){
            if(conn->fd_A[i].fd != 0) {
                ret = send(conn->fd_A[i].fd, hello, sizeof(hello)-1, 0);
                if(ret == -1) {
                    printf("socket send error!\n");
                }
                sleep(1);
            }
        }
    }
}

int main(int argc, char** argv)
{
    struct conn con;
    conn_init(&con);
    process_main(&con); 

    return 0;
}