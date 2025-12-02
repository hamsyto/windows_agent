// main.cpp
#include <winsock2.h>  // для WSADATA, WSAStartup, socket, bind, listen, connect, htons

#include <atomic>
#include <iostream>
#include <thread>

#include "commands_main.h"
using namespace std;

int SendMessages();

atomic<bool> disconnected{true};
mutex message_mutex;

int main() {
    if ( SendMessages() == 1) return 1;

    // освобождение ресурсов Winsock
    WSACleanup();
    return 0;
}

int SendMessages() {
    if (WinsockInit() != 0) return 1;

    SOCKET connect_socket = INVALID_SOCKET;
    thread recv_thread;
    Message msg = {};
    msg.header.agent_id = 1;
    int id = 1;
    Settings ser = {};
    if (!load_env_settings(ser)) return 1;
    cout << ser.ip_server << ":" << ser.port_server << endl;

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
                thread(RecvMessage, connect_socket, ref(id), ref(disconnected));
        }

        if (id > 0) {
            msg.header.agent_id = id;  // полученно от сервера
        }

        msg = GetMess(msg);
        string jsonStr = msg.toJson();  // без отступов: j.dump()

        if (disconnected) continue;  // проверка на отключение перед отправкой

        Sleep(ser.idle_time);
        SendMessageAndMessageSize(connect_socket, jsonStr);
    }
}

