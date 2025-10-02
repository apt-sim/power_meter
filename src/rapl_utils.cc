#include "rapl_utils.hh"
#include "msr_reader.hh"

#include <stdlib.h>
#include <string.h>
#include <chrono>

using namespace rapl_utils;

// Global variable definitions
namespace rapl_utils
{
  // MSR value arrays
  unsigned long long INTEL_MSR_RAPL_POWER_UNIT_VALUES[INTEL_MSR_RAPL_POWER_UNIT_NUMFIELDS];
  unsigned long long AMD_MSR_RAPL_POWER_UNIT_VALUES[AMD_MSR_RAPL_POWER_UNIT_NUMFIELDS];
  unsigned long long INTEL_MSR_PKG_ENERGY_STATUS_VALUES[INTEL_MSR_PKG_ENERGY_STATUS_NUMFIELDS];
  unsigned long long AMD_MSR_PKG_ENERGY_STATUS_VALUES[AMD_MSR_PKG_ENERGY_STATUS_NUMFIELDS];
  unsigned long long INTEL_MSR_PP0_ENERGY_STATUS_VALUES[INTEL_MSR_PP0_ENERGY_STATUS_NUMFIELDS];
  unsigned long long AMD_MSR_CORE_ENERGY_STATUS_VALUES[AMD_MSR_CORE_ENERGY_STATUS_NUMFIELDS];
  unsigned long long INTEL_MSR_PKG_POWER_INFO_VALUES[INTEL_MSR_PKG_POWER_INFO_NUMFIELDS];

  // Energy measurement variables
  float power_increment{0};
  float energy_increment{0};
  float time_increment{0};
  float energy_counter_max{0};

  int numa_nodes{0};
  std::unique_ptr<int[]> first_node_core;
  int numcores{0};
  int vendor_id{-1};
}

//////////////////////////////////////////////////////////////////////
//						            UTILITY FUNCTIONS
//////////////////////////////////////////////////////////////////////

/*
Inititialize the increment variables. Detect if the machine has several NUMA
nodes and processors, in case it does, it will initialize the variables for each
one (In case the CPUs are different).

This function assumes there is a NUMA node per CPU, and a maximum of 8 CPUs per
motherboard

This function assumes that there may be different CPU models in each socket, but
that both CPUs will have the same reporting precissions for RAPL values. This is
probably a safe assumption since valid CPU combinations will use very similar
CPUs.
*/
int rapl_utils::init()
{
  // We need to ID the CPU vendor as the MSR registers use different adresses on Intel/AMD
  int vendor_id_str;
  __asm__("cpuid" : "=c"(vendor_id_str) : /* Output the contents of ECX to vendor_id it will be "ntel" for Intel and "cAMD" for AMD
                                             0x6c65746e - "letn" as the processor is little-endian
                                             0x444d4163 - "DMAc"*/
          "a"(0) :                        // With EAX=0 CPUID returns the vendor Id string on EBX, ECX and EDX, ECX contains the last 4 characters
          "ebx", "edx");                  // CPUID clobbers EBX and EDX in addition to EAX and ECX
  /*
  and: https://www.felixcloutier.com/x86/cpuid
  */

  switch (vendor_id_str)
  {
  case 0x6c65746e:
    vendor_id = VENDOR_ID::INTEL;
    printf("POWER METER: CPU Vendor ID: Intel\n");
  case 0x444d4163:
    vendor_id = VENDOR_ID::AMD;
    printf("POWER METER: CPU Vendor ID: AMD\n");
    break;
  default:
    fprintf(stderr, "ERROR: CPU Vendor ID not recognised\n");
    return 1;
  }

  // Get the number of NUMA nodes. This file contains a list of node IDs
  // separated by "-". The length in characters of the file will be 2 for 1 node
  // (0 + \n), and increase by 2 for each succesive node
  FILE *nodes = fopen("/sys/devices/system/node/online", "r");
  char *nodelist = fgets((char *)malloc(16 * sizeof(char)), 16, nodes);
  fclose(nodes);
  numa_nodes = (int)strlen(nodelist) / 2;
  free(nodelist);

  // Allocate space for the variables of each node
  first_node_core = std::make_unique<int[]>(numa_nodes);

  // Read the values for each node
  for (int i = 0; i < numa_nodes; i++)
  {
    char filename[64];
    snprintf(filename, 64, "/sys/devices/system/node/node%d/cpulist", i);
    FILE *cpulist = fopen(filename, "r");
    // Reads the file and gets the first token (The id of the first core in the
    // NUMA node)
    first_node_core[i] = atoi(
        strtok(fgets((char *)malloc(16 * sizeof(char)), 16, cpulist), "-"));
    fclose(cpulist);
  }

  // Get the total number of cores, gets the number of the last online
  // core (should be all cores in the system), and adds 1
  FILE *onlinecores = fopen("/sys/devices/system/cpu/online", "r");
  numcores =
      atoi(strrchr(fgets((char *)malloc(16 * sizeof(char)), 16, onlinecores),
                   '-') +
           1) +
      1;
  fclose(onlinecores);

  if (vendor_id == VENDOR_ID::INTEL)
  {
    read_INTEL_MSR_RAPL_POWER_UNIT(0, INTEL_MSR_RAPL_POWER_UNIT_VALUES);
    power_increment =
        1 / (float)(1 << (unsigned int)INTEL_MSR_RAPL_POWER_UNIT_VALUES[0]);
    energy_increment =
        1 / (float)(1 << (unsigned int)INTEL_MSR_RAPL_POWER_UNIT_VALUES[1]);
    time_increment =
        1 / (float)(1 << (unsigned int)INTEL_MSR_RAPL_POWER_UNIT_VALUES[2]);
  }
  else if (vendor_id == VENDOR_ID::AMD)
  {
    read_AMD_MSR_RAPL_POWER_UNIT(0, AMD_MSR_RAPL_POWER_UNIT_VALUES);
    power_increment =
        1 / (float)(1 << (unsigned int)AMD_MSR_RAPL_POWER_UNIT_VALUES[0]);
    energy_increment =
        1 / (float)(1 << (unsigned int)AMD_MSR_RAPL_POWER_UNIT_VALUES[1]);
    time_increment =
        1 / (float)(1 << (unsigned int)AMD_MSR_RAPL_POWER_UNIT_VALUES[2]);
  }

  // The maximum value of the energy counter is 2^32, stored here in joules
  energy_counter_max = ((long)1U << 32) * energy_increment;

  printf("POWER METER: Number of NUMA nodes detected: %d\n", numa_nodes);

  return 0;
}

float rapl_utils::get_node_energy(int node, int domain)
{
  switch (domain)
  {
  // Package
  case 0:
    if (vendor_id == VENDOR_ID::INTEL)
    {
      read_INTEL_MSR_PKG_ENERGY_STATUS(first_node_core[node], INTEL_MSR_PKG_ENERGY_STATUS_VALUES);
      return (float)INTEL_MSR_PKG_ENERGY_STATUS_VALUES[0] * energy_increment;
    }
    else
    {
      read_AMD_MSR_PKG_ENERGY_STATUS(first_node_core[node], AMD_MSR_PKG_ENERGY_STATUS_VALUES);
      return (float)AMD_MSR_PKG_ENERGY_STATUS_VALUES[0] * energy_increment;
    }
    break;
  // Cores
  case 1:
    if (vendor_id == VENDOR_ID::INTEL)
    {
      read_INTEL_MSR_PP0_ENERGY_STATUS(first_node_core[node], INTEL_MSR_PP0_ENERGY_STATUS_VALUES);
      return (float)INTEL_MSR_PP0_ENERGY_STATUS_VALUES[0] * energy_increment;
    }
    else
    {
      read_AMD_MSR_CORE_ENERGY_STATUS(first_node_core[node], AMD_MSR_CORE_ENERGY_STATUS_VALUES);
      return (float)AMD_MSR_CORE_ENERGY_STATUS_VALUES[0] * energy_increment;
    }
    break;
  // Uncore
  case 2:
    fprintf(stderr, "Reading from RAPL's Uncore domain not yet implemented");
    return 0;
    break;
  // DRAM
  case 3:
    fprintf(stderr, "Reading from RAPL's DRAM domain not yet implemented");
    return 0;
    break;
  default:
    fprintf(stderr, "Bad RAPL domain (%d). Supported domains are 0-3", domain);
    return 0;
    break;
  }
}

void rapl_utils::update_aux_data(EnergyAux &data, int domain)
{
  for (int i = 0; i < numa_nodes; i++)
  {
    data.energy[i] = get_node_energy(i, domain);
  }
  clock_gettime(CLOCK_REALTIME, &data.time);
}

void rapl_utils::update_package_energy(EnergyAux &data) { update_aux_data(data, 0); }

void rapl_utils::update_cores_energy(EnergyAux &data) { update_aux_data(data, 1); }

float rapl_utils::get_energy_diff(const float *current_energy, const float *previous_energy)
{
  float energy_diff = 0;
  for (int i = 0; i < numa_nodes; i++)
  {
    float node_energy_diff = (float)(current_energy[i] - previous_energy[i]);
    /*
    If the energy counter has wrapped around for this node, we need to add the
    value before wrapping around to the diff. This is 2^32 per Intel's
    specification
    */
    if (node_energy_diff < 0)
    {
      node_energy_diff += energy_counter_max;
    }
    energy_diff += node_energy_diff;
  }
  return energy_diff;
}

float rapl_utils::get_power(const EnergyAux &previous_data, const EnergyAux &current_data)
{
  double time_diff =
      (double)(current_data.time.tv_sec - previous_data.time.tv_sec) +
      ((double)(current_data.time.tv_nsec - previous_data.time.tv_nsec) / 1E9);
  float energy_diff = get_energy_diff(current_data.energy, previous_data.energy);

  // Power = Energy delta in Joules / Time delta in seconds
  float power = (float)(energy_diff / time_diff);

  return power;
}

void rapl_utils::update_energy_data(EnergyData &output_data, const EnergyAux &previous_data, const EnergyAux &current_data)
{
  // Store average power consumption for this interval
  output_data.power = get_power(previous_data, current_data);
  // Get energy delta taking into account the counter wraparound
  float energy_diff = get_energy_diff(current_data.energy, previous_data.energy);
  // Store energy consumed during this interval
  output_data.energy = energy_diff;
  // Update the total energy consumed by this node
  output_data.total_energy += energy_diff;
}

float rapl_utils::get_processor_tdp()
{
  if (!vendor_id == VENDOR_ID::INTEL)
  {
    fprintf(stderr, "POWER METER: ERROR: get_processor_tdp() only works with Intel CPUs\n");
    return 0;
  }

  float total_tdp = 0;

  for (int i = 0; i < numa_nodes; i++)
  {
    read_INTEL_MSR_PKG_POWER_INFO(first_node_core[i], INTEL_MSR_PKG_POWER_INFO_VALUES);
    total_tdp += (float)INTEL_MSR_PKG_POWER_INFO_VALUES[0];
  }

  return total_tdp * power_increment;
}

//////////////////////////////////////////////////////////////////////
//						          READING MSR FIELDS
//////////////////////////////////////////////////////////////////////

void rapl_utils::read_INTEL_MSR_RAPL_POWER_UNIT(int core, unsigned long long *output)
{
  read_msr_fields(
      core, INTEL_MSR_RAPL_POWER_UNIT, INTEL_MSR_RAPL_POWER_UNIT_NUMFIELDS,
      INTEL_MSR_RAPL_POWER_UNIT_OFFSETS, INTEL_MSR_RAPL_POWER_UNIT_SIZES,
      output);
}

void rapl_utils::read_INTEL_MSR_PKG_ENERGY_STATUS(int core, unsigned long long *output)
{
  read_msr_fields(
      core, INTEL_MSR_PKG_ENERGY_STATUS, INTEL_MSR_PKG_ENERGY_STATUS_NUMFIELDS,
      INTEL_MSR_PKG_ENERGY_STATUS_OFFSETS, INTEL_MSR_PKG_ENERGY_STATUS_SIZES,
      output);
}

void rapl_utils::read_INTEL_MSR_PP0_ENERGY_STATUS(int core, unsigned long long *output)
{
  read_msr_fields(
      core, INTEL_MSR_PP0_ENERGY_STATUS, INTEL_MSR_PP0_ENERGY_STATUS_NUMFIELDS,
      INTEL_MSR_PP0_ENERGY_STATUS_OFFSETS, INTEL_MSR_PP0_ENERGY_STATUS_SIZES,
      output);
}

void rapl_utils::read_INTEL_MSR_PKG_POWER_INFO(int core, unsigned long long *output)
{
  read_msr_fields(
      core, INTEL_MSR_PKG_POWER_INFO, INTEL_MSR_PKG_POWER_INFO_NUMFIELDS,
      INTEL_MSR_PKG_POWER_INFO_OFFSETS, INTEL_MSR_PKG_POWER_INFO_SIZES,
      output);
}

void rapl_utils::read_AMD_MSR_RAPL_POWER_UNIT(int core, unsigned long long *output)
{
  read_msr_fields(
      core, AMD_MSR_RAPL_POWER_UNIT, AMD_MSR_RAPL_POWER_UNIT_NUMFIELDS,
      AMD_MSR_RAPL_POWER_UNIT_OFFSETS, AMD_MSR_RAPL_POWER_UNIT_SIZES,
      output);
}

void rapl_utils::read_AMD_MSR_PKG_ENERGY_STATUS(int core, unsigned long long *output)
{
  read_msr_fields(
      core, AMD_MSR_PKG_ENERGY_STATUS, AMD_MSR_PKG_ENERGY_STATUS_NUMFIELDS,
      AMD_MSR_PKG_ENERGY_STATUS_OFFSETS, AMD_MSR_PKG_ENERGY_STATUS_SIZES,
      output);
}

void rapl_utils::read_AMD_MSR_CORE_ENERGY_STATUS(int core, unsigned long long *output)
{
  read_msr_fields(
      core, AMD_MSR_CORE_ENERGY_STATUS, AMD_MSR_CORE_ENERGY_STATUS_NUMFIELDS,
      AMD_MSR_CORE_ENERGY_STATUS_OFFSETS, AMD_MSR_CORE_ENERGY_STATUS_SIZES,
      output);
}