#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define LISTEN_PORT 8888

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket failed");
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(LISTEN_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind failed");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) == -1) {
        perror("listen failed");
        close(server_fd);
        return 1;
    }

    std::cout << "========================================" << std::endl;
    std::cout << "TCP远程控制服务端已启动" << std::endl;
    std::cout << "监听端口：" << LISTEN_PORT << std::endl;
    std::cout << "等待Windows客户端连接..." << std::endl;
    std::cout << "========================================" << std::endl;

    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
    if (client_fd == -1) {
        perror("accept failed");
        close(server_fd);
        return 1;
    }

    std::cout << std::endl;
    std::cout << "Windows客户端已成功连接，可下发控制指令" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "支持指令说明：" << std::endl;
    std::cout << "A        - 按下A键" << std::endl;
    std::cout << "ENTER    - 按下回车键" << std::endl;
    std::cout << "ESC      - 按下ESC键" << std::endl;
    std::cout << "WIN_D    - 按下Win+D 快速显示桌面" << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    char send_cmd[256];
    while (true) {
        std::cout << std::endl << "请输入控制指令：";
        std::cin >> send_cmd;

        ssize_t send_len = send(client_fd, send_cmd, strlen(send_cmd), 0);
        if (send_len <= 0) {
            std::cout << "客户端已断开连接，程序退出" << std::endl;
            break;
        }
        std::cout << "已发送指令：" << send_cmd << std::endl;
    }

    close(client_fd);
    close(server_fd);
    return 0;
}
