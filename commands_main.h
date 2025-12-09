// commands_main.h

#ifndef COMMANDS_MAIN_H
#define COMMANDS_MAIN_H

#include <winsock2.h>  // для WSADATA, WSAStartup, socket, bind, listen, connect, htons
// порядок подключения
#include <windows.h>

#include <cstdint>
#include <string>

#include "transport/transport.h"
#include "models/connection.h"

int SendMessages();
int32_t Registration(Connection& connect_socket, Settings& settings);
void SendData(Connection& connect_socket, Settings& settings);
// задаёт тип сообщениия error и отправляет json с текстом ошибки
void SendTypeMsgError(Connection& connect_socket, std::string& error);

// инициализирует winsock
int WinsockInit();

SOCKET InitConnectSocket();
int ConnectServer(SOCKET& socket, Settings& settings);

uint32_t RecvMessage(Connection& connect_socket);

// закрывает сокет с проверкой на закрытие
void CloseSocketCheck(SOCKET& sck);
// если файл есть - удаляем его
void DeleteFileInCurrentDir(const std::string& fileName);

bool WriteIntToFile(const std::string& filename, uint32_t& id);

uint32_t NetworkBufferToUint32(char* buffer);
// bool ensure_file_has_int(const char* filename, int32_t expected_value);
unsigned int ReadOrCreateCounterFile(const std::string& filename);

#endif