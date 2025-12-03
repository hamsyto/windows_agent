// commands_coll.h

#ifndef COLLECTORR_COLL_H
#define COLLECTORR_COLL_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Wbemidl.h>  // содержит объявление BSTR, IWbemLocator и т.д.
#include <vector>
#include <string>

#include "../models/windows_simple_message.h"

Disk FillDiskInfo(int diskIndex);
// Вспомогательная: определить тип диска (HDD/SSD)
bool IsHDD(int diskIndex);
// Вспомогательная: получить общий размер физического диска
ULONGLONG GetPhysicalDiskSize(int diskIndex);
std::string GetOsVersionName();
std::string get_computer_domain_or_workgroup();
double GetCPUUsage();
std::vector<std::string> getMacAddresses();
std::string bstrToUtf8(BSTR bstr);  // Объявляем функцию конвертации
std::string getBiosInfo();
std::string getCpuBrand();
std::vector<std::string> getVideoAdapters();

#endif