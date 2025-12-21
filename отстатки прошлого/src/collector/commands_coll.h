// commands_coll.h

#ifndef collector_COLL_H
#define collector_COLL_H

#include <Wbemidl.h>  // содержит объявление BSTR, IWbemLocator и т.д.

#include <string>
#include <unordered_map>
#include <vector>

#include "../models/message.h"
#include "../models/settings.h"

// Вспомогательная: получить общий размер физического диска
uint64_t GetPhysicalDiskSize(int diskIndex);

int GetPhysicalDiskIndexForDriveLetter(char letter);

std::string GetMountGetUsedSpace(int targetDiskIndex, uint32_t& used);

Disk FillDiskInfo(int& diskIndex, std::string& root);
// Вспомогательная: определить тип диска (HDD/SSD)
std::string GetBusType(int diskIndex);

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