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

void SendMessageAndMessageSize(string& message, SOCKET& client_socket) {
    if ((message.size()) > kMaxBuf) {
        cout << "Message length exceeds the allowed sending length. Message "
                "cannot be sent."
             << endl;
        return;
    }
    uint32_t size_message = message.size();
    char char_size_message[kNumChar];

    // почему данное копирование уместно?
    // в клиенте при получении char преобразовывается обратно в int, а значит не
    // важно как компилятор читает char из памяти, важно, чтобы оно
    // соответствовало отправленному int
    memcpy(&char_size_message, &size_message, kNumChar);
    if (send(client_socket, char_size_message, kNumChar, 0) == SOCKET_ERROR) {
        cout << "send(char_size_message) failed with error: "
             << WSAGetLastError() << endl;
        return;
    }

    // строка будет завершена \0
    const char* char_message = message.c_str();
    if (send(client_socket, char_message, size_message, 0) == SOCKET_ERROR) {
        cout << "send() failed with error: " << WSAGetLastError() << endl;
        return;
    }
}
