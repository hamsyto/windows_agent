// main.cpp
#include <winsock2.h>  // для WSADATA, WSAStartup, socket, bind, listen, connect, htons

#include <atomic>
#include <iostream>
#include <thread>

#include "commands_main.h"
using namespace std;

SOCKET InitConnectSocket();
void ConnectServer(SOCKET connect_socket);
void RecvMessage(SOCKET connect_socket, string& str_message, atomic<bool>& disconnected);

atomic<bool> disconnected{true};
mutex message_mutex;

int main() {
    if (WinsockInit() != 0) return 1;

    SOCKET connect_socket = INVALID_SOCKET;
    thread recv_thread;
    Message msg = {};
    msg.header.agent_id = 0;
    string str_message;

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
                thread(RecvMessage, connect_socket, ref(str_message), ref(disconnected));
        }
        int id;
        {
            lock_guard<mutex> lock(message_mutex);
            id = stoi(str_message);
        }
        
        if (id > 0) {
            msg.header.agent_id = id; // полученно от сервера
        }

        msg = GetMess(msg);
        string jsonStr = msg.toJson();  // без отступов: j.dump()

        if (disconnected) continue;  // проверка на отключение перед отправкой

        Settings ser = {};
        if (!load_env_settings(ser)) return 1;
        Sleep(ser.idle_time);
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
    Settings ser = {};
    if (!load_env_settings(ser)) return;
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port =
        htons(ser.port_server);  // преобразует 16-битное число в сетевой порядок
    addr.sin_addr.s_addr = inet_addr(ser.ip_server.c_str());  //

    if (connect(connect_socket, reinterpret_cast<struct sockaddr*>(&addr),
                sizeof(addr)) != 0) {
        cout << "connect() failed with error: " << WSAGetLastError() << endl;
        disconnected = true;
        return;
    }
    cout << "connect to server :) " << endl;
    disconnected = false;
}

void RecvMessage(SOCKET connect_socket, string& str_message, atomic<bool>& disconnected) {
    while (!disconnected) {
        char buf[kMaxBuf];
        if ((recv(connect_socket, buf, kMaxBuf, 0)) < 1) {
            cout << "Connection closed." << endl;
            disconnected = true;
            return;
        }
        string message(buf, kMaxBuf);
        {
            lock_guard<std::mutex> lock(message_mutex);
            str_message = message;
        }
        
        cout << "Server:" << endl << message << endl;
    }
}
