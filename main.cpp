#include <winsock2.h>  // для WSADATA, WSAStartup, socket, bind, listen, connect, htons
// порядок подключения
#include <windows.h>

#include <iostream>

#include "models/connection.h"
#include "transport/transport.h"

using namespace std;

int main() {
    // сбор данных для подключения
    Settings settings = {};
    if (!LoadEnvSettings(settings)) return 1;
    cout << settings.ip_server << ":" << settings.port_server << endl;

    Connection client(settings);

    // проверка подключение, всё ли правильно сработало

    while (true) {
        /// client.RecvBytes();
        client.SendBytes(settings);
    }
    // освобождение ресурсов Winsock
    WSACleanup();
    return 1;
}