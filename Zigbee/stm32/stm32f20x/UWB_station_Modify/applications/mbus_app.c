#include "mbus_app.h"

#include <rtthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "stm32f2xx.h"
#include "usart.h"

#include "commontypes.h"
#include "mbusproto.h"
#include "cmd_type.h"
#include "3g_protocol.h"
#include "crc.h"
#include "devrange.h"

#include "msg_center.h"
#include "bootcfg.h"
#include "net_app.h"
#include "led_indicator.h"
#include "3g_thread.h"
#include "../../../../../version.h"

//#define LOG_DEBUG
#include "3g_log.h"

/*
** ip网络收发缓冲区
*/

static rt_uint8_t mbus_net_msg_buf[MBUS_NET_MSG_BUF_SIZE];

/*
** 接收工控机下发数据的消息队列的定义
**
*/
#define MBUS_POOL_MAX   (4 * 1024)
rt_uint8_t u8MbusPool[MBUS_POOL_MAX];
struct rt_messagequeue mbus_rec_mq;

/*
 * 上报工控机数据消息队列定义
 */
 rt_uint8_t mbus_reported_msg_pool[8 * 1024];
struct rt_messagequeue mbus_reported_mq;

/*
** mbus设备
*/
typedef struct mbus_t
{
    rt_bool_t rx_ind;
    rt_device_t device;
}MBUS_T;

static MBUS_T mbus;

/*
** modbus主从通信数据缓冲区
*/
#define MBUS_COMM_BUF_SIZE 1024
static uint_8 mbus_comm_buf[MBUS_COMM_BUF_SIZE];

/*
** modbus总线节点连接状态值
*/
enum
{
    MBUS_NODE_IDLE = 0,
    MBUS_NODE_NON_IDLE,
    MBUS_NODE_CARD_READER,
    MBUS_NODE_SENSOR_STATION,
    MBUS_NODE_POWER_SENSOR,
};

enum
{
    CARD_READER_NO_CMD = 0,
    CARD_READER_RETREAT_CMD,
    CARD_READER_CANCEL_RETREAT_CMD,
};

static uint_8 mbus_node_state[MBUS_IDMAX + 1];
static uint_8 mbus_node_data_query_err_cnt[MBUS_IDMAX + 1];

#define CARD_DATA_REPORT_INTERVAL_SEC   (5)
#define SENSOR_DATA_REPORT_INTERVAL_SEC (5)
#define LINK_REPORT_INTERVAL_SEC        (120)
#define MBUS_NODE_DATA_QUERY_MAX_CNT    (20)

#define MBUS_ENABLE_SEND_MSG()  \
    do { GPIO_SetBits(GPIOE, GPIO_Pin_4); } while (0)

#define MBUS_DISABLE_SEND_MSG()  \
    do { GPIO_ResetBits(GPIOE, GPIO_Pin_4); } while (0)

static uint_8 version_req_seq = 0;  ///< 工控机查询版本信息的seq，上报时用

#define MBUS_ENABLE_RECV_MSG()   \
    do { UART_ENABLE_IRQ(USART1_IRQn);} while (0)

#define MBUS_DISABLE_RECV_MSG()   \
    do { UART_DISABLE_IRQ(USART1_IRQn);} while (0)

#define SOC "STM32F207"

#define VERSION_REPORT_INTERVAL_SEC     (3600)

/*
 * 写上报工控机消息队列
 */
static int _mbus_write_reported_msg_queue(void* p_buf, int size)
{
    rt_err_t err = 0;

    RT_ASSERT(p_buf != RT_NULL);

    if (!size) {
        return size;
    }

    err = rt_mq_send(&mbus_reported_mq, p_buf, size);
    if (err != RT_EOK) {
        ERROR_LOG("write mbus rep mq ret %d\n", err);
        return -1;
    }

    return size;
}

/*
** 通过网络上报工控机传感基站连接信息
*/
static void _mbus_report_card_link()
{
    uint_16 i = MBUS_IDMIN;
    uint_16 link_count = 0;
    PACKET_HEADER_T* hdr = (PACKET_HEADER_T*)mbus_net_msg_buf;
    comm_card_link_rpt_t* card_link = (comm_card_link_rpt_t *)(hdr + 1);
    uint_16* link_id = (uint_16*)(card_link + 1);

    memset(mbus_net_msg_buf, 0, MBUS_NET_MSG_BUF_SIZE);
    card_link->timestamp = time(RT_NULL);
    card_link->src_pan = GET_STATION_PAN_ID();
    for (; i <= MBUS_IDMAX; i++)
    {
        if (MBUS_NODE_CARD_READER == mbus_node_state[i])
        {
            link_count++;
            *link_id = i + MBUS_RDR_BASE;
            link_id++;
        }
    }
    card_link->datelen = link_count;

    hdr->tag = COMM_CARD_LINK_RPT;
    hdr->len = sizeof(comm_card_link_rpt_t) +
                card_link->datelen * sizeof(uint_16);
    _mbus_write_reported_msg_queue(mbus_net_msg_buf,
        sizeof(PACKET_HEADER_T) + hdr->len);
}

/*
** 通过网络上报工控机传感基站连接信息
*/
static void _mbus_report_sensor_link()
{
    uint_16 i = MBUS_IDMIN;
    uint_16 link_count = 0;
    PACKET_HEADER_T* hdr = (PACKET_HEADER_T*)mbus_net_msg_buf;
    comm_sensor_link_rpt_t *sensor_link = (comm_sensor_link_rpt_t *)(hdr + 1);
    uint_16* link_id = (uint_16*)(sensor_link + 1);

    memset(mbus_net_msg_buf, 0, MBUS_NET_MSG_BUF_SIZE);
    sensor_link->timestamp = time(RT_NULL);
    sensor_link->channel = 0;
    sensor_link->src_pan = GET_STATION_PAN_ID();
    for (; i <= MBUS_IDMAX; i++)
    {
        if (MBUS_NODE_SENSOR_STATION == mbus_node_state[i])
        {
            link_count++;
            *link_id = i + MBUS_SENSOR_RDR_BASE;
            link_id++;
        }
    }
    sensor_link->count = link_count;

    hdr->tag = COMM_SENSOR_LINK_RPT;
    hdr->len = sizeof(comm_sensor_link_rpt_t) +
                sensor_link->count * sizeof(uint_16);
    _mbus_write_reported_msg_queue(mbus_net_msg_buf,
        sizeof(PACKET_HEADER_T) + hdr->len);
}

static void _mbus_report_net_node_link()
{
    TIME_LOG(LOG_INFO, "mbus report node link stauts\n");
    _mbus_report_card_link();
    _mbus_report_sensor_link();
}

/*
** 通过网络上报工控机传感基站连接信息
*/
static void _mbus_report_card_data(uint_16 dev_id, const void* data_buf,
                                        uint_16 data_len)
{
    PACKET_HEADER_T* hdr = (PACKET_HEADER_T*)mbus_net_msg_buf;
    comm_card_rpt_t* card_data = (comm_card_rpt_t *)(hdr + 1);

    if ((NULL == data_buf) || (0 == data_len))
    {
        //ERROR_LOG("para error, buf 0x%x, len %d\n", data_buf, data_len);
        return;
    }

    if (data_len + sizeof(PACKET_HEADER_T) + sizeof(comm_card_rpt_t) >
        MBUS_NET_MSG_BUF_SIZE )
    {
        ERROR_LOG("net report card data length %d beyond the buffer size\n",
            data_len);
        return;
    }

    memset(mbus_net_msg_buf, 0, MBUS_NET_MSG_BUF_SIZE);
    memcpy((void*)(card_data + 1), data_buf, data_len);
    card_data->timestamp = time(RT_NULL);
    card_data->src_pan = GET_STATION_PAN_ID();
    card_data->dev_id = dev_id + MBUS_RDR_BASE;
    card_data->datalen = data_len;

    hdr->tag = COMM_CARD_RPT;
    hdr->len = sizeof(comm_card_rpt_t) + card_data->datalen;
    _mbus_write_reported_msg_queue(mbus_net_msg_buf,
        sizeof(PACKET_HEADER_T) + hdr->len);

    DEBUG_LOG("mbus report card reader %d data len %d\n",
        card_data->dev_id, data_len);
}

/*
** 通过网络上报工控机传感基站连接信息
*/
static void _mbus_report_sensor_data(uint_16 dev_id, const void* data_buf,
                                            uint_16 data_len)
{
    PACKET_HEADER_T* hdr = (PACKET_HEADER_T*)mbus_net_msg_buf;
    comm_sensor_data_rpt_t* sensor_data = (comm_sensor_data_rpt_t *)(hdr + 1);

    if ((NULL == data_buf) || (data_len != sizeof(mbus_sensor_packed_t)))
    {
        ERROR_LOG("para error, buf 0x%x, len %d\n", data_buf, data_len);
        return;
    }

    memset(mbus_net_msg_buf, 0, MBUS_NET_MSG_BUF_SIZE);
    memcpy(&sensor_data->new_sensor_data, data_buf, data_len);
    sensor_data->timestamp = time(RT_NULL);
    sensor_data->src_pan = GET_STATION_PAN_ID();
    sensor_data->dev_id = dev_id + MBUS_SENSOR_RDR_BASE;

    hdr->tag = COMM_NEW_SENSOR_RPT;
    hdr->len = sizeof(comm_sensor_data_rpt_t);
    _mbus_write_reported_msg_queue(mbus_net_msg_buf,
        sizeof(PACKET_HEADER_T) + hdr->len);

    DEBUG_LOG("mbus report sensor station %d data\n", sensor_data->dev_id);
}

/*
** 通过网络上报工控机传感基站连接信息
*/
static void _mbus_report_power_sensor_data(const void* data_buf,
                                            uint_16 data_len)
{
    PACKET_HEADER_T* hdr = (PACKET_HEADER_T*)mbus_net_msg_buf;
    power_informati_t_Temp* sensor_data = (power_informati_t_Temp *)(hdr + 1);

    if ((NULL == data_buf) || (data_len != sizeof(power_informati_t_Temp)))
    {
        ERROR_LOG("power sensor para error, buf 0x%x, len %d\n", data_buf, data_len);
        return;
    }

    memset(mbus_net_msg_buf, 0, MBUS_NET_MSG_BUF_SIZE);
    memcpy(sensor_data, data_buf, data_len);
    hdr->tag = COMM_POWER_SENSOR_RPT;
    hdr->len = sizeof(power_informati_t_Temp);
    _mbus_write_reported_msg_queue(mbus_net_msg_buf,
        sizeof(PACKET_HEADER_T) + hdr->len);

}



/*
** 通过网络上报工控机传感基站配置信息
*/
static void _mbus_report_sensor_config(uint_16 pan_id, const void* data_buf,
                                            uint_16 data_len)
{
    PACKET_HEADER_T* hdr = (PACKET_HEADER_T*)mbus_net_msg_buf;
    comm_sensor_config_t* sensor_config = (comm_sensor_config_t *)(hdr + 1);

    if ((NULL == data_buf) || (0 == data_len))
    {
        ERROR_LOG("para error, buf 0x%x, len %d\n", data_buf, data_len);
        return;
    }
    if (data_len + sizeof(PACKET_HEADER_T) + sizeof(comm_sensor_config_t) >
        MBUS_NET_MSG_BUF_SIZE )
    {
        ERROR_LOG("net report sensor config length %d beyond the buffer size\n",
            data_len);
        return;
    }

    memset(mbus_net_msg_buf, 0, MBUS_NET_MSG_BUF_SIZE);
    memcpy((void*)(sensor_config + 1), data_buf, data_len);
    sensor_config->timestamp = time(RT_NULL);
    sensor_config->src_pan = GET_STATION_PAN_ID();
    sensor_config->channel = 0;
    sensor_config->dev_id = pan_id;
    sensor_config->datalen = data_len;

    hdr->tag = COMM_SENSOR_QUERY_CONFIG;
    hdr->len = sizeof(comm_sensor_config_t) + sensor_config->datalen;
    _mbus_write_reported_msg_queue(mbus_net_msg_buf,
        sizeof(PACKET_HEADER_T) + hdr->len);
}

static void _mbus_report_master_version()
{
    PACKET_HEADER_T* pkt_hdr = (PACKET_HEADER_T*)mbus_net_msg_buf;
    version_rsp_t*  ver_hdr = (version_rsp_t*)(pkt_hdr + 1);
    char* ver = (char*)(ver_hdr + 1);
    char buf[256] = {0};

    snprintf(buf, sizeof(buf),
        "Hardware: %s Software: Release %s Version %s Build %s %s",
        SOC, RELEASE, VERSION, __DATE__, __TIME__);

    memset(mbus_net_msg_buf, 0, MBUS_NET_MSG_BUF_SIZE);
    ver_hdr->panid = GET_STATION_PAN_ID();
    ver_hdr->timestamp = time(RT_NULL);
    ver_hdr->seq = version_req_seq;
    ver_hdr->size = strlen(buf);
    memcpy(ver, buf, ver_hdr->size);

    pkt_hdr->tag = COMM_VERSION_RSP;
    pkt_hdr->len = sizeof(version_rsp_t) + ver_hdr->size;
    _mbus_write_reported_msg_queue(mbus_net_msg_buf,
        sizeof(PACKET_HEADER_T) + pkt_hdr->len);
    DEBUG_LOG("report mbus master version %s\n", buf);
}

static void _mbus_report_slave_version(uint_16 pan_id,
                                                const void* data_buf,
                                                uint_16 data_len)
{
    PACKET_HEADER_T* pkt_hdr = (PACKET_HEADER_T*)mbus_net_msg_buf;
    version_rsp_t*  ver_hdr = (version_rsp_t*)(pkt_hdr + 1);
    char* ver = (char*)(ver_hdr + 1);
    int ver_len = (data_len > 512) ? 512 : data_len;

    memset(mbus_net_msg_buf, 0, MBUS_NET_MSG_BUF_SIZE);
    ver_hdr->panid = pan_id;
    ver_hdr->timestamp = time(RT_NULL);
    ver_hdr->seq = version_req_seq;
    ver_hdr->size = ver_len;
    memcpy(ver, data_buf, ver_hdr->size);

    pkt_hdr->tag = COMM_VERSION_RSP;
    pkt_hdr->len = sizeof(version_rsp_t) + ver_hdr->size;
    _mbus_write_reported_msg_queue(mbus_net_msg_buf,
        sizeof(PACKET_HEADER_T) + pkt_hdr->len);
    DEBUG_LOG("report mbus slave %d version %s\n", pan_id, ver);
}

/*
** modbus 总线通过485发送一帧数据，每帧数据发送时间间隔至少为7个字符时间
*/
static rt_bool_t _mbus_send_one_frame_data(const void* data_buf,
                                                    uint_16 data_len)
{
    char* buf_pos = (char*)data_buf;
    uint_16 left_size = data_len;

    RT_ASSERT((data_buf != RT_NULL) && (data_len != 0));

    while (left_size > 0)
    {
        int_16 ret = rt_device_write(mbus.device, 0, buf_pos, left_size);
        if (ret < 0)
        {
            break;
        }
        buf_pos += ret;
        left_size -= ret;
    }

    return (!left_size) ? RT_TRUE : RT_FALSE;
}

/*
** modbus 总线通过485接收一帧数据，整帧数据为连续的流传输，有超过1.5个字符时间
** 的停顿时间则认为一帧数据接收完成。
*/
static uint_16 _mbus_rec_one_frame_data(const void* data_buf,
                                                uint_16 data_len)
{
    char* buf_pos = (char*)data_buf;
    uint_16 rec_size = 0;
    uint_16 pause_time = 0;
    int_16 ret = 0;

    RT_ASSERT((data_buf != RT_NULL) && (data_len != 0));

    while (1)
    {
        if (rec_size >= data_len)
        {
            ERROR_LOG("modbus rec buffer is full\n");
            break;
        }

        ret = rt_device_read(mbus.device, 0, buf_pos, data_len - rec_size);
        if (ret <= 0)
        {
            /* 在10ms内没有接收到数据认为接收完一帧数据 */
            if (++pause_time > 2000)
            {
                break;
            }
        }
        else
        {
            pause_time = 0;
            buf_pos += ret;
            rec_size += ret;
        }
    }

    return rec_size;
}

/*
** modbus 消息处理，处理来自从机的数据
*/

static rt_bool_t _mbus_data_handler(uint_16 slv_id, const void* p_data,
                                            uint_16 data_len)
{
    char* data = (char*)p_data;
    char* data_tail = data + data_len;
    mbus_tlv_t* p_tlv = RT_NULL;

    if ((RT_NULL == p_data) || (0 == data_len))
    {
        return RT_TRUE;
    }

    while (1)
    {
        p_tlv = (mbus_tlv_t*)data;
        if (data + sizeof(mbus_tlv_t) + p_tlv->len > data_tail)
        {
            ERROR_LOG("data lenght %d error, should be tlv length %d + %d\n",
                data_tail - data, sizeof(mbus_tlv_t), p_tlv->len);
            return RT_FALSE;
        }

        switch (p_tlv->type)
        {
        case MBUS_TLV_CARD_READER:
            mbus_node_state[slv_id] = MBUS_NODE_CARD_READER;
            _mbus_report_card_data(slv_id, (void*)(p_tlv + 1), p_tlv->len);
            break;
        case MBUS_TLV_SENSOR_READER_V2:
            mbus_node_state[slv_id] = MBUS_NODE_SENSOR_STATION;
            _mbus_report_sensor_data(slv_id, (void*)(p_tlv + 1), p_tlv->len);
            break;
				case 0x23:
            mbus_node_state[slv_id] = MBUS_NODE_POWER_SENSOR;
            _mbus_report_power_sensor_data((void*)(p_tlv + 1), p_tlv->len);
            break;
        case MBUS_TLV_SENSOR_READER_V3_CONFIG:
            _mbus_report_sensor_config(slv_id + MBUS_SENSOR_RDR_BASE,
                (void*)(p_tlv + 1), p_tlv->len);
            break;
        case MBUS_TLV_VERSION_CARD_READER:
            _mbus_report_slave_version(slv_id + MBUS_RDR_BASE,
                (void*)(p_tlv + 1), p_tlv->len);
            break;
        case MBUS_TLV_VERSION_SENSOR_READER_V3:
            _mbus_report_slave_version(slv_id + MBUS_SENSOR_RDR_BASE,
                (void*)(p_tlv + 1), p_tlv->len);
            break;
        default:
            ERROR_LOG("unknown data type %d\n", p_tlv->type);
            break;
        }

        data += sizeof(mbus_tlv_t) + p_tlv->len;
        if (data >= data_tail)
        {
            break;
        }
    }
    return RT_TRUE;
}

/*
** modbus 主机向从机发送查询消息
*/
static rt_bool_t _mbus_send_query_msg(uint_16 slv_id, uint_8 cmd,
                                            uint_8 *p_data, uint_16 data_len,
                                            uint_8 seq, uint_8 *p_msg_buf,
                                            uint_16 msg_buf_len)
{
    uint_16 crc = 0;
    uint_16 data_size = (p_data) ? data_len : 0;
    uint_8 data_flag = (data_size) ? 1 : 0;
    mbus_hdr_mstr_t *mstr_hdr = (mbus_hdr_mstr_t*)p_msg_buf;
    uint_16 msg_size = sizeof(mbus_hdr_mstr_t) + data_size + sizeof(crc);

    if ((NULL == p_msg_buf) || (0 == msg_buf_len))
    {
        ERROR_LOG("msg buf is null\n");
        return RT_FALSE;
    }

    if (msg_size > MBUS_COMM_BUF_SIZE)
    {
        ERROR_LOG("mbus comm data is too big, len %d\n", data_size);
        return RT_FALSE;
    }

    mstr_hdr->cmd = cmd;
    mstr_hdr->slv_id = slv_id;
    MBUS_SET_MASTER_SEQ(mstr_hdr->frame_control, seq);
    MBUS_SET_MASTER_VERSION(mstr_hdr->frame_control, MBUS_PROTO_VERSION);
    MBUS_SET_MASTER_DATAFLAG(mstr_hdr->frame_control, data_flag);
    if (p_data != RT_NULL)
    {
        memcpy(mstr_hdr + 1, p_data, data_len);
    }
    crc = CRC16(p_msg_buf, msg_size - sizeof(crc), 0xFFFF);
    *(uint_16*)(p_msg_buf + sizeof(mbus_hdr_mstr_t) + data_size) = crc;

#ifdef LOG_DEBUG
    //mem_printf("mbus send data: ", (char*)p_msg_buf, msg_size);
#endif

    if (_mbus_send_one_frame_data(p_msg_buf, msg_size))
    {
        return RT_TRUE;
    }
    else
    {
        ERROR_LOG("mbus send msg packet error\n");
        return RT_FALSE;
    }
}

/*
** modbus 主机接收从机的回应消息
*/
static rt_bool_t _mbus_rec_respond_msg(uint_8 *p_msg_buf, uint_16 buf_len)
{
    rt_bool_t wait_rec = RT_TRUE;
    rt_tick_t start_tick = rt_tick_get();

    if ((NULL == p_msg_buf) || (0 == buf_len))
    {
        ERROR_LOG("msg buf is null\n");
        return RT_FALSE;
    }

    while (1)
    {
        uint_16 size = 0;
        int_16 pos = 0;
        int_16 index = 0;
        mbus_hdr_slv_t* slv_hdr = RT_NULL;

        if (mbus.rx_ind)
        {
            size = _mbus_rec_one_frame_data(p_msg_buf, buf_len);
#ifdef LOG_DEBUG
            //mem_printf("mbus rec: ", (char*)p_msg_buf, size);
#endif
            mbus.rx_ind = RT_FALSE;
            wait_rec = RT_FALSE;

            if (!size)
            {
                return RT_FALSE;
            }

            if (size < MBUS_SYNC_SIZE)
            {
                ERROR_LOG("mbus rec frame data size %d error\n", size);
                return RT_FALSE;
            }

            /// find sync head
            while (pos < size)
            {
                if (p_msg_buf[pos] == MBUS_SYNC[index])
                {
                    index++;
                }
                else
                {
                    index = 0;
                }

                pos++;

                if (index == MBUS_SYNC_SIZE)
                {
                    break;
                }
            }
            if (pos >= size)
            {
                ERROR_LOG("sync head YIRI is not found\n");
                return RT_FALSE;
            }

            if (pos - MBUS_SYNC_SIZE + sizeof(mbus_hdr_slv_t) >= size)
            {
                ERROR_LOG("mbus rec msg length is too short\n");
                return RT_FALSE;
            }

            slv_hdr = (mbus_hdr_slv_t*)(p_msg_buf + pos - MBUS_SYNC_SIZE);
            if ((pos - MBUS_SYNC_SIZE + sizeof(mbus_hdr_slv_t) +
                    slv_hdr->data_len + sizeof(uint_16)) > size)
            {
                ERROR_LOG("mbus rec msg length is too short\n");
                return RT_FALSE;
            }

            if (pos > MBUS_SYNC_SIZE )
            {
                for (index = 0; index < size; index++)
                {
                    p_msg_buf[index]  = p_msg_buf[pos - MBUS_SYNC_SIZE + index];
                }
            }
            return RT_TRUE;
        }

        if (wait_rec &&
            (rt_tick_get() - start_tick >
            MBUS_FRAME_INTERVAL * RT_TICK_PER_SECOND / 1000))
        {
            return RT_FALSE;
        }
    }
}

/*
** modbus 主机采用单独和从机通信，从机返回回应消息
*/
static rt_bool_t _mbus_individual_comm(uint_16 slv_id, uint_8 cmd,
                                        uint_8 *p_data, uint_16 data_len,
                                        uint_8 seq)
{
    mbus_hdr_slv_t *slv_hdr = RT_NULL;
    uint_16 send_crc = 0;
    uint_16 rec_crc = 0;
    rt_bool_t ret = RT_FALSE;

    memset(mbus_comm_buf, 0, MBUS_COMM_BUF_SIZE);
    ret = _mbus_send_query_msg(slv_id, cmd, p_data, data_len, seq,
                                mbus_comm_buf, MBUS_COMM_BUF_SIZE);
    if (!ret)
    {
        ERROR_LOG("%u mbus send slv_id %d cmd %d failed\n",
            time(RT_NULL), slv_id, cmd);
        return RT_FALSE;
    }

    THREAD_MSLEEP(100);
    //rt_kprintf("%u mbus send slv_id %d cmd %d\n", time(RT_NULL), slv_id, cmd);

    memset(mbus_comm_buf, 0, MBUS_COMM_BUF_SIZE);
    ret = _mbus_rec_respond_msg(mbus_comm_buf, MBUS_COMM_BUF_SIZE);
    if (!ret)
    {
        if (MBUS_CMD_QRY == cmd) {
            ERROR_LOG("mbus slv %d do not respond\n", slv_id);
        }
        return RT_FALSE;
    }

    slv_hdr = (mbus_hdr_slv_t*)mbus_comm_buf;
    if (slv_hdr->slv_id != slv_id)
    {
        ERROR_LOG("mbus rec slv_id %d is not send slv_id %d\n",
            slv_hdr->slv_id, slv_id);
        return RT_FALSE;
    }
    if ((slv_hdr->cmd != MBUS_CMD_RSP) &&
        (slv_hdr->cmd != MBUS_CMD_VERSION_RSP))
    {
		ERROR_LOG("mbus rec cmd %d is not a rsp cmd\n", slv_hdr->cmd);
		return RT_FALSE;
	}
    send_crc = *(uint_16*)(mbus_comm_buf + sizeof(mbus_hdr_slv_t) + slv_hdr->data_len);
    rec_crc = CRC16(mbus_comm_buf, sizeof(mbus_hdr_slv_t) + slv_hdr->data_len, 0xFFFF);
    if (send_crc != rec_crc)
    {
		ERROR_LOG("mbus rec crc %d is not send crc %d\n", rec_crc, send_crc);
		return RT_FALSE;
    }

    return _mbus_data_handler(slv_id, (void*)(slv_hdr + 1), slv_hdr->data_len);
}

/*
** modbus 主机采用广播方式与从机通信，从机不做回应
*/
static rt_bool_t _mbus_broadcast(uint_8 cmd, uint_8 *p_data, uint_16 data_len)
{
    return _mbus_send_query_msg(MBUS_IDBROADCAST, cmd, p_data, data_len, 0,
                                mbus_comm_buf, MBUS_COMM_BUF_SIZE);
}

/*
** modbus 主机向从机广播同步时间消息
*/
static void _mbus_syn_slave_time()
{
    uint_8 buf[sizeof(mbus_tlv_t) + sizeof(mbus_time_t)] = {0};
    mbus_tlv_t* p_tlv = (mbus_tlv_t*)buf;
    p_tlv->type = MBUS_TLV_REAL_TIME;
    p_tlv->len = sizeof(mbus_time_t);
    *(mbus_time_t*)(p_tlv + 1) = time(RT_NULL);

    _mbus_broadcast(MBUS_CMD_SYN_SLAVE_TIME, buf, sizeof(buf));
}

static void _mbus_send_card_alarm_ack(uint_16 card_reader_id,
                                                uint_16 card_id)
{
    uint_8 buf[sizeof(mbus_tlv_t) + sizeof(uint_16)] = {0};
    mbus_tlv_t* p_tlv = (mbus_tlv_t*)buf;
    p_tlv->type = MBUS_TLV_CARD_ALARM_HELP_ACK;
    p_tlv->len = sizeof(uint_16);
    *(uint_16*)(p_tlv + 1) = card_id;
    _mbus_send_query_msg(card_reader_id, MBUS_CMD_ALARM_ACK,
        buf, sizeof(buf), 0, mbus_comm_buf, MBUS_COMM_BUF_SIZE);
    THREAD_MSLEEP(100);
    DEBUG_LOG("mbus send card alarm ack slv id %d card id %d\n",
        card_reader_id, card_id);
}

/*
** 通过modbus总线通信查询从机连接状态信息
*/
static uint_8 _mbus_query_slv_state(uint_16 dev_id)
{
    uint_8 state = MBUS_NODE_IDLE;

    if (mbus_node_state[dev_id] != MBUS_NODE_IDLE)
    {
        return mbus_node_state[dev_id];
    }

    if (_mbus_individual_comm(dev_id, MBUS_CMD_CLR, NULL, 0, 0))
    {
        state = MBUS_NODE_NON_IDLE;
    }

    return state;
}

/*
** 通过modbus总线通信查询从机数据信息
*/
static rt_bool_t _mbus_query_slv_data(uint_16 dev_id)
{
    static uint_8 seq[MBUS_IDMAX + 1] = {0};
    uint_8 i = 0;

    for (; i < 2; i++)
    {
        if (_mbus_individual_comm(dev_id, MBUS_CMD_QRY, RT_NULL, 0,
            seq[dev_id]))
        {
            seq[dev_id] = seq[dev_id] ? 0 : 1;
            break;
        }
        THREAD_MSLEEP(100);
    }

    return (i == 2) ? RT_FALSE : RT_TRUE;
}

/*
** 通过modbus总线通信查询传感基站配置消息
*/
static void _mbus_query_sensor_config(uint_16 dev_id)
{
    int i = 0;
    for (; i < 2; i++)
    {
        if (_mbus_individual_comm(dev_id, MBUS_CMD_QUERY_SENSOR_CONFIG, RT_NULL, 0, 0))
        {
            break;
        }
    }
}

/*
** 通过modbus总线通信下发修改传感基站配置消息
*/
static void _mbus_set_sensor_config(uint_16 dev_id, const char* p_config,
                                            uint_16 config_len)
{
    mbus_tlv_t* p_tlv = RT_NULL;
    int buf_size = sizeof(mbus_tlv_t) + config_len;
    void* buf = malloc(buf_size + 1);
    if (RT_NULL == buf) {
        ERROR_LOG("malloc error\n");
        return;
    }

    memset(buf, 0, buf_size + 1);
    p_tlv = (mbus_tlv_t*)buf;
    p_tlv->type = MBUS_TLV_SENSOR_READER_V3_CONFIG;
    p_tlv->len = config_len;
    memcpy((void*)(p_tlv + 1), p_config, config_len);
    _mbus_individual_comm(dev_id, MBUS_CMD_SET_SENSOR_CONFIG, buf, buf_size, 0);

    THREAD_MSLEEP(50);
    free(buf);
}

/*
** 通过modbus总线通信下发撤离和取消撤离命令，适用于支持一代读卡器
*/
static void _mbus_down_cmd(int cmd)
{
    uint_16 mbus_id = MBUS_IDMIN;

    if ((cmd != MBUS_CMD_RETREAT) &&
        (cmd != MBUS_CMD_CANCEL_RETREAT))
    {
        return;
    }

    for (; mbus_id <= MBUS_IDMAX; mbus_id++){
        uint_8 i = 0;

        if (MBUS_NODE_IDLE == mbus_node_state[mbus_id])
        {
            continue;
        }

        for (; i < 2; i++)
        {
            if (_mbus_individual_comm(mbus_id, cmd, RT_NULL, 0, 0))
            {
                break;
            }
            THREAD_MSLEEP(100);
        }
    }
}

/*
** 通过modbus总线通信对一代读卡器下发撤离和取消撤离命令
*/
static void _mbus_down_card_reader_retreat(unsigned char* retreat_cmd)
{
    uint_16 mbus_id = MBUS_IDMIN;
    int cmd = 0;

    for (; mbus_id <= MBUS_IDMAX; mbus_id++)
    {
        uint_8 i = 0;

        if (mbus_node_state[mbus_id] != MBUS_NODE_CARD_READER)
        {
            continue;
        }

        if (CARD_READER_NO_CMD == retreat_cmd[mbus_id])
        {
            continue;
        }
        else if (CARD_READER_RETREAT_CMD == retreat_cmd[mbus_id])
        {
            cmd = MBUS_CMD_RETREAT;
            DEBUG_LOG("mbus down card reader[%d] retreat cmd\n", mbus_id);
        }
        else if (CARD_READER_CANCEL_RETREAT_CMD == retreat_cmd[mbus_id])
        {
            cmd = MBUS_CMD_CANCEL_RETREAT;
            DEBUG_LOG("mbus down card reader[%d] cancel retreat cmd\n", mbus_id);
        }
        else
        {
            continue;
        }

        for (; i < 2; i++)
        {
            if (_mbus_individual_comm(mbus_id, cmd, RT_NULL, 0, 0))
            {
                break;
            }
            THREAD_MSLEEP(100);
        }
    }
}

/**
* @brief 查询所有从机的版本信息
*/

static void _mbus_query_slv_version()
{
    uint_16 dev_id = MBUS_IDMIN;

    for (; dev_id <= MBUS_IDMAX; dev_id++)
    {
        uint_8 i = 0;

        if (MBUS_NODE_IDLE == mbus_node_state[dev_id])
        {
            continue;;
        }

        for (; i < 2; i++)
        {
            if (_mbus_individual_comm(dev_id,
                    MBUS_CMD_VERSION_QRY, RT_NULL, 0, 0))
            {
                break;
            }
            THREAD_MSLEEP(100);
        }
        DEBUG_LOG("mbus query slv %d version %s\n", dev_id,
            (2 == i) ? "failed" : "success");
    }
}

static void _mbus_handle_3g_data(char* data, int size)
{
    NWK_HDR_T *nwk_hdr = (NWK_HDR_T*)data;
    APP_HDR_T *app_hdr = (APP_HDR_T*)(nwk_hdr + 1);
    unsigned char* retreat_state = (unsigned char*)(app_hdr + 1);
    app_armTofAlarmAck_t* p_alarm_ack = (app_armTofAlarmAck_t*)(nwk_hdr + 1);

    RT_ASSERT(data != RT_NULL);

    if (!size) {
        ERROR_LOG("3g data size is zero\n");
        return;
    }

    if (app_hdr->protocoltype == APP_PROTOCOL_TYPE_CARD)
    {
        switch (app_hdr->msgtype)
        {
        case APP_TOF_MSG_RETREAT:
            DEBUG_LOG("mbus down station retreat cmd\n");
            _mbus_down_cmd(MBUS_CMD_RETREAT);
            break;
        case APP_TOF_MSG_CANCEL_RETREAT:
            DEBUG_LOG("mbus down station cancel retreat cmd\n");
            _mbus_down_cmd(MBUS_CMD_CANCEL_RETREAT);
            break;
        case APP_TOF_MSG_LINK:
            if (PAN_ADDR_ALL == nwk_hdr->dst)
            {
                _mbus_report_net_node_link();
            }
            break;
        case ARM_TOF_ALARM_ACK:
            DEBUG_LOG("mbus down card alarm ack pan %d reader %d "
                "card %d ack type %d\n",
                p_alarm_ack->panid, p_alarm_ack->jn_id,
                p_alarm_ack->short_addr, p_alarm_ack->ack_type);
            if ((p_alarm_ack->panid == GET_STATION_PAN_ID()) &&
                (p_alarm_ack->ack_type == APP_TOF_ALARM_CARD_HELP_ACK))
            {
                uint_16 mbus_id = p_alarm_ack->jn_id - MBUS_RDR_BASE;
                _mbus_send_card_alarm_ack(mbus_id, p_alarm_ack->short_addr);
            }
            break;
        case ARM_TRANS_CARDREADER_CMD:
            DEBUG_LOG("mbus down card reader retreat cmd\n");
            _mbus_down_card_reader_retreat(retreat_state);
            break;
        default :
            break;
        }
    }
}

/**
* @brief 处理工控机下发的版本查询信息
* 处理分两步: 上报主机的版本信息，查询所有从机的版本信息
*/

static void _mbus_handle_version_query(char* data, int size)
{
    version_req_t* ver_hdr = (version_req_t*)data;

    RT_ASSERT(ver_hdr != RT_NULL);

    if (sizeof(version_req_t) != size) {
        ERROR_LOG("error data len %d, version_req_t should be %d\n",
            size, sizeof(version_req_t));
        return;
    }

    version_req_seq = ver_hdr->seq;
    _mbus_report_master_version();
    _mbus_query_slv_version();
}

/*
** 处理ip网络转发的消息
*/
static void _mbus_handle_net_msg(void)
{
    PACKET_HEADER_T *pkt_hdr = (PACKET_HEADER_T *)mbus_net_msg_buf;
    comm_sensor_config_t* sensor_config = (comm_sensor_config_t*)(pkt_hdr + 1);
    char* data = (char*)(pkt_hdr + 1);

    memset(mbus_net_msg_buf, 0, MBUS_NET_MSG_BUF_SIZE);
    if (rt_mq_recv(&mbus_rec_mq, mbus_net_msg_buf, MSG_MBUS_PKT_SIZE,
        RT_WAITING_NO) != RT_EOK)
    {
        return;
    }

    switch (pkt_hdr->tag) {
    case COMM_3G_DATA:
        _mbus_handle_3g_data(data, pkt_hdr->len);
        break;
    case COMM_SENSOR_QUERY_LINK:
        _mbus_report_sensor_link();
        break;
    case COMM_SENSOR_QUERY_CONFIG:
        if (sensor_config->src_pan == GET_STATION_PAN_ID())
        {
            uint_16 id = sensor_config->dev_id - MBUS_SENSOR_RDR_BASE;
            _mbus_query_sensor_config(id);
        }
        break;
    case COMM_SENSOR_DOWN_CONFIG:
        if (sensor_config->src_pan == GET_STATION_PAN_ID())
        {
            uint_16 id = sensor_config->dev_id - MBUS_SENSOR_RDR_BASE;
            _mbus_set_sensor_config(id, (char*)(sensor_config + 1),
                                    sensor_config->datalen);
        }
        break;
    case COMM_VERSION_REQ:
        DEBUG_LOG("rec workstation query mbus device version\n");
        _mbus_handle_version_query(data, pkt_hdr->len);
    default:
        break;
    }
}

/*
** modbus 扫描总线上的所有从机，维护从机连接状态，查询从机数据
*/
static void _mbus_scan_slv_device()
{
    static uint_16 idle_id = MBUS_IDMIN;
    uint_16 id = 0;
    static uint_8 scan_cnt = 0;
    uint_16 active_cnt = 0;    //< 记录活跃节点个数
    uint_16 search_cnt = 0;

    scan_cnt++;

    for (id = MBUS_IDMIN; id <= MBUS_IDMAX; id++)
    {
        if (MBUS_NODE_IDLE == mbus_node_state[id]) {
            continue;
        }
        else if (MBUS_NODE_SENSOR_STATION == mbus_node_state[id])
        {
            /// 传感分站不需要实时更新数据
            if (scan_cnt % 5)
            {
                continue;
            }
        }

        active_cnt++;
        if (!_mbus_query_slv_data(id))
        {
            ERROR_LOG("mbus query slv %d data error\n", id);

            /// 处理modbus节点链路断开问题
            mbus_node_data_query_err_cnt[id] += 2;
            if (mbus_node_data_query_err_cnt[id] >
                MBUS_NODE_DATA_QUERY_MAX_CNT)
            {
                mbus_node_state[id] = MBUS_NODE_IDLE;
                mbus_node_data_query_err_cnt[id] = 0;
            }
        }
        else
        {
            mbus_node_data_query_err_cnt[id] = 0;
        }
    }

    search_cnt = (active_cnt < 8) ? (8 - active_cnt) : 1;

    for (id = 0; id < search_cnt; id++)
    {
        if (MBUS_NODE_IDLE == mbus_node_state[idle_id])
        {
            mbus_node_state[idle_id] = _mbus_query_slv_state(idle_id);
        }

        if (++idle_id > MBUS_IDMAX)
        {
            idle_id = MBUS_IDMIN;
        }
    }
}

static void _mbus_print_slv_id_state()
{
    uint_16 id = 0;
    uint_8 link_state = 0;

    LOG(LOG_INFO, "mbus slv dev[id]:\n");
    for (id = MBUS_IDMIN; id <= MBUS_IDMAX; id++)
    {
        switch (mbus_node_state[id])
        {
        case MBUS_NODE_IDLE:
            break;
        case MBUS_NODE_NON_IDLE:
            LOG(LOG_INFO, "unknown[%d]\n", id);
            break;
        case MBUS_NODE_CARD_READER:
            LOG(LOG_INFO, "card reader[%d]\n",id);
            break;
        case MBUS_NODE_SENSOR_STATION:
            LOG(LOG_INFO, "sensor station[%d]\n", id);
            break;
		case MBUS_NODE_POWER_SENSOR:
            LOG(LOG_INFO, "power sensor [%d]\n", id);
            break;
			 
        default:
            break;
        }

        if (mbus_node_state[id] != MBUS_NODE_IDLE)
        {
            link_state = 1;
        }
    }

    /* 如果有一个从机就将指示灯点亮 */
    if (link_state)
    {
        light_up_modbus_data_indicator();
    }
    else
    {
        light_off_modbus_data_indicator();
    }
}

/*
**用于串口中断时做的回调，通知应用程序有收到数据的标志
*/
static rt_err_t _mbus_rx_ind(rt_device_t dev, rt_size_t size)
{
    /* release semaphore to let bootsh thread rx data */
    mbus.rx_ind = RT_TRUE;

    return RT_EOK;
}

/**
 *
 * This function sets the input device of modbus.
 *
 * @param device_name the name of new input device.
 */
void _mbus_set_device(const char* device_name)
{
    rt_device_t dev = RT_NULL;

    RT_ASSERT(device_name != RT_NULL);
    dev = rt_device_find(device_name);

    if (dev != RT_NULL && rt_device_open(dev, RT_DEVICE_OFLAG_RDWR) == RT_EOK)
    {
        mbus.device = dev;
        rt_device_set_rx_indicate(dev, _mbus_rx_ind);
    }
    else
    {
        if (mbus.device != RT_NULL)
        {
            /* close old finsh device */
            rt_device_close(mbus.device);
        }
        ERROR_LOG("mbus can not find device: %s\n", device_name);
    }
    mbus.rx_ind = RT_FALSE;
}

/*
** modbus 线程相关任务逻辑初始化
*/
static void _mbus_init(void)
{
    const char mbus_dev[] = "uart1";
    _mbus_set_device(mbus_dev);
    MBUS_ENABLE_SEND_MSG(); ///< 获取modbus发送总线后不再释放
}

static void _mbus_slv_data_init(void)
{
    uint_16 id = MBUS_IDMIN;

    for (; id <= MBUS_IDMAX; id++)
    {
        if (MBUS_NODE_IDLE == mbus_node_state[id])
        {
            mbus_node_state[id] = _mbus_query_slv_state(id);
        }
    }

    for (id = MBUS_IDMIN; id <= MBUS_IDMAX; id++)
    {
        switch (mbus_node_state[id])
        {
        case MBUS_NODE_IDLE:
            break;
        case MBUS_NODE_NON_IDLE:
        case MBUS_NODE_CARD_READER:
        case MBUS_NODE_SENSOR_STATION:
            _mbus_query_slv_data(id);
            break;
        default:
            break;
        }
    }
}

void init_mbus_msg_queue()
{
    rt_mq_init(&mbus_rec_mq, "mbus rec mq", u8MbusPool,
        MBUS_NET_MSG_BUF_SIZE, sizeof(u8MbusPool), RT_IPC_FLAG_FIFO);
    rt_mq_init(&mbus_reported_mq, "mbus reported mq", mbus_reported_msg_pool,
        MBUS_NET_MSG_BUF_SIZE, sizeof(mbus_reported_msg_pool), RT_IPC_FLAG_FIFO);
}

/*
** modbus线程入口
*/
void mbus_thread_entry(void * param)
{
    uint_32 loop_cnt = 0;

    _mbus_init();
    _mbus_slv_data_init();

    _mbus_report_master_version();
    _mbus_query_slv_version();

    /// modbus 总线上读卡器需要1s读一次
    while (1)
    {
        _mbus_handle_net_msg();
        _mbus_scan_slv_device();

        loop_cnt++;
        if (!(loop_cnt % LINK_REPORT_INTERVAL_SEC))
        {
            _mbus_syn_slave_time();
            _mbus_print_slv_id_state();
            _mbus_report_net_node_link();
        }

        if (!(loop_cnt % VERSION_REPORT_INTERVAL_SEC))
        {
            _mbus_report_master_version();
            _mbus_query_slv_version();
        }

        THREAD_MSLEEP(100);
    }
}

static rt_thread_t mbus_thread = RT_NULL;

rt_bool_t start_modbus_work()
{
    mbus_thread = rt_thread_create("mbus", mbus_thread_entry,
                                    RT_NULL, 2048, 9, 10);

    if (mbus_thread == RT_NULL)
    {
        ERROR_LOG("create modbus work thread failed\n");
        return RT_FALSE;
    }

    rt_thread_startup(mbus_thread);

    return RT_TRUE;
}

void stop_modbus_work()
{
    rt_enter_critical();

    if (mbus_thread != RT_NULL && mbus_thread->stat != RT_THREAD_CLOSE)
    {
        rt_thread_delete(mbus_thread);
    }

    rt_exit_critical();

    TIME_LOG(LOG_CRITICAL, "stop modbus thread\n");
}

