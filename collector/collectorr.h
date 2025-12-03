// collectorr.h

// CPU (комплекснмя, где-то вероятно придётся использтвать sleep(wait))
// RAM
// OS (version, hostname, time+timezone)
// HDD+SSD (free + total)
// Здесь по 1 функции на каждую модель данных, каждая собирает своё и возвращает
// свой тип данных
#ifndef COLLECTORR_H
#define COLLECTORR_H
#define WIN32_LEAN_AND_MEAN

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "../models/windows_simple_message.h"
#include <vector> 

std::vector<Disk> GetDisks();
RAM GetRam();
CPU GetCpu();
OS GetOs();
Hardware GetHardware();

#endif 