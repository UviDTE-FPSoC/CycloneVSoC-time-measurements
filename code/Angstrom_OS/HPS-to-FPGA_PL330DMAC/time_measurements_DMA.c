//file: time_measuremnts_DMA.c
//performs time measurements of data transfers between processor and FPGA using
//the DMA controller PL330 available in HPS of Cyclone V SoC.Time is measured
//with the Performance Monitoring Unit (PMU)

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <stdint.h>
#include <stdarg.h>
#ifdef GENERATE_DUMMY_TRAFFIC_IN_CACHE
#include <pthread.h>
#endif

#include "configuration.h"
#include "pmu.h" //to measure time with PMU
#include "statistics.h" //do some statistics (mean, min, max, variance)

//Declaration of some extra functions
void printbuff(char* buff, int size);
//changes L2C controller lockdown by master configuration
int set_L2C_lockdown(int lockdown_cpu, int lockdown_acp);
//modify port priority and round-robin scheduler weights in SDRAM controller
int set_sdramc_priorities(unsigned int  mppriority);
int set_sdramc_weights(unsigned int  mpweight_0_4,
  unsigned int mpweight_1_4);
//generates dummy traffic in cache to slow down transfers
void genereate_dummy_traffic();


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
  int i,j,l,k; //for loops
  int f; //file pointer to open /dev/dma_pl330
  int ret; //return value for wr and rd /dev/dma_pl330
  int use_acp, prepare_microcode_in_open; //to config of DMA_PL330_LKM

  //----Save intermediate results in speed tests-------//
  unsigned long long int total_clk, min_clk, max_clk, variance_clk,
  clk_read_average;

  unsigned long long int total_dma_rd, min_dma_rd, max_dma_rd, variance_dma_rd;
  unsigned long long int total_dma_wr, min_dma_wr, max_dma_wr, variance_dma_wr;

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

  print("---------DMA TIME MEASUREMENTS IN ANGSTROM---------\n");
  print("Each measurement is repeated %d times\n\n", REP_TESTS);

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


  //-------GENERATE ADRESSES TO ACCESS FPGA MEMORY FROM PROCESSOR---------//
  // map the address space for the LED registers into user space so we can
  //interact with them. we'll actually map in the entire CSR span of the HPS
  //since we want to access various registers within that span
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

  //virtual address of the FPGA buffer
  void *on_chip_RAM_vaddr_void = virtual_base
  + ((unsigned long)(ON_CHIP_MEMORY_BASE) & (unsigned long)( MMAP_MASK ));
  uint8_t* on_chip_RAM_vaddr = (uint8_t *) on_chip_RAM_vaddr_void;


  //-----------CHECK IF THE FPGA OCR IS ACCESSIBLE AND RESET IT----------//
  //Check the on-chip RAM memory in the FPGA
  uint8_t* ocr_ptr = on_chip_RAM_vaddr;
  for (i=0; i<ON_CHIP_MEMORY_SPAN; i++)
  {
    *ocr_ptr = (uint8_t) i;
    if (*ocr_ptr != (uint8_t)i)
    {
      printf ("Error when checking On-Chip RAM in Byte %d\n", i);
      return 0;
    }
    ocr_ptr++;
  }
  print("Check On-Chip RAM OK\n");

  //Reset all memory
  ocr_ptr = on_chip_RAM_vaddr;
  for (i=0; i<ON_CHIP_MEMORY_SPAN; i++)
  {
    *ocr_ptr = 0;
    if (*ocr_ptr != 0)
    {
      printf ("Error when resetting On-Chip RAM in Byte %d\n", i);
      return 0;
    }
    ocr_ptr++;
  }
  print("Reset On-Chip RAM OK\n");

  //-----------MOVING DATA WITH DMAC------------//
  print("\n------MOVING DATA WITH THE DMA_PL330 driver-----\n");

  #ifdef GENERATE_DUMMY_TRAFFIC_IN_CACHE
  print("Dummy traffic is generated to pollute cache\n\r");
  pthread_t t;
  pthread_create(&t, NULL, genereate_dummy_traffic, NULL);
  #endif

  //-----Modify lockdown in L2 cache controller--//
  #ifdef EN_LOCKDOWN_STUDY
  int h;
  int lockdown_cpu, lockdown_acp;
  for (h=0; h<7; h++)
  {
    switch(h)
    {
    case 0:
      print("\n\rNO LOCKDOWN\n\r");
      lockdown_cpu = 0b00000000;
      lockdown_acp = 0b00000000;
      break;
    case 1:
      print("\n\rLOCK CPUS 1 way\n\r");
      lockdown_cpu = 0b00000001;
      lockdown_acp = 0b00000000;
      break;
    case 2:
      print("\n\rLOCK CPUS 4 ways\n\r");
      lockdown_cpu = 0b00001111;
      lockdown_acp = 0b00000000;
      break;
    case 3:
      print("\n\rLOCK CPUS 7 ways\n\r");
      lockdown_cpu = 0b01111111;
      lockdown_acp = 0b00000000;
      break;
    case 4:
      print("\n\rLOCK CPUS 1 way and ACP the other 7 ways\n\r");
      lockdown_cpu = 0b00000001;
      lockdown_acp = 0b11111110;
      break;
    case 5:
      print("\n\rLOCK CPUS 4 ways and ACP the other 4 ways\n\r");
      lockdown_cpu = 0b00001111;
      lockdown_acp = 0b11110000;
      break;
    case 6:
      print("\n\rLOCK CPUS 7 ways and ACP the other way\n\r");
      lockdown_cpu = 0b01111111;
      lockdown_acp = 0b10000000;
    default:
      break;
    }
    #ifndef LOCK_AFTER_CPU_GENERATES_TRANSFER_DATA
    printf("Lock at the beginning of the program\n\r");
    if (set_L2C_lockdown(lockdown_cpu, lockdown_acp)!=0) return 1;
    #else
    printf("Lock after CPU generates the data\n\r");
    #endif
  #endif

  #ifdef EN_SDRAMC_STUDY
  int g;
  for (g=0; g<7; g++)
  {
    switch(g)
    {
    case 0:
      print("\n\rSDRAM DEFAULT CONFIGURATION\n\r");
      break;
    case 1:
      print("\n\rSDRAM SAME PRIORITIES AND WEIGHTS ALL PORTS\n\r");
      set_sdramc_priorities(0x0);
      set_sdramc_weights(0x40000000, 0x2108);//same weights (1)
      break;
    case 2:
      print("\n\rSDRAM SAME PRIORITIES L3 2x WEIGHT\n\r");
      set_sdramc_priorities(0x0);
      set_sdramc_weights(0x80000000, 0x2208);
      break;
    case 3:
      print("\n\rSDRAM SAME PRIORITIES L3 4x WEIGHT\n\r");
      set_sdramc_priorities(0x0);
      set_sdramc_weights(0x0, 0x2409);
      break;
    case 4:
      print("\n\rSDRAM SAME PRIORITIES L3 8x WEIGHT\n\r");
      set_sdramc_priorities(0x0);
      set_sdramc_weights(0x0, 0x280A);
      break;
    case 5:
      print("\n\rSDRAM SAME PRIORITIES L3 16x WEIGHT\n\r");
      set_sdramc_priorities(0x0);
      set_sdramc_weights(0x0, 0x300C);
      break;
    case 6:
      print("\n\rSDRAM L3 MORE PRIORITY\n\r");
      set_sdramc_priorities(0x71C0000);
      set_sdramc_weights(0x40000000, 0x2108);//same weights (1)
      break;
    default:
      break;
    }
  #endif

  //Configure the hardware address of the buffer to use in DMA transactions
  //(the On-Chip RAM in FPGA) using sysfs
  print("\nConfig. DMA_PL330 module using sysfs entries in /sys/dma_pl330\n");

  int f_sysfs;
  char d[14];
  sprintf(d, "%u", (uint32_t) DMA_BUFF_PADD);
  f_sysfs = open("/sys/dma_pl330/pl330_lkm_attrs/dma_buff_padd", O_WRONLY);
  if (f_sysfs < 0)
  {
    printf("Failed to open sysfs for dma_buff_padd.\n");
    return errno;
  }
  write (f_sysfs, &d, 14);
  close(f_sysfs);

  for(k=0; k<4; k++)//for each configuration of the DMA_PL330 driver
  {
    //Select configuration
    switch(k)
    {
      case 0:
        use_acp = 0;
        prepare_microcode_in_open = 0;
        print("\n--DO NOT USE ACP, DO NOT REPARE DMAC MICROCODE IN OPEN--\n");
        break;
      case 1:
        use_acp = 0;
        prepare_microcode_in_open = 1;
        print("\n--DO NOT USE ACP, PREPARE DMAC MICROCODE IN OPEN--\n");
        break;
      case 2:
        use_acp = 1;
        prepare_microcode_in_open = 0;
        print("\n--USE ACP, DO NOT PREPARE DMAC MICROCODE IN OPEN--\n");
        break;
      case 3:
        use_acp = 1;
        prepare_microcode_in_open = 1;
        print("\n--USE ACP, PREPARE DMAC MICROCODE IN OPEN--\n");
        break;
    }

    //Apply configuration to driver
    sprintf(d, "%d", (int) use_acp);
    f_sysfs = open("/sys/dma_pl330/pl330_lkm_attrs/use_acp", O_WRONLY);
    if (f_sysfs < 0)
    {
      printf("Failed to open sysfs for use_acp.\n");
      return errno;
    }
    write (f_sysfs, &d, 14);
    close(f_sysfs);

    sprintf(d, "%d", (int) prepare_microcode_in_open);
    f_sysfs = open("/sys/dma_pl330/pl330_lkm_attrs/prepare_microcode_in_open",
      O_WRONLY);
    if (f_sysfs < 0)
    {
      printf("Failed to open sysfs for prepare_microcode_in_open.\n");
      return errno;
    }
    write (f_sysfs, &d,14);
    close(f_sysfs);

    print("Data Size, Avg DMA_WR, Min DMA_WR,Max DMA_WR, Var DMA_WR, ");
    print("Avg DMA_RD, Min DMA_RD, Max DMA_RD, Var DMA_RD\n");

    for(i=0; i<number_of_data_sizes; i++)//for each data size
    {
      //reset variables for statistics
      reset_cumulative(&total_dma_wr,&min_dma_wr,&max_dma_wr, &variance_dma_wr);
      reset_cumulative(&total_dma_rd,&min_dma_rd,&max_dma_rd, &variance_dma_rd);

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

      //configure the data size for the driver (only applies when microcode
      //is generated in open)
      sprintf(d, "%d", (int) data_in_one_operation);
      f_sysfs = open("/sys/dma_pl330/pl330_lkm_attrs/dma_transfer_size",
        O_WRONLY);
      if (f_sysfs < 0){
        printf("Failed to open sysfs for dma_transfer_size.\n");
        return errno;
      }
      write (f_sysfs, &d, 14);
      close(f_sysfs);

      //Open DMA driver to write and read from it
      f=open("/dev/dma_pl330",O_RDWR);
      if (f < 0)
      {
        perror("Failed to open /dev/dma_pl330...");
        return errno;
      }

      for(l = 0; l<REP_TESTS+2; l++)
      {
        //reserve the exact memory of the data size
        data = (char*) malloc(data_size[i]);
        if (data == 0)
        {
          printf("ERROR when calling malloc: Out of memory\n");
          return 1;
        }

        //fill uP memory with some data
        //save some content in data (for example: i)
        #ifdef LOCK_AFTER_CPU_GENERATES_TRANSFER_DATA
        //permit CPU to use all cache to save data
        if (set_L2C_lockdown(0, 0)!=0) return 1;
        #endif
			  for(j=0; j<data_size[i]; j++) data[j] = i+1;
        #ifdef LOCK_AFTER_CPU_GENERATES_TRANSFER_DATA
        //apply lockdown for the transfer
        if (set_L2C_lockdown(lockdown_cpu, lockdown_acp)!=0) return 1;
        #endif

        //--WRITE DATA TO FPGA ON-CHIP RAM
        pmu_counter_reset();
        for (j=0; j<operation_loops; j++)
        {
          ret = write(f, &data[j*ON_CHIP_MEMORY_SPAN], data_in_one_operation);
          if (ret < 0){
            perror("Failed to write in /dev/dma_pl330.");
            return errno;
          }
        }
        overflow = pmu_counter_read_ns(&pmu_counter_ns);
        if (overflow == 1){printf("Cycle counter overflow!! \n");return 1;}

        if (l>=2) update_cumulative(&total_dma_wr, &min_dma_wr, &max_dma_wr,
          &variance_dma_wr, 0, pmu_counter_ns, clk_read_average);
        //(We erase two first measurements because they are different from the
          // others. Reason:Branch prediction misses when entering for loop)

        //check the content of the data just read
        // Compare results
        if(0  != memcmp(&(data[0]), on_chip_RAM_vaddr_void,
          data_in_one_operation))
        {
          printf("DMA src and dst have different data on WR!!\n");return 1;
        }


        //--READ DATA FROM FPGA ON-CHIP RAM
        pmu_counter_reset();
        for (j=0; j<operation_loops; j++)
        {
          ret = read(f, &data[j*ON_CHIP_MEMORY_SPAN], data_in_one_operation);
          if (ret < 0){
            perror("Failed to read from /dev/dma_pl330.");
            return errno;
          }
        }
        overflow = pmu_counter_read_ns(&pmu_counter_ns);
        if (overflow == 1){printf("Cycle counter overflow!! \n");return 1;}

        if (l>=2) update_cumulative(&total_dma_rd, &min_dma_rd, &max_dma_rd,
          &variance_dma_rd, 0, pmu_counter_ns, clk_read_average);
        //(We erase two first measurements because they are different from the
          // others. Reason:Branch prediction misses when entering for loop)

        //check the content of the data just read
        // Compare results
        if(0  != memcmp(&(data[0]), on_chip_RAM_vaddr_void,
          data_in_one_operation))
        {
          printf("DMA src and dst have different data on WR!!\n");return 1;
        }

        //free dynamic memory
        free(data);
      }
      close(f);
      print("%d, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld\n\r",
          data_size[i], total_dma_wr/REP_TESTS, min_dma_wr, max_dma_wr,
          variance(variance_dma_wr, total_dma_wr, REP_TESTS),
          total_dma_rd/REP_TESTS, min_dma_rd, max_dma_rd,
          variance(variance_dma_rd, total_dma_rd, REP_TESTS) );
    }
  }

  #ifdef EN_SDRAMC_STUDY
  }
  #endif

  #ifdef EN_LOCKDOWN_STUDY
  }
  #endif

	// --------------clean up our memory mapping and exit -----------------//
	if( munmap( virtual_base, MMAP_SPAN ) != 0 )
  {
		printf( "ERROR: munmap() failed...\n" );
		close( fd );
		return( 1 );
	}

	close( fd );

  if (print_screen == 0) fclose (f_print);

  printf("All tests were successful!! End...\n");

	return( 0 );
}

//-----------------FUNCTION TO PRINT BUFFERS IN SCREEN------------------------//
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

//-------FUNCTION TO CHANGE L2 CONTROLLER LOCKDOWN BY MASTER SETTINGS------//
int set_L2C_lockdown(int lockdown_cpu, int lockdown_acp)
{
  int f_sysfs;
  char d[14];

  sprintf(d, "%d", (int) lockdown_cpu);
  f_sysfs = open("/sys/dma_pl330/pl330_lkm_attrs/lockdown_cpu", O_WRONLY);
  if (f_sysfs < 0)
  {
    printf("Failed to open sysfs for lockdown_cpu.\n");
    return errno;
  }
  write (f_sysfs, &d, 14);
  close(f_sysfs);
  sprintf(d, "%d", (int) lockdown_acp);
  f_sysfs = open("/sys/dma_pl330/pl330_lkm_attrs/lockdown_acp", O_WRONLY);
  if (f_sysfs < 0)
  {
    printf("Failed to open sysfs for lockdown_acp.\n");
    return errno;
  }
  write (f_sysfs, &d, 14);
  close(f_sysfs);
  return 0;
}

int set_sdramc_priorities(unsigned int  mppriority)
{
  int f_sysfs;
  char d[21];

  sprintf(d, "%u", (int) mppriority);
  f_sysfs = open("/sys/dma_pl330/pl330_lkm_attrs/sdramc_priority", O_WRONLY);
  if (f_sysfs < 0)
  {
    printf("Failed to open sysfs for mppriority.\n");
    return errno;
  }
  write (f_sysfs, &d, 21);
  close(f_sysfs);
  return 0;
}
int set_sdramc_weights(unsigned int  mpweight_0_4,
  unsigned int mpweight_1_4)
{
  int f_sysfs;
  char d[21];

  sprintf(d, "%u", (int) mpweight_0_4);
  f_sysfs = open("/sys/dma_pl330/pl330_lkm_attrs/sdramc_weight0", O_WRONLY);
  if (f_sysfs < 0)
  {
    printf("Failed to open sysfs for mpweight_0_4.\n");
    return errno;
  }
  write (f_sysfs, &d, 21);
  close(f_sysfs);

  sprintf(d, "%u", (int) mpweight_1_4);
  f_sysfs = open("/sys/dma_pl330/pl330_lkm_attrs/sdramc_weight1", O_WRONLY);
  if (f_sysfs < 0)
  {
    printf("Failed to open sysfs for mpweight_1_4.\n");
    return errno;
  }
  write (f_sysfs, &d, 21);
  close(f_sysfs);
  return 0;
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
