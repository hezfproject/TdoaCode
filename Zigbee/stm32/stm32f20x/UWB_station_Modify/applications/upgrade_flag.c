#include <rtthread.h>
#include <dfs_posix.h>

#include "upgrade_flag.h"

//#define LOG_DEBUG
#include "3g_log.h"

enum
{
    FLAG_NO_UPGRADE = 0,    /* 不需要升级 */
    FLAG_NEEDED_UPGRADE,    /* 需要升级 */
    FLAG_IN_UPGRADE,        /* 正在升级中 */
    FLAG_UPGRADE_COMPLETE,  /* 已完成升级 */
};

int station_startup_mode = 0;

const char UPGRADE_FLAG_FILE[] = "/upgrade";

int read_upgrade_flag(void)
{
    int fd = 0;
    int ret = 0;
    int flag = 0;

    fd = open(UPGRADE_FLAG_FILE, O_RDONLY, 0);
    if (fd < 0)
    {
        ERROR_LOG("open upgrade flag file %s error\n", UPGRADE_FLAG_FILE);
        return -1;
    }

    ret = read(fd, &flag, sizeof(flag));
    close(fd);
    if (ret != sizeof(flag))
    {
        unlink(UPGRADE_FLAG_FILE);
        ERROR_LOG("faile to read upgrade flag file %s, ret %d\n",
            UPGRADE_FLAG_FILE, ret);
        return -1;
    }

    return flag;
}

int write_upgrade_flag(int flag_value)
{
    int fd = 0;
    int ret = 0;
    int flag = flag_value;

    fd = open(UPGRADE_FLAG_FILE, O_CREAT | O_RDWR, 0);
    if (fd < 0)
    {
        ERROR_LOG("open upgrade flag file %s error\n", UPGRADE_FLAG_FILE);
        return -1;
    }

    ret = write(fd, &flag, sizeof(flag));
    close(fd);
    if (ret != sizeof(flag))
    {
        unlink(UPGRADE_FLAG_FILE);
        ERROR_LOG("faile to write upgrade flag file %s, ret %d\n",
            UPGRADE_FLAG_FILE, ret);
        return -1;
    }

    return flag;
}

rt_bool_t set_need_upgrade_flag(void)
{
    return (write_upgrade_flag(FLAG_NEEDED_UPGRADE) == FLAG_NEEDED_UPGRADE) ?
        RT_TRUE : RT_FALSE;
}

rt_bool_t set_in_upgrade_flag(void)
{
    return (write_upgrade_flag(FLAG_IN_UPGRADE) == FLAG_IN_UPGRADE) ?
        RT_TRUE : RT_FALSE;
}

rt_bool_t set_upgrade_complete_flag(void)
{
    return (write_upgrade_flag(FLAG_UPGRADE_COMPLETE) == FLAG_UPGRADE_COMPLETE) ?
        RT_TRUE : RT_FALSE;
}

rt_bool_t clear_upgrade_flag(void)
{
     return (write_upgrade_flag(FLAG_NO_UPGRADE) == FLAG_NO_UPGRADE) ?
        RT_TRUE : RT_FALSE;
}

rt_bool_t is_upgrade_needed(void)
{
    int ret = read_upgrade_flag();

    if (ret == FLAG_NEEDED_UPGRADE) {
        station_startup_mode = IAP_NEED_UPGRADE;
        clear_upgrade_flag();
        return RT_TRUE;
    }
    else if (ret == FLAG_IN_UPGRADE) {
        station_startup_mode = IAP_IN_UPGRADE;
        return RT_TRUE;
    }

    return RT_FALSE;
}

rt_bool_t is_in_upgrade(void)
{
    int ret = read_upgrade_flag();
    if (ret == FLAG_IN_UPGRADE) {
        return RT_TRUE;
    }

    return RT_FALSE;
}

rt_bool_t is_upgrade_complete(void)
{
    int ret = read_upgrade_flag();
    if (ret == FLAG_UPGRADE_COMPLETE) {
        clear_upgrade_flag();
        return RT_TRUE;
    }

    return RT_FALSE;
}
