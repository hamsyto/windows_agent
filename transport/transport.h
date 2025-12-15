// transport.h

// Функция 1 которая получает на вход сообщение и сериализует его для отправки
// Функция 2, которая получает на вход сокет и сообщение, сереализует с помощью
// функции 1 и отправляет в сокет.

#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <winsock2.h>
// порядок подключения
#include <windows.h>
// специализированные заголовки Windows
#include <VersionHelpers.h>
#include <winioctl.h>

#include <string>

#include "../collector/collectorr.h"

// сборка всех даыннх в одно сообщение
Message GetMess(Message& msg);
// отправляет сначала длину потом само сообщение
void SendMessageAndMessageSize(SOCKET& client_socket,
                               const std::string& jsonStr);

#endif