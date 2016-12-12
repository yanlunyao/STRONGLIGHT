#include "zkrt.h"

//��������Ҫ�����������ɻ����͡��ɻ���š��¶����ޡ��¶����ޣ�����Ҫ��flash���棿����������������������������
//���к�ȫ�ֶ��壬Ȼ�����
uint8_t zkrt_tx_seq[DEVICE_NUMBER];               		//�ֽ�6��֡���кţ�Ϊ�����֣�ÿ��������Ҫ1��ר�ŵ����к�
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
���õ����������crcУ��������ĳһʱ��
���������ã���������ĳ���ֽ�У��
����ǰ��crcУ������ĳ����ֵ
���ú�crcУ������һ���µ���ֵ
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
���õ��������mavlink��5������У����֮��
���������ã���mavlink�е����ݲ��ֽ���crcУ��
����ǰ��crc�Ǹ��ݲ������ɵ�У����
���ú󣺵õ����ݲ���֮ǰ��У����
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
���õ��������mavlinkָ�����crcУ�飨�տ�ʼ��
���������ã���mavlinkָ���buf[5��1]��5������crcУ��
����ǰ��crcУ������������
���ú󣺸��ݲ������ɵ�crcУ����
 **/
uint16_t crc_calculate(const uint8_t* pBuffer, uint16_t length)
{
	uint16_t crcTmp = 0XFFFF;//У��������Ϊ0xffff
        
	while (length--) 
	{
		crc_accumulate(*pBuffer++, &crcTmp);//����������е�ÿ���ֽڽ���У��
	}
	return crcTmp;//����У����
}




//�÷��������������������վ����ָ�������Ԫ������Ԫ����ָ�������վ���ɿأ�����ģ�鴴��ָ�������Ԫ������Ԫ����ָ�����ģ�������û�У����Բ��ؿ���seq�ظ�
//���ܣ��ڷ��ʱ���������ֽ�ȡ�̶�ֵ���ֶν��з��
//�����������packet
//����ֵ����
//��һ���֣���������ȫ����õģ�
//1������仯�����Ժ궨��ģ�start��ver��length��end
//2����ʱ�仯����ʱ���á�������Ϊ0�ģ�session_ack��padding_enc
//3������仯����������Ϊ0�ģ�APPID
//4����ʱ�仯�����ÿ��ǵģ�seq��crc

//�ڶ����֣��������޷�ֱ�Ӷ����
//��ʱ�仯����ʱ���ͽ�ȥ�ģ�cmdѡ��0��1��uavid��command��data
void zkrt_final_encode(zkrt_packet_t *packet)
{
	uint8_t i;
	
	packet->start_code = _START_CODE;                             //�ֽ�0��֡��ʼ�룬0XEB
	packet->ver = _VERSION;                                       //�ֽ�1��Э��汾
	packet->session_ack = 0X00;                                   //�ֽ�2������Ӧ������֡
	packet->padding_enc = 0X00;                                   //�ֽ�3��������
	packet->length = _LENGTH;                                     //�ֽ�5��һ��30λ
	packet->seq = zkrt_tx_seq[packet->UAVID[3]];                  //�ֽ�6��֡���к�
	zkrt_tx_seq[packet->UAVID[3]]++;
	zkrt_tx_seq[packet->UAVID[3]]%=255;
	for(i=0;i<3;i++)
	packet->APPID[i]= 0x00;                                       //�ֽ�7-9��������ID������վ
	packet->crc = crc_calculate(((const uint8_t*)(packet)),47); //��47-48��У��λ
	packet->end_code = _END_CODE;                                 //��49��������
}




/*�Ե����ֽڵ�crc����У��*/
void zkrt_update_checksum(zkrt_packet_t* packet, uint8_t ch)
{
	uint16_t crc = packet->crc;
	
	crc_accumulate(ch,&(crc));//���ֱ�Ӵ���crc_accumulate(ch,&(packet->crc));�����ԣ���Ϊ�ṹ���Ա�ĵ�ַ����ֱ�Ӵ��Σ���ᵼ��hardfault����
	
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
	
	if ((zkrt_curser_state == 0)&&(ch == _START_CODE))		//�ֽ�0���õ���ʼ��
	{
		packet->start_code = ch;
		packet->crc = 0XFFFF;
		zkrt_update_checksum(packet,ch);
		zkrt_curser_state = 1;
	}
	else if ((zkrt_curser_state == 1)&&(ch == _VERSION))	//�ֽ�1���õ��汾��
	{
		packet->ver = ch;
		zkrt_update_checksum(packet,ch);
		zkrt_curser_state = 2;
	}
	else if (zkrt_curser_state == 2)											//�ֽ�2���õ�session
	{
		packet->session_ack = ch;
		zkrt_update_checksum(packet,ch);
		zkrt_curser_state = 3;
	}
	else if (zkrt_curser_state == 3)											//�ֽ�3���õ�padding
	{
		packet->padding_enc = ch;
		zkrt_update_checksum(packet,ch);
		zkrt_curser_state = 4;
	}
	else if (zkrt_curser_state == 4)											//�ֽ�4���õ�cmd
	{
		packet->cmd = ch;
		zkrt_update_checksum(packet,ch);
		zkrt_curser_state = 5;
	}
	else if ((zkrt_curser_state == 5)&&(ch == _LENGTH))		//�ֽ�5���õ�����
	{
		packet->length = ch;
		zkrt_update_checksum(packet,ch);
		zkrt_curser_state = 6;
	}
	else if (zkrt_curser_state == 6)											//�ֽ�6���������к�
	{
		packet->seq = ch;
		zkrt_update_checksum(packet,ch);
		zkrt_curser_state = 7;
	}
	else if (zkrt_curser_state == 7)											//�ֽ�7-9������APPID
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
	else if (zkrt_curser_state == 8)											//�ֽ�10-15������UAVID
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
	else if (zkrt_curser_state == 9)											//�ֽ�16���õ�command
	{
		packet->command = ch;
		zkrt_update_checksum(packet,ch);
		zkrt_curser_state = 10;
	}
	else if (zkrt_curser_state == 10)											//�ֽ�17-46������DATA
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
	else if ((zkrt_curser_state == 11)&&(ch == (uint8_t)((packet->crc)&0xff)))	//�ֽ�47��CRC1
	{
		zkrt_curser_state = 12;
	}
	else if ((zkrt_curser_state == 12)&&(ch == (uint8_t)((packet->crc)>>8)))		//�ֽ�48��CRC2
	{
		zkrt_curser_state = 13;
	}
	else if ((zkrt_curser_state == 13)&&(ch == _END_CODE))											//�ֽ�49����β
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




