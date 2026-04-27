#include "hardware.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#ifdef __APPLE__
#include <sys/sysctl.h>
#include <sys/types.h>
#endif

HardwareInfo detect_hardware() {
  HardwareInfo hw;

#ifdef __APPLE__
  // RAM
  int64_t memsize = 0;
  size_t len = sizeof(memsize);
  if (sysctlbyname("hw.memsize", &memsize, &len, NULL, 0) == 0) {
    hw.ram_gb = memsize / (1024 * 1024 * 1024);
    // On Apple Silicon, VRAM is unified. We estimate ~75% can be used by GPU.
    hw.vram_gb = (hw.ram_gb * 3) / 4;
    hw.gpu = "Apple Unified Memory";
  }

  // CPU
  char cpu_model[256];
  len = sizeof(cpu_model);
  if (sysctlbyname("machdep.cpu.brand_string", cpu_model, &len, NULL, 0) == 0) {
    hw.cpu = cpu_model;
  }
#else
  // Linux Fallback
  std::ifstream meminfo("/proc/meminfo");
  if (meminfo.is_open()) {
    std::string line;
    while (std::getline(meminfo, line)) {
      if (line.rfind("MemTotal:", 0) == 0) {
        long total_kb = 0;
        std::stringstream ss(line.substr(9));
        ss >> total_kb;
        hw.ram_gb = total_kb / (1024 * 1024);
        break;
      }
    }
  }
  // Very basic GPU detection via lspci
  FILE* pipe = popen("lspci | grep -i 'VGA\\|3D' 2>/dev/null", "r");
  if (pipe) {
    char buffer[128];
    if (fgets(buffer, sizeof(buffer), pipe)) {
      hw.gpu = buffer;
    }
    pclose(pipe);
  }
  hw.vram_gb = 4;  // Default estimate
#endif

  return hw;
}
