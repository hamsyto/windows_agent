// settings.h
#ifndef SETTINGS_H
#define SETTINGS_H

#include <cstdint>
#include <string>
#include <unordered_map>

struct Settings {
  int idle_time;          // как часто отправлять данные (в секундах)
  std::string ip_server;  // ip или dns адрес сервера куда отправлять
  int port_server;        // port сервера
  std::string key;        // Ключ для шифрования
  uint8_t agentID = 0;
};

// УниверсФунк читает и чистит .env от комментариев и символов
std::unordered_map<std::string, std::string> ClearEnvFile(std::string path);

// читает .env и заполняет Settings
Settings LoadEnvSettings(std::string path);

// Вспомогательная функция: обрезать пробелы по краям
std::string Trim(const std::string& str);

#endif