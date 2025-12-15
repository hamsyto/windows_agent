// collector.h

// CPU (комплекснмя, где-то вероятно придётся использтвать sleep(wait))
// RAM
// OS (version, hostname, time+timezone)
// HDD+SSD (free + total)
// Здесь по 1 функции на каждую модель данных, каждая собирает своё и возвращает
// свой тип данных
#ifndef collector_H
#define collector_H

#include <vector>

#include "../models/message.h"

std::vector<Disk> GetDisks();
RAM GetRam();
CPU GetCpu();
OS GetOs();
Hardware GetHardware();
Ping GetPing();

#endif