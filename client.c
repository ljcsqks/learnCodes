#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

int main()
{
    //1,创建套接字
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1)
    {
        perror("socket");
        exit(-1);
    }
    
    //2,连接服务器
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(9999);
    inet_pton(AF_INET, "127.0.0.1", &saddr.sin_addr.s_addr);
    
    int ret = connect(sockfd, (struct sockaddr*)&saddr, sizeof(saddr));
    if(ret == -1)
    {
        perror("connect");
        exit(-1);
    }

    //3,通信
    char buf[1024];
    int count = 0;
    while(1)
    {
        //发送数据
        snprintf(buf, sizeof(buf), "Hello, Server! This is message #%d", ++count);
        ssize_t bytes_sent = send(sockfd, buf, strlen(buf), 0);
        if(bytes_sent == -1)
        {
            perror("send");
            break;
        }
        
        memset(buf, 0, sizeof(buf)); //清空缓冲区
        //接收数据
        ssize_t bytes_received = recv(sockfd, buf, sizeof(buf)-1, 0);
        if(bytes_received == -1)
        {
            perror("recv");
            break;
        }
        else if(bytes_received == 0)
        {
            printf("服务器已关闭连接。\n");
            break;
        }
        buf[bytes_received] = '\0';
        printf("收到服务器回复：%s\n", buf);

        sleep(1); //每秒发送一次消息
    }

    //4,关闭套接字
    close(sockfd);
    return 0;
}