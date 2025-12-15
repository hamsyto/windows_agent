#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>
#include <unordered_map>

struct Settings {
  int idle_time;          // как часто отправлять данные (в секундах)
  std::string ip_server;  // ip или dns адрес сервера куда отправлять
  int port_server;        // port сервера
  std::string key;        // Ключ для шифрования
};

// УниверсФунк читает и чистит .env от комментариев и символов
std::unordered_map<std::string, std::string> ClearEnvFile();

// читает .env и заполняет Settings
bool LoadEnvSettings(Settings& out);

// Вспомогательная функция: обрезать пробелы по краям
std::string Trim(const std::string& str);

#endif