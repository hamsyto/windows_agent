// Windows/WindowsCollector.h
#ifndef WINDOWS_COLLECTOR_H
#define WINDOWS_COLLECTOR_H

#include <vector>

#include "../../models/settings.h"
#include "../ICollector.h"

class WindowsCollector : public ICollector {
   private:
    Settings settings_;

    std::vector<Disk> GetDisksImpl();
    RAM GetRamImpl();
    CPU GetCpuImpl();
    OS GetOsImpl();
    Hardware GetHardwareImpl();
    Ping GetPingImpl();

   public:
    explicit WindowsCollector(const Settings& settings);

    std::vector<Disk> GetDisks() override;
    RAM GetRam() override;
    CPU GetCpu() override;
    OS GetOs() override;
    Hardware GetHardware() override;
    Ping GetPing() override;
    Message Collect() override;
};

#endif  // WINDOWS_COLLECTOR_H