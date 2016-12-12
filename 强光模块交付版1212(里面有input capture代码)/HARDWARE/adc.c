#include "adc.h"

volatile uint16_t adc1_rx_buffer[50];

//PA0——25V；PA1——12V；PA2——5V；PA3——12I；PA4——5I
void ADC1_Init(void)
{
  ADC_InitTypeDef     ADC_InitStructure;
  GPIO_InitTypeDef    GPIO_InitStructure;
	DMA_InitTypeDef   DMA_InitStructure;
  
	ADC_DeInit(ADC1);
	
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1 , ENABLE);
  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
	ADC_StructInit(&ADC_InitStructure);
  ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE; 
  ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitStructure.ADC_ScanDirection = ADC_ScanDirection_Upward;//从通道0到通道16的方向
  ADC_Init(ADC1, &ADC_InitStructure);
  
	ADC_ChannelConfig(ADC1, ADC_Channel_0 , ADC_SampleTime_239_5Cycles);
	ADC_ChannelConfig(ADC1, ADC_Channel_1 , ADC_SampleTime_239_5Cycles);
	ADC_ChannelConfig(ADC1, ADC_Channel_2 , ADC_SampleTime_239_5Cycles);
	ADC_ChannelConfig(ADC1, ADC_Channel_3 , ADC_SampleTime_239_5Cycles);
	ADC_ChannelConfig(ADC1, ADC_Channel_4 , ADC_SampleTime_239_5Cycles);
  
	/* ADC Calibration */
  ADC_GetCalibrationFactor(ADC1);
  
	/* ADC DMA request in circular mode */
  ADC_DMARequestModeConfig(ADC1, ADC_DMAMode_Circular);
  
  /* Enable ADC_DMA */
  ADC_DMACmd(ADC1, ENABLE);  
	
  /* Enable the ADC peripheral */
  ADC_Cmd(ADC1, ENABLE);
  
  /* Wait the ADRDY flag */
  while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_ADRDY));
  
  /* DMA1 Channel1 Config */
  DMA_DeInit(DMA1_Channel1);
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&(ADC1->DR));
  DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)adc1_rx_buffer;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
  DMA_InitStructure.DMA_BufferSize = 50;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
  DMA_Init(DMA1_Channel1, &DMA_InitStructure);
  /* DMA1 Channel1 enable */
  DMA_Cmd(DMA1_Channel1, ENABLE);
}

uint16_t get_adc_val(uint8_t read_type)
{
	uint8_t count = 0;
	uint32_t sum = 0;
	
	for(count=0;count<10;count++)								//取10次
	{
		sum+=adc1_rx_buffer[count*5+read_type];
	}
	
	sum /= 10;
	
	switch (read_type)
	{
		case _25V_VOL:
		sum = (sum*67925)/9216;
		break;
		case _12V_VOL:
		sum = (sum* 3575)/1024;	
		break;
		case _5V_VOL:
		sum = (sum * 715)/512; 	
		break;
		case _12V_IS:
		case _5V_IS:
		sum = (sum * 165)/128;	
		break;
	}
	
	return sum;
}

uint16_t adc_25vol;			//0
uint16_t adc_12vol;			//1
uint16_t adc_5vol;			//2
uint16_t adc_12is;			//3
uint16_t adc_5is;				//4


//电池读取
void bat_read(void)
{
	adc_25vol = get_adc_val(_25V_VOL);
	adc_12vol = get_adc_val(_12V_VOL);
	adc_5vol 	= get_adc_val(_5V_VOL);
	adc_12is  = get_adc_val(_12V_IS);
	adc_5is  	= get_adc_val(_5V_IS);
}

//电池校验
//PB3——5VEN；PA6——12VEN
uint8_t cur_5_extra_flag = 0;
void bat_check(void)
{
	if ((adc_12vol > 13530) || (adc_12vol < 11070) || (adc_12is > 7000))
	{
		GPIO_ResetBits(GPIOA,GPIO_Pin_6);
	}

	if ((adc_5vol > 6600) || (adc_5vol < 4800))		
	{
		GPIO_ResetBits(GPIOB, GPIO_Pin_3);
		cur_5_extra_flag = 0;
	}
	else if (adc_5is > 1000)											
	{
		cur_5_extra_flag++;
		if (cur_5_extra_flag >= 3)								
		{
			GPIO_ResetBits(GPIOB, GPIO_Pin_3);
			cur_5_extra_flag = 0;
		}
	}
	else
	{
		cur_5_extra_flag = 0;
	}
}



