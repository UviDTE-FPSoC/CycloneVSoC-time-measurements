#include "arm_cache_modified.h" //to use Legup cache config functions
#include <stdio.h> //printf

void cache_configuration(int cache_config)
{
	if (cache_config<=0){ //0 no cache
		printf("\n\rCACHE CONFIG:0 No cache\n\r");
	}else if (cache_config<=1){ //1 enable MMU
		printf("\n\rCACHE CONFIG:1 Enable MMU\n\r");
		enable_MMU();
	}else if (cache_config<=2){//2 do 1 and initialize L2C
		printf("\n\rCACHE CONFIG:2 Enable MMU and init L2C\n\r");
		enable_MMU();
		initialize_L2C();
	}else if (cache_config<=3){ //3 do 2 and enable SCU
		printf("\n\rCACHE CONFIG:3 Enable MMU and SCU and init L2C\n\r");
		enable_MMU();
		initialize_L2C();
		enable_SCU();
	}else if (cache_config<=4){ //4 do 3 and enable L1_I
		printf("\n\rCACHE CONFIG:4 L1_I\n\r");
		enable_MMU();
		initialize_L2C();
		enable_SCU();
		enable_L1_I();
	}else if (cache_config<=5){ //5 do 4 and enable branch prediction
		printf("\n\rCACHE CONFIG:5 L1_I and branch prediction\n\r");
		enable_MMU();
		initialize_L2C();
		enable_SCU();
		enable_L1_I();
		enable_branch_prediction();
	}else if (cache_config<=6){ // 6 do 5 and enable L1_D
		printf("\n\rCACHE CONFIG:6 L1_D, L1_I and branch prediction\n\r");
		enable_MMU();
		initialize_L2C();
		enable_SCU();
		enable_L1_D();
		enable_L1_I();
		enable_branch_prediction();
	}else if (cache_config<=7){ // 7 do 6 and enable L1 D side prefetch
		printf("\n\rCACHE CONFIG:7 L1_D with side prefetch), L1_I with branch prediction\n\r");
		enable_MMU();
		initialize_L2C();
		enable_L1_D_side_prefetch();
		enable_SCU();
		enable_L1_D();
		enable_L1_I();
		enable_branch_prediction();
	}else if (cache_config<=8){ // 8 do 7 and enable L2C
		printf("\n\rCACHE CONFIG:8 L1_D side prefetch, L1_I with branch pre. and enable L2\n\r");
		enable_MMU();
		initialize_L2C();
		enable_L1_D_side_prefetch();
		enable_SCU();
		enable_caches(); //equivalent to enable L1_D,L1_I,branch prediction and L2
	}else if (cache_config<=9){ // 9 do 8 and enable L2 prefetch hint
		printf("\n\rCACHE CONFIG:9 basic config. + L2 prefetch hint\n\r");
		enable_MMU();
		initialize_L2C();
		enable_L1_D_side_prefetch();
		enable_L2_prefetch_hint();
		enable_SCU();
		enable_caches(); //equivalent to enable L1_D,L1_I,branch prediction and L2
	}else if (cache_config<=10){ // 10 do 9 and enable write full line zeros
		printf("\n\rCACHE CONFIG:10 basic config. + L2ph + wr full line 0s\n\r");
		enable_MMU();
		initialize_L2C();
		enable_L1_D_side_prefetch();
		enable_L2_prefetch_hint();
		enable_SCU();
		enable_write_full_line_zeros();
		enable_caches(); //equivalent to enable L1_D,L1_I,branch prediction and L2
	}else if (cache_config<=11){ // 11 do 10 and enable speculative linefills of L2 cache
		printf("\n\rCACHE CONFIG:11 basic config. + L2ph + wrfl0s + speculative linefills\n\r");
		enable_MMU();
		initialize_L2C();
		enable_L1_D_side_prefetch();
		enable_L2_prefetch_hint();
		enable_SCU();
		enable_L2_speculative_linefill(); //call SCU first
		enable_write_full_line_zeros();
		enable_caches(); //equivalent to enable L1_D,L1_I,branch prediction and L2
	}else if (cache_config<=12){ // 12 do 11 and enable early BRESP
		printf("\n\rCACHE CONFIG:12 basic config. + L2ph + wrfl0s + sl + eBRESP\n\r");
		enable_MMU();
		initialize_L2C();
		enable_L1_D_side_prefetch();
		enable_L2_prefetch_hint();
		enable_SCU();
		enable_L2_speculative_linefill(); //call SCU first
		enable_write_full_line_zeros();
		enable_early_BRESP();
		enable_caches(); //equivalent to enable L1_D,L1_I,branch prediction and L2
	}else if (cache_config<=13){ // 13 do 12 and store_buffer_limitation
		printf("\n\rCACHE CONFIG:13 basic config. + L2ph + wrfl0s + sl + eBRESP + buffer store limitation\n\r");
		enable_MMU();
		initialize_L2C();
		enable_L1_D_side_prefetch();
		enable_L2_prefetch_hint();
		enable_SCU();
		enable_L2_speculative_linefill(); //call SCU first
		enable_write_full_line_zeros();
		enable_early_BRESP();
		enable_store_buffer_device_limitation();
		enable_caches(); //equivalent to enable L1_D,L1_I,branch prediction and L2
	}
}
