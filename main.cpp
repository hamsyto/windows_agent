// main.cpp

#include <windows.h>
#include <winsock2.h>  // для WSADATA, WSAStartup, socket, bind, listen, connect, htons

#include <atomic>
#include <iostream>
#include <thread>

#include "commands_main.h"
using namespace std;

int SendMessages();
void Registration();



int main() {

    if (SendMessages() == 1) {
        WSACleanup();
        return 1;
    }

    // освобождение ресурсов Winsock
    WSACleanup();
    return 0;
}