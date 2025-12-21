// IConnection.h

#ifndef ICONNECTION_H
#define ICONNECTION_H

class IConnection {
   protected:
    // Конструктор интерфейса — protected, так как напрямую IConnection
    // создавать нельзя
    IConnection(const Settings& settings) : settings_(settings) {}
    virtual ~IConnection() = default;  // виртуальный деструктор (важно!)

    virtual bool IsConnected() const = 0;
    virtual void InitSocketLibrary() = 0;
    virtual bool Connect() = 0;
    virtual bool Disconnect() = 0;
    virtual void Send(const char* data, size_t size) = 0;
    virtual std::vector<char> Recv(size_t maxBytes = 4096) = 0;

   protected:
    const Settings& settings_;  // или Settings settings_, если нужно копировать
};

#endif