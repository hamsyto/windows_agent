#ifndef WINDOWS_SIMPLE_MESSAGE_H
#define WINDOWS_SIMPLE_MESSAGE_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdint>
#include <string>
#include <vector>

struct Disk {
    double free_mb;   // Megabytes
    double total_mb;  // Megabytes
    bool is_hdd;      // Optional
};

// объём оперативной памяти
struct RAM {
    double free_mb;   // Megabytes
    double total_mb;  // Megabytes
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
/*
}': {'bios': 'EMPTY',
                          'cpu': 'EMPTY',
                          'mac': ['mac_0', 'mac_1', 'mac_2'],
                          'video': ['video_0', 'video_1', 'video_2']},
*/
enum class MessageType {
    SIMPLE_MESSAGE,
};

struct Message {
    struct Header {
        int agent_id;   // пока рандом или 1-3
        char type[64];  // тип сообщения, == SimplePCReport
    };

    struct SimplePCReport {
        std::vector<Disk> disks;
        RAM ram;
        CPU cpu;
        OS system;
        Hardware hardware;
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
};

#endif