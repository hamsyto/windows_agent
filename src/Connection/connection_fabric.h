// connection_fabric.h

#ifndef CONNECTION_FABRIC_H
#define CONNECTION_FABRIC_H

#include "IConnection.h"
#include "windows/connection.cpp"

bool WindowsConnection::Connect() {
    if (socket_ != INVALID_SOCKET) {
        Disconnect();
    }
    if (!InitConnectSocket()) {
        return false;
    }
    if (!ConnectServer()) {
        CloseSocket();
        return false;
    }
    return true;
}

std::unique_ptr<IConnection> CreateConnection(const Settings& settings) {
#ifdef _WIN32
    return std::make_unique<WindowsConnection>(settings);
#else
    // Позже: LinuxConnection и т.д.
    return nullptr;
#endif
}
#endif
