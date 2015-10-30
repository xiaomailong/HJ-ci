/***************************************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.
文件名:  throat_lock.c
作者:    WWB
版本 :   1.1	
创建日期:2011/12/27
用途:    引导总锁闭模块
历史修改记录: 
2012/8/7,V1.1，HJH:
	1.修改设置和清除锁闭标志函数
	2.修改了与引导进路相关的BUG
2013/9/2 V1.2.1 LYC
	1.command_throat_unlock_guide函数，增加了该函数的参数uint8_t throat，
	2.修改了该函数中引总解锁时仅关闭被引总解锁咽喉中的引导信号。
2013/9/4 V1.2.1 LYC
	1.修改了throat_lock函数中对已经引总锁闭的咽喉区再进行引总锁闭操作时提示信息错误的问题
	2.修改了throat_unlock函数中对未引总锁闭的咽喉区进行引总解锁操作时提示信息错误的问题

***************************************************************************************/

#include "utility_function.h"
#include "error_process.h"
#include "global_data.h"
#include "throat_lock.h"
/****************************************************
函数名:    throat_lock
功能描述:  引导总锁闭
返回值:    
参数:      throat   信号点所在的咽喉区
作者  :    WWB
日期  ：   2011/12/27
****************************************************/
void throat_lock(uint8_t throat)
{
	int16_t i = 0,j;
	CI_BOOL result = CI_TRUE;

    for (i = 0; i < TOTAL_SIGNAL_NODE; i++)
    {
        /*检查待锁闭咽喉道岔*/
        if ((gn_throat(i) == throat) && (gn_type(i) == NT_SWITCH))
        {
            /*检查待锁闭咽喉道岔未引总锁闭*/
            if (IsFALSE(is_node_locked(i, LT_SWITCH_THROAT_LOCKED)))
            {
                sn_locked_state(i, LT_SWITCH_THROAT_LOCKED);
            }
            else
            {
                result = CI_FALSE;
                break;
            }
        }
    }
	/*股道出岔*/
	for (i = 0; i < TOTAL_MIDDLLE_SWITCHES; i++)
	{
		for (j = 0; j < middle_switch_config[i].SwitchCount; j++)
		{			
			sn_locked_state(middle_switch_config[i].SwitchIndex[j], LT_SWITCH_THROAT_LOCKED);
			if (gn_another_switch(middle_switch_config[i].SwitchIndex[j]) != NO_INDEX)
			{
				sn_locked_state(gn_another_switch(middle_switch_config[i].SwitchIndex[j]), LT_SWITCH_THROAT_LOCKED);
			}
		}
	}

    if (IsTRUE(result))
    {
        CIHmi_SendDebugTips("咽喉区 %d 引总锁闭!", throat);
		CIHmi_SendNormalTips("引总锁闭：%s",hmi_throat_tips(throat));
        for (i = 0; i < TOTAL_SIGNAL_NODE; i++)
        {
            if ((throat == gn_throat(i)) && (NT_THROAT_GUIDE_LOCK == gn_type(i)))
            {
                sn_state(i, SIO_HAS_SIGNAL);
				break;
            }
        }
    }
    else
    {
        CIHmi_SendDebugTips("咽喉区 %d 已经引总锁闭!", throat);
		CIHmi_SendNormalTips("错误办理：%s已引总锁闭",hmi_throat_tips(throat));
    }
}

/****************************************************
函数名:    command_throat_unlock_guide
功能描述:  引总解锁时关闭引导信号
参数：		需关闭的信号机所在咽喉区
返回值:    
作者  :    hejh
日期  ：   2012/8/7
****************************************************/
void command_throat_unlock_guide(uint8_t throat)
{
	int16_t i = 0;

	/*关闭引导信号*/
	for (i = 0; i < TOTAL_SIGNAL_NODE; i++)
	{
		/*对信号点状态进行检查*/	/*2013/9/3 LYC 增加了仅关闭被引总解锁咽喉引导信号的条件*/
		if (((gn_type(i) == NT_ROUTE_SIGNAL) || (gn_type(i) == NT_ENTRY_SIGNAL)) && (gn_throat(i) == throat))
		{
			/*查看咽喉区引总锁闭状态*/
			if (IsTRUE(is_throat_lock(gn_throat(i))))
			{
				/*查看引导信号开放情况*/
				if ((gn_state(i) == SGS_YB) && (SGS_YB == gn_signal_expect_state(i)))
				{
					/*判断节点计时器是否在计时*/
					if (IsTRUE(is_node_timer_run(i)))
					{
						sn_stop_timer(i);
					}
					send_signal_command(i,SGS_H);
					sn_signal_expect_state(i,SGS_H);
					sn_signal_history_state(i,SGS_YB);
					CIHmi_SendDebugTips("%s 正在关闭引导信号！",gn_name(i));
				}
			}
			else
			{
				/*判断引导信号是否开放*/
				if (gn_state(i) == SGS_YB)
				{
					/*判断预期开放的是否为红灯*/
					if (SGS_H == gn_signal_expect_state(i))
					{
						/*错误计数3个周期*/
						/*检查引导进路是否计时*/
						if (IsTRUE(is_node_timer_run(i)))
						{
							/*是否完成计时*/
							if (IsTRUE(is_node_complete_timer(i)))
							{
								sn_signal_state(i,SGS_ERROR);
								sn_stop_timer(i);
								process_warning(ERR_CLOSE_SIGNAL,gn_name(i));
							}
						}
						else
						{
							sn_start_timer(i,MAX_ERROR_PER_COMMAND*CI_CYCLE_MS,DTT_ERROR);
						}
					}
				}
				/*判断引导信号是否关闭*/
				if ((gn_state(i) == SGS_H) && (gn_signal_history_state(i) == SGS_YB) && (gn_belong_route(i) == NO_ROUTE))
				{
					sn_stop_timer(i);
					sn_signal_history_state(i,SGS_H);
					CIHmi_SendDebugTips("%s 引导信号已关闭！",gn_name(i));
					CIHmi_SendNormalTips("引导信号关闭：%s",gn_name(i));
				}
			}
		}
	}
}

/****************************************************
函数名:    throat_unlock
功能描述:  引导总解锁
返回值:    
参数:      throat   信号点所在的咽喉区
作者  :    WWB
日期  ：   2011/12/27
****************************************************/
void throat_unlock(uint8_t throat)
{
	int16_t i,j,k;
	CI_BOOL result = CI_TRUE,lock_flag = CI_FALSE;

    command_throat_unlock_guide(throat);
    /*引总解锁道岔*/
    for (i = 0; i < TOTAL_SIGNAL_NODE; i++)
    {
        /*检查信号点所在咽喉是否引总锁闭*/
        if ((gn_throat(i) == throat) && (gn_type(i) == NT_SWITCH))
        {
            if (IsTRUE(is_node_locked(i, LT_SWITCH_THROAT_LOCKED)))
            {
                cn_locked_state(i, LT_SWITCH_THROAT_LOCKED);
            }
            else
            {
                result = CI_FALSE;
                break;
            }
        }
    }

	/*股道出岔*/
	for (i = 0; i < TOTAL_MIDDLLE_SWITCHES; i++)
	{
		for (j = 0; j < middle_switch_config[i].SwitchCount; j++)
		{
			for (k = 0; k < TOTAL_SIGNAL_NODE; k++)
			{
				/*检查信号点所在咽喉是否引总锁闭*/
				if ((gn_type(i) == NT_SWITCH) && (gn_throat(i) == gn_throat(middle_switch_config[i].SwitchIndex[j]))
					&& (i != middle_switch_config[i].SwitchIndex[j]))
				{					
					if (IsTRUE(is_node_locked(k, LT_SWITCH_THROAT_LOCKED)))
					{
						lock_flag = CI_TRUE;						
					}
					break;
				}
			}
			for (k = 0; k < TOTAL_SIGNAL_NODE; k++)
			{
				if ((gn_type(i) == NT_SWITCH) && (gn_throat(i) != gn_throat(middle_switch_config[i].SwitchIndex[j]))
					&& (i != middle_switch_config[i].SwitchIndex[j]))
				{					
					if (IsTRUE(is_node_locked(k, LT_SWITCH_THROAT_LOCKED)))
					{
						lock_flag = CI_TRUE;						
					}
					break;
				}
			}
			if (IsFALSE(lock_flag))
			{
				cn_locked_state(middle_switch_config[i].SwitchIndex[j], LT_SWITCH_THROAT_LOCKED);
				if (gn_another_switch(middle_switch_config[i].SwitchIndex[j]) != NO_INDEX)
				{
					cn_locked_state(gn_another_switch(middle_switch_config[i].SwitchIndex[j]), LT_SWITCH_THROAT_LOCKED);
				}
			}			
		}
	}

    if (IsTRUE(result))
    {
        CIHmi_SendDebugTips("咽喉区 %d 引总解锁!", throat);
		CIHmi_SendNormalTips("引总解锁：%s",hmi_throat_tips(throat));
		for (i = 0; i < TOTAL_SIGNAL_NODE; i++)
		{
			if ((throat == gn_throat(i)) && (NT_THROAT_GUIDE_LOCK == gn_type(i)))
			{
				sn_state(i, SIO_NO_SIGNAL);
				break;
			}
		}
    }
    else
    {
        CIHmi_SendDebugTips("咽喉区 %d 未引总锁闭!", throat);
		CIHmi_SendNormalTips("错误办理：%s未引总锁闭",hmi_throat_tips(throat));
    }
	
}

/****************************************************
函数名：   hmi_throat_tips
功能描述： 控显机咽喉提示
返回值：   char_t*
参数：     uint8_t throat
作者：	   hejh
日期：     2015/07/03
****************************************************/
char_t* hmi_throat_tips(uint8_t throat)
{
	char_t* result = "";
	char_t* throatName = "";
	int16_t i;

	for (i = 0; i < TOTAL_SIGNAL_NODE; i++)
	{
		if ((gn_type(i) == NT_THROAT_GUIDE_LOCK) && (gn_throat(i) == throat))
		{
			throatName = gn_name(i);
			break;
		}
	}	
	if (strcmp_no_case(throatName,"X-YDZS") == 0)
	{
		result = "下行咽喉";
	}
	else if (strcmp_no_case(throatName,"S-YDZS") == 0)
	{
		result = "上行咽喉";
	}
	return result;
}
