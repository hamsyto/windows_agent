// collector.h
#ifndef COLLECTOR_H
#define COLLECTOR_H

#include <vector>

#include "../models/message.h"

std::vector<Disk> GetDisks();
RAM GetRam();
CPU GetCpu();
OS GetOs();
Hardware GetHardware();
Ping GetPing();

#endif