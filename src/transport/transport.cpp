#include "transport.h"

#include <lz4.h>
#include <lz4frame.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <winsock2.h>

#include <stdexcept>

Transport::Transport(const Settings& settings, IConnection& connection)
    : m_settings(settings), m_connection(connection) {}

// --- Публичные методы ---

void Transport::SendData(const std::string& jsonStr) {
  // 1. Сжимаем
  std::string compressed = Compress(jsonStr);
  // 2. Шифруем
  std::string encrypted = Encrypt(compressed);

  // 3. Подготавливаем заголовок длины (network byte order)
  uint32_t len = static_cast<uint32_t>(encrypted.size());
  uint32_t len_network = htonl(len);

  std::string header(reinterpret_cast<const char*>(&len_network),
                     sizeof(len_network));

  // 4. Отправляем: сначала заголовок, потом тело
  m_connection.Send(header);
  m_connection.Send(encrypted);
}

std::string Transport::RecvData() {
  // 1. Получаем 4 байта длины
  std::vector<char> headerData = m_connection.Recv(4);
  if (headerData.size() < 4) {
    throw std::runtime_error("Failed to receive message header");
  }

  uint32_t len_network;
  memcpy(&len_network, headerData.data(), 4);
  uint32_t payloadSize = ntohl(len_network);

  // 2. Получаем само зашифрованное сообщение
  std::vector<char> encryptedRaw = m_connection.Recv(payloadSize);
  if (encryptedRaw.size() < payloadSize) {
    throw std::runtime_error("Incomplete message body received");
  }

  std::string encryptedStr(encryptedRaw.begin(), encryptedRaw.end());

  // 3. Расшифровываем (по вашему запросу - без разархивации)
  return Decrypt(encryptedStr);
}

// --- Защищенные методы (логика) ---

std::string Transport::Compress(const std::string& data) {
  if (data.empty()) return {};

  size_t max_dst_size = LZ4F_compressFrameBound(data.size(), NULL);
  std::string compressed;
  compressed.resize(max_dst_size);

  size_t compressedSize = LZ4F_compressFrame(&compressed[0], max_dst_size,
                                             data.data(), data.size(), NULL);

  if (LZ4F_isError(compressedSize)) {
    throw std::runtime_error("LZ4 compression failed");
  }

  compressed.resize(compressedSize);
  return compressed;
}

std::vector<unsigned char> Transport::DeriveKey(const std::string& rawKey) {
  std::vector<unsigned char> hash(SHA256_DIGEST_LENGTH);
  SHA256(reinterpret_cast<const unsigned char*>(rawKey.c_str()), rawKey.size(),
         hash.data());
  return hash;
}

std::string Transport::Encrypt(const std::string& data) {
  auto key = DeriveKey(m_settings.key);
  std::vector<unsigned char> nonce(NONCE_SIZE);
  RAND_bytes(nonce.data(), NONCE_SIZE);

  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), nullptr, nullptr, nullptr);
  EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, NONCE_SIZE, nullptr);
  EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data(), nonce.data());

  std::string ciphertext;
  ciphertext.resize(data.size());
  int len = 0;
  EVP_EncryptUpdate(ctx, reinterpret_cast<unsigned char*>(&ciphertext[0]), &len,
                    reinterpret_cast<const unsigned char*>(data.data()),
                    data.size());

  int final_len = 0;
  EVP_EncryptFinal_ex(ctx, nullptr, &final_len);

  unsigned char tag[TAG_SIZE];
  EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, TAG_SIZE, tag);
  EVP_CIPHER_CTX_free(ctx);

  // Сборка: nonce + ciphertext + tag
  std::string result;
  result.reserve(NONCE_SIZE + ciphertext.size() + TAG_SIZE);
  result.append(reinterpret_cast<const char*>(nonce.data()), NONCE_SIZE);
  result.append(ciphertext);
  result.append(reinterpret_cast<const char*>(tag), TAG_SIZE);

  return result;
}

std::string Transport::Decrypt(const std::string& encryptedData) {
  if (encryptedData.size() < NONCE_SIZE + TAG_SIZE) {
    throw std::runtime_error("Encrypted message too short");
  }

  auto key = DeriveKey(m_settings.key);

  // Извлекаем компоненты
  const char* pRaw = encryptedData.data();
  const unsigned char* nonce = reinterpret_cast<const unsigned char*>(pRaw);
  const unsigned char* ciphertext =
      reinterpret_cast<const unsigned char*>(pRaw + NONCE_SIZE);
  size_t cipherLen = encryptedData.size() - NONCE_SIZE - TAG_SIZE;
  const unsigned char* tag =
      reinterpret_cast<const unsigned char*>(pRaw + NONCE_SIZE + cipherLen);

  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  EVP_DecryptInit_ex(ctx, EVP_chacha20_poly1305(), nullptr, nullptr, nullptr);
  EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, NONCE_SIZE, nullptr);
  EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.data(), nonce);

  std::string plaintext;
  plaintext.resize(cipherLen);
  int len = 0;
  EVP_DecryptUpdate(ctx, reinterpret_cast<unsigned char*>(&plaintext[0]), &len,
                    ciphertext, cipherLen);

  // Установка тега ПЕРЕД Final
  EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_TAG, TAG_SIZE,
                      const_cast<unsigned char*>(tag));

  int ret = EVP_DecryptFinal_ex(ctx, nullptr, &len);
  EVP_CIPHER_CTX_free(ctx);

  if (ret <= 0) {
    throw std::runtime_error(
        "Decryption failed (tag mismatch or data corrupted)");
  }

  return plaintext;
}