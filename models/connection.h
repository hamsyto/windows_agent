// connection.h

#ifndef CONNECTION_H
#define CONNECTION_H

#include <winsock2.h>
// порядок подключения
#include <windows.h>

#include <cstdint>

#include "windows_simple_message.h"

class Connection {
 public:
  bool connected;
  SOCKET socket;
  uint32_t agent_id;

  // Конструктор получает Settings по константной ссылке
  Connection();

  // Методы
  void Connect(Settings& settings);

  void Disconnect();

  bool SaveAgentID(uint32_t& id);
  /*
      // Отправка бинарных данных
      bool Send(const void* data, size_t size);

      // Получение бинарных данных (ожидаем ровно size байт)
      bool Receive(void* buffer, size_t size);
  */
  SOCKET GetSocket() const;
  unsigned int GetAgentID() const;
};

#endif