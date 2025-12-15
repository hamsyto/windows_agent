#include <winsock2.h>
// порядок подключения
#include <windows.h>
// специализированные заголовки Windows
#include <VersionHelpers.h>
#include <winioctl.h>

#include <cstdint>
#include <iostream>
#include <string>

#include "../consts.h"
#include "transport.h"

using namespace std;

void SendMessageAndMessageSize(SOCKET& client_socket, const string& jsonStr) {
  if ((jsonStr.size()) > kMaxBuf) {
    cout << "Message length exceeds the allowed sending length. Message "
            "cannot be sent."
         << endl;
    return;
  }

  // 1. Подготовка длины (в network byte order)
  uint32_t len = static_cast<uint32_t>(jsonStr.size());
  uint32_t lenNetwork = htonl(len);  // host → network (big-endian)

  // 2. Отправка длины
  if (send(client_socket, reinterpret_cast<const char*>(&lenNetwork),
           sizeof(lenNetwork), 0) == SOCKET_ERROR) {
    cout << "send(char_size_message) failed with error: " << WSAGetLastError()
         << endl;
    return;
  }

  if (send(client_socket, jsonStr.c_str(), (int)jsonStr.size(), 0) ==
      SOCKET_ERROR) {
    cout << "send() failed with error: " << WSAGetLastError() << endl;
    return;
  }
}
