#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <mutex>
#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>  // htonl
#endif
#include <string>
#include <thread>

using namespace std;

#pragma comment(lib, "ws2_32.lib")

array<char, 4> int32_to_network_bytes(int32_t value);

// Конфигурация (можно вынести в consts.h)
constexpr const char* kIpAddr = "127.0.0.1";
constexpr int kPort = 8101;
constexpr int kQueue = 3;

std::atomic<int> g_reportCounter{0};
std::mutex g_fileMutex;  // для потокобезопасного счёта

// Генерация имени файла: report_001.json, report_002.json, ...
std::string GenerateFilename() {
  int id = ++g_reportCounter;
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "report_%03d.json", id);
  return std::string(buffer);
}

// Сохранение JSON в файл
bool SaveJsonToFile(const std::string& jsonStr) {
  std::lock_guard<std::mutex> lock(g_fileMutex);
  std::string filename = GenerateFilename();

  std::ofstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Failed to create file: " << filename << std::endl;
    return false;
  }
  file << jsonStr;
  file.close();
  std::cout << "Saved: " << filename << std::endl;
  return true;
}

// Обработка одного клиента
void HandleClient(SOCKET clientSocket) {
  while (true) {
    // 1. Читаем 4 байта — длина JSON (в network byte order)
    uint32_t lenNetwork = 0;
    int bytes = recv(clientSocket, reinterpret_cast<char*>(&lenNetwork),
                     sizeof(lenNetwork), 0);
    if (bytes <= 0) {
      std::cout << "Client disconnected." << std::endl;
      break;
    }
    if (bytes != sizeof(lenNetwork)) {
      std::cerr << "Invalid message length header." << std::endl;
      break;
    }

    // 2. Преобразуем длину в host byte order
    uint32_t len = ntohl(lenNetwork);
    if (len == 0 || len > 1024 * 1024) {  // защита от переполнения
      std::cerr << "Invalid JSON length: " << len << std::endl;
      break;
    }

    // 3. Читаем сам JSON
    std::string jsonStr(len, '\0');
    int totalRecv = 0;
    while (totalRecv < static_cast<int>(len)) {
      int result = recv(clientSocket, &jsonStr[totalRecv], len - totalRecv, 0);
      if (result <= 0) {
        std::cerr << "Connection broken during JSON receive." << std::endl;
        closesocket(clientSocket);
        return;
      }
      totalRecv += result;
    }

    // 4. Сохраняем в файл
    (SaveJsonToFile(jsonStr));
  }
  closesocket(clientSocket);
}

int main() {
  // Инициализация Winsock
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
    std::cerr << "WSAStartup failed." << std::endl;
    return 1;
  }

  // Создание сокета
  SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (listenSocket == INVALID_SOCKET) {
    std::cerr << "socket() failed." << std::endl;
    WSACleanup();
    return 1;
  }

  // Привязка к адресу
  sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(kPort);
  addr.sin_addr.s_addr = inet_addr(kIpAddr);

  if (bind(listenSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) ==
      SOCKET_ERROR) {
    std::cerr << "bind() failed." << std::endl;
    closesocket(listenSocket);
    WSACleanup();
    return 1;
  }

  // Ожидание подключений
  if (listen(listenSocket, kQueue) == SOCKET_ERROR) {
    std::cerr << "listen() failed." << std::endl;
    closesocket(listenSocket);
    WSACleanup();
    return 1;
  }

  std::cout << "Server listening on " << kIpAddr << ":" << kPort << std::endl;
  int x = 0;
  // Главный цикл приёма подключений
  while (true) {
    SOCKET clientSocket = accept(listenSocket, nullptr, nullptr);
    if (clientSocket == INVALID_SOCKET) {
      std::cerr << "accept() failed." << std::endl;
      continue;
    }

    std::cout << "Client connected." << std::endl;
    if (x == 0) {
      auto buffer = int32_to_network_bytes(10);
      send(clientSocket, buffer.data(), buffer.size(), 0);
      x = 1;

    }  // Обработка клиента в отдельном потоке (упрощённо — без управления
    // потоками)
    thread(HandleClient, clientSocket).detach();
  }

  // Очистка (никогда не достигается в этом цикле)
  closesocket(listenSocket);
  WSACleanup();
  return 0;
}

array<char, 4> int32_to_network_bytes(int32_t value) {
  uint32_t net_val = htonl(static_cast<uint32_t>(value));
  std::array<char, 4> buffer;
  memcpy(buffer.data(), &net_val, sizeof(net_val));
  return buffer;
}