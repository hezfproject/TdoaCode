#include <stdio.h>
#include <time.h>
#include <rtthread.h>
#include <lwip/sys.h>
#include <lwip/api.h>
#include "lwip/netifapi.h"
#include <lwip/netdb.h>
#include <lwip/sockets.h>
#include <netif/ethernetif.h>
#include <stm32f2xx_eth.h>
#include <board.h>

#include "msg_center.h"
#include "bootcfg.h"
#include "net_app.h"
#include "bsmac_parser.h"
//#include "spi_flash_app.h"
#include "nwk_protocol.h"

#include "3g_protocol.h"
#include "mbus_app.h"
#include "location_data.h"
#include "3g_watchdog.h"
#include "3g_thread.h"
#include "upgrade_flag.h"

//#define LOG_DEBUG
#include "3g_log.h"

#include "../../../../../version.h"

#define NET_RECV_THREAD_TICK        (10)
#define NET_RECV_THREAD_SLEEP_MS    (20)
#define NET_SEND_THREAD_TICK        (60)
#define NET_SEND_THREAD_SLEEP_MS    (10)
#define NET_SERVER_THREAD_TICK      (5)
#define NET_SERVER_THREAD_SLEEP_MS  (20)

rt_uint8_t u8NetPool[NET_POOL_MAX];
struct rt_messagequeue net_mq;

/* 网络线程工作标识, 线程正常运行时置1，
 * 使用喂看门狗狗定时器定时检查工作标识，工作标识为1时喂狗并清零，为0时不喂狗重启
 * 多线程正常工作标识关系为逻辑与
 */
unsigned char net_recv_fine_flag = 0;   // 网络接收线程工作正常标识
unsigned char net_send_fine_flag = 0;   // 网络发送线程工作正常标识

static unsigned char send_buf[MSG_ANALYSER_PKT_SIZE];
static unsigned char recv_buf[MSG_ANALYSER_PKT_SIZE];


static int net_sock = -1;
static int net_IsOk = RT_ERROR;

/* tcp socket 互斥量，
 * 用于LWIP协议栈中tcp socket 接收和发送的互斥操作
 */
static rt_mutex_t sock_mutex = RT_NULL;

static int _create_socket_mutex()
{
    sock_mutex = rt_mutex_create("tcp socket mutex", RT_IPC_FLAG_FIFO);
    if (sock_mutex == RT_NULL) {
        ERROR_LOG("create sock mutex failed\n");
        return -1;
    }

    return 0;
}

#if 0
static int _delete_socket_mutex()
{
    if (sock_mutex != RT_NULL) {
        if (rt_mutex_delete(sock_mutex) != RT_EOK) {
            ERROR_LOG("delete sock mutex failed\n");
            return -1;
        }
    }

    return 0;
}
#endif

static int _take_socket_mutex(int time)
{
    rt_err_t ret = 0;

    if (sock_mutex == RT_NULL) {
        return -1;
    }

    ret = rt_mutex_take(sock_mutex, time);
    if (ret != RT_EOK) {
        ERROR_LOG("take sock mutex failed, ret code %d\n", ret);
        return -1;
    }

    return 0;
}

static int _release_socket_mutex()
{
    rt_err_t ret = 0;

    if (sock_mutex == RT_NULL) {
        return -1;
    }

    ret = rt_mutex_release(sock_mutex);
    if (ret != RT_EOK) {
        ERROR_LOG("release sock mutex failed, ret code %d\n", ret);
        return -1;
    }

    return 0;
}

/*
 * 名称: net_report_restart_msg
 * 功能: 上报3G基站重启信息, 3G基站重启后必须在得到射频板ID后才能调用
 */
void net_report_restart_msg()
{
    char buf[sizeof(PACKET_HEADER_T) + sizeof(pan_restart_t)] = {0};

    PACKET_HEADER_T* hdr = (PACKET_HEADER_T*)buf;
    pan_restart_t* data = (pan_restart_t*)(hdr + 1);

    hdr->tag = COMM_PAN_RESTART;
    hdr->len = sizeof(pan_restart_t);

    data->pan = GET_STATION_PAN_ID();
    data->mode = 0;
    snprintf(data->msg, sizeof(data->msg),
        "网络型读卡主站应用程序启动(APP version: %s release: %s)",
        VERSION, RELEASE);

    rt_mq_send(&net_mq, buf, sizeof(PACKET_HEADER_T) + hdr->len);
}

/*
 * 名称: net_report_running_state_msg
 * 功能: 上报3G基站运行状态信息
 */
void net_report_running_state_msg(int state_mode, const char* p_msg, int msg_len)
{
    char buf[sizeof(PACKET_HEADER_T) + sizeof(pan_running_state_t)] = {0};

    PACKET_HEADER_T* hdr = (PACKET_HEADER_T*)buf;
    pan_running_state_t* p_data = (pan_running_state_t*)(hdr + 1);

    if (!p_msg || !msg_len) {
        return;
    }

    hdr->tag = COMM_PAN_RUNNING_STATE;
    hdr->len = sizeof(pan_running_state_t);

    p_data->pan = GET_STATION_PAN_ID();
    p_data->mode = state_mode;
    snprintf(p_data->msg, sizeof(p_data->msg),p_msg);

    rt_mq_send(&net_mq, buf, sizeof(PACKET_HEADER_T) + hdr->len);
}

void app_display_ipaddr(rt_uint32_t ip_addr)
{
    rt_uint8_t num[4] = {0};

    num[0] = (uint8_t)(ip_addr >> 24);
    num[1] = (uint8_t)(ip_addr >> 16);
    num[2] = (uint8_t)(ip_addr >> 8);
    num[3] = (uint8_t)(ip_addr);
    rt_kprintf("%d.%d.%d.%d\n", num[3], num[2], num[1], num[0]);
}

static void _dhcp_work(void)
{
    rt_uint32_t IPaddress = 0;
    rt_int32_t cnt = 0;

    while (1)
    {
        feed_watchdog();
        THREAD_MSLEEP(200);

        if (netif_default != RT_NULL)
        {
            if (IPaddress != netif_default->ip_addr.addr)
            {
                /* read the new IP address */
                IPaddress = netif_default->ip_addr.addr;
                break;
            }
            else if (!IPaddress)
            {
                rt_int32_t i = 0;

                rt_kprintf("tries = %d\n", netif_default->dhcp->tries);
                cnt++;
                rt_kprintf("dhcp wait");
                while (i++ < cnt)
                    rt_kprintf(".");
                rt_kprintf("\r");
                if (cnt == 4)
                {
                    rt_kprintf("dhcp wait    \r");
                    cnt = 0;
                }
            }
        }
        else
            rt_kprintf("netif not default configuration\n");
    }
}

static void _config_ipaddr(void)
{
    feed_watchdog();

    if (IPMODE_DHCP == sys_option.u32IpMode) {
        _dhcp_work();
    } else {
        struct ip_addr ipaddr;
        struct ip_addr netmask;
        struct ip_addr gw;

        /* 休眠一会，否则dhcp_stop函数调用会出错 */
        THREAD_MSLEEP(20);
        feed_watchdog();

        dhcp_stop(netif_default);
        netif_set_up(netif_default);

        ipaddr.addr = sys_option.u32BsIp;
        netmask.addr = sys_option.u32BsMK;
        gw.addr = sys_option.u32BsGW;
        netifapi_netif_set_addr(netif_default, &ipaddr , &netmask, &gw);
    }

    rt_kprintf("station use %s ip\n",
        (IPMODE_DHCP == sys_option.u32IpMode) ? "dhcp" : "static");

    rt_kprintf("ip addr: ");
    app_display_ipaddr(netif_default->ip_addr.addr);
    rt_kprintf("netmask addr: ");
    app_display_ipaddr(netif_default->netmask.addr);
    rt_kprintf("gw addr: ");
    app_display_ipaddr(netif_default->gw.addr);
}

/*
 * TCP 出现问题，需要在关闭socket之后下一个socket建立之间间隔一段时间
 * 创建一个socket，类型是SOCKET_STREAM，TCP类型
 */
static int _create_socket(void)
{
    net_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (net_sock < 0) {
        rt_err_t err = rt_get_errno();
        ERROR_LOG("creat tcp socket err code %d\n", err);
        return -1;
    }

    return net_sock;
}

static void _close_socket()
{
    closesocket(net_sock);
    net_sock = -1;
    net_IsOk = RT_ERROR;
    TIME_LOG(LOG_INFO, "net socket closed\n");
}

static int _connect_server(void)
{
    int ret = 0;
    struct sockaddr_in server_addr;

    /* 初始化预连接的服务端地址 */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(sys_option.u32SvPort);
    server_addr.sin_addr.s_addr = sys_option.u32SvIp;
    rt_memset(&(server_addr.sin_zero), 0, sizeof(server_addr.sin_zero));

    /* connect 等待延时有13s */
    ret = connect(net_sock, (struct sockaddr *)&server_addr,
                    sizeof(struct sockaddr));
    if (ret < 0) {
        closesocket(net_sock);
        net_sock = -1;
        net_IsOk = RT_ERROR;
        LOG(LOG_WARNING, "%u connect server failed\n", time(RT_NULL));
        return -1;
    }

    net_IsOk = RT_EOK;
    TIME_LOG(LOG_INFO, "connect to %s:%d OK\n",
        inet_ntoa(sys_option.u32SvIp), sys_option.u32SvPort);
    return 0;
}

#define DEBUG_NET
static char net_err_buf[128];
static char net_restart_flag;

static int _net_recv_data(unsigned char* data_buf, int data_size)
{
    int ret = 0;
    int rec_size = 0;
    rt_err_t err = 0;

    RT_ASSERT(data_buf);

    if (!data_size) {
        return 0;
    }

    _take_socket_mutex(RT_WAITING_FOREVER);
    while (rec_size < data_size) {
        net_recv_fine_flag = 1;
        ret = recv(net_sock, data_buf + rec_size, data_size - rec_size,
                    MSG_DONTWAIT);
        err = rt_get_errno();
        if (ret < 0) {
            if (err == EWOULDBLOCK) {
                /* 休眠一段时间继续读取 */
                _release_socket_mutex();
                THREAD_MSLEEP(100);
                _take_socket_mutex(RT_WAITING_FOREVER);
                continue;
            } else {
                /* 断开连接 */
                _close_socket();
                _release_socket_mutex();
#ifdef DEBUG_NET
                memset(net_err_buf, 0, sizeof(net_err_buf));
                snprintf(net_err_buf, sizeof(net_err_buf),
                "net rec data err ret %d len %d", err, data_size);
#endif
                return -1;
            }
        } else if (ret == 0) {
            /* 断开连接 */
            _close_socket();
            _release_socket_mutex();
#ifdef DEBUG_NET
            memset(net_err_buf, 0, sizeof(net_err_buf));
            snprintf(net_err_buf, sizeof(net_err_buf), "net socket is closed");
#endif
            return -1;
        } else {
            rec_size += ret;
            continue;
        }
    }
    _release_socket_mutex();

    return data_size;
}

static void _net_sendto_analyser(void *buf, rt_uint32_t len)
{
    RT_ASSERT(buf != RT_NULL);

    if (len <= MSG_ANALYSER_PKT_SIZE)
    {
        rt_mq_send(&msg_analyser_mq, buf, len);
    }
    else
    {
        ERROR_LOG("len %d is too big\n", len);
    }
}

#ifdef SUPPORT_BIT_ERROR_RATE_TEST
static void _net_bit_error_rate_test_handler(void* buf, int len)
{
    static char BERT_flag = 0;

    RT_ASSERT(buf != RT_NULL);

    if (len <= 0)
    {
        return;
    }

    if (!BERT_flag)
    {
        BERT_flag = 1;
        /* 停止modbus和bsmac线程工作 */
        stop_bsmac_work();
        stop_modbus_work();
    }

    rt_mq_send(&mbus_reported_mq, buf, len);
}
#else
static void _net_bit_error_rate_test_handler(void* buf, int len)
{
}
#endif

/* 网络连接的创建和关闭都放在接收线程中 */
static int _net_recv_work_loop()
{
    MSG_CENTER_HEADER_T *p_msg_hdr = RT_NULL;
    unsigned char* buf_wpos = RT_NULL;
    PACKET_HEADER_T pkt_hdr = {0, 0};
    int ret = 0;

    /* 创建网络连接 */
    _take_socket_mutex(RT_WAITING_FOREVER);
    if (net_sock < 0) {
        while (1) {
            net_recv_fine_flag = 1;
            if (_create_socket() >= 0) {
                if (!_connect_server()) {
#ifdef DEBUG_NET
                    if (!net_restart_flag) {
                        net_report_running_state_msg(ARM_NORMAL,
                            "启动网络接收线程", 128);
                        net_restart_flag = 1;
                    }
                    else {
                        net_report_running_state_msg(ARM_ERROR, net_err_buf,
                            sizeof(net_err_buf));
                    }
#endif
                    break;
                }
            }
            _release_socket_mutex();
            THREAD_MSLEEP(1000);
            _take_socket_mutex(RT_WAITING_FOREVER);
        }
    }
    _release_socket_mutex();
    
    /* 读网络包header数据 */
    ret = _net_recv_data((unsigned char*)&pkt_hdr, sizeof(pkt_hdr));
    if (ret != sizeof(pkt_hdr)) {
#if 0
#ifdef DEBUG_NET
        memset(net_err_buf, 0, sizeof(net_err_buf));
        snprintf(net_err_buf, sizeof(net_err_buf),
            "net rec pkt hdr error, ret %d", ret);
#endif
#endif
        ERROR_LOG("net rec pkt hdr error, ret %d\n", ret);
        return -1;
    }

    /* 利用接收header中的len检查数据有效性 */
    if ((pkt_hdr.len < 0) ||
        (sizeof(pkt_hdr) + pkt_hdr.len) > MSG_ANALYSER_PKT_SIZE) {
#ifdef DEBUG_NET
        memset(net_err_buf, 0, sizeof(net_err_buf));
        snprintf(net_err_buf, sizeof(net_err_buf),
        "net rec pkt hdr length error, tag %d len %d", pkt_hdr.tag, pkt_hdr.len);
#endif
        ERROR_LOG("net rec pkt hdr length error, tag %d len %d\n",
            pkt_hdr.tag, pkt_hdr.len);
        return -1;
    }

    memset(recv_buf, 0, sizeof(recv_buf));

    /* 进行误码率测试时不再将数据放到消息分析中心 */
    if (pkt_hdr.tag != COMM_ERROR_RATE_TEST)
    {
        p_msg_hdr = (MSG_CENTER_HEADER_T *)recv_buf;
        p_msg_hdr->u32MsgId = ETH0_TRANSMIT_ID;
        buf_wpos = recv_buf + sizeof(MSG_CENTER_HEADER_T);
    }
    else
    {
        buf_wpos = recv_buf;
    }

    memcpy(buf_wpos, &pkt_hdr, sizeof(pkt_hdr));
    buf_wpos += sizeof(pkt_hdr);

    /* 读网络包数据 */
    ret = _net_recv_data(buf_wpos, pkt_hdr.len);
    if (ret != pkt_hdr.len) {
#if 0
#ifdef DEBUG_NET
        memset(net_err_buf, 0, sizeof(net_err_buf));
        snprintf(net_err_buf, sizeof(net_err_buf),
        "net rec pkt data error, tag %d len %d", pkt_hdr.tag, pkt_hdr.len);
#endif
#endif
        ERROR_LOG("net rec pkt data error\n");
        return -1;
    }

    DEBUG_LOG("%u net rec data tag %d len %d\n",
        time(RT_NULL), pkt_hdr.tag, pkt_hdr.len);

    /* 如果接收到的数据是3G数据或者是MODBUS数据则放在消息处理中心 */
    if ((COMM_3G_DATA == pkt_hdr.tag) ||
	    (COMM_SENSOR_QUERY_LINK == pkt_hdr.tag) ||
        (COMM_SENSOR_QUERY_CONFIG == pkt_hdr.tag) ||
        (COMM_SENSOR_DOWN_CONFIG == pkt_hdr.tag) ||
        (COMM_VERSION_REQ == pkt_hdr.tag))
    {
		_net_sendto_analyser(recv_buf,
            MSG_HEADER_SIZE + sizeof(pkt_hdr) + pkt_hdr.len);
    }
    else if (COMM_ERROR_RATE_TEST == pkt_hdr.tag)
    {
        _net_bit_error_rate_test_handler(recv_buf,
            sizeof(pkt_hdr) + pkt_hdr.len);
    }

    return 0;
}

void _net_recv_work_thread(void *para)
{
     while (1) {
        _net_recv_work_loop();
        net_recv_fine_flag = 1;
        THREAD_MSLEEP(NET_RECV_THREAD_SLEEP_MS);
    }
}

static int _net_send_data(unsigned char* data_buf, int data_size)
{
    PACKET_HEADER_T* pkt_hdr = (PACKET_HEADER_T*)data_buf;
    int ret = 0;

    RT_ASSERT(data_buf != RT_NULL);

    if (!data_size) {
        return 0;
    }

    if (pkt_hdr->len > MSG_NET_PKT_SIZE) {
        ERROR_LOG("net send pkt tag %d len %d\n", pkt_hdr->tag, pkt_hdr->len);
        return -1;
    }

    if (net_IsOk != RT_EOK) {
        ERROR_LOG("net_IsOk false\n");
        return -1;
    }

    _take_socket_mutex(RT_WAITING_FOREVER);
    ret = send(net_sock, data_buf, data_size, MSG_DONTWAIT);

    _release_socket_mutex();

    return ret;
}

/*
** 判断并处理modbus总线上报的网络数据
*/
static rt_bool_t _net_send_mbus_data(void)
{
    PACKET_HEADER_T* pkt_hdr = (PACKET_HEADER_T*)send_buf;
    int data_size = 0;

    if ((COMM_CARD_LINK_RPT == pkt_hdr->tag) ||
        (COMM_SENSOR_LINK_RPT == pkt_hdr->tag) ||
        (COMM_CARD_RPT == pkt_hdr->tag) ||
        (COMM_NEW_SENSOR_RPT == pkt_hdr->tag) ||
        (COMM_SENSOR_QUERY_CONFIG == pkt_hdr->tag) ||
        (COMM_ERROR_RATE_TEST == pkt_hdr->tag) ||
        (COMM_VERSION_RSP == pkt_hdr->tag) ||
        (COMM_POWER_SENSOR_RPT == pkt_hdr->tag))
    {
        data_size = sizeof(PACKET_HEADER_T) + pkt_hdr->len;
        if (_net_send_data(send_buf, data_size) == data_size)
        {
			//rt_kprintf("COMM_POWER_SENSOR_RPT\r\n");
            return RT_TRUE;
        }
    }

    return RT_FALSE;
}
unsigned	int lenght = 0;

static void _net_send_work_loop()
{

    int ret = 0;
    int data_len = 0;
    PACKET_HEADER_T* pkt_hdr =(PACKET_HEADER_T*)send_buf;
    rt_err_t err = 0;
    volatile static unsigned char send_err_flag; // 发送出错标识
    APP_UWB_TDOA_TIMESTAMP_S *pstTdoaMsg;
    APP_HDR_T * pstAppHead;
    /* 发送modbus网络上报工控机的数据 */
    memset(send_buf, 0, sizeof(send_buf));
    err = rt_mq_recv(&mbus_reported_mq, send_buf, MBUS_NET_MSG_BUF_SIZE,
        RT_WAITING_NO);
    if (err == RT_EOK)
    {
        _net_send_mbus_data();
    }
    
    /* 发送jennic定位数据 */
    memset(send_buf, 0, MSG_ANALYSER_PKT_SIZE);
    err = rt_mq_recv(&net_mq, send_buf, MSG_ANALYSER_PKT_SIZE, 10);
    if (err == RT_EOK) {
        data_len = sizeof(PACKET_HEADER_T) + pkt_hdr->len;
        ret = _net_send_data(send_buf, data_len);
        pstTdoaMsg = (APP_UWB_TDOA_TIMESTAMP_S *)(send_buf + sizeof(PACKET_HEADER_T) + sizeof(NWK_HDR_T) + sizeof(app_SMT32_Data) + sizeof(APP_HDR_T));
        pstAppHead = (APP_HDR_T *)(send_buf + sizeof(PACKET_HEADER_T) + sizeof(NWK_HDR_T) + sizeof(app_SMT32_Data));

        ERROR_LOG("Len %u pT %u mT %u tag %u %u %u\n", data_len, 
            pstAppHead->protocoltype, 
            pstAppHead->msgtype,
            pstTdoaMsg->u16StationID,
            pstTdoaMsg->u16TestCardID,
            pstTdoaMsg->u16Cardseqnum);
        /* 发送失败保存定位数据并返回，不再读取flash中保存的定位数据 */
        if (ret != data_len) {
            send_err_flag = 1;
            if (COMM_3G_DATA == pkt_hdr->tag)
						{
                dump_location_data(send_buf, data_len);
            }
            return;
        } else {
            send_err_flag = 0;
        }
    }

    if ((net_IsOk == RT_EOK) && (!send_err_flag)){
        memset(send_buf, 0, MSG_ANALYSER_PKT_SIZE);
        ret = load_location_data(send_buf, MSG_ANALYSER_PKT_SIZE);
			//rt_kprintf("read  ret=0x%d\r\n",ret);
        if (ret > 0) {
            data_len = sizeof(PACKET_HEADER_T) + pkt_hdr->len;
            if (pkt_hdr->tag == COMM_3G_DATA){
                int len = _net_send_data(send_buf, data_len);
							//rt_kprintf("read and send len=0x%x\r\n",len);
							lenght+=len;
							//rt_kprintf("lenght=0x%x\r\n",lenght);

                if (len != data_len) {
                    ERROR_LOG("Net send saved data tag %d len %d failed "
                        "ret %d\n", pkt_hdr->tag, pkt_hdr->len, len);
                }
            }
        }
    }
}

static unsigned char report_msg_buf[sizeof(PACKET_HEADER_T) + sizeof(pan_restart_t)] = {0};
static int_32 recent_report_time = 0;
static int_32 restart_time = 0;
static uint_32 running_hours = 0;

#define SECONDS_PER_HOUR    (60 * 60)

void net_send_normal_running_msg()
{
    PACKET_HEADER_T* hdr = (PACKET_HEADER_T*)report_msg_buf;
    pan_restart_t* data = (pan_restart_t*)(hdr + 1);
    uint_8 ip_num[4] = {0};
    int report_interval = 0;

    /* 为了等待与工控机网络连接，启动后等待30s之后再上报启动信息 */
    if (recent_report_time == 0) {
        if (!restart_time) {
            restart_time = time(RT_NULL);
        }
        report_interval = time(RT_NULL) - restart_time;
        if ((report_interval >= 0) &&
            (report_interval < 30)) {
            return;
        }
    }

    report_interval = time(RT_NULL) - recent_report_time;
    if ((recent_report_time != 0) &&
        (report_interval >= 0) &&
        (report_interval < SECONDS_PER_HOUR)) {
        return;
    }

    recent_report_time = time(RT_NULL);
    if (report_interval < SECONDS_PER_HOUR * 2) {
        running_hours++;
    }

    memset(report_msg_buf, 0, sizeof(report_msg_buf));
    hdr->tag = COMM_PAN_RESTART;
    hdr->len = sizeof(pan_restart_t);

    data->pan = GET_STATION_PAN_ID();
    data->mode = 0;

    ip_num[0] = (uint8_t)(netif_default->ip_addr.addr >> 24);
    ip_num[1] = (uint8_t)(netif_default->ip_addr.addr >> 16);
    ip_num[2] = (uint8_t)(netif_default->ip_addr.addr >> 8);
    ip_num[3] = (uint8_t)(netif_default->ip_addr.addr);

    snprintf(data->msg, sizeof(data->msg),
        "网络型读卡主站应用程序%s(app version(%s) release(%s) "
        "ip addr(%d.%d.%d.%d) mac addr(%02x:%02x:%02x:%02x:%02x:%02x) "
        "running hours(%d) )", (!running_hours) ? ("启动") : ("运行正常"),
        VERSION, RELEASE, ip_num[3], ip_num[2], ip_num[1], ip_num[0],
        netif_default->hwaddr[0], netif_default->hwaddr[1],
        netif_default->hwaddr[2], netif_default->hwaddr[3],
        netif_default->hwaddr[4], netif_default->hwaddr[5],
        running_hours);

    rt_mq_send(&net_mq, report_msg_buf,
        sizeof(PACKET_HEADER_T) + sizeof(pan_restart_t));

    DEBUG_LOG("net report running state msg\n");
}


void _net_send_work_thread(void *para)
{
    while (1) {
        _net_send_work_loop();
        net_send_normal_running_msg();
        net_send_fine_flag = 1;
        THREAD_MSLEEP(NET_SEND_THREAD_SLEEP_MS);
    }
}

void syn_time(time_t set_tm)
{
    extern rt_err_t set_date(rt_uint32_t year, rt_uint32_t month, rt_uint32_t day);
    extern rt_err_t set_time(rt_uint32_t hour, rt_uint32_t minute, rt_uint32_t second);

    time_t cur_tm = set_tm;
    struct tm st_tm;

    if (time(RT_NULL) == set_tm) {
        return;
    }

    localtime_r(&cur_tm, &st_tm);
    if (!((set_date(st_tm.tm_year + 1900, st_tm.tm_mon + 1, st_tm.tm_mday)
                == RT_EOK) &&
        (set_time(st_tm.tm_hour, st_tm.tm_min, st_tm.tm_sec) == RT_EOK)))
    {
        ERROR_LOG("set_time error\n");
    }
    else
    {
        TIME_LOG(LOG_INFO, "syn station time\n");
    }
}

static void _net_sendto_jennic(void *buf, rt_uint32_t len)
{
    RT_ASSERT(buf != RT_NULL);

    if (len <= MSG_COM_PKT_SIZE)
        rt_mq_send(&bsmac_mq, buf, len);
    else
    {
        ERROR_LOG("send bsmac_mq len(%d) is too big\n", len);
    }
}

static void _net_sendto_mbus(void *buf, rt_uint32_t len)
{
    RT_ASSERT(buf != RT_NULL);

    if (len <= MBUS_NET_MSG_BUF_SIZE)
    {
        rt_mq_send(&mbus_rec_mq, buf, len);
    }
    else
    {
        ERROR_LOG("send mbus_rec_mq len(%d) is too big\n", len);
    }
}

static void _net_analyser_callback(void *pvParam)
{
    const MSG_CENTER_HEADER_T *pstId = (MSG_CENTER_HEADER_T *)pvParam;
    PACKET_HEADER_T *pstNet_head = (PACKET_HEADER_T *)(pstId + 1);
    NWK_HDR_T *pstNwk_head = (NWK_HDR_T *)(pstNet_head + 1);
    APP_HDR_T *pstApp_head = (APP_HDR_T *)(pstNwk_head + 1);

    if (pstId != RT_NULL && ETH0_TRANSMIT_ID == pstId->u32MsgId)
    {
        if (COMM_3G_DATA == pstNet_head->tag)
        {


            if ((sys_option.u32BsId == pstNwk_head->dst) ||
                (PAN_ADDR_UNKNOWN == pstNwk_head->dst))
            {
                if ((APP_PROTOCOL_TYPE_STATION == pstApp_head->protocoltype) &&
                    (APP_SYNC_TIME == pstApp_head->msgtype))
                {
                    time_t s32Tm = *(time_t *)(pstApp_head + 1);
                    syn_time(s32Tm);
                }
            }
            else
            {
                _net_sendto_jennic(pstNwk_head, pstNet_head->len);
                if (APP_PROTOCOL_TYPE_CARD == pstApp_head->protocoltype)
                {
                    _net_sendto_mbus(pstNet_head,
                        sizeof(PACKET_HEADER_T) + pstNet_head->len);
                }
            }
        }
        else
        {
            _net_sendto_mbus(pstNet_head,
                sizeof(PACKET_HEADER_T) + pstNet_head->len);
        }
    }
}

static int server_sock = -1;
static int connect_sock = -1;
static char server_msg_buf[512];

static void _net_close_server()
{
    if (server_sock != -1)
    {
        closesocket(server_sock);
        server_sock = -1;
        LOG(LOG_INFO, "close server sock\n");
    }
}

static void _net_close_client_connect()
{
    if (connect_sock != -1)
    {
        closesocket(connect_sock);
        connect_sock = -1;
        LOG(LOG_INFO, "close client connect\n");
    }
}

static rt_bool_t _net_init_server()
{
    struct sockaddr_in server_addr;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1)
    {
        int err = rt_get_errno();
        ERROR_LOG("failed to creat sock, ret %d\n", err);
        return RT_FALSE;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(5000);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    rt_memset(&(server_addr.sin_zero), 0, sizeof(server_addr.sin_zero));

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr))
         < 0)
    {
        _net_close_server();
        ERROR_LOG("unable to bind\n");
        return RT_FALSE;
    }

    if (listen(server_sock, 5) < 0)
    {
        _net_close_server();
        ERROR_LOG("listen err\n");
        return RT_FALSE;
    }

    DEBUG_LOG("net server waiting for client on port 5000...\n");

    return RT_TRUE;
}

static void _printf_station_cfg(const CFG_OPTION_T* p_cfg)
{
    rt_kprintf("id = %u\n", p_cfg->u32BsId + 20000);
    rt_kprintf("ip = "); app_display_ipaddr(p_cfg->u32BsIp);
    rt_kprintf("gw = "); app_display_ipaddr(p_cfg->u32BsGW);
    rt_kprintf("mk = "); app_display_ipaddr(p_cfg->u32BsMK);
    rt_kprintf("svip = "); app_display_ipaddr(p_cfg->u32SvIp);
    rt_kprintf("svport = %u\n", p_cfg->u32SvPort);
    rt_kprintf("ipmode = %s\n", p_cfg->u32IpMode? "static" : "dhcp");
    rt_kprintf("delay = %u\n", p_cfg->u32Delay);
}

static void _net_report_station_config()
{
    PACKET_HEADER_T *pNet_head = (PACKET_HEADER_T*)server_msg_buf;
    NWK_HDR_T *pNwk_head = (NWK_HDR_T *)(pNet_head + 1);
    APP_HDR_T *pApp_head = (APP_HDR_T *)(pNwk_head + 1);
    CFG_OPTION_T* p_cfg = (CFG_OPTION_T*)(pApp_head + 1);

    memset(server_msg_buf, 0, sizeof(server_msg_buf));
    pNet_head->tag = COMM_3G_DATA;
    pNet_head->len = sizeof(NWK_HDR_T) + sizeof(app_stm32_config_t);
    pApp_head->protocoltype = APP_PROTOCOL_TYPE_STATION;
    pApp_head->msgtype = APP_REPORT_CONFIG;
    pApp_head->len = sizeof(stm32_config_t);

    if (get_sys_cfg(p_cfg))
    {
        send(connect_sock, server_msg_buf,
            sizeof(PACKET_HEADER_T) + pNet_head->len, 0);
        DEBUG_LOG("report station config\n");
        _printf_station_cfg(p_cfg);
    }
}

static void _net_report_station_sw_version()
{
    PACKET_HEADER_T *pNet_head = (PACKET_HEADER_T*)server_msg_buf;
    NWK_HDR_T *pNwk_head = (NWK_HDR_T *)(pNet_head + 1);
    APP_HDR_T *pApp_head = (APP_HDR_T *)(pNwk_head + 1);
    app_station_sw_version_t* p_sw_ver = (app_station_sw_version_t *)pApp_head;
    int ret = 0;

    memset(server_msg_buf, 0, sizeof(server_msg_buf));
    pNet_head->tag = COMM_3G_DATA;
    pNet_head->len = sizeof(NWK_HDR_T) + sizeof(app_station_sw_version_t);
    pApp_head->protocoltype = APP_PROTOCOL_TYPE_STATION;
    pApp_head->msgtype = APP_REPORT_SW_VERSION;
    pApp_head->len = sizeof(p_sw_ver->sw_version);
    p_sw_ver->sw_version[0] = is_upgrade_complete() ?
        IAP_UPGRADE_COMPLETE : APP_RUNNING;
    snprintf((char*)p_sw_ver->sw_version + 1, sizeof(p_sw_ver->sw_version),
        "APP software version %s release %s", VERSION, RELEASE);

    ret = send(connect_sock, server_msg_buf,
        sizeof(PACKET_HEADER_T) + pNet_head->len, 0);
    if (ret != sizeof(PACKET_HEADER_T) + pNet_head->len) {
        ERROR_LOG("failed to report station sw version, ret %d\n", ret);
        THREAD_MSLEEP(10);
        send(connect_sock, server_msg_buf,
            sizeof(PACKET_HEADER_T) + pNet_head->len, 0);
    }
    DEBUG_LOG("report station sw version %s %s\n", VERSION, RELEASE);
}

static void _net_server_msg_handler(void* buf, int len)
{
    PACKET_HEADER_T *pstNet_head = (PACKET_HEADER_T*)buf;
    NWK_HDR_T *pstNwk_head = (NWK_HDR_T *)(pstNet_head + 1);
    APP_HDR_T *pstApp_head = (APP_HDR_T *)(pstNwk_head + 1);
    CFG_OPTION_T* p_cfg = (CFG_OPTION_T*)(pstApp_head + 1);

    if (!buf || !len)
    {
        return;
    }

    if (COMM_3G_DATA == pstNet_head->tag)
    {
        if ((APP_PROTOCOL_TYPE_STATION == pstApp_head->protocoltype) || (APP_PROTOCOL_TYPE_UWB_CARD == pstApp_head->protocoltype))
        {
            if (APP_QUERY_CONFIG == pstApp_head->msgtype)
            {
                _net_report_station_config();
            }
            else if (APP_SET_CONFIG == pstApp_head->msgtype)
            {
                if (set_sys_cfg(p_cfg))
                {
                    DEBUG_LOG("set station config\n");
                    _printf_station_cfg(p_cfg);
                    _net_report_station_config();

                    DEBUG_LOG("wait 1s ...\n");
                    THREAD_MSLEEP(1000);

                    DEBUG_LOG("reboot the station to activate the config\n");
                    _net_close_client_connect();
                    _net_close_server();
                    DEBUG_LOG("\n");
                    rt_hw_board_reboot();
                }
                else
                {
                    ERROR_LOG("set sys config error\n");
                }
            }
            else if (APP_UPGRADE_STATON_SW == pstApp_head->msgtype)
            {
                set_need_upgrade_flag();
                _net_close_client_connect();
                _net_close_server();

                DEBUG_LOG("wait 1s ...\n");
                THREAD_MSLEEP(1000);

                DEBUG_LOG("reboot the station to upgrade software\n\n");
                rt_hw_board_reboot();
            }
            else if(APP_SET_STATION_CHANNEL == pstApp_head->msgtype)
            {
                //MSG_CENTER_HEADER_T *p_msg_hdr = RT_NULL;
                //unsigned char* buf_wpos = RT_NULL;
                //memset(channel_buf, 0, sizeof(channel_buf));
                //p_msg_hdr = (MSG_CENTER_HEADER_T *)channel_buf;
                //p_msg_hdr->u32MsgId = ETH0_TRANSMIT_ID;
                //buf_wpos = channel_buf + sizeof(MSG_CENTER_HEADER_T);
                //memcpy(buf_wpos, buf, sizeof(pstNet_head)+pstNet_head->len);
                //_net_sendto_analyser(channel_buf,MSG_HEADER_SIZE + sizeof(pstNet_head) + pstNet_head->len);


                bsmac_send_packet((rt_uint8_t*)pstNwk_head, sizeof(struct nwkhdr) + pstNwk_head->len, 0);
                bsmac_send_packet((rt_uint8_t*)pstNwk_head, sizeof(struct nwkhdr) + pstNwk_head->len, 1);
                bsmac_send_packet((rt_uint8_t*)pstNwk_head, sizeof(struct nwkhdr) + pstNwk_head->len, 2);
                rt_kprintf("channel %u\n", pstNet_head->len);
            }
	else if(APP_UWB_MSG_SET == pstApp_head->msgtype)
	{
		bsmac_send_packet((rt_uint8_t*)pstNwk_head, sizeof(struct nwkhdr) + pstNwk_head->len, 0);
                bsmac_send_packet((rt_uint8_t*)pstNwk_head, sizeof(struct nwkhdr) + pstNwk_head->len, 1);
                bsmac_send_packet((rt_uint8_t*)pstNwk_head, sizeof(struct nwkhdr) + pstNwk_head->len, 2);
		rt_kprintf("set %u\n", pstNet_head->len);		
	}
        }
    }
}

static int _net_server_recv_data(char* data_buf, int data_size)
{
    int ret = 0;
    int rec_size = 0;
    rt_err_t err = 0;
    int err_cnt = 0;

    RT_ASSERT(data_buf);

    if (!data_size) {
        return 0;
    }

    while (rec_size < data_size) {
        ret = recv(connect_sock, data_buf + rec_size, data_size - rec_size,
                    MSG_DONTWAIT);
        err = rt_get_errno();
        DEBUG_LOG("net server rec ret %d, err %d\n", ret, err);
        if (ret < 0) {
            if (err == EWOULDBLOCK) {
                /* 休眠一段时间继续读取 */
                if (++err_cnt >= 11)
                {
                        return -1;
                }
                else
                {
                    THREAD_MSLEEP(200);
                    continue;
                }
            } else {
                return -1;
            }
        } else if (ret == 0) {
            return -1;
        } else {
            err_cnt = 0;
            rec_size += ret;
            continue;
        }
    }

    return data_size;
}

static void _net_server_work()
{
    u32_t sin_size = 0;
    struct sockaddr_in client_addr;
    int bytes_rec = 0;
    PACKET_HEADER_T pkt_hdr = {0, 0};
    rt_err_t err = 0;

    if (server_sock == -1)
    {
        if (!_net_init_server())
        {
            THREAD_MSLEEP(1000);
            return;
        }
    }

    if (connect_sock == -1)
    {
        sin_size = sizeof(struct sockaddr_in);
        connect_sock = accept(server_sock, (struct sockaddr *)&client_addr, &sin_size);
        if (connect_sock == -1)
        {
            err = rt_get_errno();
            THREAD_MSLEEP(1000);
            ERROR_LOG("net server accpet err %d\n", err);
            return;
        }

        _net_report_station_sw_version();
        DEBUG_LOG("net got connection from (%s : %d)\n",
            inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    }

#ifdef NET_SERVER_BLOCKING_RECEIVE
    bytes_rec = recv(connect_sock, &pkt_hdr, sizeof(pkt_hdr), 0);
#else
    bytes_rec = _net_server_recv_data((char*)&pkt_hdr, sizeof(pkt_hdr));
#endif
    if (bytes_rec != sizeof(pkt_hdr))
    {
        _net_close_client_connect();
        ERROR_LOG("rec size err %d, need rec %d\n",
            bytes_rec, sizeof(pkt_hdr));
        return;
    }

    if ((pkt_hdr.len <= 0) ||
        (sizeof(pkt_hdr) + pkt_hdr.len) > sizeof(server_msg_buf))
    {
        _net_close_client_connect();
        ERROR_LOG("net rec pkt hdr length error, tag %d len %d\n",
            pkt_hdr.tag, pkt_hdr.len);
        return;
    }

    memset(server_msg_buf, 0, sizeof(server_msg_buf));
    memcpy(server_msg_buf, &pkt_hdr, sizeof(pkt_hdr));

#ifdef NET_SERVER_BLOCKING_RECEIVE
    bytes_rec = recv(connect_sock, server_msg_buf + sizeof(pkt_hdr),
        pkt_hdr.len, 0);
#else
    bytes_rec = _net_server_recv_data(server_msg_buf + sizeof(pkt_hdr),
        pkt_hdr.len);
#endif
    if (bytes_rec != pkt_hdr.len)
    {
        _net_close_client_connect();
        ERROR_LOG("rec size err %d, need rec %d\n",
            bytes_rec, pkt_hdr.len);
        return;
    }

    _net_server_msg_handler(server_msg_buf, sizeof(pkt_hdr) + pkt_hdr.len);
}

static void _net_server_work_thread(void *para)
{
     while (1) {
        _net_server_work();
        THREAD_MSLEEP(NET_SERVER_THREAD_SLEEP_MS);
    }
}

rt_bool_t start_net_work()
{
    rt_thread_t net_recv_thread = RT_NULL;
    rt_thread_t net_send_thread = RT_NULL;
    rt_thread_t net_server_thread = RT_NULL;

    net_recv_fine_flag = 0;
    net_send_fine_flag = 0;

    /* 网络线程开始喂狗 */
    feed_watchdog();
    iwdg_net_feed_flag = 1;

    msg_analyser_register(ETH0_TRANSMIT_ID, _net_analyser_callback);

    _config_ipaddr();

    if(_create_socket_mutex() < 0)
    {
       return RT_FALSE;
    }

    net_recv_thread = rt_thread_create("net recv thread",
        _net_recv_work_thread, RT_NULL, 2048, 8, NET_RECV_THREAD_TICK);
    net_send_thread = rt_thread_create("net send thread",
        _net_send_work_thread, RT_NULL, 2048, 8, NET_SEND_THREAD_TICK);
    net_server_thread = rt_thread_create("net server thread",
        _net_server_work_thread, RT_NULL, 2048, 8, NET_SERVER_THREAD_TICK);

    if (net_recv_thread == RT_NULL)
    {
        ERROR_LOG("create net recv thread failed\n");
        return RT_FALSE;
    }

    if (net_send_thread == RT_NULL)
    {
        ERROR_LOG("create net send thread failed\n");
        return RT_FALSE;
    }

    if (net_server_thread == RT_NULL)
    {
        ERROR_LOG("create net server thread failed\n");
        return RT_FALSE;
    }

    rt_thread_startup(net_recv_thread);
    rt_thread_startup(net_send_thread);
    rt_thread_startup(net_server_thread);

    /* 定时器开始喂狗 */
    startup_iwdg_feeding_timer();

    return RT_TRUE;
}

