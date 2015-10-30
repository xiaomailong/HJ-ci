/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年10月11日 10:51:14
用途        : 
历史修改记录: 
**********************************************************************/
#include "util/ci_header.h"

#ifdef LINUX_ENVRIONMENT

#include "cpu_heartbeat_int.h"
#include "timer.h"
#include "failures_int.h"
#include "cpu_manage.h"

#define CPU_HEARTBEAT_CYCLE_NANO 50000000      /*CPU心跳的周期，使用纳秒级，默认为50ms*/
#define CPU_HEARTBEAT_CYCLE_RELATIVE ((int)(CPU_HEARTBEAT_CYCLE_NANO / TIMER_CYCLE_NANO))  /*相对于主定时器的周期*/

static void cpu_heartbeat_action(void);

static CI_BOOL cpu_heartbeat_condition_check(uint32_t elapsed_cycle);

static const CITimer cpu_timer = {
    CPU_HEARTBEAT_CYCLE_RELATIVE,
    cpu_heartbeat_action,
    cpu_heartbeat_condition_check,
    "cpu_timer",
    CI_TRUE,                        /*默认打开该定时器*/
};

/*接受延迟计数，当接受不成功时递增*/
static uint32_t recv_delay_count = 0;
/*最大延迟次数*/
static const uint32_t max_delay_count = 3;
/*
功能描述    : CPU心跳中断触发的函数
             这里我们需要讨论一个问题，使用非阻塞发送，读不到数据的可能性有多大？这个问
             题的概率有多大，它决定了我们的程序的正确运行
             遗留，请估计该部分程序的执行时间。
返回值      : 成功为0，失败为-1
参数        : 无
作者        : 张彦升
日期        : 2013年10月28日 14:33:39
*/
static void cpu_heartbeat_action(void)
{
    int count = 0;

    /*尝试10次，避免发送不成功而造成的心跳信号丢失*/
    while (10 > count)
    {
        if (-1 == CICpu_SendHeartbeat())
        {
            count ++;
        }
        else
        {
            break;
        }
    }
    
    if (-1 == CICpu_RecvHeartbeat())
    {
        recv_delay_count ++;
    }
    else
    {
        /*reset*/
        recv_delay_count = 0;

        /*这里不做返回值检查，在该函数内部若发现故障则直接设置故障标志*/
        CICpu_CheckHearbeat();
    }

    /*若心跳信号在3个周期内都未能收到信号则认为其是严重故障*/
    if (recv_delay_count > max_delay_count)
    {
        CIFailureInt_SetInfo(FAILURE_SERIOUS,"连续3个周期内未成功收到心跳信号");
        recv_delay_count = 0;
    }

    return;
}

static CI_BOOL cpu_heartbeat_condition_check(uint32_t elapsed_cycle)
{
    if (CI_FALSE == cpu_timer.b_open)
    {
        return CI_FALSE;
    }
    if (0 != elapsed_cycle % cpu_timer.relative_cycle)
    {
        return CI_FALSE;
    }

    return CI_TRUE;
}
/*
功能描述    : CPU心跳初始化
返回值      : 成功为0，失败为-1
参数        : 误
作者        : 张彦升
日期        : 2013年10月17日 11:03:34
*/
int32_t CICpuHeartbeatInt_init(void)
{
    static CI_BOOL b_initialized = CI_FALSE;
    int32_t ret = 0;

    if (CI_TRUE == b_initialized)
    {
        return -1;
    }

    ret = CITimer_Regist(&cpu_timer);                   /*CPU心跳*/

    b_initialized = CI_TRUE;

    return ret;
}

#endif /*!LINUX_ENVRIONMENT*/
