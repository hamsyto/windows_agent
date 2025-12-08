// commands_main.h

#ifndef COMMANDS_MAIN_H
#define COMMANDS_MAIN_H

#include <winsock2.h>  // для WSADATA, WSAStartup, socket, bind, listen, connect, htons
// порядок подключения
#include <windows.h>

#include <cstdint>
#include <string>

#include "transport/transport.h"

int SendMessages();
unsigned int Registration(SOCKET& connect_socket, Settings& setting);
void SendData(SOCKET& connect_socket, Settings& setting);
int StartConnection(SOCKET& connect_socket, Settings& setting);
void SendData(SOCKET& connect_socket, Settings& setting);
// задаёт тип сообщениия error и отправляет json с текстом ошибки
void SendTypeMsgError(SOCKET& connect_socket, std::string& error,
                      Settings& settings);
// инициализирует winsock
int WinsockInit();

SOCKET InitConnectSocket();
void ConnectServer(SOCKET connect_socket);

unsigned int RecvMessage(SOCKET connect_socket);

// закрывает сокет с проверкой на закрытие
void CloseSocketCheck(SOCKET& sck);

int64_t network_buffer_to_int32(char* buffer);

bool write_int_to_file(const char* filename, int32_t value);
bool ensure_file_has_int(const char* filename, int32_t expected_value);
unsigned int ReadOrCreateCounterFile(const std::string& filename);

#endif