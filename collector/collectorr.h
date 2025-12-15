// collectorr.h

// CPU (комплекснмя, где-то вероятно придётся использтвать sleep(wait))
// RAM
// OS (version, hostname, time+timezone)
// HDD+SSD (free + total)
// Здесь по 1 функции на каждую модель данных, каждая собирает своё и возвращает
// свой тип данных
#ifndef COLLECTORR_H
#define COLLECTORR_H

#include <models/windows_simple_message.h>

#include <vector>

std::vector<Disk> GetDisks();
RAM GetRam();
CPU GetCpu();
OS GetOs();
Hardware GetHardware();
Ping GetPing();

#endif