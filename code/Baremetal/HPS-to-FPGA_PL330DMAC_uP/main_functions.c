/*
 * main_functions.h
 *
 *  Created on: 30/3/2016
 *      Author: Roberto Fernandez Molanes
 *      Explanation: Main functions used in the main() file. Putr here just to clean-up the other file.
 *
 */
//
#include <stdio.h>
#include <string.h> //to use memcpy
#include <inttypes.h>
#include "alt_address_space.h"
#include "alt_dma_modified.h"
#include "socal/alt_acpidmap.h"
#include "socal/socal.h"
#include "alt_address_space.h" //to use ACP ID mapper functions

#include "arm_cache_modified.h" //to use Legup cache config functions

//--------------------------DMA FUNCTIONS--------------------------//
ALT_STATUS_CODE system_init(void)
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

ALT_STATUS_CODE system_uninit(void)
{
    printf("INFO: System shutdown.\n\r");
    printf("\n\r");
    return ALT_E_SUCCESS;
}

//--------------------------CACHE CONFIGURATION--------------------------//
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

//---------------------------ACP CONFIGURATION---------------------------//
ALT_STATUS_CODE acp_configuration(void)
{
	printf("\n\rConfiguring ACP\n\r");
	//printf("Before acp config VID3RD_S:%#x, VID4WR_S:%#x, DYNRD:%#x, DYNWR:%#x\n\r", *VID3RD_S, *VID4WR_S, *DYNRD, *DYNWR);
	const uint32_t ARUSER = 0b11111; //coherent cacheable reads
	const uint32_t AWUSER = 0b11111; //coherent cacheable writes
	ALT_STATUS_CODE status = ALT_E_SUCCESS;
	
	//Set output ID3 for dynamic reads and ID4 for dynamic writes
	status = alt_acp_id_map_dynamic_read_set(ALT_ACP_ID_OUT_DYNAM_ID_3);
	if (status != ALT_E_SUCCESS)
	{
		printf ("Error when configuring ID3 as dynamic for reads\n\r"); return status;
	}		
	status = alt_acp_id_map_dynamic_write_set(ALT_ACP_ID_OUT_DYNAM_ID_4);
	if (status != ALT_E_SUCCESS)
	{
		printf ("Error when configuring ID4 as dynamic for writes\n\r"); return status;
	}
	//Configure the page and user write sideband signal options that are applied 
	//to all write transactions that have their input IDs dynamically mapped.
	status = alt_acp_id_map_dynamic_read_options_set(ALT_ACP_ID_MAP_PAGE_0, ARUSER);
	if (status != ALT_E_SUCCESS)
	{
		printf ("Error when setting options for dynamic reads\n\r"); return status;
	}
	status = alt_acp_id_map_dynamic_write_options_set(ALT_ACP_ID_MAP_PAGE_0, AWUSER);
	if (status != ALT_E_SUCCESS)
	{
		printf ("Error when setting options for dynamic writes\n\r"); return status;
	}
	//printf("After acp config VID3RD_S:%#x, VID4WR_S:%#x, DYNRD:%#x, DYNWR:%#x\n\r", *VID3RD_S, *VID4WR_S, *DYNRD, *DYNWR);
	return status;
}

//---------------EXTRA FUNCTIONS TO CALCULATE SOME STATISTICS IN EXPERIMENTS-------//
void reset_cumulative(unsigned long long int * total, unsigned long long int* min, unsigned long long int * max, unsigned long long int * variance)
{
	*total = 0;
	*min = ~0;
	*max = 0;
	*variance = 0;
}

void update_cumulative(unsigned long long int * total,  unsigned long long int* min, unsigned long long int * max, unsigned long long int * variance, unsigned long long int ns_begin, unsigned long long int ns_end, unsigned long long int clk_read_delay)
{
	unsigned long long int tmp = ( ns_end < ns_begin ) ? 1000000000 - (ns_begin - ns_end) - clk_read_delay :
														ns_end - ns_begin - clk_read_delay ;

	*total = *total + tmp;
	*variance = *variance + tmp*tmp;

	if (tmp < *min) *min = tmp;
	if (tmp > *max) *max = tmp;

//	printf("total %lld, begin %lld, end %lld\n", *total, ns_begin, ns_end);
}

unsigned long long variance (unsigned long long variance , unsigned long long total, unsigned long long rep_tests)
{

	float media_cuadrados, quef1, quef2, cuadrado_media, vari;

	media_cuadrados = (variance/(float)(rep_tests-1));
	quef1 = (total/(float)rep_tests);
	quef2=(total/(float)(rep_tests-1));
	cuadrado_media = quef1 * quef2;
	vari = media_cuadrados - cuadrado_media;
/*
	printf("media_cuadrados %f,",media_cuadrados );
	printf("quef1 %f,",quef1 );
	printf("quef2 %f,",quef2 );
	printf("cuadrado_media %f,",cuadrado_media );
	printf("variance %f\n",vari );
*/
	return (unsigned long long) vari;

	//return ((variance/(rep_tests-1))-(total/rep_tests)*(total/(rep_tests-1)));
}
