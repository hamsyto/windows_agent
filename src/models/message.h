// message.h
#ifndef message_H
#define message_H

#include <windows.h>

#include <cstdint>
#include <string>
#include <vector>

struct Disk {
  uint8_t number;  // номер физического диска
  std::string mount;
  std::string type;  // Тип шины
  double total;      // Megabytes
  double used;       // Megabytes
};

struct USB {
  double usage;
  double total;
};

// объём оперативной памяти
struct RAM {
  double total = 0;  // Megabytes
  double used = 0;   // Megabytes
};

struct CPU {
  int cores;     // Потоки
  double usage;  // Использование 0 - 1
};

struct OS {
  std::string hostname;
  std::string domain;  // WORKGROUP
  std::string version;
  int timestamp;  // время в формате timestamp
};

struct Hardware {
  std::string bios;
  std::string cpu;
  std::vector<std::string> mac;
  std::vector<std::string> video;  // видеоадаптер
};

enum class MessageType {
  SimplePCReport,
  WhoAmI,
  Error,
};

struct Header {
  int agent_id;      // пока рандом или 1-3
  std::string type;  // SimplePCReport, whoami, error
};
struct Payload {
  std::string error_text;
  std::vector<Disk> disks;
  std::vector<USB> usb;
  RAM ram;
  CPU cpu;
  OS system;
  Hardware hardware;
  int ping;
};
struct Message {
  Header header;
  Payload payload;
  // auto* message; // сам должен определить тип данных

  // Метод сериализации
  std::string toJson(int intend = -1) const;
};

#endif