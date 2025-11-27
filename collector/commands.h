// commands.h

#include <VersionHelpers.h>
#include <windows.h>
#include <winioctl.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

using namespace std;

string GetOsVersionName();
// Вспомогательная: определить тип диска (HDD/SSD)
bool IsHDD(int diskIndex);
// Вспомогательная: получить общий размер физического диска
ULONGLONG GetPhysicalDiskSize(int diskIndex);
string GetOsVersionName();
double GetCPUUsage();

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

bool IsHDD(int diskIndex) {
    char path[64];
    snprintf(path, sizeof(path), "\\\\.\\PhysicalDrive%d", diskIndex);
    HANDLE hDevice = CreateFileA(path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 nullptr, OPEN_EXISTING, 0, nullptr);
    if (hDevice == INVALID_HANDLE_VALUE) return true;

    BYTE buffer[4096] = {};
    STORAGE_PROPERTY_QUERY query = {};
    query.PropertyId = StorageDeviceProperty;
    query.QueryType = PropertyStandardQuery;
    DWORD bytesReturned = 0;
    bool isHDD = true;

    if (DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, &query,
                        sizeof(query), buffer, sizeof(buffer), &bytesReturned,
                        nullptr)) {
        if (bytesReturned > 0x20) {
            BYTE mediaType = buffer[0x20];
            if (mediaType == 4)
                isHDD = false;  // SSD
            else if (mediaType == 3)
                isHDD = true;  // HDD
        }
    }
    CloseHandle(hDevice);
    return isHDD;
}

ULONGLONG GetPhysicalDiskSize(int diskIndex) {
    char path[64];
    snprintf(path, sizeof(path), "\\\\.\\PhysicalDrive%d", diskIndex);
    HANDLE hDevice = CreateFileA(path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 nullptr, OPEN_EXISTING, 0, nullptr);
    if (hDevice == INVALID_HANDLE_VALUE) return 0;

    DISK_GEOMETRY_EX geom = {};
    DWORD bytesReturned = 0;
    ULONGLONG size = 0;
    if (DeviceIoControl(hDevice, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, nullptr, 0,
                        &geom, sizeof(geom), &bytesReturned, nullptr)) {
        size = geom.DiskSize.QuadPart;
    }
    CloseHandle(hDevice);
    return size;
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
