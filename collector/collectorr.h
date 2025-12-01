// collectorr.h

// CPU (комплекснмя, где-то вероятно придётся использтвать sleep(wait))
// RAM
// OS (version, hostname, time+timezone)
// HDD+SSD (free + total)
// Здесь по 1 функции на каждую модель данных, каждая собирает своё и возвращает
// свой тип данных
#include <VersionHelpers.h>
#include <windows.h>
#include <winioctl.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

#include "../models/windows_simple_message.h"
#include "commands_coll.h"

using namespace std;

vector<Disk> GetDisks();
RAM GetRam();
CPU GetCpu();
OS GetOs();

vector<Disk> GetDisks() {
    vector<Disk> disks;
    for (int i = 0;; ++i) {
        char path[64];
        snprintf(path, sizeof(path), "\\\\.\\PhysicalDrive%d", i);
        HANDLE hDevice =
            CreateFileA(path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                        OPEN_EXISTING, 0, nullptr);
        if (hDevice == INVALID_HANDLE_VALUE) {
            if (GetLastError() == ERROR_FILE_NOT_FOUND) {
                break;  // больше дисков нет
            }
            continue;  // пропустить недоступные
        }
        CloseHandle(hDevice);
        disks.push_back(FillDiskInfo(i));
    }
    return disks;
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

    // Первый вызов GetCPUUsage() инициализирует состояние
    double usage1 = GetCPUUsage();
    // Ждём немного (например, 1000 мс - 1 сек), чтобы получить точную загрузку
    Sleep(1000);
    cpu.usage = GetCPUUsage();
    return cpu;
}

OS GetOs() {
    OS osys = {};

    char name[256];
    DWORD size = sizeof(name);
    if (GetComputerNameA(name, &size)) {
        osys.hostname = string(name, size);
    }

    osys.version = string(GetOsVersionName());

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
    return osys;
}