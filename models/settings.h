#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>

struct Settings {
  int idle_time;             // как часто отправлять данные (в секундах)
  std::string ip_server;     // ip или dns адрес сервера куда отправлять
  int port_server;           // port сервера
  std::string listener_key;  // Ключ для шифрования
};

#endif