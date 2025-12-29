// helpers.h

#ifndef HELPERS_H
#define HELPERS_H

#include <Wbemidl.h>  // содержит объявление BSTR, IWbemLocator и т.д.
#include <setupapi.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "../../models/message.h"
#include "../../models/settings.h"

// общие для USB и Disks
double GetTotalDisk(const std::string& root);
int GetPhysicalDiskIndexForDriveLetter(char letter);
std::string GetBusType(int diskIndex);

// диски
double GetUsedDisk(const std::string& root);
Disk FillDiskInfo(int& diskIndex, std::string& root);

// USB
// Вспомогательная функция для проверки родительских отношений
// Вспомогательная функция: извлекает VID из DeviceInstanceId
std::string ExtractVendorId(const std::string& deviceId);
// Основная функция
bool GetUsbDeviceInfo(char driveLetter, std::string& outVendorId,
                      std::string& outDeviceId, std::string& outName);
bool DoesDeviceBelongToParent(HDEVINFO hChild, SP_DEVINFO_DATA* childData,
                              HDEVINFO hParent, SP_DEVINFO_DATA* parentData);
int GetPhysicalDiskIndexForDriveLetter(char driveLetter);

std::string GetVolumeLabel(const std::string& root);
USB FillUSBInfo(const std::string& root, int diskIndex);
// ОС
std::string GetOsVersionName();
std::string GetComputerDomainOrWorkgroup();
double GetCPUUsage();
std::vector<std::string> getMacAddresses();
// Функция конвертации
std::string BstrToUtf8(BSTR bstr);
std::string GetBiosInfo();
std::string GetCpuBrand();
std::vector<std::string> GetVideoAdapters();
// Возвращает средний пинг в миллисекундах (0 при ошибке)
DWORD GetAveragePing(const char* host, int attempts = 3);

#endif