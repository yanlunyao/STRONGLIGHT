#include "sys.h"
#include "led.h"
#include "zkrt.h"
#include "adc.h"
#include "uart.h"
#include "can.h"
#include "light.h"
#include "key.h"
#include "pwm.h"
/*
头文件中
can.h包含了zkrt.h
light.h包含了can.h、zkrt.h
key.h包含了light.h
*/


extern uint8_t  TIM5CH1_CAPTURE_STA;		//输入捕获状态		    				
extern uint32_t	TIM5CH1_CAPTURE_VAL;	//输入捕获值  
  

void bsp_init(void)
{
	SystemInit ();		/*系统初始化*/
	RCC_Configuration();
	SysTick_Init();
	LED_Init();
	ADC1_Init();
//	USART1_Config();
	TIM1_CH3_Cap_Init(0xFFFF-1,48-1); //以1Mhz的频率计数，1us计数一次。重载值为ffff，定时器溢出时间为655.36ms
	CAN_Mode_Init(CAN_Mode_Normal);
	KEY_Init();
}
long long temp=0;  
uint8_t status_light[8] = {0XAA, 0XBB, 0XCC, 0XDD, 0XEE, DEVICE_TYPE_IRRADIATE, 0X00, 0X00};

int main()
{
//	long long temp=0;  
  bsp_init();
	
	while (1)
	{		
////add by yanly for pwm input handle
		if(TIM5CH1_CAPTURE_STA&0X80)        //成功捕获到了一次高电平
		{
			temp=TIM5CH1_CAPTURE_STA&0X3F; 
			temp*=0XFFFFFFFF;		 		         //溢出时间总和
			temp+=TIM5CH1_CAPTURE_VAL;		   //得到总的高电平时间
			if((temp <1500)) //小于1.5ms
			{
				GPIO_ResetBits(GPIOA, GPIO_Pin_6); //light off
			}	
			else if(temp >=1500)//&&(temp <2000)) //大于1.5ms
			{
				GPIO_SetBits(GPIOA, GPIO_Pin_6); //light on
			}
			else
			{
				//invalid
			}	
			TIM5CH1_CAPTURE_STA=0;			     //开启下一次捕获
		}
////		
		
		zkrt_decode();
		KEY_Rock();
		
		if (_10ms_count - TimingDelay >= 10)								//10ms一个时间片
		{
			_10ms_count = TimingDelay;
			ADC_StartOfConversion(ADC1);											//每10ms一次，读取板载电压
			
			if ((_10ms_flag%10) == 0)													//每100ms一次，整合电压、检测电压、发送心跳
			{				
//				if (MAVLINK_TX_INIT_VAL - TimingDelay > 2000)		//初始化的2S内不执行检查，以后每次读取到后都检查
//				{
//					bat_read();
//					if (stand_count - TimingDelay > 500)
//					{
//						bat_check();
//					}
//				}
			}
			
			if ((_10ms_flag%100) == 0)												//每1000ms一次，发送一次心跳
			{
				if (MAVLINK_TX_INIT_VAL - TimingDelay > 3000)		//初始化的3S内不执行发送心跳，以后每次都发送心跳
				{
					status_light[6] = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_6);
					status_light[7]++;
					if (status_light[7] == 0XFF)
					{
						status_light[7] = 0;
					}
					Can_Send_Msg(status_light, 8);								//这个简单的语句，便于调试响应
				}
			}
			_10ms_flag++;
		}
		
		if (led_rx_count - TimingDelay > 50)
		{
			GPIO_SetBits(GPIOB, GPIO_Pin_7);
		}
		
		if (led_tx_count - TimingDelay > 50)
		{
			GPIO_SetBits(GPIOB, GPIO_Pin_6);
		}
	}
}



