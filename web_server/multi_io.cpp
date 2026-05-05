#include <iostream>
#include <cstring>      // 用于 memset
#include <unistd.h>     // 用于 close() 函数
#include <sys/socket.h> // socket 核心头文件
#include <netinet/in.h> // sockaddr_in 结构体
#include <arpa/inet.h>  // 地址转换
#include <pthread.h>    //多线程

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

    // ==============================================
    // 5. 等待客户端连接 (accept)
    // ==============================================
    // accept 会阻塞，直到有客户端连接
    while (1)
    {
        struct sockaddr_in clientaddr;
        socklen_t len = sizeof(clientaddr);
        int client_fd = accept(server_fd, (struct sockaddr *)&clientaddr, &len); // 后续通信都是与client_fd进行，每多一个客户端FD编号增加1，通过client_fd收发数据

        if (client_fd == -1)
        {
            cerr << "接受客户端连接失败！" << endl;
            continue;
        }

        int *p_client = (int *)malloc(sizeof(int));
        *p_client = client_fd;

        pthread_t tid;                                          // thread的编号，保存线程的ID
        pthread_create(&tid, nullptr, handle_client, p_client); // 运行handle_client函数，传参*p_client这是一个函数指针，指向handle_client的起始地址
        pthread_detach(tid);                                    // 自动回收线程资源
    }
    close(server_fd);
    return 0;
}