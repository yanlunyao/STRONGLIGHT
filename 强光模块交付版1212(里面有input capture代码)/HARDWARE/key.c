#include "key.h"
#include "light.h"

//按键初始化函数，PB4
void KEY_Init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;

   RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
 
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
} 

//对按键操作的轮询动作
uint8_t key_up=1;					//KEY_UP=1代表着已经松开了
uint8_t Key_value = 1;

void KEY_Rock(void)
{
	Key_value = GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_4);
	
	if((key_up == 1)&&(Key_value == 0)&&(_key_count - TimingDelay > 500))
	{
		_key_count = TimingDelay;
		key_up=0;
		stand_count = TimingDelay;
		if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_6) == 0)
		{
			GPIO_SetBits(GPIOA, GPIO_Pin_6); 
		}
		else
		{
			GPIO_ResetBits(GPIOA, GPIO_Pin_6);
		}
	}
	else															
	{
		key_up = 1;
	}
}
 

















