// commands_main.cpp

#include <winsock2.h>  // для WSADATA, WSAStartup, socket, bind, listen, connect, htons
// порядок подключения
#include <windows.h>

// == ==

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
// #include <thread>
#include <unordered_map>
#include <vector>

#include "collector/commands_coll.h"
#include "commands_main.h"
#include "connection.h"
#include "const.h"
#include "transport/transport.h"

using namespace std;

//  ConnectServer
int SendMessages() {
    if (WinsockInit() != 0) return 1;

    Settings settings = {};
    // чтение или создание файла с id = 0
    settings.agentID = ReadOrCreateCounterFile(kFileName);

    if (!LoadEnvSettings(settings)) return 1;
    cout << settings.ip_server << ":" << settings.port_server << endl;

    Connection connection = Connection(settings);

    while (true) {
        if (settings.agentID == 0) {
            settings.agentID = Registration(connection, settings);
        } else {
            SendData(connect_socket, settings);
        }
    }
    return 1;
}

// TODO: Создать файл и записать (переделать возврат функции на INT (agentID))
unsigned int Registration(SOCKET& connect_socket, Settings& settings) {
    SendData(connect_socket, settings);

    unsigned int id = RecvMessage(connect_socket);
    if (id < 1) return 0;  // Если id значение неправильное
    settings.agentID = id;

    // на случай если значение поменяется после перой итерации SendMessages()
    // msg.header.agent_id = ReadOrCreateCounterFile(kFileName);

    try {
        if (ensure_file_has_int(kFileName, id))
            cout << "id: " << id << " saved." << endl;
    } catch (const exception& e) {
        string error = e.what();
        SendTypeMsgError(connect_socket, error, settings);
    }
    return id;
}

void SendData(SOCKET& connect_socket, Settings& settings) {
    if (StartConnection(connect_socket, settings) == 1) return;

    Message msg = {};
    msg.header.agent_id = settings.agentID;

    string error = "";
    try {
        msg = GetMess(msg);
    } catch (const exception& e) {
        error = e.what();
    }
    string json_msg = msg.toJson();
    if (disconnected) return;  // проверка на отключение перед отправкой

    if (error == "") {
        SendMessageAndMessageSize(connect_socket, json_msg);
    } else {
        SendTypeMsgError(connect_socket, error, settings);
    }
}

void SendTypeMsgError(SOCKET& connect_socket, string& error,
                      Settings& settings) {
    Message msg = {};
    msg.header.agent_id = settings.agentID;
    msg.header.type = "error";
    msg.payload.error_text = error;
    string json_error = msg.toJson();
    SendMessageAndMessageSize(connect_socket, json_error);
}

int StartConnection(SOCKET& connect_socket, Settings& setting) {
    if (disconnected) {
        if (connect_socket != INVALID_SOCKET) {
            CloseSocketCheck(connect_socket);
            // паузы между подключениями
            Sleep(setting.idle_time * 1000);
        }
        connect_socket = InitConnectSocket();
        if (connect_socket == INVALID_SOCKET) {
            cout << "connect_socket don't init" << endl;
            return 1;
        }
        ConnectServer(connect_socket);
        if (disconnected) {
            CloseSocketCheck(connect_socket);
            return 1;
        }
    }
    return 0;
}

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

SOCKET InitConnectSocket() {
    // IPv4, тип сокета - потоковый, протокол - TCP
    SOCKET connect_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (connect_socket == INVALID_SOCKET) {
        cout << "socket() failed with error: " << WSAGetLastError() << endl;
        return INVALID_SOCKET;
    }
    return connect_socket;
}

void ConnectServer(SOCKET connect_socket) {
    Settings setting = {};
    if (!LoadEnvSettings(setting)) return;
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    // преобразует 16-битное число в сетевой порядок
    addr.sin_port = htons(setting.port_server);
    addr.sin_addr.s_addr = inet_addr(setting.ip_server.c_str());  //

    if (connect(connect_socket, reinterpret_cast<struct sockaddr*>(&addr),
                sizeof(addr)) != 0) {
        cout << "connect() failed with error: " << WSAGetLastError() << endl;
        disconnected = true;
        return;
    }
    cout << "connect to server :) " << endl;
    disconnected = false;
}

void CloseSocketCheck(SOCKET& sck) {
    if ((closesocket(sck)) == SOCKET_ERROR) {
        cout << "closesocket failed with error: " << WSAGetLastError() << endl;
    }
    sck = INVALID_SOCKET;
}

unsigned int RecvMessage(SOCKET connect_socket) {
    while (!disconnected) {
        char buf[kMaxBuf] = {};
        size_t bytes_received = recv(connect_socket, buf, kMaxBuf, 0);
        if (bytes_received < 4) {  // минимум 4 байта для int32
            cout << "Connection closed or incomplete data." << endl;
            disconnected = true;
            return 0;
        }
        unsigned int id = network_buffer_to_int32(buf);
        id = id > 0 ? id : 0;
        if (id == 0) disconnected = true;
        return id;
    }
    return 0;
}

unsigned int ReadOrCreateCounterFile(const string& file_name) {
    // Попытка прочитать файл
    ifstream in_file(file_name);
    string content;
    unsigned int value = 0;

    if (!in_file.is_open()) return 0;  //  Файл не найден

    getline(in_file, content);  // читаем до первого \n (или весь файл)
    in_file.close();
    // Удаляем начальные и конечные пробельные символы
    size_t start = content.find_first_not_of(" \t\r\n");
    // файл не пустой или не только пробелы
    if (start == string::npos) return 0;

    size_t end = content.find_last_not_of(" \t\r\n");
    string trimmed = content.substr(start, end - start + 1);

    // Проверяем, что строка состоит только из цифр (и не начинается с
    // '0', если не "0")
    if (trimmed.empty()) return 0;
    if (trimmed.find_first_not_of("0123456789") != string::npos) return 0;

    // Исключаем "0" и числа с ведущими нулями (кроме самого "0", но
    // он нам не нужен)
    if (trimmed != "0" && trimmed[0] != '0') {
        // Преобразуем
        istringstream iss(trimmed);
        if (iss >> value && value > 0) {
            return value;
        }
    }

    // Во всех остальных случаях — создаём/перезаписываем файл как "0"
    ofstream outfile(file_name, ios::trunc);
    if (outfile.is_open()) {
        outfile << "0";
        outfile.close();
    }

    return value;
}

bool ensure_file_has_int(const char* filename, int32_t expected_value) {
    ifstream file(filename);
    if (!file) {
        // Файл не существует — создаём и записываем
        return write_int_to_file(filename, expected_value);
    }

    // Читаем весь файл как строку
    string content((istreambuf_iterator<char>(file)),
                   istreambuf_iterator<char>());
    file.close();

    // Проверяем: пустой или содержит не-цифры?
    if (content.empty()) {
        return write_int_to_file(filename, expected_value);
    }

    // Убеждаемся, что строка состоит ТОЛЬКО из цифр
    for (char c : content) {
        if (!isdigit(static_cast<unsigned char>(c))) {
            // Недопустимый символ — перезаписываем
            return write_int_to_file(filename, expected_value);
        }
    }

    // Преобразуем строку в число
    try {
        // stoi может выбросить исключение при переполнении
        size_t pos = 0;
        long parsed = stol(content, &pos);
        if (pos != content.size()) {
            // Часть строки не распознана как число
            return write_int_to_file(filename, expected_value);
        }
        if (parsed < INT32_MIN || parsed > INT32_MAX) {
            return write_int_to_file(filename, expected_value);
        }
        int32_t current_value = static_cast<int32_t>(parsed);

        if (current_value == expected_value) {
            return true;  // уже совпадает — ничего не делаем
        }
    } catch (...) {
        // Ошибка парсинга — перезаписываем
        return write_int_to_file(filename, expected_value);%
    }

    // Значения не совпадают — перезаписываем
    return write_int_to_file(filename, expected_value);
}

int64_t network_buffer_to_int32(char* buffer) {
    int64_t net_value = 0;

    memcpy(&net_value, buffer, sizeof(net_value));
    auto result = static_cast<int32_t>(ntohl(net_value));
    return result;
}

bool write_int_to_file(const char* filename, int32_t value) {
    ofstream file(filename);
    if (!file) return false;
    file << value;
    return file.good();
}
