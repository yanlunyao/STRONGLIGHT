#ifndef __CAN_H
#define __CAN_H	 
#include "sys.h"

#define CAN_BUFFER_SIZE 254 

uint8_t CAN_Mode_Init(uint8_t mode);//CAN初始化
uint8_t CAN1_rx_check(void);
uint8_t CAN1_rx_byte(void);
uint8_t Can_Send_Msg(uint8_t* msg,uint8_t len);						//发送数据
uint8_t CAN1_send_message_fun(uint8_t *message, uint8_t len);//发送指令
#endif

















