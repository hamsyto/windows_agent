// transport/transport.h

#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../connection/IConnection.h"
#include "../models/settings.h"

class Transport {
 public:
  Transport(const Settings& settings, IConnection& connection);
  virtual ~Transport() = default;

  // Публичный интерфейс
  void SendData(const std::string& jsonStr);
  std::string RecvData();

 protected:
  // Вспомогательные методы трансформации данных
  std::string Compress(const std::string& data);
  std::string Encrypt(const std::string& data);
  std::string Decrypt(const std::string& encryptedData);

  // Хеширование ключа (SHA256)
  std::vector<unsigned char> DeriveKey(const std::string& rawKey);

 private:
  const Settings& m_settings;
  IConnection& m_connection;

  // Константы для протокола
  static constexpr size_t NONCE_SIZE = 12;
  static constexpr size_t TAG_SIZE = 16;
};

#endif