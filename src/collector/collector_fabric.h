// collector_fabric.h

#ifndef COLLECTOR_FABRIC_H
#define COLLECTOR_FABRIC_H

#include "ICollector.h"
#include "windows/collector.h"

std::unique_ptr<ICollector> CreateCollector(const Settings& settings) {
#ifdef _WIN32
  return std::make_unique<WindowsCollector>(settings);
#else
  // Позже: LinuxCollector и т.д.
  return nullptr;
#endif
}

#endif