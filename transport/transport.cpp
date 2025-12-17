// transport.cpp
#include <winsock2.h>
// порядок подключения
#include <windows.h>
// специализированные заголовки Windows
#include <lz4.h>
#include <lz4frame.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "../consts.h"
#include "transport.h"
using namespace std;

string CompressJsonString(string& jsonStr) {
    if (jsonStr.empty()) {
        return {};
    }

    size_t src_size = jsonStr.size();

    // 1. Получаем размер буфера, необходимого для Frame (он включает заголовки)
    size_t max_dst_size = LZ4F_compressFrameBound(src_size, NULL);

    string compressed;
    compressed.resize(max_dst_size);

    // 2. Создаем Frame
    // LZ4F_compressFrame сам запишет заголовки и магические байты, которые ждет
    // Python
    size_t compressedSize =
        LZ4F_compressFrame(compressed.data(),  // Куда писать
                           max_dst_size,       // Размер буфера
                           jsonStr.data(),     // Что сжимать
                           src_size,           // Размер исходных данных
                           NULL                // Настройки по умолчанию
        );

    if (LZ4F_isError(compressedSize)) {
        throw runtime_error(string("LZ4 Frame compression failed: ") +
                            LZ4F_getErrorName(compressedSize));
    }

    // 3. Обрезаем до реального размера
    compressed.resize(compressedSize);
    return compressed;
}

#include <openssl/sha.h>

// 1. Хешируем ключ как в Python
std::vector<unsigned char> DeriveKey(const std::string& raw_key) {
    std::vector<unsigned char> hash(SHA256_DIGEST_LENGTH);
    SHA256(reinterpret_cast<const unsigned char*>(raw_key.c_str()),
           raw_key.size(), hash.data());
    return hash;
}

string EncryptRawWithNonce(const string& jsonStr, const string& raw_key) {
    auto key = DeriveKey(raw_key);
    std::vector<unsigned char> nonce(12);
    RAND_bytes(nonce.data(), 12);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), nullptr, nullptr, nullptr);

    // Установка длины nonce (по умолчанию 12, но лучше явно)
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, 12, nullptr);
    EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data(), nonce.data());

    std::vector<unsigned char> ciphertext(jsonStr.size());
    int len = 0;
    EVP_EncryptUpdate(ctx, ciphertext.data(), &len,
                      reinterpret_cast<const unsigned char*>(jsonStr.data()),
                      jsonStr.size());

    EVP_EncryptFinal_ex(ctx, nullptr, &len);

    // ПОЛУЧЕНИЕ ТЕГА (16 байт) - Критично для ChaCha20Poly1305
    unsigned char tag[16];
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag);
    EVP_CIPHER_CTX_free(ctx);

    // Формируем результат: [nonce(12)] + [ciphertext(N)] + [tag(16)]
    std::string result;
    result.append(reinterpret_cast<const char*>(nonce.data()), 12);
    result.append(reinterpret_cast<const char*>(ciphertext.data()),
                  ciphertext.size());
    result.append(reinterpret_cast<const char*>(tag), 16);

    return result;
}

// Подготовка длины (в network byte order) на отправку
string LenPreparationMessToSend(const string& text_str) {
    uint32_t len = static_cast<uint32_t>(text_str.size());
    uint32_t len_network = htonl(len);
    // вектор (начало_диапазона, конец_буфера)
    return string(
        reinterpret_cast<const char*>(&len_network),
        reinterpret_cast<const char*>(&len_network) + sizeof(len_network));
}

void SendMessageAndMessageSize(SOCKET& client_socket, string& jsonStr,
                               Settings& settings) {
    string compressed = CompressJsonString(jsonStr);
    string encrypted = EncryptRawWithNonce(compressed, settings.key);

    uint32_t len = static_cast<uint32_t>(encrypted.size());
    uint32_t len_network = htonl(len);

    // Отправляем заголовок (4 байта)
    send(client_socket, reinterpret_cast<const char*>(&len_network), 4, 0);
    // Отправляем тело (nonce + cipher + tag)
    send(client_socket, encrypted.data(), encrypted.size(), 0);

    // НИКАКИХ mess_char.push_back('\0')!
}