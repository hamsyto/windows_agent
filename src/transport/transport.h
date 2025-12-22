#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <cstdint>
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
  // Вспомогательные методы
  std::string Compress(const std::string& data);
  std::string Encrypt(const std::string& data);
  std::string Decrypt(const std::string& encryptedData);

  // Методы сериализации длины (Little/Big Endian independent)
  std::string LenToSend(const std::string& data_str);
  uint32_t BytesToLength(const std::vector<char>& headerData);

  std::vector<unsigned char> DeriveKey(const std::string& rawKey);

 private:
  const Settings& settings_;
  IConnection& connection_;

  static constexpr size_t NONCE_SIZE = 12;
  static constexpr size_t TAG_SIZE = 16;
};

#endif