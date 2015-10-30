/***************************************************************************************
Copyright (C), 2012,  Co.Hengjun, Ltd.
文件名:  cancel_route.c
作者:    LYC
版本 :   1.2	
创建日期:2012/3/27
用途:    取消进路模块
历史修改记录: 
2012/7/19,V1.1，LYC:
	1.在command_cancel_route函数中增加了进路状态为“进路已锁闭”时也可以取消进路的代码。

2012/8/14,V1.2，HJH:
	1.修改了"自动解锁判断区段故障","改方运行时离去区段占用时信号不关闭的问题;
	2.修改改方运行时离去区段占用时信号不关闭的问题，列车进路始端信号断丝时的信号降级显示问题，
	  通过进路的接车进路和发车进路在显示上要有联系;
	3.修改接近区段的判断BUG;
	4.修改信号保持中的BUG（断丝）;
2013/01/24,v1.2.1,LYC
	修改了侵限区段占用无法取消进路的BUG
2013/8/2 V1.2.1	LYC 
	CR_check_condition增加中间和防护道岔位置检查
***************************************************************************************/
#include "cancel_route.h"
#include "check_lock_route.h"
#include "keep_signal.h"


/****************************************************
函数名:    cancel_route
功能描述:  被进路控制模块调用的取消进路模块
返回值:    无
参数:      route_index
作者  :    LYC
日期  ：   2012/3/26
****************************************************/
void cancel_route(route_t route_index)
{
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*判断该进路是否存在*/
		if (IsTRUE(is_route_exist(route_index)))
		{
			/*判断进路状态是取消进路正在非正常关闭信号*/
			if (RS_CR_A_SIGNAL_CLOSING == gr_state(route_index))
			{
				CR_signal_closoing_disposal(route_index);
			}
		}
	}
}

/****************************************************
函数名:    command_cancel_route
功能描述:  被命令处理模块调用的取消进路
返回值:  
参数:      route_index

作者  :    LYC
日期  ：   2012/4/16
****************************************************/
void command_cancel_route(route_t route_index)
{
	int16_t temp;
	
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*hjh 20150611 取消进路时需必须检查列车位置状态机*/
		if ((gr_state_machine(route_index) == RSM_INIT)
			|| (gr_state_machine(route_index) == RSM_TRAIN_APPROACH)
			|| (gr_state_machine(route_index) == RSM_TRAIN_ABNORMAL_APPROACH)
			|| (gr_state_machine(route_index) == RSM_TRAIN_IN_POSSIBLE))
		{
			EN_route_state rs = gr_state(route_index);
			/*根据不同的进路状态执行操作*/
			switch(rs)
			{
				/*进路状态为"信号开放模块正在开放信号"*/
				case RS_SO_SIGNAL_OPENING:
					CR_A_signal_closed(route_index);
					break;

					/*进路状态为"重复开放信号模块正在开放信号"*/
				case RS_SRO_SIGNAL_OPENING:
					CR_A_signal_closed(route_index);
					break;

					/*进路状态为"信号已开放"*/
				case RS_SIGNAL_OPENED:
					signal_opened_disposal(route_index);
					break;

					/*进路状态为"信号非正常关闭"*/
				case RS_A_SIGNAL_CLOSED:
					CR_A_signal_closed(route_index);
					break;

					/*进路状态为"进路已锁闭"*/
				case RS_ROUTE_LOCKED:
					/*检查是否存在特殊联锁，表示灯*/
					if ((temp = gs_special(route_index,SPT_INDICATION_LAMP)) != NO_INDEX)
					{
						/*获取表示灯的状态*/
						if (gn_state(gs_indication_lamp(temp)) == SIO_HAS_SIGNAL)
						{
							/*设置表示灯的状态*/
							sn_state(gs_indication_lamp(temp),SIO_NO_SIGNAL);
							sr_state(route_index,RS_CR_A_SIGNAL_CLOSING);
							sr_clear_error_count(route_index);
						}
					}
					/*检查其他条件*/
					CR_check_condition(route_index);
					break;

					/*没有符合条件的直接退出本模块*/
				default:
					/*操作错误*/
					process_warning(ERR_OPERATION,gr_name(route_index));
					/*输出提示信息*/
					CIHmi_SendNormalTips("错误办理：%s",gn_name(gr_start_signal(route_index)));
					break;
			}
		}
		else
		{
			CIHmi_SendNormalTips("车在进路中");
		}
	}	
}

/****************************************************
函数名:    signal_opened_disposal
功能描述:  进路状态为“信号已开放”时的处理
返回值:  
参数:      route_index
作者  :    LYC
日期  ：   2012/3/27
****************************************************/
void signal_opened_disposal(route_t route_index)
{
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*根据进路类型关闭相应进路的信号机,并设置进路状态*/
		close_signal(route_index);
		sr_state(route_index,RS_CR_A_SIGNAL_CLOSING);
		sr_clear_error_count(route_index);
	}
}

/****************************************************
函数名:    CR_signal_closoing_disposal
功能描述:  进路状态为“取消进路模块正在非正常关闭信号”时的处理
返回值:  
参数:      route_index
作者  :    LYC
日期  ：   2012/3/27
****************************************************/
void CR_signal_closoing_disposal(route_t route_index)
{
	int16_t temp;
	
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*检查是否存在特殊联锁，表示灯*/
		if ((temp = gs_special(route_index,SPT_INDICATION_LAMP)) != NO_INDEX)
		{
			/*获取表示灯的状态*/
			if (gn_state(gs_indication_lamp(temp)) != SIO_HAS_SIGNAL)
			{
				/*表示灯已关闭后处理*/
				sr_state(route_index,RS_A_SIGNAL_CLOSED);
				sr_clear_error_count(route_index);
				CR_check_condition(route_index);
			}
			else
			{
				/*每个周期增加一次无法关闭信号的计数*/
				sr_increament_error_count(route_index);
				/*三个周期无法关闭信号则设置进路状态为错误、清除错误计数、提示错误*/
				if (gr_error_count(route_index) > MAX_ERROR_PER_COMMAND)
				{
					sr_state(route_index,RS_FAILURE);
					sr_clear_error_count(route_index);
					process_warning(ERR_COMMAND_EXECUTE,gr_name(route_index));
				}
			}
		}
		else
		{
			/*检查该进路信号是否关闭*/
			if (IsTRUE(is_signal_close(gr_start_signal(route_index))))
			{
				/*信号已关闭后处理*/
				sr_state(route_index,RS_A_SIGNAL_CLOSED);
				sr_clear_error_count(route_index);
				CR_check_condition(route_index);
			}
			/*断丝*/
			else if (gn_state(gr_start_signal(route_index)) == SGS_FILAMENT_BREAK)
			{
				process_warning(ERR_FILAMENT_BREAK,gr_name(route_index));
				sr_clear_error_count(route_index);
				CR_check_condition(route_index);
			}
			else
			{
				/*每个周期增加一次无法关闭信号的计数*/
				sr_increament_error_count(route_index);
				/*三个周期无法关闭信号则设置进路状态为错误、清除错误计数、提示错误*/
				if (gr_error_count(route_index) > MAX_ERROR_PER_COMMAND)
				{
					process_warning(ERR_COMMAND_EXECUTE,gr_name(route_index));
					sr_state(route_index,RS_FAILURE);
					sr_clear_error_count(route_index);										
				}
			}
		}
	}	
}

/****************************************************
函数名:    CR_A_signal_closed
功能描述:  进路状态为“信号非正常关闭”时的处理
返回值:  
参数:      route_index
作者  :    LYC
日期  ：   2012/3/28
****************************************************/
void CR_A_signal_closed(route_t route_index)
{
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*检查信号机显示状态，在信号关闭或断丝的情况下可以处理*/
		if(IsTRUE(is_signal_close(gr_start_signal(route_index)))
			|| (gn_signal_state(gr_start_signal(route_index)) == SGS_FILAMENT_BREAK))
		{
			close_signal(route_index);
			/*信号已关闭后处理*/
			CR_check_condition(route_index);
		}	
	}
}

/****************************************************
函数名:    CR_check_condition
功能描述:  取消进路条件检查及处理
返回值:  
参数:      route_index
作者  :    LYC
日期  ：   2012/3/27
****************************************************/
void CR_check_condition(route_t route_index)
{
	CI_BOOL result = CI_TRUE;
	route_t bwr = 0;
	node_t end_signal,last_section,last_switch_section,node;
	int16_t i;

	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		bwr = gr_backward(route_index);
		/*判断该进路不是延续进路*/
		if ((bwr != NO_INDEX) && IsTRUE(have_successive_route(bwr)))
		{
			end_signal = gb_node(gr_end_button(bwr));
			last_section = gr_forward_section(bwr,gr_node_index_route(bwr,end_signal));
			last_switch_section = gr_backward_section(bwr,gr_node_index_route(bwr,end_signal));

			if (IsTRUE(is_guide_route(bwr)))
			{
				if ((ROF_GUIDE_APPROACH == gr_other_flag(bwr)) || (RSM_FIRST_SECTION == gr_state_machine(bwr)))
				{					
					/*当前的区段是股道，股道占用，进路内方的第一个区段（股道的后方区段）也是占用的*/
					if (!(((gn_type(last_section) == NT_TRACK) || (gn_type(last_section) == NT_NON_SWITCH_SECTION))
						&& IsFALSE(is_node_locked(last_switch_section,LT_LOCKED))))
					{
						result = CI_FALSE;
						CIHmi_SendNormalTips("接车进路未解锁");
						process_warning(ERR_SUCCESSIVE_CANCLE,gr_name(route_index));
					}
				}
			}
			else
			{
				/*最后区段锁闭*/
				if (IsTRUE(is_node_locked(last_switch_section,LT_LOCKED)))
				{
					result = CI_FALSE;
					CIHmi_SendNormalTips("接车进路未解锁");
					process_warning(ERR_SUCCESSIVE_CANCLE,gr_name(route_index));
				}
				else
				{
					/*最后区段解锁，车进入进路*/
					if (RSM_TRAIN_IN_ROUTE == gr_state_machine(bwr))
					{
						for (i = 0; i < gr_nodes_count(bwr);i++)
						{
							node = gr_node(bwr,i);	
							if (IsTRUE(is_section(node)) && IsTRUE(is_node_locked(node,LT_LOCKED)) && (gn_belong_route(node) == bwr))
							{
								result = CI_FALSE;
								CIHmi_SendNormalTips("接车进路未解锁");
								process_warning(ERR_SUCCESSIVE_CANCLE,gr_name(route_index));
								break;
							}
						}
					}
				}
			}			
		}
		else
		{
			/*延续进路，车列压入接车进路则不能取消*/
			if ((bwr == NO_INDEX) && IsTRUE(is_successive_route(route_index)))
			{
				if (ROF_SUCCESSIVE_DELAY == gr_other_flag(route_index))
				{
					result = CI_FALSE;
					CIHmi_SendNormalTips("车已进入接车进路");
					process_warning(ERR_OPERATION, gr_name(route_index));
				}
			}
		}

		/*确认区段处于空闲状态*/
		if (IsFALSE(check_track_cleared(route_index,CI_FALSE)))
		{
			process_warning(ERR_SECTION_OCCUPIED, gr_name(route_index));
			result = CI_FALSE;
		}

		///*检查敌对进路未建立*/
		//if (IsTRUE(check_conflict_signal(route_index)))
		//{
		//	process_warning(ERR_CONFLICT_ROUTE, gr_name(route_index));
		//	result = CI_FALSE;
		//}

		///*检查迎面敌对信号未开放*/
		//if (IsTRUE(check_face_conflict(route_index)))
		//{
		//	process_warning(ERR_FACE_CONFLICT, gr_name(route_index));
		//	result = CI_FALSE;
		//}

		/*道岔位置检查*/	
		if (IsFALSE(check_switch_location(route_index)))
		{
			process_warning(ERR_SWITCH_LOCATION, gr_name(route_index));
			result = CI_FALSE;
		}
		/*2013/8/2 LYC 中间道岔位置检查*/	
		/*if (IsFALSE(check_middle_switch_location(route_index)))
		{
			process_tips(ERR_SWITCH_LOCATION, gr_name(route_index));
			result = CI_FALSE;
		}*/
		/*2013/8/2 LYC 特殊防护道岔位置检查*/	
		/*if (IsFALSE(check_special_switch_location(route_index)))
		{
			process_tips(ERR_SWITCH_LOCATION, gr_name(route_index));
			result = CI_FALSE;
		}*/
		/*检查信号点的进路锁闭标志*/
		if (IsFALSE(check_node_route_locked(route_index)))
		{
			result = CI_FALSE;
			process_warning(ERR_NODE_UNLOCK,gr_name(route_index));
		}

		/*信号非正常关闭时，车列运行*/
		if ((RS_A_SIGNAL_CLOSED == gr_state(route_index))
			&& ((RSM_FIRST_SECTION == gr_state_machine(route_index)) 
			|| (RSM_SECOND_SECTION == gr_state_machine(route_index))
			|| (RSM_TRAIN_IN_ROUTE == gr_state_machine(route_index))))
			//&& (SCSM_SELF_UNLOCK == gn_state_machine(gr_first_section(route_index))))
		{
			process_warning(ERR_TRAIN_IN_ROUTE, gr_name(route_index));
			result = CI_FALSE;
		}
	
		/*检查接近区段空闲（信号未开放不构成接近锁闭）*/
		//if ((RS_ROUTE_LOCKED != gr_state(route_index)) && IsFALSE(is_approach_cleared(route_index,CI_TRUE)))
		/*调车进路无接近记忆功能*/
		if (RT_SHUNTING_ROUTE == gr_type(route_index))
		{
			if (IsTRUE(result) && (RS_ROUTE_LOCKED != gr_state(route_index)) && IsFALSE(is_approach_cleared(route_index,CI_TRUE)))
			{
				result = CI_FALSE;
				process_warning(ERR_APPROACH_OCCUPIED,gr_name(route_index));
				/*输出提示信息*/
				CIHmi_SendNormalTips("接近区段占用：%s",gn_name(gr_approach(route_index)));
			}
		}
		else
		{
			if (IsTRUE(result) && ((RSM_TRAIN_APPROACH == gr_state_machine(route_index)) || (ROF_APPROACH_ADDED == gr_other_flag(route_index))))
			{
				result = CI_FALSE;
				process_warning(ERR_APPROACH_OCCUPIED,gr_name(route_index));
				/*输出提示信息*/
				CIHmi_SendNormalTips("接近区段占用：%s",gn_name(gr_approach(route_index)));
			}
		}	

		/*取消进路条件检查检查满足后删除此进路*/
		if (IsTRUE(result))
		{
			sr_state(route_index,RS_UNLOCKED);	
			delete_route(route_index);
		}
	}	
}