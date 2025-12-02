// commands.h
#include <winsock2.h>  // для WSADATA, WSAStartup, socket, bind, listen, connect, htons

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include "const.h"
#include "transport/transport.h"
using namespace std;

// инициализирует winsock
int WinsockInit();
SOCKET InitConnectSocket();
void ConnectServer(SOCKET connect_socket);
// переводит преобразование char в int из сетевого порядка в порядок хоста
int32_t get_int32_from_network_buffer(const char* buffer, size_t offset = 0);
void RecvMessage(SOCKET connect_socket, int32_t& id,
                 atomic<bool>& disconnected);
// закрывает сокет с проверкой на закрытие
void close_socket_check(SOCKET& sck);
// отправляет сначала длину потом само сообщение
void SendMessageAndMessageSize(SOCKET& client_socket, const string& jsonStr);
// Вспомогательная функция: обрезать пробелы по краям
string trim(const string& str);
// Основная функция: прочитать .env и заполнить Settings
bool load_env_settings(Settings& out);
int64_t network_buffer_to_int32(char* buffer);

bool write_int_to_file(const char* filename, int32_t value);
bool ensure_file_has_int(const char* filename, int32_t expected_value);

atomic<bool> disconnected{true};

int SendMessages() {
    if (WinsockInit() != 0) return 1;

    SOCKET connect_socket = INVALID_SOCKET;
    Message msg = {};
    msg.header.agent_id = 1;
    int32_t id = 0;
    Settings setting = {};
    if (!load_env_settings(setting)) return 1;
    cout << setting.ip_server << ":" << setting.port_server << endl;

    while (true) {
        if (disconnected) {
            if (connect_socket != INVALID_SOCKET) {
                close_socket_check(connect_socket);
            }

            connect_socket = InitConnectSocket();
            if (connect_socket == INVALID_SOCKET) {
                cout << "connect_socket don't init" << endl;
                continue;
            }

            ConnectServer(connect_socket);
            if (disconnected) {
                close_socket_check(connect_socket);
                continue;
            }
        }

        msg = GetMess(msg);
        string jsonStr = msg.toJson();  // без отступов: j.dump()

        if (disconnected) continue;  // проверка на отключение перед отправкой

        SendMessageAndMessageSize(connect_socket, jsonStr);

        if (id == 0) {
            RecvMessage(connect_socket, id, disconnected);
            if (id < 1) continue;
            cout << id;  // полученно от сервера
        }
        Sleep(setting.idle_time * 1000);
    }
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
        const char* filename = "id_file";
        if (ensure_file_has_int(filename, id)) cout << "id save." << endl;
    }
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

string trim(const string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

bool load_env_settings(Settings& out) {
    ifstream file(".env");  // ищет .env файл
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
    Settings ser = {};
    if (!load_env_settings(ser)) return;
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    // преобразует 16-битное число в сетевой порядок
    addr.sin_port = htons(ser.port_server);
    addr.sin_addr.s_addr = inet_addr(ser.ip_server.c_str());  //

    if (connect(connect_socket, reinterpret_cast<struct sockaddr*>(&addr),
                sizeof(addr)) != 0) {
        cout << "connect() failed with error: " << WSAGetLastError() << endl;
        disconnected = true;
        return;
    }
    cout << "connect to server :) " << endl;
    disconnected = false;
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
    uint32_t lenNetwork = htonl(len);  // host → network (big-endian)

    // 2. Отправка длины
    if (send(client_socket, reinterpret_cast<const char*>(&lenNetwork),
             sizeof(lenNetwork), 0) == SOCKET_ERROR) {
        cout << "send(char_size_message) failed with error: "
             << WSAGetLastError() << endl;
        return;
    }

    if (send(client_socket, jsonStr.c_str(), (int)jsonStr.size(), 0) ==
        SOCKET_ERROR) {
        cout << "send() failed with error: " << WSAGetLastError() << endl;
        return;
    }
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
