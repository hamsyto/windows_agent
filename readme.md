сделать
+ 4 функции в collector.cpp
+ убрать отправку длины сообщения
+ поменять порты на 8000+
+   Надо сделать автоматическую регистрацию.
    1. Если agent_id не задан, то на сервер идёт сообщение,     что клиент хочет зарегистрироваться
        в формате json
        Payload пустой словарик, a тип сообщения whoami
        Ну, в header
        А agent_ID null
    2. Сервер отсылает ему его id.
    3. Клиент сохраняет у себя в памяти и в файле, и потом  отсылает его с каждым сообщением
+ system.sleep(settings.sleepInSeconds);
+ убрать вывод о нажатии enter чтобы переконектиться
+   если будет время / желание, сделай модуль, который  будет       инициализировать глобальную переменную   "SETTINGS", это     та самая наша структура с     настройками проекта.
    Там обязательно должны быть:
    1. int — время слипа между опросами;
    2. str — адрес сервера;
    3. int/str — порт сервера, на который будет закидываться       данные;
    
    Можешь сделать в корне проекта файл ".env" (именно такое       название), и попробовать сделать штуку, которая будет  читать этот файл, называется — файл окружения.
    Либо через settings.conf, будет читать conf файл
    


- обёртка в commands.h для строк 60-64, 70-73
- команды от сервера




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









/////////////////////////////






Только я не знаю как там с глобальными переменными и Singleton в c++, там у них какие-то приколы с этим были, гугли или спрашивай у нейронки, как сейчас делают у вас там с общей переменной настроек
