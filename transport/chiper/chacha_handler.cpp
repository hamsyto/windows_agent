// chacha_handler.cpp
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include <cstring>
#include <iomanip>
#include <sstream>

#include "chacha_handler.h"
#include "consts.h"

namespace agent::crypto {

ChaChaHandler::ChaChaHandler(const CryptoSettings& settings)
    : settings_(settings) {
  // Подготавливаем ключ при инициализации
  key_ = prepare_key(settings_.listener_key);
}

std::array<uint8_t, kKeySize> ChaChaHandler::prepare_key(
    const std::string& listener_key) {
  std::array<uint8_t, kKeySize> key;

  // Хэшируем ключ с помощью SHA-256 до 32 байт
  SHA256_CTX sha256;
  SHA256_Init(&sha256);
  SHA256_Update(&sha256, listener_key.data(), listener_key.size());
  SHA256_Final(key.data(), &sha256);

  return key;
}

std::array<uint8_t, kNonceSize> ChaChaHandler::generate_nonce() {
  std::array<uint8_t, kNonceSize> nonce;

  // Генерируем криптографически безопасные случайные байты
  if (RAND_bytes(nonce.data(), kNonceSize) != 1) {
    throw std::runtime_error("Failed to generate random nonce");
  }

  return nonce;
}

std::vector<uint8_t> ChaChaHandler::encrypt_internal(
    const std::vector<uint8_t>& plaintext,
    const std::array<uint8_t, kNonceSize>& nonce) {
  // Создаем контекст шифрования
  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  if (!ctx) {
    throw std::runtime_error("Failed to create EVP context");
  }

  // Инициализируем шифрование ChaCha20-Poly1305
  if (EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), nullptr, nullptr,
                         nullptr) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    throw std::runtime_error("Failed to initialize ChaCha20-Poly1305");
  }

  // Устанавливаем ключ и nonce
  if (EVP_EncryptInit_ex(ctx, nullptr, nullptr, key_.data(), nonce.data()) !=
      1) {
    EVP_CIPHER_CTX_free(ctx);
    throw std::runtime_error("Failed to set key and nonce");
  }

  // Выделяем буфер для ciphertext + tag
  std::vector<uint8_t> ciphertext(plaintext.size() + kTagSize);

  int out_len = 0;
  int total_len = 0;

  // Шифруем данные
  if (EVP_EncryptUpdate(ctx, ciphertext.data(), &out_len, plaintext.data(),
                        static_cast<int>(plaintext.size())) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    throw std::runtime_error("Encryption failed");
  }
  total_len = out_len;

  // Завершаем шифрование
  if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + total_len, &out_len) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    throw std::runtime_error("Encryption finalization failed");
  }
  total_len += out_len;

  // Получаем аутентификационный tag (16 байт)
  if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, kTagSize,
                          ciphertext.data() + plaintext.size()) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    throw std::runtime_error("Failed to get authentication tag");
  }

  EVP_CIPHER_CTX_free(ctx);

  return ciphertext;
}

std::vector<uint8_t> ChaChaHandler::decrypt_internal(
    const std::vector<uint8_t>& ciphertext_with_tag,
    const std::array<uint8_t, kNonceSize>& nonce) {
  // Проверяем минимальный размер
  if (ciphertext_with_tag.size() < kTagSize) {
    throw std::runtime_error("Ciphertext too short");
  }

  // Создаем контекст дешифрования
  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  if (!ctx) {
    throw std::runtime_error("Failed to create EVP context");
  }

  // Инициализируем дешифрование
  if (EVP_DecryptInit_ex(ctx, EVP_chacha20_poly1305(), nullptr, nullptr,
                         nullptr) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    throw std::runtime_error("Failed to initialize decryption");
  }

  // Устанавливаем ключ и nonce
  if (EVP_DecryptInit_ex(ctx, nullptr, nullptr, key_.data(), nonce.data()) !=
      1) {
    EVP_CIPHER_CTX_free(ctx);
    throw std::runtime_error("Failed to set key and nonce");
  }

  // Размер данных без tag
  size_t data_len = ciphertext_with_tag.size() - kTagSize;
  std::vector<uint8_t> plaintext(data_len);

  int out_len = 0;
  int total_len = 0;

  // Дешифруем данные
  if (EVP_DecryptUpdate(ctx, plaintext.data(), &out_len,
                        ciphertext_with_tag.data(),
                        static_cast<int>(data_len)) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    throw std::runtime_error("Decryption failed");
  }
  total_len = out_len;

  // Устанавливаем tag для проверки аутентификации
  if (EVP_CIPHER_CTX_ctrl(
          ctx, EVP_CTRL_AEAD_SET_TAG, kTagSize,
          const_cast<uint8_t*>(ciphertext_with_tag.data() + data_len)) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    throw std::runtime_error("Failed to set authentication tag");
  }

  // Завершаем дешифрование (проверяет tag)
  if (EVP_DecryptFinal_ex(ctx, plaintext.data() + total_len, &out_len) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    throw std::runtime_error("Authentication failed: invalid tag");
  }
  total_len += out_len;

  EVP_CIPHER_CTX_free(ctx);

  return plaintext;
}

std::vector<uint8_t> ChaChaHandler::encrypt(
    const std::vector<uint8_t>& plaintext) {
  // 1. Генерируем случайный nonce
  auto nonce = generate_nonce();

  // 2. Шифруем данные
  auto ciphertext_with_tag = encrypt_internal(plaintext, nonce);

  // 3. Формируем итоговое сообщение: nonce + ciphertext + tag
  std::vector<uint8_t> result;
  result.reserve(kNonceSize + ciphertext_with_tag.size());

  // Добавляем nonce
  result.insert(result.end(), nonce.begin(), nonce.end());
  // Добавляем ciphertext с tag
  result.insert(result.end(), ciphertext_with_tag.begin(),
                ciphertext_with_tag.end());

  return result;
}

std::vector<uint8_t> ChaChaHandler::decrypt(
    const std::vector<uint8_t>& message) {
  // Проверяем минимальный размер
  if (message.size() < kNonceSize + kTagSize) {
    throw std::runtime_error("Message too short");
  }

  // 1. Извлекаем nonce (первые 12 байт)
  std::array<uint8_t, kNonceSize> nonce;
  std::memcpy(nonce.data(), message.data(), kNonceSize);

  // 2. Извлекаем ciphertext с tag (остальные байты)
  std::vector<uint8_t> ciphertext_with_tag(message.begin() + kNonceSize,
                                           message.end());

  // 3. Дешифруем
  return decrypt_internal(ciphertext_with_tag, nonce);
}

std::vector<uint8_t> ChaChaHandler::encrypt_with_nonce(
    const std::vector<uint8_t>& plaintext,
    const std::array<uint8_t, kNonceSize>& nonce) {
  return encrypt_internal(plaintext, nonce);
}

std::vector<uint8_t> ChaChaHandler::decrypt_with_nonce(
    const std::vector<uint8_t>& ciphertext_with_tag,
    const std::array<uint8_t, kNonceSize>& nonce) {
  return decrypt_internal(ciphertext_with_tag, nonce);
}

std::vector<uint8_t> ChaChaHandler::create_network_message(
    const std::vector<uint8_t>& plaintext) {
  // 1. Шифруем данные
  auto encrypted_message = encrypt(plaintext);

  // 2. Получаем длину зашифрованного сообщения
  uint32_t message_length = static_cast<uint32_t>(encrypted_message.size());

  // 3. Формируем сетевое сообщение: [4 байта длины][сообщение]
  std::vector<uint8_t> network_message;
  network_message.reserve(4 + encrypted_message.size());

  // Добавляем длину (big-endian)
  network_message.push_back(
      static_cast<uint8_t>((message_length >> 24) & 0xFF));
  network_message.push_back(
      static_cast<uint8_t>((message_length >> 16) & 0xFF));
  network_message.push_back(static_cast<uint8_t>((message_length >> 8) & 0xFF));
  network_message.push_back(static_cast<uint8_t>(message_length & 0xFF));

  // Добавляем зашифрованное сообщение
  network_message.insert(network_message.end(), encrypted_message.begin(),
                         encrypted_message.end());

  return network_message;
}

std::pair<std::array<uint8_t, kNonceSize>, std::vector<uint8_t>>
ChaChaHandler::parse_network_message(
    const std::vector<uint8_t>& network_message) {
  // Проверяем минимальный размер
  if (network_message.size() < 4 + kNonceSize + kTagSize) {
    throw std::runtime_error("Network message too short");
  }

  // 1. Извлекаем длину (первые 4 байта)
  uint32_t length = (network_message[0] << 24) | (network_message[1] << 16) |
                    (network_message[2] << 8) | network_message[3];

  // Проверяем длину
  if (network_message.size() < 4 + length) {
    throw std::runtime_error("Network message length mismatch");
  }

  // 2. Извлекаем зашифрованное сообщение
  std::vector<uint8_t> encrypted_message(network_message.begin() + 4,
                                         network_message.begin() + 4 + length);

  // 3. Извлекаем nonce из сообщения
  if (encrypted_message.size() < kNonceSize) {
    throw std::runtime_error("Encrypted message too short for nonce");
  }

  std::array<uint8_t, kNonceSize> nonce;
  std::memcpy(nonce.data(), encrypted_message.data(), kNonceSize);

  // 4. Извлекаем ciphertext с tag
  std::vector<uint8_t> ciphertext_with_tag(
      encrypted_message.begin() + kNonceSize, encrypted_message.end());

  return {nonce, ciphertext_with_tag};
}

}  // namespace agent::crypto