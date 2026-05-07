#include <cstring>
#include <string>
#include <vector>
#include <cstdio>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

static HANDLE console_out() {
    return GetStdHandle(STD_OUTPUT_HANDLE);
}

// 源码保存为 UTF-8：将 UTF-8 字面量转成宽字符，用 WriteConsoleW 输出（与 chcp / cout 无关，不乱码）
static void out_u8(const char* utf8, int byte_len) {
    if (!utf8 || byte_len == 0)
        return;
    int nw = MultiByteToWideChar(CP_UTF8, 0, utf8, byte_len, nullptr, 0);
    if (nw <= 0)
        return;
    std::vector<wchar_t> w(static_cast<size_t>(nw));
    MultiByteToWideChar(CP_UTF8, 0, utf8, byte_len, w.data(), nw);
    DWORD written = 0;
    WriteConsoleW(console_out(), w.data(), nw, &written, nullptr);
}

static void out_u8(const char* utf8) {
    if (utf8)
        out_u8(utf8, static_cast<int>(strlen(utf8)));
}

static void out_u8(const std::string& s) {
    if (!s.empty())
        out_u8(s.data(), static_cast<int>(s.size()));
}

static void out_u8_line(const char* utf8) {
    out_u8(utf8);
    static const wchar_t nl = L'\n';
    DWORD w = 0;
    WriteConsoleW(console_out(), &nl, 1, &w, nullptr);
}

static void out_u8_line(const std::string& s) {
    out_u8(s);
    static const wchar_t nl = L'\n';
    DWORD w = 0;
    WriteConsoleW(console_out(), &nl, 1, &w, nullptr);
}

static void wait_enter(const char* utf8_hint) {
    out_u8_line(utf8_hint);
    (void)getchar();
}

// 启用 VT（emoji 等）；窄字节输出已改用 WriteConsoleW，不再依赖控制台代码页
static void init_console() {
    const DWORD vt = ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    for (DWORD id : {STD_OUTPUT_HANDLE, STD_ERROR_HANDLE}) {
        HANDLE h = GetStdHandle(id);
        if (!h || h == INVALID_HANDLE_VALUE)
            continue;
        DWORD mode = 0;
        if (GetConsoleMode(h, &mode))
            SetConsoleMode(h, mode | vt);
    }
}

// MSVC 自动链接 ws2_32；MinGW 请在编译命令中加 -lws2_32
#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif

// 【必填修改】这里改为你的Linux服务器IP地址
#define SERVER_IP "139.180.154.66"
// 端口必须和服务器端完全一致
#define SERVER_PORT 8888

// 键盘模拟函数：按下+松开按键
void SimulateKey(WORD vk_key) {
    // 按下按键
    keybd_event(vk_key, 0, 0, 0);
    Sleep(30);
    // 松开按键
    keybd_event(vk_key, 0, KEYEVENTF_KEYUP, 0);
    Sleep(30);
}

// 组合键模拟函数（Win+D 显示桌面）
void SimulateWinD() {
    keybd_event(VK_LWIN, 0, 0, 0);
    Sleep(30);
    keybd_event(0x44, 0, 0, 0);
    Sleep(30);
    keybd_event(0x44, 0, KEYEVENTF_KEYUP, 0);
    Sleep(30);
    keybd_event(VK_LWIN, 0, KEYEVENTF_KEYUP, 0);
}

// 指令解析与执行
void ExecuteCommand(const char* cmd) {
    if (_strcmpi(cmd, "A") == 0) {
        SimulateKey(0x41);
        out_u8_line("✅ 已执行：按下A键");
    }
    else if (_strcmpi(cmd, "ENTER") == 0) {
        SimulateKey(VK_RETURN);
        out_u8_line("✅ 已执行：按下回车键");
    }
    else if (_strcmpi(cmd, "ESC") == 0) {
        SimulateKey(VK_ESCAPE);
        out_u8_line("✅ 已执行：按下ESC键");
    }
    else if (_strcmpi(cmd, "WIN_D") == 0) {
        SimulateWinD();
        out_u8_line("✅ 已执行：Win+D 显示桌面");
    }
    else if (_strcmpi(cmd, "SPACE") == 0) {
        SimulateKey(VK_SPACE);
        out_u8_line("✅ 已执行：按下空格键");
    }
    else {
        out_u8_line("❌ 未知指令，无法执行");
    }
}

int main() {
    init_console();

    // 隐藏控制台黑窗口（可选，去掉注释即可后台运行）
    // ShowWindow(GetConsoleWindow(), SW_HIDE);

    // 1. 初始化Windows套接字环境
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        out_u8_line("❌ 网络环境初始化失败");
        wait_enter("按 Enter 键退出...");
        return 1;
    }

    // 2. 创建客户端套接字
    SOCKET client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == INVALID_SOCKET) {
        out_u8_line("❌ 套接字创建失败");
        WSACleanup();
        wait_enter("按 Enter 键退出...");
        return 1;
    }

    // 3. 配置服务器地址
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    unsigned long addr = inet_addr(SERVER_IP);
    if (addr == INADDR_NONE) {
        out_u8_line("❌ IP 地址格式无效");
        closesocket(client_fd);
        WSACleanup();
        wait_enter("按 Enter 键退出...");
        return 1;
    }
    server_addr.sin_addr.s_addr = addr;

    // 4. 连接Linux服务器
    out_u8_line("🔗 正在连接Linux服务器...");
    if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        out_u8_line("❌ 连接服务器失败，请检查IP、端口、网络");
        closesocket(client_fd);
        WSACleanup();
        wait_enter("按 Enter 键退出...");
        return 1;
    }

    out_u8_line("=========================================");
    out_u8_line("🎉 成功连接服务器！");
    out_u8_line("✅ 等待服务器下发控制指令");
    out_u8_line("=========================================");

    // 循环接收服务器指令
    char recv_buf[256];
    while (true) {
        memset(recv_buf, 0, sizeof(recv_buf));
        int recv_len = recv(client_fd, recv_buf, sizeof(recv_buf) - 1, 0);

        // 连接断开处理
        if (recv_len <= 0) {
            out_u8_line("❌ 服务器连接已断开");
            break;
        }

        out_u8_line(std::string("\n📩 收到服务器指令：") + recv_buf);
        ExecuteCommand(recv_buf);
    }

    // 资源释放
    closesocket(client_fd);
    WSACleanup();
    wait_enter("按 Enter 键退出...");
    return 0;
}