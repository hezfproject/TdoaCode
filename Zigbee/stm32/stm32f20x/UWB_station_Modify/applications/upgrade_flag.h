#ifndef _UPGRADE_FLAG_H_
#define _UPGRADE_FLAG_H_

rt_bool_t is_upgrade_needed(void);

rt_bool_t set_need_upgrade_flag(void);

rt_bool_t set_in_upgrade_flag(void);

rt_bool_t set_upgrade_complete_flag(void);

rt_bool_t clear_upgrade_flag(void);

rt_bool_t is_in_upgrade(void);

rt_bool_t is_upgrade_complete(void);

/// 主站软件启动模式
enum {
    APP_RUNNING = 0,
    IAP_NEED_UPGRADE,
    IAP_IN_UPGRADE,
    IAP_NO_APP,
    IAP_UPGRADE_COMPLETE,
};

extern int station_startup_mode;

#endif // _UPGRADE_FLAG_H_
