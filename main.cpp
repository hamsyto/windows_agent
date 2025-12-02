// main.cpp
#include <winsock2.h>  // для WSADATA, WSAStartup, socket, bind, listen, connect, htons

#include <atomic>
#include <iostream>
#include <thread>

#include "commands_main.h"
using namespace std;

int SendMessages();



int main() {
    if (SendMessages() == 1) return 1;

    // освобождение ресурсов Winsock
    WSACleanup();
    return 0;
}