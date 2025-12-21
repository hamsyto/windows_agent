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
#include <vector>

#include "../models/message.h"
#include "../models/settings.h"

std::string CompressJsonString(const std::string& jsonStr);
// Основная функция шифрования
std::string EncryptRawWithNonce(const std::string& jsonStr,
                                const std::string& key);
std::string LenPreparationMessToSend(const std::string& text_str);
// отправляет сначала длину потом само сообщение
void SendMessageAndMessageSize(SOCKET& client_socket, std::string& jsonStr,
                               Settings& settings);

#endif