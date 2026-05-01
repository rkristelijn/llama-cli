/**
 * @file hardware.h
 * @brief Hardware detection for model auto-selection (RAM, VRAM, CPU, GPU).
 */

#ifndef HARDWARE_H
#define HARDWARE_H

#include <string>

/// Hardware specifications detected at runtime
struct HardwareInfo {
  long ram_gb = 0;   ///< Total RAM in GB
  long vram_gb = 0;  ///< Estimated usable VRAM in GB
  std::string cpu;   ///< CPU brand string
  std::string gpu;   ///< GPU description
};

/// Detect hardware specs (RAM, VRAM, CPU, GPU) for model selection
HardwareInfo detect_hardware();

#endif
