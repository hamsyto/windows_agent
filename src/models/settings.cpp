// settings.cpp
#include "settings.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>

using namespace std;
// path - путь
Settings LoadEnvSettings(string path) {
    Settings settings = {};
    unordered_map<string, string> env_map = ClearEnvFile(path);

    // Проверяем наличие всех нужных переменныхs
    if (env_map.find("IDLE_TIME") == env_map.end() ||
        env_map.find("IP_SERVER") == env_map.end() ||
        env_map.find("PORT_SERVER") == env_map.end() ||
        env_map.find("KEY") == env_map.end()) {
        cout << "Ошибка: в .env отсутствуют обязательные переменные\n";
        // return false;
        // могу ли сделать переменную для хранения ошибки заполнения?
    }

    try {
        settings.idle_time = stoi(env_map["IDLE_TIME"]);
        settings.ip_server = env_map["IP_SERVER"];
        settings.port_server = stoi(env_map["PORT_SERVER"]);
        settings.key = env_map["KEY"];
    } catch (const exception& e) {
        cout << "Ошибка парсинга числа в .env: " << e.what() << endl;
        // return false;
    }

    return settings;
}

std::unordered_map<string, string> ClearEnvFile(string path) {
    ifstream file(path);  // ищет .env файл
    if (!file.is_open()) {
        cout << "Ошибка: не удалось открыть .env\n";
        // return false;
    }
    unordered_map<string, string> env_map;
    string line;
    while (getline(file, line)) {
        // Удаляем комментарии
        size_t comment_pos = line.find('#');
        if (comment_pos != string::npos) {
            line = line.substr(0, comment_pos);
        }

        line = Trim(line);
        if (line.empty()) continue;

        size_t eq_pos = line.find('=');
        if (eq_pos == string::npos) continue;

        string key = Trim(line.substr(0, eq_pos));
        string value = Trim(line.substr(eq_pos + 1));

        // Убираем кавычки, если есть (поддержка "value" и 'value')
        if ((value.size() >= 2) &&
            ((value.front() == '"' && value.back() == '"') ||
             (value.front() == '\'' && value.back() == '\''))) {
            value = value.substr(1, value.size() - 2);
        }

        env_map[key] = value;
    }
    file.close();
    return env_map;
}

string Trim(const string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}