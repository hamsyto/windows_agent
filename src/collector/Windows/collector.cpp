// windows/collector.cpp

#include "collector.h"

#include <windows.h>
// Windows API
#include <VersionHelpers.h>
#include <winioctl.h>

#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

#include "helpers.h"  // содержит FillDiskInfo, GetCPUUsage и т.д.

using namespace std;

WindowsCollector::WindowsCollector(const Settings& settings) {
  settings_ = settings;
}

vector<Disk> WindowsCollector::GetDisks() {
  vector<Disk> disks;
  DWORD drives = GetLogicalDrives();

  for (char letter = 'A'; letter <= 'Z'; ++letter) {
    if (!(drives & (1U << (letter - 'A')))) continue;

    string root = string(1, letter) + ":\\";
    if (GetDriveTypeA(root.c_str()) != DRIVE_FIXED) continue;

    int diskIndex = GetPhysicalDiskIndexForDriveLetter(letter);
    if (diskIndex < 0) continue;

    auto info = FillDiskInfo(diskIndex, root);
    disks.push_back(info);
  }
  return disks;
}

RAM WindowsCollector::GetRam() {
  RAM ram = {};
  MEMORYSTATUSEX m{};
  m.dwLength = sizeof(m);
  ULONGLONG installedKB = 0;

  if (GetPhysicallyInstalledSystemMemory(&installedKB)) {
    ram.total = installedKB * 1024;
  }
  if (GlobalMemoryStatusEx(&m)) {
    if (ram.total == 0) ram.total = m.ullTotalPhys;
    ram.used = ram.total - m.ullAvailPhys;
  }

  ram.total = round(ram.total / (1024.0 * 1024.0) * 100.0) / 100.0;
  ram.used = round(ram.used / (1024.0 * 1024.0) * 100.0) / 100.0;

  return ram;
}

CPU WindowsCollector::GetCpu() {
  CPU cpu = {};
  SYSTEM_INFO sys_info;
  GetSystemInfo(&sys_info);
  cpu.cores = sys_info.dwNumberOfProcessors;

  double usage1 = GetCPUUsage();  // инициализация
  Sleep(1000);
  cpu.usage = GetCPUUsage();
  cpu.usage = round(cpu.usage * 100.0) / 100.0;
  return cpu;
}

string get_dns_fqdn_hostname() {
  DWORD size = 0;
  if (!GetComputerNameExA(ComputerNameDnsFullyQualified, nullptr, &size)) {
    if (GetLastError() != ERROR_MORE_DATA) {
      return "";
    }
  }

  string hostname(size, '\0');
  if (GetComputerNameExA(ComputerNameDnsFullyQualified, &hostname[0], &size)) {
    hostname.resize(size);
    return hostname;
  }
  return "";
}

OS WindowsCollector::GetOs() {
  OS osys = {};
  osys.hostname = get_dns_fqdn_hostname();
  osys.domain = GetComputerDomainOrWorkgroup();
  osys.version = GetOsVersionName();

  FILETIME time;
  GetSystemTimeAsFileTime(&time);
  ULARGE_INTEGER ull;
  ull.LowPart = time.dwLowDateTime;
  ull.HighPart = time.dwHighDateTime;

  const ULONGLONG UNIX_EPOCH_DIFF = 116444736000000000ULL;
  const ULONGLONG HUNDRED_NS_PER_SECOND = 10000000ULL;
  ULONGLONG unixTime = (ull.QuadPart - UNIX_EPOCH_DIFF) / HUNDRED_NS_PER_SECOND;
  osys.timestamp = static_cast<int>(unixTime);

  return osys;
}

Hardware WindowsCollector::GetHardware() {
  Hardware hardware = {};
  hardware.bios = GetBiosInfo();
  hardware.cpu = GetCpuBrand();
  hardware.mac = getMacAddresses();
  hardware.video = GetVideoAdapters();
  return hardware;
}

int WindowsCollector::GetPing() {
  DWORD avg = GetAveragePing(settings_.ip_server.c_str(), 4);
  return (avg > 0) ? static_cast<int>(avg) : 0;
}

Payload WindowsCollector::GetPayload() {
  Payload payload = {};
  payload.disks = GetDisks();
  payload.ram = GetRam();
  payload.cpu = GetCpu();
  payload.system = GetOs();
  payload.hardware = GetHardware();
  payload.ping = GetPing();
  return payload;
}
