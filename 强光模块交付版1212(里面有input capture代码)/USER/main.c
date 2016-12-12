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
ͷ�ļ���
can.h������zkrt.h
light.h������can.h��zkrt.h
key.h������light.h
*/


extern uint8_t  TIM5CH1_CAPTURE_STA;		//���벶��״̬		    				
extern uint32_t	TIM5CH1_CAPTURE_VAL;	//���벶��ֵ  
  

void bsp_init(void)
{
	SystemInit ();		/*ϵͳ��ʼ��*/
	RCC_Configuration();
	SysTick_Init();
	LED_Init();
	ADC1_Init();
//	USART1_Config();
	TIM1_CH3_Cap_Init(0xFFFF-1,48-1); //��1Mhz��Ƶ�ʼ�����1us����һ�Ρ�����ֵΪffff����ʱ�����ʱ��Ϊ655.36ms
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
		if(TIM5CH1_CAPTURE_STA&0X80)        //�ɹ�������һ�θߵ�ƽ
		{
			temp=TIM5CH1_CAPTURE_STA&0X3F; 
			temp*=0XFFFFFFFF;		 		         //���ʱ���ܺ�
			temp+=TIM5CH1_CAPTURE_VAL;		   //�õ��ܵĸߵ�ƽʱ��
			if((temp <1500)) //С��1.5ms
			{
				GPIO_ResetBits(GPIOA, GPIO_Pin_6); //light off
			}	
			else if(temp >=1500)//&&(temp <2000)) //����1.5ms
			{
				GPIO_SetBits(GPIOA, GPIO_Pin_6); //light on
			}
			else
			{
				//invalid
			}	
			TIM5CH1_CAPTURE_STA=0;			     //������һ�β���
		}
////		
		
		zkrt_decode();
		KEY_Rock();
		
		if (_10ms_count - TimingDelay >= 10)								//10msһ��ʱ��Ƭ
		{
			_10ms_count = TimingDelay;
			ADC_StartOfConversion(ADC1);											//ÿ10msһ�Σ���ȡ���ص�ѹ
			
			if ((_10ms_flag%10) == 0)													//ÿ100msһ�Σ����ϵ�ѹ������ѹ����������
			{				
//				if (MAVLINK_TX_INIT_VAL - TimingDelay > 2000)		//��ʼ����2S�ڲ�ִ�м�飬�Ժ�ÿ�ζ�ȡ���󶼼��
//				{
//					bat_read();
//					if (stand_count - TimingDelay > 500)
//					{
//						bat_check();
//					}
//				}
			}
			
			if ((_10ms_flag%100) == 0)												//ÿ1000msһ�Σ�����һ������
			{
				if (MAVLINK_TX_INIT_VAL - TimingDelay > 3000)		//��ʼ����3S�ڲ�ִ�з����������Ժ�ÿ�ζ���������
				{
					status_light[6] = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_6);
					status_light[7]++;
					if (status_light[7] == 0XFF)
					{
						status_light[7] = 0;
					}
					Can_Send_Msg(status_light, 8);								//����򵥵���䣬���ڵ�����Ӧ
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



