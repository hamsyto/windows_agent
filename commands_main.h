// commands.h
#include <winsock2.h>  // для WSADATA, WSAStartup, socket, bind, listen, connect, htons

#include <algorithm>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <sstream>

#include "transport/transport.h"
#include "const.h"
using namespace std;

// инициализирует winsock
int WinsockInit();
// закрывает сокет с проверкой на закрытие
void close_socket_check(SOCKET& sck);
// отправляет сначала длину потом само сообщение
void SendMessageAndMessageSize(string& message, SOCKET& client_socket);

// Вспомогательная функция: обрезать пробелы по краям
string trim(const string& str);
// Основная функция: прочитать .env и заполнить Settings
bool load_env_settings(Settings& out);

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

    if (send(client_socket, jsonStr.c_str(), (int)jsonStr.size(), 0) == SOCKET_ERROR) {
        cout << "send() failed with error: " << WSAGetLastError() << endl;
        return;
    }
}


string trim(const string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

bool load_env_settings(Settings& out) {
    ifstream file(".env");
    if (!file.is_open()) {
        cerr << "Ошибка: не удалось открыть .env\n";
        return false;
    }

    unordered_map<string, string> env_map;

    string line;
    while (getline(file, line)) {
        // Удаляем комментарии
        size_t comment_pos = line.find('#');
        if (comment_pos != string::npos) {
            line = line.substr(0, comment_pos);
        }

        line = trim(line);
        if (line.empty()) continue;

        size_t eq_pos = line.find('=');
        if (eq_pos == string::npos) continue;

        string key = trim(line.substr(0, eq_pos));
        string value = trim(line.substr(eq_pos + 1));

        // Убираем кавычки, если есть (поддержка "value" и 'value')
        if ((value.size() >= 2) &&
            ((value.front() == '"' && value.back() == '"') ||
             (value.front() == '\'' && value.back() == '\''))) {
            value = value.substr(1, value.size() - 2);
        }

        env_map[key] = value;
    }

    file.close();

    // Проверяем наличие всех нужных переменных
    if (env_map.find("IDLE_TIME") == env_map.end() ||
        env_map.find("IP_SERVER") == env_map.end() ||
        env_map.find("PORT_SERVER") == env_map.end()) {
        cerr << "Ошибка: в .env отсутствуют обязательные переменные\n";
        return false;
    }

    try {
        out.idle_time = stoi(env_map["IDLE_TIME"]);
        out.ip_server = env_map["IP_SERVER"];
        out.port_server = stoi(env_map["PORT_SERVER"]);
    } catch (const exception& e) {
        cerr << "Ошибка парсинга числа в .env: " << e.what() << "\n";
        return false;
    }

    return true;
}