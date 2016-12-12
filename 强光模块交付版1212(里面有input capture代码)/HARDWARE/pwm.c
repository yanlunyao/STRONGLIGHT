#include "pwm.h"

TIM_ICInitTypeDef  TIM1_ICInitStructure;

//��ʱ��1ͨ��3���벶������
//arr���Զ���װֵ(TIM1��16λ��!!)
//psc��ʱ��Ԥ��Ƶ��
void TIM1_CH3_Cap_Init(uint32_t arr,uint16_t psc)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1,ENABLE);  	//TIM1ʱ��ʹ��    
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE); 	//ʹ��PORTAʱ��	
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10; //GPIOA10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//���ù���
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//�ٶ�50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //���츴�����
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL; //���� //modify by yanly
	GPIO_Init(GPIOA,&GPIO_InitStructure); //��ʼ��PA0

	GPIO_PinAFConfig(GPIOA,GPIO_PinSource10,GPIO_AF_2); //PA0����λ��ʱ��1
  
	  
	TIM_TimeBaseStructure.TIM_Prescaler=psc;  //��ʱ����Ƶ
	TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up; //���ϼ���ģʽ
	TIM_TimeBaseStructure.TIM_Period=arr;   //�Զ���װ��ֵ
	TIM_TimeBaseStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	
	TIM_TimeBaseInit(TIM1,&TIM_TimeBaseStructure);
	

	//��ʼ��TIM5���벶�����
	TIM1_ICInitStructure.TIM_Channel = TIM_Channel_3; //CC1S=01 	ѡ������� IC1ӳ�䵽TI1��
  TIM1_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;//TIM_ICPolarity_BothEdge;	//�����ز���
  TIM1_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI; //ӳ�䵽TI1��
  TIM1_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;	 //���������Ƶ,����Ƶ 
  TIM1_ICInitStructure.TIM_ICFilter = 0x00;//IC1F=0000 ���������˲��� ���˲�
  TIM_ICInit(TIM1, &TIM1_ICInitStructure);
		
	TIM_ITConfig(TIM1, TIM_IT_CC3,ENABLE);//��������ж� ,����CC1IE�����ж�	
	
  TIM_Cmd(TIM1,ENABLE ); 	//ʹ�ܶ�ʱ��1

 
  NVIC_InitStructure.NVIC_IRQChannel = TIM1_CC_IRQn;//TIM1_BRK_UP_TRG_COM_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPriority=1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ�����
	
}
//����״̬
//[7]:0,û�гɹ��Ĳ���;1,�ɹ�����һ��.
//[6]:0,��û���񵽵͵�ƽ;1,�Ѿ����񵽵͵�ƽ��.
//[5:0]:����͵�ƽ������Ĵ���(����32λ��ʱ����˵,1us��������1,���ʱ��:4294��)
volatile uint8_t  TIM5CH1_CAPTURE_STA=0;	//���벶��״̬		    				
volatile uint32_t	TIM5CH1_CAPTURE_VAL;	//���벶��ֵ(TIM2/TIM5��32λ)
//��ʱ��5�жϷ������	 
void TIM1_CC_IRQHandler(void)	
{ 		    
 	if((TIM5CH1_CAPTURE_STA&0X80)==0)//��δ�ɹ�����	
	{
//		if(TIM_GetITStatus(TIM1, TIM_IT_Update) != RESET)//���
//		{	     
//			if(TIM5CH1_CAPTURE_STA&0X40)//�Ѿ����񵽸ߵ�ƽ��
//			{
//				if((TIM5CH1_CAPTURE_STA&0X3F)==0X3F)//�ߵ�ƽ̫����
//				{
//					TIM5CH1_CAPTURE_STA|=0X80;		//��ǳɹ�������һ��
//					TIM5CH1_CAPTURE_VAL=0XFFFF; //MODIFY BY YANLY
//				}
//				else 
//					TIM5CH1_CAPTURE_STA++;
//			}	 
//		}
		if(TIM_GetITStatus(TIM1, TIM_IT_CC3) != RESET)//����1���������¼�
		{	
			if(TIM5CH1_CAPTURE_STA&0X40)		//����һ���½��� 		
			{	  			
				TIM5CH1_CAPTURE_STA|=0X80;		//��ǳɹ�����һ�θߵ�ƽ����
				TIM5CH1_CAPTURE_VAL=TIM_GetCapture3(TIM1);//��ȡ��ǰ�Ĳ���ֵ.
	 			TIM_OC3PolarityConfig(TIM1,TIM_ICPolarity_Rising); //CC1P=0 ����Ϊ�����ز���
			}
			else  								//��δ��ʼ,��һ�β���������
			{
				TIM5CH1_CAPTURE_STA=0;			//���
				TIM5CH1_CAPTURE_VAL=0;
				TIM5CH1_CAPTURE_STA|=0X40;		//��ǲ�����������
				TIM_Cmd(TIM1,DISABLE ); 	//�رն�ʱ��5
	 			TIM_SetCounter(TIM1,0);
	 			TIM_OC3PolarityConfig(TIM1,TIM_ICPolarity_Falling);		//CC1P=1 ����Ϊ�½��ز���
				TIM_Cmd(TIM1,ENABLE ); 	//ʹ�ܶ�ʱ��5
			}		    
		}			     	    					   
 	}
	TIM_ClearITPendingBit(TIM1, TIM_IT_CC3); //����жϱ�־λ
}

