//file: statistics.c
//functions to help calculate some statistics for the measurements like
//mean, min, max and variance

//reset cumulative buffers for mean, max, min and variance
void reset_cumulative(unsigned long long int * total,
  unsigned long long int* min, unsigned long long int * max,
  unsigned long long int * variance)
{
  *total = 0;
  *min = ~0;
  *max = 0;
  *variance = 0;
}

//update cumulative buffers for mean, max, min and variance with new measurement
void update_cumulative(unsigned long long int * total,
  unsigned long long int* min, unsigned long long int * max,
  unsigned long long int * variance, unsigned long long int ns_begin,
  unsigned long long int ns_end, unsigned long long int clk_read_delay)
{
  unsigned long long int tmp = ( ns_end < ns_begin ) ?
    1000000000 - (ns_begin - ns_end) - clk_read_delay :
    ns_end - ns_begin - clk_read_delay ;

  *total = *total + tmp;
  *variance = *variance + tmp*tmp;

  if (tmp < *min) *min = tmp;
  if (tmp > *max) *max = tmp;

//  printf("total %lld, begin %lld, end %lld\n", *total, ns_begin, ns_end);
}

//calculate the variance from the cumulative
unsigned long long variance (unsigned long long variance ,
  unsigned long long total, unsigned long long rep_tests)
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
