//file: statistics.h
//functions to help calculate some statistics for the measurements like
//mean, min, max and variance

void reset_cumulative(unsigned long long int * total,
  unsigned long long int* min, unsigned long long int * max,
  unsigned long long int * variance);
void update_cumulative(unsigned long long int * total,
  unsigned long long int* min, unsigned long long int * max,
  unsigned long long int * variance, unsigned long long int ns_begin,
  unsigned long long int ns_end, unsigned long long int clk_read_delay);
unsigned long long variance (unsigned long long variance ,
  unsigned long long total, unsigned long long rep_tests);
