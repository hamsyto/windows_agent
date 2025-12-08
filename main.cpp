// main.cpp

#include <winsock2.h>  // для WSADATA, WSAStartup, socket, bind, listen, connect, htons
// порядок подключения
#include <windows.h>
// #include <thread>

#include "commands_main.h"
using namespace std;

int main() {
    if (SendMessages() == 1) {
        WSACleanup();
        return 1;
    }
    // освобождение ресурсов Winsock
    WSACleanup();
    return 0;
}