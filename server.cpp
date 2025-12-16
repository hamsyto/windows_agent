// server.cpp
//  g++ -Ilibs/lz4 libs\lz4\lz4hc.c libs\lz4\lz4.c  -g server.cpp -o
//  compiled/server.exe -liphlpapi -lwbemuuid -loleaut32 -lole32 -lnetapi32
//  -lssl -lcrypto -lcrypt32 -lwsock32 -lgdi32 -Ilibs/lz4  -lws2_32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

// === ДОБАВЛЕНО: библиотеки для шифрования и сжатия ===
#include <lz4.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <winioctl.h>

using namespace std;

// === Конфигурация ===
constexpr const char* kIpAddr = "127.0.0.1";
constexpr int kPort = 8101;
constexpr int kQueue = 3;
const string kEncryptionKey =
    "k7mQp9xL2vNwRfTbYhUjIoAeSdGcVnXz";  // 32-байтный ключ!

std::atomic<int> g_reportCounter{0};
std::mutex g_fileMutex;

// === Функция распаковки LZ4 (raw format) ===
std::string decompress_lz4(const std::vector<unsigned char>& compressed,
                           size_t max_uncompressed_size) {
    std::string uncompressed(max_uncompressed_size, '\0');
    int result = LZ4_decompress_safe(
        reinterpret_cast<const char*>(compressed.data()), uncompressed.data(),
        static_cast<int>(compressed.size()),
        static_cast<int>(max_uncompressed_size));
    if (result <= 0) {
        throw runtime_error("LZ4 decompression failed");
    }
    uncompressed.resize(result);
    return uncompressed;
}

// === Генерация имени файла ===
std::string GenerateFilename() {
    int id = ++g_reportCounter;
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "report_%03d.json", id);
    return std::string(buffer);
}

// === Сохранение JSON ===
bool SaveJsonToFile(const std::string& jsonStr) {
    std::lock_guard<std::mutex> lock(g_fileMutex);
    std::string filename = GenerateFilename();

    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to create file: " << filename << std::endl;
        return false;
    }
    file << jsonStr;
    file.close();
    std::cout << "Saved: " << filename << std::endl;
    return true;
}

// === Обработка клиента ===
void HandleClient(SOCKET clientSocket) {
    try {
        // 1. Читаем nonce для длины (12 байт)
        std::array<unsigned char, 12> nonce_len;
        recv_all(clientSocket, reinterpret_cast<char*>(nonce_len.data()), 12);

        // 2. Читаем зашифрованную длину (4 байта)
        std::string encrypted_len(4, '\0');
        recv_all(clientSocket, encrypted_len.data(), 4);

        // 3. Расшифровываем длину
        std::string len_raw =
            ChaCha20DecryptRaw(encrypted_len, kEncryptionKey, nonce_len);
        if (len_raw.size() != 4)
            throw runtime_error("Invalid len after decrypt");
        uint32_t body_len =
            ntohl(*reinterpret_cast<const uint32_t*>(len_raw.data()));

        if (body_len == 0 || body_len > 2 * 1024 * 1024)
            throw runtime_error("Invalid body length");

        // 4. Читаем nonce для тела (12 байт)
        std::array<unsigned char, 12> nonce_body;
        recv_all(clientSocket, reinterpret_cast<char*>(nonce_body.data()), 12);

        // 5. Читаем зашифрованное тело
        std::string encrypted_body(body_len, '\0');
        recv_all(clientSocket, encrypted_body.data(), body_len);

        // 6. Расшифровываем и распаковываем
        std::string compressed =
            ChaCha20DecryptRaw(encrypted_body, kEncryptionKey, nonce_body);
        std::string jsonStr = decompress_lz4(
            std::vector<unsigned char>(compressed.begin(), compressed.end()),
            8 * 1024 * 1024);

        SaveJsonToFile(jsonStr);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    closesocket(clientSocket);
}

// Расшифровка с заданным nonce
std::string ChaCha20DecryptRaw(const std::string& ciphertext,
                               const std::string& key,
                               const std::array<unsigned char, 12>& nonce) {
    if (key.size() != 32) {
        throw std::runtime_error("Key must be 32 bytes");
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("Failed to create cipher context");

    if (EVP_DecryptInit_ex(ctx, EVP_chacha20(), nullptr,
                           reinterpret_cast<const unsigned char*>(key.data()),
                           nonce.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("DecryptInit failed");
    }

    string plaintext(ciphertext.size() + 16);
    int len = 0;
    int total_len = 0;

    if (EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext.data(),
                          static_cast<int>(ciphertext.size())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("DecryptUpdate failed");
    }
    total_len = len;

    if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("DecryptFinal failed");
    }
    total_len += len;

    EVP_CIPHER_CTX_free(ctx);
    plaintext.resize(total_len);
    return plaintext;
}

// === Вспомогательная функция (оставлена для совместимости) ===
array<char, 4> int32_to_network_bytes(int32_t value) {
    uint32_t net_val = htonl(static_cast<uint32_t>(value));
    std::array<char, 4> buffer;
    memcpy(buffer.data(), &net_val, sizeof(net_val));
    return buffer;
}

// === main ===
int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return 1;
    }

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        std::cerr << "socket() failed." << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(kPort);
    addr.sin_addr.s_addr = inet_addr(kIpAddr);

    if (bind(listenSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) ==
        SOCKET_ERROR) {
        std::cerr << "bind() failed." << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    if (listen(listenSocket, kQueue) == SOCKET_ERROR) {
        std::cerr << "listen() failed." << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server listening on " << kIpAddr << ":" << kPort << std::endl;

    int x = 0;
    while (true) {
        SOCKET clientSocket = accept(listenSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "accept() failed." << std::endl;
            continue;
        }

        std::cout << "Client connected." << std::endl;
        if (x == 0) {
            auto buffer = int32_to_network_bytes(10);
            send(clientSocket, buffer.data(), buffer.size(), 0);
            x = 1;
        }

        thread(HandleClient, clientSocket).detach();
    }

    closesocket(listenSocket);
    WSACleanup();
    return 0;
}