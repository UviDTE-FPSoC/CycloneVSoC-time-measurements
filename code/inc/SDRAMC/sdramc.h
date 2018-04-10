#ifndef _SDRAMC_H_
#define _ACP_HIGH_LEVEL_API_H_

//SDRAM Addresses
//SDRAMC beginning address
#define SDRAMC_REGS 0xFFC20000
//SDRAMC Span 128kB
#define SDRAMC_REGS_SPAN 0x20000 //128kB
//Offset of FPGA-to-SDRAMC ports reset register from the beginning of SDRAMC
#define FPGAPORTRST 0x5080

void set_sdramc_priorities(unsigned int  mppriority);
void set_sdramc_weights(unsigned int  mpweight_0_4,
  unsigned int mpweight_1_4);
void print_sdramc_priorities_weights();

#endif //_SDRAMC_H_
