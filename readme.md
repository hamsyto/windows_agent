сделать
+ 4 функции в collector.cpp

- обёртка в commands.h для строк 60-64, 70-73



   // си-стиль: (struct sockaddr*)&addr;
    // си++ -стиль: reinterpret_cast<struct sockaddr*>(&addr);  static_cast,
    // const_cast
    // struct sockaddr* pAddr = reinterpret_cast<struct sockaddr*>(&addr);



////////////////// RAM /////////////

- - отражает память видимую ОС - - 
    
// возвращает true при успехе
GlobalMemoryStatusEx() //  <windows.h>

 MEMORYSTATUSEX.
    DWORDLONG ullTotalPhys;        // общий объём физической памяти (в байтах)
    DWORDLONG ullAvailPhys;        // доступно физической памяти
    // DWORDLONG — это unsigned __int64 (или uint64_t в терминах C++).

MEMORYSTATUSEX mem = {};
mem.dwLength = sizeof(mem);
if (GlobalMemoryStatusEx(&mem)) {
    ULONGLONG totalMB = mem.ullTotalPhys / (1024 * 1024);
}


  - - отражает реальное железо - - 
    
GetPhysicallyInstalledSystemMemory  //  <windows.h>

 ULONGLONG memKB = 0;
if (GetPhysicallyInstalledSystemMemory(&memKB)) {
    ULONGLONG totalMB = memKB / 1024;
} 
