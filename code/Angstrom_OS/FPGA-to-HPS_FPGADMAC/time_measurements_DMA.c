//file: time_measuremnts_FPGA_DMA.c
//performs time measurements of data transfers between FPGA and processor using
//5 DMA controllers (DMACs) available in the FPGA.  One DMA controller is
//connected to the FPGA-to-HPS bridge. The rest are connected to the
//FPGA-to-SDRAM ports. When the bridge width is 128-bits only 2 FPGA-to-SDRAM
//ports (and DMACs) can be configured. When it is 32- or 64-bit 4 of them
//can be configured.
//Remember to insert the Alloc_DMAble_buff_LKM driver before using the program.
//(https://github.com/robertofem/CycloneVSoC-examples/tree/soft_dma_baremetal/Linux-modules/Alloc_DMAble_buff_LKM)

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <stdint.h>
#include <stdarg.h>
#include <math.h>

#ifdef GENERATE_DUMMY_TRAFFIC_IN_CACHE
#include <pthread.h>
#endif

#include "configuration.h"
#include "pmu.h" //to measure time with PMU
#include "statistics.h" //do some statistics (mean, min, max, variance)
#include "fpga_dmac_api.h" //to control DMACs in FPGA

//Declaration of some extra functions
//Print buffer in screen
void printbuff(char* buff, int size);
//Functions to check and reset memory portions
int check_mem(void* addr, int size);
int reset_mem(void* addr, int size);
//find a transfer size aligned to the next power of 2
int upper_aligned(int number, int align);
#ifdef GENERATE_DUMMY_TRAFFIC_IN_CACHE
//generates dummy traffic in cache to slow down transfers
void genereate_dummy_traffic();
#endif
//allocate physically contiguous buffers using driver
void* alloc_phys_contiguous (size_t size, int uncached, int* filep);
void free_phys_contiguous(int filep);

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

//Declaration of the function and variables to print in file or screen
int print_screen; //0 save results into file, 1 print results in screen
FILE* f_print;//file to print the results in case file is selected
void print(const char *str, ...) {
    va_list args;
    va_start(args, str);
    if(print_screen==1)
    {
      vprintf(str, args);
    }
    else
    {
      vfprintf(f_print, str, args);
    }
    va_end(args);
}

int main(int argc, char **argv) {

  //---decide if the output should be saved in file or printed----//
  if (argc == 2) //file name provided in call
  {
    f_print = fopen( argv[1],  "w" );
    if( f_print  == NULL ) {
      printf( "ERROR: could not open %s...\n" , argv[1]);
      return( 1 );
    }
    printf("Results are saved in %s\n", argv[1]);
    print_screen = 0;
  }
  else
  {
    print_screen = 1;
  }

  print("---MEASURING FPGA-HPS BRIDGES SPEED IN BAREMETAL WITH FPGA DMACs--\n\r\r");
	#ifdef BRIDGES_32BIT
		#ifdef WR_HPS
			print("WRITING HPS THROUGH 32-BIT BRIDGES\n\r");
    #else
		  #ifdef RD_HPS
			   print("READING HPS THROUGH 32-BIT BRIDGES\n\r");
      #else
			print("CHOOSE RD OR WR!!\n\r");
			return 0;
      #endif
		#endif
	#endif
	#ifdef BRIDGES_64BIT
		#ifdef WR_HPS
			print("WRITING HPS THROUGH 64-BIT BRIDGES\n\r");
    #else
		  #ifdef RD_HPS
			   print("READING HPS THROUGH 64-BIT BRIDGES\n\r");
		  #else
			   print("CHOOSE RD OR WR!!\n\r");
			   return 0;
      #endif
		#endif
	#endif
	#ifdef BRIDGES_128BIT
		#ifdef WR_HPS
			print("WRITING HPS THROUGH 128-BIT BRIDGES\n\r");
    #else
		  #ifdef RD_HPS
			   print("READING HPS THROUGH 128-BIT BRIDGES\n\r");
		  #else
			   print("CHOOSE RD OR WR!!\n\r");
			   return 0;
      #endif
		#endif
	#endif

	print("Each measurement is repeated %d times\n\r", REP_TESTS);

  int i, j, l, k, h; //vars for loops

  //-----------some variables to define speed tests-------//
	//define data sizes
	int number_of_data_sizes = 21; //size of data_size[]
  int number_of_data_sizes_test; //from all, which ones to test??
	//data pointer to buffers in CPU memory
	char* data[5]; //alligned
	char* data_dmac[5];//data as seen by dmacs
  //file entry of an intermediate buffer in driver
  int f_intermediate_buff[5];
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

  //----Save intermediate results in speed tests-------//
  unsigned long long int total_clk, min_clk, max_clk, variance_clk,
  clk_read_average;
  unsigned long long int total_dma, min_dma, max_dma, var_dma;

  //define the size of the transfer for each DMAC (data payload is
  //distributed among the number of DMACs tested)
  //data_size_dmac[x][y] = x is the number of dmacs working, y data size
  int data_size_dmac[5][number_of_data_sizes];
  for (i=0; i<5; i++)
  {
    for (j=0; j<number_of_data_sizes; j++)
    {
      data_size_dmac[i][j] = max( MIN_TRANSFER_SIZE,
        (upper_aligned((float)data_size[j]/(float)(i+1), MIN_TRANSFER_SIZE)));
    }
  }

  for (i=0; i<5; i++)
	{
    print("\n\rdata_size_dmac[%d]=",i);
		for (j=0; j<number_of_data_sizes; j++)
		{
      print("%d,",data_size_dmac[i][j]);
		}
	}
  print("\n\r");

  //-----GENERATE ADRESSES TO ACCESS FPGA MEMORY AND DMACS FROM PROCESSOR-----//
  void *virtual_base;
  int fd;
  if( ( fd = open( "/dev/mem", ( O_RDWR | O_SYNC ) ) ) == -1 ) {
	  printf( "ERROR: could not open \"/dev/mem\"...\n" );
	  return( 1 );
  }

  virtual_base = mmap( NULL, MMAP_SPAN, ( PROT_READ | PROT_WRITE ),
    MAP_SHARED, fd, MMAP_BASE );

  if( virtual_base == MAP_FAILED ) {
	  printf( "ERROR: mmap() failed...\n" );
	  close( fd );
	  return( 1 );
  }


  //save virtual addresses of DMACs and fpga ocrs in vectors
  void* dma_addr[5] = 		{	virtual_base + (unsigned long) DMA_FPGA_HPS_BASE,
                            virtual_base + (unsigned long) DMA_FPGA_SDRAM_0_BASE,
                            virtual_base + (unsigned long) DMA_FPGA_SDRAM_1_BASE,
                            virtual_base + (unsigned long) DMA_FPGA_SDRAM_2_BASE,
                            virtual_base + (unsigned long) DMA_FPGA_SDRAM_3_BASE};
  void* fpga_ocr_addr[5] ={ virtual_base + (unsigned long) OCR_FPGA_HPS_BASE,
                            virtual_base + (unsigned long) OCR_FPGA_SDRAM_0_BASE,
                            virtual_base + (unsigned long) OCR_FPGA_SDRAM_1_BASE,
                            virtual_base + (unsigned long) OCR_FPGA_SDRAM_2_BASE,
                            virtual_base + (unsigned long) OCR_FPGA_SDRAM_3_BASE};


  //-----------INITIALIZATION OF PMU AS TIMER------------//
  pmu_init_ns(800, 1); //Initialize PMU cycle cntr, 800MHz src, freq divider 1
  pmu_counter_enable();//Enable cycle counter inside PMU (it starts counting)
  float pmu_res = pmu_getres_ns();
  print("PMU is used like timer with the following characteristics\n\r");
  print("PMU cycle counter resolution is %f ns\n\r", pmu_res );
  print("Measuring PMU counter overhead...\n\r");

  reset_cumulative(&total_clk, &min_clk, &max_clk, &variance_clk);
  int overflow = 0;
  unsigned long long pmu_counter_ns;
  for(i = 0; i<CLK_REP_TESTS+2; i++)
  {
    pmu_counter_reset();
    //simulate time lost when activating and reading a DMAC in FPGA
		fpga_dma_write_bit(dma_addr[0], FPGA_DMA_CONTROL, FPGA_DMA_GO, 0);
		fpga_dma_transfer_done(dma_addr[0]);
    overflow = pmu_counter_read_ns(&pmu_counter_ns);
    //printf("PMU counter (ns): %lld \n\r", pmu_counter_ns);
    if (overflow == 1){
      printf("Cycle counter overflow!! Program ended\n\r");return 1;}
    if (i>=2) update_cumulative(&total_clk, &min_clk, &max_clk, &variance_clk,
      0, pmu_counter_ns, 0);
    //(We erase two first measurements because they are different from the
      //others. Reason:Branch prediction misses when entering for loop)
  }
  print("PMU Cycle Timer Stats for %d consecutive reads\n\r", CLK_REP_TESTS);
  print("Average, Minimum, Maximum, Variance\n\r");
  print("%lld,%lld,%lld,%lld\n\r", clk_read_average = total_clk/CLK_REP_TESTS,
      min_clk, max_clk, variance (variance_clk , total_clk, CLK_REP_TESTS));


  //-------------CHECK AND RESET FPGA 1kB FPGA ON-CHIP RAMs-------------//
	//Check the FPGA-OCRs
  int status;
	for (i=FIRST_FPGA_OCR_CHECK; i<=LAST_FPGA_OCR_CHECK; i++)
	{
		status = check_mem(fpga_ocr_addr[i], FPGA_OCR_SIZE);
		if (status == 0)
			print("Check FPGA On-Chip RAM %d OK\n\r", i);
		else
		{
			printf ("Error when checking FPGA On-Chip RAM %d\n\r", i);
			return 1;
		}
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
  //Write data to the GPIO connected to AXI signals
  void *AXI_GPIO_vaddr_void = virtual_base
  + ((unsigned long)(GPIO_QSYS_ADDRESS) & (unsigned long)( MMAP_MASK ));
  uint32_t* AXI_GPIO_vaddr = (uint32_t *) AXI_GPIO_vaddr_void;
  printf("AXI_GPIO_vaddr=%x\n", (unsigned int)(*AXI_GPIO_vaddr));
  *AXI_GPIO_vaddr = AXI_SIGNALS;
  printf("AXI_GPIO_vaddr=%x\n", (unsigned int)(*AXI_GPIO_vaddr));

  //To do the transfers it is needed to enable access to PMU from user space,
  //configure ACP and remove FPGA-to-SDRAM ports from reset. This is done
  //by the Alloc_DMAble_buff_LKM driver, available at
  //https://github.com/robertofem/CycloneVSoC-examples/tree/soft_dma_baremetal/Linux-modules/Alloc_DMAble_buff_LKM
  //It must be inserted before using the program.

  //-----------MOVING DATA WITH DMAC------------//
  print("\n\r--MOVING DATA WITH THE DMACs--\n\r");

  #ifdef GENERATE_DUMMY_TRAFFIC_IN_CACHE
  print("Dummy traffic is generated to pollute cache\n\r");
  pthread_t t;
  pthread_create(&t, NULL, genereate_dummy_traffic, NULL);
  #endif

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

    //Allocate intermediate buffers in driver
    for (j=first_dmac_test; j<=last_dmac_test; j++)
    {
      //alloc the intermediate buffers used by the DMACs
      if (h==2) //If the transfer is through ACP
        data_dmac[j] = alloc_phys_contiguous(2*1024*1024,
          0, &(f_intermediate_buff[j])) + 0x80000000;
      else
        data_dmac[j] = alloc_phys_contiguous(2*1024*1024,
          1, &(f_intermediate_buff[j]));
    }

    //Moving data with DMAC (DMAC program preparation is measured)
    printf("data sizes test = %d\n\r", number_of_data_sizes_test);
	  printf("Data Size, Average, Min, Max, Variance\n\r");
    for(i=0; i<number_of_data_sizes_test; i++)//for each data size
	  {
		  reset_cumulative( &total_dma, &min_dma, &max_dma, &var_dma);

      //reserve buffers needed to do the transfers
      for (j=first_dmac_test; j<=last_dmac_test; j++)
      {
        //alloc the buffer in the processor
        data[j] = (char*) malloc(data_size_dmac[number_dmacs_test-1][i]);
        if (data[j] == 0)
        {
          printf("ERROR when calling malloc: Out of memory \n\r");
          return 1;
        }
      }

		  for(l = 0; l<REP_TESTS+2; l++)
		  {
				//clear destiny and fill source
				for (j=first_dmac_test; j<=last_dmac_test; j++)
				{
					for (k= 0; k<data_size_dmac[number_dmacs_test-1][i]; k++)
							((char*)(data[j]))[k] = (char) 3;
          for (k= 0; k<MIN_TRANSFER_SIZE;k++)
							((char*)(fpga_ocr_addr[j]))[k] = (char) 5;
          //printbuff(DMA_SRC_UP[j], 32);
          //printbuff(DMA_DST_UP[j], 32);
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
        //Copy data to intermediate buffer if RD operation
        #ifdef RD_HPS
        for (j=first_dmac_test; j<=last_dmac_test; j++)
        {
          write(f_intermediate_buff[j], data[j], data_size_dmac[number_dmacs_test-1][i]);
        }
        #endif
        //start DMACs
        for (j=first_dmac_test; j<=last_dmac_test; j++)
				{
					fpga_dma_start_transfer(dma_addr[j]);
				}
				// Wait for each dmac to finish its transfer
				for (j=first_dmac_test; j<=last_dmac_test; j++)
				{
					while(fpga_dma_transfer_done(dma_addr[j])==0);
				}
        // Copy from intermediate buffers to data[] if WR
        #ifdef WR_HPS
        for (j=first_dmac_test; j<=last_dmac_test; j++)
				{
          read(f_intermediate_buff[j], data[j], data_size_dmac[number_dmacs_test-1][i]);
        }
        #endif
				overflow = pmu_counter_read_ns(&pmu_counter_ns);
				if (overflow == 1){printf("Cycle counter overflow!! Program ended\n\r");
        	return 1;}

        //printf("PMU counter (ns): %lld \n\r", pmu_counter_ns);

				if (l>=2) update_cumulative(&total_dma, &min_dma, &max_dma,
			    &var_dma, 0, pmu_counter_ns, clk_read_average);
				//(We erase two first measurements because they are different from the
			  //others. Reason:Branch prediction misses when entering for loop)

				//check the content of the data just read
				// Compare results
				for (j=first_dmac_test; j<=last_dmac_test; j++)
				{
          //printbuff(DMA_SRC_UP[j], 32);
          //printbuff(DMA_DST_UP[j], 32);
					if(0  != memcmp(DMA_SRC_UP[j], DMA_DST_UP[j], 2))
					{
						printf("DMA src and dst have different data!! Program ended\n\r");
				    return 1;
					}
				}

			}//for rep tests
			printf("%d, %lld, %lld, %lld, %lld\n\r",
	    data_size[i], total_dma/REP_TESTS, min_dma, max_dma,
	    variance(var_dma, total_dma, REP_TESTS));
		}//for data_sizes
    //free dynamic memory
    for (j=first_dmac_test; j<=last_dmac_test; j++)
    {
      free_phys_contiguous(f_intermediate_buff[j]);
    }
	}//for h

  if (print_screen == 0) fclose (f_print);

	print("\n\rData transfer measurements finished!!!!\n\r");

  return 0;

}//end main

void printbuff(char* buff, int size)
{
  int i;
  printf("[");
  for (i=0; i<size; i++)
  {
    printf("%u",buff[i]);
    if (i<(size-1)) printf(",");
  }
  printf("]");
  printf("\n");
}

int check_mem(void* addr, int size)
{
	char* ptr = (char*) addr;
	int i;
	for (i=0; i<(size); i++)
	{
		*ptr = (char)i;
		if (*ptr != (char) i)
		{
			return -1;
		}
		ptr++;
	}
	return 0;
}

int reset_mem(void* addr, int size)
{
	char* ptr = (char*) addr;
	int i;
	for (i=0; i<(size); i++)
	{
		*ptr = 0;
		if (*ptr != 0) return -1;
		ptr++;
	}
	return 0;
}

int upper_aligned(int number, int align)
{
  int partial = (int) ceil(((float)number)/((float)align));
  return (partial * align);
}

void genereate_dummy_traffic()
{
  char* dummydata = (char*) malloc(2*1024*1024); //2MB
  int i_dummy;
  while(1)
  {
    i_dummy++; if (i_dummy==2*1024*1024) i_dummy = 0;
    dummydata[i_dummy] = dummydata[i_dummy]+1;
  }
  free(dummydata);
}

//Allocate a physically continuous buffer using the driver
//returns physical address (for DMA) of the physically contiguous buffer
//size is the desired size for the buffer
//uncached = 0 (cached, for ACP access) or 1 (uncached, for SDRAMC access)
//filep, return value of the file identifier of the open buffer (needed to free
//the buffer using free_phys_contiguous()
uint32_t buffer_to_use = 0;
void* alloc_phys_contiguous (size_t size, int uncached, int* filep)
{
  //Set size
  int f_sysfs;
  char d[14];
  //printf("\nConfig. dmable_buff module using sysfs entries in /sys/alloc_dmable_buffers\n");
  sprintf(d, "%u", (uint32_t) size);
  char file_name[100];
  sprintf(file_name, "/sys/alloc_dmable_buffers/attributes/buff_size[%u]", buffer_to_use);
  f_sysfs = open(file_name, O_WRONLY);
  if (f_sysfs < 0){
    printf("Failed to open sysfs for buff_size.\n");
    return 0;
  }
  write (f_sysfs, &d, 14);
  close(f_sysfs);
  //Set cacheable (goes through ACP)
  sprintf(d, "%u", (uint32_t) uncached);
  sprintf(file_name, "/sys/alloc_dmable_buffers/attributes/buff_uncached[%u]", buffer_to_use);
  f_sysfs = open(file_name, O_WRONLY);
  if (f_sysfs < 0){
    printf("Failed to open sysfs for buff_uncached.\n");
    return 0;
  }
  write (f_sysfs, &d, 14);
  close(f_sysfs);
  //Open the entry to allocate the buffer
  sprintf(file_name, "/dev/dmable_buff%u", buffer_to_use);
  int f_intermediate_buff = open(file_name,O_RDWR);
  if (f_intermediate_buff < 0){
    printf("Failed to open /dev/dmable_buff%u...", buffer_to_use);
    return 0;
  }
  //get physical address
  sprintf(d, "%u", (uint32_t) 0);
  sprintf(file_name, "/sys/alloc_dmable_buffers/attributes/phys_buff[%u]", buffer_to_use);
  f_sysfs = open(file_name, O_RDONLY);
  if (f_sysfs < 0){
    printf("Failed to open sysfs for phys_buff.\n");
    return 0;
  }
  read(f_sysfs, d, 14);
  //fgets(d, 14, )
  unsigned int intermediate_buff_phys = 0;
  //printf("d=%s, phys=%x", d, intermediate_buff_phys);
  intermediate_buff_phys = strtoul (d, NULL, 0);
  //printf("d=%s, phys=%x", d, intermediate_buff_phys);
  close(f_sysfs);

  //return the file identifier
  *filep = f_intermediate_buff;
  //Increase the buffer to use next
  buffer_to_use++;

  //return the physical address
  return (void*) intermediate_buff_phys;
}

void free_phys_contiguous(int filep)
{
  close(filep);
  buffer_to_use--;
}
