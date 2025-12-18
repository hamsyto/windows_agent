// connection.h

#ifndef CONNECTION_H
#define CONNECTION_H

#include <winsock2.h>
// порядок подключения
#include <windows.h>

#include <cstdint>

#include "IConnection.h"
#include "message.h"
#include "settings.h"

class Connection {
   public:
    bool connected;
    SOCKET Socket;
    uint32_t agent_id;
    Settings settings;

    // Конструктор получает Settings по константной ссылке
    Connection();

    // публичные методы

    // static Connection InitClient(Settings& settings);

    // инициализирует winsock
    int WinsockInit();

    SOCKET InitConnectSocket();
    int ConnectServer();
    // закрывает сокет с проверкой на закрытие
    void CloseSocketCheck(SOCKET& Socket);

    int32_t Registration();
    void SendData();
    // задаёт тип сообщениия error и отправляет json с текстом ошибки
    void SendTypeMsgError(std::string& error);
    void SendBytes();
    uint32_t RecvBytes();

   private:
    // приватные методы

    void Connect();
    void Disconnect();

    uint32_t NetworkBufferToUint32(char* buffer);
    bool SaveAgentID(uint32_t& id);
    bool WriteIntToFile(const std::string& filename, uint32_t& id);
    /*
        // Отправка бинарных данных
        bool Send(const void* data, size_t size);

        // Получение бинарных данных (ожидаем ровно size байт)
        bool Receive(void* buffer, size_t size);
    */
    SOCKET GetSocket() const;
    unsigned int GetAgentID() const;
};

#endif