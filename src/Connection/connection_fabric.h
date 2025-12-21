// connection_fabric.h
#ifndef CONNECTION_FABRIC_H
#define CONNECTION_FABRIC_H

#include <memory>

#include "IConnection.h"
#include "Windows/windowsConnection.h"

std::unique_ptr<IConnection> CreateConnection(const Settings& settings) {
#ifdef _WIN32
    return std::make_unique<WindowsConnection>(settings);
#else
    // Позже: LinuxConnection и т.д.
    return nullptr;
#endif
}

#endif