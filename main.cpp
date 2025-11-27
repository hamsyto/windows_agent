// исключает малоиспользуемые части Windows API (устаревшие функции, редко
// используемые компоненты, макросы и структуры, которые не нужны в большинстве
// современных приложений): ускоряет компиляцию, подключение библиотек виндовс
// будет занимать меньше места и памяти нужен только с <winsock.h>, тк там много
// "лишнего"
// #define WIN32_LEAN_AND_MEAN

// для inet_ntop - IPv6
// #define _WIN32_WINNT 0x0600
// #include <ws2tcpip.h>

// масщтабный заголовок.    сам определяет #define WIN32_LEAN_AND_MEAN
#include <winsock2.h>  // для WSADATA, WSAStartup, socket, bind, listen, connect, htons

#include <atomic>
#include <iostream>
#include <thread>

#include "commands.h"
#include "const.h"
using namespace std;

SOCKET InitConnectSocket();
void ConnectServer(SOCKET connect_socket);
void RecvMessage(SOCKET connect_socket, atomic<bool>& disconnected);

atomic<bool> disconnected{true};

int main() {
    if (WinsockInit() != 0) return 1;

    SOCKET connect_socket = INVALID_SOCKET;
    thread recv_thread;

    while (true) {
        if (disconnected) {
            if (connect_socket != INVALID_SOCKET) {
                close_socket_check(connect_socket);
                if (recv_thread.joinable()) {  // проверка на запуск потока
                    recv_thread.join();        // дождаться завершения потока
                }
            }

            connect_socket = InitConnectSocket();
            if (connect_socket == INVALID_SOCKET) {
                cout << "connect_socket don't init" << endl;
                continue;
            }

            ConnectServer(connect_socket);
            if (disconnected) {
                close_socket_check(connect_socket);
                continue;
            }

            recv_thread =
                thread(RecvMessage, connect_socket, ref(disconnected));
        }

        if (disconnected) continue;  // проверка на отключение перед отправкой




        
        SendMessageAndMessageSize(message, connect_socket);
    }

    // освобождение ресурсов Winsock
    WSACleanup();
    return 0;
}

SOCKET InitConnectSocket() {
    // IPv4, тип сокета - потоковый, протокол - TCP
    SOCKET connect_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (connect_socket == INVALID_SOCKET) {
        cout << "socket() failed with error: " << WSAGetLastError() << endl;
        return INVALID_SOCKET;
    }
    return connect_socket;
}

void ConnectServer(SOCKET connect_socket) {
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(kPort);  // преобразует 16-битное число в сетевой порядок
    addr.sin_addr.s_addr = inet_addr(kIpAddr);  //

    // си-стиль: (struct sockaddr*)&addr;
    // си++ -стиль: reinterpret_cast<struct sockaddr*>(&addr);  static_cast,
    // const_cast
    // struct sockaddr* pAddr = reinterpret_cast<struct sockaddr*>(&addr);

    if (connect(connect_socket, reinterpret_cast<struct sockaddr*>(&addr),
                sizeof(addr)) != 0) {
        cout << "connect() failed with error: " << WSAGetLastError() << endl;
        disconnected = true;
        return;
    }
    cout << "connect to server :) " << endl;
    disconnected = false;
}

void RecvMessage(SOCKET connect_socket, atomic<bool>& disconnected) {
    while (!disconnected) {
        char char_size_message[kNumChar];
        int result = recv(connect_socket, char_size_message, kNumChar, 0);
        if (result < 1) {
            cout << "Connection closed. Press ENTER to reconnect." << endl;
            disconnected = true;
            return;
        }

        uint32_t size_message = 0;
        // memcpy копирует байты
        memcpy(&size_message, &char_size_message, kNumChar);

        char char_message[size_message];

        result = recv(connect_socket, char_message, size_message, 0);
        if (result < 1) {
            cout << "Connection closed." << endl;
            disconnected = true;
            return;
        }
        string message(char_message, size_message);
        cout << "Server:" << endl << message << endl;
    }
}
