/***************************************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.
文件名:  switch_control.c
作者:    CY
版本 :   1.2	
创建日期:2011/12/2
用途:    道岔控制模块
历史修改记录:  
2012/8/7,V1.2，LYC:
	1.增加了检查道岔区段锁闭的条件
2012/11/8，V1.2.1 hjh
	删除switch_control中检查道岔所在区段锁闭的条件
2013/7/26，V1.2.1 LYC
	1.修改了占用双动道岔的其中一动所在区段，操作另一动时，提示另一动轨道占用的错误
	2.增加了道岔转换到位后清空待转换道岔队列中的数据功能
2014/5/8 V1.2.3 hjh
	增加高速道岔。
2014/5/23 V1.2.3 LYC
	增加了同意动岔场间道岔位置表示灯开放检查
***************************************************************************************/
#include "switch_control.h"
#include "global_data.h"
#include "utility_function.h"
#include "error_process.h"
#include "global_data.h"


/****************************************************
函数名:    init_switch_control
功能描述:  初始化道岔控制模块
返回值:    
作者  :    CY
日期  ：   2011/12/12
****************************************************/
void init_switch_control(void)
{
	int16_t i = 0;
	/*初始化待转换道岔队列*/
	for (i = 0; i < MAX_WAIT_SWITCH; i++)
	{
		wait_switches[i].switch_index  = NO_INDEX;
		wait_switches[i].timer_counter = NO_TIMER;
	}
	/*初始化正在转换道岔队列*/
	for (i = 0; i < MAX_CONCURRENCY_SWITCH; i++)
	{
		switching[i].switch_index = NO_INDEX;
		switching[i].timer_counter = NO_TIMER;
	}
	/*初始化挤岔报警*/
	for (i = 0; i < MAX_SWITCH_JCBJ; i++)
	{
		switch_alarm[i].switch_index = NO_INDEX;
		switch_alarm[i].timer_counter = NO_TIMER;
	}	
}
/****************************************************
函数名:    switch_control
功能描述:  道岔控制模块
返回值:    
作者  :    CY
日期  ：   2011/12/2
****************************************************/
void switch_control(void)
{
	int8_t i = 0,j = 0;
	int16_t current_switch = wait_switches[0].switch_index;
	int16_t another_switch = 0,switch_section = 0,switch_lamp;
	node_t switch_index;
	uint16_t switch_need_location;
	CI_BOOL result = CI_FALSE;
	route_t fwr = NO_INDEX,bwr = NO_INDEX,route_index = NO_INDEX;
	FUNCTION_IN;
	
	/*总启动*/
	zqd();

	/*挤岔报警*/
	SwitchNoindicateAlarm(NO_INDEX);

	/*安全线道岔*/
	safe_line_switch_opreate();

	/*2014/5/23 LYC 同意动岔场间道岔位置表示灯开放*/
	for (i = 0; i < TOTAL_SWITCH_INDICATION_LAMP; i++)
	{
		/*判断转换到位的道岔是否存在场间道岔表示灯的特殊配置*/
		switch_lamp = agree_opreater_switch_config[i].switch_indicatton_lamp;
		if (switch_lamp != NO_INDEX)
		{
			/*获取同意动岔的道岔索引号和要求位置*/
			switch_index = agree_opreater_switch_config[i].conditon_switch_location.index;
			switch_need_location = agree_opreater_switch_config[i].conditon_switch_location.state;
			/*检查场间道岔表示灯开放的条件：道岔都在要求位置上*/
			if (gn_switch_state(switch_index) != switch_need_location)
			{
				result = CI_TRUE;
			}
			if (IsFALSE(result))
			{
				sn_state(switch_lamp,SIO_HAS_SIGNAL);
			}
			else
			{
				sn_state(switch_lamp,SIO_NO_SIGNAL);
			}
		}
	}	

	/*正在转岔队列中所有道岔的处理过程*/
	for (i = 0 ; (i < MAX_CONCURRENCY_SWITCH) && (switching_count > 0); i++)
	{
		/*正在转岔列表中检查道岔索引号的有效性*/
		if ( NO_INDEX == switching[i].switch_index)
			continue;
		
		/*道岔转换到位后处理*/
		if (switching[i].need_location == gn_switch_state(switching[i].switch_index))
		{   
			/*转换到位则清除在正在转岔列表中的索引号和计数标志*/
			clear_switch_command(switching[i].switch_index);
			switching[i].switch_index = NO_INDEX;
			switching[i].need_location = SWS_ERROR;
			switching[i].timer_counter = NO_TIMER;			
			switching_count--;
			if (switching_count < 0)
			{
				switching_count = 0;
			}
		}
		/*道岔转岔后还保持在之前的位置*/
		else if (NO_TIMER != switching[i].timer_counter 
			&& (CICycleInt_GetCounter() - switching[i].timer_counter) > (3 * 1000 / CI_CYCLE_MS))
		{
			if (((switching[i].need_location == SWS_NORMAL) && (gn_switch_state(switching[i].switch_index) == SWS_REVERSE))
				|| ((switching[i].need_location == SWS_REVERSE) && (gn_switch_state(switching[i].switch_index) == SWS_NORMAL)))
			{
				CIHmi_SendNormalTips("不能转岔：%s",gn_name(switching[i].switch_index));
				if (gn_belong_route(switching[i].switch_index) != NO_INDEX)
				{
					route_index = gn_belong_route(switching[i].switch_index);
					switch_out_time(route_index);
					fwr = gr_forward(route_index);
					bwr = gr_backward(route_index);
					/*检查关联的长调车进路*/
					for (i = fwr; i != NO_INDEX;)
					{
						switch_out_time(i);				
						i = gr_forward(i);
					}	
					for (i = bwr; i != NO_INDEX;)
					{
						switch_out_time(i);
						i = gr_backward(i);
					}
				}
				else
				{
					clear_switch_command(switching[i].switch_index);
					switching[i].switch_index = NO_INDEX;
					switching[i].need_location = SWS_ERROR;
					switching[i].timer_counter = NO_TIMER;
					switching_count--;
					if (switching_count < 0)
					{
						switching_count = 0;
					}
				}
			}
		}		
		/*判断道岔转换是否超时*/
		else if (NO_TIMER != switching[i].timer_counter 
			&& (CICycleInt_GetCounter() - switching[i].timer_counter) > MAX_SWITCH_TIME)
		{
			clear_switch_command(switching[i].switch_index);
			/*hjh 2015-08-06 有时道岔转动13后才有表示，故再加3秒*/
			if ((CICycleInt_GetCounter() - switching[i].timer_counter) > (MAX_SWITCH_TIME + (3 * 1000 / CI_CYCLE_MS)))
			{
				/*转岔超时*/
				process_warning(ERR_SWITCH_OUTTIME,gn_name(switching[i].switch_index));		
				CIHmi_SendNormalTips("不能转岔：%s",gn_name(switching[i].switch_index));
				if (gn_belong_route(switching[i].switch_index) != NO_INDEX)
				{
					route_index = gn_belong_route(switching[i].switch_index);
					switch_out_time(route_index);
					fwr = gr_forward(route_index);
					bwr = gr_backward(route_index);
					/*检查关联的长调车进路*/
					for (i = fwr; i != NO_INDEX;)
					{
						switch_out_time(i);				
						i = gr_forward(i);
					}	
					for (i = bwr; i != NO_INDEX;)
					{
						switch_out_time(i);
						i = gr_backward(i);
					}
				}
				else
				{
					clear_switch_command(switching[i].switch_index);
					switching[i].switch_index = NO_INDEX;
					switching[i].need_location = SWS_ERROR;
					switching[i].timer_counter = NO_TIMER;
					switching_count--;
					if (switching_count < 0)
					{
						switching_count = 0;
					}
				}
			}			
		}
		else
		{
			/*如果道岔不满足以上条件则表明是新加入正在转换队列中的道岔，则控制该道岔向要求位置转动*/
            send_switch_command(switching[i].switch_index, switching[i].need_location 
				== SWS_REVERSE ? SWS_REVERSE:SWS_NORMAL);
    //        /*道岔在规定时间内没有转换到位处理*/
    //        if (NO_TIMER != switching[i].timer_counter 
				//&& ((CICycleInt_GetCounter() - switching[i].timer_counter) > (MAX_SWITCH_TIME )))
    //        {  
				///*转岔超时*/           
				//if (gn_belong_route(switching[i].switch_index) != NO_INDEX)
				//{
				//	route_index = gn_belong_route(switching[i].switch_index);
				//	switch_out_time(route_index);
				//	fwr = gr_forward(route_index);
				//	bwr = gr_backward(route_index);
				//	/*检查关联的长调车进路*/
				//	for (i = fwr; i != NO_INDEX;)
				//	{
				//		switch_out_time(i);				
				//		i = gr_forward(i);
				//	}	
				//	for (i = bwr; i != NO_INDEX;)
				//	{
				//		switch_out_time(i);
				//		i = gr_backward(i);
				//	}
				//}
				//else
				//{
				//	clear_switch_command(switching[i].switch_index);
				//	switching[i].switch_index = NO_INDEX;
				//	switching[i].need_location = SWS_ERROR;
				//	switching[i].timer_counter = NO_TIMER;
				//	switching_count--;
				//	if (switching_count < 0)
				//	{
				//		switching_count = 0;
				//	}
				//}
    //        }			
		}
	}
	
	/*将待转换道岔队列中的道岔添加到正在转换道岔队列处理过程*/
	/*检查是否有待转道岔需要加入未满的正在转换道岔队列*/
	if ((NO_INDEX != current_switch) && (MAX_CONCURRENCY_SWITCH > switching_count) && (wait_switches_count > 0))
	{
		another_switch = gn_another_switch(current_switch);
		switch_section = (NO_INDEX != another_switch ?gn_switch_section(another_switch):NO_INDEX);
		/*检查道岔锁闭或道岔区段占用情况*/
		if ( SCS_CLEARED == gn_section_state(gn_switch_section(current_switch)) &&
			IsFALSE(is_node_locked(current_switch,LT_LOCKED |
			LT_SWITCH_SIGNLE_LOCKED | LT_SWITCH_THROAT_LOCKED | LT_MIDDLE_SWITCH_LOCKED)) &&
			 ((NO_INDEX != another_switch &&
			 SCS_CLEARED == gn_section_state(gn_switch_section(another_switch)) &&
			IsFALSE(is_node_locked(gn_switch_section(another_switch),LT_LOCKED)) &&
			IsFALSE(is_node_locked(another_switch,LT_LOCKED |
			LT_SWITCH_SIGNLE_LOCKED | LT_SWITCH_THROAT_LOCKED | LT_MIDDLE_SWITCH_LOCKED)))
			 || NO_INDEX == another_switch))
		{
			/*当前道岔是高速道岔*/
			if (IsTRUE(is_high_speed_switch(current_switch)))
			{
				if (switching_count == 0)
				{
					switching[0].switch_index = wait_switches[0].switch_index;
					switching[0].need_location = wait_switches[0].need_location;
					switching[0].timer_counter = CICycleInt_GetCounter();
					//PRINTF2("转换高速道岔 %s,%s",gn_name(switching[0].switch_index),((switching[0].need_location == SWS_NORMAL)?"SWS_NORMAL":"SWS_REVERSE"));
					CIHmi_SendDebugTips("转换%s#高速道岔,%s",gn_name(switching[0].switch_index),((switching[0].need_location == SWS_NORMAL)?"定位":"反位"));
					switching_count++;					
					for (i = 0; i < wait_switches_count - 1; i ++)
					{
						wait_switches[i].switch_index = wait_switches[i + 1].switch_index;
						wait_switches[i].need_location = wait_switches[i + 1].need_location;
						wait_switches[i].timer_counter = NO_TIMER;
					}
					/*2013/7/26LYC道岔转换到位后清空待转换道岔队列中的数据*/
					wait_switches[i].switch_index  = NO_INDEX;
					wait_switches[i].need_location = SWS_ERROR;
					wait_switches[i].timer_counter = NO_TIMER;
					wait_switches_count--;
					if (wait_switches_count < 0)
					{
						wait_switches_count = 0;
					}
				}
			}
			else
			{
				for (j = 0; j < MAX_CONCURRENCY_SWITCH; j++)
				{
					/*遍历正在转岔队列是否有高速道岔*/
					if (NO_INDEX != switching[j].switch_index && IsTRUE(is_high_speed_switch(switching[j].switch_index)))
					{
						break;
					}
					/*检查待转换道岔队列不为空*/
					if (NO_INDEX == switching[j].switch_index && wait_switches_count > 0)
					{
						switching[j].switch_index = wait_switches[0].switch_index;
						switching[j].need_location = wait_switches[0].need_location;
						switching[j].timer_counter = CICycleInt_GetCounter();
						//PRINTF2("转换道岔 %s,%s",gn_name(switching[j].switch_index),((switching[j].need_location == SWS_NORMAL)?"SWS_NORMAL":"SWS_REVERSE"));
						CIHmi_SendDebugTips("转换%s#道岔,%s",gn_name(switching[j].switch_index),((switching[j].need_location == SWS_NORMAL)?"定位":"反位"));
						switching_count++;					
						for (i = 0; i < wait_switches_count - 1; i ++)
						{
							wait_switches[i].switch_index = wait_switches[i + 1].switch_index;
							wait_switches[i].need_location = wait_switches[i + 1].need_location;
							wait_switches[i].timer_counter = NO_TIMER;
						}
						/*2013/7/26LYC道岔转换到位后清空待转换道岔队列中的数据*/
						wait_switches[i].switch_index  = NO_INDEX;
						wait_switches[i].need_location = SWS_ERROR;
						wait_switches[i].timer_counter = NO_TIMER;
						wait_switches_count--;
						if (wait_switches_count < 0)
						{
							wait_switches_count = 0;
						}
						break;
					}
				}
			}
		}
		else
		{
			for (i = 0; i < wait_switches_count - 1; i++)
			{
				wait_switches[i].switch_index = wait_switches[i + 1].switch_index;
				wait_switches[i].need_location = wait_switches[i + 1].need_location;
				wait_switches[i].timer_counter = NO_TIMER;
			}
			wait_switches[i].switch_index = NO_INDEX;
			wait_switches[i].need_location = SWS_ERROR;
			wait_switches[i].timer_counter = NO_TIMER;
			wait_switches_count--;
			if (wait_switches_count < 0)
			{
				wait_switches_count = 0;
			}

			if (wait_switches_count > 0)
			{
				if (wait_switches[0].need_location == gn_switch_state(wait_switches[0].switch_index))
				{
					/*转换到位 hjh 2015-01-19 added*/
					wait_switches[0].switch_index = NO_INDEX;
					wait_switches[0].need_location = SWS_ERROR;
					wait_switches[0].timer_counter = NO_TIMER;

					for (i = 0; i < wait_switches_count - 1; i ++)
					{
						wait_switches[i].switch_index = wait_switches[i + 1].switch_index;
						wait_switches[i].need_location = wait_switches[i + 1].need_location;
						wait_switches[i].timer_counter = NO_TIMER;
					}
					wait_switches[i].switch_index = NO_INDEX;
					wait_switches[i].need_location = SWS_ERROR;
					wait_switches[i].timer_counter = NO_TIMER;
					wait_switches_count--;
					if (wait_switches_count < 0)
					{
						wait_switches_count = 0;
					}
				}

				/*提示错误信息*/
				if (another_switch != NO_INDEX)
				{
					/*道岔所在区段占用错误提示*/
					if (SCS_CLEARED != gn_section_state(gn_switch_section(current_switch)))
					{
						process_warning(ERR_SECTION_OCCUPIED,gn_name(gn_switch_section(current_switch)));
						CIHmi_SendNormalTips("区段占用：%s",gn_name(gn_switch_section(current_switch)));
					}
					/*双动道岔另一动所在区段占用错误提示*/
					if (SCS_CLEARED != gn_section_state(gn_switch_section(another_switch)))
					{
						process_warning(ERR_SECTION_OCCUPIED,gn_name(gn_switch_section(another_switch)));
						CIHmi_SendNormalTips("区段占用：%s",gn_name(gn_switch_section(another_switch)));
					}

					/*道岔锁闭错误提示*/
					if(IsTRUE(is_node_locked(current_switch,LT_LOCKED | LT_SWITCH_SIGNLE_LOCKED | LT_SWITCH_THROAT_LOCKED | LT_MIDDLE_SWITCH_LOCKED)))
					{
						process_warning(ERR_SWITCH_LOCKED,gn_name(current_switch));
						CIHmi_SendNormalTips("道岔锁闭：%s",gn_name(current_switch));
					}
					/*双动道岔另一动锁闭错误提示*/
					if(IsTRUE(is_node_locked(another_switch,LT_LOCKED | LT_SWITCH_SIGNLE_LOCKED | LT_SWITCH_THROAT_LOCKED | LT_MIDDLE_SWITCH_LOCKED)))
					{
						process_warning(ERR_SWITCH_LOCKED,gn_name(another_switch));
						CIHmi_SendNormalTips("道岔锁闭：%s",gn_name(another_switch));
					}
					/*道岔所在区段锁闭错误提示*/
					if (IsTRUE(is_node_locked(gn_switch_section(current_switch),LT_LOCKED)))
					{
						process_warning(ERR_NODE_LOCKED,gn_name(gn_switch_section(current_switch)));
						CIHmi_SendNormalTips("区段锁闭：%s",gn_name(gn_switch_section(current_switch)));
					}
					/*双动道岔另一动所在区段锁闭错误提示*/
					if (IsTRUE(is_node_locked(gn_switch_section(another_switch),LT_LOCKED)))
					{
						process_warning(ERR_NODE_LOCKED,gn_name(gn_switch_section(another_switch)));
						CIHmi_SendNormalTips("区段锁闭：%s",gn_name(gn_switch_section(another_switch)));
					}
				}
				else
				{
					/*道岔所在区段占用错误提示*/
					if (SCS_CLEARED != gn_section_state(gn_switch_section(current_switch)))
					{
						process_warning(ERR_SECTION_OCCUPIED,gn_name(gn_switch_section(current_switch)));
						CIHmi_SendNormalTips("区段占用：%s",gn_name(gn_switch_section(current_switch)));
					}
					/*道岔锁闭错误提示*/
					if(IsTRUE(is_node_locked(current_switch,LT_LOCKED | LT_SWITCH_CLOSED | LT_SWITCH_SIGNLE_LOCKED | LT_SWITCH_THROAT_LOCKED | LT_MIDDLE_SWITCH_LOCKED)))
					{
						process_warning(ERR_SWITCH_LOCKED,gn_name(current_switch));
						CIHmi_SendNormalTips("道岔锁闭：%s",gn_name(current_switch));
					}
					/*道岔所在区段锁闭错误提示*/
					if (IsTRUE(is_node_locked(gn_switch_section(current_switch),LT_LOCKED)))
					{
						process_warning(ERR_NODE_LOCKED,gn_name(gn_switch_section(current_switch)));
						CIHmi_SendNormalTips("区段锁闭：%s",gn_name(gn_switch_section(current_switch)));
					}
				}
			}			
		}
	}
	FUNCTION_OUT;
}

/****************************************************
函数名:    start_switch
功能描述:  向待转换道岔队列中添加道岔
返回值:    
参数:      index      待转换道岔
           need_location  要求转换到的位置
作者  :    CY
日期  ：   2011/12/7
****************************************************/
void start_switch(int16_t index,EN_switch_state need_location)
{
	int16_t i = 0;
	CI_BOOL addFlag = CI_TRUE;

	/*参数检查*/
	if (((index >= 0) && (index < TOTAL_SIGNAL_NODE))
		&& (gn_type(index) == NT_SWITCH)
		&& ((SWS_NORMAL == need_location) || (SWS_REVERSE == need_location)))
	{		
		/*检查待转换道岔队列未满*/
		if (MAX_WAIT_SWITCH == wait_switches_count)
		{
			return;
		}
		for (i = 0; i < wait_switches_count; i++)
		{
			/*检查道岔索引号是否存在*/
			if ((wait_switches[i].switch_index == index)
				|| (wait_switches[i].switch_index == gn_another_switch(index)))
			{
				addFlag = CI_FALSE;
				CIHmi_SendNormalTips("正在排队：",gn_name(index));
				break;
			}
		}
		for (i = 0; i < switching_count; i++)
		{
			/*检查道岔索引号是否存在*/
			if ((switching[i].switch_index == index)
				|| (switching[i].switch_index == gn_another_switch(index)))
			{
				addFlag = CI_FALSE;
				CIHmi_SendNormalTips("正在转岔：",gn_name(index));
				break;
			}
		}
		/*向待转换道岔队列中添加道岔*/
		if (IsTRUE(addFlag))
		{
			CIHmi_SendDebugTips("准备转岔 %s",gn_name(index));
			wait_switches[wait_switches_count].switch_index = index;
			wait_switches[wait_switches_count].need_location = need_location;
			wait_switches[wait_switches_count].timer_counter = NO_TIMER;
			wait_switches_count++;	
		}		
	}	
}

/****************************************************
函数名：   safe_line_switch_opreate
功能描述： 安全线道岔操作
返回值：   void
参数：     void
作者：	   hejh
日期：     2014/04/24
****************************************************/
void safe_line_switch_opreate(void)
{
	int8_t i;
	node_t switch_index;
	EN_switch_state switch_location;

	for (i = 0; i < TOTAL_SAFE_LINE_SWITCH; i++)
	{
		/*获取安全线道岔及位置*/
		switch_index = safeline_switch_config[i].SwitchIndex;
		switch_location = safeline_switch_config[i].Location;
		/*道岔位置未开通安全线方向且未锁闭*/
		if ((switch_location != gn_switch_state(switch_index)) 
			&& IsFALSE(is_node_locked(gn_switch_section(switch_index),LT_LOCKED)) 
			&& IsFALSE(gn_used_state(gn_switch_section(switch_index))))
		{
			/*存在另一动*/
			if (gn_another_switch(switch_index) != NO_INDEX)
			{
				/*另一动未锁闭*/
				if (IsFALSE(is_node_locked(gn_switch_section(gn_another_switch(switch_index)),LT_LOCKED)) 
					&& IsFALSE(gn_used_state(gn_switch_section(gn_another_switch(switch_index)))))
				{
					if (IsTRUE(is_node_timer_run(switch_index)))
					{
						if (IsTRUE(is_node_complete_timer(switch_index)))
						{
							/*process_warning(ERR_SAFE_LINE,gn_name(switch_index));*/
							sn_stop_timer(switch_index);
							sn_start_timer(switch_index,SECONDS_15,DTT_OTHER);
						}
					}
					else
					{
						sn_start_timer(switch_index,SECONDS_15,DTT_OTHER);
					}	
				}
			}
			else
			{
				if (IsTRUE(is_node_timer_run(switch_index)))
				{
					if (IsTRUE(is_node_complete_timer(switch_index)))
					{
						/*process_warning(ERR_SAFE_LINE,gn_name(switch_index));*/
						sn_stop_timer(switch_index);
						sn_start_timer(switch_index,SECONDS_15,DTT_OTHER);
					}
				}
				else
				{
					sn_start_timer(switch_index,SECONDS_15,DTT_OTHER);
				}	
			}

					
		}
		else
		{
			sn_stop_timer(switch_index);
		}
	}

}

/****************************************************
函数名：   switch_out_time
功能描述： 道岔转换超时后的处理
返回值：   void
参数：     node_t switch_index
作者：	   hejh
日期：     2014/05/19
****************************************************/
void switch_out_time(route_t route_index)
{
	int16_t i,j;

	if (route_index != NO_INDEX)
	{
		/*正在转岔队列中所有道岔的处理过程*/
		for (i = 0 ; i < MAX_CONCURRENCY_SWITCH; i++)
		{
			if ((switching[i].switch_index != NO_INDEX) && (route_index == gn_belong_route(switching[i].switch_index)))
			{
				clear_switch_command(switching[i].switch_index);
				switching[i].switch_index = NO_INDEX;
				switching[i].need_location = SWS_ERROR;
				switching[i].timer_counter = NO_TIMER;
				switching_count--;
				if (switching_count < 0)
				{
					switching_count = 0;
				}
			}				
		}
		/*待转岔队列中所有道岔的处理过程*/
		for (i = 0 ; i < MAX_WAIT_SWITCH; i++)
		{
			if ((wait_switches[i].switch_index != NO_INDEX) && (route_index == gn_belong_route(wait_switches[i].switch_index)))
			{
				wait_switches_count--;
				if (wait_switches_count < 0)
				{
					wait_switches_count = 0;
				}
				/*将待转化道岔队列中的道岔前移*/
				if (i < MAX_WAIT_SWITCH - 1)
				{
					for (j = i; j < wait_switches_count; j++)
					{
						wait_switches[j].switch_index = wait_switches[j + 1].switch_index;
						wait_switches[j].need_location = wait_switches[j + 1].need_location;	
						wait_switches[j].timer_counter = NO_TIMER;
					}
					wait_switches[j].switch_index  = NO_INDEX;
					wait_switches[j].need_location = SWS_ERROR;
					wait_switches[j].timer_counter = NO_TIMER;
					i--;
				}
			}
		}
		/*删除进路*/
		sr_state(route_index,RS_FAILURE_TO_BUILD);
		sr_stop_timer(route_index);
	}	
}

/****************************************************
函数名：   zqd
功能描述： 总启动
返回值：   void
作者：	   hejh
日期：     2015/07/27
****************************************************/
void zqd()
{
	int16_t i;
	node_t zqd_index = NO_INDEX;

	/*获取总启动索引号*/
	for (i = 0; i < TOTAL_SIGNAL_NODE; i++)
	{
		if (gn_type(i) == NT_ZQD)
		{
			zqd_index = i;
			break;
		}
	}
	if (zqd_index != NO_INDEX)
	{
		/*输出总启动命令*/
		if (switching_count > 0)
		{
			//CIHmi_SendDebugTips("总启动输出");
			send_command(zqd_index,SZQD_HAS_SIGNAL);		
		}
		else
		{
			if (gn_state(zqd_index) == SZQD_HAS_SIGNAL)
			{
				//CIHmi_SendDebugTips("总启动停止输出");
				send_command(zqd_index,SZQD_NO_SIGNAL);
			}
			else
			{
				send_command(zqd_index,SZQD_ERROR);
			}
		}
	}
}

/****************************************************
函数名：   SwitchNoindicateAlarm
功能描述： 挤岔报警
返回值：   void
参数：     int16_t button_index
作者：	   hejh
日期：     2015/07/23
****************************************************/
void SwitchNoindicateAlarm(int16_t button_index)
{
	int16_t i, j;
	CI_BOOL repeatedFlag = CI_FALSE;
	CI_BOOL allSwitchOK = CI_TRUE;
	node_t alarm_index = NO_INDEX;

	/*获取挤岔报警索引号*/
	for (i = 0; i < TOTAL_SIGNAL_NODE; i++)
	{
		if (gn_type(i) == NT_JCBJ)
		{
			alarm_index = i;
			break;
		}
	}

	if (alarm_index != NO_INDEX)
	{
		if (button_index != NO_INDEX)
		{
			/*挤岔报警-->挤岔确认*/
			if (gn_state(alarm_index) == JCBJ_ALARM)
			{
				for (j = 0; j < MAX_SWITCH_JCBJ; j++)
				{
					if (switch_alarm[j].switch_index != NO_INDEX)
					{
						switch_alarm[j].timer_counter = NO_TIMER;
					}
				}
				sn_state(alarm_index,JCBJ_KNOW);
			}
			/*挤岔恢复提醒-->挤岔报警*/
			if (gn_state(alarm_index) == JCBJ_WARNING)
			{
				for (i = 0; i < MAX_SWITCH_JCBJ; i++)
				{
					if (switch_alarm[i].switch_index != NO_INDEX)
					{
						allSwitchOK = CI_FALSE;
						sn_state(alarm_index,JCBJ_KNOW);
						break;
					}
				}
				/*挤岔恢复提醒-->正常*/
				if (IsTRUE(allSwitchOK))
				{
					sn_state(alarm_index,JCBJ_NORMAL);
				}
			}
		}
		else
		{
			/*轮询道岔无表示时间超过挤岔报警时间*/
			if ((gn_state(alarm_index) == JCBJ_NORMAL)
				|| (gn_state(alarm_index) == JCBJ_KNOW)
				|| (gn_state(alarm_index) == JCBJ_WARNING))
			{
				for (i = 0; i < MAX_SWITCH_JCBJ; i++)
				{
					if (switch_alarm[i].switch_index != NO_INDEX)
					{
						if (switch_alarm[i].timer_counter - CICycleInt_GetCounter() <= 0)
						{
							/*设置挤岔报警为挤岔报警*/
							sn_state(alarm_index,JCBJ_ALARM);
							//switch_alarm[i].timer_counter = NO_TIMER;
						}
					}
				}
			}

			/*轮询道岔恢复表示*/
			for (i = 0; i < MAX_SWITCH_JCBJ; i++)
			{
				if (switch_alarm[i].switch_index != NO_INDEX)
				{
					if ((gn_switch_state(switch_alarm[i].switch_index) == SWS_NORMAL)
						|| (gn_switch_state(switch_alarm[i].switch_index) == SWS_REVERSE))
					{
						if (switch_alarm[i].timer_counter != NO_TIMER)
						{
							switch_alarm[i].switch_index = NO_INDEX;
							switch_alarm[i].timer_counter = NO_TIMER;

							allSwitchOK = CI_TRUE;
							for (j = 0; j < MAX_SWITCH_JCBJ; j++)
							{
								if (switch_alarm[j].switch_index != NO_INDEX)
								{
									allSwitchOK = CI_FALSE;
									break;
								}
							}
							/*挤岔报警-->正常*/
							if (gn_state(alarm_index) == JCBJ_ALARM)
							{						
								sn_state(alarm_index,JCBJ_NORMAL);
							}
						}
						else
						{
							switch_alarm[i].switch_index = NO_INDEX;
							switch_alarm[i].timer_counter = NO_TIMER;
							/*设置挤岔报警为挤岔恢复提醒*/
							if ((gn_state(alarm_index) == JCBJ_KNOW)
								|| (gn_state(alarm_index) == JCBJ_NORMAL))
							{						
								sn_state(alarm_index,JCBJ_WARNING);
							}
						}											
					}
				}
			}				

			/*添加道岔*/
			for (i = 0; i < TOTAL_SIGNAL_NODE; i++)
			{
				if (gn_type(i) == NT_SWITCH)
				{
					if ((gn_switch_state(i) == SWS_NO_INDICATE)
						|| (gn_switch_state(i) == SWS_ERROR))
					{
						/*检查重复*/
						repeatedFlag = CI_FALSE;
						for (j = 0; j < MAX_SWITCH_JCBJ; j++)
						{
							if (switch_alarm[j].switch_index == i)
							{
								repeatedFlag = CI_TRUE;
								break;
							}
						}
						/*添加至列表*/
						if (IsFALSE(repeatedFlag))
						{
							for (j = 0; j < MAX_SWITCH_JCBJ; j++)
							{
								if (switch_alarm[j].switch_index == NO_INDEX)
								{
									switch_alarm[j].switch_index = i;
									switch_alarm[j].timer_counter = CICycleInt_GetCounter() + TIME_JCBJ / CI_CYCLE_MS;
									break;
								}
							}
						}
					}
				}
			}
		}
	}		
}