// Windows/сollector.h
#ifndef COLLECTOR_H
#define COLLECTOR_H

#include <vector>

#include "../../models/settings.h"
#include "../ICollector.h"

class WindowsCollector : public ICollector {
   private:
    Settings settings_;

    std::vector<Disk> GetDisks() override;
    std::vector<USB> GetUSBs() override;
    RAM GetRam() override;
    CPU GetCpu() override;
    OS GetOs() override;
    Hardware GetHardware() override;
    int GetPing() override;

   public:
    explicit WindowsCollector(const Settings& settings);
    // explicit WindowsCollector(Message& msg);
    Payload GetPayload() override;
};

#endif  // WINDOWS_COLLECTOR_H