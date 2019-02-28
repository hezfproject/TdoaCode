/*
 * Hardware_Timer.c
 *
 *  Created on: 2011-5-7
 *      Author: Administrator
 */

PUBLIC Timer_Init (uint8 timer_num)
{
    void vAHI_TimerClockSelect(timer_num,FALSE,FALSE);
    void vAHI_TimerEnable(timer_num,uint8 u8Prescale,bool_t bIntRiseEnable,bool_t bIntPeriodEnable,bool_t bOutputEnable);
}


bool MBusMac::SendCmd(eMBusCmd cmd, uint_16 dst_id, uint_8 seq, const void * buf, uint_16 size)
{
	bool ret = false;
	uint_16 buf_size = buf ? size : 0;
	buf_size += sizeof(mbus_hdr_mstr_t) + 2;
	mbus_hdr_mstr_t * phdr = (mbus_hdr_mstr_t *)malloc(buf_size);
	if (NULL == phdr) {
		return false;
	}
	if (buf_size > sizeof(mbus_hdr_mstr_t) + 2) {
		MBUS_SET_MASTER_DATAFLAG(phdr->frame_control, true);
		//phdr->data_flag = true;
	} else {
		MBUS_SET_MASTER_DATAFLAG(phdr->frame_control, false);
		//phdr->data_flag = false;
	}
	MBUS_SET_MASTER_SEQ(phdr->frame_control, seq);
	MBUS_SET_MASTER_VERSION(phdr->frame_control, MBUS_PROTO_VERSION);
	//phdr->seq = seq;
	//phdr->version = MBUS_PROTO_VERSION;
	phdr->cmd = cmd;
	phdr->slv_id = dst_id;
	if (buf) {
		char * plen = (char *)(phdr + 1);
		memcpy(plen, buf, size);
		//memcpy(pdata, buf, size);
	}

	uint_16 data_len = buf ? size : 0;
	char * pcrc = (char *)phdr + sizeof(mbus_hdr_mstr_t) + data_len;
	uint_16 crc = CRC16((uint_8 *)phdr, buf_size - 2, 0xFFFF);
	memcpy(pcrc, &crc, sizeof(uint_16));
	if (m_dev) {
		m_dev->Write((char *)phdr, buf_size);
		////////////////////////////////////
		switch (cmd) {
		case MBUS_CMD_CLR:
		case MBUS_CMD_QRY:
		case MBUS_CMD_RETREAT:
		case MBUS_CMD_CANCEL_RETREAT:
		{
			DPRINT("\nModbus Mac:Send ");
			DPR_MEM((char *)phdr, buf_size);
			DPRINT("\n");
			break;
		}
		case MBUS_CMD_EXECUTE:
		{
			usleep(1000*1000);
			break;
		}
		default:
			break;
		}
		////////////////////////////////////
		ret = true;
	}

	free(phdr);
	ret = false;

	return ret;
}
