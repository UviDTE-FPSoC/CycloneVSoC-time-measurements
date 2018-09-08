#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__

//#define PRINT_TRANSFER_DETAILS

//Select one the size of the bridges
//#define BRIDGES_32BIT
//#define BRIDGES_64BIT
#define BRIDGES_128BIT

//Select writing HPS (WR) or reading the HPS (RD)
#define WR_HPS
//#define RD_HPS

//Limit the F2H tests to MAX_SIZE_F2H_TESTS
//#define LIMIT_F2H_TESTS
#define NUMBER_DATA_SIZES_TEST  16

//What to test
#define TEST_ALL_COMBINATIONS
//#define TEST_ONLY_F2S_BRIDGES
//#define TEST_F2H_AND_ALL_BRIDGES

#include "hps_0_FPGA-HPS.h" //qsys addresses of the DMACs and FPGA-OCRs

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

//Constants to do mmap and get access to FPGA and HPS peripherals
#define HPS_FPGA_BRIDGE_BASE 0xC0000000
#define MMAP_BASE ( HPS_FPGA_BRIDGE_BASE )
#define MMAP_SPAN ( 0x40000000 )
#define MMAP_MASK ( MMAP_SPAN - 1 )

//Size of the FPGA-OCRs
#define FPGA_OCR_SIZE  OCR_FPGA_SDRAM_0_SPAN

//GPIO to change AXI AXI_SIGNALS
#define GPIO_QSYS_ADDRESS PIO_0_BASE

//define some macros depending on the situation
#ifdef BRIDGES_32BIT
	#define NUM_OF_DMACS 5
	#define FIRST_FPGA_OCR_CHECK 0
	#define LAST_FPGA_OCR_CHECK 4
	#define TRANSFER_WORD_SIZE FPGA_DMA_WORD_TRANSFERS
	#define MIN_TRANSFER_SIZE 4 //Bytes
#endif

#ifdef BRIDGES_64BIT
	#define NUM_OF_DMACS 5
	#define FIRST_FPGA_OCR_CHECK 0
	#define LAST_FPGA_OCR_CHECK 4
	#define TRANSFER_WORD_SIZE FPGA_DMA_DOUBLEWORD_TRANSFERS
	#define MIN_TRANSFER_SIZE 8 //Bytes
#endif

#ifdef BRIDGES_128BIT
	#define NUM_OF_DMACS 3
	#define FIRST_FPGA_OCR_CHECK (0)
	#define LAST_FPGA_OCR_CHECK (2)
	#define TRANSFER_WORD_SIZE FPGA_DMA_QUADWORD_TRANSFERS
	#define MIN_TRANSFER_SIZE 16 //Bytes
#endif


#define REP_TESTS 100 //repetitions of every time measurement
#define CLK_REP_TESTS 1000

//Uncomment to generate dummy traffic in CPU memory to pollute cache and
//slowdown transfer through ACP
//#define GENERATE_DUMMY_TRAFFIC_IN_CACHE

#endif //__CONFIGURATION_H__
