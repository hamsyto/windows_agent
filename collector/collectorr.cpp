// collectorr.cpp

// CPU (комплекснмя, где-то вероятно придётся использтвать sleep(wait))
// RAM
// OS (version, hostname, time+timezone)
// HDD+SSD (free + total)
// Здесь по 1 функции на каждую модель данных, каждая собирает своё и возвращает
// свой тип данных

#include <winsock2.h>
// порядок подключения
#include <windows.h>
//
#include <VersionHelpers.h>
#include <winioctl.h>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

#include "collectorr.h"
#include "commands_coll.h"

using namespace std;

vector<Disk> GetDisks() {
    vector<Disk> disks;

    for (int diskIndex = 0; diskIndex < 16; ++diskIndex) {
        uint64_t size = GetPhysicalDiskSize(diskIndex);
        if (size == 0) continue;  // диск отсутствует или недоступен

        // теперь по томам ходим

        // получаем битовую маску логических дисков
        DWORD drives = GetLogicalDrives();
        string root;

        for (char letter = 'A'; letter <= 'Z'; ++letter) {
            if (!(drives & (1U << (letter - 'A')))) continue;

            root = string(1, letter) + ":\\";
            if (GetDriveTypeA(root.c_str()) != DRIVE_FIXED) continue;

            int diskIndex = GetPhysicalDiskIndexForDriveLetter(letter);

            auto info = FillDiskInfo(diskIndex, root);
            disks.push_back(info);
        }
    }
    return disks;
}

RAM GetRam() {
    RAM mem_MB = {};
    MEMORYSTATUSEX m{};
    m.dwLength = sizeof(m);
    ULONGLONG installedKB = 0;
    if (GetPhysicallyInstalledSystemMemory(&installedKB)) {
        mem_MB.total = installedKB * 1024;
    }
    if (GlobalMemoryStatusEx(&m)) {
        if (mem_MB.total == 0) {
            mem_MB.total = m.ullTotalPhys;
        }
        mem_MB.used = mem_MB.total - m.ullAvailPhys;

        mem_MB.total = round(mem_MB.total / (1024 * 1024) * 100.0) / 100.0;
        mem_MB.used = round(mem_MB.used / (1024 * 1024) * 100.0) / 100.0;
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
    cpu.used = GetCPUUsage();
    cpu.used = round(cpu.used * 100.0) / 100.0;
    return cpu;
}

string get_dns_fqdn_hostname() {
    DWORD size = 0;
    // Запрос размера: size будет = длина + 1 (включая \0)
    if (!GetComputerNameExA(ComputerNameDnsFullyQualified, nullptr, &size)) {
        if (GetLastError() != ERROR_MORE_DATA) {
            return "";  // настоящая ошибка
        }
    }

    string hostname(size, '\0');  // выделяем size = N + 1 символов
    // ВАЖНО: size может измениться во втором вызове!
    if (GetComputerNameExA(ComputerNameDnsFullyQualified, &hostname[0],
                           &size)) {
        // После успешного вызова: size = длина строки БЕЗ \0
        hostname.resize(size);  // ← НЕ size - 1, а просто size
        return hostname;
    }
    return "";
}

OS GetOs() {
    OS osys = {};

    osys.hostname = get_dns_fqdn_hostname();
    osys.domain = GetComputerDomainOrWorkgroup();
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

    return osys;
}

Hardware GetHardware() {
    Hardware hardware = {};

    hardware.bios = GetBiosInfo();
    hardware.cpu = GetCpuBrand();
    hardware.mac = getMacAddresses();
    hardware.video = GetVideoAdapters();

    return hardware;
}

Ping GetPing() {
    Settings setting = {};
    Ping ping_m = {};
    if (!LoadEnvSettings(setting)) {
        ping_m.ping_millisec = 0;
        return ping_m;
    }

    DWORD avg = GetAveragePing(setting.ip_server.c_str(), 4);
    // DWORD avg = GetAveragePing("google.com", 3); // чист для проверки

    if (avg > 0) {
        ping_m.ping_millisec = avg;
        return ping_m;
    }
    ping_m.ping_millisec = 0;
    return ping_m;
}