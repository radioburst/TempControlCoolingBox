#ifndef _ds18b20_h_
#define _ds18b20_h_

#include <stdio.h>

void start_meas(void);
uint8_t read_meas(float *temp);

#endif
