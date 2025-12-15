#include <winsock2.h>
// порядок подключения
#include <windows.h>
// специализированные заголовки Windows
#include <VersionHelpers.h>
#include <lz4.h>
#include <winioctl.h>

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

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

string encrypt(const string& jsonStr, Settings& settigns) { return jsonStr; }

void SendMessageAndMessageSize(SOCKET& client_socket, string& jsonStr,
                               Settings& settings) {
    if ((jsonStr.size()) > kMaxBuf) {
        cout << "Message length exceeds the allowed sending length. Message "
                "cannot be sent."
             << endl;
        return;
    }
    cout << "JSON size: " << jsonStr.size();
    string compress_json_str = CompressJsonString(jsonStr);
    cout << " | Compressed size: " << compress_json_str.size() << endl;

    jsonStr = encrypt(jsonStr, settings);

    // 1. Подготовка длины (в network byte order)
    uint32_t len = static_cast<uint32_t>(jsonStr.size());
    uint32_t lenNetwork = htonl(len);  // host → network (big-endian)

    // 2. Отправка длины
    if (send(client_socket, reinterpret_cast<const char*>(&lenNetwork),
             sizeof(lenNetwork), 0) == SOCKET_ERROR) {
        cout << "send(char_size_message) failed with error: "
             << WSAGetLastError() << endl;
        return;
    }

    if (send(client_socket, jsonStr.c_str(), (int)jsonStr.size(), 0) ==
        SOCKET_ERROR) {
        cout << "send() failed with error: " << WSAGetLastError() << endl;
        return;
    }
}
