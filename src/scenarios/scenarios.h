#pragma once

#include <iostream>

#include "../collector/collector_fabric.h"
#include "../connection/connection_fabric.h"
#include "../consts.h"
#include "../transport/transport.h"

using namespace std;

int Registrate(Payload payload, IConnection* connection,
               const Settings& settings);

void SendReport(Payload payload, IConnection* connection,
                const Settings& settings);
