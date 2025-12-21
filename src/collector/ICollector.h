// ICollector.h
#ifndef ICOLLECTOR_H
#define ICOLLECTOR_H

#include <memory>
#include <vector>

#include "../models/message.h"
#include "../models/settings.h"

// Чисто абстрактный интерфейс — только = 0 методы
class ICollector {
   public:
    virtual ~ICollector() = default;

    virtual std::vector<Disk> GetDisks() = 0;
    virtual RAM GetRam() = 0;
    virtual CPU GetCpu() = 0;
    virtual OS GetOs() = 0;
    virtual Hardware GetHardware() = 0;
    virtual Ping GetPing() = 0;

    virtual Message GetPayLoad() = 0;
};

std::unique_ptr<ICollector> CreateCollector(const Settings& settings);

#endif