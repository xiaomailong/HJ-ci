/***************************************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.
文件名:  switch_single_lock.c
作者:    WWB
版本 :   1.2	
创建日期:2011/12/22
用途:    道岔单锁模块
历史修改记录: 
2012/7/5,V1.1，CY:
	1.修改了检查道岔锁闭和征用情况检查函数返回值永远为真的BUG
	2.修改了提示信息。根据实际情况将错误信息，警告信息，提示信息正确分类处理；修改了部分Bug
2012/8/7,V1.2，HJH:
	1.修改设置和清除锁闭标志函数
	2.修改了与引导进路相关的BUG
***************************************************************************************/
#include "switch_control.h"
#include "utility_function.h"
#include "error_process.h"
#include "global_data.h"
/****************************************************
函数名:    switch_single_lock
功能描述:  道岔单独锁闭
返回值:
参数:      index      待单锁道岔
           
作者  :    WWB
日期  ：   2011/12/23
****************************************************/
void switch_single_lock(int16_t index)
{
	int16_t another_switch = gn_another_switch(index);
	
	/*参数检查*/
	if ((index >= 0) && (index < TOTAL_ILT))
	{
		/*检查道岔未被征用*/
		if (IsTRUE(gn_used_state(index)))
		{
			process_warning(ERR_OPERATION,gn_name(index));
			return;
		}
		/*检查道岔未被单锁*/
		if (IsTRUE(is_node_locked(index,LT_SWITCH_SIGNLE_LOCKED)))
		{
			process_warning(ERR_SWITCH_SINGLE_LOCKED,gn_name(index));
			return;
		}
		/*判断该道岔不是否是双动道岔*/
		if (another_switch == NO_INDEX)
		{
			sn_locked_state(index,LT_SWITCH_SIGNLE_LOCKED);
			CIHmi_SendDebugTips("%s #道岔单锁!",gn_name(index));
		}
		else
		{
			/*检查双动道岔另一动未被征用*/
			if (IsTRUE(gn_used_state(another_switch)))
			{
				process_warning(ERR_OPERATION,gn_name(another_switch));
				return;
			}
			/*检查双动道岔另一动未被单锁*/
			if (IsTRUE(is_node_locked(another_switch,LT_SWITCH_SIGNLE_LOCKED)))
			{
				process_warning(ERR_SWITCH_SINGLE_LOCKED,gn_name(another_switch));
				return;
			}
			sn_locked_state(index,LT_SWITCH_SIGNLE_LOCKED);
			sn_locked_state(another_switch,LT_SWITCH_SIGNLE_LOCKED);
			CIHmi_SendDebugTips("%s #道岔单锁!",gn_name(index));
		}	
	}
}

/****************************************************
函数名:    switch_single_unlock
功能描述:  道岔单独解锁
返回值:    
参数:      index      待解锁道岔
           
作者  :    WWB
日期  ：   2011/12/22
****************************************************/
void switch_single_unlock(int16_t index)
{ 
	int16_t another_switch = 0;
	another_switch = gn_another_switch(index);

	FUNCTION_IN;
	/*参数检查*/
	if ((index >= 0) && (index < TOTAL_ILT))
	{
		/*检查道岔处于单锁状态*/
		if (IsTRUE(is_node_locked(index,LT_SWITCH_SIGNLE_LOCKED)))
		{   
			/*检查道岔为单动道岔时清除单锁标志*/
			if (another_switch == NO_INDEX)
			{ 
				cn_locked_state(index,LT_SWITCH_SIGNLE_LOCKED);
				CIHmi_SendDebugTips("%s #道岔解除单锁!",gn_name(index));
			}
			/*检查道岔为双动道岔的情况*/
			else  /*另一动也处于单锁状态时清除这一组道岔的单锁标志*/
			{      
				if (IsTRUE(is_node_locked(another_switch,LT_SWITCH_SIGNLE_LOCKED)))
				{   
					cn_locked_state(index,LT_SWITCH_SIGNLE_LOCKED);
					cn_locked_state(another_switch,LT_SWITCH_SIGNLE_LOCKED);
					CIHmi_SendDebugTips("%s #道岔解除单锁!",gn_name(index));
				}
				else  /*提示错误信息*/
				{   
					CIHmi_SendDebugTips("%s #道岔未单锁!",gn_name(another_switch));
				}
			}
		}
		else
		{
			CIHmi_SendDebugTips("%s #道岔未单锁!",gn_name(index));
		}
	}	
    FUNCTION_OUT;
}