// windowsConnection.h
#ifndef WINDOWS_CONNECTION_H
#define WINDOWS_CONNECTION_H

#include <winsock2.h>
// порядок подключения
#include <windows.h>

#include "../IConnection.h"

class WindowsConnection : public IConnection {
   private:
    Settings settings_;
    SOCKET socket_ = INVALID_SOCKET;

   public:
    // explicit
    WindowsConnection(const Settings& settings);
    ~WindowsConnection();

    bool Connect() override;
    bool Disconnect() override;
    void Send(const std::string& data) override;
    std::vector<char> Recv(size_t maxBytes = 4096) override;

    // Вспомогательный метод (опционально)
    SOCKET GetSocket() const { return socket_; }
};

#endif  // WINDOWS_CONNECTION_H