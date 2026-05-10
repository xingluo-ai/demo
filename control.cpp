// 控制端：连接中继服务器并上报身份 ctrl（控制台中文用 WriteConsoleW，避免乱码）

#include <cstring>
#include <string>
#include <vector>
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>

#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

static HANDLE console_out()
{
    return GetStdHandle(STD_OUTPUT_HANDLE);
}

static void out_u8(const char* utf8, int byte_len)
{
    if (!utf8 || byte_len <= 0)
        return;
    int nw = MultiByteToWideChar(CP_UTF8, 0, utf8, byte_len, nullptr, 0);
    if (nw <= 0)
        return;
    std::vector<wchar_t> w(static_cast<size_t>(nw));
    MultiByteToWideChar(CP_UTF8, 0, utf8, byte_len, w.data(), nw);
    DWORD written = 0;
    WriteConsoleW(console_out(), w.data(), static_cast<DWORD>(nw), &written, nullptr);
}

static void out_u8(const char* utf8)
{
    if (utf8)
        out_u8(utf8, static_cast<int>(strlen(utf8)));
}

static void out_u8(const std::string& s)
{
    if (!s.empty())
        out_u8(s.data(), static_cast<int>(s.size()));
}

static void out_u8_line(const char* utf8)
{
    out_u8(utf8);
    static const wchar_t nl = L'\n';
    DWORD w = 0;
    WriteConsoleW(console_out(), &nl, 1, &w, nullptr);
}

static void out_u8_line(const std::string& s)
{
    out_u8(s);
    static const wchar_t nl = L'\n';
    DWORD w = 0;
    WriteConsoleW(console_out(), &nl, 1, &w, nullptr);
}

static void init_console()
{
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

// 中继服务器 IP / 端口（与服务器 listening 一致）
const char* SERVER_IP = "139.180.154.66";
const int SERVER_PORT = 8888;

int main()
{
    init_console();

    WSADATA wsaData{};
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        out_u8_line("❌ WSAStartup 失败");
        return 1;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        out_u8_line("❌ 创建套接字失败");
        WSACleanup();
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(static_cast<u_short>(SERVER_PORT));
    unsigned long addr = inet_addr(SERVER_IP);
    if (addr == INADDR_NONE) {
        out_u8_line("❌ IP 地址无效");
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    server_addr.sin_addr.s_addr = addr;

    if (connect(sock, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == SOCKET_ERROR) {
        out_u8_line("❌ 连接服务器失败，请检查 IP、端口与网络");
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    out_u8_line("[控制端] 已连接服务器");

    if (send(sock, "ctrl", 4, 0) != 4) {
        out_u8_line("❌ 上报身份失败");
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    char cmd[256];
    for (;;) {
        out_u8("\n输入指令(a/enter/space/esc)：");
        std::cin >> cmd;
        send(sock, cmd, static_cast<int>(strlen(cmd)), 0);
        out_u8_line(std::string("已发送：") + cmd);
    }
}
