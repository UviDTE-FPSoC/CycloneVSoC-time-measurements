#include <stdio.h> // printf
#include <stdlib.h> // to use malloc and free
#include <inttypes.h>
#include <string.h> //to use memcpy and memcmp
#include <stdarg.h> //to use va lists
#include <math.h>

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
//functions to modify SDRAM controller behaviour
#include "sdramc.h"
//functions to control the FPGA DMA CONTROLLER
#include "fpga_dmac_api.h"

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

/* enable semihosting with gcc by defining an __auto_semihosting symbol */
int __auto_semihosting;

ALT_STATUS_CODE check_mem(void* addr, int size);
ALT_STATUS_CODE reset_mem(void* addr, int size);
int upper_aligned(int number, int align);

int main()
{
	printf("---MEASURING FPGA-HPS BRIDGES SPEED IN BAREMETAL WITH FPGA DMACs--\n\r\r");
	#ifdef BRIDGES_32BIT
		#ifdef WR_HPS
			printf("WRITING HPS THROUGH 32-BIT BRIDGES\n\r");
    #else
		  #ifdef RD_HPS
			   printf("READING HPS THROUGH 32-BIT BRIDGES\n\r");
      #else
			printf("CHOOSE RD OR WR!!\n\r");
			return 0;
      #endif
		#endif
	#endif
	#ifdef BRIDGES_64BIT
		#ifdef WR_HPS
			printf("WRITING HPS THROUGH 64-BIT BRIDGES\n\r");
    #else
		  #ifdef RD_HPS
			   printf("READING HPS THROUGH 64-BIT BRIDGES\n\r");
		  #else
			   printf("CHOOSE RD OR WR!!\n\r");
			   return 0;
      #endif
		#endif
	#endif
	#ifdef BRIDGES_128BIT
		#ifdef WR_HPS
			printf("WRITING HPS THROUGH 128-BIT BRIDGES\n\r");
    #else
		  #ifdef RD_HPS
			   printf("READING HPS THROUGH 128-BIT BRIDGES\n\r");
		  #else
			   printf("CHOOSE RD OR WR!!\n\r");
			   return 0;
      #endif
		#endif
	#endif

	printf("Each measurement is repeated %d times\n\r", REP_TESTS);

	//------------------------VARIABLES INSTANTIATION-----------------------//
	//-------------Common variables------------//
	ALT_STATUS_CODE status = ALT_E_SUCCESS;

	int i, j, l, k, h; //vars for loops
  #ifdef GENERATE_DUMMY_TRAFFIC_IN_CACHE
  int i_dummy = 0;
  #endif

	//cache configuration
	int cache_config = CACHE_CONFIG;

	//-----------some variables to define speed tests-------//
	//define data sizes
	int number_of_data_sizes = 21; //size of data_size[]
  int number_of_data_sizes_test; //from all, which ones to test??
	//data pointer to buffers in CPU memory
	char* data[5]; //alligned
	char* unalligned_data[5]; //alligned
	char* data_dmac[5];//data as seen by dmacs
	//data size to be used when a single DMA is working
	int data_size [] =
	{2, 4, 8, 16, 32, 64, 128, 256, 512, 1024,
	2048, 4096, 8192, 16384, 32768,
	65536, 131072, 262144, 524288, 1048576,
	2097152}; //data size in bytes

	//addresses of the OCRs as seen by DMACs
	void* fpga_ocr_dmac[5] = {0, 0, 0, 0, 0};

	//variables to manage the dmacs involved in transaction
	int number_dmacs_test, first_dmac_test, last_dmac_test;

	//data_size_dmac[x][y] = x+1 is the number of dmacs working, y data size
	int data_size_dmac[5][number_of_data_sizes];
	for (i=0; i<5; i++)
	{
		for (j=0; j<number_of_data_sizes; j++)
		{
			data_size_dmac[i][j] = max( MIN_TRANSFER_SIZE,
                                  (upper_aligned((float)data_size[j]/(float)(i+1), MIN_TRANSFER_SIZE))
                             );
		}
	}

  for (i=0; i<5; i++)
	{
    printf("\n\rdata_size_dmac[%d]=",i);
		for (j=0; j<number_of_data_sizes; j++)
		{
      printf("%d,",data_size_dmac[i][j]);
		}
	}
printf("\n\r");

	//-------------Memory pointers------------//
	//save address of DMACs in a vector
	void* dma_addr[5] = 		{	HF_BRIDGE_BASE + DMA_FPGA_HPS_BASE,
														HF_BRIDGE_BASE + DMA_FPGA_SDRAM_0_BASE,
														HF_BRIDGE_BASE + DMA_FPGA_SDRAM_1_BASE,
														HF_BRIDGE_BASE + DMA_FPGA_SDRAM_2_BASE,
														HF_BRIDGE_BASE + DMA_FPGA_SDRAM_3_BASE};
	void* fpga_ocr_addr[5] ={ HF_BRIDGE_BASE + OCR_FPGA_HPS_BASE,
														HF_BRIDGE_BASE + OCR_FPGA_SDRAM_0_BASE,
														HF_BRIDGE_BASE + OCR_FPGA_SDRAM_1_BASE,
														HF_BRIDGE_BASE + OCR_FPGA_SDRAM_2_BASE,
														HF_BRIDGE_BASE + OCR_FPGA_SDRAM_3_BASE};

	//----Save intermediate results in speed tests-------//
	unsigned long long int total_clk, min_clk, max_clk, var_clk, clk_overhead;
	unsigned long long int total_dma, min_dma, max_dma, var_dma;


	//-------------CHECK AND RESET FPGA 1kB FPGA ON-CHIP RAMs-------------//
	//Check the FPGA-OCRs
	for (i=FIRST_FPGA_OCR_CHECK; i<=LAST_FPGA_OCR_CHECK; i++)
	{
		status = check_mem(fpga_ocr_addr[i], FPGA_OCR_SIZE);
		if (status == ALT_E_SUCCESS)
			printf("Check FPGA On-Chip RAM %d OK\n\r", i);
		else
		{
			printf ("Error when checking FPGA On-Chip RAM %d\n\r", i);
			return 1;
		}
	}

	//Reset the FPGA-OCRs
	for (i=FIRST_FPGA_OCR_CHECK; i<=LAST_FPGA_OCR_CHECK; i++)
    reset_mem(fpga_ocr_addr[i], FPGA_OCR_SIZE);

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

  //------DEFINE AXI SIGNALS THAT CAN AFFECT THE TRANSACTION-------//
  //AXI_SIGNALS[3-0]  = AWCACHE = 0111 (Cacheable write-back, allocate reads only)
  //AXI_SIGNALS[6-4]  = AWPROT = 000 (normal access, non-secure, data)
  //AXI_SIGNALS[11-7] = AWUSER = 00001 (Coherent access)
  //AXI_SIGNALS[19-16]  = ARCACHE = 0111
  //AXI_SIGNALS[22-20]  = ARPROT = 000
  //AXI_SIGNALS[27-23] = ARUSER = 00001
  //AXI_SIGNALS = 0x00870087; //works for WR and RD and gives fastest accesses
  uint32_t AXI_SIGNALS = 0x00870087;
  uint8_t* gpio_add = GPIO_ADDRESS;
  //Write data to the GPIO connected to AXI signals
  *gpio_add = AXI_SIGNALS;

  //----CONFIGURING SDRAM CONTROLLER-------//
  //Remove FPGA-to-SDRAMC ports from reset so FPGA can access SDRAM through them
  *((unsigned int *)(SDRAMC_REGS + FPGAPORTRST)) = 0xFFFF;

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
		//simulate time lost when activating and reading a DMAC in FPGA
		fpga_dma_write_bit(dma_addr[0], FPGA_DMA_CONTROL, FPGA_DMA_GO, 0);
		fpga_dma_transfer_done(dma_addr[0]);
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
	printf("Average (ns), Minimum, Maximum, Variance\n\r");
	printf("%lld,%lld,%lld,%lld\n\r", clk_overhead = total_clk/CLK_REP_TESTS,
    min_clk, max_clk, variance (var_clk , total_clk, CLK_REP_TESTS));

	//-----------MOVING DATA WITH DMAC------------//
	printf("\n\r--MOVING DATA WITH THE DMACs--\n\r");

  #ifdef TEST_ALL_COMBINATIONS
  for (h=0; h<5; h++)
  #endif
  #ifdef TEST_ONLY_F2S_BRIDGES
  for (h=0; h<2; h++)
  #endif
  #ifdef TEST_F2H_AND_ALL_BRIDGES
  for (h=2; h<5; h++)
  #endif
  {
    switch(h)
		{
    case 0:
      printf("\n\r1xFPGA-SDRAMC\n\r");
			number_dmacs_test = 1;
			first_dmac_test = 1;
			last_dmac_test = 1;
      number_of_data_sizes_test = number_of_data_sizes;
      break;
    case 1:
      printf("\n\rAll FPGA-SDRAMC\n\r");
			number_dmacs_test = NUM_OF_DMACS-1;
			first_dmac_test = 1;
			last_dmac_test = NUM_OF_DMACS-1;
      number_of_data_sizes_test = number_of_data_sizes;
      break;
    case 2:
      printf("\n\rFPGA-HPS ACP\n\r");
			number_dmacs_test = 1;
			first_dmac_test = 0;
			last_dmac_test = 0;
      #ifdef LIMIT_F2H_TESTS
        number_of_data_sizes_test = NUMBER_DATA_SIZES_TEST;
      #else
        number_of_data_sizes_test = number_of_data_sizes;
      #endif
      break;
    case 3:
      printf("\n\rFPGA-HPS L3->SDRAMC\n\r");
			number_dmacs_test = 1;
			first_dmac_test = 0;
			last_dmac_test = 0;
      #ifdef LIMIT_F2H_TESTS
        number_of_data_sizes_test = NUMBER_DATA_SIZES_TEST;
      #else
        number_of_data_sizes_test = number_of_data_sizes;
      #endif
      break;
    case 4:
      printf("\n\rAll Bridges\n\r");
			number_dmacs_test = NUM_OF_DMACS;
			first_dmac_test = 0;
			last_dmac_test = NUM_OF_DMACS-1;
      #ifdef LIMIT_F2H_TESTS
        number_of_data_sizes_test = NUMBER_DATA_SIZES_TEST;
      #else
        number_of_data_sizes_test = number_of_data_sizes;
      #endif
      break;
    default:
      break;
    }

		printf("INFO: Initializing DMA controllers.\n\r");
		for (j=first_dmac_test; j<=last_dmac_test; j++)
		{
			fpga_dma_init(dma_addr[j],
		                TRANSFER_WORD_SIZE | //DMA transfer size is = to bridge size
		                FPGA_DMA_END_WHEN_LENGHT_ZERO |
										FPGA_DMAC_CONSTANT_ADDR
									  );
		}

    #ifdef EN_SDRAMC_STUDY
    int g;
    for (g=0; g<7; g++)
    {
      switch(g)
      {
      case 0:
        printf("\n\rSDRAM DEFAULT CONFIGURATION\n\r");
        print_sdramc_priorities_weights();
        break;
      case 1:
        printf("\n\rSDRAM SAME PRIORITIES AND WEIGHTS ALL PORTS\n\r");
        set_sdramc_priorities(0x0);
        set_sdramc_weights(0x40000000, 0x2108);//same weights (1)
        print_sdramc_priorities_weights();
        break;
      case 2:
        printf("\n\rSDRAM SAME PRIORITIES L3 2x WEIGHT\n\r");
        set_sdramc_priorities(0x0);
        set_sdramc_weights(0x80000000, 0x2208);
        print_sdramc_priorities_weights();
        break;
      case 3:
        printf("\n\rSDRAM SAME PRIORITIES L3 4x WEIGHT\n\r");
        set_sdramc_priorities(0x0);
        set_sdramc_weights(0x0, 0x2409);
        print_sdramc_priorities_weights();
        break;
      case 4:
        printf("\n\rSDRAM SAME PRIORITIES L3 8x WEIGHT\n\r");
        set_sdramc_priorities(0x0);
        set_sdramc_weights(0x0, 0x280A);
        print_sdramc_priorities_weights();
        break;
      case 5:
        printf("\n\rSDRAM SAME PRIORITIES L3 16x WEIGHT\n\r");
        set_sdramc_priorities(0x0);
        set_sdramc_weights(0x0, 0x300C);
        print_sdramc_priorities_weights();
        break;
      case 6:
        printf("\n\rSDRAM L3 MORE PRIORITY\n\r");
        set_sdramc_priorities(0x71C0000);
        set_sdramc_weights(0x40000000, 0x2108);//same weights (1)
        print_sdramc_priorities_weights();
        break;
      default:
        break;
      }
    #endif

    #ifdef GENERATE_DUMMY_TRAFFIC_IN_CACHE
    printf("Dummy traffic is generated to pollute cache\n\r");
    char* dummydata = (char*) malloc(2*1024*1024); //2MB
    if (dummydata == 0)
    {
      printf("ERROR when calling malloc for dummy data: Out of memory\n\r");
      return 1;
    }
    #endif

    //Moving data with DMAC (DMAC program preparation is measured)
    printf("data sizes test = %d\n\r", number_of_data_sizes_test);
	  printf("Data Size, Average, Min, Max, Variance\n\r");
    for(i=0; i<number_of_data_sizes_test; i++)//for each data size
	  {
		  reset_cumulative( &total_dma, &min_dma, &max_dma, &var_dma);

		  for(l = 0; l<REP_TESTS+2; l++)
		  {
			  //reserve the exact memory of the data size
				for (j=first_dmac_test; j<=last_dmac_test; j++)
				{
					data[j] = (char*) align_malloc(data_size_dmac[number_dmacs_test-1][i],
						(void**)&unalligned_data[j]);
					if (data[j] == 0)
				  {
					  printf("ERROR when calling malloc: Out of memory \n\r");
					  return 1;
				  }
				}

				if (h==2) //If the transfer is through ACP
					for (j=first_dmac_test; j<=last_dmac_test; j++) data_dmac[j]
						= data[j] + 0x80000000;
				else
					for (j=first_dmac_test; j<=last_dmac_test; j++) data_dmac[j] = data[j];

				//clear destiny and fill source
				for (j=first_dmac_test; j<=last_dmac_test; j++)
				{
					for (k= 0; k<data_size_dmac[number_dmacs_test-1][i];k++)
							((char*)(DMA_SRC_UP[j]))[k] = (char) i;
					reset_mem(DMA_DST_UP[j], data_size_dmac[number_dmacs_test-1][i]);
				}
				//Configure DMACs for their respective transfers
				for (j=first_dmac_test; j<=last_dmac_test; j++)
        {
					fpga_dma_config_transfer(dma_addr[j],
			                           	DMA_SRC_DMAC[j],
			                           	DMA_DST_DMAC[j],
			                           	data_size_dmac[number_dmacs_test-1][i]);
          #ifdef PRINT_TRANSFER_DETAILS
            printf("dma address=%X\n\r",(unsigned int)dma_addr[j]);
            printf("src address=%X\n\r",(unsigned int)DMA_SRC_DMAC[j]);
            printf("dst address=%X\n\r",(unsigned int)DMA_DST_DMAC[j]);
            printf("data_size=%d\n\r",(int)data_size_dmac[number_dmacs_test-1][i]);
          #endif
        }
			  //----DO THE TRANSFER----//
			  pmu_counter_reset();
				//start DMACs
				for (j=first_dmac_test; j<=last_dmac_test; j++)
				{
					fpga_dma_start_transfer(dma_addr[j]);
				}
				// Wait for each dmac to finish its transfer
				for (j=first_dmac_test; j<=last_dmac_test; j++)
				{
					while(fpga_dma_transfer_done(dma_addr[j])==0)
					{
						#ifdef GENERATE_DUMMY_TRAFFIC_IN_CACHE
						i_dummy++; if (i_dummy==2*1024*1024) i_dummy = 0;
						dummydata[i_dummy] = dummydata[i_dummy]+1;
						i_dummy++; if (i_dummy==2*1024*1024) i_dummy = 0;
						dummydata[i_dummy] = dummydata[i_dummy]+1;
						#endif
					}
				}

				overflow = pmu_counter_read_ns(&pmu_counter_ns);
				if (overflow == 1){printf("Cycle counter overflow!! Program ended\n\r");
        	return 1;}

        //printf("PMU counter (ns): %lld \n\r", pmu_counter_ns);

				if (l>=2) update_cumulative(&total_dma, &min_dma, &max_dma,
			    &var_dma, 0, pmu_counter_ns, clk_overhead);
				//(We erase two first measurements because they are different from the
			  //others. Reason:Branch prediction misses when entering for loop)

				//check the content of the data just read
				// Compare results
				for (j=first_dmac_test; j<=last_dmac_test; j++)
				{
					if(0  != memcmp(DMA_SRC_DMAC[j], DMA_SRC_DMAC[j], 2))
					{
						printf("DMA src and dst have different data!! Program ended\n\r");
				    return 1;
					}
				}

				//free dynamic memory
				for (j=first_dmac_test; j<=last_dmac_test; j++)
				{
					free(unalligned_data[j]);
				}
			}

			printf("%d, %lld, %lld, %lld, %lld\n\r",
	    data_size[i], total_dma/REP_TESTS, min_dma, max_dma,
	    variance(var_dma, total_dma, REP_TESTS));
		}

  	#ifdef GENERATE_DUMMY_TRAFFIC_IN_CACHE
  	free(dummydata);
  	#endif
  	#ifdef EN_SDRAMC_STUDY
  	}
		#endif

	}//for h
	printf("\n\rData transfer measurements finished!!!!\n\r");
  return 0;
}

//Functions to check and reset memory portions
ALT_STATUS_CODE check_mem(void* addr, int size)
{
	char* ptr = (char*) addr;
	int i;
	for (i=0; i<(size); i++)
	{
		*ptr = (char)i;
		if (*ptr != (char) i)
		{
			return ALT_E_ERROR;
		}
		ptr++;
	}
	return ALT_E_SUCCESS;
}

ALT_STATUS_CODE reset_mem(void* addr, int size)
{
	char* ptr = (char*) addr;
	int i;
	for (i=0; i<(size); i++)
	{
		*ptr = 0;
		if (*ptr != 0) return ALT_E_ERROR;
		ptr++;
	}
	return ALT_E_SUCCESS;
}

int upper_aligned(int number, int align)
{
  int partial = (int) ceil(((float)number)/((float)align));
  return (partial * align);
}
