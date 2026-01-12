#include "config.h"

#ifdef USE_DIP_SHIFT
  #include "dip_shift.c"
#else
  #include "dip_gpio.c"
#endif
