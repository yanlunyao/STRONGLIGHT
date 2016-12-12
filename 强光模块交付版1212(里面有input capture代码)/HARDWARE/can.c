#include "can.h"
#include "zkrt.h"	//��ҪZKRT���THROW

volatile uint8_t can1_rx_buff[CAN_BUFFER_SIZE];
volatile uint16_t can1_rx_buff_store = 0;
uint16_t can1_rx_buff_get = 0;

uint8_t CAN_Mode_Init(uint8_t mode)
{
	GPIO_InitTypeDef       GPIO_InitStructure; 
	CAN_InitTypeDef        CAN_InitStructure;
	CAN_FilterInitTypeDef  CAN_FilterInitStructure;
	NVIC_InitTypeDef  NVIC_InitStructure;

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN, ENABLE);


	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_PinAFConfig(GPIOA, GPIO_PinSource11, GPIO_AF_4);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource12, GPIO_AF_4);

	CAN_DeInit(CAN);

	CAN_InitStructure.CAN_TTCM=DISABLE;
	CAN_InitStructure.CAN_ABOM=DISABLE;
	CAN_InitStructure.CAN_AWUM=DISABLE;
	CAN_InitStructure.CAN_NART=DISABLE;
	CAN_InitStructure.CAN_RFLM=DISABLE;
	CAN_InitStructure.CAN_TXFP=DISABLE;

	CAN_InitStructure.CAN_Mode= mode;
	CAN_InitStructure.CAN_SJW=CAN_SJW_1tq;
	CAN_InitStructure.CAN_BS1=CAN_BS1_8tq;
	CAN_InitStructure.CAN_BS2=CAN_BS2_3tq;
	CAN_InitStructure.CAN_Prescaler=8;

	CAN_Init(CAN, &CAN_InitStructure);// ��ʼ��CAN1

	CAN_FilterInitStructure.CAN_FilterNumber=0;	 
	CAN_FilterInitStructure.CAN_FilterMode=CAN_FilterMode_IdList; 
	CAN_FilterInitStructure.CAN_FilterScale=CAN_FilterScale_32bit; 
	CAN_FilterInitStructure.CAN_FilterIdHigh=(DEVICE_TYPE_IRRADIATE<<5);
	CAN_FilterInitStructure.CAN_FilterIdLow=0x0000;
	CAN_FilterInitStructure.CAN_FilterMaskIdHigh=0x0000;
	CAN_FilterInitStructure.CAN_FilterMaskIdLow=0x0000;
	CAN_FilterInitStructure.CAN_FilterFIFOAssignment=CAN_Filter_FIFO0;
	CAN_FilterInitStructure.CAN_FilterActivation=ENABLE; 
	CAN_FilterInit(&CAN_FilterInitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = CEC_CAN_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	CAN_ITConfig(CAN,CAN_IT_FMP0,ENABLE);

	return 0;
}   
		    
void CEC_CAN_IRQHandler(void)
{
	uint8_t i;
	CanRxMsg RxMessage;
	
  CAN_Receive(CAN, CAN_FIFO0, &RxMessage);//CAN�豸���FIFO0�����ݣ���ȡ�����浽RxMessage�ṹ����
	
	for (i = 0; i < RxMessage.DLC; i++)
	{
		can1_rx_buff[can1_rx_buff_store] = RxMessage.Data[i];
		
		can1_rx_buff_store++;
		if (can1_rx_buff_store == CAN_BUFFER_SIZE)
		{
			can1_rx_buff_store = 0;
		}
	}
	
	GPIO_ResetBits(GPIOB, GPIO_Pin_7);
	led_rx_count = TimingDelay;
}

//ͨ���ж�get��store��λ�ù�ϵ�������Ƿ�����ֵ
uint8_t CAN1_rx_check(void)
{
	if (can1_rx_buff_store == can1_rx_buff_get)//��û���µ�ֵ����ʱ���洢ֵλ�õ���ȡ��ֵλ��
		return 0;
	else 
		return 1;//����ֵ����ʱ������1
}

//������ֵ��ʱ��ȡ��һ��ֵ
uint8_t CAN1_rx_byte(void)
{
	uint8_t ch;	
	
	ch = can1_rx_buff[can1_rx_buff_get];//��ch��¼�����յ���һ������
	
	can1_rx_buff_get++;
	if (can1_rx_buff_get == CAN_BUFFER_SIZE)
	{
		can1_rx_buff_get = 0;
	}

	return ch;
}

//can����һ������(�̶���ʽ:IDΪ0X01,��׼֡,����֡)	
//len:���ݳ���(���Ϊ8)				     
//msg:����ָ��,���Ϊ8���ֽ�.
//����ֵ:0,�ɹ�;����,ʧ��;
uint8_t Can_Send_Msg(uint8_t* msg,uint8_t len)
{	
  uint8_t mbox;
  uint16_t i=0;
  CanTxMsg TxMessage;						    
	
	TxMessage.StdId=(DEVICE_TYPE_IRRADIATE<<4); 
  TxMessage.ExtId=0x00;				         
  TxMessage.IDE=CAN_Id_Standard;         
  TxMessage.RTR=CAN_RTR_Data;		          
  TxMessage.DLC=len;						           
  for(i=0;i<len;i++)
	TxMessage.Data[i]=msg[i];              
  
	mbox= CAN_Transmit(CAN, &TxMessage);     
	
  i=0;
  while((CAN_TransmitStatus(CAN, mbox)==CAN_TxStatus_Failed)&&(i<0XFFF))i++;	
	
  if(i>=0XFFF)
	{
		CAN_Mode_Init(CAN_Mode_Normal);
		return 1;
  }
	
	GPIO_ResetBits(GPIOB, GPIO_Pin_6);
	led_tx_count = TimingDelay;
	
	return 0;		//�ɹ�����

}

//����ģ������ݷ��ظ�����ģ��
uint8_t CAN1_send_message_fun(uint8_t *message, uint8_t len)
{
	uint8_t count;		             
	uint8_t time;
	
	time = len/8;            
	
	for (count = 0; count < time; count++)
	{
		Can_Send_Msg(message, 8);
		message += 8;
		delay_us(999);
	}
	if (len%8)                          
	{
		Can_Send_Msg(message, len%8);
		delay_us(999);
	}
	
	return 0;
}














