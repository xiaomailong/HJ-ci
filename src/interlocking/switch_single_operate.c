/***************************************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.
文件名:  switch_single_operate.c
作者:    WWB
版本 :   1.2	
创建日期:2011/12/23
用途:    道岔单独操纵模块
历史修改记录:  
2012/8/1,V1.2，HJH:
	1.修改锁闭标志，及清除锁闭标志的代码
***************************************************************************************/

#include "switch_control.h"
#include "check_ci_condition.h"
#include "utility_function.h"
#include "error_process.h"
#include "global_data.h"
/****************************************************
函数名：   check_switch_condition
功能描述： 检查道岔单操的条件
返回值：   CI_BOOL
参数：     int16_t node_index
作者：	   hejh
日期：     2014/06/03
****************************************************/
CI_BOOL check_switch_condition(int16_t node_index);

/****************************************************
函数名:    switch_single_operate
功能描述:  道岔单独操纵
返回值:    
参数:      index: 单独操纵的道岔
			need_location：道岔要求转换的位置
作者  :    WWB
日期  ：   2011/12/23
****************************************************/
void switch_single_operate(int16_t index, EN_switch_state need_location)
{ 
	int16_t another_switch = gn_another_switch(index);
	CI_BOOL error_flag = CI_FALSE; 

	FUNCTION_IN;
	/*参数检查*/
	if (((index >= 0) && (index < TOTAL_ILT))
		&& ((SWS_NORMAL == need_location) || (SWS_REVERSE == need_location)))
	{
		/*道岔实际位置与需要的位置一致时退出，不一致则判断其它条件*/
		if (gn_switch_state(index) == need_location)
		{
			CIHmi_SendDebugTips("%s#道岔实际位置与要求位置一致!",gn_name(index));
			CIHmi_SendNormalTips("道岔实际位置与要求位置一致：%s",gn_name(index));
			error_flag = CI_TRUE;
		}
		
		if (IsFALSE(error_flag))
		{
			if (IsTRUE(check_switch_condition(index)))
			{
				if (another_switch == NO_INDEX)
				{	/*将该单动道岔添加到待转换道岔队列中*/
					start_switch(index,need_location); 
					error_flag = CI_TRUE;
				}
				if (IsFALSE(error_flag))
				{
					if (IsTRUE(check_switch_condition(another_switch)))
					{/*将该双动道岔的第一动道岔号添加到待转换道岔队列中*/
						start_switch(index,need_location); 
					}
					else
					{
						CIHmi_SendDebugTips("%s#道岔不能转动!",gn_name(another_switch));
					}	
				}	
			}
			else
			{
				CIHmi_SendDebugTips("%s#道岔不能转动!",gn_name(index));
			}	
		}
	}

	
	FUNCTION_OUT;
}

/****************************************************
函数名：   check_switch_condition
功能描述： 检查道岔单操的条件
返回值：   CI_BOOL
参数：     int16_t node_index
作者：	   hejh
日期：     2014/06/03
****************************************************/
CI_BOOL check_switch_condition(int16_t node_index)
{
	int16_t section;
	CI_BOOL judge_result = CI_TRUE;

	FUNCTION_IN;
	/*参数检查*/
	if ((node_index >= 0) && (node_index < TOTAL_SIGNAL_NODE))
	{
		section = gn_switch_section(node_index);
		
		/*检查道岔及双动道岔另一动所在区段空闲*/
		if (gn_section_state(section) != SCS_CLEARED)
		{
			process_warning(ERR_SECTION_OCCUPIED,gn_name(section));
			CIHmi_SendNormalTips("区段占用：%s",gn_name(node_index));
			judge_result = CI_FALSE;
		}
		/*检查道岔及双动道岔的另一动未被进路锁闭*/
		if(IsTRUE(judge_result) && IsTRUE(is_node_locked(node_index,LT_LOCKED)))
		{
			process_warning(ERR_SWITCH_LOCKED,gn_name(node_index));
			CIHmi_SendNormalTips("道岔锁闭：%s",gn_name(node_index));
			judge_result = CI_FALSE;			
		}
		/*检查道岔及双动道岔另一动所在区段是否被锁闭*/
		if(IsTRUE(judge_result) && IsTRUE(is_node_locked(section,LT_LOCKED)))
		{
			process_warning(ERR_NODE_LOCKED,gn_name(section));
			CIHmi_SendNormalTips("区段锁闭：%s",gn_name(node_index));
			judge_result = CI_FALSE;			
		}		
		/*检查道岔及双动道岔另一动是否被单锁*/
		if(IsTRUE(judge_result) && IsTRUE(is_node_locked(node_index,LT_SWITCH_SIGNLE_LOCKED)))
		{
			process_warning(ERR_SWITCH_LOCKED,gn_name(node_index));
			CIHmi_SendNormalTips("道岔单锁：%s",gn_name(node_index));
			judge_result = CI_FALSE;			
		}
		/*检查道岔及双动道岔另一动是否被中间道岔锁闭*/
		if(IsTRUE(judge_result) && IsTRUE(is_node_locked(node_index,LT_MIDDLE_SWITCH_LOCKED)))
		{
			process_warning(ERR_SWITCH_LOCKED,gn_name(node_index));
			CIHmi_SendNormalTips("道岔锁闭：%s",gn_name(node_index));
			judge_result = CI_FALSE;			
		}
	}
	else
	{
		judge_result = CI_FALSE;
	}
	FUNCTION_OUT;
	return judge_result;
}