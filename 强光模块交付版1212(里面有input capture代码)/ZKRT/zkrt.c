#include "zkrt.h"

//有两处需要保存起来：飞机类型、飞机编号、温度上限、温度下限，必须要由flash保存？？？？？？？？？？？？？？？
//序列号全局定义，然后递增
uint8_t zkrt_tx_seq[DEVICE_NUMBER];               		//字节6，帧序列号，为了区分，每个操作需要1个专门的序列号
uint8_t now_uav_type = 0;
uint16_t now_uav_num = 0;

/**
 * @brief Accumulate the X.25 CRC by adding one char at a time.
 *
 * The checksum function adds the hash of one char at a time to the
 * 16 bit checksum (uint16_t).
 *
 * @param data new char to hash
 * @param crcAccum the already accumulated checksum
调用的情况：正在crc校验码计算的某一时刻
函数的作用：对数组中某个字节校验
调用前：crc校验码是某个数值
调用后：crc校验码是一个新的数值
 **/
void crc_accumulate(uint8_t data, uint16_t *crcAccum)
{
	/*Accumulate one byte of data into the CRC*/
	uint8_t tmp;

	tmp = data ^ (uint8_t)(*crcAccum &0xff);
	tmp ^= (tmp<<4);
	*crcAccum = (*crcAccum>>8) ^ (tmp<<8) ^ (tmp <<3) ^ (tmp>>4);
}


/**
 * @brief Accumulate the X.25 CRC by adding an array of bytes
 *
 * The checksum function adds the hash of one char at a time to the
 * 16 bit checksum (uint16_t).
 *
 * @param data new bytes to hash
 * @param crcAccum the already accumulated checksum
调用的情况：对mavlink的5个参数校验完之后
函数的作用：对mavlink中的数据部分进行crc校验
调用前：crc是根据参数生成的校验码
调用后：得到数据部分之前的校验码
 **/
void crc_accumulate_buffer(uint16_t *crcAccum, const char *pBuffer, uint16_t length)
{
	const uint8_t *p = (const uint8_t *)pBuffer;
	
	while (length--)
	{
		crc_accumulate(*p++, crcAccum);
	}
}



/**
 * @brief Calculates the X.25 checksum on a byte buffer
 *
 * @param  pBuffer buffer containing the byte array to hash
 * @param  length  length of the byte array
 * @return the checksum over the buffer bytes
调用的情况：对mavlink指令进行crc校验（刚开始）
函数的作用：对mavlink指令的buf[5：1]共5个参数crc校验
调用前：crc校验码是任意数
调用后：根据参数生成的crc校验码
 **/
uint16_t crc_calculate(const uint8_t* pBuffer, uint16_t length)
{
	uint16_t crcTmp = 0XFFFF;//校验码设置为0xffff
        
	while (length--) 
	{
		crc_accumulate(*pBuffer++, &crcTmp);//逐个对数组中的每个字节进行校验
	}
	return crcTmp;//返回校验码
}




//该封包函数用于三处：地面站创建指令给管理单元、管理单元创建指令给地面站（飞控）、子模块创建指令给管理单元，管理单元创建指令给子模块的情形没有，所以不必考虑seq重复
//功能：在封包时，对所有字节取固定值的字段进行封包
//参数：封包的packet
//返回值：无
//第一部分：在这里完全定义好的：
//1、不会变化，可以宏定义的：start，ver，length，end
//2、随时变化、暂时不用、被定义为0的：session_ack，padding_enc
//3、不会变化，可以设置为0的：APPID
//4、随时变化、不用考虑的：seq，crc

//第二部分：在这里无法直接定义的
//随时变化、随时传送进去的：cmd选择0或1；uavid，command，data
void zkrt_final_encode(zkrt_packet_t *packet)
{
	uint8_t i;
	
	packet->start_code = _START_CODE;                             //字节0，帧起始码，0XEB
	packet->ver = _VERSION;                                       //字节1，协议版本
	packet->session_ack = 0X00;                                   //字节2，无需应答，数据帧
	packet->padding_enc = 0X00;                                   //字节3，不加密
	packet->length = _LENGTH;                                     //字节5，一共30位
	packet->seq = zkrt_tx_seq[packet->UAVID[3]];                  //字节6，帧序列号
	zkrt_tx_seq[packet->UAVID[3]]++;
	zkrt_tx_seq[packet->UAVID[3]]%=255;
	for(i=0;i<3;i++)
	packet->APPID[i]= 0x00;                                       //字节7-9，服务器ID，地面站
	packet->crc = crc_calculate(((const uint8_t*)(packet)),47); //字47-48，校验位
	packet->end_code = _END_CODE;                                 //字49，结束符
}




/*对单个字节的crc更新校验*/
void zkrt_update_checksum(zkrt_packet_t* packet, uint8_t ch)
{
	uint16_t crc = packet->crc;
	
	crc_accumulate(ch,&(crc));//如果直接传参crc_accumulate(ch,&(packet->crc));不可以，因为结构体成员的地址不能直接传参，这会导致hardfault错误
	
	packet->crc = crc;
}


uint8_t zkrt_curser_state = 0;
uint8_t zkrt_recv_success = 0;
uint8_t zkrt_app_index = 0;
uint8_t zkrt_uav_index = 0;
uint8_t zkrt_dat_index = 0;
uint8_t zkrt_decode_char(zkrt_packet_t *packet, uint8_t ch)
{
	zkrt_recv_success = 0;
	
	if ((zkrt_curser_state == 0)&&(ch == _START_CODE))		//字节0，得到起始码
	{
		packet->start_code = ch;
		packet->crc = 0XFFFF;
		zkrt_update_checksum(packet,ch);
		zkrt_curser_state = 1;
	}
	else if ((zkrt_curser_state == 1)&&(ch == _VERSION))	//字节1，得到版本号
	{
		packet->ver = ch;
		zkrt_update_checksum(packet,ch);
		zkrt_curser_state = 2;
	}
	else if (zkrt_curser_state == 2)											//字节2，得到session
	{
		packet->session_ack = ch;
		zkrt_update_checksum(packet,ch);
		zkrt_curser_state = 3;
	}
	else if (zkrt_curser_state == 3)											//字节3，得到padding
	{
		packet->padding_enc = ch;
		zkrt_update_checksum(packet,ch);
		zkrt_curser_state = 4;
	}
	else if (zkrt_curser_state == 4)											//字节4，得到cmd
	{
		packet->cmd = ch;
		zkrt_update_checksum(packet,ch);
		zkrt_curser_state = 5;
	}
	else if ((zkrt_curser_state == 5)&&(ch == _LENGTH))		//字节5，得到长度
	{
		packet->length = ch;
		zkrt_update_checksum(packet,ch);
		zkrt_curser_state = 6;
	}
	else if (zkrt_curser_state == 6)											//字节6，接收序列号
	{
		packet->seq = ch;
		zkrt_update_checksum(packet,ch);
		zkrt_curser_state = 7;
	}
	else if (zkrt_curser_state == 7)											//字节7-9，接收APPID
	{
		packet->APPID[zkrt_app_index] = ch;
		zkrt_update_checksum(packet,ch);
		zkrt_app_index++;
		if (zkrt_app_index == 3)
		{
			zkrt_app_index = 0;
			zkrt_curser_state = 8;
		}
	}
	else if (zkrt_curser_state == 8)											//字节10-15，接收UAVID
	{
		packet->UAVID[zkrt_uav_index] = ch;
		zkrt_update_checksum(packet,ch);
		zkrt_uav_index++;
		if (zkrt_uav_index == 6)
		{
			zkrt_uav_index = 0;
			zkrt_curser_state = 9;
		}
	}
	else if (zkrt_curser_state == 9)											//字节16，得到command
	{
		packet->command = ch;
		zkrt_update_checksum(packet,ch);
		zkrt_curser_state = 10;
	}
	else if (zkrt_curser_state == 10)											//字节17-46，接收DATA
	{
		packet->data[zkrt_dat_index] = ch;
		zkrt_update_checksum(packet,ch);
		zkrt_dat_index++;
		if (zkrt_dat_index == 30)
		{
			zkrt_dat_index = 0;
			zkrt_curser_state = 11;
		}
	}
	else if ((zkrt_curser_state == 11)&&(ch == (uint8_t)((packet->crc)&0xff)))	//字节47，CRC1
	{
		zkrt_curser_state = 12;
	}
	else if ((zkrt_curser_state == 12)&&(ch == (uint8_t)((packet->crc)>>8)))		//字节48，CRC2
	{
		zkrt_curser_state = 13;
	}
	else if ((zkrt_curser_state == 13)&&(ch == _END_CODE))											//字节49，结尾
	{
		packet->end_code = ch;
		zkrt_curser_state = 0;
		zkrt_recv_success = 1;
	}
	else
	{
		zkrt_curser_state = 0;
		zkrt_app_index = 0;
		zkrt_uav_index = 0;
		zkrt_dat_index = 0;
	}
	
	return zkrt_recv_success;
}




