// settings.h

#ifndef SETTINGS_H
#define SETTINGS_H

#include <cstdint>
#include <string>
#include <unordered_map>

struct Settings {
    int idle_time = 0;         // как часто отправлять данные (в секундах)
    std::string ip_server;     // ip или dns адрес сервера куда отправлять
    int port_server = 0;       // port сервера
    std::string key = "asdf";  // Ключ для шифрования
    int agent_id = 0;          // id агента
    std::string type;          // выбор сценария из .env

    bool validate();
};

// УниверсФунк читает и чистит .env от комментариев и символов
std::unordered_map<std::string, std::string> ClearEnvFile(
    const std::string path);

// читает .env и заполняет Settings
Settings LoadEnvSettings(const std::string path);

// Вспомогательная функция: обрезать пробелы по краям
std::string Trim(const std::string& str);

bool UpdateAgentIdInEnv(const std::string path, int newId);

#endif