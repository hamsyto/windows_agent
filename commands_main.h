// commands_main.h

#ifndef COMMANDS_MAIN_H
#define COMMANDS_MAIN_H

#include <atomic>
#include <string>
#include "transport/transport.h"

int SendMessages();
void Registration(SOCKET& connect_socket, Message& msg,
                  const Settings& setting);
int StartConnection(SOCKET& connect_socket, const Settings& setting);
void SendData(SOCKET& connect_socket, Message& msg, const Settings& setting);
// инициализирует winsock
int WinsockInit();
SOCKET InitConnectSocket();
void ConnectServer(SOCKET connect_socket);

void RecvMessage(SOCKET connect_socket, int32_t& id,
                std::atomic<bool>& disconnected);

// закрывает сокет с проверкой на закрытие
void CloseSocketCheck(SOCKET& sck);

int64_t network_buffer_to_int32(char* buffer);

bool write_int_to_file(const char* filename, int32_t value);
bool ensure_file_has_int(const char* filename, int32_t expected_value);
unsigned long ReadOrCreateCounterFile(const std::string& filename);

#endif