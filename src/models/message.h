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
    double usege;
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
struct Ping {
    DWORD ping_millisec;
};

enum class MessageType {
    SIMPLE_MESSAGE,
};

struct Header {
    uint32_t agent_id;  // пока рандом или 1-3
    std::string type;   // SimplePCReport, whoami, error
};
struct SimplePCReport {
    std::string error_text;
    std::vector<Disk> disks;
    std::vector<USB> usb;
    RAM ram;
    CPU cpu;
    OS system;
    Hardware hardware;
    Ping ping_m;
};
struct Message {
    Header header;
    SimplePCReport payload;
    // auto* message; // сам должен определить тип данных

    // Метод сериализации
    std::string toJson() const;
};

#endif