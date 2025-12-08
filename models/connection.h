#include <winsock2.h> 
// порядок подключения
#include <windows.h>
#include "windows_simple_message.h"

class Connection {
public:
    bool connected;
    SOCKET socket;

    // Конструктор получает Settings по константной ссылке
    Connection(Settings& settings) 
        : connected(false), socket(INVALID_SOCKET) {
        // Здесь можно добавить логику инициализации, например:
        // socket = socket(AF_INET, SOCK_STREAM, 0);
        // ...настройка подключения по данным из settings...
    }

    // Отправка бинарных данных
    bool Send(const void* data, size_t size) {
        if (socket == INVALID_SOCKET || !connected)
            return false;
        int sent = send(socket, static_cast<const char*>(data), static_cast<int>(size), 0);
        return (sent == static_cast<int>(size));
    }

    // Получение бинарных данных (ожидаем ровно size байт)
    bool Receive(void* buffer, size_t size) {
        if (socket == INVALID_SOCKET || !connected)
            return false;
        size_t total = 0;
        char* ptr = static_cast<char*>(buffer);
        while (total < size) {
            int received = recv(socket, ptr + total, static_cast<int>(size - total), 0);
            if (received <= 0) {
                // Ошибка или соединение закрыто
                return false;
            }
            total += static_cast<size_t>(received);
        }
        return true;
    }
};