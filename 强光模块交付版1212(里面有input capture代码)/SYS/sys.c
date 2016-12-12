#include "sys.h"
#include "core_cm0.h"
/*********************************************系统时钟**********************/

void RCC_Configuration(void)
{
	uint32_t StartUpCounter = 0, HSIStatus = 0;
	
	RCC_DeInit();//RCC复位
	RCC_HSICmd(ENABLE);//HSI使能
	
	while((RCC_GetFlagStatus(RCC_FLAG_HSIRDY) == RESET) && (StartUpCounter != HSI_STARTUP_TIMEOUT))//等待HSI使能结束
	{
		StartUpCounter++;
	}
	
	if (RCC_GetFlagStatus(RCC_FLAG_HSIRDY) != RESET)//如果使能成功了
	{
		HSIStatus = (uint32_t)0x01;
	}
	else
	{
		HSIStatus = (uint32_t)0x00;
	}	
	
	if (HSIStatus == (uint32_t)0x01)//如果HSI使能成功
  {
    /* Enable Prefetch Buffer */
		FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY;
		
		RCC_HCLKConfig(RCC_SYSCLK_Div1);//HCLK=SYSCLK
		RCC_PCLKConfig(RCC_HCLK_Div1);  //PCLK=HCLK
		
		RCC_PLLConfig(RCC_PLLSource_HSI_Div2, RCC_PLLMul_12);//HSI二分频，再12倍频，得到48M的PLL
		
		RCC_PLLCmd(ENABLE);//使能PLL时钟

		while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET)//等待PLL使能结束
		{
		}
		
		RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);//配置PLL作为SYS的时钟源

		while (RCC_GetSYSCLKSource() != (uint8_t)RCC_CFGR_SWS_PLL)//等待配置完成
		{
		}
  }
  else
  {
		//say something else!
  }  
}

/************************************delay*********************************/

void SysTick_Init(void)
{ 
  while (SysTick_Config(SystemCoreClock / 1000) !=  0);	//配置systick，中断时间1ms
}

//24位计数器，最大可以填充16777216个拍，1us等于48个拍，也就是最多可以传入349525us，也就是约等于349ms
void delay_us(uint16_t nus)						//传入最多65535us
{
	uint32_t start_num;									//用于记录val的初始值
	uint32_t temp;											//用于记录val的当前值，然后用于计算差额
	uint32_t nus_pai;
	
	nus_pai = 48 * nus;
	start_num = SysTick->VAL;						//记录下来初始的val值
	do
	{
		temp=SysTick->VAL;								//记录下来当前的val值
		if (temp < start_num)							//由于向下计数，如果当前值小于初始值，说明没出一个周期
		{
			temp = start_num - temp;				//没出一个周期，那么计算一下差额即可
		}
		else															//如果跨越一个周期了
		{
			temp = start_num+48000-temp;		//这是计算剩余的数，以及总长和当前值的差距，再求和
		}
	}while (temp < nus_pai);      					//当这个差距小于nus_pai的时候，会持续动作
}

volatile uint32_t TimingDelay = 0XFFFFFFFF;				//设置为全局变量，不断的做减法
volatile uint32_t led_rx_count = 0XFFFFFFFF;
volatile uint32_t led_tx_count = 0XFFFFFFFF;
volatile uint32_t _10ms_count = 0XFFFFFFFF;
volatile uint32_t _10ms_flag = 0;
volatile uint32_t _key_count = 0XFFFFFFFF;

void delay_ms(uint16_t nms)								//最多传入65535ms
{
	uint32_t start_num;
	
	start_num = TimingDelay;								//记录下当前的TimingDelay值
	while ((start_num - TimingDelay) < nms);//用起始值减去当前的TimingDelay，如果这个间隔小于nms则延时
}

void SysTick_Handler(void)
{
	TimingDelay--;
}



