// Windows/WindowsCollector.h
#ifndef WINDOWS_COLLECTOR_H
#define WINDOWS_COLLECTOR_H

#include <vector>

#include "../../models/settings.h"
#include "../ICollector.h"

class WindowsCollector : public ICollector {
   private:
    Settings settings_;

   public:
    explicit WindowsCollector(Message& msg);

    std::vector<Disk> GetDisks() override;
    RAM GetRam() override;
    CPU GetCpu() override;
    OS GetOs() override;
    Hardware GetHardware() override;
    Ping GetPing() override;

    Message GetPayload() override;
};

#endif  // WINDOWS_COLLECTOR_H