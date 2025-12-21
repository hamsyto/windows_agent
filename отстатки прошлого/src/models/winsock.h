// winsock.h

#ifndef WINSOCK_H
#define WINSOCK_H

#include "IConnection.h"

class WinSock : public IConnection {
   public:
    WinSock(Settings& settings) {
        Settings& this.settings = settings;
    };                  // Конструктор
    void f() override;  // чисто виртуальная функция
    void LibSockInit() override;
    void Send() override;

   private:
};

#endif