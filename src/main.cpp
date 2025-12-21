
#ifdef _WIN32
#include <winsock2.h>
// Порядок
#include <windows.h>
#endif

#include <iostream>

#include "collector/collector_fabric.h"
#include "connection/connection_fabric.h"
#include "consts.h"
#include "transport/transport.h"

using namespace std;

bool InitIalize() {
#ifdef _WIN32
    WSADATA wsData;
    if (WSAStartup(MAKEWORD(2, 2), &wsData) != 0) {
        // cout << "WSAStartup failed" << endl;
        Shutdown();
        return false;
    }
#endif
    return true;
}

bool Shutdown() {
#ifdef _WIN32
    WSACleanup();
#endif
    return true;
}

int main() {
    if (!InitIalize()) return 1;

    Settings settings = LoadEnvSettings(kFileName);
    auto connection = CreateConnection(settings);
    connection->Connect();

    auto collector = CreateCollector(settings);

    while (true) {
        collector->GetPayload();

        string message = GetMassage(collector);
        connection->Send(message);
    }

    Shutdown();
    return 0;
}