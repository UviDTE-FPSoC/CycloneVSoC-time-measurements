#ifndef _CONFIGURATION_
#define _CONFIGURATION_

//Where the FPGA On-Chip RAM is connected?Select one of the 2 situations
//#define ON_CHIP_RAM_ON_LIGHTWEIGHT
#define ON_CHIP_RAM_ON_HFBRIDGE
//Constants to do mmap and get access to FPGA through HPS-FPGA bridge
#ifdef ON_CHIP_RAM_ON_HFBRIDGE
  #define HPS_FPGA_BRIDGE_BASE 0xC0000000 //Beginning of H2F bridge
  #define MMAP_BASE ( HPS_FPGA_BRIDGE_BASE )
  #define MMAP_SPAN ( 0x04000000 )
  #define MMAP_MASK ( MMAP_SPAN - 1 )
  #define ON_CHIP_MEMORY_BASE 0 //FPGA On-Chip RAM address relative to H2F bridge
#endif
//Consts to do mmap and get access to FPGA through Lightweight HPS-FPGA bridge
#ifdef ON_CHIP_RAM_ON_LIGHTWEIGHT
  #define HPS_FPGA_BRIDGE_BASE 0xFF200000 //Beginning of LW H2F bridge
  #define MMAP_BASE ( HPS_FPGA_BRIDGE_BASE )
  #define MMAP_BASE_SPAN ( 0x00080000 )
  #define MMAP_BASE_MASK ( MMAP_SPAN - 1 )
  #define ON_CHIP_MEMORY_BASE 0x40000 //FPGA On-Chip RAM address relative to LW H2F bridge
#endif

//Constants for the time experiments:
#define REP_TESTS 100 //repetitions of each time experiment
#define CLK_REP_TESTS 1000 //repetitions to get clock statistics
#define ON_CHIP_MEMORY_SPAN 262144 //FPGA On-Chip RAM size in Bytes
//DMA_BUFF_PADD: Physical address of the FPGA On-Chip RAM
#define DMA_BUFF_PADD (HPS_FPGA_BRIDGE_BASE + ON_CHIP_MEMORY_BASE)

//L2C LOCKDOWN STUDY MACROS
//enables repetition of experiments with different lockdown cofigurations
//1,4,7 ways locking are tested for CPU and ACP
//It only makes sense if cache is switched on
//#define EN_LOCKDOWN_STUDY
#ifdef EN_LOCKDOWN_STUDY
  //LOCKDOWN STUDY OPTIONS:
  //Uncomment to lock only after transfer data is generated in cache by CPU
  #define LOCK_AFTER_CPU_GENERATES_TRANSFER_DATA
  //Uncomment to generate dummy traffic in CPU memory to pollute cache and
  //slowdown transfer through ACP
  //#define GENERATE_DUMMY_TRAFFIC_IN_CACHE
#endif

#endif
