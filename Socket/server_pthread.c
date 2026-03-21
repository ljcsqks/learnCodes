#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

struct SockInfo
{
    int connfd;
    struct sockaddr_in cliaddr;
};

static struct SockInfo sockinfo[512];

void* handle_client(void* arg);

int main()
{
    //1,创建监听套接字
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd == -1)
    {
        perror("socket");
        exit(-1);
    }

    //2,绑定本地IP port
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(9999);
    saddr.sin_addr.s_addr = INADDR_ANY;
    if(bind(listenfd, (struct sockaddr*)&saddr, sizeof(saddr)) == -1)
    {
        perror("bind");
        exit(-1);
    }

    //3,监听
    if(listen(listenfd, 8) == -1)
    {
        perror("listen");
        exit(-1);
    }
    printf("服务器启动成功，等待客户端连接...\n");

    //4,阻塞并等待客户端连接
    int max = sizeof(sockinfo) / sizeof(sockinfo[0]);
    for (int i = 0; i < max; ++i)
    {
        bzero(&sockinfo[i], sizeof(sockinfo[i]));
        sockinfo[i].connfd = -1;
    }

    socklen_t addrlen = sizeof(struct sockaddr_in);
    while (1)
    {
        struct SockInfo* info = NULL;
        for (int i = 0; i < max; ++i)
        {
            if (sockinfo[i].connfd == -1)
            {
                info = &sockinfo[i];
                break;
            }
        }
        int connfd = accept(listenfd, (struct sockaddr*)&info->cliaddr, &addrlen);
        if(connfd == -1)
        {
            perror("accept");
            exit(-1);
        }
        info->connfd = connfd;

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, info);
        pthread_detach(tid); //分离线程，自动回收资源 
    }

    close(listenfd);
    return 0;
}

void* handle_client(void* arg)
{
    struct SockInfo* info = (struct SockInfo*)arg;
    int connfd = info->connfd;
    printf("客户端连接成功，IP地址为：%s，端口号为：%d\n", inet_ntoa(info->cliaddr.sin_addr), ntohs(info->cliaddr.sin_port));
    //5,通信
    char buf[1024];
    while(1)
    {
        //接收数据
        ssize_t bytes_received = recv(connfd, buf, sizeof(buf)-1, 0);
        if(bytes_received == -1)
        {
            perror("recv");
            break;
        }
        else if(bytes_received == 0)
        {
            printf("客户端已关闭连接。\n");
            break;
        }
        buf[bytes_received] = '\0';
        send(connfd, buf, bytes_received, 0);
        printf("收到客户端消息：%s\n", buf);
    }

    //6,关闭套接字
    info->connfd = -1;
    close(connfd);
    return NULL;
}