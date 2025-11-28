// commands.h
#include <winsock2.h>  // для WSADATA, WSAStartup, socket, bind, listen, connect, htons

#include <algorithm>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "const.h"
using namespace std;

//
int WinsockInit();
// закрывает сокет с проверкой на закрытие
void close_socket_check(SOCKET& sck);
// отправляет сначала длину потом само сообщение
void SendMessageAndMessageSize(string& message, SOCKET& client_socket);

int WinsockInit() {
    // структура, которая заполняется в WSAStartup
    WSADATA wsa;

    // инициализирует winsock, (указание версии, указатель на структуру которая
    // будет заполнена инфой о реализации Winsock Без вызова WSAStartup любые
    // сетевые функции (например, socket, bind, connect) не будут работать
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {  // возвр 0 - успех
        cout << "WSAStartup failed" << endl;
        WSACleanup();
        return 1;
    }
    return 0;
}

void close_socket_check(SOCKET& sck) {
    if ((closesocket(sck)) == SOCKET_ERROR) {
        cout << "closesocket failed with error: " << WSAGetLastError() << endl;
    }
    sck = INVALID_SOCKET;
}

void SendMessageAndMessageSize(SOCKET& client_socket, const string& jsonStr) {
    if ((jsonStr.size()) > kMaxBuf) {
        cout << "Message length exceeds the allowed sending length. Message "
                "cannot be sent."
             << endl;
        return;
    }

    // 1. Подготовка длины (в network byte order)
    uint32_t len = static_cast<uint32_t>(jsonStr.size());
    uint32_t lenNetwork = htonl(len); // host → network (big-endian)

    // 2. Отправка длины
    if (send(client_socket, reinterpret_cast<const char*>(&lenNetwork), sizeof(lenNetwork), 0) == SOCKET_ERROR) {
        cout << "send(char_size_message) failed with error: "
             << WSAGetLastError() << endl;
        return;
    }


    if (send(client_socket, jsonStr.c_str(), (int)jsonStr.size(), 0) == SOCKET_ERROR) {
        cout << "send() failed with error: " << WSAGetLastError() << endl;
        return;
    }
}
