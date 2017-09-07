#ifndef _CACHE_HIGH_LEVEL_API_H_
#define _CACHE_HIGH_LEVEL_API_H_

//--macros that can be sent to configure cache--//
//Each level goes adding a new feature so the effect of adding one feature can
//be easily tested.
//0: no cache
#define NO_CACHE                                        0
//1: enable MMU
#define EN_MMU                                          1
//2: do 1 and initialize L2C
#define EN_MMU_INIT_L2C                                 2
//3: do 2 and enable SCU
#define EN_MMU_SCU_INIT_L2C                             3
//4: do 3 and enable L1_I
#define EN_MMU_SCU_L1I_INIT_L2C                         4
//5: do 4 and enable branch prediction
#define EN_MMU_SCU_L1IBPREDICT_INIT_L2C                 5
//6: do 5 and enable L1_D
#define EN_MMU_SCU_L1IBPREDICT_L1D_INIT_L2C             6
//7: do 6 and enable L1 D side prefetch
#define EN_MMU_SCU_L1IBPREDICT_L1DSIDEPREFETCH_INIT_L2C 7
//8: do 7 and enable L2C (All caches enabled with basic config.)
#define EN_CACHES                                       8
//9: do 8 and enable L2 prefetch hint
#define EN_CACHES_L2PREFETCH                            9
//10: do 9 and enable write full line zeros
#define EN_CACHES_L2PREF_FULL0                          10
//11: do 10 and enable speculative linefills of L2 cache
#define EN_CACHES_L2PREF_FULL0_SPECFILLS                11
//12: do 11 and enable early BRESP
#define EN_CACHES_L2PREF_FULL0_SPECFILLS_EBRESP         12
//13: do 12 and store_buffer_limitation
#define EN_CACHES_L2PREF_FULL0_SPECFILLS_EBRESP_STORLIM 13

//--High level function to configure cache with one single call--//
void cache_configuration(int cache_config);

#endif
