// transport.h
#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <winsock2.h>
// порядок подключения
#include <windows.h>
// специализированные заголовки Windows
#include <VersionHelpers.h>
#include <winioctl.h>

#include <string>

#include "../models/message.h"
#include "../models/settings.h"

// сборка всех даыннх в одно сообщение
Message GetMess(Message& msg);

std::string CompressJsonString(const std::string& jsonStr);
std::string encrypt(const std::string& jsonStr, Settings& settigns);

// отправляет сначала длину потом само сообщение
void SendMessageAndMessageSize(SOCKET& client_socket, std::string& jsonStr,
                               Settings& settings);

#endif