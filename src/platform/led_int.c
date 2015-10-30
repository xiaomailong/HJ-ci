/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年10月15日 10:31:46
用途        : LED点灯
             注意，该模块除了初始化函数最好不要使用任何打印语句，因为其即被信号中断函数
             所使用，又被联锁周期内部函数使用
历史修改记录: v1.0    创建
**********************************************************************/
#include "util/ci_header.h"

#ifdef LINUX_ENVRIONMENT

#include <linux/parport.h>
#include <linux/ppdev.h>
#include <sys/ioctl.h>

#include "util/ci_header.h"
#include "util/log.h"

#include "timer.h"
#include "led_int.h"

#define LED_CPU_RUN         1     /*CPU正在运行显示灯*/
#define LED_INIT_OK         2     /*初始化成功显示灯*/
#define LED_SERIES_PRIMAY   3     /*主系状态标志*/
#define LED_SERIES_STANDBY  4     /*备系状态标志*/
#define LED_FAILURE         5     /*故障标志*/
#define LED_MASK            0x1F  /*led的屏蔽标志，只用了5位，前面3位没有用*/

#define LED_CYCLE_NANO 50000000      /*LED控制的周期，使用纳秒级，默认为50ms*/
#define LED_CYCLE_RELATIVE ((int)(LED_CYCLE_NANO / TIMER_CYCLE_NANO))  /*相对于主定时器的周期*/

static int32_t led_fd = 0;              /*控制led的文件句柄*/
static unsigned char led_status = 0;    /*LED灯闪烁的标志*/

static void led_action(void);

static CI_BOOL led_condition_check(uint32_t elapsed_cycle);

static const CITimer led_timer = 
{
    LED_CYCLE_RELATIVE,
    led_action,
    led_condition_check,
    "led_timer",
    CI_TRUE,                        /*默认打开该定时器*/
};
/*
 功能描述    : 设置addr指向的地址的32位整数的第nr位为1
 返回值      : 无
 参数        : @nr 第nr位
              @addr 要改变整数的地址
 作者        : 张彦升
 日期        : 2014年4月28日 15:49:04
*/
static inline void led_set_bit(uint32_t nr,unsigned char* p_addr)
{
    unsigned char mask = 0x01;
    assert(8 > nr);

    mask <<= (nr - 1);
    *p_addr |= mask;
    *p_addr &= LED_MASK;

    return;
}
/*
 功能描述    : 清除addr指向的地址的32位整数的第nr位为0
 返回值      : 无
 参数        : @nr 第nr位
              @addr 要改变的整数的地址
 作者        : 张彦升
 日期        : 2014年4月28日 15:50:18
*/
static inline void led_clear_bit(uint32_t nr,unsigned char* p_addr)
{
    unsigned char mask = 0x01;
    assert(8 > nr);

    mask <<= (nr - 1);
    *p_addr &= ~mask;
    *p_addr &= LED_MASK;

    return;
}
/*
 功能描述    : 向led灯缓存写入数据
 返回值      : 成功为0，失败为-1
 参数        : @data 写入的数据，8位
 作者        : 张彦升
 日期        : 2014年4月28日 16:27:30
*/
static int32_t led_write_data(unsigned char data)
{
    int ret = 0;

    ret = ioctl(led_fd, PPWDATA, &data);
    if (0 > ret)
    {
        return -1;
    }

    return 0;
}

/*
 功能描述    : led定时器动作
 返回值      : 无
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月28日 15:52:01
*/
static void led_action(void)
{
    led_write_data(led_status);

    return ;
}
/*
 功能描述    : 定时中断条件检查函数
 返回值      : 成功为CI_TRUE，失败为CI_FALSE
 参数        : @elapsed_cycle 流逝的定时中断号
 作者        : 张彦升
 日期        : 2014年8月8日 14:18:01
*/
static CI_BOOL led_condition_check(uint32_t elapsed_cycle)
{
    if (CI_FALSE == led_timer.b_open)
    {
        return CI_FALSE;
    }
    if (0 != elapsed_cycle % led_timer.relative_cycle)
    {
        return CI_FALSE;
    }

    return CI_TRUE;
}
/*
 功能描述    : 点亮初始化成功指示灯
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月28日 16:26:01
*/
int32_t CILedInt_LightInit(void)
{
    led_set_bit(LED_INIT_OK,&led_status);

    led_write_data(led_status);

    return 0;
}
/*
 功能描述    : 熄灭初始化成功指示灯
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月28日 16:26:01
*/
int32_t CILedInt_BlankingInit(void)
{
    led_clear_bit(LED_INIT_OK,&led_status);

    led_write_data(led_status);

    return 0;
}
/*
 功能描述    : 点亮主系状态指示灯
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月28日 16:26:01
*/
int32_t CILedInt_LightMaster(void)
{
    led_set_bit(LED_SERIES_PRIMAY,&led_status);

    led_write_data(led_status);

    return 0;
}
/*
 功能描述    : 熄灭主系状态指示灯
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月28日 16:26:01
*/
int32_t CILedInt_BlankingMaster(void)
{
    led_clear_bit(LED_SERIES_PRIMAY,&led_status);

    led_write_data(led_status);

    return 0;
}
/*
 功能描述    : 点亮备系状态指示灯
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月28日 16:26:01
*/
int32_t CILedInt_LightStandby(void)
{
    led_set_bit(LED_SERIES_STANDBY,&led_status);

    led_write_data(led_status);

    return 0;
}
/*
 功能描述    : 熄灭备系状态指示灯
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月28日 16:26:01
*/
int32_t CILedInt_BlankingStandby(void)
{
    led_clear_bit(LED_SERIES_STANDBY,&led_status);

    led_write_data(led_status);

    return 0;
}
/*
 功能描述    : 点亮故障显示灯
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月28日 16:26:01
*/
int32_t CILedInt_LightFailure(void)
{
    led_set_bit(LED_FAILURE,&led_status);

    led_write_data(led_status);

    return 0;
}
/*
 功能描述    : 点亮故障显示灯
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月28日 16:26:01
*/
int32_t CILedInt_BlankingFailure(void)
{
    led_clear_bit(LED_FAILURE,&led_status);

    led_write_data(led_status);

    return 0;
}
/*
 功能描述    : 熄灭所有显示灯
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月28日 16:26:01
*/
int32_t CILedInt_BlankingAll(void)
{
    led_status = 0;

    led_write_data(led_status);

    return 0;
}
/*
 功能描述    : 初始化led灯
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月28日 15:52:11
*/
int32_t CILedInt_Init(void)
{
    static CI_BOOL b_initialized = CI_FALSE;
    int32_t ret = 0;

    if (CI_TRUE == b_initialized)
    {
        return -1;
    }

    /*led连在并口上面*/
    led_fd = open("/dev/parport0",O_WRONLY);
    if (-1 == led_fd)
    {
        CILog_Errno("open /dev/parport0 failed");
        return -1;
    }
    /* Claim the port to start using it */
    ret = ioctl(led_fd, PPCLAIM);
    if(-1 == ret)
    {
        CILog_Errno("PPCLAIM failed");
        close(led_fd);
        return -1;
    }

    /*点亮CPU运行灯*/
    led_set_bit(LED_CPU_RUN,&led_status);
    led_write_data(led_status);

    ret = CITimer_Regist(&led_timer);                   /*LED点灯*/

    b_initialized = CI_TRUE;

    return ret;
}

#endif /*!LINUX_ENVRIONMENT*/
