// Windows/WindowsCollector.h
#ifndef WINDOWS_COLLECTOR_H
#define WINDOWS_COLLECTOR_H

#include <vector>

#include "../../models/settings.h"
#include "../ICollector.h"

class WindowsCollector : public ICollector {
 private:
  Settings settings_;

  std::vector<Disk> GetDisks() override;
  RAM GetRam() override;
  CPU GetCpu() override;
  OS GetOs() override;
  Hardware GetHardware() override;
  Ping GetPing() override;

 public:
  explicit WindowsCollector(const Settings& settings);
  explicit WindowsCollector(Message& msg);
  SimplePCReport GetPayload() override;
};

#endif  // WINDOWS_COLLECTOR_H