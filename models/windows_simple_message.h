#ifndef WINDOWS_SIMPLE_MESSAGE_H
#define WINDOWS_SIMPLE_MESSAGE_H

#include <windows.h>

#include <cstdint>
#include <string>
#include <vector>

struct Disk {
    double free_mb;   // Megabytes
    double total_mb;  // Megabytes
    bool is_hdd;      // Optional
    bool fail_fill = false;
};

// объём оперативной памяти
struct RAM {
    double free_mb;   // Megabytes
    double total_mb;  // Megabytes
    bool fail_fill = false;
};

struct CPU {
    int cores;     // Потоки
    double usage;  // Использование 0 - 1
    bool fail_fill = false;
};

struct OS {
    std::string hostname;
    std::string domain;  // WORKGROUP
    std::string version;
    int timestamp;  // время в формате timestamp
    bool fail_fill = false;
};

struct Hardware {
    std::string bios;
    std::string cpu;
    std::vector<std::string> mac;
    std::vector<std::string> video;  // видеоадаптер
    bool fail_fill = false;
};
struct Ping {
    DWORD ping_millisec;
    bool fail_fill = false;
};

enum class MessageType {
    SIMPLE_MESSAGE,
};

struct Message {
    struct Header {
        unsigned long agent_id;  // пока рандом или 1-3
        std::string type;        // SimplePCReport, whoami, error, IAmNotOK,
    };

    struct SimplePCReport {
        std::string error_text;
        std::vector<Disk> disks;
        RAM ram;
        CPU cpu;
        OS system;
        Hardware hardware;
        Ping ping_m;
    };

    Header header;
    SimplePCReport payload;
    // auto* message; // сам должен определить тип данных

    // Метод сериализации
    std::string toJson() const;
};

struct Settings {
    int idle_time;          // как часто отправлять данные
    std::string ip_server;  // ip или dns адрес сервера куда отправлять
    int port_server;
    unsigned int agentID = 0;
};

#endif