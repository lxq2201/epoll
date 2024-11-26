#ifndef UTILITY_H_INCLUDE
#define UTILITY_H_INCLUDE

#include <iostream>
#include <list>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace std;

// clients_list save all the client's socket
list<int> clients_list;

/**************************macro defintion************************ */

// server ip
#define SER_IP "192.168.189.129"

// server port
#define SER_PORT 8888

// epoll size
#define EPOLL_SIZE 5000

// message buffer size
#define BUF_SIZE 0xFFFF

#define SER_WELCOME "Welcome you join to the chat room! Your chat ID is: Client %d"

#define SER_MESSAGE "ClientID %d say >> %s"

// exit
#define EXIT "EXIT"

#define CAUTION "There is only one in the chat room!"

/**************************some function*************************** */

/**
 * @param sockfd : socket descriptor
 * @return 0
 **/

int setnonblock(int sockfd)
{
    fcntl(sockfd, F_SETFD, fcntl(sockfd, F_GETFD) | O_NONBLOCK);
    return 0;
}

/**
 * @param epfd: epoll handle
 * @param fd: socket descriptor
 * @param enable_et: enable_et = true,epoll use ET; otherwise LT
 **/

void addfd(int epfd, int fd, bool enable_et)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN;
    if (enable_et)
    {
        ev.events = EPOLLIN | EPOLLET;
    }

    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    setnonblock(fd); //设置为非阻塞模式
}

int sendBroadcastmessage(int cfd)
{
    //buf[BUF_SIZE] 接受新的聊天信息
    //message[BUF_SIZE] 发送格式信息
    char buf[BUF_SIZE],message[BUF_SIZE];
    bzero(buf,BUF_SIZE);
    bzero(message,BUF_SIZE);

    //接受信息
    printf("read from client(clientID = %d)\n",cfd);
    int len=recv(cfd,buf,BUF_SIZE,0);

    if(len<=0)
    {
        close(cfd);
        clients_list.remove(cfd); 
        printf("clientID = %d closed\n",cfd);
        printf("Now there %d client in the chat room\n",clients_list.size());
    }
    //广播信息
    else
    {
        if(clients_list.size()==1)
        {
            send(cfd,CAUTION,strlen(CAUTION),0);
            return len;
        }
        sprintf(message,SER_MESSAGE,cfd,buf);

        list<int>::iterator it;
        for(it=clients_list.begin();it!=clients_list.end();it++)
        {
            if(*it != cfd)
            {
                if(send(*it,message,BUF_SIZE,0)==-1)
                {
                    perror("send error");
                    return -1;
                }
            }
        }
    }

    return len;
}

int deal_connection(int sfd, int epfd)
{
    struct sockaddr_in cin;
    socklen_t socklen = sizeof(cin);
    int newfd = accept(sfd, (struct sockaddr *)&cin, &socklen);
    if(newfd==-1)
    {
        perror("accept error");
        return -1;
    }
    printf("client connection from: [%s:%d]: newfd = %d\n", inet_ntoa(cin.sin_addr), ntohs(cin.sin_port), newfd);

    addfd(epfd, newfd, true); // 把这个新的客户端加入到内核事件列表

    // 服务端用list保存用户连接
    clients_list.push_back(newfd);
    printf("Add newfd = %d to epoll\n", newfd);
    printf("Now there are %d clients in the chat room!!!\n", clients_list.size());

    // 服务端发送欢迎消息
    char message[BUF_SIZE];

    bzero(message, BUF_SIZE);

    sprintf(message, SER_WELCOME, newfd);

    int res = send(newfd, message, BUF_SIZE, 0);
    if (res == -1)
    {
        perror("send error");
        return -1;
    }
}

#endif