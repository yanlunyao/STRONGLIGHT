#include "sys.h"
#include "core_cm0.h"
/*********************************************ϵͳʱ��**********************/

void RCC_Configuration(void)
{
	uint32_t StartUpCounter = 0, HSIStatus = 0;
	
	RCC_DeInit();//RCC��λ
	RCC_HSICmd(ENABLE);//HSIʹ��
	
	while((RCC_GetFlagStatus(RCC_FLAG_HSIRDY) == RESET) && (StartUpCounter != HSI_STARTUP_TIMEOUT))//�ȴ�HSIʹ�ܽ���
	{
		StartUpCounter++;
	}
	
	if (RCC_GetFlagStatus(RCC_FLAG_HSIRDY) != RESET)//���ʹ�ܳɹ���
	{
		HSIStatus = (uint32_t)0x01;
	}
	else
	{
		HSIStatus = (uint32_t)0x00;
	}	
	
	if (HSIStatus == (uint32_t)0x01)//���HSIʹ�ܳɹ�
  {
    /* Enable Prefetch Buffer */
		FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY;
		
		RCC_HCLKConfig(RCC_SYSCLK_Div1);//HCLK=SYSCLK
		RCC_PCLKConfig(RCC_HCLK_Div1);  //PCLK=HCLK
		
		RCC_PLLConfig(RCC_PLLSource_HSI_Div2, RCC_PLLMul_12);//HSI����Ƶ����12��Ƶ���õ�48M��PLL
		
		RCC_PLLCmd(ENABLE);//ʹ��PLLʱ��

		while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET)//�ȴ�PLLʹ�ܽ���
		{
		}
		
		RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);//����PLL��ΪSYS��ʱ��Դ

		while (RCC_GetSYSCLKSource() != (uint8_t)RCC_CFGR_SWS_PLL)//�ȴ��������
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
  while (SysTick_Config(SystemCoreClock / 1000) !=  0);	//����systick���ж�ʱ��1ms
}

//24λ�����������������16777216���ģ�1us����48���ģ�Ҳ���������Դ���349525us��Ҳ����Լ����349ms
void delay_us(uint16_t nus)						//�������65535us
{
	uint32_t start_num;									//���ڼ�¼val�ĳ�ʼֵ
	uint32_t temp;											//���ڼ�¼val�ĵ�ǰֵ��Ȼ�����ڼ�����
	uint32_t nus_pai;
	
	nus_pai = 48 * nus;
	start_num = SysTick->VAL;						//��¼������ʼ��valֵ
	do
	{
		temp=SysTick->VAL;								//��¼������ǰ��valֵ
		if (temp < start_num)							//�������¼����������ǰֵС�ڳ�ʼֵ��˵��û��һ������
		{
			temp = start_num - temp;				//û��һ�����ڣ���ô����һ�²���
		}
		else															//�����Խһ��������
		{
			temp = start_num+48000-temp;		//���Ǽ���ʣ��������Լ��ܳ��͵�ǰֵ�Ĳ�࣬�����
		}
	}while (temp < nus_pai);      					//��������С��nus_pai��ʱ�򣬻��������
}

volatile uint32_t TimingDelay = 0XFFFFFFFF;				//����Ϊȫ�ֱ��������ϵ�������
volatile uint32_t led_rx_count = 0XFFFFFFFF;
volatile uint32_t led_tx_count = 0XFFFFFFFF;
volatile uint32_t _10ms_count = 0XFFFFFFFF;
volatile uint32_t _10ms_flag = 0;
volatile uint32_t _key_count = 0XFFFFFFFF;

void delay_ms(uint16_t nms)								//��ഫ��65535ms
{
	uint32_t start_num;
	
	start_num = TimingDelay;								//��¼�µ�ǰ��TimingDelayֵ
	while ((start_num - TimingDelay) < nms);//����ʼֵ��ȥ��ǰ��TimingDelay�����������С��nms����ʱ
}

void SysTick_Handler(void)
{
	TimingDelay--;
}



