// chacha_handler.h
#ifndef CHACHA_HANDLER_H
#define CHACHA_HANDLER_H

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "../../consts.h"

// Простая структура настроек
struct CryptoSettings {
    std::string listener_key;  // Ключ в виде строки

    CryptoSettings(const std::string& key) : listener_key(key) {}
};

namespace agent::crypto {

/**
 * @brief Класс для шифрования/дешифрования ChaCha20-Poly1305
 */
class ChaChaHandler {
   public:
    explicit ChaChaHandler(const CryptoSettings& settings);

    /**
     * @brief Шифрует данные
     * @param plaintext Исходные данные
     * @return Вектор в формате: [nonce:12][ciphertext][tag:16]
     */
    std::vector<uint8_t> encrypt(const std::vector<uint8_t>& plaintext);

    /**
     * @brief Дешифрует данные
     * @param message Сообщение в формате: [nonce:12][ciphertext][tag:16]
     * @return Расшифрованные данные
     */
    std::vector<uint8_t> decrypt(const std::vector<uint8_t>& message);

    /**
     * @brief Шифрует данные с заданным nonce (для тестирования)
     * @param plaintext Исходные данные
     * @param nonce Nonce для использования (12 байт)
     * @return Вектор: [ciphertext][tag:16]
     */
    std::vector<uint8_t> encrypt_with_nonce(
        const std::vector<uint8_t>& plaintext,
        const std::array<uint8_t, kNonceSize>& nonce);

    /**
     * @brief Дешифрует данные с заданным nonce
     * @param ciphertext_with_tag Данные в формате: [ciphertext][tag:16]
     * @param nonce Nonce (12 байт)
     * @return Расшифрованные данные
     */
    std::vector<uint8_t> decrypt_with_nonce(
        const std::vector<uint8_t>& ciphertext_with_tag,
        const std::array<uint8_t, kNonceSize>& nonce);

    /**
     * @brief Создает сетевое сообщение с заголовком длины
     * @param plaintext Исходные данные
     * @return Буфер для отправки: [4 байта длины][nonce:12][ciphertext][tag:16]
     */
    std::vector<uint8_t> create_network_message(
        const std::vector<uint8_t>& plaintext);

    /**
     * @brief Парсит сетевое сообщение
     * @param network_message Сообщение в формате: [4 байта
     * длины][nonce:12][ciphertext][tag:16]
     * @return Пару (nonce, ciphertext_with_tag)
     */
    std::pair<std::array<uint8_t, kNonceSize>, std::vector<uint8_t>>
    parse_network_message(const std::vector<uint8_t>& network_message);

   private:
    CryptoSettings settings_;
    std::array<uint8_t, kKeySize> key_;

    // Подготовка ключа
    std::array<uint8_t, kKeySize> prepare_key(const std::string& listener_key);

    // Генерация случайного nonce
    std::array<uint8_t, kNonceSize> generate_nonce();

    // Внутреннее шифрование
    std::vector<uint8_t> encrypt_internal(
        const std::vector<uint8_t>& plaintext,
        const std::array<uint8_t, kNonceSize>& nonce);

    // Внутреннее дешифрование
    std::vector<uint8_t> decrypt_internal(
        const std::vector<uint8_t>& ciphertext_with_tag,
        const std::array<uint8_t, kNonceSize>& nonce);
};

}  // namespace agent::crypto

#endif  // AGENT_CHACHA_HANDLER_H