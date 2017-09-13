#include <stdio.h>

//SDRAM controller address
#define SDRREGS 0xFFC20000
#define SDRREGS_SPAN 0x20000 //128kB
//Offset of the internal registers in SDRAM controller
#define MMPRIORITY 0x50AC
#define MPWEIGHT_0_4 0x50B0
#define MPWEIGHT_1_4 0x50B4

void set_sdramc_priorities(unsigned int  mppriority)
{
  *((unsigned int *)(SDRREGS + MMPRIORITY)) = mppriority;
  return;
}
void set_sdramc_weights(unsigned int  mpweight_0_4,
  unsigned int mpweight_1_4)
{
  *((unsigned int *)(SDRREGS + MPWEIGHT_0_4)) = mpweight_0_4;
  *((unsigned int *)(SDRREGS + MPWEIGHT_1_4)) = mpweight_1_4;
  return;
}
void print_sdramc_priorities_weights()
{
  printf("priorities in reg 0x%x: 0x%x\n\r",
    (unsigned int)(SDRREGS + MMPRIORITY),
    (unsigned int)(*((unsigned int*)(SDRREGS + MMPRIORITY))));
  printf("weights 0 in reg 0x%x: 0x%x\n\r",
    (unsigned int)(SDRREGS + MPWEIGHT_0_4),
    (unsigned int)(*((unsigned int*)(SDRREGS + MPWEIGHT_0_4))));
  printf("weights 1 in reg 0x%x: 0x%x\n\r",
    (unsigned int)(SDRREGS + MPWEIGHT_1_4),
    (unsigned int)(*((unsigned int*)(SDRREGS + MPWEIGHT_1_4))));
    return;
}
