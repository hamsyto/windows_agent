// helpers.cpp
#define INITGUID
//== 1 == Важен порядок
#include <winsock2.h>
//== 2 ==
#include <windows.h>
//== 3 ==
#include <ws2tcpip.h>
//== 4 ==
#include <iphlpapi.h>
//== 5 ==
#include <icmpapi.h>

// NetAPI
#include <lmapibuf.h>  // объявляет NetApiBufferFree
#include <lmerr.h>     // определяет NERR_Success (как DWORD)
#include <lmjoin.h>    // объявляет NetGetJoinInformation

// Windows API
#include <VersionHelpers.h>
#include <cfgmgr32.h>
#include <devguid.h>
#include <intrin.h>
#include <setupapi.h>
// == 1 ==
#include <winioctl.h>  // обязательно перед ntdddisk.h
// == 2 ==
// #include <ntdddisk.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../../consts.h"
#include "helpers.h"

// === предложение нейронки: линковка WMI ===
// #pragma comment(lib, "wbemuuid.lib")

using namespace std;

// общие для USB и Disks

double GetTotalDisk(const string& root) {
    ULARGE_INTEGER freeBytes = {};
    ULARGE_INTEGER totalBytes = {};

    if (!GetDiskFreeSpaceExA(root.c_str(), &freeBytes, &totalBytes, nullptr)) {
        return 0;  // ошибка — вернуть 0 или можно бросить исключение
    }

    if (totalBytes.QuadPart < freeBytes.QuadPart)
        return 0;  // некорректные данные

    return round(
               (static_cast<double>(totalBytes.QuadPart) / (1024.0 * 1024.0)) *
               100.0) /
           100.0;
}

// Возвращает индекс физического диска (0, 1, 2...) по букве логического тома
// (C, D, ...) Возвращает -1 при ошибке.
int GetPhysicalDiskIndexForDriveLetter(char letter) {
    char volumePath[64];
    snprintf(volumePath, sizeof(volumePath), "\\\\?\\%c:", letter);

    HANDLE hVolume =
        CreateFileA(volumePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                    OPEN_EXISTING, 0, nullptr);
    if (hVolume == INVALID_HANDLE_VALUE) return -1;

    STORAGE_DEVICE_NUMBER sdNumber = {};
    DWORD bytesReturned = 0;
    bool success =
        DeviceIoControl(hVolume, IOCTL_STORAGE_GET_DEVICE_NUMBER, nullptr, 0,
                        &sdNumber, sizeof(sdNumber), &bytesReturned, nullptr);
    CloseHandle(hVolume);
    return success ? static_cast<int>(sdNumber.DeviceNumber) : -1;
}

string GetBusType(int diskIndex) {
    char path[64] = {};
    snprintf(path, sizeof(path), "\\\\.\\PhysicalDrive%d", diskIndex);

    HANDLE hDevice = CreateFileA(path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 nullptr, OPEN_EXISTING, 0, nullptr);
    if (hDevice == INVALID_HANDLE_VALUE) {
        return "ERROR";
    }

    STORAGE_PROPERTY_QUERY query = {};
    query.PropertyId = StorageDeviceProperty;
    query.QueryType = PropertyStandardQuery;

    // Сначала получаем нужный размер буфера
    STORAGE_DESCRIPTOR_HEADER header = {};
    DWORD bytesReturned = 0;

    if (!DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, &query,
                         sizeof(query), &header, sizeof(header), &bytesReturned,
                         nullptr)) {
        DWORD err = GetLastError();
        CloseHandle(hDevice);

        // Если диск не поддерживает запрос, возвращаем UNKNOWN
        if (err == ERROR_INVALID_FUNCTION || err == ERROR_NOT_SUPPORTED) {
            return "UNKNOWN";
        }
        return "ERROR";
    }

    // Выделяем буфер нужного размера
    DWORD bufferSize = header.Size;
    if (bufferSize == 0) {
        CloseHandle(hDevice);
        return "UNKNOWN";
    }

    auto buffer = make_unique<BYTE[]>(bufferSize);
    PSTORAGE_DEVICE_DESCRIPTOR desc =
        reinterpret_cast<PSTORAGE_DEVICE_DESCRIPTOR>(buffer.get());

    if (!DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, &query,
                         sizeof(query), desc, bufferSize, &bytesReturned,
                         nullptr)) {
        CloseHandle(hDevice);
        DWORD err = GetLastError();
        if (err == ERROR_INVALID_FUNCTION || err == ERROR_NOT_SUPPORTED) {
            return "UNKNOWN";
        }
        return "ERROR";
    }

    CloseHandle(hDevice);

    // Проверяем, что поле BusType доступно
    if (desc->BusType == 0 ||
        desc->Size < FIELD_OFFSET(STORAGE_DEVICE_DESCRIPTOR, BusType) +
                         sizeof(desc->BusType)) {
        return "UNKNOWN";
    }

    switch (desc->BusType) {
        case BusTypeUnknown:
            return "UNKNOWN";
        case BusTypeScsi:
            return "SCSI";
        case BusTypeAtapi:
            return "ATAPI";
        case BusTypeAta:
            return "ATA";
        case BusType1394:
            return "1394";
        case BusTypeSsa:
            return "SSA";
        case BusTypeFibre:
            return "FIBRE";
        case BusTypeUsb:
            return "USB";
        case BusTypeiScsi:
            return "iSCSI";
        case BusTypeSas:
            return "SAS";
        case BusTypeSata:
            return "SATA";
        case BusTypeSd:
            return "SD";
        case BusTypeMmc:
            return "MMC";
        case BusTypeVirtual:
            return "VIRTUAL";
        case BusTypeFileBackedVirtual:
            return "FILE_BACKED_VIRTUAL";
        case BusTypeSpaces:
            return "SPACES";
        case BusTypeNvme:
            return "NVME";
        case BusTypeSCM:
            return "SCM";
        case BusTypeUfs:
            return "UFS";
        case BusTypeMax:
            return "MAX";
        default:
            return "UNKNOWN";
    }
}

// диски

double GetUsedDisk(const string& root) {
    ULARGE_INTEGER freeBytes = {};
    ULARGE_INTEGER totalBytes = {};

    if (!GetDiskFreeSpaceExA(root.c_str(), &freeBytes, &totalBytes, nullptr)) {
        return 0;  // ошибка — вернуть 0 или можно бросить исключение
    }

    if (totalBytes.QuadPart < freeBytes.QuadPart)
        return 0;  // некорректные данные

    return round(
               (static_cast<double>(totalBytes.QuadPart - freeBytes.QuadPart) /
                (1024.0 * 1024.0)) *
               100.0) /
           100.0;
}

Disk FillDiskInfo(int& diskIndex, string& root) {
    Disk disk = {};

    disk.number = diskIndex;
    disk.mount = root;
    disk.type = GetBusType(diskIndex);
    disk.total = GetTotalDisk(root);
    disk.used = GetUsedDisk(root);
    return disk;
}

// USB
// Вспомогательная функция: извлекает VID из DeviceInstanceId
std::string ExtractVendorId(const std::string& deviceId) {
    size_t pos = deviceId.find("VID_");
    if (pos == std::string::npos) return "UNKNOWN";
    pos += 4;
    size_t end = deviceId.find_first_not_of("0123456789ABCDEF", pos);
    if (end == std::string::npos) end = deviceId.size();
    std::string vid = deviceId.substr(pos, end - pos);
    if (vid.size() != 4) return "UNKNOWN";
    std::transform(vid.begin(), vid.end(), vid.begin(), ::toupper);
    return vid;
}

// Основная функция
bool GetUsbDeviceInfo(char driveLetter, std::string& outVendorId,
                      std::string& outDeviceId, std::string& outName) {
    if (driveLetter == '\0') return false;

    // === ШАГ 1: Получить DeviceNumber целевого тома ===
    std::string volPath = "\\\\.\\\\" + std::string(1, driveLetter) + ":";
    HANDLE hVol =
        CreateFileA(volPath.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
                    nullptr, OPEN_EXISTING, 0, nullptr);
    if (hVol == INVALID_HANDLE_VALUE) return false;

    STORAGE_DEVICE_NUMBER sdnTarget = {};
    DWORD bytes = 0;
    DeviceIoControl(hVol, IOCTL_STORAGE_GET_DEVICE_NUMBER, nullptr, 0,
                    &sdnTarget, sizeof(sdnTarget), &bytes, nullptr);
    CloseHandle(hVol);

    // === ШАГ 2: Перечислить все устройства ===
    HDEVINFO hDevInfo = SetupDiGetClassDevsA(nullptr, nullptr, nullptr,
                                             DIGCF_PRESENT | DIGCF_ALLCLASSES);
    if (hDevInfo == INVALID_HANDLE_VALUE) return false;

    SP_DEVINFO_DATA devInfo = {sizeof(SP_DEVINFO_DATA)};
    bool found = false;
    std::string deviceId, friendlyName;
    char parentId[MAX_PATH] = {};

    for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfo); ++i) {
        char instanceId[MAX_PATH] = {};
        if (!SetupDiGetDeviceInstanceIdA(hDevInfo, &devInfo, instanceId,
                                         MAX_PATH, nullptr))
            continue;

        std::string id(instanceId);
        if (id.find("DISK&") == std::string::npos) continue;

        // Проверяем родителя на USB
        DWORD parentDevInst = 0;
        if (CM_Get_Parent(&parentDevInst, devInfo.DevInst, 0) != CR_SUCCESS)
            continue;

        if (CM_Get_Device_IDA(parentDevInst, parentId, MAX_PATH, 0) !=
            CR_SUCCESS)
            continue;

        if (std::string(parentId).find("USB\\VID_") == std::string::npos)
            continue;

        // Получаем имя
        char nameBuf[256] = {};
        std::string name = "Unknown USB Device";
        if (SetupDiGetDeviceRegistryPropertyA(
                hDevInfo, &devInfo, SPDRP_FRIENDLYNAME, nullptr, (PBYTE)nameBuf,
                sizeof(nameBuf), nullptr) ||
            SetupDiGetDeviceRegistryPropertyA(
                hDevInfo, &devInfo, SPDRP_DEVICEDESC, nullptr, (PBYTE)nameBuf,
                sizeof(nameBuf), nullptr)) {
            name = nameBuf;
        }

        // === СРАВНЕНИЕ ЧЕРЕЗ РЕЕСТР (вместо DeviceIoControl на интерфейс) ===
        HKEY hKey = SetupDiOpenDevRegKey(hDevInfo, &devInfo, DICS_FLAG_GLOBAL,
                                         0, DIREG_DEV, KEY_READ);
        if (hKey == INVALID_HANDLE_VALUE) continue;

        DWORD devNumber = 0;
        DWORD type, size = sizeof(DWORD);
        LONG res = RegQueryValueExA(hKey, "DeviceNumber", nullptr, &type,
                                    (LPBYTE)&devNumber, &size);
        RegCloseKey(hKey);

        // if (res != ERROR_SUCCESS) {
        //     // Например, выведите в консоль или через OutputDebugStringA
        //     printf("RegQueryValueExA failed: %lu\n", res);
        //     // или:
        //     // OutputDebugStringA(("Reg error: " + std::to_string(res) +
        //     // "\n").c_str());
        // }
        // printf("sdnTarget.DeviceNumber = %lu\n", sdnTarget.DeviceNumber);
        // printf("devNumber from registry = %lu\n", devNumber);
        // if (res == ERROR_SUCCESS && devNumber == sdnTarget.DeviceNumber) {
        //     deviceId = id;
        //     friendlyName = name;
        //     found = true;
        //     break;
        // }

        deviceId = id;
        friendlyName = name;
        found = true;
        break;
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);
    if (!found) return false;

    outDeviceId = deviceId;
    outVendorId = ExtractVendorId(parentId);
    outName = friendlyName;
    return true;
}

// Вспомогательная функция для проверки родительских отношений
bool DoesDeviceBelongToParent(HDEVINFO hChild, SP_DEVINFO_DATA* childData,
                              HDEVINFO hParent, SP_DEVINFO_DATA* parentData) {
    char childId[512], parentId[512];

    if (!SetupDiGetDeviceInstanceIdA(hChild, childData, childId,
                                     sizeof(childId), nullptr) ||
        !SetupDiGetDeviceInstanceIdA(hParent, parentData, parentId,
                                     sizeof(parentId), nullptr))
        return false;

    // Проверяем, содержит ли childId parentId как префикс
    size_t parentLen = strlen(parentId);
    return (strncmp(childId, parentId, parentLen) == 0);
}

string GetVolumeLabel(const string& root) {
    char label[MAX_PATH] = {};
    if (GetVolumeInformationA(root.c_str(), label, MAX_PATH, nullptr, nullptr,
                              nullptr, nullptr, 0)) {
        return string(label);
    }
    return "";  // без метки
}

USB FillUSBInfo(const std::string& root, int diskIndex) {
    USB usb = {};
    usb.mount = root;

    char driveLetter = root.empty() ? '\0' : root[0];

    string vendorId, deviceId, name;
    if (GetUsbDeviceInfo(driveLetter, vendorId, deviceId, name)) {
        usb.device_id = deviceId;
        usb.vendor_id = vendorId;
        usb.name = name;
    } else {
        usb.vendor_id = "UNKNOWN";
        usb.device_id = "UNKNOWN";
        usb.name = "Unknown USB Device";
    }

    usb.label = GetVolumeLabel(root);
    usb.total = static_cast<uint32_t>(GetTotalDisk(root));

    return usb;
}

// ОС
// возвращает название ОС
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

string GetComputerDomainOrWorkgroup() {
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
uint64_t s_lastIdleTime = 0;
uint64_t s_lastTotalTime = 0;

double GetCPUUsage() {
    FILETIME idleTime, kernelTime, userTime;
    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        return -1.0;  // ошибка
    }

    // Объединяем FILETIME в 64-битные значения (в 100-нс интервалах)
    ULARGE_INTEGER idle, kernel, user;
    idle.QuadPart = (static_cast<uint64_t>(idleTime.dwHighDateTime) << 32) |
                    idleTime.dwLowDateTime;
    kernel.QuadPart = (static_cast<uint64_t>(kernelTime.dwHighDateTime) << 32) |
                      kernelTime.dwLowDateTime;
    user.QuadPart = (static_cast<uint64_t>(userTime.dwHighDateTime) << 32) |
                    userTime.dwLowDateTime;

    uint64_t total = kernel.QuadPart + user.QuadPart;  // kernel включает idle

    // Пропускаем первый вызов (нет предыдущих данных)
    static bool firstCall = true;
    if (firstCall) {
        firstCall = false;
        s_lastIdleTime = idle.QuadPart;
        s_lastTotalTime = total;
        return 0.0;
    }

    // Считаем дельты
    uint64_t idleDelta = idle.QuadPart - s_lastIdleTime;
    uint64_t totalDelta = total - s_lastTotalTime;

    // Обновляем состояние
    s_lastIdleTime = idle.QuadPart;
    s_lastTotalTime = total;

    // Избегаем деления на ноль
    if (totalDelta == 0) return 0.0;

    // Загрузка = 1 - (idle / total)
    double used =
        1.0 - static_cast<double>(idleDelta) / static_cast<double>(totalDelta);

    // Ограничиваем диапазон [0.0, 1.0]
    if (used < 0.0) used = 0.0;
    if (used > 1.0) used = 1.0;

    return used;  // или (usage1 + usage2) / 2 для сглаживания
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

// преобразует строку в формате BSTR
string BstrToUtf8(BSTR bstr) {
    if (!bstr) return "";
    int len =
        WideCharToMultiByte(CP_UTF8, 0, bstr, -1, nullptr, 0, nullptr, nullptr);
    if (len <= 1) return "";
    string result(len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, bstr, -1, &result[0], len, nullptr,
                        nullptr);
    return result;
}

string GetBiosInfo() {
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
                string man = BstrToUtf8(vMan.bstrVal);
                string ver = BstrToUtf8(vVer.bstrVal);
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

string GetCpuBrand() {
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

vector<string> GetVideoAdapters() {
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
    hres = pLoc->ConnectServer(const_cast<LPWSTR>(L"ROOT\\CIMV2"), nullptr,
                               nullptr, 0, 0, nullptr, nullptr, &pSvc);
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

    // === ИСПРАВЛЕНО: query объявлен ===
    const wchar_t* queryStr = L"SELECT Name FROM Win32_VideoController";
    IEnumWbemClassObject* pEnum = nullptr;
    hres = pSvc->ExecQuery(
        const_cast<LPWSTR>(L"WQL"), const_cast<LPWSTR>(queryStr),
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
                adapters.push_back(BstrToUtf8(vName.bstrVal));
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
DWORD GetAveragePing(const char* host, int attempts) {
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
