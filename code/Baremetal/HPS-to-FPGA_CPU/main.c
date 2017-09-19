#include <stdio.h> //printf
#include <string.h> //to use memcpy
#include <stdlib.h> // to use malloc and free
#include <inttypes.h>

#define soc_cv_av
#include "socal/socal.h"
#include "socal/hps.h"

//select configuration (bridge type and other experiment-related variables)
#include "configuration.h"
//high level API that permits to start cache with a single line
#include "cache_high_level_API.h"
//extract statistics from experiments (mean value, min, max, variance...)
#include "statistics.h"
//functions to init and control PMU as timer
#include "pmu.h"

/* enable semihosting with gcc by defining an __auto_semihosting symbol */
int __auto_semihosting;

int main(void)
{
	printf("---MEASURING HPS-FPGA BRIDGES SPEED IN BAREMETAL WITH CPU--\n\r\r");
	#ifdef ON_CHIP_RAM_ON_LIGHTWEIGHT
		printf("TESTING ON_CHIP_RAM_ON_LIGHTWEIGHT\n\r");
	#endif //ON_CHIP_RAM_ON_LIGHTWEIGHT
	#ifdef ON_CHIP_RAM_ON_HFBRIDGE32
		printf("TESTING ON_CHIP_RAM_ON_HFBRIDGE32\n\r");
	#endif //ON_CHIP_RAM_ON_HFBRIDGE32
	#ifdef ON_CHIP_RAM_ON_HFBRIDGE64
		printf("TESTING ON_CHIP_RAM_ON_HFBRIDGE64\n\r");
	#endif //ON_CHIP_RAM_ON_HFBRIDGE64
	#ifdef ON_CHIP_RAM_ON_HFBRIDGE128
		printf("TESTING ON_CHIP_RAM_ON_HFBRIDGE128\n\r");
	#endif //ON_CHIP_RAM_ON_HFBRIDGE128

	printf("Each measurement is repeated %d times\n\r", REP_TESTS);

	//------------------------VARIABLES INSTANTIATION-----------------------//
	//-------------Common variables------------//
	ALT_STATUS_CODE status = ALT_E_SUCCESS;

	int i, j, l; //vars for loops

	//cache configuration
	int cache_config = CACHE_CONFIG;

	//-------------Memory pointers------------//
	//FPGA On-Chip RAM
	void* FPGA_on_chip_RAM_addr = (void*)(BRIDGE_BASE + ON_CHIP_MEMORY_BASE);
	UINT_SOC* fpgaocr_ptr;//FPGA on chip RAM pointer
	//HPS On-Chip RAM
	void* hpsocr_addr = (void*) 0xFFFF0000;
	int* hpsocr_ptr;//HPS on chip RAM pointer
	int hpsocr_size = 65536/4;//HPS OCR, size in bytes/4=size in 32-bit words

	//----Save intermediate results in speed tests-------//
	unsigned long long int total_clk, min_clk, max_clk, var_clk, clk_overhead;

	unsigned long long int total_mcp_rd, min_mcp_rd, max_mcp_rd, var_mcp_rd;
	unsigned long long int total_mcp_wr, min_mcp_wr, max_mcp_wr, var_mcp_wr;

	//-----------some variables to define speed tests-------//
	//define data sizes
	int number_of_data_sizes = 21; //size of data_size[]
	int data_size [] =
	{2, 4, 8, 16, 32, 64, 128, 256, 512, 1024,
	2048, 4096, 8192, 16384, 32768,
	65536, 131072, 262144, 524288, 1048576,
	2097152}; //data size in bytes

	//-----------auxiliar variables to perform speed tests-------//
	char* data;
	int data_in_one_operation;//data to write or read from memory (bytes)
	int operation_loops; //number of loops to read/write all data from/to memory

	//Variables if For Loop Defined
	#ifdef FORLOOP
	int k;
	unsigned long long int total_dir_rd, min_dir_rd, max_dir_rd, var_dir_rd;
	unsigned long long int total_dir_wr, min_dir_wr, max_dir_wr, var_dir_wr;
	UINT_SOC* datai; //pointer to data
	int data_in_one_operation_w;//data to write or read from memory (words)
	int operation_w_loops; //number of loops to read/write all data from/to memory
	#endif

	//----------CHECKING HPS 64k ON-CHIP RAM-----------//
	//Check the memory
	hpsocr_ptr = (int*) hpsocr_addr;
	for (i=0; i<(hpsocr_size-1); i++)
	{
		*hpsocr_ptr = i;
		if (*hpsocr_ptr != i)
		{
			status = ALT_E_ERROR;
		}
		hpsocr_ptr++;
	}

	if (status == ALT_E_SUCCESS) printf("Check HPS On-Chip RAM OK\n\r");
	else
	{
		printf ("Error when checking HPS On-Chip RAM\n\r");
		return 1;
	}

	//-------------CHECKING FPGA 256kB ON-CHIP RAM-------------//

	//Check the memory
	fpgaocr_ptr = (UINT_SOC *)FPGA_on_chip_RAM_addr;
	for (i=4; i<(MEMORY_SIZE_IN_WORDS); i++)
	{
		*fpgaocr_ptr = i;
		if (*fpgaocr_ptr != i){
			status = ALT_E_ERROR;
			printf("i during error %d\n\r", i);
		}
		fpgaocr_ptr++;
	}
	if (status == ALT_E_SUCCESS) printf("Check FPGA On-Chip RAM OK\n\r");
	else
	{
		printf ("Error when checking FPGA On-Chip RAM\n\r");
		return 1;
	}
	//Reset all memory
	fpgaocr_ptr = (UINT_SOC *)FPGA_on_chip_RAM_addr;
	for (i=0; i<MEMORY_SIZE_IN_WORDS; i++)
	{
		*fpgaocr_ptr = 0;
		if (*fpgaocr_ptr != 0) status = ALT_E_ERROR;;
		fpgaocr_ptr++;
	}
	if (status == ALT_E_SUCCESS) printf("Reset FPGA On-Chip RAM OK\n\r");
	else
	{
		printf ("Error when resetting FPGA On-Chip RAM \n\r");
		return 1;
	}

	//----------CONFIGURING CACHE-----------//
	cache_configuration(cache_config);

	//--------------------TESTS USING PMU------------------------------//
	printf("\n\r------------STARTING TIME MEASUREMENTS------------\n\r");

	unsigned long long pmu_counter_ns;
	int overflow = 0;

	//-----------INITIALIZATION OF PMU AS TIMER------------//
	pmu_init_ns(800, 1); //Init PMU cycle counter, 800MHz source, freq. divider 1
	pmu_counter_enable();//Enable cycle counter inside PMU (it starts counting)
	float pmu_res = pmu_getres_ns();
	printf("PMU is used like timer with the following characteristics\n\r");
	printf("PMU cycle counter resolution is %f ns\n\r", pmu_res );
	printf("Measuring PMU counter overhead...\n\r");
	reset_cumulative(&total_clk, &min_clk, &max_clk, &var_clk);
	for(i = 0; i<CLK_REP_TESTS+2; i++)
	{
		pmu_counter_reset();
		overflow = pmu_counter_read_ns(&pmu_counter_ns);
		//printf("PMU counter (ns): %lld \n\r", pmu_counter_ns);
		if (overflow == 1)
		{
			printf("Cycle counter overflow!! Program ended\n\r");
			return 1;
		}
		if (i>=2) update_cumulative(&total_clk, &min_clk, &max_clk, &var_clk,
			0, pmu_counter_ns, 0);
			//(We erase two first measurements because they are different from the
	    //others. Reason:Branch prediction misses when entering for loop)
	}
	printf("PMU Cycle Timer Stats for %d consecutive reads\n\r", CLK_REP_TESTS);
	printf("Average, Minimum, Maximum, Variance\n\r");
	printf("%lld,%lld,%lld,%lld\n\r", clk_overhead = total_clk/CLK_REP_TESTS,
	 min_clk, max_clk, variance (var_clk , total_clk, CLK_REP_TESTS));

	//-----------MOVING DATA WITH PROCESSOR------------//
	printf("\n\r--MOVING DATA WITH THE PROCESSOR--\n\r");
	//----------Do the speed test (using memcpy)--------------//
	printf("\n\rStart memcpy test\n\r");

	printf("Data Size,Average DMA_WR,Min DMA_WR,Max DMA_WR,Variance DMA_WR");
	printf(",Average DMA_RD,Min DMA_RD,Max DMA_RD,Variance DMA_RD\n\r");

	for(i=0; i<number_of_data_sizes; i++)//for each data size
	{
		reset_cumulative( &total_mcp_wr, &min_mcp_wr, &max_mcp_wr, &var_mcp_wr);
		reset_cumulative( &total_mcp_rd, &min_mcp_rd, &max_mcp_rd, &var_mcp_rd);

		for(l = 0; l<REP_TESTS+2; l++)
		{
			//reserve the exact memory of the data size
			data = (char*) malloc(data_size[i]);
			if (data == 0)
			{
				printf("ERROR when calling malloc: Out of memory\n\r");
				return 1;
			}

			//save some content in data (for example: i)
			for (j=0; j<data_size[i]; j++) data[j] = i;

			//if data is bigger than on-chip size in bytes
			if (data_size[i]>ON_CHIP_MEMORY_SPAN)
			{
				data_in_one_operation = ON_CHIP_MEMORY_SPAN;
				operation_loops = data_size[i]/ON_CHIP_MEMORY_SPAN;
			}
			else
			{
				data_in_one_operation = data_size[i];
				operation_loops = 1;
			}

			//WRITE DATA TO ON-CHIP RAM

			pmu_counter_reset();

			for (j=0; j<operation_loops; j++)
				memcpy((void*) FPGA_on_chip_RAM_addr,
					(void *) (&(data[j*ON_CHIP_MEMORY_SPAN])),
					data_in_one_operation);

			overflow = pmu_counter_read_ns(&pmu_counter_ns);
			if (overflow == 1)
			{
				printf("Cycle counter overflow!! Program ended\n\r");
				return 1;
			}
			//printf("PMU counter (ns): %lld \n\r", pmu_counter_ns);

			if (l>=2) update_cumulative(&total_mcp_wr, &min_mcp_wr, &max_mcp_wr,
				&var_mcp_wr, 0, pmu_counter_ns, clk_overhead);
				//(We erase two first measurements because they are different from the
		    //others. Reason:Branch prediction misses when entering for loop)

			//printf("%d: %lld, %lld, %lld, %lld, %lld\n\r",l, pmu_counter_ns-
			//	clk_overhead, total_mcp_wr, min_mcp_wr, max_mcp_wr, var_mcp_wr);


			//READ DATA FROM ON-CHIP RAM

			pmu_counter_reset();

			for (j=0; j<operation_loops; j++)
				memcpy((void *) (&(data[j*ON_CHIP_MEMORY_SPAN])),
					(void*) FPGA_on_chip_RAM_addr,
					data_in_one_operation);

			overflow = pmu_counter_read_ns(&pmu_counter_ns);
			if (overflow == 1)
			{
				printf("Cycle counter overflow!! Program ended\n\r");
				return 1;
			}

			if (l>=2) update_cumulative(&total_mcp_rd, &min_mcp_rd, &max_mcp_rd,
				&var_mcp_rd, 0, pmu_counter_ns, clk_overhead);
				//(We erase two first measurements because they are different from the
		    //others. Reason:Branch prediction misses when entering for loop)

			//check the content of the data just read
			for (j=0; j<data_size[i]; j++)
			{
				if (data[j] != i) status = ALT_E_ERROR;
			}
			if (status != ALT_E_SUCCESS)
			{
				printf ("Error checking On-Chip RAM data size %dB\n\r", data_size[i]);
				return 1;
			}
			//else
			//printf("Check On-Chip RAM with data size %dB was OK\n\r", data_size[i]);

			//free dynamic memory
			free(data);

		}

		printf("%d, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld\n\r",
			data_size[i], total_mcp_wr/REP_TESTS, min_mcp_wr, max_mcp_wr,
			variance(var_mcp_wr, total_mcp_wr, REP_TESTS),
			total_mcp_rd/REP_TESTS, min_mcp_rd, max_mcp_rd,
			variance(var_mcp_rd, total_mcp_rd, REP_TESTS) );
	}

	#ifdef FORLOOP
	//----------Do the speed test (using for loop)--------------//
	printf("\n\rStart for loop test\n\r");
	printf("Data Size,Average DMA_WR,Min DMA_WR,Max DMA_WR,Variance DMA_WR");
	printf(",Average DMA_RD,Min DMA_RD,Max DMA_RD,Variance DMA_RD\n\r");

	for(i=0; i<number_of_data_sizes; i++)//for each data size
	{
		reset_cumulative( &total_dir_wr, &min_dir_wr, &max_dir_wr, &var_dir_wr);
		reset_cumulative( &total_dir_rd, &min_dir_rd, &max_dir_rd, &var_dir_rd);

		for(l = 0; l<REP_TESTS; l++)
		{
			//reserve the exact memory of the data size (except when data is smaller
			//than data bus size)
			if (data_size[i]<BYTES_DATA_BUS) datai=(UINT_SOC*) malloc(BYTES_DATA_BUS);
			else datai = (UINT_SOC*) malloc(data_size[i]);
			if (datai == 0)
			{
				printf("ERROR when calling malloc: Out of memory\n\r");
				return 1;
			}

			//save some content in data (for example: i)
			if (data_size[i]<BYTES_DATA_BUS) datai[0] = i;
			else for (j=0; j<(data_size[i]/BYTES_DATA_BUS); j++) datai[j] = i;

			//if data is bigger than on-chip size in bytes
			if (data_size[i]>ON_CHIP_MEMORY_SPAN)
			{
				data_in_one_operation_w = MEMORY_SIZE_IN_WORDS;
				operation_w_loops = (data_size[i]/BYTES_DATA_BUS)/MEMORY_SIZE_IN_WORDS;
			}
			else
			{
				if (data_size[i]<BYTES_DATA_BUS) //if data is smaller than bus width
				{
					data_in_one_operation_w = 1;
					operation_w_loops = 1;
				}
				else //data size is between data bus size and memory size
				{
					data_in_one_operation_w = data_size[i]/BYTES_DATA_BUS;
					operation_w_loops = 1;
				}
			}

			//WRITE DATA TO ON-CHIP RAM

			//read a value from clock to eliminate "first-read-jitter"
			pmu_counter_reset();

			int offset;
			for (j=0; j<operation_w_loops; j++)
			{
				offset = j*MEMORY_SIZE_IN_WORDS;
				for(k=0; k<data_in_one_operation_w; k++)
					FPGA_on_chip_RAM_addr[k] = datai[offset+k];
			}

			overflow = pmu_counter_read_ns(&pmu_counter_ns);
			if (overflow == 1)
			{
				printf("Cycle counter overflow!! Program ended\n\r");
				return 1;
			}

			update_cumulative(&total_dir_wr, &min_dir_wr, &max_dir_wr, &var_dir_wr,
				0, pmu_counter_ns, clk_overhead);
				//(We erase two first measurements because they are different from the
				//others. Reason:Branch prediction misses when entering for loop)

			//READ DATA FROM ON-CHIP RAM

			pmu_counter_reset();

			for (j=0; j<operation_w_loops; j++)
			{
				offset = j*MEMORY_SIZE_IN_WORDS;
				for(k=0; k<data_in_one_operation_w; k++)
					datai[offset+k] = FPGA_on_chip_RAM_addr[k];
			}

			overflow = pmu_counter_read_ns(&pmu_counter_ns);
			if (overflow == 1)
			{
				printf("Cycle counter overflow!! Program ended\n\r");
				return 1;
			}

			update_cumulative(&total_dir_rd, &min_dir_rd, &max_dir_rd,
				&var_dir_rd, 0, pmu_counter_ns, clk_overhead);
				//(We erase two first measurements because they are different from the
				//others. Reason:Branch prediction misses when entering for loop)

			//check the content of the data just read
			for (j=0; j<data_size[i]/BYTES_DATA_BUS; j++)
			{
				if (datai[j] != i) status = ALT_E_ERROR;
			}
			if (status != ALT_E_SUCCESS)
			{
					printf ("Error checking On-Chip RAM data size %dB\n\r", data_size[i]);
					return 1;

			}
			//else printf("Check On-Chip RAM data size %dB was OK\n\r", data_size[i]);

			//free memory used
			free(datai);
		}

		printf("%d, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld\n\r",
			data_size[i], total_dir_wr/REP_TESTS, min_dir_wr, max_dir_wr,
			variance(var_dir_wr, total_dir_wr, REP_TESTS), total_dir_rd/REP_TESTS,
			min_dir_rd, max_dir_rd,
			variance(var_dir_rd, total_dir_rd, REP_TESTS) );
	}
	#endif

	printf("\n\rData transfer measurements finished!!!!\n\r");

  return 0;
}
