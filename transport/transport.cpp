// transport.cpp
#include <winsock2.h>
// порядок подключения
#include <windows.h>
// специализированные заголовки Windows
#include <VersionHelpers.h>
#include <bcrypt.h>  // для BCryptGenRandom
#include <lz4.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <wincrypt.h>
#include <windows.h>
#include <winioctl.h>

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../consts.h"
#include "transport.h"
using namespace std;

string CompressJsonString(string& jsonStr) {
    if (jsonStr.empty()) {
        return {};  // пустая строка → пустой результат
    }

    // 1. Определяем размер входных данных
    size_t src_size = jsonStr.size();
    int src_size_int = static_cast<int>(src_size);

    // 2. Получаем верхнюю границу размера сжатых данных
    int max_compressed_size = LZ4_compressBound(src_size_int);
    if (max_compressed_size <= 0) {
        throw runtime_error("LZ4_compressBound failed");
    }

    // 3. Выделяем выходной буфер
    string compressed;
    compressed.resize(static_cast<size_t>(max_compressed_size));

    // 4. Сжимаем
    int compressedSize =
        LZ4_compress_default(jsonStr.data(),      // вход
                             compressed.data(),   // выход
                             src_size_int,        // размер входа
                             max_compressed_size  // макс. размер выхода
        );

    if (compressedSize <= 0) {
        throw runtime_error("LZ4 compression failed");
    }

    // 5. Обрезаем до реального размера
    compressed.resize(static_cast<size_t>(compressedSize));
    return compressed;
}

// Основная функция шифрования
/*
string EncryptRawWithNonce(const string& jsonStr, const string& key) {
    if ((key).size() != 32) {
        throw invalid_argument("ChaCha20 key must be 32 bytes (256 bits).");
    }

    // Генерация случайного 12-байтного nonce
    vector<unsigned char> nonce(12);
    if (!RAND_bytes(nonce.data(), static_cast<int>(nonce.size()))) {
        throw runtime_error("Failed to generate random nonce.");
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw runtime_error("Failed to create cipher context.");
    }

    if (EVP_EncryptInit_ex(ctx, EVP_chacha20(), nullptr,
                           reinterpret_cast<const unsigned char*>((key).data()),
                           nonce.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw runtime_error("Failed to initialize ChaCha20 encryption.");
    }

    vector<unsigned char> ciphertext(jsonStr.size() +
                                     16);  // +16 для подстраховки
    int len = 0;
    int ciphertext_len = 0;

    if (EVP_EncryptUpdate(
            ctx, ciphertext.data(), &len,
            reinterpret_cast<const unsigned char*>(jsonStr.data()),
            static_cast<int>(jsonStr.size())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw runtime_error("Encryption update failed.");
    }
    ciphertext_len = len;

    if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw runtime_error("Encryption final failed.");
    }
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    // Собираем nonce + шифротекст
    vector<unsigned char> result;
    result.insert(result.end(), nonce.begin(), nonce.end());
    result.insert(result.end(), ciphertext.begin(),
                  ciphertext.begin() + ciphertext_len);

    // Возвращаем как base64
    return Base64Encode(result.data(), result.size());
}
*/
void printNonceAsInts(const std::vector<unsigned char>& nonce) {
    for (unsigned char b : nonce) {
        std::cout << static_cast<int>(b) << ' ';
    }
    std::cout << '\n';
}

void printSha256(const std::vector<char>& data) {
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    BYTE rgbHash[32] = {0};
    DWORD cbHash = sizeof(rgbHash);

    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES,
                             CRYPT_VERIFYCONTEXT)) {
        std::cerr << "CryptAcquireContext failed.\n";
        return;
    }

    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
        std::cerr << "CryptCreateHash failed.\n";
        CryptReleaseContext(hProv, 0);
        return;
    }

    if (!CryptHashData(hHash, reinterpret_cast<const BYTE*>(data.data()),
                       static_cast<DWORD>(data.size()), 0)) {
        std::cerr << "CryptHashData failed.\n";
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return;
    }

    if (!CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0)) {
        std::cerr << "CryptGetHashParam failed.\n";
    } else {
        std::ostringstream oss;
        for (DWORD i = 0; i < cbHash; ++i) {
            oss << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(rgbHash[i]);
        }
        std::cout << "SHA-256: " << oss.str() << '\n';
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
}

string EncryptRawWithNonce(const string& jsonStr, const string& key) {
    // Проверка длины ключа
    if (key.size() != 32) {
        throw std::invalid_argument("ChaCha20 key must be exactly 32 bytes.");
    }

    // Генерация 12-байтного nonce
    std::vector<unsigned char> nonce(12);
    if (!RAND_bytes(nonce.data(), static_cast<int>(nonce.size()))) {
        throw runtime_error("Failed to generate random nonce.");
    }
    printNonceAsInts(nonce);
    // Инициализация контекста шифрования
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create cipher context.");
    }

    if (EVP_EncryptInit_ex(ctx, EVP_chacha20(), nullptr,
                           reinterpret_cast<const unsigned char*>(key.data()),
                           nonce.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize ChaCha20 encryption.");
    }

    // Шифрование
    std::vector<unsigned char> ciphertext(jsonStr.size() +
                                          16);  // +16 для подстраховки
    int len = 0;
    int ciphertext_len = 0;

    if (EVP_EncryptUpdate(
            ctx, ciphertext.data(), &len,
            reinterpret_cast<const unsigned char*>(jsonStr.data()),
            static_cast<int>(jsonStr.size())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Encryption update failed.");
    }
    ciphertext_len = len;

    if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Encryption final failed.");
    }
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    // Формируем результат: [nonce][ciphertext] как бинарная строка
    std::string result;
    result.reserve(nonce.size() + ciphertext_len);
    result.append(reinterpret_cast<const char*>(nonce.data()), nonce.size());
    result.append(reinterpret_cast<const char*>(ciphertext.data()),
                  ciphertext_len);

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
    string comressed = CompressJsonString(jsonStr);
    string encrypted = EncryptRawWithNonce(comressed, settings.key);
    cout << "Compressed size: " << comressed.size() << " ";
    cout << " | Encrypted size: " << encrypted.size() << endl;

    // 1. подготовка длины (в network byte order) на отправку
    string len_network = LenPreparationMessToSend(encrypted);

    string mess_str = len_network + encrypted;
    std::vector<char> mess_char(mess_str.begin(), mess_str.end());
    printSha256(mess_char);  // ← вызов перед send
    // Если нужен завершающий \0 (для C-функций):
    mess_char.push_back('\0');

    printSha256(mess_char);  // ← вызов перед send
    send(client_socket, mess_char.data(), mess_char.size(), 0);
}
