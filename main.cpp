// main.cpp
#include <winsock2.h>  // для WSADATA, WSAStartup, socket, bind, listen, connect, htons

#include <atomic>
#include <iostream>
#include <thread>

#include "commands_main.h"
#include "const.h"
#include "transport/transport.h"
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

        Message msg = GetMess();
        string jsonStr = msg.toJson();  // без отступов: j.dump()

        SendMessageAndMessageSize(connect_socket, jsonStr);
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
    addr.sin_port =
        htons(kPort);  // преобразует 16-битное число в сетевой порядок
    addr.sin_addr.s_addr = inet_addr(kIpAddr);  //

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
