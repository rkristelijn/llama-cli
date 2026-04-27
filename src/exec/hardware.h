#ifndef HARDWARE_H
#define HARDWARE_H

#include <string>

/// Hardware specifications
struct HardwareInfo {
  long ram_gb = 0;   ///< Total RAM in GB
  long vram_gb = 0;  ///< Estimated usable VRAM in GB
  std::string cpu;   ///< CPU brand string
  std::string gpu;   ///< GPU description
};
HardwareInfo detect_hardware();

#endif
