/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年10月11日 10:06:57
用途        : 
历史修改记录: 
**********************************************************************/
#include "util/ci_header.h"

#ifdef LINUX_ENVRIONMENT

#include "util/app_config.h"
#include "util/ci_header.h"
#include "util/algorithms.h"
#include "util/log.h"

#include "timer.h"
#include "performance.h"

#define MAX_REGIST_TIMER_NUM 32             /*可供注册的定时器的最大个数，默认为32个*/
#define CLOCKID CLOCK_REALTIME              /*定时中断的类型，默认为实时中断*/

static const CITimer* ci_timers[MAX_REGIST_TIMER_NUM];

static CI_BOOL b_print_timer = CI_FALSE;    /*是否打印定时器输出信息*/
static int32_t timer_counter;              /*存储所有已经注册了的定时器的个数*/
static uint32_t timer_freq_lcm = 1;         /*所有定时器频率的最小公倍数*/
static uint32_t elapsed_cycle = 0;          /*记录定时周期*/
static timer_t timerid;                     /*POSIX定时器标识符*/
static uint32_t system_start_counter = 0;   /*系统启动计数*/

/*
 功能描述    : 通过该函数向系统注册新的定时器
 返回值      : 0表示成功，-1表示失败
 参数        : ci_timer新的定时器，这里传入的是定时器的位置指针，请确保该指针是静态常量的
              从而保证系统当中只有一份
 作者        : 
 日期        : 2013年10月11日 10:13:41
*/
int32_t CITimer_Regist(const CITimer* p_ci_timer)
{
    int32_t i = 0;

    if (NULL == p_ci_timer)
    {
        return -1;
    }

    if (timer_counter >= MAX_REGIST_TIMER_NUM)
    {
        CILog_Msg("本系统最多支持%d个定时器，不能容纳更多",MAX_REGIST_TIMER_NUM);
        return -1;
    }
    for (i = 0;i < timer_counter;i++)
    {
        if (ci_timers[i] == p_ci_timer)
        {
            CILog_Msg("对不起！您已注册过该定时器%s",p_ci_timer->name);
            return -1;
        }
    }
    ci_timers[timer_counter++] = p_ci_timer;

    timer_freq_lcm = CIAlgorithm_Lcm(timer_freq_lcm,p_ci_timer->relative_cycle);

    return 0;
}
/*
 功能描述    :每次定时器到来时的动作
             由于该函数由信号触发，请确保在该函数当中使用异步安全函数。
             对于定时周期使用uint32_t类型时需要将近49.7天的时间才会轮转一次。
             note！:该函数是核心函数，十分有必要写相应单元测试代码。
             在该函数当中切记不能直接或者间接调用别人写的任何代码，glibc的也不行，所实现
             功能一律直接使用系统调用，另外该函数内部直接或间接调用的函数不能为其它模块
             使用，如CILog_Msg函数，若在状态机内部调用该函数打印信息，在此函数内部也同样
             使用该函数打印信息，将极有可能造成死锁，因为glibc内部使用了futex等多项同步
             机制。
 返回值      : 
 参数        : 
 作者        : 张彦升
 日期        : 2013年10月11日 11:26:13
*/
static void timer_action(int32_t UNUSED(sig), siginfo_t *UNUSED(si), void *UNUSED(uc))
{
    register int32_t i = 0;

    if (system_start_counter < 500)
    {
        system_start_counter++;
    }

    elapsed_cycle++;   /*该步骤保证其不可能为0，当为0时下面的代码会出现问题*/
    /*该函数用来检测中断周期*/
    CIPerformance_SigSend(PDT_INTRRUPT_BEGIN);
    if (CI_TRUE == b_print_timer)
    {
        CILog_SigMsg("timer_enter:%#x",elapsed_cycle);
    }

    for (i = 0;i < timer_counter;i++)
    {
        assert(0 != ci_timers[i]->relative_cycle);

        if (CI_TRUE == ci_timers[i]->condition_check(elapsed_cycle))
        {
            if (CI_TRUE == b_print_timer)
            {
                CILog_SigMsg("%s:%#x",ci_timers[i]->name,elapsed_cycle);
            }

            ci_timers[i]->action();
        }
    }

    if (CI_TRUE == b_print_timer)
    {
        CILog_SigMsg("timer_out:%#x",elapsed_cycle);
    }

    elapsed_cycle %= ((UINT_MAX / timer_freq_lcm) * timer_freq_lcm);   /*循环周期，以防溢出*/

    return ;
}

/*
 功能描述    : 使用getter封装SIG_CI_TIMER信号的处理函数
 返回值      : 与sigactionhandler_t相同类型的函数指针
 参数        : 
 作者        : 张彦升
 日期        : 2013年10月12日 14:43:30
*/
sigactionhandler_t CITimer_GetHandler(void)
{
    return (sigactionhandler_t)timer_action;
}
/*
 功能描述    : 得到流逝的周期数
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2013年11月27日 8:55:09
*/
uint32_t CITimer_GetElapsedCycles(void)
{
    return elapsed_cycle;
}
/*
 功能描述    : 跟新周期号，这里一定要注意向前调整和向后调整的问题：
              向前调整：因为前几个周期已经触发过响应的定时器，所以导致部分工作会重复。
              向后调整，因为中间漏了几个周期可能会出现部分工作没有做
              实际当中只有不是50ms的定时器才会出现此类问题（看门狗，联锁周期），联锁
              周期检查定时器函数内部做了这类的处理，看门狗每隔1s一次，而其超时时间为
              60s，我们简单的认为不可能60次都被错过
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年8月5日 13:00:03
*/
void CITimer_SetElapsedCycles(uint32_t t_cycle)
{
#if 0
    if (CI_TRUE == b_print_timer)
    {
        CILog_SigSafeMsg("更新中断周期号from:%x->%x",elapsed_cycle,t_cycle);
    }
#endif

    elapsed_cycle = t_cycle;
}
/*
 功能描述    : 得到系统启动计数
 返回值      : 
 参数        : 
 作者        : 何境泰
 日期        : 2014年5月20日 16:14:46
*/
uint32_t CITimer_GetSystemStartCounter(void)
{
    return system_start_counter;
}
/*
 功能描述     : 设置系统系统计数
 返回值       : 
 参数         : 
 作者         : 何境泰
 日期         : 2014年6月5日 14:21:15
*/
void CITimer_ResetSystemStartCounter(void)
{
    system_start_counter = 0;
}
/*
 功能描述    : 打开定时器输出
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年4月11日 15:22:32
*/
int CITimer_OpenPrintTimer(void)
{
    b_print_timer = CI_TRUE;

    return 0;
}
/*
 功能描述    : 启动定时器
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年5月5日 18:35:46
*/
int CITimer_Start(void)
{
    struct itimerspec its;

    /*设置定时器*/
    its.it_value.tv_sec = TIMER_CYCLE_NANO / NANO_ONE_SECOND;
    its.it_value.tv_nsec = TIMER_CYCLE_NANO % NANO_ONE_SECOND;
    its.it_interval.tv_sec = its.it_value.tv_sec;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;
    if (-1 == timer_settime(timerid, 0, &its, NULL))
    {
        CILog_Errno("设置定时器失败");

        return -1;
    }
    return 0;
}
/*
 功能描述    : 因为时间周期是在所有周期的最小公倍数的基础之上做余运算循环进行的，所以要
              计算两个周期的差需要特殊做处理，因此提供该函数，该函数不会返回负数，也就是
              其返回的是两个余数之差的绝对值
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年7月10日 9:57:18
*/
uint32_t CITimer_RoundSub(uint32_t a,uint32_t b)
{
    uint32_t min_val = MIN(a,b);
    uint32_t max_val = MAX(a,b);

    uint32_t actual_max_freq = ((UINT_MAX / timer_freq_lcm) * timer_freq_lcm);

    if (max_val - min_val > actual_max_freq / 2)
    {
        return (min_val + actual_max_freq - max_val) % actual_max_freq;
    }
    else
    {
        return max_val - min_val;
    }
    return 0;
}
/*
 功能描述    : 初始化定时器
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2013年10月11日 11:05:49
*/
int CITimer_Init(void)
{
    static CI_BOOL b_initialized = CI_FALSE;

    struct sigevent sev;

    if (CI_TRUE == b_initialized)
    {
        return -1;
    }

    /*创建定时器*/
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIG_CI_TIMER;
    sev.sigev_value.sival_ptr = &timerid;

    if (-1 == timer_create(CLOCKID, &sev, &timerid))
    {
        CILog_Errno("创建定时器失败");
        return -1;
    }

    b_initialized = CI_TRUE;

    return 0;
}

#endif /*!LINUX_ENVRIONMENT*/
