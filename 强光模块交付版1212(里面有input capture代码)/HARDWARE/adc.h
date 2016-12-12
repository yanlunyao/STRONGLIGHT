#ifndef __ADC_H
#define __ADC_H
#include "sys.h" 

#define _25V_VOL 0
#define _12V_VOL 1
#define _5V_VOL  2
#define _12V_IS  3
#define _5V_IS   4

void ADC1_Init(void); 				//ADC通道初始化
uint16_t get_adc_val(uint8_t read_type);
void bat_read(void);
void bat_check(void);

#endif 

