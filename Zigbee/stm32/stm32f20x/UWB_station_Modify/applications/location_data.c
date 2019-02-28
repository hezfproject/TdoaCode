#include <rtthread.h>
#include <dfs_posix.h>

#include "nwk_protocol.h"
#include "msg_center.h"

//#define LOG_DEBUG
#include "3g_log.h"

#define DATA_FILE_MAX_SIZE  (1024 * 1024 * 12)
//#define DATA_FILE_MAX_SIZE  (1024 * 84)
#define BLOCK_CNT   12
#define BLOCK_SIZE   DATA_FILE_MAX_SIZE/BLOCK_CNT





struct data_block_state
{
	unsigned int   r_pos;
  unsigned int   w_pos;
  unsigned int   state;
};

struct data_rw_state
{
	unsigned int	current_write_block;
	struct data_block_state block_state[BLOCK_CNT];
};


static struct data_rw_state ldata_state;

static char sav_data_file[] = "/ldata.1";
static int sav_data_fd = -1;


void ldata_state_init(void)
{
	memset(&ldata_state, 0, sizeof(ldata_state));
}



int dump_location_data(const unsigned char* p_data, int data_len)
{
    rt_err_t err = 0;
	unsigned int  current_write;
    if (p_data == NULL)
	{
		return -RT_ERROR;
	}

    if (data_len == 0)
    {
        return 0;
    }
    if (sav_data_fd < 0)
    {
        unlink(sav_data_file);
        sav_data_fd = open(sav_data_file, O_CREAT | O_RDWR, 0);
        if (sav_data_fd < 0)
        {
            err = rt_get_errno();
            ERROR_LOG("open file: %s failed, err code %d\n",
            sav_data_file, err);
            err = 1;
        }
    }
		current_write = ldata_state.current_write_block;


	if(ldata_state.block_state[current_write].w_pos+data_len < BLOCK_SIZE )
	{
		lseek(sav_data_fd, current_write * BLOCK_SIZE + ldata_state.block_state[current_write].w_pos, SEEK_SET);
		write(sav_data_fd, p_data, data_len);
		ldata_state.block_state[current_write].w_pos += data_len;
		ldata_state.block_state[current_write].state = 1;
	}
	else
	{
		current_write=(++current_write) % BLOCK_CNT;
		if(current_write == 0)
            DEBUG_LOG("write full\r\n");
		ldata_state.current_write_block=current_write;
		ldata_state.block_state[current_write].state=0;
		ldata_state.block_state[current_write].w_pos=0;
		ldata_state.block_state[current_write].r_pos=0;

		lseek(sav_data_fd, current_write*BLOCK_SIZE + ldata_state.block_state[current_write].w_pos, SEEK_SET);
		write(sav_data_fd, p_data, data_len);
		ldata_state.block_state[current_write].w_pos += data_len;
		ldata_state.block_state[current_write].state = 1;
		/*for(j=0;j<BLOCK_CNT;j++)
		{
			rt_kprintf("current_write_block state=0x%x\r\n",ldata_state.current_write_block);
			rt_kprintf("read %x state=0x%x\r\n",j,ldata_state.block_state[j].state);
			rt_kprintf("read %x w_pos=0x%x\r\n",j,ldata_state.block_state[j].w_pos);
			rt_kprintf("read %x r_pos=0x%x\r\n",j,ldata_state.block_state[j].r_pos);

		}*/
	}
					//rt_kprintf("write data  0x%x state=0x%x w_pos=0x%x\r\n",current_write,ldata_state.block_state[current_write].state,ldata_state.block_state[current_write].w_pos);
    return data_len;
}

int load_location_data(unsigned char* p_buf, int buf_len)
{
    PACKET_HEADER_T* pkt_hdr = NULL;
    unsigned int pkt_hdr_size = sizeof(PACKET_HEADER_T);
    unsigned int pkt_size = 0;
	int i;
	unsigned int  current_read;

    if (sav_data_fd < 0)
    {
        return -RT_EEMPTY;
    }
    if ((p_buf == NULL) || (buf_len == 0))
    {
        return -RT_ERROR;
    }
		current_read = ldata_state.current_write_block+1;
		current_read %= BLOCK_CNT;
		for(i = 0;i < BLOCK_CNT;i++)
		{
			if(ldata_state.block_state[current_read].state != 0)
			{
				if(ldata_state.block_state[current_read].r_pos < ldata_state.block_state[current_read].w_pos )
				{
					lseek(sav_data_fd, current_read*BLOCK_SIZE + ldata_state.block_state[current_read].r_pos, SEEK_SET);
					read(sav_data_fd, p_buf, pkt_hdr_size);
					pkt_hdr = (PACKET_HEADER_T*)p_buf;
					pkt_size = pkt_hdr_size + pkt_hdr->len;
					read(sav_data_fd, p_buf + pkt_hdr_size, pkt_hdr->len);
					ldata_state.block_state[current_read].r_pos += pkt_size;
					//rt_kprintf("read data  0x%x state=0x%x r_pos=0x%x\r\n",current_read,ldata_state.block_state[current_read].state,ldata_state.block_state[current_read].r_pos);
					break;
				}
				else
				{
				/*rt_kprintf("read %x state=0x%x\r\n",current_read,ldata_state.block_state[current_read].state);
				rt_kprintf("read %x w_pos=0x%x\r\n",current_read,ldata_state.block_state[current_read].w_pos);
				rt_kprintf("read %x r_pos=0x%x\r\n",current_read,ldata_state.block_state[current_read].r_pos);*/

					ldata_state.block_state[current_read].state = 0;
					ldata_state.block_state[current_read].w_pos = 0;
					ldata_state.block_state[current_read].r_pos = 0;
				}
			}
			if(current_read == (BLOCK_CNT - 1))
				current_read = 0;
			else
				current_read++;
		}

		if(i == BLOCK_CNT)
			return -RT_EEMPTY;
		else
			return pkt_size;
}



