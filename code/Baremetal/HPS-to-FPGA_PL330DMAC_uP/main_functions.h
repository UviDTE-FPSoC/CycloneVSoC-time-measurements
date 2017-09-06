/*
 * main_functions.h
 *
 *  Created on: 29/10/2015
 *  Author: Roberto Fernandez Molanes
 */

#ifndef MAIN_FUNCTIONS_H_
#define MAIN_FUNCTIONS_H_

//--------------------------DMA FUNCTIONS--------------------------//
ALT_STATUS_CODE system_init(void);
ALT_STATUS_CODE system_uninit(void);

//----------------------CACHE CONFIGURATION-----------------------//
void cache_configuration(int cache_config);

//---------------------------ACP CONFIGURATION---------------------------//
ALT_STATUS_CODE acp_configuration(void);

//---------------EXTRA FUNCTIONS TO CALCULATE SOME STATISTICS IN EXPERIMENTS-------//
void reset_cumulative(unsigned long long int * total, unsigned long long int* min, unsigned long long int * max, unsigned long long int * variance);
void update_cumulative(unsigned long long int * total,  unsigned long long int* min, unsigned long long int * max, unsigned long long int * variance, unsigned long long int ns_begin, unsigned long long int ns_end, unsigned long long int clk_read_delay);
unsigned long long variance (unsigned long long variance , unsigned long long total, unsigned long long rep_tests);


#endif /* MAIN_FUNCTIONS_H_ */
