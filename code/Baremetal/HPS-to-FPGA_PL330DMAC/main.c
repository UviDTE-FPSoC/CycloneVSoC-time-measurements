#include <stdio.h> // printf
#include <stdlib.h> // to use malloc and free
#include <inttypes.h>
#include <string.h> //to use memcpy and memcmp
#include <stdarg.h> //to use va lists

#include "alt_dma_modified.h"

#define soc_cv_av
#include "socal/socal.h"
#include "socal/hps.h"

//select configuration (bridge type and other experiment-related variables)
#include "configuration.h"
//high level API that permits to start cache with a single line
#include "cache_high_level_API.h"
//high level API that permits do a basic startup of ACP port
#include "acp_high_level_API.h"
//extract statistics from experiments (mean value, min, max, variance...)
#include "statistics.h"
//functions to init and control PMU as timer
#include "pmu.h"

/* enable semihosting with gcc by defining an __auto_semihosting symbol */
int __auto_semihosting;

//Some functions to init and uninit DMA Controller
ALT_STATUS_CODE PL330_DMAC_init(void);
ALT_STATUS_CODE PL330_DMAC_uninit(void);
ALT_STATUS_CODE PL330_DMAC_wait(ALT_DMA_CHANNEL_t Dma_Channel);

int main()
{
	printf("-----MEASURING HPS-to_FPGA BRIDGES SPEED IN BAREMETAL----\n\r\r");
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

	//------------------------VARIABLES INSTANTIATION-----------------------//
	//-------------Common variables------------//
	ALT_STATUS_CODE status = ALT_E_SUCCESS;

	int i, j, l; //vars for loops

	//cache configuration
	int cache_config = CACHE_CONFIG;

	//-----------some variables to define speed tests-------//
	//define data sizes
	int number_of_data_sizes = 21; //size of data_size[]
	int data_size [] =
	{2, 4, 8, 16, 32, 64, 128, 256, 512, 1024,
	2048, 4096, 8192, 16384, 32768,
	65536, 131072, 262144, 524288, 1048576,
	2097152}; //data size in bytes
	//maximum number of times a transfer will be repeated (since the memory
  //in the FPGA is smaller than max transfer size small transfers with size
	//equal to memory size are repeated untill the whole data is transferred
	//to FPGA)
	#define MAX_TRANSFER_SIZE 2097152
	#define MAX_OPERATIONS MAX_TRANSFER_SIZE/ONCHIP_MEMORY2_0_SPAN

	//DMA ones
	ALT_DMA_CHANNEL_t Dma_Channel;
	ALT_DMA_PROGRAM_t program;
	ALT_DMA_CHANNEL_STATE_t channel_state;
	ALT_DMA_CHANNEL_FAULT_t fault;
	//source and destiny pointers for DMA when reading and writing from/to FPGA
	char *rd_dst, *rd_src, *wr_dst, *wr_src;
	ALT_DMA_PROGRAM_t *programs_wr[MAX_OPERATIONS];
	ALT_DMA_PROGRAM_t *programs_rd[MAX_OPERATIONS];
	for (j=0; j<MAX_OPERATIONS; j++)
	{
		programs_wr[j]=(ALT_DMA_PROGRAM_t*) malloc(sizeof(ALT_DMA_PROGRAM_t));
		programs_rd[j]=(ALT_DMA_PROGRAM_t*) malloc(sizeof(ALT_DMA_PROGRAM_t));
	}

	//-------------Memory pointers------------//
	//FPGA On-Chip RAM (OCR)
	void* FPGA_on_chip_RAM_addr = (void*)(BRIDGE_BASE + ON_CHIP_MEMORY_BASE);
	UINT_SOC* fpgaocr_ptr;//FPGA on chip RAM pointer
	//HPS On-Chip RAM
	void* hpsocr_addr = (void*) 0xFFFF0000;
	int* hpsocr_ptr;//HPS on chip RAM pointer
	int hpsocr_size = 65536/4;//HPS OCR, size in bytes/4=size in 32-bit words

	//----Save intermediate results in speed tests-------//
	unsigned long long int total_clk, min_clk, max_clk, var_clk, clk_overhead;

	unsigned long long int total_dma_rd, min_dma_rd, max_dma_rd, var_dma_rd;
	unsigned long long int total_dma_wr, min_dma_wr, max_dma_wr, var_dma_wr;

	//-----------auxiliar variables to perform speed tests-------//
	char* data;
	int data_in_one_operation;//data to write or read from memory (bytes)
	int operation_loops; //number of loops to read/write all data from/to memory

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

	//-----------CONFIGURING ACP------------//
	status = acp_configuration();
	if(status == ALT_E_SUCCESS)
		printf("ACP ID Mapper configuration was succesful\n\r");
	else
	{
		printf("ACP ID Mapper configuration was not succesful\n\r");
		return ALT_E_ERROR;
	}

	//-----------INITIALIZATION OF PMU AS TIMER------------//
	printf("\n\r");
	unsigned long long pmu_counter_ns;
	int overflow = 0;
	pmu_init_ns(800, 1); //Init PMU cycle counter, 800MHz src, frequency divider 1
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
		if (overflow == 1){
      printf("Cycle counter overflow!! Program ended\n\r");
      return 1;}
		if (i>=2)
      update_cumulative(&total_clk, &min_clk, &max_clk, &var_clk, 0,
        pmu_counter_ns, 0);
		//(We erase two first measurements because they are different from the
    //others. Reason:Branch prediction misses when entering for loop)
	 }
	 printf("PMU Cycle Timer Stats for %d consecutive reads\n\r", CLK_REP_TESTS);
	 printf("Average, Minimum, Maximum, Variance\n\r");
	 printf("%lld,%lld,%lld,%lld\n\r", clk_overhead = total_clk/CLK_REP_TESTS,
    min_clk, max_clk, variance (var_clk , total_clk, CLK_REP_TESTS));

	 //-----------MOVING DATA WITH DMAC------------//
	 printf("\n\r--MOVING DATA WITH THE DMAC--\n\r");

	 // Initialize DMA Controller
   printf("INFO: Initializing DMA controller.\n\r");
   status = PL330_DMAC_init();
   if (status != ALT_E_SUCCESS)
	 {
      printf("ERROR when initializing DMA Controller. Return\n\r");
		  return 1;
	  }

	  // Allocate DMA Channel
    printf("INFO: Allocating DMA channel.\n\r");
    status = alt_dma_channel_alloc_any(&Dma_Channel);
    if (status != ALT_E_SUCCESS)
	  {
		  printf("ERROR allocating DMA channel. Return\n\r");
		  return 1;
	  }

	  printf("Doing measurements....\n\r");

    //-----Modify lockdown in L2 cache controller--//
    #ifdef EN_LOCKDOWN_STUDY
    int h;
    for (h=0; h<7; h++)
    {
      switch(h)
      {
      case 0:
        printf("\n\rNO LOCKDOWN\n\r");
        break;
      case 1:
        printf("\n\rLOCK CPUS 1 way\n\r");
        L2_lockdown_by_master(0b00000001, 0b00000001, 0b00000000, 3, 4);
        break;
      case 2:
        printf("\n\rLOCK CPUS 4 ways\n\r");
        L2_lockdown_by_master(0b00001111, 0b00001111, 0b00000000, 3, 4);
        break;
      case 3:
        printf("\n\rLOCK CPUS 7 ways\n\r");
        L2_lockdown_by_master(0b01111111, 0b01111111, 0b00000000, 3, 4);
        break;
      case 4:
        printf("\n\rLOCK CPUS 1 way and ACP the other 7 ways\n\r");
        L2_lockdown_by_master(0b00000001, 0b00000001, 0b11111110, 3, 4);
        break;
      case 5:
        printf("\n\rLOCK CPUS 4 ways and ACP the other 4 ways\n\r");
        L2_lockdown_by_master(0b00001111, 0b00001111, 0b11110000, 3, 4);
        break;
      case 6:
        printf("\n\rLOCK CPUS 7 ways and ACP the other way\n\r");
        L2_lockdown_by_master(0b01111111, 0b01111111, 0b10000000, 3, 4);
      default:
        break;
      }
    #endif

	  printf("\n\r--PL330 DMAC tests (DMAC uCode preparation time included)--\n\r");

    //Moving data with DMAC (DMAC program preparation is measured)
	  printf("Data Size,Average DMA_WR,Min DMA_WR,Max DMA_WR,Variance DMA_WR");
    printf(",Average DMA_RD,Min DMA_RD,Max DMA_RD,Variance DMA_RD\n\r");
    for(i=0; i<number_of_data_sizes; i++)//for each data size
	  {
		  reset_cumulative( &total_dma_wr, &min_dma_wr, &max_dma_wr, &var_dma_wr);
		  reset_cumulative( &total_dma_rd, &min_dma_rd, &max_dma_rd, &var_dma_rd);

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

		  for(l = 0; l<REP_TESTS+2; l++)
		  {
			  //reserve the exact memory of the data size
			  data = (char*) malloc(data_size[i]);
			  if (data == 0)
			  {
				  printf("ERROR when calling malloc: Out of memory\n\r");
				  return 1;
			  }

        //if data cache is on access through ACP to coherent data in cache
			  if (cache_config >= 6)
			  {
				  wr_dst = (char*) FPGA_on_chip_RAM_addr;
				  wr_src = (char*)((void*)(data)+0x80000000); //+0x80000000
				  rd_dst = (char*)((void*)(data)+0x80000000); //to access through ACP
				  rd_src = (char*) FPGA_on_chip_RAM_addr;
			  }
        //if data cache is off it does not make sense to access through ACP
        //(it will be slower than direct access to sdram controller)
			  else
			  {
				  wr_dst = (char*) FPGA_on_chip_RAM_addr;
				  wr_src = data;
				  rd_dst = data;
				  rd_src = (char*) FPGA_on_chip_RAM_addr;
			  }

			  //save some content in data (for example: i)
			  for (j=0; j<data_size[i]; j++) data[j] = i;

			  //--WRITE DATA TO FPGA ON-CHIP RAM
			  pmu_counter_reset();

			  for (j=0; j<operation_loops; j++)
			  {
				  status = alt_dma_memory_to_memory(Dma_Channel, &program, wr_dst,
					&(wr_src[j*ON_CHIP_MEMORY_SPAN]), data_in_one_operation, false,
          (ALT_DMA_EVENT_t)0);

				  // Wait for transfer to complete
				  if (status == ALT_E_SUCCESS)
				  {
					  //printf("INFO: Waiting for DMA transfer to complete.\n\r");
					  channel_state = ALT_DMA_CHANNEL_STATE_EXECUTING;
					  while((status == ALT_E_SUCCESS) && (channel_state != ALT_DMA_CHANNEL_STATE_STOPPED))
					  {
						  status = alt_dma_channel_state_get(Dma_Channel, &channel_state);
						  if(channel_state == ALT_DMA_CHANNEL_STATE_FAULTING)
						  {
							  alt_dma_channel_fault_status_get(Dma_Channel, &fault);
							  printf("ERROR: DMA Channel Fault: %d. Return\n\r", (int)fault);
							  return 1;
						  }
					  }
				 }
			}
			overflow = pmu_counter_read_ns(&pmu_counter_ns);
			if (overflow == 1){printf("Cycle counter overflow!! Program ended\n\r");
        return 1;}
			//printf("PMU counter (ns): %lld \n\r", pmu_counter_ns);

			if (l>=2) update_cumulative(&total_dma_wr, &min_dma_wr, &max_dma_wr,
        &var_dma_wr, 0, pmu_counter_ns, clk_overhead);
			//(We erase two first measurements because they are different from the
      //others. Reason:Branch prediction misses when entering for loop)

			//check the content of the data just read
			// Compare results
			if(0  != memcmp(&(data[0]), FPGA_on_chip_RAM_addr, data_in_one_operation))
			{
				printf("DMA src and dst have different data on WR!! Program ended\n\r");
        return 1;
			}
			//printf("%d: %lld, %lld, %lld, %lld, %lld\n\r",l, pmu_counter_ns-
      //clk_overhead, total_dma_wr, min_dma_wr, max_dma_wr, var_dma_wr);


			//--READ DATA FROM FPGA ON-CHIP RAM
			pmu_counter_reset();

			for (j=0; j<operation_loops; j++)
			{
				status = alt_dma_memory_to_memory(Dma_Channel, &program,
          &(rd_dst[j*ON_CHIP_MEMORY_SPAN]), rd_src, data_in_one_operation,
          false, (ALT_DMA_EVENT_t)0);

				// Wait for transfer to complete
				if (status == ALT_E_SUCCESS)
				{
					//printf("INFO: Waiting for DMA transfer to complete.\n\r");
					channel_state = ALT_DMA_CHANNEL_STATE_EXECUTING;
					while((status == ALT_E_SUCCESS) && (channel_state != ALT_DMA_CHANNEL_STATE_STOPPED))
					{
						status = alt_dma_channel_state_get(Dma_Channel, &channel_state);
						if(channel_state == ALT_DMA_CHANNEL_STATE_FAULTING)
						{
							alt_dma_channel_fault_status_get(Dma_Channel, &fault);
							printf("ERROR: DMA CHannel Fault: %d. Return\n\r", (int)fault);
							return 1;
						}
					}
				}
			}

			overflow = pmu_counter_read_ns(&pmu_counter_ns);
			if (overflow == 1){printf("Cycle counter overflow!! Program ended\n\r");
        return 1;}

			if (l>=2) update_cumulative(&total_dma_rd, &min_dma_rd, &max_dma_rd,
        &var_dma_rd, 0, pmu_counter_ns, clk_overhead);
			//(We erase two first measurements because they are different from the
      //others. Reason:Branch prediction misses when entering for loop)

			//check the content of the data just read
			// Compare results
			//printf("INFO: Comparing source and destination buffers.\n\r");
			if(0  != memcmp(&(data[0]), FPGA_on_chip_RAM_addr, data_in_one_operation))
			{
				printf("DMA src and dst have different data on RD!! Program ended\n\r");
        return 1;
			}

			//free dynamic memory
			free(data);
		}

		printf("%d, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld\n\r",
    data_size[i], total_dma_wr/REP_TESTS, min_dma_wr, max_dma_wr,
    variance(var_dma_wr, total_dma_wr, REP_TESTS),
    total_dma_rd/REP_TESTS, min_dma_rd, max_dma_rd,
    variance(var_dma_rd, total_dma_rd, REP_TESTS) );
	}

	printf("\n\r--PL330 DMAC tests (DMAC uCode prep. time NOT included)--\n\r");
	//Moving data with DMAC (DMAC program preparation is measured)
  printf("Data Size,Average DMA_WR,Min DMA_WR,Max DMA_WR,Variance DMA_WR");
  printf(",Average DMA_RD,Min DMA_RD,Max DMA_RD,Variance DMA_RD\n\r");
  for(i=0; i<number_of_data_sizes; i++)//for each data size
	{
		reset_cumulative( &total_dma_wr, &min_dma_wr, &max_dma_wr, &var_dma_wr);
		reset_cumulative( &total_dma_rd, &min_dma_rd, &max_dma_rd, &var_dma_rd);

		//reserve the exact memory of the data size
		data = (char*) malloc(data_size[i]);
		if (data == 0)
		{
			printf("ERROR when calling malloc: Out of memory\n\r");
			return 1;
		}

		if (cache_config >= 6) //if data cache is ON, access coherently through ACP
		{
			wr_dst = (char*) FPGA_on_chip_RAM_addr;
			wr_src = (char*)((void*)(data)+0x80000000); //+0x80000000
			rd_dst = (char*)((void*)(data)+0x80000000); //to access through ACP
			rd_src = (char*) FPGA_on_chip_RAM_addr;
		}
    //if data cache is off it does not make sense to access through ACP
    //(it will be slower than direct access to sdram controller)
		else
		{
			wr_dst = (char*) FPGA_on_chip_RAM_addr;
			wr_src = data;
			rd_dst = data;
			rd_src = (char*) FPGA_on_chip_RAM_addr;
		}

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

		for (j=0; j<operation_loops; j++)
		{
			//write programs
			alt_dma_memory_to_memory_only_prepare_program(Dma_Channel, programs_wr[j],
        wr_dst, &(wr_src[j*ON_CHIP_MEMORY_SPAN]), data_in_one_operation, false,
        (ALT_DMA_EVENT_t)0);
			//read programs
			alt_dma_memory_to_memory_only_prepare_program(Dma_Channel, programs_rd[j],
        &(rd_dst[j*ON_CHIP_MEMORY_SPAN]), rd_src, data_in_one_operation, false,
        (ALT_DMA_EVENT_t)0);
		}

		for(l = 0; l<REP_TESTS+2; l++)
		{
			//save some content in data (for example: i)
			for (j=0; j<data_size[i]; j++) data[j] = i;

			//--WRITE DATA TO FPGA ON-CHIP RAM
			pmu_counter_reset();

			for (j=0; j<operation_loops; j++)
			{
				status = alt_dma_channel_exec(Dma_Channel, programs_wr[j]);

				// Wait for transfer to complete
				if (status == ALT_E_SUCCESS)
				{
					//printf("INFO: Waiting for DMA transfer to complete.\n\r");
					channel_state = ALT_DMA_CHANNEL_STATE_EXECUTING;
					while((status == ALT_E_SUCCESS) && (channel_state != ALT_DMA_CHANNEL_STATE_STOPPED))
					{
						status = alt_dma_channel_state_get(Dma_Channel, &channel_state);
						if(channel_state == ALT_DMA_CHANNEL_STATE_FAULTING)
						{
							alt_dma_channel_fault_status_get(Dma_Channel, &fault);
							printf("ERROR: DMA Channel Fault: %d. Return\n\r", (int)fault);
							return 1;
						}
					}
				}
			}

			overflow = pmu_counter_read_ns(&pmu_counter_ns);
			if (overflow == 1){printf("Cycle counter overflow!! Program ended\n\r");
        return 1;}
			//printf("PMU counter (ns): %lld \n\r", pmu_counter_ns);

			if (l>=2) update_cumulative(&total_dma_wr, &min_dma_wr, &max_dma_wr,
        &var_dma_wr, 0, pmu_counter_ns, clk_overhead);
			//(We erase two first measurements because they are different from the
      //others. Reason:Branch prediction misses when entering for loop)

			//check the content of the data just read
			// Compare results
			//printf("INFO: Comparing source and destination buffers.\n\r");
			if(0  != memcmp(&(data[0]), FPGA_on_chip_RAM_addr, data_in_one_operation))
			{
				printf("DMA source and destiny have different data on WR!!");
        printf("Program ended\n\r");
        return 1;
			}
			//printf("%d: %lld, %lld, %lld, %lld, %lld\n\r",l,pmu_counter_ns-
      //clk_overhead, total_dma_wr, min_dma_wr, max_dma_wr, var_dma_wr);
			//--READ DATA FROM FPGA ON-CHIP RAM
			pmu_counter_reset();

			for (j=0; j<operation_loops; j++)
			{
				status = alt_dma_channel_exec(Dma_Channel, programs_rd[j]);

				// Wait for transfer to complete
				if (status == ALT_E_SUCCESS)
				{
					//printf("INFO: Waiting for DMA transfer to complete.\n\r");
					channel_state = ALT_DMA_CHANNEL_STATE_EXECUTING;
					while((status == ALT_E_SUCCESS) && (channel_state != ALT_DMA_CHANNEL_STATE_STOPPED))
					{
						status = alt_dma_channel_state_get(Dma_Channel, &channel_state);
						if(channel_state == ALT_DMA_CHANNEL_STATE_FAULTING)
						{
							alt_dma_channel_fault_status_get(Dma_Channel, &fault);
							printf("ERROR: DMA CHannel Fault: %d. Return\n\r", (int)fault);
							return 1;
						}
					}
				}
			}

			overflow = pmu_counter_read_ns(&pmu_counter_ns);
			if (overflow == 1){printf("Cycle counter overflow!! Program ended\n\r");
        return 1;}

			if (l>=2) update_cumulative(&total_dma_rd, &min_dma_rd, &max_dma_rd,
        &var_dma_rd, 0, pmu_counter_ns, clk_overhead);
			//(We erase two first measurements because they are different from the
      //others. Reason:Branch prediction misses when entering for loop)

			//check the content of the data just read
			// Compare results
			//printf("INFO: Comparing source and destination buffers.\n\r");
			if(0  != memcmp(&(data[0]), FPGA_on_chip_RAM_addr, data_in_one_operation))
			{
				printf("DMA source and destiny have different data on RD!!");
        printf("Program ended\n\r");return 1;
			}
		}

		printf("%d, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld\n\r",
    data_size[i], total_dma_wr/REP_TESTS, min_dma_wr, max_dma_wr,
    variance(var_dma_wr, total_dma_wr, REP_TESTS),
    total_dma_rd/REP_TESTS, min_dma_rd, max_dma_rd,
    variance(var_dma_rd, total_dma_rd, REP_TESTS) );

		//free dynamic memory
		free(data);
	}

  #ifdef EN_LOCKDOWN_STUDY
  }
  #endif

	printf("\n\rData transfer measurements finished!!!!\n\r");
  return 0;
}

//--------------------------DMA FUNCTIONS--------------------------//
ALT_STATUS_CODE PL330_DMAC_init(void)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    printf("INFO: Setting up DMA.\n\r");

    // Uninit DMA
    if(status == ALT_E_SUCCESS)
    {
        status = alt_dma_uninit();
    }

    // Configure everything as defaults.
    if (status == ALT_E_SUCCESS)
    {
        ALT_DMA_CFG_t dma_config;
        dma_config.manager_sec = ALT_DMA_SECURITY_DEFAULT;
        for (int i = 0; i < 8; ++i)
        {
            dma_config.irq_sec[i] = ALT_DMA_SECURITY_DEFAULT;
        }
        for (int i = 0; i < 32; ++i)
        {
            dma_config.periph_sec[i] = ALT_DMA_SECURITY_DEFAULT;
        }
        for (int i = 0; i < 4; ++i)
        {
            dma_config.periph_mux[i] = ALT_DMA_PERIPH_MUX_DEFAULT;
        }

        status = alt_dma_init(&dma_config);
    }
    return status;
}

ALT_STATUS_CODE PL330_DMAC_uninit(void)
{
    printf("INFO: DMA shutdown.\n\r");
    printf("\n\r");
    return ALT_E_SUCCESS;
}

ALT_STATUS_CODE PL330_DMAC_wait(ALT_DMA_CHANNEL_t Dma_Channel)
{
  //to call after a transfer is initiated, to wait for a transfer to end
  //printf("INFO: Waiting for DMA transfer to complete.\n\r");
  ALT_STATUS_CODE status = ALT_E_SUCCESS;
  ALT_DMA_CHANNEL_FAULT_t fault;
  ALT_DMA_CHANNEL_STATE_t channel_state = ALT_DMA_CHANNEL_STATE_EXECUTING;

  while((status == ALT_E_SUCCESS) && (channel_state != ALT_DMA_CHANNEL_STATE_STOPPED))
  {
    status = alt_dma_channel_state_get(Dma_Channel, &channel_state);
    if(channel_state == ALT_DMA_CHANNEL_STATE_FAULTING)
    {
      alt_dma_channel_fault_status_get(Dma_Channel, &fault);
      printf("ERROR: DMA Channel Fault: %d. Return\n\r", (int)fault);
      return 1;
    }
  }
  return 0;
}
