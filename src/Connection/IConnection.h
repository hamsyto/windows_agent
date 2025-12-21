// IConnection.h

#ifndef ICONNECTION_H
#define ICONNECTION_H

#include <memory>
#include <string>
#include <vector>

#include "../models/settings.h"

// Чисто абстрактный интерфейс — только = 0 методы
class IConnection {
   protected:
    virtual bool InitSocketLibrary() = 0;

   public:
    virtual ~IConnection() = default;  // виртуальный деструктор (важно!)

    virtual bool Connect() = 0;
    virtual bool Disconnect() = 0;
    virtual void Send(const std::string& data) = 0;
    virtual std::vector<char> Recv(size_t maxBytes = 4096) = 0;
};

std::unique_ptr<IConnection> CreateConnection(const Settings& settings);

#endif
