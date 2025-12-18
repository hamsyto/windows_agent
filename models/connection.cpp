// connection.cpp

#include <winsock2.h>
// порядок подключения
#include <windows.h>

#include <cstdint>
#include <fstream>
#include <iostream>

#include "../consts.h"
#include "../transport/transport.h"
#include "connection.h"
#include "message.h"
#include "settings.h"

using namespace std;

// Конструктор получает Settings по константной ссылке
Connection::Connection() {
    // свойства
    Socket = INVALID_SOCKET;
    connected = false;
    agent_id = 0;
    // сбор данных для подключения
    settings = {};
    if (!LoadEnvSettings(settings)) return;
    cout << settings.ip_server << ":" << settings.port_server << endl;

    // методы
    if (WinsockInit() != 0) return;
    while (agent_id == 0) {
        agent_id = Registration();
    }
    cout << "ID: " << agent_id << " saved\n";
    Disconnect();
}

// Методы
int Connection::WinsockInit() {
    WSADATA wsa;  // структура заполн в WSAStartup инфой о реализации Winsock

    // инициализирует winsock, (указание версии, указатель на структуру). Без
    // вызова WSAStartup любые сетевые функции не будут работать
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {  // возвр 0 - успех
        cout << "WSAStartup failed" << endl;
        WSACleanup();
        return 1;
    }
    return 0;
}

void Connection::Connect() {
    if (Socket != INVALID_SOCKET) {
        Disconnect();
    }

    Socket = InitConnectSocket();
    if (Socket == INVALID_SOCKET) {
        cout << "connect_socket don't init" << endl;
        connected = false;
    }
    if (ConnectServer() > 0) connected = true;

    if (!connected) {
        CloseSocketCheck(Socket);
        Socket = INVALID_SOCKET;
    }
}

void Connection::SendData() {
    Connect();
    if (!connected) return;

    Message msg = {};
    msg.header.agent_id = agent_id;

    string error = "";
    try {
        msg = GetMess(msg);
    } catch (const exception& e) {
        error = e.what();
    }
    string json_msg = msg.toJson();
    if (!connected) return;  // проверка на отключение перед отправкой

    if (error == "") {
        SendMessageAndMessageSize(Socket, json_msg, settings);
    } else {
        SendTypeMsgError(error);
    }
}

// TODO: Создать файл и записать (переделать возврат функции на INT (agent_id))
int32_t Connection::Registration() {
    uint32_t id = 0;
    while (id == 0) {
        SendData();
        if (!connected) continue;
        id = RecvBytes();  // нужно как-то отпределять что пришло и
                           // возвращать только нужное
        // проверка соединения, тк в RecvBytes оно могло прерваться
        if (!connected) continue;
        try {
            if (SaveAgentID(id)) {
                return id;
            }
        } catch (const exception& e) {
            string error = e.what();
            SendTypeMsgError(error);
        }
    }
    return 0;
}

void Connection::SendTypeMsgError(string& error) {
    Message msg = {};
    msg.header.agent_id = agent_id;
    msg.header.type = "error";
    msg.payload.error_text = error;
    string json_error = msg.toJson();
    SendMessageAndMessageSize(Socket, json_error, settings);
}

SOCKET Connection::InitConnectSocket() {
    // IPv4, тип сокета - потоковый, протокол - TCP
    SOCKET connect_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (connect_socket == INVALID_SOCKET) {
        cout << "socket() failed with error: " << WSAGetLastError() << endl;
        return INVALID_SOCKET;
    }
    return connect_socket;
}

int Connection::ConnectServer() {
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    // преобразует 16-битное число в сетевой порядок
    addr.sin_port = htons(settings.port_server);
    addr.sin_addr.s_addr = inet_addr(settings.ip_server.c_str());

    if (connect(Socket, reinterpret_cast<struct sockaddr*>(&addr),
                sizeof(addr)) != 0) {
        cout << "connect() failed with error: " << WSAGetLastError() << endl;
        return -1;
    }
    cout << "connect to server :) " << endl;
    return 1;
}

void Connection::CloseSocketCheck(SOCKET& sck) {
    if ((closesocket(sck)) == SOCKET_ERROR) {
        cout << "closesocket failed with error: " << WSAGetLastError() << endl;
    }
    sck = INVALID_SOCKET;
}

uint32_t Connection::NetworkBufferToUint32(char* buffer) {
    uint64_t net_value = 0;

    memcpy(&net_value, buffer, sizeof(net_value));
    auto result = static_cast<int32_t>(ntohl(net_value));
    return result;
}

// пока старая не универсальная функция
uint32_t Connection::RecvBytes() {
    char buf[kMaxBuf] = {};
    uint32_t bytes_received = recv(Socket, buf, kMaxBuf, 0);
    if (bytes_received < 4) {  // минимум 4 байта для uint32
        cout << "Connection closed or incomplete data." << endl;
        connected = false;
        return 0;
    }
    uint32_t id = NetworkBufferToUint32(buf);
    id = id > 0 ? id : 0;
    if (id == 0) connected = false;
    connected = true;
    return id;
}

void Connection::SendBytes() {
    if (agent_id == 0) {
        agent_id = Registration();
    }
    SendData();
}

void Connection::Disconnect() {
    if (Socket == INVALID_SOCKET) return;
    CloseSocketCheck(Socket);
    connected = false;
    cout << "disconnected from the server" << endl;
    Sleep(settings.idle_time * 1000);
}

bool Connection::SaveAgentID(uint32_t& id) {
    // std::remove возвращает 0 при успехе, ненулевое значение — при ошибке
    remove(kFileName);
    if (!WriteIntToFile(kFileName, id)) {
        cout << "ID not saved\n";
        return false;
    }
    return true;
}

bool Connection::WriteIntToFile(const string& filename, uint32_t& id) {
    ofstream file(filename);
    if (!file) return false;
    file << id;
    return file.good();
}

/*
// Отправка бинарных данных
bool Connection::Send(const void* data, size_t size) {
    if (Socket == INVALID_SOCKET || !connected) return false;
    int sent =
        send(Socket, static_cast<const char*>(data), static_cast<int>(size), 0);
    return (sent == static_cast<int>(size));
}

// Получение бинарных данных (ожидаем ровно size байт)
bool Receive(void* buffer, size_t size) {
    if (Socket == INVALID_SOCKET || !connected) return false;
    size_t total = 0;
    char* ptr = static_cast<char*>(buffer);
    while (total < size) {
        int received =
            recv(Socket, ptr + total, static_cast<int>(size - total), 0);
        if (received <= 0) {
            // Ошибка или соединение закрыто
            return false;
        }
        total += static_cast<size_t>(received);
    }
    return true;
}
*/
SOCKET Connection::GetSocket() const { return Socket; }
uint32_t Connection::GetAgentID() const { return agent_id; }