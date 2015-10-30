/***************************************************************************************
Copyright (C), 2014,  Co.Hengjun, Ltd.
文件名:hold_route_shunting.c
作者:  hejh
版本:  V1.0	
日期:  2014/08/11
用途:  非进路调车
历史修改记录:  

***************************************************************************************/
#include "init_manage.h"
#include "utility_function.h"
#include "global_data.h"
#include "error_process.h"
#include "check_ci_condition.h"
#include "check_lock_route.h"
#include "open_signal.h"
#include "switch_control.h"
#include "hold_route_shunting.h"

/****************************************************
函数名：   command_hold_route_shunting
功能描述： 非进路调车
返回值：   void
参数：     int16_t special_index
作者：	   hejh
日期：     2014/08/11
****************************************************/
void command_hold_route_shunting(int16_t special_index)
{
	int16_t hold_routes[MAX_CHANGE_ROUTE][MAX_ILT_PER_ROUTES];
	node_t start_signal = NO_INDEX,end_signal = NO_INDEX;
	int16_t change_route_index = NO_INDEX;
	uint8_t i,j,k;
	CI_BOOL result = CI_FALSE;
	node_t node_index = NO_INDEX;
	
	/*获取非进路调车的始端和终端信号*/
	start_signal = gs_hold_route_shunting_start_signal(special_index);
	end_signal = gs_hold_route_shunting_end_signal(special_index);

	/*存在非进路调车*/
	if (gs_hrs_state(special_index) == HRSS_OPENING_SIGNAL)
	{
		ss_hrs_state(special_index,HRSS_CLOSING_SIGNAL);
	}
	else if (gs_hrs_state(special_index) == HRSS_KEEP_SIGNAL_FAILURE)
	{
		ss_hrs_state(special_index,HRSS_DELAY_UNLOCK);
	}
	else
	{
		process_warning(ERR_OPERATION,gn_name(start_signal));
	}

	/*不存在非进路调车*/
	if (gs_hrs_state(special_index) == HRSS_ERROR)
	{
		/*搜索非进路调车*/
		search_hold_route(hold_routes,start_signal,end_signal);
		change_route_index = select_hold_route(hold_routes);
		if (change_route_index != NO_INDEX)
		{
			for (j = 0; (j < MAX_ILT_PER_ROUTES) && (hold_routes[change_route_index][j] != NO_INDEX);j++)
			{
				for (i = 0; i < MAX_ROUTE; i++)
				{     
					/*初始化基本的联锁表数据*/
					if(NO_INDEX == routes[i].ILT_index)
					{
						routes[i].ILT_index = hold_routes[change_route_index][j];
						routes[i].other_flag = ROF_HOLD_ROUTE_SHUNTING;
						CIHmi_SendDebugTips("搜索到非进路调车：%s ---> %s",device_name[gr_start_signal(i)],device_name[gb_node(gr_end_button(i))]);

						/*设置节点所属进路*/
						for ( k = 0; k < gr_nodes_count(i); k++)
						{
							node_index = gr_node(i,k);
							if ((gn_belong_route(node_index) == NO_INDEX) && 
								!((gn_type(node_index) == NT_SINGLE_SHUNTING_SIGNAL) && 
								(k+1 == gr_nodes_count(i))))
							{
								sn_belong_route(node_index,i);
							}			
						}
						break;
					}
				}
			}

			/*检查非进路调车条件*/
			/*遍历进路表，查找非进路调车相关的进路*/
			for (i = 0; i < MAX_ROUTE; i++)
			{				
				if (routes[i].other_flag == ROF_HOLD_ROUTE_SHUNTING)
				{
					result = CI_TRUE;
					result = hold_route_check_ci_condition(i);
					if (IsFALSE(result))
					{
						/*删除非进路调车*/
						for (j = 0; j < MAX_ROUTE; j++)
						{
							if (routes[j].other_flag == ROF_HOLD_ROUTE_SHUNTING)
							{
								sr_state(j,RS_FAILURE_TO_BUILD);
							}
						}
						break;
					}
				}
			}

			/*驱动道岔*/
			if (IsTRUE(result))
			{

				CIHmi_SendDebugTips("非进路调车联锁条件检查成功!");
				for (i = 0; i < MAX_ROUTE; i++)
				{
					if (routes[i].other_flag == ROF_HOLD_ROUTE_SHUNTING)
					{
						drive_switch(i);
					}		
				}
				CIHmi_SendDebugTips("非进路调车开始转岔!");
				ss_hrs_state(special_index,HRSS_SWITCHING);
			}
		}
		else
		{
			ss_hrs_state(special_index,HRSS_ERROR);
		}		
	}	
}

/****************************************************
函数名：   hold_route_shunting_fault
功能描述： 非进路调车故障恢复
返回值：   void
参数：     int16_t special_index
作者：	   hejh
日期：     2014/08/12
****************************************************/
void hold_route_shunting_fault(int16_t special_index)
{
	uint8_t i;
	node_t start_signal = NO_INDEX;

	start_signal = gs_hold_route_shunting_start_signal(special_index);	
	/*存在非进路调车*/
	if (gs_hrs_state(special_index) == HRSS_FAULT_UNLOCK)
	{
		/*未开始计时*/
		if (IsFALSE(is_node_timer_run(start_signal)))
		{
			sn_start_timer(start_signal,SECONDS_30,DTT_UNLOCK);
			ss_hrs_state(special_index,HRSS_FAULT_DELAY);
			CIHmi_SendDebugTips("非进路调车故障延时!");
		}
		else
		{
			CIHmi_SendDebugTips("非进路调车故障延时未完成!");
		}
	}
	if (gs_hrs_state(special_index) == HRSS_FAULT_DELAY_FINISH)
	{
		/*遍历进路表，查找非进路调车相关的进路*/
		for (i = 0; i < MAX_ROUTE; i++)
		{
			if (routes[i].other_flag == ROF_HOLD_ROUTE_SHUNTING)
			{
				sr_state(i,RS_UNLOCKED);
				sr_stop_timer(i);
				reset_node(gr_approach(i));
				delete_route(i);
			}
		}
		ss_hrs_state(special_index,HRSS_ERROR);
		sn_state(gs_hold_route_shunting_FJL(special_index),SIO_NO_SIGNAL);
		sn_state(gs_hold_route_shunting_FJLGZ(special_index),SIO_NO_SIGNAL);
	}
}

/****************************************************
函数名：   hold_route_shunting
功能描述： 非进路调车
返回值：   void
参数：     void
作者：	   hejh
日期：     2014/08/11
****************************************************/
void hold_route_shunting(void)
{
	uint8_t i;

	/*获取始端信号*/
	for (i = 0; i < TOTAL_HOLD_SHUNGTING_ROUTE; i++)
	{
		if (gs_hrs_state(i) != HRSS_ERROR)
		{
			hold_route_shunting_process(i);
		}
	}
}

/****************************************************
函数名：   hold_route_shunting_process
功能描述： 非进路调车执行流程
返回值：   void
参数：     int16_t special_index
作者：	   hejh
日期：     2014/08/12
****************************************************/
void hold_route_shunting_process(int16_t special_index)
{
	uint8_t i,j;
	CI_BOOL result = CI_FALSE;
	node_t node_index = NO_INDEX,start_signal = NO_INDEX;

	start_signal = gs_hold_route_shunting_start_signal(special_index);
	switch(gs_hrs_state(special_index))
	{
		/*检查选排一致*/
		case HRSS_SWITCHING:		
			/*遍历进路表，查找非进路调车相关的进路*/
			for (i = 0; i < MAX_ROUTE; i++)
			{				
				if (routes[i].other_flag == ROF_HOLD_ROUTE_SHUNTING)
				{
					/*判断计时是否结束*/
					if (IsFALSE(is_timer_run(i)))
					{
						sr_start_timer(i,MAX_ROUTE_SELECT_TIME,DTT_OTHER);
					}
					else
					{
						/*判断道岔转动是否超时*/
						if(IsTRUE(is_complete_timer(i)))
						{							
							/*删除非进路调车*/
							for (j = 0; j < MAX_ROUTE; j++)
							{
								if (routes[j].other_flag == ROF_HOLD_ROUTE_SHUNTING)
								{
									switch_out_time(j);
								}
							}
							break;
						}
					}
					/*检查选排一致条件*/
					result = CI_TRUE;
					result = hold_route_check_select_complete(i);
					if (IsFALSE(result))
					{
						break;
					}
					else
					{
						sr_stop_timer(i);
					}
				}
			}
			if (IsTRUE(result))
			{
				CIHmi_SendDebugTips("非进路调车选排一致!");
				sn_state(gs_hold_route_shunting_FJL(special_index),SIO_HAS_SIGNAL);
				ss_hrs_state(special_index,HRSS_CHECK_SELECTED);
			}
			break;
		/*信号条件检查*/
		case HRSS_CHECK_SELECTED:		
			/*遍历进路表，查找非进路调车相关的进路*/
			for (i = 0; i < MAX_ROUTE; i++)
			{				
				if (routes[i].other_flag == ROF_HOLD_ROUTE_SHUNTING)
				{
					result = CI_TRUE;
					result = hold_route_check_signal_condition(i);
					if (IsFALSE(result))
					{						
						break;
					}
				}
			}
			if (IsTRUE(result))
			{
				CIHmi_SendDebugTips("非进路调车信号条件检查成功!");
				ss_hrs_state(special_index,HRSS_SIGNAL_CONDITION_OK);
			}
			else
			{
				/*删除非进路调车*/
				for (j = 0; j < MAX_ROUTE; j++)
				{
					if (routes[j].other_flag == ROF_HOLD_ROUTE_SHUNTING)
					{
						sr_state(j,RS_FAILURE_TO_BUILD);
					}
				}
				ss_hrs_state(special_index,HRSS_ERROR);
				break;
			}
			//break;
		/*锁闭进路*/
		case HRSS_SIGNAL_CONDITION_OK:		
			/*遍历进路表，查找非进路调车相关的进路*/
			for (i = 0; i < MAX_ROUTE; i++)
			{
				if (routes[i].other_flag == ROF_HOLD_ROUTE_SHUNTING)
				{
					/*检查进路中和与进路相关的所有信号节点都处于被征用且未锁闭状态*/
					if (IsTRUE(is_all_node_unlock(i)))
					{	
						/*设置进路中及与进路相关的信号节点锁闭标志*/
						set_route_lock_nodes(i);						
						/*清除进路中及与进路相关信号节点的征用标志*/
						clear_ci_used_flag(i);
					}
				}
			}
			CIHmi_SendDebugTips("非进路调车锁闭进路!");
			ss_hrs_state(special_index,HRSS_LOCKED);
			//break;
		/*开放信号*/
		case HRSS_LOCKED:		
			/*遍历进路表，查找非进路调车相关的进路*/
			for (i = 0; i < MAX_ROUTE; i++)
			{
				if (routes[i].other_flag == ROF_HOLD_ROUTE_SHUNTING)
				{
					result = CI_TRUE;
					if (IsFALSE(hold_route_check_open_signal(i)))
					{
						result = CI_FALSE;						
						break;
					}
				}
			}
			if (IsTRUE(result))
			{				
				/*遍历进路表，查找非进路调车相关的进路*/
				for (i = 0; i < MAX_ROUTE; i++)
				{
					if (routes[i].other_flag == ROF_HOLD_ROUTE_SHUNTING)
					{
						for (j = 0; j < gr_nodes_count(i); j++)
						{
							node_index = gr_node(i,j);
							if (IsTRUE(is_shunting_signal(node_index)))
							{
								send_signal_command(node_index,SGS_B);								
							}
						}
					}
				}
				CIHmi_SendDebugTips("非进路调车开放信号!");
				ss_hrs_state(special_index,HRSS_OPENING_SIGNAL);
			}
			else
			{
				/*删除非进路调车*/
				for (j = 0; j < MAX_ROUTE; j++)
				{
					if (routes[j].other_flag == ROF_HOLD_ROUTE_SHUNTING)
					{
						sr_state(j,RS_FAILURE_TO_BUILD);
					}
				}
				ss_hrs_state(special_index,HRSS_ERROR);
			}
			break;
		/*信号保持*/
		case HRSS_OPENING_SIGNAL:
			/*遍历进路表，查找非进路调车相关的进路*/
			for (i = 0; i < MAX_ROUTE; i++)
			{
				if (routes[i].other_flag == ROF_HOLD_ROUTE_SHUNTING)
				{
					result = CI_TRUE;
					if (IsFALSE(hold_route_keep_signal(i)))
					{
						result = CI_FALSE;						
						break;
					}
				}
			}
			/*遍历进路表，查找非进路调车相关的进路*/
			for (i = 0; i < MAX_ROUTE; i++)
			{
				if (routes[i].other_flag == ROF_HOLD_ROUTE_SHUNTING)
				{
					for (j = 0; j < gr_nodes_count(i); j++)
					{
						node_index = gr_node(i,j);
						if (IsTRUE(is_shunting_signal(node_index)))
						{
							if (IsTRUE(result))
							{
								send_signal_command(node_index,SGS_B);
								ss_hrs_state(special_index,HRSS_OPENING_SIGNAL);
							}
							else
							{
								send_signal_command(node_index,SGS_A);
								ss_hrs_state(special_index,HRSS_KEEP_SIGNAL_FAILURE);
							}

						}
					}
				}
			}
			break;
		/*关闭信号*/
		case HRSS_CLOSING_SIGNAL:
			/*遍历进路表，查找非进路调车相关的进路*/
			for (i = 0; i < MAX_ROUTE; i++)
			{
				if (routes[i].other_flag == ROF_HOLD_ROUTE_SHUNTING)
				{
					for (j = 0; j < gr_nodes_count(i); j++)
					{
						node_index = gr_node(i,j);
						if (IsTRUE(is_shunting_signal(node_index)))
						{
							send_signal_command(node_index,SGS_A);
						}
					}
				}
			}
			CIHmi_SendDebugTips("非进路调车关闭信号!");
			ss_hrs_state(special_index,HRSS_DELAY_UNLOCK);
			break;
		/*延时解锁*/
		case HRSS_DELAY_UNLOCK:
			if (gn_belong_route(start_signal) != NO_INDEX)
			{
				/*检查信号关闭*/
				if (IsTRUE(hold_route_signal_closed()))
				{
					/*检查区段出清*/
					if (IsTRUE(hold_route_section_cleared(special_index)))
					{
						/*未开始计时*/
						if (IsFALSE(is_node_timer_run(start_signal)))
						{
							sn_start_timer(start_signal,SECONDS_30,DTT_UNLOCK);
							ss_hrs_state(special_index,HRSS_DELAY_UNLOCK_START);
							CIHmi_SendDebugTips("非进路调车延时解锁!");
						}
					}
					else
					{
						sn_state(gs_hold_route_shunting_FJLGZ(special_index),SIO_HAS_SIGNAL);
						ss_hrs_state(special_index,HRSS_FAULT_UNLOCK);
					}
				}
			}
			else
			{
				ss_hrs_state(special_index,HRSS_ERROR);
				start_signal = NO_INDEX;
			}
			break;
		case HRSS_DELAY_UNLOCK_START:
			/*完成计时*/
			if (IsTRUE(is_node_complete_timer(start_signal)))
			{
				/*遍历进路表，查找非进路调车相关的进路*/
				for (i = 0; i < MAX_ROUTE; i++)
				{
					if (routes[i].other_flag == ROF_HOLD_ROUTE_SHUNTING)
					{
						sr_state(i,RS_UNLOCKED);
						sr_stop_timer(i);
						reset_node(gr_approach(i));
						delete_route(i);
					}
				}
				sn_stop_timer(start_signal);
				ss_hrs_state(special_index,HRSS_ERROR);
				sn_state(gs_hold_route_shunting_FJL(special_index),SIO_NO_SIGNAL);
				sn_state(gs_hold_route_shunting_FJLGZ(special_index),SIO_NO_SIGNAL);
			}
			else
			{
				/*检查区段出清*/
				if (IsFALSE(hold_route_section_cleared(special_index)))
				{
					sn_stop_timer(start_signal);
					ss_hrs_state(special_index,HRSS_FAULT_UNLOCK);
				}
			}
			break;
		case HRSS_FAULT_DELAY:
			/*完成计时*/
			if (IsTRUE(is_node_complete_timer(start_signal)))
			{
				sn_stop_timer(start_signal);
				ss_hrs_state(special_index,HRSS_FAULT_DELAY_FINISH);
				CIHmi_SendDebugTips("非进路调车故障延时完成!");
			}
			break;
		default:
			break;
	}
}

/****************************************************
函数名：   search_hold_route
功能描述： 搜索非进路调车
返回值：   void
参数：     int16_t searched_routes[MAX_CHANGE_ROUTE][MAX_ILT_PER_ROUTES]
参数：     node_t start_node
参数：     node_t end_node
作者：	   hejh
日期：     2014/08/11
****************************************************/
void search_hold_route (int16_t searched_routes[MAX_CHANGE_ROUTE][MAX_ILT_PER_ROUTES],node_t start_node,node_t end_node)
{
	int16_t sp = 0;
	int16_t ILT_index=0;
	int16_t same_index = 0;
	int16_t i;
	node_t start_signal = start_node;
	CI_BOOL result = CI_FALSE;

	FUNCTION_IN;

	while(sp >= 0)
	{
		/*要求的始端信号和进路始端信号相同，并且他们的始端按钮也要相同，减少查找次数*/
		if( gb_node(ILT[ILT_index].start_button) == start_signal &&
			((ILT[ILT_index].start_button == signal_nodes[start_signal].buttons[0]) ||
			(ILT[ILT_index].start_button == signal_nodes[start_signal].buttons[1])))
		{
			searched_routes[same_index][sp] = ILT_index;

			/*匹配终端按钮，考虑长进路*/
			for (i = 0; i < sp + 1; i++)
			{				
				if (gb_node(ILT[searched_routes[same_index][i]].end_button) == end_node)
				{
					result = CI_TRUE;
					break;
				}
			}
			
			if (IsTRUE(result))
			{			
				/*搜索到始端按钮和终端按钮能够匹配的进路*/
				for (i = 0; i <= sp; i++)
				{
					searched_routes[same_index + 1][i] = searched_routes[same_index][i];
				}
				same_index ++;
				ILT_index ++;
			}			
			else if (IsTRUE(is_dead_ILTroute(ILT_index)) || (NO_INDEX == get_next_node_index(ILT_index)))
			{
				/*按钮匹配不成功，且进路前方没有进路，则继续往下搜索*/
				ILT_index ++;
			} 	
			else
			{
				/*获取前方进路的始端后，从联锁表的头部开始搜索*/
				start_signal = get_next_node_index(ILT_index);			
				sp++;
				ILT_index = 0;
			}
		}
		else
		{
			/*完全不匹配，所以继续往下搜索*/
			ILT_index++;
		}
		/*联锁表搜索完了还没有搜索到符合要求的进路，所以要退栈*/
		if (ILT_index == TOTAL_ILT)
		{
			sp--;
			if (sp >= 0)
			{
				/*以当前堆栈顶的进路的始端信号继续往下搜索*/
				start_signal = gb_node(ILT[searched_routes[same_index][sp]].start_button);
				/*退栈后，从前面一条进路开始，在联锁表中继续往下找*/
				ILT_index = searched_routes[same_index][sp]+1;
			}
			/*退栈*/			
			searched_routes[same_index][sp + 1] = NO_INDEX;
		}
	}

	FUNCTION_OUT;
}

/****************************************************
函数名：   get_next_node_index
功能描述： 获取下一个起始节点
返回值：   int16_t
参数：     int16_t ILT_index
作者：	   hejh
日期：     2014/08/11
****************************************************/
int16_t get_next_node_index(int16_t ILT_index)
{
	int16_t result = NO_INDEX;
	int16_t end_point = ILT[ILT_index].nodes[ILT[ILT_index].nodes_count - 1];
	EN_node_type type = gn_type(end_point);
	int16_t i;
	FUNCTION_IN;

	/*hjh 2013-4-22 进路上最后一个节点是股道或无岔区段，则以倒数第2个节点开始搜索*/
	if ((type == NT_TRACK)|| (type == NT_NON_SWITCH_SECTION))
	{
		end_point = ILT[ILT_index].nodes[ILT[ILT_index].nodes_count - 2];
		type = gn_type(end_point);
	}
	/*进路上最后一个节点不是股道或无岔区段，则按照终端按钮搜索*/
	else
	{
		end_point = gb_node(ILT[ILT_index].end_button);
		type = gn_type(end_point);
	}
	/*并置和差置信号机,出站信号机，出站兼调车*/
	if (NT_JUXTAPOSE_SHUNGTING_SIGNAL == type || NT_DIFF_SHUNTING_SIGNAL == type
		//|| NT_SINGLE_SHUNTING_SIGNAL == type 
		|| NT_OUT_SIGNAL == type || NT_OUT_SHUNTING_SIGNAL == type
		|| NT_ROUTE_SIGNAL == type || NT_TRAIN_END_BUTTON == type)
	{
		result = (int16_t)(signal_nodes[end_point].property & 0xFFFF);
		/*hjh 2013-4-25 判断获取结果的类型是不是信号机*/
		for (i = 0; i < TOTAL_SIGNAL_NODE; i++)
		{
			if ((result != NO_INDEX) && IsFALSE(is_signal(result)))
			{
				/*不是信号机则沿着进路方向继续查找*/
				if (gn_direction(gb_node(ILT[ILT_index].start_button)) == DIR_UP)
				{
					result = gn_previous(result);
				}
				else
				{
					result = gn_next(result);
				}
			}
			else
			{
				break;
			}
		}

	}
	/*单置信号机*/
	if (NT_SINGLE_SHUNTING_SIGNAL == type)
	{
		result = end_point;
	}
	/*延续进路的始端信号点获取*/
	if(result == NO_INDEX && IsTRUE(is_successive_ILT(ILT_index)))
	{
		if (gn_direction(end_point)== DIR_DOWN)
		{
			result = gn_previous(gn_previous(end_point));
		}
		else
		{
			if (gn_direction(end_point)== DIR_UP)
			{
				result = gn_next(gn_next(end_point));
			}
		}
	}
	/*防护*/
	if ((result == NO_INDEX) || IsFALSE(is_signal(result)))
	{
		result = NO_INDEX;
	}
	FUNCTION_OUT;
	return result;
}

/****************************************************
函数名：   is_dead_ILTroute
功能描述： 不存在后续进路
返回值：   CI_BOOL
参数：     int16_t ILT_index
作者：	   hejh
日期：     2014/08/11
****************************************************/
CI_BOOL is_dead_ILTroute( int16_t ILT_index ) 
{
	CI_BOOL result = CI_FALSE;
	result = is_dead_node(gb_node(ILT[ILT_index].end_button),
		signal_nodes[gb_node(ILT[ILT_index].start_button)].direction,CI_FALSE);
	return result;
}

/****************************************************
函数名：   select_hold_route
功能描述： 选择合理的进路
返回值：   int16_t
参数：     int16_t ILTs[MAX_CHANGE_ROUTE][MAX_ILT_PER_ROUTES]
作者：	   hejh
日期：     2014/08/11
****************************************************/
int16_t select_hold_route(int16_t ILTs[MAX_CHANGE_ROUTE][MAX_ILT_PER_ROUTES])
{
	int16_t i,j,m,n;
	uint8_t exclude[MAX_CHANGE_ROUTE];
	int16_t only = NO_INDEX;
	int16_t result = NO_INDEX;

	memory_set(exclude,0,sizeof(exclude));
	/*检查多条进路*/
	for (i = 0; i < MAX_CHANGE_ROUTE && ILTs[i][0] != NO_INDEX; i++)
	{
		/*经过筛选，此时选出正确的延续进路有且仅有一条*/
		if (IsTRUE(is_successive_ILT(ILTs[i][0])))
		{
			only = i;
			break;
		}
		/*短进路*/
		else
		{
			/*两个按钮*/
			if ((third_button == NO_INDEX) && (fourth_button == NO_INDEX))
			{
				/*hjh 2014-2-20 添加关于进路信号机XLIII-SL处办理长进路的特殊处理，只选组合进路，不选短进路*/
				for (j = 1; j < MAX_ILT_PER_ROUTES && ILTs[i][j] != NO_INDEX; j++)
				{
					if ((ILTs[i][j] != NO_INDEX) 
						&& (gn_type(gb_node(ILT[ILTs[i][j]].end_button)) == NT_ROUTE_SIGNAL))
					{
						only = i;
						break;
					}
				}
				if (only != NO_INDEX)
				{
					break;
				}

				/*普通进路*/
				if ((ILT[ILTs[i][0]].change_button == NO_INDEX) 
					&& (ILT[ILTs[i][0]].change_button1 == NO_INDEX)
					&& (ILT[ILTs[i][0]].end_button == second_button))
				{
					only = i;
					break;
				}
			}
			/*三个按钮*/
			if ((third_button != NO_INDEX) && (fourth_button == NO_INDEX))
			{
				/*正确进路*/
				if ((ILT[ILTs[i][0]].change_button != NO_INDEX) 
					&& (ILT[ILTs[i][0]].change_button1 == NO_INDEX) 
					&& (ILT[ILTs[i][0]].change_button == second_button)
					&& (ILT[ILTs[i][0]].end_button == third_button))
				{
					only = i;
					break;
				}			
			}
			/*四个按钮*/
			if ((third_button != NO_INDEX) && (fourth_button != NO_INDEX))
			{
				/*正确进路*/
				if ((ILT[ILTs[i][0]].change_button != NO_INDEX) 
					&& (ILT[ILTs[i][0]].change_button1 != NO_INDEX)
					&& (ILT[ILTs[i][0]].change_button == second_button)
					&& (ILT[ILTs[i][0]].change_button1 == third_button)
					&& (ILT[ILTs[i][0]].end_button == fourth_button))
				{
					only = i;
					break;
				}
			}
		}		
	}

	/*多条进路中，如果有且只有一条是短进路，则以此为最合适的进路*/
	if (only > NO_INDEX)
	{
		result = only;
	}
	/*长进路选择*/
	else
	{   
		/*2013-3-6 hjh 长进路选择时优先选择基本进路*/
		/*长进路上的短进路均要求是基本进路，不能选出变更进路*/
		while (ILTs[result + 1][0] != NO_INDEX)
		{
			/*短进路不允许是变更进路*/	
			result++;
			for (n = 0; n < MAX_ILT_PER_ROUTES && ILTs[0][n] != NO_INDEX; n++)
			{
				/*短进路存在变更进路则将其删除*/
				if ((ILT[ILTs[result][n]].change_button != NO_INDEX)
					|| (ILT[ILTs[result][n]].change_button1 != NO_INDEX))
				{
					/*先将本区域清零,-1*/
					for (n = 0; n < MAX_ILT_PER_ROUTES && ILTs[result][n] != NO_INDEX; n++)
					{
						ILTs[result][n] = NO_INDEX;
					}
					/*将后面的数据填充至本区域*/
					for (m = result; m < MAX_CHANGE_ROUTE && ILTs[m + 1][0] != NO_INDEX; m++)
					{
						for (n = 0; n < MAX_ILT_PER_ROUTES && ILTs[m + 1][n] != NO_INDEX; n++)
						{
							ILTs[m][n] = ILTs[m + 1][n];
							ILTs[m + 1][n] = NO_INDEX;
						}
					}
					result--;
					break;
				}
			}
		}

		/*存在多条符合要求的长进路*/
		if (result > 0)
		{
			/*hjh 2013-3-21 只选择排在最前面的一条进路，方便配置*/
			/*若搜索到的进路与要求不一致只需更改联锁表中的顺序即可*/
			if (ILTs[0][0] != NO_INDEX)
			{
				result = 0;
			}
			else
			{
				result = NO_INDEX;
			}
		}
	}
	return result;
}

/****************************************************
函数名：   hold_route_check_ci_condition
功能描述： 非进路调车检查联锁条件
返回值：   CI_BOOL
参数：     route_t route_index
作者：	   hejh
日期：     2014/08/11
****************************************************/
CI_BOOL hold_route_check_ci_condition(route_t route_index)
{
	CI_BOOL result = CI_TRUE;

	FUNCTION_IN;

	/*判断该进路已选出*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*检查进路上的信号点没有设备错误状态*/
		if (IsFALSE(is_all_nodes_device_ok(route_index))) 
		{
			//result = CI_FALSE;
		}	 
		/*跨咽喉调车条件检查*/
		if (IsTRUE(result) && IsFALSE(check_throat_shungting(route_index)))
		{
			process_warning(ERR_THROAT_SHUNTING, gr_name(route_index));	
			result = CI_FALSE;
		}

		/*检查进路上所有的信号点未进路锁闭*/
		if (IsTRUE(result) && IsTRUE(is_any_node_route_locked(route_index)))
		{
			process_warning(ERR_SIGNAL_NODE_LOCKED, gr_name(route_index));
			result = CI_FALSE;
		}

		/*检查道岔OK*/
		if (IsTRUE(result) && IsFALSE(is_switch_ok(route_index)))
		{
			process_warning(ERR_SWITCH_LOCATION, gr_name(route_index));
			result = CI_FALSE;
		}

		/*检查并设置征用标志*/
		if (IsTRUE(result) && IsFALSE(judge_and_set_used_flag(route_index)))
		{
			process_warning(ERR_SIGNAL_NODE_USED, gr_name(route_index));
			result = CI_FALSE;
		}

		/*确认区段处于空闲状态*/
		if (IsTRUE(result) && IsFALSE(check_track_cleared(route_index,CI_TRUE)))
		{
			process_warning(ERR_SECTION_OCCUPIED, gr_name(route_index));
			result = CI_FALSE;
		}

		/*检查敌对进路未建立*/
		if (IsTRUE(result) && IsTRUE(check_conflict_signal(route_index)))
		{
			process_warning(ERR_CONFLICT_ROUTE, gr_name(route_index));
			result = CI_FALSE;
		}

		/*检查迎面敌对信号未开放*/
		if (IsTRUE(result) && IsTRUE(check_face_conflict(route_index)))
		{
			process_warning(ERR_FACE_CONFLICT, gr_name(route_index));
			result = CI_FALSE;
		}
		/*确认办理进路的咽喉区未办理咽喉区总锁闭*/
		if (IsTRUE(result) && IsTRUE(check_throat_locked(route_index)))
		{
			process_warning(ERR_GUIDE_ALL_LOCKED, gr_name(route_index));
			result = CI_FALSE;
		}	

		/*检查进路相关的侵限条件是否满足*/
		if (IsTRUE(result) && IsTRUE(check_exceed_limit(route_index)))
		{
			process_warning(ERR_EXCEED_LIMIT_OCCUPIED, gr_name(route_index));
			result = CI_FALSE;
		}

		/*检查进路上的联系条件*/
		if (IsTRUE(result) && IsFALSE(check_relation_condition(route_index)))
		{
			process_warning(ERR_RELATION_CONDITION, gr_name(route_index));
			result = CI_FALSE;
		}

		/*检查中间道岔*/
		if (IsTRUE(result) && IsFALSE(is_middle_switch_ok(route_index)))
		{
			process_warning(ERR_SWITCH_LOCATION, gr_name(route_index));
			result = CI_FALSE;
		}

		/*检查特殊防护道岔*/
		if (IsTRUE(result) && IsFALSE(is_special_switch_ok(route_index)))
		{
			process_warning(ERR_SWITCH_LOCATION, gr_name(route_index));
			result = CI_FALSE;
		}

		/*设置进路状态为联锁条件检查完毕，否则建立进路失败*/
		if(IsFALSE(result))
		{
			/*清除进路中及与进路相关信号节点的征用标志*/
			clear_ci_used_flag(route_index);
		}
	}
	FUNCTION_OUT;
	return result;
}

/****************************************************
函数名：   hold_route_check_select_complete
功能描述： 非进路调车选排一致
返回值：   CI_BOOL
参数：     route_t route_index
作者：	   hejh
日期：     2014/08/11
****************************************************/
CI_BOOL hold_route_check_select_complete(route_t route_index)
{	
	int16_t i,j;
	int16_t switch_count = 0;
	int16_t another_index = 0;
	CI_BOOL result = CI_FALSE;
	CI_BOOL in_route_flag = CI_FALSE;

	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		switch_count = gr_switches_count(route_index);
		/*检查进路中是否有双动道岔*/	
		for (i = 0; i < switch_count;i++)
		{
			if (IsTRUE(is_follow_switch(route_index,i)))
			{
				continue;
			}
			else
			{
				/*hjh 2014-2-20 不检查不在进路上的防护道岔*/
				in_route_flag = CI_FALSE;
				for (j = 0; j < gr_nodes_count(route_index); j++)
				{
					if ((NT_SWITCH == gn_type(gr_node(route_index,j)))
						&& ((gr_switch(route_index,i) == gr_node(route_index,j)) || (gr_switch(route_index,i) == gn_another_switch(gr_node(route_index,j)))))
					{
						in_route_flag = CI_TRUE;
					}
				}
				if (IsTRUE(in_route_flag))
				{
					another_index = gn_another_switch(gr_switch(route_index,i));
					/*若该道岔为双动道岔，则检查双动道岔位置是否一致*/
					if (another_index != NO_INDEX)
					{
						/*检查双动道岔位置是否一致*/
						if (gn_switch_state(gr_switch(route_index,i)) != gn_switch_state(another_index))
						{
							/*检查道岔动作是否超时*/
							if (IsTRUE(is_complete_timer(route_index)))
							{
								process_warning(ERR_SWITCH_OUTTIME,gn_name(gr_switch(route_index,i)));
								CIHmi_SendNormalTips("不能转岔：%s",gn_name(gr_switch(route_index,i)));
							}					
							break;
						}
					}	
					/*检查道岔实际位置和进路要求位置是否一致*/
					if (gn_switch_state(gr_switch(route_index,i)) != gr_switch_location(route_index,i))
					{
						/*检查道岔动作是否超时*/
						if (IsTRUE(is_complete_timer(route_index)))
						{
							process_warning(ERR_SWITCH_OUTTIME,gn_name(gr_switch(route_index,i)));
							CIHmi_SendNormalTips("不能转岔：%s",gn_name(gr_switch(route_index,i)));
						}					
						break;
					}
				}
			}
		}
		/*判断进路中所有的道岔是否转换到位*/
		if (i == switch_count)
		{
			result = CI_TRUE;
		}
	}

	return result;
}

/****************************************************
函数名：   hold_route_check_signal_condition
功能描述： 非进路调车信号条件检查
返回值：   CI_BOOL
参数：     route_t route_index
作者：	   hejh
日期：     2014/08/11
****************************************************/
CI_BOOL hold_route_check_signal_condition(route_t route_index)
{
	CI_BOOL result = CI_TRUE;

	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*确认区段处于空闲状态*/
		if (IsFALSE(check_track_cleared(route_index,CI_FALSE)))
		{
			process_warning(ERR_SECTION_OCCUPIED, gr_name(route_index));
			result = CI_FALSE;
		}

		/*检查进路相关的侵限条件是否满足*/
		if (IsTRUE(check_exceed_limit(route_index)))
		{
			process_warning(ERR_EXCEED_LIMIT_OCCUPIED, gr_name(route_index));
			result = CI_FALSE;
		}

		/*检查敌对进路未建立*/
		if (IsTRUE(check_conflict_signal(route_index)))
		{
			process_warning(ERR_CONFLICT_ROUTE, gr_name(route_index));
			result = CI_FALSE;
		}

		/*检查迎面敌对信号未开放*/
		if (IsTRUE(check_face_conflict(route_index)))
		{
			process_warning(ERR_FACE_CONFLICT, gr_name(route_index));
			result = CI_FALSE;
		}

		/*检查进路上的联系条件*/
		if (IsFALSE(is_successive_route(route_index)) && IsFALSE(check_relation_condition(route_index)))
		{
			result = CI_FALSE;
		}

		/*道岔位置检查*/	
		if (IsFALSE(check_switch_location(route_index)) 
			|| IsFALSE(check_middle_switch_location(route_index))
			|| IsFALSE(check_special_switch_location(route_index)))
		{
			process_warning(ERR_SWITCH_LOCATION, gr_name(route_index));
			result = CI_FALSE;
		}
	}	

	FUNCTION_OUT;
	return result;
}

/****************************************************
函数名：   hold_route_check_open_signal
功能描述： 非进路调车检查开放信号条件
返回值：   CI_BOOL
参数：     route_t route_index
作者：	   hejh
日期：     2014/08/11
****************************************************/
CI_BOOL hold_route_check_open_signal(route_t route_index)
{
	CI_BOOL result = CI_TRUE;

	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*区段空闲检查*/
		if (IsFALSE(check_track_cleared(route_index,CI_FALSE)))
		{
			process_warning(ERR_SECTION_OCCUPIED,gr_name(route_index));
			result = CI_FALSE;
		} 	
		/*侵限检查*/
		if (IsTRUE(check_exceed_limit(route_index))) 
		{
			process_warning(ERR_EXCEED_LIMIT_OCCUPIED,gr_name(route_index));
			result = CI_FALSE;
		}

		/*迎面敌对检查*/
		if (IsTRUE(check_face_conflict(route_index)))  
		{
			process_warning(ERR_FACE_CONFLICT,gr_name(route_index));
			result = CI_FALSE;
		}

		/*联系条件检查*/
		if (IsFALSE(is_relation_condition_ok(route_index)))
		{
			process_warning(ERR_RELATION_CONDITION,gr_name(route_index));
			result = CI_FALSE;
		}

		/*道岔位置检查*/
		if (IsFALSE(check_switch_location(route_index))
			|| IsFALSE(check_middle_switch_location(route_index))
			|| IsFALSE(keep_signal_middle_switch_ok(route_index))
			|| IsFALSE(check_special_switch_location(route_index)))
		{
			process_warning(ERR_SWITCH_LOCATION,gr_name(route_index));
			result = CI_FALSE;
		}

		/*检查进路上的信号点均被锁闭*/
		if (IsFALSE(check_node_route_locked(route_index)))
		{
			process_warning(ERR_NODE_UNLOCK,gr_name(route_index));
			result = CI_FALSE;
		}
	}
	FUNCTION_OUT;
	return result;
}

/****************************************************
函数名：   hold_route_keep_signal
功能描述： 非进路调车信号保持
返回值：   CI_BOOL
参数：     route_t route_index
作者：	   hejh
日期：     2014/08/12
****************************************************/
CI_BOOL hold_route_keep_signal(route_t route_index)
{
	CI_BOOL result = CI_TRUE;

	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*区段空闲检查*/
		if (IsFALSE(check_track_cleared(route_index,CI_FALSE)))
		{
			//process_warning(ERR_SECTION_OCCUPIED,gr_name(route_index));
			//result = CI_FALSE;
		} 	
		/*侵限检查*/
		if (IsTRUE(check_exceed_limit(route_index))) 
		{
			process_warning(ERR_EXCEED_LIMIT_OCCUPIED,gr_name(route_index));
			result = CI_FALSE;
		}

		/*迎面敌对检查*/
		if (IsTRUE(check_face_conflict(route_index)))  
		{
			process_warning(ERR_FACE_CONFLICT,gr_name(route_index));
			result = CI_FALSE;
		}

		/*联系条件检查*/
		if (IsFALSE(is_relation_condition_ok(route_index)))
		{
			process_warning(ERR_RELATION_CONDITION,gr_name(route_index));
			result = CI_FALSE;
		}

		/*道岔位置检查*/
		if (IsFALSE(check_switch_location(route_index))
			|| IsFALSE(check_middle_switch_location(route_index))
			|| IsFALSE(keep_signal_middle_switch_ok(route_index))
			|| IsFALSE(check_special_switch_location(route_index)))
		{
			process_warning(ERR_SWITCH_LOCATION,gr_name(route_index));
			result = CI_FALSE;
		}

		/*检查进路上的信号点均被锁闭*/
		if (IsFALSE(check_node_route_locked(route_index)))
		{
			process_warning(ERR_NODE_UNLOCK,gr_name(route_index));
			result = CI_FALSE;
		}
	}
	FUNCTION_OUT;
	return result;
}

/****************************************************
函数名：   hold_route_signal_closed
功能描述： 取消非进路调车时检查信号关闭
返回值：   CI_BOOL
作者：	   hejh
日期：     2014/08/12
****************************************************/
CI_BOOL hold_route_signal_closed()
{
	uint8_t i,j;
	node_t node_index = NO_INDEX;
	CI_BOOL result = CI_TRUE;

	/*检查信号机已关闭*/
	/*遍历进路表，查找非进路调车相关的进路*/
	for (i = 0; i < MAX_ROUTE; i++)
	{
		if (routes[i].other_flag == ROF_HOLD_ROUTE_SHUNTING)
		{
			for (j = 0; j < gr_nodes_count(i); j++)
			{
				node_index = gr_node(i,j);
				if (IsTRUE(is_shunting_signal(node_index)))
				{
					if (gn_signal_state(node_index) != SGS_A)
					{
						send_signal_command(node_index,SGS_A);
						result = CI_FALSE;
					}
				}
			}
		}
	}
	return result;
}

/****************************************************
函数名：   hold_route_section_cleared
功能描述： 取消非进路调车时检查区段出清
返回值：   CI_BOOL
参数：     int16_t special_index
作者：	   hejh
日期：     2014/08/12
****************************************************/
CI_BOOL hold_route_section_cleared(int16_t special_index)
{
	uint8_t i;
	node_t node_index = NO_INDEX;
	CI_BOOL result = CI_TRUE;

	/*检查相关区段已出清*/
	if (special_index != NO_INDEX)
	{
		for (i = 0; i < MAX_HOLD_ROUTE_SECTIONS; i++)
		{
			node_index = gs_hold_route_shunting_section(special_index,i);
			if (gn_section_state(node_index) != SCS_CLEARED)
			{
				result = CI_FALSE;
				process_warning(ERR_BLOCK_OCCUPIED,gn_name(node_index));
				break;
			}
		}
	}
	return result;
}

/****************************************************
函数名：   gs_hrs_state
功能描述： 获取非进路调车状态
返回值：   EN_hold_route_shunting_state
参数：     int16_t index
作者：	   hejh
日期：     2014/08/12
****************************************************/
EN_hold_route_shunting_state gs_hrs_state(int16_t index)
{
	return hold_route_shunting_config[index].state;
}

/****************************************************
函数名：   ss_hrs_state
功能描述： 设置非进路调车状态
返回值：   void
参数：     int16_t index
参数：     EN_hold_route_shunting_state state
作者：	   hejh
日期：     2014/08/12
****************************************************/
void ss_hrs_state(int16_t index,EN_hold_route_shunting_state state)
{
	hold_route_shunting_config[index].state = state;
}

/****************************************************
函数名：   gs_hold_route_shunting_index
功能描述： 获取非进路调车特殊索引号
返回值：   int16_t
参数：     node_t node_index
作者：	   hejh
日期：     2014/08/11
****************************************************/
int16_t gs_hold_route_shunting_index(node_t node_index)
{
	uint8_t i;
	int16_t result = NO_INDEX;

	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE) )
	{
		process_warning(ERR_INDEX,"");
		result = NO_INDEX;
	}
	else
	{
		for (i = 0; i < TOTAL_HOLD_SHUNGTING_ROUTE; i++)
		{
			if (hold_route_shunting_config[i].start_signal == node_index)
			{
				result = i;
			}
		}
	}	

	return result;
}

/****************************************************
函数名：   gs_hold_route_shunting_start_signal
功能描述： 获取非进路调车始端信号的索引号
返回值：   node_t
参数：     special_t special
作者：	   hejh
日期：     2014/08/11
****************************************************/
node_t gs_hold_route_shunting_start_signal(special_t special)
{
	node_t result = NO_INDEX;

	/*参数检查*/
	if ((special < 0) || (special >= TOTAL_HOLD_SHUNGTING_ROUTE))
	{
		process_warning(ERR_INDEX,"");
		result = NO_INDEX;
	}
	else
	{
		/*获取非进路调车始端信号的索引号*/
		result = hold_route_shunting_config[special].start_signal;
	}
	return result;
}

/****************************************************
函数名：   gs_hold_route_shunting_end_signal
功能描述： 获取非进路调车终端信号的索引号
返回值：   node_t
参数：     special_t special
作者：	   hejh
日期：     2014/08/11
****************************************************/
node_t gs_hold_route_shunting_end_signal(special_t special)
{
	node_t result = NO_INDEX;

	/*参数检查*/
	if ((special < 0) || (special >= TOTAL_HOLD_SHUNGTING_ROUTE))
	{
		process_warning(ERR_INDEX,"");
		result = NO_INDEX;
	}
	else
	{
		/*获取非进路调车终端信号的索引号*/
		result = hold_route_shunting_config[special].end_signal;
	}
	return result;
}

/****************************************************
函数名：   gs_hold_route_shunting_FJL
功能描述： 获取非进路调车FJL表示灯的索引号
返回值：   node_t
参数：     special_t special
作者：	   hejh
日期：     2014/08/11
****************************************************/
node_t gs_hold_route_shunting_FJL(special_t special)
{
	node_t result = NO_INDEX;

	/*参数检查*/
	if ((special < 0) || (special >= TOTAL_HOLD_SHUNGTING_ROUTE))
	{
		process_warning(ERR_INDEX,"");
		result = NO_INDEX;
	}
	else
	{
		/*获取非进路调车表示灯的索引号*/
		result = hold_route_shunting_config[special].FJL_indicatton_lamp;
	}
	return result;
}

/****************************************************
函数名：   gs_hold_route_shunting_FJLGZ
功能描述： 获取非进路调车FJLGZ表示灯的索引号
返回值：   node_t
参数：     special_t special
作者：	   hejh
日期：     2014/08/11
****************************************************/
node_t gs_hold_route_shunting_FJLGZ(special_t special)
{
	node_t result = NO_INDEX;

	/*参数检查*/
	if ((special < 0) || (special >= TOTAL_HOLD_SHUNGTING_ROUTE))
	{
		process_warning(ERR_INDEX,"");
		result = NO_INDEX;
	}
	else
	{
		/*获取非进路调车表示灯的索引号*/
		result = hold_route_shunting_config[special].FJLGZ_indicatton_lamp;
	}
	return result;
}

/****************************************************
函数名：   gs_hold_route_shunting_section
功能描述： 获取非进路调车检查区段的索引号
返回值：   node_t
参数：     special_t special
参数：     uint8_t i
作者：	   hejh
日期：     2014/08/11
****************************************************/
node_t gs_hold_route_shunting_section(special_t special,uint8_t i)
{
	node_t result = NO_INDEX;

	/*参数检查*/
	if ((special < 0) || (special >= TOTAL_HOLD_SHUNGTING_ROUTE))
	{
		process_warning(ERR_INDEX,"");
		result = NO_INDEX;
	}
	else
	{
		/*获取非进路调车检查区段的索引号*/
		result = hold_route_shunting_config[special].check_sections[i];
	}
	return result;
}
