/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年10月15日 10:30:26
用途        : 故障检测
历史修改记录: v1.0    创建
**********************************************************************/
#include "util/ci_header.h"

#ifdef LINUX_ENVRIONMENT

#include "util/ci_header.h"
#include "util/log.h"

#include "failures_int.h"
#include "led_int.h"
#include "timer.h"

#define FAILURE_CYCLE_NANO 50000000      /*故障检测的周期，使用纳秒级，默认为50ms*/
#define FAILURE_CYCLE_RELATIVE ((int)(FAILURE_CYCLE_NANO / TIMER_CYCLE_NANO))  /*相对于主定时器的周期*/

static void failure_action(void);

static CI_BOOL failure_condition_check(uint32_t elapsed_cycle);

static const CITimer failure_timer = 
{ 
    FAILURE_CYCLE_RELATIVE,
    failure_action,
    failure_condition_check,
    "failure_timer",
    CI_TRUE,                        /*默认打开该定时器*/
};

/*故障信息*/
typedef struct _FailureInfo
{
    FailureType type;
    const char* msg;
    const char* function_name;
    int32_t line_num;
}FailureInfo;

#define FAILURE_MAX_RECORD 10
/*故障记录池*/
static FailureInfo failure_info_pool[FAILURE_MAX_RECORD];
static int32_t failure_count = 0;

static CI_BOOL b_terminate = CI_FALSE;
/*
 功能描述    : 故障处理定时函数
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年8月11日 14:14:30
*/
static void failure_action(void)
{
    int i = 0;

    /*在故障检测的时候清除故障显示灯，若没有故障则灯应该是灭的*/
    CILedInt_BlankingFailure();

    for (i = 0;i < failure_count;i++)
    {
        CILog_SigMsg("系统故障%d:%s(函数%s第%d行)",
            (int32_t)failure_info_pool[i].type,
            failure_info_pool[i].msg,
            failure_info_pool[i].function_name,
            failure_info_pool[i].line_num
            );

        switch (failure_info_pool[i].type)
        {
        case FAILURE_NONE:
            break;
        case FAILURE_EEU_CHANNEL_HALT:
        case FAILURE_CPU_INPUT_CMP_FAIL:
        case FAILURE_SERIES_INPUT_CMP_FAIL:
        case FAILURE_SWITCH_BOARD_COMMUNICATE:
            CILedInt_LightFailure();
            break;
        case FAILURE_DUAL_SERIES_CHANNEL_HALT:
            /*原本在这里应该实现双系通信道断开点亮故障灯的逻辑，但是，双系通信道断开
             *随即会发生切换，点亮故障灯无意义，另外判断双系通信道断开比较复杂
             */
            break;
        case FAILURE_SERIOUS:
        case FAILURE_INTERRUPT:
        case FAILURE_SYNRESULT:
        case FAILURE_EEU_STATUS_FAULT:
        case FAILURE_DUAL_MASTER_SERIES:
        case FAILURE_DUAL_STANDBY_SERIES:
        case FAILURE_DUAL_MASTER_CPU:
        case FAILURE_DUAL_SLAVE_CPU:
        case FAILURE_SERIES_HEARTBEAT_LOST:
            /*切换逻辑已迁移至状态机内，这里将备系停机即可*/
            b_terminate = CI_TRUE;
            /*点亮故障指示灯*/
            CILedInt_LightFailure();
            break;
        case FAILURE_MANUL:
            /*人工恢复故障*/
            break;
        default:
            break;
        }
    }

    /*reset counter*/
    failure_count = 0;

    return ;
}
/*
 功能描述    : 故障到来条件检查函数
 返回值      : 如果条件为真则返回CI_TRUE，否则返回CI_FALSE
 参数        : 无
 作者        : 张彦升
 日期        : 2014年8月11日 14:16:06
*/
static CI_BOOL failure_condition_check(uint32_t elapsed_cycle)
{
    if (CI_FALSE == failure_timer.b_open)
    {
        return CI_FALSE;
    }
    if (0 != elapsed_cycle % failure_timer.relative_cycle)
    {
        return CI_FALSE;
    }

    return CI_TRUE;
}

int32_t CIFailureInt_SetInfoExt(FailureType type,
                             const char* p_msg,
                             const char* p_function_name,
                             int32_t line_num)
{
    if (FAILURE_MAX_RECORD <= failure_count)
    {
        return -1;
    }

    failure_info_pool[failure_count].type = type;
    failure_info_pool[failure_count].msg = p_msg;
    failure_info_pool[failure_count].function_name = p_function_name;
    failure_info_pool[failure_count].line_num = line_num;

    failure_count ++;

    return 0;
}
/*
 功能描述    : 判读是否因故障而终止
 返回值      : 如果需终止则返回CI_TRUE，否则返回CI_FALSE
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月29日 16:09:59
*/
CI_BOOL CIFailureInt_BeTerminate(void)
{
    return b_terminate;
}
/*
功能描述    : 故障检测初始化
返回值      : 成功为0，失败为-1
参数        : 无
作者        : 张彦升
日期        : 2013年10月28日 15:21:58
*/
int32_t CIFailureInt_Init(void)
{
    static CI_BOOL b_initialized = CI_FALSE;
    int32_t ret = 0;

    if (CI_TRUE == b_initialized)
    {
        return -1;
    }

    ret = CITimer_Regist(&failure_timer);

    if (-1 == ret)
    {
        return ret;
    }

    b_initialized = CI_TRUE;

    return ret;
}

#endif /*!LINUX_ENVRIONMENT*/
