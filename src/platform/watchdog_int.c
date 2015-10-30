/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年10月17日 13:19:03
用途        : 看门狗管理，默认情况下60s不喂狗便会重启
历史修改记录: v1.0    创建
**********************************************************************/
#include "util/ci_header.h"

#ifdef LINUX_ENVRIONMENT

#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/watchdog.h>

#include "util/ci_header.h"
#include "util/log.h"
#include "watchdog_int.h"
#include "timer.h"

/*看门狗文件句柄*/
static int watchdog_fd = 0;

#define WATCHDOG_CYCLE_NANO 1000000000      /*看门狗的周期，使用纳秒级，默认为1s*/
#define WATCHDOG_CYCLE_RELATIVE ((int)(WATCHDOG_CYCLE_NANO / TIMER_CYCLE_NANO))  /*相对于主定时器的周期*/

static void watchdog_int_action(void);

static CI_BOOL watchdog_condition_check(uint32_t elapsed_cycle);

static const CITimer watchdog_timer = 
{ 
    WATCHDOG_CYCLE_RELATIVE,
    watchdog_int_action,
    watchdog_condition_check,
    "watchdog_timer",
    CI_TRUE,                        /*默认打开该定时器*/
};

/*
 功能描述    : 开启看门狗
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年4月25日 13:36:58
*/
static int32_t watchdog_start(void)
{
    int ret = 0;
    int flags = 0;

    flags = WDIOS_ENABLECARD;
    ret = ioctl(watchdog_fd, WDIOC_SETOPTIONS, &flags);
    if(-1 == ret)
    {
        CILog_Errno("start watchdog error");
        return -1;
    }

    return 0;
}
/*
 功能描述    : 停止看门狗
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年4月25日 13:34:16
*/
int32_t CIWatchdog_Stop(void)
{
    int ret = 0;
    int flags = 0;

    flags = WDIOS_DISABLECARD;
    ret = ioctl(watchdog_fd, WDIOC_SETOPTIONS, &flags);
    if(-1 == ret)
    {
        CILog_Errno("stop watchdog error");

        return -1;
    }
    return 0;
}
/*
 功能描述    : 看门狗喂狗
 返回值      : 无
 参数        : 
 作者        : 张彦升
 日期        : 2014年4月25日 13:45:10
*/
static void watchdog_keep_alive(void)
{
    int dummy = 0;

    ioctl(watchdog_fd, WDIOC_KEEPALIVE, &dummy);

    return;
}
/*
 功能描述    : 有间隔的喂狗函数
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年4月25日 13:45:41
*/
static void watchdog_int_action(void)
{
    static int b_started = CI_FALSE;

    if (CI_FALSE == b_started)
    {
        watchdog_start();
        b_started = CI_TRUE;
    }

    watchdog_keep_alive();

    return;
}

static CI_BOOL watchdog_condition_check(uint32_t elapsed_cycle)
{
    if (CI_FALSE == watchdog_timer.b_open)
    {
        return CI_FALSE;
    }
    if (0 != elapsed_cycle % watchdog_timer.relative_cycle)
    {
        return CI_FALSE;
    }

    return CI_TRUE;
}
/*
 功能描述    : 初始化看门狗
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年4月25日 13:34:23
*/
int32_t CIWatchdog_Init(void)
{
    static CI_BOOL b_initialized = CI_FALSE;
    int ret = 0;

    if (CI_TRUE == b_initialized)
    {
        return -1;
    }

    watchdog_fd = open("/dev/watchdog",O_WRONLY);
    if (-1 == watchdog_fd)
    {
        CILog_Errno("open /dev/watchdog failed");

        return -1;
    }
    /*初始化的时候不让看门狗运行，等到程序进入状态机后再运行看门狗*/
    ret = CIWatchdog_Stop();
    if (-1 == ret)
    {
        return ret;
    }

    ret = CITimer_Regist(&watchdog_timer);               /*通信状态*/

    if (-1 == ret)
    {
        return ret;
    }

    b_initialized = CI_TRUE;

    return 0;
}

#endif /*!LINUX_ENVRIONMENT*/
