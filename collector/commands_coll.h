// commands_coll.h

#ifndef COLLECTORR_COLL_H
#define COLLECTORR_COLL_H

#include <Wbemidl.h>  // содержит объявление BSTR, IWbemLocator и т.д.

#include <string>
#include <unordered_map>
#include <vector>

#include "../models/windows_simple_message.h"

// УниверсФунк читает и чистит .env от комментариев и символов
std::unordered_map<std::string, std::string> ClearEnvFile();
// Вспомогательная функция: обрезать пробелы по краям
std::string Trim(const std::string& str);
// Вспомогательная функция — нормализация имени тома
std::string NormalizeVolumeName(const char* volumeName);
// Вспомогательная: получить общий размер физического диска
uint64_t GetPhysicalDiskSize(int diskIndex);

int GetPhysicalDiskIndexForDriveLetter(char letter);

uint64_t GetUsedSpaceOnPhysicalDisk(int targetDiskIndex);

Disk FillDiskInfo(int diskIndex);
// читает .env и заполняет Settings
bool LoadEnvSettings(Settings& out);
// Вспомогательная: определить тип диска (HDD/SSD)
bool IsHDD(int diskIndex);

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