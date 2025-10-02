#ifndef RAPL_CONST_HH
#define RAPL_CONST_HH

// This file defines the constants needed to read each MSR

namespace rapl_utils
{
#define INTEL_MSR_RAPL_POWER_UNIT 0x606
#define INTEL_MSR_RAPL_POWER_UNIT_NUMFIELDS 3
    inline const char *INTEL_MSR_RAPL_POWER_UNIT_NAMES[] = {
        "Power Units", "Energy Status Units", "Time Units"};
    inline const unsigned int INTEL_MSR_RAPL_POWER_UNIT_SIZES[] = {4, 5, 4};
    inline const unsigned int INTEL_MSR_RAPL_POWER_UNIT_OFFSETS[] = {0, 8, 16};

#define AMD_MSR_RAPL_POWER_UNIT 0xC0010299
#define AMD_MSR_RAPL_POWER_UNIT_NUMFIELDS 3
    inline const char *AMD_MSR_RAPL_POWER_UNIT_NAMES[] = {
        "Power Units", "Energy Status Units", "Time Units"};
    inline const unsigned int AMD_MSR_RAPL_POWER_UNIT_SIZES[] = {4, 5, 4};
    inline const unsigned int AMD_MSR_RAPL_POWER_UNIT_OFFSETS[] = {0, 8, 16};

#define INTEL_MSR_PKG_ENERGY_STATUS 0x611
#define INTEL_MSR_PKG_ENERGY_STATUS_NUMFIELDS 1
    inline const char *INTEL_MSR_PKG_ENERGY_STATUS_NAMES[] = {"Total Energy Consumed"};
    inline const unsigned int INTEL_MSR_PKG_ENERGY_STATUS_SIZES[] = {32};
    inline const unsigned int INTEL_MSR_PKG_ENERGY_STATUS_OFFSETS[] = {0};

#define AMD_MSR_PKG_ENERGY_STATUS 0xC001029B
#define AMD_MSR_PKG_ENERGY_STATUS_NUMFIELDS 1
    inline const char *AMD_MSR_PKG_ENERGY_STATUS_NAMES[] = {"Total Energy Consumed"};
    inline const unsigned int AMD_MSR_PKG_ENERGY_STATUS_SIZES[] = {32};
    inline const unsigned int AMD_MSR_PKG_ENERGY_STATUS_OFFSETS[] = {0};

// The cores RAPL domain is called PP0 on Intel CPUs and Core on AMD
#define INTEL_MSR_PP0_ENERGY_STATUS 0x639
#define INTEL_MSR_PP0_ENERGY_STATUS_NUMFIELDS 1
    inline const char *INTEL_MSR_PP0_ENERGY_STATUS_NAMES[] = {"Total Energy Consumed"};
    inline const unsigned int INTEL_MSR_PP0_ENERGY_STATUS_SIZES[] = {32};
    inline const unsigned int INTEL_MSR_PP0_ENERGY_STATUS_OFFSETS[] = {0};

#define AMD_MSR_CORE_ENERGY_STATUS 0xC001029A
#define AMD_MSR_CORE_ENERGY_STATUS_NUMFIELDS 1
    inline const char *AMD_MSR_CORE_ENERGY_STATUS_NAMES[] = {"Total Energy Consumed"};
    inline const unsigned int AMD_MSR_CORE_ENERGY_STATUS_SIZES[] = {32};
    inline const unsigned int AMD_MSR_CORE_ENERGY_STATUS_OFFSETS[] = {0};

#define INTEL_MSR_PKG_POWER_INFO 0x614
#define INTEL_MSR_PKG_POWER_INFO_NUMFIELDS 4
    inline const char *INTEL_MSR_PKG_POWER_INFO_NAMES[] = {
        "Thermal Spec Power", "Minimum Power", "Maximum Power",
        "Maximum Time Window"};
    inline const unsigned int INTEL_MSR_PKG_POWER_INFO_SIZES[] = {15, 15, 15, 6};
    inline const unsigned int INTEL_MSR_PKG_POWER_INFO_OFFSETS[] = {0, 16, 32, 48};
} // namespace rapl_utils

#endif