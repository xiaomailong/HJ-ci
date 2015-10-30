/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年10月15日 10:29:03
用途        : 双系心跳管理
历史修改记录: v1.0    创建
**********************************************************************/
#include "util/ci_header.h"

#ifdef LINUX_ENVRIONMENT

#include "util/ci_header.h"
#include "util/log.h"

#include "series_heartbeat_int.h"
#include "series_manage.h"
#include "failures_int.h"
#include "timer.h"
#include "cpu_manage.h"

#define SERIES_CYCLE_NANO 50000000      /*双系心跳的周期，使用纳秒级，默认为50ms*/
#define SERIES_CYCLE_RELATIVE ((int)(SERIES_CYCLE_NANO / TIMER_CYCLE_NANO))  /*相对于主定时器的周期*/

static void series_heartbeat_action(void);

static CI_BOOL series_heartbeat_condition_check(uint32_t elapsed_cycle);

static const CITimer series_timer = 
{
    SERIES_CYCLE_RELATIVE,
    series_heartbeat_action,
    series_heartbeat_condition_check,
    "series_timer",
    CI_TRUE,                        /*默认打开该定时器*/
};

/*接受延迟计数，当接受不成功时递增*/
static uint32_t recv_delay_count = 0;
/*最大延迟次数*/
static const uint32_t max_delay_count = 3;
/*
 功能描述    : 双系心跳信号同步函数，主备系的两个CPU都彼此向外发送数据
 返回值      : 无
 参数        : 
 作者        : 张彦升
 日期        : 2014年8月7日 15:08:53
*/
static void series_heartbeat_action(void)
{
    int count = 0;

    if (SERIES_PENDING == CISeries_GetLocalState())
    {
        CILog_SigMsg("系统处于SERIES_PENDING状态");
        return;
    }

    /*主CPU发送*/
    if (CPU_STATE_MASTER == CICpu_GetLocalState())
    {
        /*尝试10次，避免发送不成功而造成的心跳信号丢失*/
        while (10 > count)
        {
            if (-1 == CISeries_SendHeartbeat())
            {
                count ++;
            }
            else
            {
                break;
            }
        }
    }
    /*四个CPU都接收双系心跳信号从而知道对方状态*/
    if (-1 == CISeries_RecvHeartbeat())
    {
        recv_delay_count ++;
    }
    else
    {
        /*reset*/
        recv_delay_count = 0;
        CISeries_CheckHeartbeat();
    }

    return;
}

static CI_BOOL series_heartbeat_condition_check(uint32_t elapsed_cycle)
{
    if (CI_FALSE == series_timer.b_open)
    {
        return CI_FALSE;
    }
    if (0 != elapsed_cycle % series_timer.relative_cycle)
    {
        return CI_FALSE;
    }

    return CI_TRUE;
}
/*
 功能描述    : 心跳信号是否丢失，表示通讯已中断
 返回值      : 心跳信号丢失则返回CI_TRUE，否则返回CI_FALSE
 参数        : 无
 日期        : 2015年5月29日 19:37:39
*/
CI_BOOL CISeriesHeartbeatInt_IsLost(void)
{
    /*若心跳信号在3个周期内都未能收到信号则认为心跳丢失*/
    if (recv_delay_count > max_delay_count)
    {
        return CI_TRUE;
    }
    return CI_FALSE;
}
/*
功能描述    : 双系心跳初始化
返回值      : 
参数        : 
作者        : 张彦升
日期        : 2013年10月22日 12:49:55
*/
int32_t CISeriesHeartbeatInt_Init(void)
{
    static CI_BOOL b_initialized = CI_FALSE;
    int32_t ret = 0;

    if (CI_TRUE == b_initialized)
    {
        return -1;
    }

    ret = CITimer_Regist(&series_timer);                /*双系心跳*/

    b_initialized = CI_TRUE;

    return ret;
}

#endif /*!LINUX_ENVRIONMENT*/
