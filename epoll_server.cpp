#include "utility.h"

int main(int argc, const char *argv[])
{
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1)
    {
        perror("socket error");
        return -1;
    }
    printf("socket success sfd = %d\n", sfd);

    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr(SER_IP);
    sin.sin_port = htons(SER_PORT);

    if (bind(sfd, (struct sockaddr *)&sin, sizeof(sin)) == -1)
    {
        perror("bind error");
        return -1;
    }
    printf("bind success\n");

    if (listen(sfd, 128) == -1)
    {
        perror("liisten error");
        return -1;
    }
    printf("listen success\n");

    int epfd = epoll_create(EPOLL_SIZE);
    if (epfd == -1)
    {
        perror("epoll_create error");
        return -1;
    }
    printf("epoll_create success epfd = %d\n", epfd);

    static struct epoll_event evs[EPOLL_SIZE];

    // 往内核事件表中添加事件
    addfd(epfd, sfd, true);

    // 主循环
    while (1)
    {
        int epoll_event_count = epoll_wait(epfd, evs, EPOLL_SIZE, -1);
        if (epoll_event_count == -1)
        {
            perror("epoll_wait error");
            break;
        }
        printf("epoll_event_count = %d\n",epoll_event_count);

        //处理着epoll_event_count个事件
        for(int i=0;i<epoll_event_count;i++)
        {
            int fd=evs[i].data.fd;

            //新用户连接
            if(fd==sfd)
            {
                int res=deal_connection(fd,epfd);
                if(res==-1) return -1;
            }
            // 客户端唤醒/处理客户端发来的消息，并广播，是其他用户收到
            else
            {
                int res = sendBroadcastmessage(fd);
                if(res==-1)
                {
                    perror("sendBroadcastmessage error");
                    return -1;
                }
            }
        }
    }

    close(sfd); //关闭套接字
    close(epfd); //关闭内核，不再监控这些事件是否发生

    return 0;
}