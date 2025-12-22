// settings.h
#ifndef SETTINGS_H
#define SETTINGS_H

#include <cstdint>
#include <string>
#include <unordered_map>

struct Settings {
    int idle_time = 0;      // как часто отправлять данные (в секундах)
    std::string ip_server;  // ip или dns адрес сервера куда отправлять
    int port_server = 0;    // port сервера
    std::string key;        // Ключ для шифрования
    int agent_id = 0;
    bool valid = false;

    bool validate();
};

// УниверсФунк читает и чистит .env от комментариев и символов
std::unordered_map<std::string, std::string> ClearEnvFile(std::string path);

// читает .env и заполняет Settings
Settings LoadEnvSettings(std::string path);

// Вспомогательная функция: обрезать пробелы по краям
std::string Trim(const std::string& str);

bool UpdateAgentIdInEnv(const std::string& filename, int newId);

#endif