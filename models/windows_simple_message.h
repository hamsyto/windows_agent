#include <cstdint>
#include <string>
#include <vector>
using namespace std;

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
    string hostname;
    string domain;  // WORKGROUP
    string version;
    int timestamp;  // время в формате timestamp
};

enum class MessageType {
    SIMPLE_MESSAGE,
};

struct Message {
    struct Header {
        int agent_id;   // пока рандом или 1-3
        char type[64];  // тип сообщения, == SimplePCReport
    };

    struct SimplePCReport {
        vector<Disk> disks;
        RAM ram;
        CPU cpu;
        OS system;
    };

    Header header;
    SimplePCReport payload;
    // auto* message; // сам должен определить тип данных

    // Метод сериализации
    string toJson() const;
};

struct Settings {
    int idle_time;  // как часто отправлять данные
    string ip_server;  // ip или dns адрес сервера куда отправлять
    int port_server;
};
