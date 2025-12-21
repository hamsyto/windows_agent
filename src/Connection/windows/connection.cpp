// windowsConnection.cpp
#include "connection.h"

#include <ws2tcpip.h>

#include <iostream>
#include <stdexcept>

using namespace std;

WindowsConnection::WindowsConnection(const Settings& settings) {}
WindowsConnection::~WindowsConnection() { Disconnect(); }

bool WindowsConnection::Connect() {
  socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (socket_ == INVALID_SOCKET) {
    cout << "socket() failed with error: " << WSAGetLastError() << endl;
    return false;
  }

  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  // преобразует 16-битное число в сетевой порядок
  addr.sin_port = htons(settings_.port_server);
  addr.sin_addr.s_addr = inet_addr(settings_.ip_server.c_str());

  if (connect(socket_, reinterpret_cast<struct sockaddr*>(&addr),
              sizeof(addr)) != 0) {
    cout << "connect() failed with error: " << WSAGetLastError() << endl;
    return false;
  }
  cout << "connect to server :) " << endl;
  return true;
}

bool WindowsConnection::Disconnect() {
  if ((closesocket(socket_)) == SOCKET_ERROR) {
    cout << "closesocket failed with error: " << WSAGetLastError() << endl;
    return false;
  }
  socket_ = INVALID_SOCKET;
  cout << "disconnected from the server" << endl;
  Sleep(settings_.idle_time * 1000);
  return true;
}

// передаётся полностью заполненый json (id и тп)
void WindowsConnection::Send(const string& data) {
  if (socket_ == INVALID_SOCKET) {
    throw runtime_error("Connection to the server is lost");
    // return false;
  }

  uint32_t len = static_cast<uint32_t>(data.size());
  uint32_t len_network = htonl(len);

  if (data == "" || len == 0) {
    return;
  }
  vector<char> message(data.begin(), data.end());

  // Отправляем заголовок (4 байта)
  int result = send(socket_, reinterpret_cast<const char*>(&len_network), 4, 0);
  if (result == SOCKET_ERROR) {
    // В Windows: WSAGetLastError()
    throw runtime_error("Failed to send data: socket error");
  }
  result = 0;
  // Отправляем тело (nonce + cipher + tag)
  result = send(socket_, message.data(), message.size(), 0);
  if (result == SOCKET_ERROR) {
    throw runtime_error("Failed to send data: socket error");
  }
  // НИКАКИХ mess_char.push_back('\0')!
}

vector<char> WindowsConnection::Recv(size_t maxBytes) {
  if (socket_ == INVALID_SOCKET) {
    throw std::runtime_error("Socket not connected");
  }
  if (maxBytes == 0) {
    maxBytes = 4096;
  }

  std::vector<char> buffer(maxBytes);
  int received = recv(socket_, buffer.data(), static_cast<int>(maxBytes), 0);

  if (received == SOCKET_ERROR) {
    std::cerr << "recv() failed: " << WSAGetLastError() << std::endl;
    throw std::runtime_error("Failed to receive data");
  }

  // Обрезаем вектор до реально полученного размера
  buffer.resize(static_cast<size_t>(received));
  return buffer;
}

// void WindowsConnection::Send(const char* data, size_t size) {
//     if (socket_ == INVALID_SOCKET) {
//         throw std::runtime_error("Socket not connected");
//     }
//     if (data == nullptr || size == 0) {
//         return;
//     }
//
//     int totalSent = 0;
//     while (totalSent < static_cast<int>(size)) {
//         int sent = send(socket_, data + totalSent,
//                         static_cast<int>(size - totalSent), 0);
//         if (sent == SOCKET_ERROR) {
//             std::cerr << "send() failed: " << WSAGetLastError() <<
//             std::endl; throw std::runtime_error("Failed to send data");
//         }
//         totalSent += sent;
//     }
// }
