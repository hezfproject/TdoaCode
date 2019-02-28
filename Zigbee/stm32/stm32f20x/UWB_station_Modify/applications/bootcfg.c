#include "bootcfg.h"
#include "board.h"
#include <string.h>
#include <stdlib.h>
#include <dfs_posix.h>
#include "net_app.h"
#include <ctype.h>
#include <lwip/inet.h>

#include "3g_log.h"

#include "stm32_flash.h"

#define BOOT_INFO_FN    "/boot.inf"
#define BOOTSH_PROMPT   "bootsh>>"

#define OPTION_BSIP     "bsip="
#define OPTION_BSMK     "bsmk="
#define OPTION_BSGW     "bsgw="
#define OPTION_BSID     "bsid="
#define OPTION_SVIP     "svip="
#define OPTION_DELAY    "delay="
#define OPTION_REBOOT   "reboot"
#define OPTION_SHOW     "show"
#define OPTION_IPMODE   "ipmode="
#define OPTION_HELP     "help"
#define OPTION_SAVEENV  "saveenv"

#define CMD_LINE_MAX    256

typedef struct bootsh_cmd_t
{
    char *cmd;
    void(*pfn)(char *op, const char* pos, rt_uint32_t* pVal);
    rt_uint32_t *pVal;
}BOOTSH_CMD_T;

typedef struct bootsh_t
{
    struct rt_semaphore rx_sem;
    char line[CMD_LINE_MAX];
    int line_position;
    rt_device_t device;
}BOOTSH_T;

BOOTSH_T *bootsh;

CFG_OPTION_T sys_option;
struct rt_semaphore sys_option_sem;

static rt_err_t bootsh_rx_ind(rt_device_t dev, rt_size_t size)
{
	RT_ASSERT(bootsh != RT_NULL);

	/* release semaphore to let bootsh thread rx data */
	rt_sem_release(&bootsh->rx_sem);

	return RT_EOK;
}

/**
 * @ingroup finsh
 *
 * This function sets the input device of boot shell.
 *
 * @param device_name the name of new input device.
 */
void bootsh_set_device(const char* device_name)
{
	rt_device_t dev = RT_NULL;

	RT_ASSERT(bootsh != RT_NULL);
	dev = rt_device_find(device_name);

	if (dev != RT_NULL && rt_device_open(dev, RT_DEVICE_OFLAG_RDWR) == RT_EOK)
	{
		if (bootsh->device != RT_NULL)
		{
			/* close old finsh device */
			rt_device_close(bootsh->device);
		}

		bootsh->device = dev;
		rt_device_set_rx_indicate(dev, bootsh_rx_ind);
	}
	else
	{
		rt_kprintf("finsh: can not find device:%s\n", device_name);
	}
}

void bootsh_show_menu(void)
{
    rt_kprintf("\n*******************************************\n");
    rt_kprintf("*        Startup configuration options    *\n");
    rt_kprintf("*******************************************\n");
    rt_kprintf("* options:                                *\n");
    rt_kprintf("*         bsip=?  (Base Station IPaddress *\n");
    rt_kprintf("*                  do without port number)*\n");
    rt_kprintf("*         bsmk=?  (Base Station Netmask)  *\n");
    rt_kprintf("*         bsgw=?  (Base Station GateWay)  *\n");
//  rt_kprintf("*         bsid=?  (Base Station ID)       *\n");
    rt_kprintf("*         svip=?  (do with port number)   *\n");
    rt_kprintf("*         ipmode  (dhcp/static)           *\n");
    rt_kprintf("*         delay=? (second for unit)       *\n");
    rt_kprintf("*         saveenv (save configuration)    *\n");
    rt_kprintf("*         show    (show config)           *\n");
    rt_kprintf("*         reboot  (reboot)                *\n");
    rt_kprintf("*         help    (display this window)   *\n");
    rt_kprintf("* example:                                *\n");
    rt_kprintf("*     bootsh>>bsip=192.168.8.8            *\n");
//  rt_kprintf("*     bootsh>>bsid=12345                  *\n");
    rt_kprintf("*     bootsh>>svip=192.168.8.8:8080       *\n");
    rt_kprintf("*     bootsh>>delay=3                     *\n");
    rt_kprintf("*     bootsh>>ipmode=dhcp                 *\n");
    rt_kprintf("*******************************************\n");
}

void _show_cfg(char *option, const char *val_pos, rt_uint32_t *pVal)
{
    if (!val_pos || *val_pos != ';')
        return;
    rt_kprintf("\nbsid=%u\n", sys_option.u32BsId + 20000);
    rt_kprintf("\nbsip="); app_display_ipaddr(sys_option.u32BsIp);
    rt_kprintf("\nbsgw="); app_display_ipaddr(sys_option.u32BsGW);
    rt_kprintf("\nbsmk="); app_display_ipaddr(sys_option.u32BsMK);
    rt_kprintf("\nsvip="); app_display_ipaddr(sys_option.u32SvIp);
    rt_kprintf("\nsvport=%u\n", sys_option.u32SvPort);
    rt_kprintf("\nipmode=%s\n", sys_option.u32IpMode?"static":"dhcp");
    rt_kprintf("\ndelay=%u\n\n", sys_option.u32Delay);
}

static rt_bool_t _getStrOfIp(char *ipbuf, const char *val_pos)
{
    int i = 0;
    char *point = (char *)(val_pos);
    char *end;
    char *p;

    end = strchr(val_pos, ';');
    while (' ' == *point)
        point++;
    if (point >= end)
        return RT_FALSE;

    p = point;
    while (i < 3 && p && p < end)
    {
        if ('.' == *p)
        {
            i++;
        }
        else if (!isdigit(*p))
            break;
        p++;
    }

    if (i < 3)
    {
        return RT_FALSE;
    }

    while (*p && p < end)
    {
        if (isdigit(*p))
            p++;
        else
            break;
    }

    if (p - point < 16)
        strncpy(ipbuf, point, p - point);
    else
        return RT_FALSE;
    return RT_TRUE;
}

static void _getStrOfPort(const char *val_pos)
{
    char *point;

    point = strchr(val_pos, ':');
    if (point)
    {
        int port = atoi(point + 1);

        if (port > 0 && port < 99999)
        {
            sys_option.u32SvPort = port;
            rt_kprintf("server port is %d ok\n", port);
        }
        else
        {
            rt_kprintf("server port is error\n");
        }
    }
}

void _ipmkgw_process(char *option, const char *val_pos, rt_uint32_t *pVal)
{
    struct in_addr ipaddr;
    char ipbuf[16] = {0};

    if (!option || !val_pos || !pVal)
        goto ERR;

    if (!_getStrOfIp(ipbuf, val_pos))
        goto ERR;
    if (!inet_aton(ipbuf, (struct in_addr*)(&ipaddr)))
    {
        goto ERR;
    }
    else
    {
        *pVal = ipaddr.s_addr;
        rt_kprintf("%s\b value ok\n", option);

        if (!strncmp(val_pos - 5, "svip", 4))
        {
            _getStrOfPort(val_pos);
        }

        return;
    }

ERR:
    rt_kprintf("%s\b value error\n", option);
    return;
}

void _delayid_process(char *option, const char *val_pos, rt_uint32_t* pVal)
{
    int id;

    if (!option || !val_pos || !pVal)
        return;

    if ((id = atoi(val_pos)) < 0)
        rt_kprintf("%s\b value error\n", option);
    else
    {
        if (id > 9 && !strcasecmp(option, OPTION_DELAY))
        {
            rt_kprintf("%s\b value error\n", option);
            return;
        }

        *pVal = id;
        rt_kprintf("%s\b value ok\n", option);
    }
}

rt_bool_t save_configuration(void)
{
#if 0
    int bootfd;
    int wsize = -1;

    bootfd = open(BOOT_INFO_FN, O_CREAT | O_RDWR, 0);

    if (bootfd > -1)
    {
        wsize = write(bootfd, &sys_option, sizeof(CFG_OPTION_T));

        if (close(bootfd) < 0)
        {
            ERROR_LOG("close file %d failed\n", BOOT_INFO_FN);
        }
    }
    else
    {
        ERROR_LOG("open %s error\n", BOOT_INFO_FN);
    }

    if (wsize != sizeof(CFG_OPTION_T))
    {
        ERROR_LOG("save configuration error\n");
    }
    else
    {
        LOG(LOG_CRITICAL, "save configuration ok\n");
        return RT_TRUE;
    }

    return RT_FALSE;
#else
    rt_bool_t status;

    status = flash_save(OPTION_FLASH_ADDR, (u8*)(&sys_option), sizeof(CFG_OPTION_T));

    if (!status)
    {
        ERROR_LOG("save configuration error\n");
    }
    else
    {
        LOG(LOG_CRITICAL, "save configuration ok\n");
    }

    return status;
#endif
}

void _save_configuration(char *option, const char *val_pos, rt_uint32_t* pVal)
{
    if (!val_pos || *val_pos != ';')
        return;

    save_configuration();
}

void _reboot_process(char *option, const char *val_pos, rt_uint32_t* pVal)
{
    if (!val_pos || *val_pos != ';')
        return;

    LOG(LOG_CRITICAL, "reboot station...\n\n");
    rt_hw_board_reboot();
}

void _ipmode_process(char *option, const char *val_pos, rt_uint32_t* pVal)
{
    if (!option || !pVal || !val_pos)
        return;

    if (!strncasecmp(val_pos, "dhcp;", 5))
    {
        *pVal = 0;
        rt_kprintf("dhcp first, ok\n");

    }
    else if (!strncasecmp(val_pos, "static;", 3))
    {
         // 关闭DHCP前必须先配置好ip
        if (!sys_option.u32BsIp)
        {
            rt_kprintf("static first, static ipaddr is error\n");
        }
        else
        {
            *pVal = 1;
        }
    }
    else
    {
        rt_kprintf("%s\b error\n");
    }
    return;
}

void _help_process(char *option, const char *val_pos, rt_uint32_t* pVal)
{
    if (!val_pos || *val_pos != ';')
        return;

    bootsh_show_menu();
}

void bootsh_run_line(char* cmd, int cmdlen)
{
    char *pos, *pre;
    int i, cmd_len;
    int cmd_cnt = 0;

    BOOTSH_CMD_T cmd_process[] = {
        {OPTION_BSIP, _ipmkgw_process, &sys_option.u32BsIp},
//      {OPTION_BSID, _delayid_process, &sys_option.u32BsId},
        {OPTION_BSGW, _ipmkgw_process, &sys_option.u32BsGW},
        {OPTION_BSMK, _ipmkgw_process, &sys_option.u32BsMK},
        {OPTION_SVIP, _ipmkgw_process, &sys_option.u32SvIp},
        {OPTION_DELAY, _delayid_process, &sys_option.u32Delay},
        {OPTION_SAVEENV, _save_configuration, RT_NULL},
        {OPTION_REBOOT, _reboot_process, RT_NULL},
        {OPTION_SHOW, _show_cfg, RT_NULL},
        {OPTION_IPMODE, _ipmode_process, &sys_option.u32IpMode},
        {OPTION_HELP, _help_process, RT_NULL}
    };

    if (!cmd || !cmdlen)
    {
        return;
    }

    while (*cmd != '\0' && cmdlen > 0)
    {
        if (*cmd == ';' || *cmd == ' ')
        {
            cmd++;cmdlen--;
        }
        else
        {
            break;
        }
    }

    pos = cmd;
    cmd_len = cmdlen;
    cmd_cnt = sizeof(cmd_process)/sizeof(BOOTSH_CMD_T);

    while (cmdlen > 0)
    {
        for (i=0; i<cmd_cnt; i++)
        {
            int option_len = strlen(cmd_process[i].cmd);

            if ((cmdlen >= option_len) && !strncasecmp(pos, cmd_process[i].cmd, option_len))
            {
                // 偏移到值域
                pos += option_len;

                cmd_process[i].pfn(cmd_process[i].cmd, pos, cmd_process[i].pVal);

                // 偏移到下一条命令
                pre = strchr(pos, ';');

                if (pre)
                {
                    pre++;
                    // 去掉上一条命令的长度
                    cmdlen -= pre - pos;
                    pos = pre;
                }
                break;
            }
        }
        if (cmd_len != cmdlen)
        {
            cmd_len = cmdlen;
        }
        else// 无法解析
        {
            if (!pos &&  *pos )
                rt_kprintf("%s command error\n", pos);
            break;
        }
    }
}

#ifndef RT_USING_HEAP
BOOTSH_T _bootsh;
#endif
void bootsh_init(void)
{
#ifdef RT_USING_HEAP
    bootsh = (BOOTSH_T*)rt_malloc(sizeof(BOOTSH_T));
#else
    bootsh = &_bootsh;
#endif

    if (!bootsh)
    {
        rt_kprintf("no memory for shell\n");
        return;
    }

    memset(bootsh, 0, sizeof(BOOTSH_T));

    rt_sem_init(&(bootsh->rx_sem), "btsrx", 0, RT_IPC_FLAG_FIFO);
}

rt_bool_t _get_flashparam(void)
{
#if 0
    int bootfd;

    bootfd = open(BOOT_INFO_FN, O_RDONLY, 0);

    if (bootfd >= 0)
    {
        if (read(bootfd, &sys_option, sizeof(CFG_OPTION_T))
            != sizeof(CFG_OPTION_T))
        {
            close(bootfd);
            unlink(BOOT_INFO_FN);
            rt_kprintf("No default configuration, the need for configuration\n");
            return RT_FALSE;
        }

        close(bootfd);
        return RT_TRUE;
    }
    else
    {
        rt_kprintf("open %s error\n", BOOT_INFO_FN);
        return RT_FALSE;
    }
#else
    rt_bool_t status;

    status = flash_read(OPTION_FLASH_ADDR, (u8*)(&sys_option), sizeof(CFG_OPTION_T));

    if (!status)
    {
        rt_kprintf("read configure data failure\n");
    }

    return status;
#endif
}

rt_bool_t _verify_flashparam(void)
{
    if (sys_option.u32Delay > 9)
    {
        rt_kprintf("%s %d is not allowed\n", OPTION_DELAY, sys_option.u32Delay);
        return RT_FALSE;
    }

    if (!sys_option.u32SvIp || !sys_option.u32SvPort)
    {
        rt_kprintf("%s %d:%d is not allowed\n",
                    OPTION_SVIP, sys_option.u32SvIp, sys_option.u32SvPort);
        return RT_FALSE;
    }

    if (IPMODE_DHCP != sys_option.u32IpMode && !sys_option.u32BsIp)
    {
        rt_kprintf("%s %d is not allowed\n", OPTION_BSIP, sys_option.u32BsIp);
        return RT_FALSE;
    }

    return RT_TRUE;
}

/*
如果返回true说明获取到了默认配置，否则将需要进行配置才能启动
*/
rt_bool_t get_bootparam(void)
{
    char ch, passwd[5]= "asdf";
    int i = 0, delay, dels = 0; // 10S

    if (!_get_flashparam())
        return RT_FALSE;

    if (!_verify_flashparam())
        return RT_FALSE;

    delay = sys_option.u32Delay;
    if (!delay)
        delay = 1;
    /* wait passwd */
    while (delay >= 0)
    {
        if (!dels)
        {
            rt_kprintf("\r");
            rt_kprintf("wait time: %d", delay);
            delay--;
            dels = RT_TICK_PER_SECOND;
        }
        if (rt_sem_take(&bootsh->rx_sem, 1) == RT_EOK)
        {
            while (rt_device_read(bootsh->device, 0, &ch, 1) == 1)
            {
                if (passwd[i] != ch)
                {
                    i = 0;
                }
                else if (i + 1 > 3)
                {
                    return RT_FALSE;
                }
                i = ((i + 1) & 3);
            }
        }

        dels--;
    }
    rt_kprintf("\n");
    return RT_TRUE;
}

void bootsh_thread_entry(void* parameter)
{
    char ch;

    if (get_bootparam())
    {
        rt_sem_release(&sys_option_sem);
        return;
    }

    bootsh_show_menu();
    rt_kprintf(BOOTSH_PROMPT);

	while (1)
	{
        /* wait receive */
		while (rt_sem_take(&bootsh->rx_sem, RT_WAITING_FOREVER) != RT_EOK);
		/* read one character from device */
		while (rt_device_read(bootsh->device, 0, &ch, 1) == 1)
		{
			/* handle CR key */
			if (ch == '\r')
			{
				char next;

				if (rt_device_read(bootsh->device, 0, &next, 1) == 1)
					ch = next;
				else ch = '\n';
			}
			/* handle backspace key */
			else if (ch == 0x7f || ch == 0x08)
			{
				if (bootsh->line_position != 0)
				{
					rt_kprintf("%c %c", ch, ch);
				}
				if (bootsh->line_position <= 0)
                    bootsh->line_position = 0;
				else
                    bootsh->line_position--;
				bootsh->line[bootsh->line_position] = 0;
				continue;
			}

			/* handle end of line, break */
			if (ch == '\n' || ch == '\r')
			{
				/* change to ';' and break */
				bootsh->line[bootsh->line_position] = ';';
                rt_kprintf("\n");

				if (bootsh->line_position != 0)
                    bootsh_run_line(bootsh->line, bootsh->line_position);

				rt_kprintf(BOOTSH_PROMPT);
				memset(bootsh->line, 0, sizeof(bootsh->line));
				bootsh->line_position = 0;

				break;
			}

			/* it's a large line, discard it */
			if (bootsh->line_position >= CMD_LINE_MAX)
                bootsh->line_position = 0;

			/* normal character */
			bootsh->line[bootsh->line_position] = ch;
            rt_kprintf("%c", ch);
			bootsh->line_position++;
		} /* end of device read */
	}
}

static rt_thread_t boot_thread = RT_NULL;

rt_bool_t start_boot_work()
{
    boot_thread = rt_thread_create("boot",
        bootsh_thread_entry, RT_NULL, 1024, 9, 200);

    if (boot_thread == RT_NULL)
    {
        ERROR_LOG("create boot work thread failed\n");
        return RT_FALSE;
    }

    rt_thread_startup(boot_thread);

    return RT_TRUE;
}

rt_bool_t get_sys_cfg(CFG_OPTION_T* cfg_buf)
{
#if 0
    int fd = 0;
    int ret = 0;

    fd = open(BOOT_INFO_FN, O_RDONLY, 0);
    if (fd < 0)
    {
        ERROR_LOG("open config file %s error\n", BOOT_INFO_FN);
        return RT_FALSE;
    }

    ret = read(fd, cfg_buf, sizeof(CFG_OPTION_T));
    close(fd);
    if (ret != sizeof(CFG_OPTION_T))
    {
        unlink(BOOT_INFO_FN);
        ERROR_LOG("faile to read config file %s, ret %d\n",
            BOOT_INFO_FN, ret);
        return RT_FALSE;
    }

    cfg_buf->u32BsId = sys_option.u32BsId;
#endif
    if (!cfg_buf)
        return RT_FALSE;

    *cfg_buf = sys_option;

    return RT_TRUE;
}

rt_bool_t set_sys_cfg(const CFG_OPTION_T* cfg_buf)
{
    if (!cfg_buf)
        return RT_FALSE;

    sys_option = *cfg_buf;

    return save_configuration();
}

