// commands_coll.h

#ifndef COLLECTORR_COLL_H
#define COLLECTORR_COLL_H

#include <winsock2.h>

#include <string>

#include "../models/windows_simple_message.h"
using namespace std;

Disk FillDiskInfo(int diskIndex);
// Вспомогательная: определить тип диска (HDD/SSD)
bool IsHDD(int diskIndex);
// Вспомогательная: получить общий размер физического диска
ULONGLONG GetPhysicalDiskSize(int diskIndex);

string GetOsVersionName();

string GetOsVersionName();
double GetCPUUsage();

bool IsHDD(int diskIndex);  // прототип
string get_computer_domain_or_workgroup();

#endif