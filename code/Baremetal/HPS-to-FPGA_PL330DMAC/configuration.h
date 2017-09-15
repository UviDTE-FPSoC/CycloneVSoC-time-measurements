#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__
/*
This file indicates where On-Chip RAM is connected:
-On chip RAM connected to 32 bits LIGHTWEIGHT bus
-On chip RAM connected to 32 bits HPS to FPGA High Performance Bridge
-On chip RAM connected to 64 bits HPS to FPGA High Performance Bridge
-On chip RAM connected to 128 bits HPS to FPGA High Performance Bridge
*/

//Select one of the 4 situations
//#define ON_CHIP_RAM_ON_LIGHTWEIGHT
//#define ON_CHIP_RAM_ON_HFBRIDGE32
//#define ON_CHIP_RAM_ON_HFBRIDGE64
#define ON_CHIP_RAM_ON_HFBRIDGE128

#include "hwlib.h"
#include "socal/socal.h"
#include "socal/hps.h"
#include "socal/alt_gpio.h"

//define some variable types
typedef uint32_t	uint32_soc;
typedef uint64_t	uint64_soc;
typedef uint64_t  uint128_soc; //max len data size in Cortex A9 is 64-bit

//define some macros depending on the situation
#ifdef ON_CHIP_RAM_ON_LIGHTWEIGHT
	//import some constants from hps_0.h
	#include "hps_0_LW.h"
	#define ON_CHIP_MEMORY_BASE ONCHIP_MEMORY2_0_BASE
	#define ON_CHIP_MEMORY_SPAN ONCHIP_MEMORY2_0_SPAN //size in bytes of memory
	#define ONCHIP_MEMORY_END ONCHIP_MEMORY2_0_END
	//define variable type depending on on-chip RAM data width
	#define UINT_SOC uint32_soc
	//constants to write and read memory
	#define BYTES_DATA_BUS 4
	#define MEMORY_SIZE_IN_WORDS (ON_CHIP_MEMORY_SPAN/BYTES_DATA_BUS) //number of 32bit words
	//macros to define hardware directions
 	#define BRIDGE_BASE (ALT_LWFPGASLVS_OFST) //base of lightweight bridge
#endif //ON_CHIP_RAM_ON_LIGHTWEIGHT

#ifdef ON_CHIP_RAM_ON_HFBRIDGE32
	//import some constants from hps_0.h
	#include "hps_0_HPS-FPGA-32.h"
	//#include "hps_0_LW.h"
	#define ON_CHIP_MEMORY_BASE ONCHIP_MEMORY2_0_BASE
	#define ON_CHIP_MEMORY_SPAN ONCHIP_MEMORY2_0_SPAN //size in bytes of memory
	#define ONCHIP_MEMORY_END ONCHIP_MEMORY2_0_END
	//define variable type depending on on-chip RAM data width
	#define UINT_SOC uint32_soc
	//constants to write and read memory
	#define BYTES_DATA_BUS 4
	#define MEMORY_SIZE_IN_WORDS (ON_CHIP_MEMORY_SPAN/BYTES_DATA_BUS) //number of 32bit words
	//macros to define hardware directions
 	#define BRIDGE_BASE 0xC0000000 //default start for HPS-FPGA High performance bridge
#endif //ON_CHIP_RAM_ON_HFBRIDGE32

#ifdef ON_CHIP_RAM_ON_HFBRIDGE64
	//import some constants from hps_0.h
	#include "hps_0_HPS-FPGA-64.h"
	//#include "hps_0_LW.h"
	#define ON_CHIP_MEMORY_BASE ONCHIP_MEMORY2_0_BASE
	#define ON_CHIP_MEMORY_SPAN ONCHIP_MEMORY2_0_SPAN //size in bytes of memory
	#define ONCHIP_MEMORY_END ONCHIP_MEMORY2_0_END
	//define variable type depending on on-chip RAM data width
	#define UINT_SOC uint64_soc
	//constants to write and read memory
	#define BYTES_DATA_BUS 8
	#define MEMORY_SIZE_IN_WORDS (ON_CHIP_MEMORY_SPAN/BYTES_DATA_BUS) //number of 64bit words
	//macros to define hardware directions
 	#define BRIDGE_BASE 0xC0000000 //default start for HPS-FPGA High performance bridge
#endif //ON_CHIP_RAM_ON_HFBRIDGE64


#ifdef ON_CHIP_RAM_ON_HFBRIDGE128
	//import some constants from hps_0.h
	#include "hps_0_HPS-FPGA-128.h"
	//#include "hps_0_LW.h"
	#define ON_CHIP_MEMORY_BASE ONCHIP_MEMORY2_0_BASE
	#define ON_CHIP_MEMORY_SPAN ONCHIP_MEMORY2_0_SPAN //size in bytes of memory
	#define ONCHIP_MEMORY_END ONCHIP_MEMORY2_0_END
	//define variable type depending on on-chip RAM data width
	#define UINT_SOC uint128_soc
	//constants to write and read memory
	#define BYTES_DATA_BUS 8
	#define MEMORY_SIZE_IN_WORDS (ON_CHIP_MEMORY_SPAN/BYTES_DATA_BUS) //number of 128bit words
	//macros to define hardware directions
 	#define BRIDGE_BASE 0xC0000000 //default start for HPS-FPGA High performance bridge
#endif //ON_CHIP_RAM_ON_HFBRIDGE128

//----------------------CACHE CONFIGURATION------------------------//
#define CACHE_CONFIG 9
/*Options for cache config (each config is added to the all previous ones):
0 no cache
(basic config and optimizations)
1 enable MMU
2 do 1 and initialize L2C
3 do 2 and enable SCU
4 do 3 and enable L1_I
5 do 4 and enable branch prediction
6 do 5 and enable L1_D
7 do 6 and enable L1 D side prefetch
8 do 7 and enable L2C
(special L2C-310 controller + Cortex A9 optimizations)
9 do 8 and enable L2 prefetch hint
10 do 9 and enable write full line zeros
11 do 10 and enable speculative linefills of L2 cache
12 do 11 and enable early BRESP
13 do 12 and store_buffer_limitation
*/

#define REP_TESTS 100 //repetitions of every time measurement
#define CLK_REP_TESTS 1000

//L2C LOCKDOWN STUDY MACROS
//enables repetition of experiments with different lockdown cofigurations
//1,4,7 ways locking are tested for CPU and ACP
//It only makes sense if cache is switched on
#if CACHE_CONFIG > 5
	//uncomment to permit lockdown study
	//#define EN_LOCKDOWN_STUDY
	#ifdef EN_LOCKDOWN_STUDY
		//LOCKDOWN STUDY OPTIONS:
		//Uncomment to lock only after transfer data is generated in cache by CPU
		#define LOCK_AFTER_CPU_GENERATES_TRANSFER_DATA
	#endif
#endif

//SDRAM CONTROLLER STUDY MACROS
//It only makes sense if cache is switched off when transfer is through SDRAMC
#if (CACHE_CONFIG == 0)
	//Uncomment to permit SDRAM Controller study. It uses different combinations
	//of SDRAM priority and round-robin weights to increase priority of
	//L3->SDRAMC port that PL330 DMAC uses to access memory.
	//#define EN_SDRAMC_STUDY
#endif

//Uncomment to generate dummy traffic in CPU memory to pollute cache and
//slowdown transfer through ACP
//#define GENERATE_DUMMY_TRAFFIC_IN_CACHE

#endif //__CONFIGURATION_H__
