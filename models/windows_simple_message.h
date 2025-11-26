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
        CPU cpu;
        RAM ram;
        OS system;
        vector<Disk> disks;
    };

    Header header;
    // auto* message; // сам должен определить тип данных
};

struct Settings {
    int idle_time;  // как часто отправлять данные
    string server;  // ip или dns адрес сервера куда отправлять
};