// commands_main.cpp

#include <winsock2.h>  // для WSADATA, WSAStartup, socket, bind, listen, connect, htons
// порядок подключения
#include <windows.h>

// == ==

#include <algorithm>
#include <cstdint>
// #include <filesystem>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "collector/commands_coll.h"
#include "commands_main.h"
#include "consts.h"
#include "models/connection.h"
#include "transport/transport.h"

using namespace std;

//  ConnectServer
int SendMessages() {
  if (WinsockInit() != 0) return 1;

  Settings settings = {};
  if (!LoadEnvSettings(settings)) return 1;
  cout << settings.ip_server << ":" << settings.port_server << endl;

  Connection connect_socket;

  while (true) {
    if (connect_socket.agent_id == 0) {
      connect_socket.agent_id = Registration(connect_socket, settings);
      if (connect_socket.agent_id == 0) continue;
      cout << "ID: " << connect_socket.agent_id << " saved\n";
    } else {
      SendData(connect_socket, settings);
    }
  }
  return 1;
}

// TODO: Создать файл и записать (переделать возврат функции на INT (agent_id))
int32_t Registration(Connection& connect_socket, Settings& settings) {
  SendData(connect_socket, settings);
  if (!connect_socket.connected) return 0;

  uint32_t id = RecvMessage(connect_socket);
  if (connect_socket.connected == false) return 0;

  try {
    if (connect_socket.SaveAgentID(id)) {
      return id;
    }
  } catch (const exception& e) {
    string error = e.what();
    SendTypeMsgError(connect_socket, error);
  }
  return 0;
}

void SendData(Connection& connect_socket, Settings& settings) {
  connect_socket.Connect(settings);
  if (!connect_socket.connected) return;

  Message msg = {};
  msg.header.agent_id = connect_socket.agent_id;

  string error = "";
  try {
    msg = GetMess(msg);
  } catch (const exception& e) {
    error = e.what();
  }
  string json_msg = msg.toJson();
  if (!connect_socket.connected)
    return;  // проверка на отключение перед отправкой

  if (error == "") {
    SendMessageAndMessageSize(connect_socket.socket, json_msg);
  } else {
    SendTypeMsgError(connect_socket, error);
  }
}

void SendTypeMsgError(Connection& connect_socket, string& error) {
  Message msg = {};
  msg.header.agent_id = connect_socket.agent_id;
  msg.header.type = "error";
  msg.payload.error_text = error;
  string json_error = msg.toJson();
  SendMessageAndMessageSize(connect_socket.socket, json_error);
}

int WinsockInit() {
  // структура, которая заполняется в WSAStartup
  WSADATA wsa;

  // инициализирует winsock, (указание версии, указатель на структуру которая
  // будет заполнена инфой о реализации Winsock Без вызова WSAStartup любые
  // сетевые функции (например, socket, bind, connect) не будут работать
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {  // возвр 0 - успех
    cout << "WSAStartup failed" << endl;
    WSACleanup();
    return 1;
  }
  return 0;
}

SOCKET InitConnectSocket() {
  // IPv4, тип сокета - потоковый, протокол - TCP
  SOCKET connect_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (connect_socket == INVALID_SOCKET) {
    cout << "socket() failed with error: " << WSAGetLastError() << endl;
    return INVALID_SOCKET;
  }
  return connect_socket;
}

int ConnectServer(SOCKET& socket, Settings& settings) {
  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  // преобразует 16-битное число в сетевой порядок
  addr.sin_port = htons(settings.port_server);
  addr.sin_addr.s_addr = inet_addr(settings.ip_server.c_str());

  if (connect(socket, reinterpret_cast<struct sockaddr*>(&addr),
              sizeof(addr)) != 0) {
    cout << "connect() failed with error: " << WSAGetLastError() << endl;
    return -1;
  }
  cout << "connect to server :) " << endl;
  return 1;
}

void CloseSocketCheck(SOCKET& sck) {
  if ((closesocket(sck)) == SOCKET_ERROR) {
    cout << "closesocket failed with error: " << WSAGetLastError() << endl;
  }
  sck = INVALID_SOCKET;
}

uint32_t RecvMessage(Connection& connect_socket) {
  char buf[kMaxBuf] = {};
  uint32_t bytes_received = recv(connect_socket.socket, buf, kMaxBuf, 0);
  if (bytes_received < 4) {  // минимум 4 байта для uint32
    cout << "Connection closed or incomplete data." << endl;
    connect_socket.connected = false;
    return 0;
  }
  uint32_t id = NetworkBufferToUint32(buf);
  id = id > 0 ? id : 0;
  if (id == 0) connect_socket.connected = false;
  connect_socket.connected = true;
  return id;
}

void DeleteFileInCurrentDir(const string& fileName) {
  // std::remove возвращает 0 при успехе, ненулевое значение — при ошибке
  remove(fileName.c_str());
}

bool WriteIntToFile(const string& filename, uint32_t& id) {
  ofstream file(filename);
  if (!file) return false;
  file << id;
  return file.good();
}

uint32_t NetworkBufferToUint32(char* buffer) {
  uint64_t net_value = 0;

  memcpy(&net_value, buffer, sizeof(net_value));
  auto result = static_cast<int32_t>(ntohl(net_value));
  return result;
}

unsigned int ReadOrCreateCounterFile(const string& file_name) {
  // Попытка прочитать файл
  ifstream in_file(file_name);
  string content;
  unsigned int value = 0;

  if (!in_file.is_open()) return 0;  //  Файл не найден

  getline(in_file, content);  // читаем до первого \n (или весь файл)
  in_file.close();
  // Удаляем начальные и конечные пробельные символы
  size_t start = content.find_first_not_of(" \t\r\n");
  // файл не пустой или не только пробелы
  if (start == string::npos) return 0;

  size_t end = content.find_last_not_of(" \t\r\n");
  string trimmed = content.substr(start, end - start + 1);

  // Проверяем, что строка состоит только из цифр (и не начинается с
  // '0', если не "0")
  if (trimmed.empty()) return 0;
  if (trimmed.find_first_not_of("0123456789") != string::npos) return 0;

  // Исключаем "0" и числа с ведущими нулями (кроме самого "0", но
  // он нам не нужен)
  if (trimmed != "0" && trimmed[0] != '0') {
    // Преобразуем
    istringstream iss(trimmed);
    if (iss >> value && value > 0) {
      return value;
    }
  }

  // Во всех остальных случаях — создаём/перезаписываем файл как "0"
  ofstream outfile(file_name, ios::trunc);
  if (outfile.is_open()) {
    outfile << "0";
    outfile.close();
  }

  return value;
}
