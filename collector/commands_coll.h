// commands_coll.h

#ifndef COLLECTORR_COLL_H
#define COLLECTORR_COLL_H

#include <Wbemidl.h>  // содержит объявление BSTR, IWbemLocator и т.д.

#include <string>
#include <vector>
#include <unordered_map>

#include "../models/windows_simple_message.h"

// УниверсФунк читает и чистит .env
std::unordered_map<std::string, std::string> ClearEnvFile() ;
// Прочитать .env и заполнить Settings
bool LoadEnvSettings(Settings& out);

Disk FillDiskInfo(int diskIndex);
// Вспомогательная: определить тип диска (HDD/SSD)
bool IsHDD(int diskIndex);
// Вспомогательная: получить общий размер физического диска
ULONGLONG GetPhysicalDiskSize(int diskIndex);
std::string GetOsVersionName();
std::string get_computer_domain_or_workgroup();
double GetCPUUsage();
std::vector<std::string> getMacAddresses();
// Функция конвертации
std::string bstrToUtf8(BSTR bstr);
std::string getBiosInfo();
std::string getCpuBrand();
std::vector<std::string> getVideoAdapters();
// Возвращает средний пинг в миллисекундах (0 при ошибке)
DWORD getAveragePing(const char* host, int attempts = 3);
// Вспомогательная функция: обрезать пробелы по краям
std::string Trim(const std::string& str);


#endif