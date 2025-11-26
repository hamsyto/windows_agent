// collector.cpp
// CPU (комплекснмя, где-то вероятно придётся использтвать sleep(wait))
// RAM
// OS (version, hostname, time+timezone)
// HDD+SSD (free + total)
// Здесь по 1 функции на каждую модель данных, каждая собирает своё и возвращает
// свой тип данных
#include <Versionhelpers.h>
#include <windows.h>

#include <cstdint>
#include <iostream>
#include <vector>

#include "windows_simple_message.h"
using namespace std;

vector<Disk> GetDisks();
RAM GetRam();  // готово
CPU GetCpu();
OS GetOs();  // готово

// мрак просто
Disk GetDisk() {
    Disk disk = {};
    // список дисков, проитерироваться
    // 1ф - список дисков, 2ф - собирает инфу
    DWORD drives =
        GetLogicalDrives();  // битовая маска: A=бит 0, B=бит 1, ..., Z=бит 25
    // Пропускаем нефиксированные диски (CD-ROM, сетевые, RAM-диски и т.д.)
    if (GetDriveTypeA(root.c_str()) != DRIVE_FIXED)
        ;  // continue;

    ULARGE_INTEGER total = {};  // Всего
    ULARGE_INTEGER free = {};   // Физически свободн
    if (GetDiskFreeSpaceExA(root.c_str(), nullptr, &total, &free)) {
        disks.total_mb =
            static_cast<double>(total.QuadPart) / (1024.0 * 1024.0);
        disks.free_mb = static_cast<double>(free.QuadPart) / (1024.0 * 1024.0);
    }
    return disk;

    /*

// Обрабатывает ОДИН диск по его букве (например, 'C')
Disk GetDisk(char driveLetter) {
    Disk disk = {}; // все поля = 0 / false

    // Формируем корневой путь: "C:\\"
    std::string root(1, driveLetter);
    root += ":\\";

    // Проверяем, что это фиксированный диск (HDD/SSD)
    if (GetDriveTypeA(root.c_str()) != DRIVE_FIXED) {
        return disk; // возвращаем {0, 0, false}
    }

    ULARGE_INTEGER total = {};
    ULARGE_INTEGER free = {};

    if (GetDiskFreeSpaceExA(root.c_str(), nullptr, &total, &free)) {
        disk.total_mb = static_cast<double>(total.QuadPart) / (1024.0 * 1024.0);
        disk.free_mb  = static_cast<double>(free.QuadPart)  / (1024.0 * 1024.0);
        disk.is_hdd   = true; // ← заглушка; замени позже на реальную проверку
    }

    return disk;
}


    */
}

RAM GetRam() {
    RAM mem_MB = {};

    ULONGLONG mem_KB = 0;
    if (GetPhysicallyInstalledSystemMemory(&mem_KB)) {
        mem_MB.total_mb = mem_KB / 1024;
    }
    MEMORYSTATUSEX mem_byte = {};
    // установка версии структуры для обратной совместимости
    mem_byte.dwLength = sizeof(mem_byte);
    if (GlobalMemoryStatusEx(&mem_byte)) {
        mem_MB.free_mb = mem_byte.ullAvailPhys / (1024 * 1024);
    }

    return mem_MB;
}

CPU GetCpu() {
    CPU cpu = {};

    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);                   // Получаем информацию о системе
    cpu.cores = sys_info.dwNumberOfProcessors;  // Извлекаем количество
                                                // логических ядер (потоков)

    // Глобальные или статические переменные для хранения состояния
    static ULONGLONG s_lastIdleTime = 0;
    static ULONGLONG s_lastTotalTime = 0;

    cpu.usage;
}

OS GetOs() {
    OS osys = {};

    const int namelen = 255;
    char name[namelen + 1];  // + 1 на '\0'
    if (gethostname(name, namelen) == 0) {
        osys.hostname = name;
    }

    osys.version = GetOsVersionName();

    FILETIME time;
    GetSystemTimeAsFileTime(&time);
    // Объединяем два DWORD в 64-битное целое (в 100-нс интервалах с 1601 года)
    ULARGE_INTEGER ull;
    ull.LowPart = time.dwLowDateTime;
    ull.HighPart = time.dwHighDateTime;
    // Конвертируем в секунды с 1970 года
    const ULONGLONG UNIX_EPOCH_DIFF =
        116444736000000000ULL;  // 100-нс между 1601 и 1970
    const ULONGLONG HUNDRED_NS_PER_SECOND = 10000000ULL;
    ULONGLONG unixTime =
        (ull.QuadPart - UNIX_EPOCH_DIFF) / HUNDRED_NS_PER_SECOND;
    osys.timestamp = static_cast<int>(unixTime);

    /* избыточно
    SYSTEMTIME st_utc, st_local;
    if (FileTimeToSystemTime(&time, &st_utc)) {}*/
}

const char* GetOsVersionName() {
    if (IsWindows8Point1OrGreater()) {
        return "8.1";
    }
    if (IsWindows8OrGreater()) {
        return "8";
    }
    if (IsWindows7OrGreater()) {
        return "7";
    }
    if (IsWindowsVistaOrGreater()) {
        return "Vista";
    }
    if (IsWindowsXPOrGreater()) {
        return "XP";
    }
    return "Unknown";
}