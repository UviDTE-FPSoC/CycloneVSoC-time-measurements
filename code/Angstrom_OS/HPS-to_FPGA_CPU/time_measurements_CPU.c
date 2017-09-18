//file: time_measuremnts_CPU.c
//performs time measurements of data transfers between processor and FPGA using
//the DMA controller PL330 available in HPS of Cyclone V SoC.Time is measured
//with the Performance Monitoring Unit (PMU)

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <math.h>
#include <stdarg.h>

#define soc_cv_av
#include "configuration.h"
#include "statistics.h" //do some statistics (mean, min, max, variance)
#include <time.h> //time measuremnt wit OS resources
#include "pmu.h" //time measurements wit Performance Monitoring Unit (PMU)

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
	void *virtual_base;
	int fd;
	void *on_chip_RAM_addr_void;
	int i, j, k, l, error;
	UINT_SOC* ocr_ptr;//on chip RAM pointer

  //---------Save intermediate results in speed tests------------//
  unsigned long long int total_clk, min_clk, max_clk, var_clk, clk_read_avrg;

  unsigned long long int total_mcp_rd, min_mcp_rd, max_mcp_rd, var_mcp_rd;
  unsigned long long int total_mcp_wr, min_mcp_wr, max_mcp_wr, var_mcp_wr;

  unsigned long long int total_dir_rd, min_dir_rd, max_dir_rd, var_dir_rd;
  unsigned long long int total_dir_wr, min_dir_wr, max_dir_wr, var_dir_wr;

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

  UINT_SOC* datai; //pointer to data
	int data_in_one_operation_w;//data to write or read from memory (words)
	int operation_w_loops; //number of loops to read/write all data from/to memory

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

	print("---------CPU TIME MEASUREMENTS IN ANGSTROM---------\n");
	#ifdef ON_CHIP_RAM_ON_LIGHTWEIGHT
		print("TESTING ON_CHIP_RAM_ON_LIGHTWEIGHT\n");
		print("Size of uint32_soc is %d\n", sizeof(uint32_soc));
	#endif //ON_CHIP_RAM_ON_LIGHTWEIGHT
	#ifdef ON_CHIP_RAM_ON_HFBRIDGE32
		print("TESTING ON_CHIP_RAM_ON_HFBRIDGE32\n");
		print("Size of uint32_soc is %d\n", sizeof(uint32_soc));
	#endif //ON_CHIP_RAM_ON_HFBRIDGE32
	#ifdef ON_CHIP_RAM_ON_HFBRIDGE64
		print("TESTING ON_CHIP_RAM_ON_HFBRIDGE64\n");
		print("Size of uint64_soc is %d\n", sizeof(uint64_soc));
	#endif //ON_CHIP_RAM_ON_HFBRIDGE64
	#ifdef ON_CHIP_RAM_ON_HFBRIDGE128
		print("Size of uint128_t is %d\n", sizeof(uint128_soc));
		print("TESTING ON_CHIP_RAM_ON_HFBRIDGE128\n");
	#endif //ON_CHIP_RAM_ON_HFBRIDGE128


	//---------GENERATE ADRESSES----------//
	// Map FPGA On-chip memory. It is the element in the FPGA to do transfers with
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

	on_chip_RAM_addr_void = virtual_base +
		( ( unsigned long  )( BRIDGE_ADRESS_MAP_START +  ON_CHIP_MEMORY_BASE)
		& ( unsigned long)( MMAP_MASK ) );
	UINT_SOC* on_chip_RAM_addr = (UINT_SOC *) on_chip_RAM_addr_void;

	//---------------------------TESTS USING TIME.H-----------------------------//
	print("\n------------TESTS USING TIME.H------------\n");

	//-----------CLOCK INIT------------//
	clockid_t clk = CLOCK_REALTIME;
	struct timespec clk_struct_begin, clk_struct_end;

	 if (clock_getres(clk, &clk_struct_begin))
		 print("Failed in checking CLOCK_REALTIME resolution");
	 else
		 print("CLOCK_REALTIME resolution is %ld ns\n", clk_struct_begin.tv_nsec );

	print("Measuring clock read overhead\n");
	reset_cumulative(&total_clk, &min_clk, &max_clk, &var_clk);
	clock_gettime(clk, &clk_struct_begin);
		for(i = 0; i<CLK_REP_TESTS; i++)
	{
		clock_gettime(clk, &clk_struct_begin);
		clock_gettime(clk, &clk_struct_end);

		update_cumulative(&total_clk, &min_clk, &max_clk, &var_clk,
			clk_struct_begin.tv_nsec, clk_struct_end.tv_nsec, 0);
	}
	print("Clock Statistics for %d consecutive reads\n", CLK_REP_TESTS);
	print("Average, Minimum, Maximum, Variance\n");
	print("%lld,%lld,%lld,%lld\n", clk_read_avrg = total_clk/CLK_REP_TESTS,
		min_clk, max_clk, variance (var_clk , total_clk, CLK_REP_TESTS));


	//---------READ AND WRITE ON-CHIP RAM-------//

	//Check the memory
	ocr_ptr = on_chip_RAM_addr;
	error = 0;
	for (i=0; i<MEMORY_SIZE_IN_WORDS; i++)
	{
		*ocr_ptr = i;
		if (*ocr_ptr != i) error = 1;
		ocr_ptr++;
	}
	if (error == 0) print("Check On-Chip RAM OK\n");
	else
	{
		printf ("Error when checking On-Chip RAM\n");
		return 1;
	}

	//Reset all memory
	ocr_ptr = on_chip_RAM_addr;
	error = 0;
	for (i=0; i<MEMORY_SIZE_IN_WORDS; i++)
	{
		*ocr_ptr = 0;
		if (*ocr_ptr != 0) error = 1;
		ocr_ptr++;
	}
	if (error == 0) print("Reset On-Chip RAM OK\n");
	else
	{
		printf ("Error when resetting On-Chip RAM \n");
		return 0;
	}

	//----------Do the speed test (using memcpy)--------------//
	print("\nStart memcpy test\n");

	print("Data Size, Avg DMA_WR, Min DMA_WR,Max DMA_WR, Var DMA_WR, ");
	print("Avg DMA_RD, Min DMA_RD, Max DMA_RD, Var DMA_RD\n");

	for(i=0; i<number_of_data_sizes; i++)//for each data size
	{
		reset_cumulative( &total_mcp_wr, &min_mcp_wr, &max_mcp_wr, &var_mcp_wr);
		reset_cumulative( &total_mcp_rd, &min_mcp_rd, &max_mcp_rd, &var_mcp_rd);

		for(l = 0; l<REP_TESTS; l++)
		{
			//reserve the exact memory of the data size
			data = (char*) malloc(data_size[i]);
			if (data == 0)
			{
				printf("ERROR when calling malloc: Out of memory\n");
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

			//read a value from clock to eliminate "first-read-jitter"
			clock_gettime(clk, &clk_struct_begin);

			clock_gettime(clk, &clk_struct_begin);

			for (j=0; j<operation_loops; j++)
			memcpy((void*) on_chip_RAM_addr,
				(void *) (&(data[j*ON_CHIP_MEMORY_SPAN])), data_in_one_operation);

			clock_gettime(clk, &clk_struct_end);

			update_cumulative(&total_mcp_wr, &min_mcp_wr, &max_mcp_wr, &var_mcp_wr,
				clk_struct_begin.tv_nsec, clk_struct_end.tv_nsec, clk_read_avrg);

			//READ DATA FROM ON-CHIP RAM

			clock_gettime(clk, &clk_struct_begin);
			for (j=0; j<operation_loops; j++)
			memcpy((void *) (&(data[j*ON_CHIP_MEMORY_SPAN])),
				(void*) on_chip_RAM_addr, data_in_one_operation);
			clock_gettime(clk, &clk_struct_end);

			update_cumulative(&total_mcp_rd, &min_mcp_rd, &max_mcp_rd, &var_mcp_rd,
				clk_struct_begin.tv_nsec, clk_struct_end.tv_nsec, clk_read_avrg);
			//check the content of the data just read
			error = 0;
			for (j=0; j<data_size[i]; j++)
			{
				if (data[j] != i) error = 1;
			}
			if (error != 0)
			{
				printf ("Error checking On-Chip RAM data size %dB\n", data_size[i]);
				return 0;
			}
//			else
//				printf("Check On-Chip RAM with data size %dB was OK\n", data_size[i]);


			//free dynamic memory
			free(data);

		}
		print("%d, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld\n", data_size[i],
		total_mcp_wr/REP_TESTS, min_mcp_wr, max_mcp_wr,
		variance(var_mcp_wr, total_mcp_wr, REP_TESTS), total_mcp_rd/REP_TESTS,
		min_mcp_rd, max_mcp_rd,  variance(var_mcp_rd, total_mcp_rd, REP_TESTS) );
	}


	//----------Do the speed test (using for loop)--------------//
	print("\nStart for loop test\n");
	print("Data Size, Avg DMA_WR, Min DMA_WR,Max DMA_WR, Var DMA_WR, ");
	print("Avg DMA_RD, Min DMA_RD, Max DMA_RD, Var DMA_RD\n");

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
				printf("ERROR when calling malloc: Out of memory\n");
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
			clock_gettime(clk, &clk_struct_begin);

			clock_gettime(clk, &clk_struct_begin);

			int offset;
			for (j=0; j<operation_w_loops; j++)
			{
				offset = j*MEMORY_SIZE_IN_WORDS;
				for(k=0; k<data_in_one_operation_w; k++)
					on_chip_RAM_addr[k] = datai[offset+k];
			}
			clock_gettime(clk, &clk_struct_end);

			update_cumulative(&total_dir_wr, &min_dir_wr, &max_dir_wr, &var_dir_wr,
				clk_struct_begin.tv_nsec, clk_struct_end.tv_nsec, clk_read_avrg);

			//READ DATA FROM ON-CHIP RAM

			clock_gettime(clk, &clk_struct_begin);

			for (j=0; j<operation_w_loops; j++)
			{
				offset = j*MEMORY_SIZE_IN_WORDS;
				for(k=0; k<data_in_one_operation_w; k++)
					datai[offset+k] = on_chip_RAM_addr[k];
			}

			clock_gettime(clk, &clk_struct_end);
			update_cumulative(&total_dir_rd, &min_dir_rd, &max_dir_rd, &var_dir_rd,
				clk_struct_begin.tv_nsec, clk_struct_end.tv_nsec, clk_read_avrg);

			//check the content of the data just read
			error = 0;
			for (j=0; j<data_size[i]/BYTES_DATA_BUS; j++)
			{
				if (datai[j] != i) error = 1;
			}
			if (error != 0)
			{
				printf ("Error checking On-Chip RAM data size %dB\n", data_size[i]);
				return 0;

			}
			//else printf("Check On-Chip RAM with data size %dB was OK\n", data_size[i]);

			//free memory used
			free(datai);
		}

		print("%d, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld\n", data_size[i],
		total_dir_wr/REP_TESTS, min_dir_wr, max_dir_wr,
		variance(var_dir_wr, total_dir_wr, REP_TESTS),
		total_dir_rd/REP_TESTS, min_dir_rd, max_dir_rd,
		variance(var_dir_rd, total_dir_rd, REP_TESTS) );
	}
	//--------------------- END OF TESTS USING TIME.H---------------------------//



	//---------------------------TESTS USING PMU--------------------------------//
	print("\n------------TESTS USING PMU------------\n");
	unsigned long long pmu_counter_ns;
	int overflow = 0;

	//-----------CLOCK INIT------------//
	//Initialize PMU cycle counter, 800MHz source, frequency divider 64
	pmu_init_ns(800, 1);
	//Enable cycle counter inside PMU (it starts counting)
	pmu_counter_enable();
	float pmu_res = pmu_getres_ns();
	print("PMU cycle counter resolution is %f ns\n", pmu_res );

	print("Measuring clock read overhead\n");
	reset_cumulative(&total_clk, &min_clk, &max_clk, &var_clk);

	for(i = 0; i<CLK_REP_TESTS+2; i++)
	{
		pmu_counter_reset();
		overflow = pmu_counter_read_ns(&pmu_counter_ns);
		//printf("PMU counter (ns): %lld \n", pmu_counter_ns);
		if (overflow == 1){
			printf("Cycle counter overflow!! Program ended\n");
			return 1;
		}
		if (i>=2) update_cumulative(&total_clk, &min_clk, &max_clk, &var_clk,
			0, pmu_counter_ns, 0);
		//(We erase two first measurements because they are different from the
		// others. Reason:Branch prediction misses when entering for loop)
	}

	print("PMU Cycle Timer Statistics for %d consecutive reads\n", CLK_REP_TESTS);
	print("Average, Minimum, Maximum, Variance\n");
	print("%lld,%lld,%lld,%lld\n", clk_read_avrg = total_clk/CLK_REP_TESTS,
		min_clk, max_clk, variance (var_clk , total_clk, CLK_REP_TESTS));


	//---------READ AND WRITE ON-CHIP RAM-------//

	//----------Do the speed test (using memcpy)--------------//
	print("\nStart memcpy test\n");

	print("Data Size, Avg DMA_WR, Min DMA_WR,Max DMA_WR, Var DMA_WR, ");
	print("Avg DMA_RD, Min DMA_RD, Max DMA_RD, Var DMA_RD\n");

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
				printf("ERROR when calling malloc: Out of memory\n");
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
			memcpy((void*) on_chip_RAM_addr,
				(void *) (&(data[j*ON_CHIP_MEMORY_SPAN])), data_in_one_operation);

			overflow = pmu_counter_read_ns(&pmu_counter_ns);
			if (overflow == 1)
			{
				printf("Cycle counter overflow!! Program ended\n");
				return 1;
			}
			//printf("PMU counter (ns): %lld \n", pmu_counter_ns);

			if (l>=2) update_cumulative(&total_mcp_wr, &min_mcp_wr, &max_mcp_wr,
				&var_mcp_wr, 0, pmu_counter_ns, clk_read_avrg);
			//(We erase two first measurements because they are different from the
			// others. Reason:Branch prediction misses when entering for loop)

			//READ DATA FROM ON-CHIP RAM

			pmu_counter_reset();

			for (j=0; j<operation_loops; j++)
			memcpy((void *) (&(data[j*ON_CHIP_MEMORY_SPAN])),
				(void*) on_chip_RAM_addr, data_in_one_operation);

			overflow = pmu_counter_read_ns(&pmu_counter_ns);
			if (overflow == 1)
			{
				printf("Cycle counter overflow!! Program ended\n");
				return 1;
			}

			if (l>=2) update_cumulative(&total_mcp_rd, &min_mcp_rd, &max_mcp_rd,
				&var_mcp_rd, 0, pmu_counter_ns, clk_read_avrg);
			//(We erase two first measurements because they are different from the
			// others. Reason:Branch prediction misses when entering for loop)

			//check the content of the data just read
			error = 0;
			for (j=0; j<data_size[i]; j++)
			{
				if (data[j] != i) error = 1;
			}
			if (error != 0)
			{
				printf ("Error checking On-Chip RAM data size %dB\n", data_size[i]);
				return 0;
			}
//			else
//				printf("Check On-Chip RAM with data size %dB was OK\n", data_size[i]);

			//free dynamic memory
			free(data);

		}
		print("%d, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld\n", data_size[i],
		total_mcp_wr/REP_TESTS, min_mcp_wr, max_mcp_wr,
		variance(var_mcp_wr, total_mcp_wr, REP_TESTS),
		total_mcp_rd/REP_TESTS, min_mcp_rd, max_mcp_rd,
		variance(var_mcp_rd, total_mcp_rd, REP_TESTS) );
	}


	//----------Do the speed test (using for loop)--------------//
	print("\nStart for loop test\n");
	print("Data Size, Avg DMA_WR, Min DMA_WR,Max DMA_WR, Var DMA_WR, ");
	print("Avg DMA_RD, Min DMA_RD, Max DMA_RD, Var DMA_RD\n");

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
				printf("ERROR when calling malloc: Out of memory\n");
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
					on_chip_RAM_addr[k] = datai[offset+k];
			}

			overflow = pmu_counter_read_ns(&pmu_counter_ns);
			if (overflow == 1)
			{
				printf("Cycle counter overflow!! Program ended\n");
				return 1;
			}

			update_cumulative(&total_dir_wr, &min_dir_wr, &max_dir_wr, &var_dir_wr,
				0, pmu_counter_ns, clk_read_avrg);
				//(We erase two first measurements because they are different from the
				// others. Reason:Branch prediction misses when entering for loop)

			//READ DATA FROM ON-CHIP RAM

			pmu_counter_reset();

			for (j=0; j<operation_w_loops; j++)
			{
				offset = j*MEMORY_SIZE_IN_WORDS;
				for(k=0; k<data_in_one_operation_w; k++)
					datai[offset+k] = on_chip_RAM_addr[k];
			}

			overflow = pmu_counter_read_ns(&pmu_counter_ns);
			if (overflow == 1)
			{
				printf("Cycle counter overflow!! Program ended\n");
				return 1;
			}

			update_cumulative(&total_dir_rd, &min_dir_rd, &max_dir_rd, &var_dir_rd,
				0, pmu_counter_ns, clk_read_avrg);
			//(We erase two first measurements because they are different from the
			// others. Reason:Branch prediction misses when entering for loop)

			//check the content of the data just read
			error = 0;
			for (j=0; j<data_size[i]/BYTES_DATA_BUS; j++)
			{
				if (datai[j] != i) error = 1;
			}
			if (error != 0)
			{
				printf ("Error checking On-Chip RAM with size %dB\n", data_size[i]);
				return 0;

			}
			//else printf("Check On-Chip RAM with size %dB was OK\n", data_size[i]);

			//free memory used
			free(datai);
		}

		print("%d, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld\n",
		data_size[i], total_dir_wr/REP_TESTS, min_dir_wr, max_dir_wr,
		variance(var_dir_wr, total_dir_wr, REP_TESTS),
		total_dir_rd/REP_TESTS, min_dir_rd, max_dir_rd,
		variance(var_dir_rd, total_dir_rd, REP_TESTS) );
	}
	//--------------------------END OF TESTS USING PMU--------------------------//


	// ------clean up our memory mapping and exit -------//
	if( munmap( virtual_base, MMAP_SPAN ) != 0 ) {
		printf( "ERROR: munmap() failed...\n" );
		close( fd );
		return( 1 );
	}

	close( fd );

	if (print_screen == 0) fclose (f_print);

  printf("All tests were successful!! End...\n");

	return( 0 );
}
