/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年10月15日 10:29:44
用途        : 通信状态管理
历史修改记录: v1.0    创建
**********************************************************************/
#include "util/ci_header.h"

#ifdef LINUX_ENVRIONMENT

#include "communicate_state_int.h"
#include "timer.h"
#include "failures_int.h"
#include "util/log.h"

#define COMMUNICATE_STATE_CYCLE_NANO 50000000      /*通信状态的周期，使用纳秒级，默认为50ms*/
#define COMMUNICATE_STATE_CYCLE_RELATIVE ((int)(COMMUNICATE_STATE_CYCLE_NANO / TIMER_CYCLE_NANO))  /*相对于主定时器的周期*/

static void communicate_state_action(void);

static CI_BOOL communicate_state_condition_check(uint32_t elapsed_cycle);

static const CITimer communicate_state_timer = 
{
    COMMUNICATE_STATE_CYCLE_RELATIVE,
    communicate_state_action,
    communicate_state_condition_check,
    "communicate_state_timer",
    CI_FALSE,                        /*默认打开该定时器*/
};

static CommState communicate_state = CS_NONE;
static const char* p_communicate_state_msg = NULL;
static const char* p_communicate_state_function_name = NULL;
static int32_t communicate_state_line_num = 0;

static void communicate_state_action(void)
{
    if (CS_NONE == communicate_state)
    {
        return;
    }
    /*CILog_SigSafeMsg("系统通信中断故障,故障号[%d]。",(int32_t)communicate_state);*/

    /*联锁机与电子单元之间通信状态检测*/
    if((communicate_state & CS_EEU_CHANNEL_A_INT) == CS_EEU_CHANNEL_A_INT)
    {
        /*CILog_SigSafeMsg("联锁机和电子执行单元a路通信中断");*/
    }

    if((communicate_state & CS_EEU_CHANNEL_B_INT) == CS_EEU_CHANNEL_B_INT)
    {
        /*CILog_SigSafeMsg("联锁机和电子执行单元b路通信中断");*/
    }

    if ((communicate_state & (CS_EEU_CHANNEL_A_INT|CS_EEU_CHANNEL_B_INT)) == (CS_EEU_CHANNEL_A_INT|CS_EEU_CHANNEL_B_INT))
    {
        CIFailureInt_SetInfo(FAILURE_EEU_CHANNEL_HALT,"电子单元通信信道断开连接");
    }

    /*联锁机与切换器之间通信状态检测*/
    if((communicate_state & CS_UART_CHANNEL_A_INT) == CS_UART_CHANNEL_A_INT)
    {
        CILog_SigMsg("联锁机和切换器之间a路通信中断");

    }

    if((communicate_state & CS_UART_CHANNEL_B_INT) == CS_UART_CHANNEL_B_INT)
    {
        CILog_SigMsg("联锁机和切换器之间b路通信中断");	
    }

    if ((communicate_state & (CS_UART_CHANNEL_A_INT|CS_UART_CHANNEL_B_INT)) == (CS_UART_CHANNEL_A_INT|CS_UART_CHANNEL_B_INT))
    {
        CIFailureInt_SetInfo(FAILURE_DUAL_SERIES_SWITCH_HALT,"双系切换版通信信道断开连接,无法进行双系切换");
    }

    /*控显机和联锁机之间通信状态检测*/
    if((communicate_state & CS_HMI_CHANNEL_A_INT) == CS_HMI_CHANNEL_A_INT)
    {
        CILog_SigMsg("控显机和联锁机之间a路通信中断");

    }

    if((communicate_state & CS_HMI_CHANNEL_B_INT) == CS_HMI_CHANNEL_B_INT)
    {
        CILog_SigMsg("控显机和联锁机之间b路通信中断");	
    }

    if ((communicate_state & (CS_HMI_CHANNEL_A_INT|CS_HMI_CHANNEL_B_INT)) == (CS_HMI_CHANNEL_A_INT|CS_HMI_CHANNEL_B_INT))
    {
        CIFailureInt_SetInfo(FAILURE_HMI_CHANNEL_HALT,"控显机通信信道断开连接");
    }

    /*双系通信状态检测*/
    if((communicate_state & CS_FIBER_CHANNEL_A_INT) == CS_FIBER_CHANNEL_A_INT)
    {
        CILog_SigMsg("双系之间a路通信中断");
    }

    if((communicate_state & CS_FIBER_CHANNEL_B_INT) == CS_FIBER_CHANNEL_B_INT)
    {
        CILog_SigMsg("双系之间b路通信中断");	
    }

    if ((communicate_state & (CS_FIBER_CHANNEL_A_INT|CS_FIBER_CHANNEL_B_INT)) == (CS_FIBER_CHANNEL_A_INT|CS_FIBER_CHANNEL_B_INT))
    {
        CIFailureInt_SetInfo(FAILURE_DUAL_SERIES_CHANNEL_HALT,"双系通信信道断开连接");
    }

    return;
}

static CI_BOOL communicate_state_condition_check(uint32_t elapsed_cycle)
{
    if (CI_FALSE == communicate_state_timer.b_open)
    {
        return CI_FALSE;
    }
    if (0 != elapsed_cycle % communicate_state_timer.relative_cycle)
    {
        return CI_FALSE;
    }
    return CI_TRUE;
}

int32_t CICommunicateStateInt_Init(void)
{
    static CI_BOOL b_initialized = CI_FALSE;
    int32_t ret = 0;

    if (CI_TRUE == b_initialized)
    {
        return -1;
    }

    ret = CITimer_Regist(&communicate_state_timer);     /*通信状态*/

    b_initialized = CI_TRUE;

    return ret;
}

int32_t CICommunicateStateInt_SetInfoExt(CommState state,
                                         const char* p_msg,
                                         const char* p_function_name,
                                         int32_t line_num)
{	
	/*如果该通信故障状态已经设置，不做处理*/
	if (state == (communicate_state & state))
	{
		return 0;
	}
    communicate_state = communicate_state | state;
    p_communicate_state_msg = p_msg;
    p_communicate_state_function_name = p_function_name;
    communicate_state_line_num = line_num;

    CILog_Msg("系统通信中断故障,故障号[%d]:%s(函数%s第%d行)",
        (int32_t)state,
        p_communicate_state_msg,
        p_communicate_state_function_name,
        communicate_state_line_num);

    return 0;
}

CIAPI_FUNC(int32_t) CICommunicateStateInt_RecoverInfoExt(CommState state,
                                                         const char* msg,
                                                         const char* function_name,
                                                         int32_t line_num)
{
    communicate_state = communicate_state & (~state);
	p_communicate_state_msg = msg;
	p_communicate_state_function_name = function_name;
	communicate_state_line_num = line_num;
    CILog_Msg("通信线路恢复[%d]:%s(函数%s第%d行)",
		(int32_t)state,
		p_communicate_state_msg,
		function_name,
		communicate_state_line_num);

    return 0;
}

int32_t CICommunicateStateInt_SetState(CommState state)
{
    communicate_state = communicate_state | state;
    return 0;
}

/*
功能描述    : 去除该通信故障
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2014年4月28日 9:26:20
*/
int32_t CICommunicateStateInt_RecoverState(CommState state)
{
    communicate_state = communicate_state & (~state);
    return 0;
}

#endif /*!LINUX_ENVRIONMENT*/
