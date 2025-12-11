// connection.cpp

#include <winsock2.h>
// порядок подключения
#include <windows.h>

#include <cstdint>
#include <iostream>

#include "../commands_main.h"
#include "../const.h"
#include "../transport/transport.h"
#include "connection.h"
#include "windows_simple_message.h"

using namespace std;

// Конструктор получает Settings по константной ссылке
Connection::Connection() {
    socket = INVALID_SOCKET;
    connected = false;
    agent_id = 0;
}

// Методы
void Connection::Connect(Settings& settings) {
    if (socket != INVALID_SOCKET) {
        Disconnect();
        Sleep(settings.idle_time * 1000);
    }

    socket = InitConnectSocket();
    if (socket == INVALID_SOCKET) {
        cout << "connect_socket don't init" << endl;
        connected = false;
    }
    if (ConnectServer(socket, settings) > 0) connected = true;

    if (!connected) {
        CloseSocketCheck(socket);
        socket = INVALID_SOCKET;
    }
}

void Connection::Disconnect() {
    if (socket == INVALID_SOCKET) return;
    CloseSocketCheck(socket);
    connected = false;
    cout << "disconnected from the server" << endl;
}

bool Connection::SaveAgentID(uint32_t& id) {
    DeleteFileInCurrentDir(kFileName);
    if (!WriteIntToFile(kFileName, id)) {
        cout << "ID not saved\n";
        return false;
    }
    return true;
}
/*
// Отправка бинарных данных
bool Connection::Send(const void* data, size_t size) {
    if (socket == INVALID_SOCKET || !connected) return false;
    int sent =
        send(socket, static_cast<const char*>(data), static_cast<int>(size), 0);
    return (sent == static_cast<int>(size));
}

// Получение бинарных данных (ожидаем ровно size байт)
bool Receive(void* buffer, size_t size) {
    if (socket == INVALID_SOCKET || !connected) return false;
    size_t total = 0;
    char* ptr = static_cast<char*>(buffer);
    while (total < size) {
        int received =
            recv(socket, ptr + total, static_cast<int>(size - total), 0);
        if (received <= 0) {
            // Ошибка или соединение закрыто
            return false;
        }
        total += static_cast<size_t>(received);
    }
    return true;
}
*/
SOCKET Connection::GetSocket() const { return socket; }
uint32_t Connection::GetAgentID() const { return agent_id; }