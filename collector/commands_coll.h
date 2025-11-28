// commands_coll.h

#include <VersionHelpers.h>
#include <windows.h>
#include <winioctl.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

#include "../transport/nlohmann/json.hpp"  // или "#include <nlohmann/json.hpp>"

using namespace std;

Disk FillDiskInfo(int diskIndex);
string GetOsVersionName();
// Вспомогательная: определить тип диска (HDD/SSD)
bool IsHDD(int diskIndex);
// Вспомогательная: получить общий размер физического диска
ULONGLONG GetPhysicalDiskSize(int diskIndex);
string GetOsVersionName();
double GetCPUUsage();

bool IsHDD(int diskIndex);  // прототип

Disk FillDiskInfo(int diskIndex) {
    Disk disk = {};
    disk.is_hdd = IsHDD(diskIndex);
    disk.total_mb =
        static_cast<double>(GetPhysicalDiskSize(diskIndex)) / (1024.0 * 1024.0);

    ULONGLONG totalFree = 0;
    unordered_set<string> processed;
    char volumeName[MAX_PATH];

    HANDLE hFind = FindFirstVolumeA(volumeName, MAX_PATH);
    if (hFind == INVALID_HANDLE_VALUE) {
        disk.free_mb = 0.0;
        return disk;
    }

    do {
        if (processed.count(volumeName)) continue;
        processed.insert(volumeName);

        // КРИТИЧЕСКИ ВАЖНО: добавить завершающий '\'
        size_t len = strlen(volumeName);
        if (len > 0 && len < MAX_PATH - 1 && volumeName[len - 1] != '\\') {
            volumeName[len] = '\\';
            volumeName[len + 1] = '\0';
        }

        HANDLE hVol =
            CreateFileA(volumeName, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        nullptr, OPEN_EXISTING, 0, nullptr);
        if (hVol == INVALID_HANDLE_VALUE) {
            continue;
        }

        // Получаем привязку к физическим дискам
        DWORD bytesReturned = 0;
        if (!DeviceIoControl(hVol, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                             nullptr, 0, nullptr, 0, &bytesReturned, nullptr)) {
            if (GetLastError() != ERROR_MORE_DATA) {
                CloseHandle(hVol);
                continue;
            }
        }

        PVOLUME_DISK_EXTENTS extents =
            static_cast<PVOLUME_DISK_EXTENTS>(malloc(bytesReturned));
        if (!extents) {
            CloseHandle(hVol);
            continue;
        }

        bool success =
            DeviceIoControl(hVol, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, nullptr,
                            0, extents, bytesReturned, &bytesReturned, nullptr);

        bool belongs = false;
        if (success && extents->NumberOfDiskExtents > 0) {
            for (DWORD i = 0; i < extents->NumberOfDiskExtents; ++i) {
                if (static_cast<int>(extents->Extents[i].DiskNumber) ==
                    diskIndex) {
                    belongs = true;
                    break;
                }
            }
        }
        free(extents);
        CloseHandle(hVol);

        if (!belongs) continue;

        // Получаем точки монтирования (C:\, D:\, или другие пути)
        char mountPaths[MAX_PATH * 4] = {};
        DWORD charCount = 0;
        if (GetVolumePathNamesForVolumeNameA(volumeName, mountPaths,
                                             sizeof(mountPaths), &charCount)) {
            char* p = mountPaths;
            while (*p) {
                ULARGE_INTEGER freeBytes = {};
                if (GetDiskFreeSpaceExA(p, &freeBytes, nullptr, nullptr)) {
                    totalFree += freeBytes.QuadPart;
                }
                p += strlen(p) + 1;
            }
        }
    } while (FindNextVolumeA(hFind, volumeName, MAX_PATH));

    ULARGE_INTEGER freeBytes = {};
    FindVolumeClose(hFind);

    // Fallback: если ничего не найдено, но это системный диск
    if (totalFree == 0 && diskIndex == 0) {
        ULARGE_INTEGER freeBytes = {};
        if (GetDiskFreeSpaceExA("C:\\", &freeBytes, nullptr, nullptr)) {
            totalFree = freeBytes.QuadPart;
        }
    }

    disk.free_mb = static_cast<double>(totalFree) / (1024.0 * 1024.0);
    return disk;
}

bool IsHDD(int diskIndex) {
    char path[64];
    snprintf(path, sizeof(path), "\\\\.\\PhysicalDrive%d", diskIndex);
    HANDLE hDevice = CreateFileA(path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 nullptr, OPEN_EXISTING, 0, nullptr);
    if (hDevice == INVALID_HANDLE_VALUE) {
        return false;
    }

    STORAGE_PROPERTY_QUERY query = {};
    query.PropertyId = StorageDeviceProperty;
    query.QueryType = PropertyStandardQuery;

    BYTE buffer[1024] = {};
    DWORD bytesReturned = 0;
    bool success = DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY,
                                   &query, sizeof(query), buffer,
                                   sizeof(buffer), &bytesReturned, nullptr);
    CloseHandle(hDevice);

    // STORAGE_BUS_TYPE находится по смещению 12
    if (bytesReturned >= 16) {
        STORAGE_BUS_TYPE busType =
            *reinterpret_cast<STORAGE_BUS_TYPE*>(buffer + 12);
        if (busType == BusTypeNvme || busType == BusTypeUsb) {
            return false;  // точно SSD или внешний — не HDD
        }
    }

    ULONG rotationRate = *reinterpret_cast<ULONG*>(buffer + 40);
    return (rotationRate != 0);  // true = HDD, false = SSD
}

ULONGLONG GetPhysicalDiskSize(int diskIndex) {
    char path[64];
    snprintf(path, sizeof(path), "\\\\.\\PhysicalDrive%d", diskIndex);

    HANDLE hDevice = CreateFileA(
        path,
        0,  // GENERIC_READ не обязателен для IOCTL_DISK_GET_DRIVE_GEOMETRY_EX
        FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);

    if (hDevice == INVALID_HANDLE_VALUE) {
        return 0;
    }

    DISK_GEOMETRY_EX geometry = {};
    DWORD bytesReturned = 0;

    bool success =
        DeviceIoControl(hDevice, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, nullptr, 0,
                        &geometry, sizeof(geometry), &bytesReturned, nullptr);

    CloseHandle(hDevice);

    if (!success) {
        return 0;
    }

    return geometry.DiskSize.QuadPart;
}

string GetOsVersionName() {
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (hNtdll) {
        typedef long(WINAPI * RtlGetVersionPtr)(OSVERSIONINFOEXW*);
        auto RtlGetVersion =
            (RtlGetVersionPtr)GetProcAddress(hNtdll, "RtlGetVersion");
        if (RtlGetVersion) {
            OSVERSIONINFOEXW osvi = {};
            osvi.dwOSVersionInfoSize = sizeof(osvi);
            if (RtlGetVersion(&osvi) == 0) {
                if (osvi.dwMajorVersion == 10 && osvi.dwMinorVersion == 0) {
                    if (osvi.dwBuildNumber >= 22000) {
                        return "Windows 11";
                    } else {
                        return "Windows 10";
                    }
                }
                if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 3) {
                    return "Windows 8.1";
                }
                if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 2) {
                    return "Windows 8";
                }
                if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 1) {
                    return "Windows 7";
                }
                if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0) {
                    return "Windows Vista";
                }
                if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1) {
                    return "Windows XP";
                }
                return "Windows (old)";
            }
        }
    }

    // Fallback на VersionHelpers (если RtlGetVersion недоступен)
    if (IsWindows10OrGreater()) return "Windows 10/11";
    if (IsWindows8Point1OrGreater()) return "Windows 8.1";
    if (IsWindows8OrGreater()) return "Windows 8";
    if (IsWindows7OrGreater()) return "Windows 7";
    if (IsWindowsVistaOrGreater()) return "Windows Vista";
    return "Windows (unknown)";
}
// Глобальные или статические переменные для хранения состояния
static ULONGLONG s_lastIdleTime = 0;
static ULONGLONG s_lastTotalTime = 0;

double GetCPUUsage() {
    FILETIME idleTime, kernelTime, userTime;
    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        return -1.0;  // ошибка
    }

    // Объединяем FILETIME в 64-битные значения (в 100-нс интервалах)
    ULARGE_INTEGER idle, kernel, user;
    idle.QuadPart = (static_cast<ULONGLONG>(idleTime.dwHighDateTime) << 32) |
                    idleTime.dwLowDateTime;
    kernel.QuadPart =
        (static_cast<ULONGLONG>(kernelTime.dwHighDateTime) << 32) |
        kernelTime.dwLowDateTime;
    user.QuadPart = (static_cast<ULONGLONG>(userTime.dwHighDateTime) << 32) |
                    userTime.dwLowDateTime;

    ULONGLONG total = kernel.QuadPart + user.QuadPart;  // kernel включает idle

    // Пропускаем первый вызов (нет предыдущих данных)
    static bool firstCall = true;
    if (firstCall) {
        firstCall = false;
        s_lastIdleTime = idle.QuadPart;
        s_lastTotalTime = total;
        return 0.0;
    }

    // Считаем дельты
    ULONGLONG idleDelta = idle.QuadPart - s_lastIdleTime;
    ULONGLONG totalDelta = total - s_lastTotalTime;

    // Обновляем состояние
    s_lastIdleTime = idle.QuadPart;
    s_lastTotalTime = total;

    // Избегаем деления на ноль
    if (totalDelta == 0) return 0.0;

    // Загрузка = 1 - (idle / total)
    double usage =
        1.0 - static_cast<double>(idleDelta) / static_cast<double>(totalDelta);

    // Ограничиваем диапазон [0.0, 1.0]
    if (usage < 0.0) usage = 0.0;
    if (usage > 1.0) usage = 1.0;

    return usage;  // или (usage1 + usage2) / 2 для сглаживания
}
