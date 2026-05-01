/**
 * @file hardware_test.cpp
 * @brief Unit tests for hardware detection.
 */
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "exec/hardware.h"

#include <doctest/doctest.h>

SCENARIO ("detect_hardware returns valid info") {
  GIVEN ("the current machine") {
    HardwareInfo hw = detect_hardware();
    THEN ("RAM is detected (at least 1 GB)") {
      CHECK (hw.ram_gb >= 1)
        ;
    }
    THEN ("VRAM estimate is positive") {
      CHECK (hw.vram_gb > 0)
        ;
    }
#ifdef __APPLE__
    THEN ("CPU brand string is populated on macOS") {
      CHECK_FALSE (hw.cpu.empty())
        ;
    }
    THEN ("GPU reports unified memory on Apple Silicon") {
      CHECK (hw.gpu.find("Unified") != std::string::npos)
        ;
    }
    THEN ("VRAM is approximately 75% of RAM") {
      // Allow some rounding: vram should be between 50% and 80% of ram
      CHECK (hw.vram_gb >= hw.ram_gb / 2)
        ;
      CHECK (hw.vram_gb <= hw.ram_gb)
        ;
    }
#endif
  }
}

SCENARIO ("HardwareInfo struct defaults") {
  GIVEN ("a default-constructed HardwareInfo") {
    HardwareInfo hw;
    THEN ("all fields are zero/empty") {
      CHECK (hw.ram_gb == 0)
        ;
      CHECK (hw.vram_gb == 0)
        ;
      CHECK (hw.cpu.empty())
        ;
      CHECK (hw.gpu.empty())
        ;
    }
  }
}
