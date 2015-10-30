/***************************************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.
文件名:  switch_close_up.c
作者:    WWB
版本 :   1.2	
创建日期:2011/12/23
用途:    道岔封锁模块
历史修改记录:  
2012/8/7,V1.2，HJH:
	1.修改设置和清除锁闭标志函数
	2.修改了引导进路相关的BUG
***************************************************************************************/
#include "utility_function.h"
#include "error_process.h"
#include "switch_control.h"
#include "global_data.h"
/****************************************************
函数名:    switch_close_up
功能描述:  道岔封锁
返回值:    
参数:      index      待封锁道岔
      
作者  :    WWB
日期  ：   2011/12/23
****************************************************/
void switch_close_up(int16_t index)
{
	int16_t another_switch = gn_another_switch(index);

	/*参数检查*/
	if (((index >= 0) && (index < TOTAL_ILT)))
	{
		/*检查待封锁道岔未被征用*/
		if (IsTRUE(gn_used_state(index)))
		{
			process_warning(ERR_OPERATION,gn_name(index));
			return;
		}
		/*检查待封锁道岔未被锁闭*/
		if(IsTRUE(is_node_locked(index,LT_SWITCH_CLOSED)))
		{
			process_warning(ERR_SWITCH_LOCKED,gn_name(index));
			return;
		}
		/*检查待封锁道岔是否是双动道岔*/
		if (another_switch == NO_INDEX)
		{
			sn_locked_state(index,LT_SWITCH_CLOSED);
			CIHmi_SendDebugTips("%s #道岔封锁!",gn_name(index));
		}
		else
		{
			/*检查双动道岔另一动是否被征用*/
			if (IsTRUE(gn_used_state(another_switch)))
			{
				process_warning(ERR_OPERATION,gn_name(another_switch));
				return;
			}
			/*检查双动道岔另一动是否被锁闭*/
			if(IsTRUE(is_node_locked(another_switch,LT_SWITCH_CLOSED)))
			{
				process_warning(ERR_SWITCH_LOCKED,gn_name(another_switch));
				return;
			}
			sn_locked_state(index,LT_SWITCH_CLOSED);
			sn_locked_state(another_switch,LT_SWITCH_CLOSED);
			CIHmi_SendDebugTips("%s #道岔封锁!",gn_name(index));
		}	
	}
	
}

/****************************************************
函数名:    switch_unclose_up
功能描述:  道岔解封
返回值:    
参数:      index      待解封道岔
      
作者  :    WWB
日期  ：   2011/12/22
****************************************************/
void switch_unclose_up(int16_t index)
{ 
	int16_t another_switch = gn_another_switch(index);

	FUNCTION_IN;
	/*参数检查*/
	if ((index >= 0) && (index < TOTAL_ILT))
	{
		/*检查道岔处于封锁状态*/
		if (IsTRUE(is_node_locked(index,LT_SWITCH_CLOSED)))
		{
			/*检查道岔为单动道岔时清除封锁标志*/
			if (another_switch == NO_INDEX)
			{
				cn_locked_state(index,LT_SWITCH_CLOSED);
				CIHmi_SendDebugTips("%s #道岔解除封锁!",gn_name(index));
			}
				  /*检查道岔为双动道岔的情况*/
			else  /*另一动也处于封锁状态时同时清除这一组道岔的封锁标志*/
			{
				if (IsTRUE(is_node_locked(another_switch,LT_SWITCH_CLOSED)))
				{
					cn_locked_state(index,LT_SWITCH_CLOSED);
					cn_locked_state(another_switch,LT_SWITCH_CLOSED);
					CIHmi_SendDebugTips("%s #道岔解除封锁!",gn_name(index));
				}
				else  /*提示错误信息*/
				{
					CIHmi_SendDebugTips("%s #道岔未封锁!",gn_name(another_switch));
				}
			}
		}
		else
		{
			CIHmi_SendDebugTips("%s #道岔未封锁!",gn_name(index));
		}
	}
    
	FUNCTION_OUT;
}