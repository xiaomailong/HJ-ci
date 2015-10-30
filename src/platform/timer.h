/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.1
创建日期    : 2013年10月11日 9:36:29
用途        : 在平台软件当中使用统一定时中断系统，其它所有定时中断全部在该中断
             基础之上模拟，另外该定时中断是通过信号模拟的软件中断。
历史修改记录: 
    v1.2    将主定时器的周期设置为50ms
**********************************************************************/

#ifndef _ci_timer_h__
#define _ci_timer_h__

#include "util/ci_header.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TIMER_CYCLE_NANO 50000000      /*主定时中断的周期，使用纳秒级，默认为50ms*/
#define NANO_ONE_SECOND  1000000000     /*1s代表1000000000纳秒，该宏为时间转换保留*/
#define NANO_ONE_MILSECOND  1000000     /*1ms代表1000000纳秒，该宏为时间转换保留*/

typedef struct _CITimer 
{
    uint16_t relative_cycle;                    /*注册定时中断的周期，该频率相对于主
                                                  定时中断，如50ms的定时中断变为50，
                                                  不应为50*1000000*/
    void (*action)(void);                       /*当定时中断到来时触发的动作*/
    CI_BOOL (*condition_check)(uint32_t elapsed_cycle);    /*检查条件函数*/
    const char* name;                           /*本定时中断的名称*/
    CI_BOOL b_open;                             /*是否打开，没有打开的话不执行action*/
} CITimer;

/*
功能描述    : 通过该函数向系统注册新的定时器
返回值      : 0表示成功，-1表示失败
参数        : ci_timer新的定时器，这里传入的是定时器的位置指针，请确保该指针是静态常量的
             从而保证系统当中只有一份
作者        : 
日期        : 2013年10月11日 10:13:41
*/
CIAPI_FUNC(int) CITimer_Regist(const CITimer *p_ci_timer);

/*
功能描述    : 使用getter封装SIG_CI_TIMER信号的处理函数
返回值      : 与sigactionhandler_t相同类型的函数指针
参数        : 
作者        : 张彦升
日期        : 2013年10月12日 14:43:30
*/
CIAPI_FUNC(sigactionhandler_t) CITimer_GetHandler(void);
/*
功能描述    : 得到流逝的周期数
返回值      : 成功为0，失败为-1
参数        : 
作者        : 张彦升
日期        : 2013年11月27日 8:55:09
*/
CIAPI_FUNC(uint32_t) CITimer_GetElapsedCycles(void);

CIAPI_FUNC(void) CITimer_SetElapsedCycles(uint32_t cycle);
/*
功能描述    : 得到系统启动计数
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2014年5月20日 16:14:46
*/
CIAPI_FUNC(uint32_t) CITimer_GetSystemStartCounter(void);
/*
功能描述     : 设置系统系统计数
返回值       : 
参数         : 
作者         : 何境泰
日期         : 2014年6月5日 14:21:15
*/
CIAPI_FUNC(void) CITimer_ResetSystemStartCounter(void);
/*
 功能描述    : 打开定时器信息输出
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年4月11日 15:21:55
*/
CIAPI_FUNC(int32_t) CITimer_OpenPrintTimer(void);
/*
 功能描述    : 启动定时器
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年5月5日 18:35:46
*/
CIAPI_FUNC(int32_t) CITimer_Start(void);
/*
 功能描述    : 因为时间周期是在所有周期的最小公倍数的基础之上做余运算循环进行的，所以要
              计算两个周期的差需要特殊做处理，因此提供该函数
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年7月10日 9:57:18
*/
CIAPI_FUNC(uint32_t) CITimer_RoundSub(uint32_t a,uint32_t b);
/*
功能描述    : 初始化定时器
返回值      : 
参数        : 
作者        : 
日期        : 2013年10月11日 11:05:49
*/
CIAPI_FUNC(int32_t) CITimer_Init(void);

#ifdef __cplusplus
}
#endif

#endif /*!_ci_timer_h__*/