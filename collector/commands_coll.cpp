// commands_coll.cpp

#define NET_API_STATUS DWORD

//== 1 == Важен порядок
#include <windows.h>
//== 2 ==
#include <winsock2.h>
//== 3 ==
#include <ws2tcpip.h>

// === Заголовки NetAPI ===
#include <lmapibuf.h>  // объявляет NetApiBufferFree
#include <lmerr.h>     // определяет NERR_Success (как DWORD)
#include <lmjoin.h>    // объявляет NetGetJoinInformation

//== 1 == Важен порядок
#include <ipexport.h>
//== 2 ==
#include <iphlpapi.h>
//== 3 ==
#include <icmpapi.h>

// === Другие заголовки ===
#include <VersionHelpers.h>
#include <Wbemidl.h>
#include <intrin.h>
#include <winioctl.h>

#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../const.h"
#include "commands_coll.h"

using namespace std;






unordered_map<string, string> ClearEnvFile() {
    ifstream file(kEnvFile);  // ищет .env файл
    if (!file.is_open()) {
        cout << "Ошибка: не удалось открыть .env\n";
        // return false;
    }
    unordered_map<string, string> env_map;
    string line;
    while (getline(file, line)) {
        // Удаляем комментарии
        size_t comment_pos = line.find('#');
        if (comment_pos != string::npos) {
            line = line.substr(0, comment_pos);
        }

        line = Trim(line);
        if (line.empty()) continue;

        size_t eq_pos = line.find('=');
        if (eq_pos == string::npos) continue;

        string key = Trim(line.substr(0, eq_pos));
        string value = Trim(line.substr(eq_pos + 1));

        // Убираем кавычки, если есть (поддержка "value" и 'value')
        if ((value.size() >= 2) &&
            ((value.front() == '"' && value.back() == '"') ||
             (value.front() == '\'' && value.back() == '\''))) {
            value = value.substr(1, value.size() - 2);
        }

        env_map[key] = value;
    }
    file.close();
    return env_map;
}

bool LoadEnvSettings(Settings& out) {
    unordered_map<string, string> env_map = ClearEnvFile();

    // Проверяем наличие всех нужных переменных
    if (env_map.find("IDLE_TIME") == env_map.end() ||
        env_map.find("IP_SERVER") == env_map.end() ||
        env_map.find("PORT_SERVER") == env_map.end()) {
        cout << "Ошибка: в .env отсутствуют обязательные переменные\n";
        return false;
    }

    try {
        out.idle_time = stoi(env_map["IDLE_TIME"]);
        out.ip_server = env_map["IP_SERVER"];
        out.port_server = stoi(env_map["PORT_SERVER"]);
    } catch (const exception& e) {
        cout << "Ошибка парсинга числа в .env: " << e.what() << endl;
        return false;
    }

    return true;
}

string Trim(const string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

Disk FillDiskInfo(int diskIndex) {
    Disk disk = {};
    disk.is_hdd = IsHDD(diskIndex);
    disk.total_mb =
        static_cast<double>(GetPhysicalDiskSize(diskIndex)) / (1024.0 * 1024.0);
    disk.total_mb = round(disk.total_mb * 100.0) / 100.0;

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
    disk.free_mb = round(disk.free_mb * 100.0) / 100.0;
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

string get_computer_domain_or_workgroup() {
    PWSTR domainName = nullptr;
    NETSETUP_JOIN_STATUS joinStatus;

    NET_API_STATUS status =
        NetGetJoinInformation(nullptr,      // локальный компьютер
                              &domainName,  // получим имя домена/рабочей группы
                              &joinStatus   // тип присоединения
        );

    string result;
    if (status == NERR_Success && domainName != nullptr) {
        // Преобразуем из UTF-16 (wchar_t*) в string (ANSI/UTF-8)
        // Используем простой способ через WideCharToMultiByte (ANSI)
        int len = WideCharToMultiByte(CP_ACP, 0, domainName, -1, nullptr, 0,
                                      nullptr, nullptr);
        if (len > 0) {
            string temp(len - 1, '\0');  // -1 чтобы исключить \0
            WideCharToMultiByte(CP_ACP, 0, domainName, -1, &temp[0], len,
                                nullptr, nullptr);
            result = move(temp);
        }
        NetApiBufferFree(domainName);  // обязательно освободить
    }

    return result;
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

vector<string> getMacAddresses() {
    vector<string> macList;

    ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST |
                  GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME;

    ULONG size = 0;
    // Узнаём нужный размер буфера
    if (GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, nullptr, &size) !=
        ERROR_BUFFER_OVERFLOW) {
        return macList;  // нет адаптеров или ошибка
    }

    vector<BYTE> buffer(size);
    PIP_ADAPTER_ADDRESSES adapters =
        reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());

    if (GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, adapters, &size) !=
        NO_ERROR) {
        return macList;
    }

    PIP_ADAPTER_ADDRESSES adapter = adapters;
    while (adapter) {
        // Пропускаем loopback и адаптеры без MAC
        if (adapter->PhysicalAddressLength == 6 &&
            adapter->IfType != IF_TYPE_SOFTWARE_LOOPBACK) {
            char mac[18];
            snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
                     adapter->PhysicalAddress[0], adapter->PhysicalAddress[1],
                     adapter->PhysicalAddress[2], adapter->PhysicalAddress[3],
                     adapter->PhysicalAddress[4], adapter->PhysicalAddress[5]);
            macList.push_back(string(mac));
        }
        adapter = adapter->Next;
    }

    return macList;
}

string bstrToUtf8(BSTR bstr) {
    if (!bstr) return "";
    int len =
        WideCharToMultiByte(CP_UTF8, 0, bstr, -1, nullptr, 0, nullptr, nullptr);
    if (len <= 1) return "";
    string result(len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, bstr, -1, &result[0], len, nullptr,
                        nullptr);
    return result;
}

string getBiosInfo() {
    HRESULT hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) return "EMPTY";

    hres = CoInitializeSecurity(
        nullptr, -1, nullptr, nullptr, RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE, nullptr);
    if (FAILED(hres)) {
        CoUninitialize();
        return "EMPTY";
    }

    IWbemLocator* pLoc = nullptr;
    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
                            IID_IWbemLocator, (LPVOID*)&pLoc);
    if (FAILED(hres)) {
        CoUninitialize();
        return "EMPTY";
    }

    IWbemServices* pSvc = nullptr;
    // Передаём wide-строку как (BSTR)
    hres = pLoc->ConnectServer((BSTR)L"ROOT\\CIMV2", nullptr, nullptr, 0, 0,
                               nullptr, nullptr, &pSvc);
    if (FAILED(hres)) {
        pLoc->Release();
        CoUninitialize();
        return "EMPTY";
    }

    hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr,
                             RPC_C_AUTHN_LEVEL_CALL,
                             RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);
    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return "EMPTY";
    }

    IEnumWbemClassObject* pEnum = nullptr;
    hres = pSvc->ExecQuery(
        (BSTR)L"WQL",
        (BSTR)L"SELECT Manufacturer, SMBIOSBIOSVersion FROM Win32_BIOS",
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &pEnum);

    string result = "EMPTY";
    if (SUCCEEDED(hres) && pEnum) {
        IWbemClassObject* pObj = nullptr;
        ULONG uReturn = 0;
        if (pEnum->Next(WBEM_INFINITE, 1, &pObj, &uReturn) == S_OK && uReturn) {
            VARIANT vMan, vVer;
            VariantInit(&vMan);
            VariantInit(&vVer);
            pObj->Get(L"Manufacturer", 0, &vMan, 0, 0);
            pObj->Get(L"SMBIOSBIOSVersion", 0, &vVer, 0, 0);
            if (vMan.vt == VT_BSTR && vVer.vt == VT_BSTR) {
                string man = bstrToUtf8(vMan.bstrVal);
                string ver = bstrToUtf8(vVer.bstrVal);
                if (!man.empty() || !ver.empty()) {
                    result = man + " - " + ver;
                }
            }
            VariantClear(&vMan);
            VariantClear(&vVer);
            pObj->Release();
        }
        pEnum->Release();
    }

    pSvc->Release();
    pLoc->Release();
    CoUninitialize();
    return result;
}

string getCpuBrand() {
    int cpuInfo[4] = {0};
    char brand[49] = {0};  // 3 chunks по 16 байт = 48 + '\0'

    // Проверяем, поддерживает ли CPUID функцию 0x80000004 (brand string)
    __cpuid(cpuInfo, 0x80000000);
    unsigned int maxExId = cpuInfo[0];

    if (maxExId < 0x80000004) {
        return "Unknown CPU";
    }

    // Собираем brand string из трёх вызовов
    __cpuidex(reinterpret_cast<int*>(brand + 0), 0x80000002, 0);
    __cpuidex(reinterpret_cast<int*>(brand + 16), 0x80000003, 0);
    __cpuidex(reinterpret_cast<int*>(brand + 32), 0x80000004, 0);

    return string(brand);
}

vector<string> getVideoAdapters() {
    vector<string> adapters;
    HRESULT hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) return adapters;

    hres = CoInitializeSecurity(
        nullptr, -1, nullptr, nullptr, RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE, nullptr);
    if (FAILED(hres)) {
        CoUninitialize();
        return adapters;
    }

    IWbemLocator* pLoc = nullptr;
    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
                            IID_IWbemLocator, (LPVOID*)&pLoc);
    if (FAILED(hres)) {
        CoUninitialize();
        return adapters;
    }

    IWbemServices* pSvc = nullptr;
    hres = pLoc->ConnectServer((BSTR)L"ROOT\\CIMV2", nullptr, nullptr, 0, 0,
                               nullptr, nullptr, &pSvc);
    if (FAILED(hres)) {
        pLoc->Release();
        CoUninitialize();
        return adapters;
    }

    hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr,
                             RPC_C_AUTHN_LEVEL_CALL,
                             RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);
    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return adapters;
    }

    IEnumWbemClassObject* pEnum = nullptr;
    hres = pSvc->ExecQuery(
        (BSTR)L"WQL", (BSTR)L"SELECT Name FROM Win32_VideoController",
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &pEnum);

    if (SUCCEEDED(hres) && pEnum) {
        IWbemClassObject* pObj = nullptr;
        ULONG uReturn = 0;
        while (pEnum->Next(WBEM_INFINITE, 1, &pObj, &uReturn) == S_OK &&
               uReturn) {
            VARIANT vName;
            VariantInit(&vName);
            if (SUCCEEDED(pObj->Get(L"Name", 0, &vName, 0, 0)) &&
                vName.vt == VT_BSTR) {
                adapters.push_back(bstrToUtf8(vName.bstrVal));
            }
            VariantClear(&vName);
            pObj->Release();
        }
        pEnum->Release();
    }

    pSvc->Release();
    pLoc->Release();
    CoUninitialize();
    return adapters;
}

// Возвращает средний пинг в миллисекундах (0 при ошибке)
DWORD getAveragePing(const char* host, int attempts) {
    if (!host || attempts <= 0) return 0;

    // Инициализация Winsock (требуется для inet_addr / getaddrinfo)
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return 0;
    

    // Преобразуем имя хоста в IP-адрес
    ADDRINFOA hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = 0;
    hints.ai_protocol = 0;

    ADDRINFOA* result = nullptr;
    if (getaddrinfo(host, nullptr, &hints, &result) != 0) {
        WSACleanup();
        return 0;
    }

    sockaddr_in* sa = reinterpret_cast<sockaddr_in*>(result->ai_addr);
    IPAddr destIp = sa->sin_addr.S_un.S_addr;
    freeaddrinfo(result);

    // Открываем ICMP-файл
    HANDLE hIcmp = IcmpCreateFile();
    if (hIcmp == INVALID_HANDLE_VALUE) {
        WSACleanup();
        return 0;
    }

    // Буфер для ответа (достаточно для одного эха + данные)
    constexpr size_t replySize = sizeof(ICMP_ECHO_REPLY) + 8;
    vector<BYTE> replyBuf(replySize);

    vector<DWORD> rtts;
    rtts.reserve(attempts);

    for (int i = 0; i < attempts; ++i) {
        DWORD replyCount =
            IcmpSendEcho(hIcmp, destIp,
                         nullptr,  // данные (можно nullptr)
                         0,        // размер данных
                         nullptr,  // опции (обычно nullptr)
                         replyBuf.data(), static_cast<DWORD>(replyBuf.size()),
                         10000  // таймаут в миллисекундах
            );

        if (replyCount > 0) {
            PICMP_ECHO_REPLY reply =
                reinterpret_cast<PICMP_ECHO_REPLY>(replyBuf.data());
            if (reply->Status == IP_SUCCESS) {
                rtts.push_back(reply->RoundTripTime);
            }
        }

        Sleep(100);  // небольшая пауза между попытками
    }

    IcmpCloseHandle(hIcmp);

    if (rtts.empty()) {
        return 0;  // ни один пинг не удался
    }

    // Среднее арифметическое
    unsigned long long sum = 0;
    for (DWORD rtt : rtts) {
        sum += rtt;
    }
    return static_cast<DWORD>(sum / rtts.size());
}
