#include <sodium.h>

#include <array>
#include <string>
#include <vector>

#include "consts.h"

namespace agent::crypto {

class ChaChaHandler {
 public:
  explicit ChaChaHandler(const Settings& settings) {
    if (sodium_init() < 0) {
      throw std::runtime_error("Failed to initialize libsodium");
    }

    // Подготавливаем ключ
    key_ = prepare_key(settings);
  }

  std::vector<uint8_t> encrypt(const std::vector<uint8_t>& plaintext) {
    // Генерируем nonce
    std::array<uint8_t, kNonceSize> nonce;
    randombytes_buf(nonce.data(), kNonceSize);

    // Шифруем
    std::vector<uint8_t> ciphertext(plaintext.size() +
                                    crypto_aead_chacha20poly1305_ABYTES);
    unsigned long long ciphertext_len;

    crypto_aead_chacha20poly1305_encrypt(
        ciphertext.data(), &ciphertext_len, plaintext.data(), plaintext.size(),
        nullptr, 0,  // дополнительная аутентифицируемая информация
        nullptr,     // nsec (не используется)
        nonce.data(), key_.data());

    // Формируем результат: nonce + ciphertext
    std::vector<uint8_t> result;
    result.reserve(kNonceSize + ciphertext_len);
    result.insert(result.end(), nonce.begin(), nonce.end());
    result.insert(result.end(), ciphertext.begin(),
                  ciphertext.begin() + ciphertext_len);

    return result;
  }

 private:
  std::array<uint8_t, kKeySize> key_;

  std::array<uint8_t, kKeySize> prepare_key(const Settings& settings) {
    std::array<uint8_t, kKeySize> key;
    crypto_generichash(
        key.data(), key.size(),
        reinterpret_cast<const unsigned char*>(settings.listener_key.data()),
        settings.listener_key.size(), nullptr, 0);  // без ключа для хэширования
    return key;
  }
};

}  // namespace agent::crypto