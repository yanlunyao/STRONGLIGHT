#include "light.h"
#include "zkrt.h"	//调用需要
#include "can.h"	//解析CAN的字节

uint32_t stand_count = 0XFFFFFFFF;
uint8_t can_value;
zkrt_packet_t zkrt_packet_buffer;

void zkrt_decode(void)
{
	while (CAN1_rx_check() == 1)
	{
		can_value = CAN1_rx_byte();
		if (zkrt_decode_char(&zkrt_packet_buffer,can_value)==1)
		{
			stand_count = TimingDelay;
			if (zkrt_packet_buffer.data[0] == 1)
			{
				GPIO_SetBits(GPIOA, GPIO_Pin_6);
			}
			else
			{
				GPIO_ResetBits(GPIOA, GPIO_Pin_6);
			}
		}
	}
}
