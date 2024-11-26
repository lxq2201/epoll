#include "utility.h"

int main(int argc, const char *argv[])
{
    // 创建客户端套接字
    int cfd = socket(AF_INET, SOCK_STREAM, 0);
    if (cfd == -1)
    {
        perror("socket error");
        return -1;
    }
    printf("socket success cfd = %d\n");

    // 创建服务器地址信息结构体
    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr(SER_IP);
    sin.sin_port = htons(SER_PORT);

    // 连接服务器
    if (connect(cfd, (struct sockaddr *)&sin, sizeof(sin)) == -1)
    {
        perror("connect error");
        return -1;
    }
    printf("connect success\n");

    // 创建管道，其中fd[0]用于父进程读，fd[1]用于子进程写
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1)
    {
        perror("pipe error");
        return -1;
    }

    // 创建epoll
    int epfd = epoll_create(EPOLL_SIZE);
    if (epfd == -1)
    {
        perror("epoll_create error");
        return -1;
    }

    static struct epoll_event evs[EPOLL_SIZE];

    addfd(epfd, cfd, true);
    addfd(epfd, pipe_fd[0], true);

    // 表示客户端是否正常工作
    bool isClientwork = true;

    // 聊天信息缓冲区
    char message[BUF_SIZE];

    // 创建父子进程
    pid_t pid = fork();
    if (pid == -1)
    {
        perror("fork error");
        return -1;
    }
    // 子进程
    else if (pid == 0)
    {
        // 子进程负责写入管道，因此先关闭读端
        close(pipe_fd[0]);
        printf("Please input 'EXIT' to exit the chat room\n");

        while (isClientwork)
        {
            bzero(&message, BUF_SIZE);
            fgets(message, BUF_SIZE, stdin);

            // 客户端输入exit，退出
            if (strncasecmp(message, EXIT, strlen(EXIT)) == 0)
            {
                isClientwork = 0;
            }
            // 子进程将信息写入管道
            else
            {
                if (write(pipe_fd[1], message, strlen(message) - 1) == -1)
                {
                    perror("write error");
                    return -1;
                }
            }
        }
    }
    else
    {
        // 父进程负责读管道数据，因此先关闭写端
        close(pipe_fd[1]);

        // 主循环(epoll_wait)
        while (isClientwork)
        {
            int epoll_event_count = epoll_wait(epfd, evs, EPOLL_SIZE, -1);

            // 处理就绪的事件
            for (int i = 0; i < epoll_event_count; i++)
            {
                bzero(&message, BUF_SIZE);

                int fd=evs[i].data.fd;

                // 服务端发来消息
                if (fd == cfd)
                {
                    // 接受服务端消息
                    int res = recv(cfd, message, BUF_SIZE, 0);
                    if (res == 0) // res==0 服务端关闭
                    {
                        printf("Server closed connection: %d\n", cfd);
                        close(cfd);
                        isClientwork = false;
                    }
                    else
                    {
                        printf("%s\n", message);
                    }
                }
                // 子进程写入事件发生，父进程处理并发送服务端
                else if(fd == pipe_fd[0])
                {
                    int res = read(pipe_fd[0], message, BUF_SIZE);

                    // res==0 客户端关闭
                    if (res == 0) isClientwork = false;
                    else
                    {
                        if(send(cfd,message,BUF_SIZE,0)==-1)
                        {
                            perror("send error");
                            return -1;
                        }
                    }

                }
            }
        }
    }

    if(pid>0)
    {
        //关闭父进程和cfd
        close(pipe_fd[0]);
        close(cfd);
    }
    else
    {
        //关闭子进程
        close(pipe_fd[1]);
    }

    return 0;
}