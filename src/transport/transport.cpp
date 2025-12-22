#include "transport.h"

#include <lz4.h>
#include <lz4frame.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include <iostream>
#include <stdexcept>

Transport::Transport(const Settings& settings, IConnection& connection)
    : settings_(settings), connection_(connection) {}

// 1. Превращаем длину данных в строку 4 байт (Big Endian)
std::string Transport::LenToSend(const std::string& data_str) {
  uint32_t len = static_cast<uint32_t>(data_str.size());
  char header[4];

  // Ручная упаковка (работает везде одинаково)
  header[0] = static_cast<char>((len >> 24) & 0xFF);
  header[1] = static_cast<char>((len >> 16) & 0xFF);
  header[2] = static_cast<char>((len >> 8) & 0xFF);
  header[3] = static_cast<char>((len) & 0xFF);

  return std::string(header, 4);
}

// 2. Превращаем 4 байта (Big Endian) обратно в число
uint32_t Transport::BytesToLength(const std::vector<char>& headerData) {
  if (headerData.size() < 4) {
    throw std::runtime_error("Header size must be at least 4 bytes");
  }

  // Приводим к unsigned, чтобы битовые сдвиги не "размножали" знак
  const unsigned char* buf =
      reinterpret_cast<const unsigned char*>(headerData.data());

  // Собираем число обратно из Big Endian
  uint32_t len = (static_cast<uint32_t>(buf[0]) << 24) |
                 (static_cast<uint32_t>(buf[1]) << 16) |
                 (static_cast<uint32_t>(buf[2]) << 8) |
                 (static_cast<uint32_t>(buf[3]));
  return len;
}

void Transport::SendData(const std::string& jsonStr) {
  std::string compressed = Compress(jsonStr);
  std::string encrypted = Encrypt(compressed);
  std::string header = LenToSend(encrypted);
  connection_.Send(header + encrypted);
}

// --- Версия без сжатия и шифрования ---
// void Transport::SendData(const std::string& jsonStr) {
//   std::string header = LenToSend(jsonStr);
//   connection_.Send(header + jsonStr);
// }

std::string Transport::RecvData() {
  // 1. Получаем 4 байта
  std::vector<char> headerData = connection_.Recv(4);
  if (headerData.size() < 4) {
    throw std::runtime_error("Failed to receive message header");
  }

  // 2. Декодируем длину с помощью нового метода
  uint32_t payloadSize = BytesToLength(headerData);

  // 3. Получаем тело сообщения
  std::vector<char> encryptedRaw = connection_.Recv(payloadSize);
  if (encryptedRaw.size() < payloadSize) {
    throw std::runtime_error("Incomplete message body received");
  }

  std::string encryptedStr(encryptedRaw.begin(), encryptedRaw.end());

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
  auto key = DeriveKey(settings_.key);

  // Генерируем Nonce
  std::vector<unsigned char> nonce(NONCE_SIZE);
  if (RAND_bytes(nonce.data(), NONCE_SIZE) != 1) {
    throw std::runtime_error("RAND_bytes failed");
  }

  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  if (!ctx) throw std::runtime_error("Failed to create EVP_CIPHER_CTX");

  // Инициализация
  if (1 != EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), nullptr, nullptr,
                              nullptr)) {
    EVP_CIPHER_CTX_free(ctx);
    throw std::runtime_error("EVP_EncryptInit_ex failed");
  }

  // Установка длины IV (Nonce) - критично для Poly1305
  if (1 !=
      EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, NONCE_SIZE, nullptr)) {
    EVP_CIPHER_CTX_free(ctx);
    throw std::runtime_error("Set IV len failed");
  }

  // Установка ключа и nonce
  if (1 !=
      EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data(), nonce.data())) {
    EVP_CIPHER_CTX_free(ctx);
    throw std::runtime_error("Key/Nonce init failed");
  }

  // Буфер для шифротекста
  std::vector<unsigned char> ciphertext_buf(data.size() + EVP_MAX_BLOCK_LENGTH);
  int len = 0;
  int ciphertext_len = 0;

  // Шифрование данных
  if (1 !=
      EVP_EncryptUpdate(ctx, ciphertext_buf.data(), &len,
                        reinterpret_cast<const unsigned char*>(data.data()),
                        static_cast<int>(data.size()))) {
    EVP_CIPHER_CTX_free(ctx);
    throw std::runtime_error("EVP_EncryptUpdate failed");
  }
  ciphertext_len = len;

  // Финализация. ВАЖНО: передаем буфер, даже если он не используется для Stream
  // ciphers. Используем хвост буфера или временную переменную.
  int final_len = 0;
  if (1 != EVP_EncryptFinal_ex(ctx, ciphertext_buf.data() + len, &final_len)) {
    EVP_CIPHER_CTX_free(ctx);
    throw std::runtime_error("EVP_EncryptFinal_ex failed");
  }
  ciphertext_len += final_len;

  // Получение тега (MAC)
  unsigned char tag[TAG_SIZE];
  if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, TAG_SIZE, tag)) {
    EVP_CIPHER_CTX_free(ctx);
    throw std::runtime_error("Failed to get Poly1305 tag");
  }

  EVP_CIPHER_CTX_free(ctx);

  // Сборка сообщения: [Nonce 12] + [Ciphertext N] + [Tag 16]
  std::string result;
  result.reserve(NONCE_SIZE + ciphertext_len + TAG_SIZE);
  result.append(reinterpret_cast<const char*>(nonce.data()), NONCE_SIZE);
  result.append(reinterpret_cast<const char*>(ciphertext_buf.data()),
                ciphertext_len);
  result.append(reinterpret_cast<const char*>(tag), TAG_SIZE);

  return result;
}
std::string Transport::Decrypt(const std::string& encryptedData) {
  if (encryptedData.size() < NONCE_SIZE + TAG_SIZE) {
    throw std::runtime_error("Encrypted message too short");
  }

  auto key = DeriveKey(settings_.key);

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