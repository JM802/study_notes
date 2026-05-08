#include <iostream>
#include <cstring>      // 用于 memset
#include <unistd.h>     // 用于 close() 函数
#include <sys/socket.h> // socket 核心头文件
#include <netinet/in.h> // sockaddr_in 结构体
#include <arpa/inet.h>  // 地址转换
#include <pthread.h>    //多线程
#include <poll.h>
#include <sys/epoll.h>

#define MAX_EVENTS 1024

using namespace std;

// 线程函数：专门处理一个客户端
// 每个客户端进来，都会创建一个线程跑这个函数
void *handle_client(void *arg)
{
    int client_fd = *(int *)arg;
    free(arg);

    char buffer[1024] = {0};
    int count = recv(client_fd, buffer, 1024, 0);

    if (count > 0)
    {
        cout << "\n✅客户端消息: " << buffer << endl;
        cout << "serverfd:3 clientfd:" << client_fd << " count:" << count << endl;

        // 回复客户端
        const char *msg = "all received!";
        send(client_fd, msg, strlen(msg), 0);
    }
    close(client_fd); // 关闭客户端
    cout << "❌ 客户端已断开\n";
    return nullptr;
}

int main()
{
    // ==============================================
    // 1. 创建 TCP 套接字 (Socket)
    // ==============================================
    int server_fd = socket(AF_INET, SOCK_STREAM, 0); // AF_INET指定ipv4,SOCK_STREAM指定通信协议，0代表自动选择；这里IPv4默认就是tcp
    // 这里只创建socket但没有具体绑定
    if (server_fd == -1)
    {
        cerr << "创建 socket 失败！" << endl; // cerr打印错误信息，没有缓冲直接显示
        return 1;
    }
    cout << "✅ socket 创建成功" << endl;

    // ==============================================
    // 2. 定义服务器地址结构体
    // ==============================================
    struct sockaddr_in server_addr; // 存储地址信息的新盒子，装ip+port
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;                // ip地址类型，和socket相同的IPv4
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // ip地址这里为0.0.0.0表示监听所有ip，host to network long为ipaddress
    server_addr.sin_port = htons(8080);              // port，host to network short为port

    // ==============================================
    // 3. 绑定 IP 和端口 (bind)
    // ==============================================
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) // 要转换为sockaddr*指针类型，只有用地址才能转换为指针
    {
        cerr << "绑定失败！" << endl;
        close(server_fd);
        return 1;
    }
    cout << "✅ 绑定成功" << endl;

    // ==============================================
    // 4. 开始监听 (listen)
    // ==============================================
    // 第二个参数：等待连接的队列长度
    if (listen(server_fd, 5) == -1) // 等待连接队列长度为5
    {
        cerr << "监听失败！" << endl;
        close(server_fd);
        return 1;
    }
    cout << "✅ 服务器正在监听 8080 端口..." << endl;

    // // --------------------------
    // // select 多路复用核心部分
    // // --------------------------
    // fd_set rfds; // 监听的fd集合副本
    // fd_set rset; // 真正监听的fd集合（监听读事件(有数据可以读 / 有新客户端连接到来)）
    // FD_ZERO(&rset);
    // FD_SET(server_fd, &rset); // 设置监听server_fd上的读事件
    // int maxfd = server_fd;

    // while (1)
    // {
    //     rfds = rset;

    //     int nready = select(maxfd + 1, &rfds, nullptr, nullptr, nullptr);
    //     if (nready < 0)
    //     {
    //         perror("select error");
    //         break;
    //     }
    //     // ==============================================
    //     // 1. 处理新连接（监听 socket 可读）
    //     // ==============================================
    //     if (FD_ISSET(server_fd, &rfds))
    //     {
    //         struct sockaddr_in clientaddr;
    //         socklen_t len = sizeof(clientaddr);
    //         int client_fd = accept(server_fd, (struct sockaddr *)&clientaddr, &len); // 后续通信都是与client_fd进行，每多一个客户端FD编号增加1，通过client_fd收发数据

    //         if (client_fd == -1)
    //         {
    //             cerr << "接受客户端连接失败！" << endl;
    //             continue;
    //         }
    //         printf("new client connected: fd=%d, IP=%s:%d\n",
    //                client_fd, inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

    //         // 把新的客户端 socket 加入集合
    //         FD_SET(client_fd, &rset);
    //         if (client_fd > maxfd)
    //         {
    //             maxfd = client_fd;
    //         }
    //     }

    //     // ==============================================
    //     // 2. 处理已连接客户端的数据读写
    //     // ==============================================
    //     for (int i = server_fd + 1; i <= maxfd; i++)
    //     // 只遍历所有客户端
    //     {
    //         if (FD_ISSET(i, &rfds))
    //         {
    //             char buffer[128] = {0};
    //             int count = recv(i, buffer, sizeof(buffer) - 1, 0);

    //             if (count == 0)
    //             {
    //                 // 客户端主动断开连接
    //                 printf("client fd=%d disconnected\n", i);
    //                 FD_CLR(i, &rset);
    //                 close(i);
    //                 continue;
    //             }
    //             else if (count < 0)
    //             {
    //                 perror("recv error");
    //                 FD_CLR(i, &rset);
    //                 close(i);
    //                 continue;
    //             }
    //             printf("recv from fd=%d, count=%d, data: %s\n", i, count, buffer);
    //             send(i, buffer, count, 0);
    //         }
    //     }
    // }

    // // --------------------------
    // // poll 多路复用核心部分
    // // --------------------------
    // // pollfd 数组：保存所有关心的文件描述符和事件
    // struct pollfd fds[1024];
    // int maxfd = server_fd;

    // // 初始化：把监听 socket 加入 poll 数组
    // fds[server_fd].fd = server_fd;
    // fds[server_fd].events = POLLIN; // 监听“读事件”（有新连接/有数据），读事件等价于有新客户端连接或者发送数据（读客户端）

    // while (1)
    // {
    //     // 调用 poll，阻塞等待事件
    //     // 参数说明：
    //     // 1. fds：pollfd 数组
    //     // 2. maxfd + 1：数组里有效元素的个数
    //     // 3. -1：永久阻塞，直到有事件发生

    //     int nready = poll(fds, maxfd + 1, -1);
    //     if (nready < 0)
    //     {
    //         perror("poll error");
    //         break;
    //     }

    //     // ==============================================
    //     // 1. 处理新连接（监听 socket 可读）
    //     // ==============================================
    //     if (fds[server_fd].revents & POLLIN) // 内核返回确实发生了读事件
    //     {
    //         struct sockaddr_in clientaddr;
    //         socklen_t len = sizeof(clientaddr);
    //         int client_fd = accept(server_fd, (struct sockaddr *)&clientaddr, &len);
    //         if (client_fd == -1)
    //         {
    //             perror("accept");
    //             continue;
    //         }

    //         printf("new client connected: fd=%d, IP=%s:%d\n",
    //                client_fd, inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

    //         fds[client_fd].fd = client_fd;
    //         fds[client_fd].events = POLLIN;

    //         // 更新 maxfd，保证下次 poll 能遍历到这个 fd
    //         if (client_fd > maxfd)
    //         {
    //             maxfd = client_fd;
    //         }
    //     }

    //     // ==============================================
    //     // 2. 处理已连接客户端的数据读写
    //     // ==============================================
    //     for (int i = server_fd + 1; i <= maxfd; i++)
    //     {
    //         // 判断这个 fd 是否有读事件发生
    //         if (fds[i].revents & POLLIN)
    //         {
    //             char buffer[128] = {0};
    //             int count = recv(i, buffer, sizeof(buffer) - 1, 0);

    //             if (count == 0)
    //             {
    //                 // 客户端主动断开连接
    //                 printf("client fd=%d disconnected\n", i);
    //                 fds[i].fd = -1; // 标记为无效，下次 poll 会忽略
    //                 close(i);
    //                 continue;
    //             }
    //             else if (count < 0)
    //             {
    //                 perror("recv error");
    //                 fds[i].fd = -1;
    //                 close(i);
    //                 continue;
    //             }

    //             printf("recv from fd=%d, count=%d, data: %s\n", i, count, buffer);
    //             send(i, buffer, count, 0);
    //         }
    //     }
    // }

    // ===================== epoll 核心开始 =====================
    // 1. 创建epoll实例
    int epoll_fd = epoll_create1(1);
    if (epoll_fd < 0)
    {
        perror("epoll_create1 error");
        close(server_fd);
        return 1;
    }

    // 2. 把监听fd加入epoll，监听读事件
    // ev是一个临时变量用来添加修改fd，events是返回的所有就绪事件
    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;

    // 注册ev到epoll
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev);

    while (1)
    {
        // 只把就绪的fd装到events里面
        int nready = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nready < 0)
        {
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < nready; i++)
        {
            int fd = events[i].data.fd;

            // 情况1：监听fd就绪 → 有新客户端连接
            if (fd == server_fd)
            {
                struct sockaddr_in clientaddr;
                socklen_t len = sizeof(clientaddr);
                int client_fd = accept(server_fd, (struct sockaddr *)&clientaddr, &len);
                if (client_fd < 0)
                {
                    perror("accept");
                    continue;
                }

                printf("new client: fd=%d IP=%s:%d\n",
                       client_fd, inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

                // 新客户端fd加入epoll，利用ev添加
                ev.events = EPOLLIN;
                ev.data.fd = client_fd;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
            }
            // 情况2：普通客户端fd就绪 → 有数据可读/断开
            else
            {
                char buf[128] = {0};
                int cnt = recv(fd, buf, sizeof(buf) - 1, 0);
                if (cnt <= 0)
                {
                    // 客户端断开/异常
                    printf("client fd=%d disconnect\n", fd);
                    // 从epoll移除
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
                    close(fd);
                    continue;
                }

                // 回显数据
                printf("recv fd=%d: %s\n", fd, buf);
                send(fd, buf, cnt, 0);
            }
        }
    }
    close(epoll_fd);

    // // ==============================================
    // // 5. 等待客户端连接 (accept)
    // // ==============================================
    // // accept 会阻塞，直到有客户端连接
    // while (1)
    // {
    //     struct sockaddr_in clientaddr;
    //     socklen_t len = sizeof(clientaddr);
    //     int client_fd = accept(server_fd, (struct sockaddr *)&clientaddr, &len); // 后续通信都是与client_fd进行，每多一个客户端FD编号增加1，通过client_fd收发数据

    //     if (client_fd == -1)
    //     {
    //         cerr << "接受客户端连接失败！" << endl;
    //         continue;
    //     }

    //     int *p_client = (int *)malloc(sizeof(int));
    //     *p_client = client_fd;

    //     pthread_t tid;                                          // thread的编号，保存线程的ID
    //     pthread_create(&tid, nullptr, handle_client, p_client); // 运行handle_client函数，传参*p_client这是一个函数指针，指向handle_client的起始地址
    //     pthread_detach(tid);                                    // 自动回收线程资源
    // }
    close(server_fd);
    return 0;
}