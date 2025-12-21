#include <winsock2.h>  // для WSADATA, WSAStartup, socket, bind, listen, connect, htons
// порядок подключения
#include <windows.h>

#include <iostream>

#include "models/IConnection.h"
#include "models/connection.h"
#include "transport/transport.h"

using namespace std;

int main() {
    Connection client;

    // проверка подключение, всё ли правильно сработало

    while (true) {
        /// client.RecvBytes();
        client.SendBytes();
    }
    // освобождение ресурсов Winsock
    WSACleanup();
    return 1;
}