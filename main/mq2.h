#ifndef _MQ2_H_
#define _MQ2_H_

#define RL 5.0
#define CAL_PPM 20
#define max_ppm 17.0
#define MQ2_CHANNEL ADC_CHANNEL_6

void init_MQ2();
void get_MQ2_data(int *data);
float calculatePPM(float rs);

#endif
