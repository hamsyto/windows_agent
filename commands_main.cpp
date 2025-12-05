// commands_main.cpp

#include <windows.h>
#include <winsock2.h>  // для WSADATA, WSAStartup, socket, bind, listen, connect, htons

// == ==

#include <string>

#include "transport/transport.h"
// == ==

#include <algorithm>
#include <atomic>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <vector>

#include "commands_main.h"
#include "const.h"

using namespace std;

atomic<bool> disconnected{true};

void Registration() {
    string error = "";
    Hardware hardware = GetHardware();










}








int SendMessages() {
    if (WinsockInit() != 0) return 1;

    Message msg = {};
    msg.header.agent_id = ReadOrCreateCounterFile(kFileName);

    Settings setting = {};
    if (!LoadEnvSettings(setting)) return 1;
    cout << setting.ip_server << ":" << setting.port_server << endl;

    SOCKET connect_socket = INVALID_SOCKET;
    while (true) {
        if (disconnected) {
            if (connect_socket != INVALID_SOCKET) {
                CloseSocketCheck(connect_socket);
            }
            connect_socket = InitConnectSocket();
            if (connect_socket == INVALID_SOCKET) {
                cout << "connect_socket don't init" << endl;
                continue;
            }
            ConnectServer(connect_socket);
            if (disconnected) {
                CloseSocketCheck(connect_socket);
                continue;
            }
        }

        msg = GetMess(msg);
        string jsonStr = msg.toJson();

        if (disconnected) continue;  // проверка на отключение перед отправкой

        SendMessageAndMessageSize(connect_socket, jsonStr);
        int id = 0;
        if (msg.header.agent_id == 0) {
            RecvMessage(connect_socket, id, disconnected);
            if (id < 1) continue;
            msg.header.agent_id = id;
            cout << id;  // полученно от сервера
        }
        Sleep(setting.idle_time * 10000);
        disconnected = true;
    }
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

void RecvMessage(SOCKET connect_socket, int32_t& id,
                 atomic<bool>& disconnected) {
    while (!disconnected) {
        char buf[kMaxBuf] = {};
        size_t bytes_received = recv(connect_socket, buf, kMaxBuf, 0);
        if (bytes_received < 4) {  // минимум 4 байта для int32
            cout << "Connection closed or incomplete data." << endl;
            disconnected = true;
            return;
        }
        int32_t value = network_buffer_to_int32(buf);
        id = (value > 0 ? value : 0);
        if (id == 0) {
            disconnected = true;
            return;
        }
        const char* filename = "id_file";
        if (ensure_file_has_int(filename, id)) cout << "id save." << endl;
    }
}

unsigned long ReadOrCreateCounterFile(const string& file_name) {
    // Попытка прочитать файл
    ifstream in_file(file_name);
    string content;
    unsigned long long value = 0;

    if (in_file.is_open()) {
        getline(in_file, content);  // читаем до первого \n (или весь файл)
        in_file.close();
        // Удаляем начальные и конечные пробельные символы
        size_t start = content.find_first_not_of(" \t\r\n");
        // файл не пустой или не только пробелы
        if (start != string::npos) {
            size_t end = content.find_last_not_of(" \t\r\n");
            string trimmed = content.substr(start, end - start + 1);

            // Проверяем, что строка состоит только из цифр (и не начинается с
            // '0', если не "0")
            if (!trimmed.empty() &&
                trimmed.find_first_not_of("0123456789") == string::npos) {
                // Исключаем "0" и числа с ведущими нулями (кроме самого "0", но
                // он нам не нужен)
                if (trimmed != "0" && trimmed[0] != '0') {
                    // Преобразуем
                    istringstream iss(trimmed);
                    if (iss >> value && value > 0) {
                        // Успешно: корректное положительное число
                        return value;
                    }
                }
            }
        }

        // Во всех остальных случаях — создаём/перезаписываем файл как "0"
        ofstream outfile(file_name, ios::trunc);
        if (outfile.is_open()) {
            outfile << "0";
            outfile.close();
        }
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
        return write_int_to_file(filename, expected_value);
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
