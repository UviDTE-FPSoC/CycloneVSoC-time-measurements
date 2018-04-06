#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__

//Select one the size of the bridges
//#define BRIDGES_32BIT
//#define BRIDGES_64BIT
#define BRIDGES_128BIT

//Select writing HPS (WR) or reading the HPS (RD)
#define WR_HPS
//#define RD_HPS
#ifdef WR_HPS
	//When writing to HPS the FPGA should read from a constant address in
	//FPGA-OCR because OCR size is only 1kB and it would overflow otherwise
	#define FPGA_DMAC_CONSTANT_ADDR FPGA_DMA_READ_CONSTANT_ADDR
	//source and destiny of FPGA DMA transfers as seen by processor
	#define DMA_SRC_UP fpga_ocr_addr //fpga-ocrs
	#define DMA_DST_UP data //cpu buffers
	//source and destiny of FPGA DMA transfers as seen by DMAC
	#define DMA_SRC_DMAC fpga_ocr_dmac //fpga-ocrs
	#define DMA_DST_DMAC data_dmac //cpu buffers
#endif
#ifdef RD_HPS
	//When writing to HPS the FPGA should read from a constant address in
	//FPGA-OCR because OCR size is only 1kB and it would overflow otherwise
	#define FPGA_DMAC_CONSTANT_ADDR FPGA_DMA_WRITE_CONSTANT_ADDR
	//source and destiny of FPGA DMA transfers
	#define DMA_SRC_UP data //cpu buffers
	#define DMA_DST_UP fpga_ocr_addr //fpga-ocrs
	//source and destiny of FPGA DMA transfers as seen by DMAC
	#define DMA_SRC_DMAC data_dmac //cpu buffers
	#define DMA_DST_DMAC fpga_ocr_dmac //fpga-ocrs
#endif

#include "hwlib.h"
#include "socal/socal.h"
#include "socal/hps.h"
#include "hps_0_FPGA-HPS.h" //qsys addresses of the DMACs and FPGA-OCRs

//HPS-FPGA bridge base
#define HF_BRIDGE_BASE (void*)0xC0000000
//Create cleaner macros for DMACs and FPGA-OCRs in the FPGA
#define DMA_FPGA_HPS_ADDR				DMA_FPGA_HPS_BASE
#define DMA_FPGA_SDRAMC0_ADDR   DMA_FPGA_SDRAM_0_BASE
#define DMA_FPGA_SDRAMC1_ADDR   DMA_FPGA_SDRAM_1_BASE
#define DMA_FPGA_SDRAMC2_ADDR		DMA_FPGA_SDRAM_2_BASE
#define DMA_FPGA_SDRAMC3_ADDR		DMA_FPGA_SDRAM_3_BASE
#define OCR_FPGA_HPS_ADDR				OCR_FPGA_HPS_BASE
#define OCR_FPGA_SDRAMC0_ADDR		OCR_FPGA_SDRAM_0_BASE
#define OCR_FPGA_SDRAMC1_ADDR		OCR_FPGA_SDRAM_1_BASE
#define OCR_FPGA_SDRAMC2_ADDR		OCR_FPGA_SDRAM_2_BASE
#define OCR_FPGA_SDRAMC3_ADDR		OCR_FPGA_SDRAM_3_BASE

//Size of the FPGA-OCRs
#define FPGA_OCR_SIZE  OCR_FPGA_SDRAM_0_SPAN

//define some macros depending on the situation
#ifdef BRIDGES_32BIT
	#define NUM_OF_DMACS 5
	#define NUM_OF_FPGA_OCR 5
	#define TRANSFER_WORD_SIZE FPGA_DMA_WORD_TRANSFERS
#endif

#ifdef BRIDGES_64BIT
	#define NUM_OF_DMACS 5
	#define NUM_OF_FPGA_OCR 5
	#define TRANSFER_WORD_SIZE FPGA_DMA_DOUBLEWORD_TRANSFERS
#endif

#ifdef BRIDGES_128BIT
	#define NUM_OF_DMACS 3
	#define NUM_OF_FPGA_OCR 3
	#define TRANSFER_WORD_SIZE FPGA_DMA_QUADWORD_TRANSFERS
#endif

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

#define REP_TESTS 10 //repetitions of every time measurement
#define CLK_REP_TESTS 1000

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
