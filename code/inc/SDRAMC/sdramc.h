#ifndef _SDRAMC_H_
#define _ACP_HIGH_LEVEL_API_H_

void set_sdramc_priorities(unsigned int  mppriority);
void set_sdramc_weights(unsigned int  mpweight_0_4,
  unsigned int mpweight_1_4);
void print_sdramc_priorities_weights();

#endif //_SDRAMC_H_
