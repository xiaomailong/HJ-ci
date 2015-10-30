/***************************************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.
文件名:  open_signal.c
作者:    CY
版本 :   1.0	
创建日期:2011/12/2
用途:    开放信号模块
历史修改记录: 
2012/7/26,V1.1,CY:
	1.添加进路接近区段解锁判断功能
	2.解决长调车进路和通过进路分段办理后，解锁不正确的Bug。添加了link_route函数。
	3.修改了 延续进路 SX向XI的接车进路无法办理的问题 Issue #57.修改了进站信号显示颜色的代码，开放信号主要依据联锁表进行。
2012/8/10,V1.2,HJH:
	1.修改改方运行时离去区段占用时信号不关闭的问题，列车进路始端信号断丝时的信号降级显示问题，通过进路的接车进路和发车进路在显示上要有联系
2012/12/31 V1.2.1 hjh
	check_open_signal增加检查中间道岔的条件
2013/1/25 V1.2.1 hjh
	link_route对进路信号机的连接关系进行单独处理
2013/1/28 V1.2.1 hjh
	check_open_signal中增加检查特殊防护道岔的条件
2013/3/28 V1.2.1 hjh
	link_route中判断是延续进路时不连接后方进路
2013/3/28 V1.2.1 hjh
	1.open_shunting_signal中增加延续进路上的调车进路不检查信号开放的条件
	2.link_route中连接长调车进路时增加判断要连接的进路类型是调车进路
2013/6/3 V1.2.1 hjh
	link_route增加终端是列车终端按钮,解决终端按钮处的进路连接问题
2013/8/2 V1.2.1 hjh
	route_locked 开放信号之前也要对延续部分的联锁条件进行检查
2013/8/21 V1.2.1 hjh
	signal_opening增加信号机状态不是红灯或断丝时认为是信号开放颜色错误
2014/2/20 V1.2.1 hjh
	link_route判断条件增加进路类型是列车进路且终端是出站信号机时连接前后进路
2014-4-14 V1.2.2 LYC matis：3644
	link_route判断条件增加进路类型是列车进路且终端按钮是列车按钮时连接前后进路
***************************************************************************************/
#include "open_signal.h"
#include "auto_unlock.h"
#include "global_data.h"

/****************************************************
函数名:    open_signal
功能描述:  开放信号
返回值:    
参数:      int16_t route_index
作者  :    hejh
日期  ：   2012/7/23
****************************************************/
void open_signal(route_t route_index)
{
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*调车进路*/
		if (RT_SHUNTING_ROUTE == gr_type(route_index))
		{
			open_shunting_signal(route_index);
		}
		/*列车进路*/
		else
		{
			open_train_signal(route_index);
		}
	}
}

/****************************************************
函数名:    check_open_signal
功能描述:  检查列车信号开放条件
返回值:    
参数:      route_index
作者  :    CY
日期  ：   2011/12/12
****************************************************/
CI_BOOL check_open_signal(route_t route_index)
{
	CI_BOOL result = CI_TRUE;
	int16_t i = 0,si,ms;

	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*检查红灯断丝*/
		if (IsTRUE(is_red_filament(route_index)))
		{
			result = CI_FALSE;
			//CIHmi_SendNormalTips("红灯断丝：%s",gn_name(gr_start_signal(route_index)));
		}
		/*区段空闲检查*/
		if (IsTRUE(result) && IsFALSE(check_track_cleared(route_index,CI_FALSE)))
		{
			process_warning(ERR_SECTION_OCCUPIED,gr_name(route_index));
			result = CI_FALSE;
		} 	
		
		/*敌对信号检查*/
		if (IsTRUE(result) && IsTRUE(check_conflict_signal(route_index))) 
		{
			process_warning(ERR_CONFLICT_ROUTE,gr_name(route_index));
			result = CI_FALSE;
		}

		/*迎面敌对检查*/
		if (IsTRUE(result) && IsTRUE(check_face_conflict(route_index)))  
		{
			process_warning(ERR_FACE_CONFLICT,gr_name(route_index));
			result = CI_FALSE;
		}

		/*检查始端信号机未通信中断*/
		if (gn_signal_state(gr_start_signal(route_index)) == SGS_ERROR)
		{
			result = CI_FALSE;
		}

		/*联系条件检查*/
		if (IsTRUE(result) && IsFALSE(is_relation_condition_ok(route_index)))
		{
			process_warning(ERR_RELATION_CONDITION,gr_name(route_index));
			result = CI_FALSE;
		}

		/*道岔位置检查*/
		if (IsTRUE(result) && (IsFALSE(check_switch_location(route_index))
			|| IsFALSE(check_middle_switch_location(route_index))
			|| IsFALSE(keep_signal_middle_switch_ok(route_index))
			|| IsFALSE(check_special_switch_location(route_index))))
		{
			process_warning(ERR_SWITCH_LOCATION,gr_name(route_index));
			result = CI_FALSE;
		}

		/*检查进路上的信号点均被锁闭*/
		if (IsTRUE(result) && IsFALSE(check_node_route_locked(route_index)))
		{
			process_warning(ERR_NODE_UNLOCK,gr_name(route_index));
			result = CI_FALSE;
		}

		/*侵限检查*/
		if (IsTRUE(result) && IsTRUE(check_exceed_limit(route_index))) 
		{
			process_warning(ERR_EXCEED_LIMIT_OCCUPIED,gr_name(route_index));
			result = CI_FALSE;
		}

		/*设置中间道岔的历史状态*/
		if (IsTRUE(result))
		{
			si = gs_middle_switch_index(gr_start_signal(route_index));
			/*检查中间道岔是否存在*/
			if (si != NO_INDEX)
			{
				for (i = 0; i < MAX_MIDDLE_SWITCHES; i++)
				{
					/*找出中间道岔索引号*/
					ms = gs_middle_switch(si,i);
					if (ms != NO_INDEX)
					{
						sn_history_state(ms,gn_switch_state(ms));
					}
				}
			}
		}
	}
	FUNCTION_OUT;
	return result;
}

/****************************************************
函数名:    open_train_signal
功能描述:  开放列车信号
返回值:    
参数:      route_index
作者  :    CY
日期  ：   2011/12/2
****************************************************/
void open_train_signal(route_t route_index)
{
	EN_route_state rs;
	CI_BOOL error = CI_FALSE;

	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		rs = gr_state(route_index);
		/*错误处理*/
		if ((RT_TRAIN_ROUTE != gr_type(route_index)) 
			|| ((RS_ROUTE_LOCKED != rs) && (RS_SO_SIGNAL_OPENING != rs)))
		{
			error = CI_TRUE;
		}

		if (IsFALSE(error))
		{
			switch(rs)
			{
				/*进路状态为进路已锁闭*/	
				case RS_ROUTE_LOCKED:
					link_route(route_index);
					route_locked(route_index);
					break;
				/*进路状态为信号开放模块正在开放信号*/
				case RS_SO_SIGNAL_OPENING:
					if (IsTRUE(is_route_cycle_right(route_index)))
					{
						signal_opening(route_index);
					}			
					break;
			default:
				break;
			}
			/*记录当前处理周期*/
			//sr_updata_cycle(route_index);
		}
	}
	FUNCTION_OUT;
}

/****************************************************
函数名:    route_locked
功能描述:  进路状态为进路锁闭的执行过程
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/3/6
****************************************************/
void route_locked(route_t route_index)
{
	route_t fwr,bwr;
	CI_BOOL result = CI_FALSE;
	CI_BOOL run_flag = CI_TRUE;
	EN_signal_state change_result;
	int16_t start_signal = gr_start_signal(route_index);

	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		fwr = gr_forward(route_index);
		bwr = gr_backward(route_index);
		/*本进路或后一进路不是延续进路*/
		if (((bwr != NO_INDEX) && IsTRUE(have_successive_route(bwr))) || IsTRUE(is_successive_route(route_index)))
		{
			run_flag = CI_FALSE;
		}

		//if (IsTRUE(run_flag))
		//{
		//	/*检查红灯断丝*/
		//	if (IsTRUE(is_red_filament(route_index)))
		//	{
		//		run_flag = CI_FALSE;
		//	}
		//}

		if (IsTRUE(run_flag))
		{
			/*检查联锁条件*/
			if (IsTRUE(check_open_signal(route_index)))
			{
				/*存在前一进路则先开放前一进路的信号*/
				if ((fwr != NO_INDEX) && IsFALSE(have_successive_route(route_index)))
				{
					open_train_signal(fwr);
					if (gr_state(fwr) == RS_SIGNAL_OPENED)
					{
						result = CI_TRUE;
					}
					else
					{
						/*证明不是一次办理*/
						if (gr_cycle(route_index) > gr_cycle(fwr))
						{
							result = CI_TRUE;
						}
					}
				}
				/*hjh 2013-8-2 开放信号之前也要对延续部分的联锁条件进行检查*/
				else if ((fwr != NO_INDEX) && IsTRUE(have_successive_route(route_index)))
				{
					if (gr_state(fwr) == RS_SIGNAL_OPENED)
					{
						result = CI_TRUE;
					}
					else
					{
						/*检查延续部分联锁条件*/
						if (IsTRUE(check_open_signal(fwr)))
						{
							result = CI_TRUE;
						}
					}					
				}
				else
				{
					result = CI_TRUE;
				}
			}
			if (IsTRUE(result))
			{
				/*开放信号*/
				change_result = change_signal_color(route_index);
				if (change_result != SGS_H)
				{
					/*设置进路状态*/
					send_signal_command(start_signal,change_result);
					sr_state(route_index,RS_SO_SIGNAL_OPENING);
					sn_signal_expect_state(gr_start_signal(route_index),change_result);
				}
			}
		}
	}	
	FUNCTION_OUT;
}

/****************************************************
函数名:    signal_opening
功能描述:  信号正在开放执行过程
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/3/7
****************************************************/
void signal_opening( route_t route_index )
{
	int8_t i;
	node_t node_index;

	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*列车进路*/
		if(gr_type(route_index) != RT_SHUNTING_ROUTE)
		{
			/*判断信号机按照预期的颜色开放*/
			if (gn_signal_state(gr_start_signal(route_index)) == gn_signal_expect_state(gr_start_signal(route_index)))
			{
				/*设置进路状态为信号已开放*/
				sr_state(route_index,RS_SIGNAL_OPENED);
				sr_clear_error_count(route_index);	
				/*输出提示信息*/
				CIHmi_SendNormalTips("信号开放：%s，%s",gn_name(gr_start_signal(route_index)),hmi_signal_tips(gn_signal_state(gr_start_signal(route_index))));
				link_route(route_index);
			}
			else
			{
				/*信号已开放，但和预期的颜色不一致*/
				/*hjh 2013-8-21 信号机状态不是红灯或断丝时认为是信号开放颜色错误*/
				if ((gn_signal_state(gr_start_signal(route_index)) != SGS_H)
					&& (gn_signal_state(gr_start_signal(route_index)) != SGS_FILAMENT_BREAK)
					&& (gn_signal_state(gr_start_signal(route_index)) != SGS_ERROR))
				{
					/*发送红灯命令，进路状态置为信号正在非正常关闭*/
					send_signal_command(route_index,SGS_H);
					sr_state(route_index,RS_SK_A_SIGNAL_CLOSING);
					sr_clear_error_count(route_index);
					process_warning(ERR_SIGNAL_OPEN,gr_name(route_index));
				}
				/*错误计数*/
				else
				{
					sr_increament_error_count(route_index);
					/*命令执行失败*/
					if (gr_error_count(route_index) > MAX_ERROR_PER_COMMAND)
					{
						sr_clear_error_count(route_index);
						process_warning(ERR_COMMAND_EXECUTE,gr_name(route_index));
						CIHmi_SendNormalTips("信号无法开放");
					}
				}
			}
						
		}
		/*调车进路*/
		else
		{
			/*开放白灯*/
			if (SGS_B == gn_signal_state(gr_start_signal(route_index)))
			{
				/*设置进路状态为信号已开放*/
				sr_state(route_index,RS_SIGNAL_OPENED);
				sr_clear_error_count(route_index);
				CIHmi_SendNormalTips("信号开放：%s，%s",gn_name(gr_start_signal(route_index)),hmi_signal_tips(gn_signal_state(gr_start_signal(route_index))));
				link_route(route_index);
			}
			/*增加错误计数*/
			else
			{
				sr_increament_error_count(route_index);
				/*命令执行失败*/
				if (gr_error_count(route_index) >= MAX_ERROR_PER_COMMAND)
				{
					sr_clear_error_count(route_index);
				}
			}
		}
		/*设置历史状态*/
		sn_signal_history_state(gr_start_signal(route_index),gn_signal_state(gr_start_signal(route_index)));

		/*区段故障时在执行下一步操作时将其状态机置为“初始化”*/
		if (RS_SIGNAL_OPENED == gr_state(route_index))
		{
			for (i = 0; i < gr_nodes_count(route_index); i++)
			{
				node_index = gr_node(route_index,i);
				if (IsTRUE(is_section(node_index)) && IsTRUE(is_section_failure(route_index,i)))
				{
					sn_state_machine(node_index,SCSM_INIT);	
				}
			}
			sr_state_machine(route_index,RSM_INIT);
		}
	}
	FUNCTION_OUT;
}

/****************************************************
函数名:    open_shunting_signal
功能描述:  开放调车信号
返回值:    
参数:      route_index
作者  :    CY
日期  ：   2011/12/2
****************************************************/
void open_shunting_signal(route_t route_index)
{
	EN_route_state rs;
	route_t fwr,bwr;
	int16_t i,start_signal;
	CI_BOOL error = CI_FALSE;
	CI_BOOL result = CI_FALSE;

	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		rs = gr_state(route_index);
		/*检查进路类型和进路状态*/
		if (RT_SHUNTING_ROUTE != gr_type(route_index) 
			|| ((RS_ROUTE_LOCKED != rs) && (RS_SO_SIGNAL_OPENING != rs)))
		{
			error = CI_TRUE;
		}
		if (IsFALSE(error))
		{
			start_signal = gr_start_signal(route_index);
			/*进路状态为信号开放模块正在开放信号*/
			if ((RS_SO_SIGNAL_OPENING == rs) && IsTRUE(is_route_cycle_right(route_index)))
			{
				send_signal_command(start_signal,SGS_B);
				/*调车信号已开放，设置进路状态*/
				if (SGS_B == gn_signal_state(start_signal))
				{
					sr_state(route_index,RS_SIGNAL_OPENED);
					sr_clear_error_count(route_index);
					/*输出提示信息*/
					CIHmi_SendNormalTips("信号开放：%s，%s",gn_name(gr_start_signal(route_index)),hmi_signal_tips(gn_signal_state(gr_start_signal(route_index))));
				}
				/*错误计数*/
				else
				{
					sr_increament_error_count(route_index);
					if (gr_error_count(route_index) >= MAX_ERROR_PER_COMMAND)
					{
						sr_clear_error_count(route_index);
						process_warning(ERR_COMMAND_EXECUTE,gr_name(route_index));
					}
				}
			}

			/*延时开放*/
			if (RS_SO_SIGNAL_DELAY_OPENING == rs)
			{
				if (IsTRUE(check_open_signal(route_index)))
				{
					if (IsTRUE(is_timer_run(route_index)))
					{
						if (IsTRUE(is_complete_timer(route_index)))
						{
							send_signal_command(start_signal,SGS_B);
							if (gs_special(route_index,SPT_INDICATION_LAMP) != NO_INDEX)
							{
								/*设置表示灯的状态*/
								sn_state(gs_indication_lamp(gs_special(route_index,SPT_INDICATION_LAMP)),SIO_HAS_SIGNAL);
							}
							sr_stop_timer(route_index);
							sr_state(route_index,RS_SO_SIGNAL_OPENING);
						}
					}
					else
					{
						send_signal_command(start_signal,SGS_B);
						if (gs_special(route_index,SPT_INDICATION_LAMP) != NO_INDEX)
						{
							/*设置表示灯的状态*/
							sn_state(gs_indication_lamp(gs_special(route_index,SPT_INDICATION_LAMP)),SIO_HAS_SIGNAL);
						}
						sr_state(route_index,RS_SO_SIGNAL_OPENING);
					}					
				}
				else
				{
					sr_stop_timer(route_index);
					sr_state(route_index,RS_ROUTE_LOCKED);
				}
			}

			/*进路状态为进路已锁闭*/
			if (RS_ROUTE_LOCKED == rs)
			{
				/*hjh 2013-4-3 延续进路上的调车进路不检查开放信号的条件*/
				bwr = gr_backward(route_index);
				if (bwr != NO_INDEX)
				{
					/*后方进路是带有延续进路的接车进路*/
					if (IsTRUE(have_successive_route(bwr)))
					{
						error = CI_TRUE;
					}
					else
					{
						/*循环找出后方进路是带有延续进路的接车进路*/
						for (i = 0; i < TOTAL_MIDDLLE_SWITCHES; i++)
						{
							/*判断bwr不为空*/
							if (bwr == NO_INDEX)
							{
								break;	
							}
							else
							{
								bwr = gr_backward(bwr);
								if ((bwr != NO_INDEX) && IsTRUE(have_successive_route(bwr)))
								{
									error = CI_TRUE;
									break;
								}
							}
						}
					}
				}

				/*检查信号开放条件*/
				if (IsFALSE(error) && IsTRUE(check_open_signal(route_index)))
				{
					fwr = gr_forward(route_index);
					link_route(route_index);
					/*前方信号开放后才允许本进路开放信号*/
					if (NO_INDEX != fwr)
					{
						open_shunting_signal(fwr);
						if (gr_state(fwr) == RS_SIGNAL_OPENED)
						{
							result = CI_TRUE;
						}
					}
					/*不存在前方进路*/
					else
					{
						result = CI_TRUE;
					}

					/*信号延时开放*/
					for (i = 0; i < MAX_SIGNAL_DELAY_OPEN; i++)
					{
						if ((signal_delay_open_config[i].SignalIndex != NO_INDEX)
							&& (start_signal == signal_delay_open_config[i].SignalIndex))
						{
							sr_start_timer(route_index,signal_delay_open_config[i].TimerCounter,DTT_OTHER);
							sr_state(route_index,RS_SO_SIGNAL_DELAY_OPENING);
							CIHmi_SendNormalTips("信号延时开放：%s",gn_name(start_signal));
							result = CI_FALSE;
							break;
						}
					}

					/*开放调车信号*/
					if (IsTRUE(result))
					{
						send_signal_command(start_signal,SGS_B);
						if (gs_special(route_index,SPT_INDICATION_LAMP) != NO_INDEX)
						{
							/*设置表示灯的状态*/
							sn_state(gs_indication_lamp(gs_special(route_index,SPT_INDICATION_LAMP)),SIO_HAS_SIGNAL);
						}
						sr_state(route_index,RS_SO_SIGNAL_OPENING);
					} 
				}
			}

			/*更新当前周期*/
			sr_updata_cycle(route_index);
		}
	}
	FUNCTION_OUT;
}


/****************************************************
函数名:    link_route
功能描述:  连接已开放信号的进路
返回值:    
参数:      route_t route_index
作者  :    CY
日期  ：   2012/7/24
****************************************************/
void link_route(route_t route_index)
{
	int16_t end,start;
	route_t i;

	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*进路状态为进路已锁闭*/
		if ((gr_state(route_index) == RS_ROUTE_LOCKED) || (gr_state(route_index) == RS_SIGNAL_OPENED))
		{	
			/*获取始端和终端信号*/
			end = gb_node(gr_end_button(route_index));
			start = gr_start_signal(route_index);
			/*关联前方进路*/
			if (gr_forward(route_index)== NO_INDEX)
			{
				/*终端信号机类型是进路信号机*/
				if (gn_type(end) == NT_ROUTE_SIGNAL)
				{
					/*遍历进路表中的所有进路*/
					for (i=0; i < MAX_ROUTE; i++)
					{
						/*将本进路的终端作为后方进路的始端*/
						if ((i != route_index)  && IsTRUE(is_route_exist(i)) 
							&& (gr_backward(i) == NO_INDEX) && (gr_start_signal(i) == end)
							&& (gr_direction(route_index) == gr_direction(i))
							&& (gr_type(i) == gr_type(route_index))
							&& (gr_state(i) == RS_SIGNAL_OPENED))
						{
							sr_backward(i,route_index);
							sr_forward(route_index,i);					
							break;
						}
						/*将本进路的终端对应的另一架信号机作为后方进路的始端*/
						if ((i != route_index) && IsTRUE(is_route_exist(i))
							&& (gr_backward(i) == NO_INDEX) && (gr_start_signal(i) == gn_another_signal(end))
							&& (gr_direction(route_index) == gr_direction(i))
							&& (gr_type(i) == gr_type(route_index))
							&& (gr_state(i) == RS_SIGNAL_OPENED))
						{
							sr_backward(i,route_index);
							sr_forward(route_index,i);					
							break;
						}
					}
				}
				/*终端信号机类型是单置调车*/
				else if (gn_type(end) == NT_SINGLE_SHUNTING_SIGNAL)
				{
					/*遍历进路表中的所有进路*/
					for (i=0; i < MAX_ROUTE; i++)
					{
						/*将本进路的终端作为后方进路的始端*/
						if ((i != route_index)  && IsTRUE(is_route_exist(i)) && (gr_backward(i) == NO_INDEX) 
							&& (gr_start_signal(i) == end)  && (gr_type(i) == RT_SHUNTING_ROUTE)
							&& (gr_direction(route_index) == gr_direction(i))
							&& (gr_type(i) == gr_type(route_index))
							&& (gr_state(i) == RS_SIGNAL_OPENED))
						{
							sr_backward(i,route_index);
							sr_forward(route_index,i);	
							break;
						}
					}
				}
				/*终端信号机类型是差，并置调车或出站信号机*/
				else if (gn_type(end) == NT_DIFF_SHUNTING_SIGNAL || gn_type(end) == NT_JUXTAPOSE_SHUNGTING_SIGNAL
					|| gn_type(end) == NT_OUT_SHUNTING_SIGNAL || gn_type(end) == NT_OUT_SIGNAL	)
				{
					/*LYC 2014-4-14 终端是列车按钮,解决终端按钮处的进路连接问题*/

					/*遍历进路表中的所有进路*/
					for (i=0; i < MAX_ROUTE; i++)
					{
						/*将本进路的终端对应的另一架信号机作为后方进路的始端*/
						/*hjh 2013-4-3 判断条件增加进路类型是调车进路*/
						if ((i != route_index) && IsTRUE(is_route_exist(i)) && (gr_backward(i) == NO_INDEX)
							&& (gr_start_signal(i) == gn_another_signal(end)) && (gr_type(i) == gr_type(route_index))
							&& (gr_direction(route_index) == gr_direction(i))
							&& (gr_state(i) == RS_SIGNAL_OPENED))
						{
							sr_backward(i,route_index);
							sr_forward(route_index,i);					
							break;
						}
						/*hjh 2014-2-20 判断条件增加进路类型是列车进路且终端是列车按钮*/
						if ((i != route_index) && IsTRUE(is_route_exist(i)) && (gr_backward(i) == NO_INDEX)
							&& (gr_start_signal(i) == gn_another_signal(end)) && (gr_type(i) == RT_TRAIN_ROUTE)
							&& ((gn_type(end) == NT_OUT_SIGNAL) || (gn_type(end) == NT_OUT_SHUNTING_SIGNAL))
							&& (gr_direction(route_index) == gr_direction(i))
							&& (gr_type(i) == gr_type(route_index))
							&& (gr_state(i) == RS_SIGNAL_OPENED))
						{
							sr_backward(i,route_index);
							sr_forward(route_index,i);					
							break;
						}

						/*2014-4-14 LYC 判断条件增加进路类型是列车进路且终端按钮是列车按钮*/
						if ((i != route_index) && IsTRUE(is_route_exist(i)) && (gr_backward(i) == NO_INDEX)
							&& (gr_start_signal(i) == gn_another_signal(end)) && (gr_type(i) == RT_TRAIN_ROUTE)
							&& ((gn_type(end) == NT_OUT_SIGNAL) || (gb_type(gr_end_button(route_index)) == BT_TRAIN))
							&& (gr_direction(route_index) == gr_direction(i))
							&& (gr_type(i) == gr_type(route_index))
							&& (gr_state(i) == RS_SIGNAL_OPENED))
						{
							sr_backward(i,route_index);
							sr_forward(route_index,i);					
							break;
						}							
					}
				}
				/*hjh 2013-6-3 终端是列车终端按钮,解决终端按钮处的进路连接问题*/
				else if (gn_type(end) == NT_TRAIN_END_BUTTON)
				{
					/*遍历进路表中的所有进路*/
					for (i=0; i < MAX_ROUTE; i++)
					{
						/*将本进路的终端对应的另一架信号机作为后方进路的始端*/
						if ((i != route_index) && IsTRUE(is_route_exist(i)) && (gr_backward(i) == NO_INDEX)
							&& (gr_start_signal(i) == gn_another_signal(end)) && (gr_type(i) == RT_TRAIN_ROUTE)
							&& (gr_direction(route_index) == gr_direction(i))
							&& (gr_type(i) == gr_type(route_index))
							&& (gr_state(i) == RS_SIGNAL_OPENED))
						{
							sr_backward(i,route_index);
							sr_forward(route_index,i);					
							break;
						}
					}
				}
			}

			/*关联后方进路*/
			if ((gr_backward(route_index) == NO_INDEX) && (gr_state(route_index) == RS_SIGNAL_OPENED))
			{
				/*终端信号机类型是进路信号机*/
				if ((gn_type(start) == NT_ROUTE_SIGNAL) || ((gn_type(start) == NT_OUT_SHUNTING_SIGNAL)) || ((gn_type(start) == NT_OUT_SIGNAL)))
				{
					/*遍历进路表中的所有进路*/
					for (i=0; i < MAX_ROUTE; i++)
					{
						/*将本进路的始端作为后方进路的终端*/
						if ((i != route_index) && IsTRUE(is_route_exist(i)) && (gr_forward(i) == NO_INDEX) 
							&& (gb_node(gr_end_button(i)) == start) && (gr_type(i) == gr_type(route_index))
							&& (gr_direction(route_index) == gr_direction(i)))
						{
							sr_backward(route_index,i);
							sr_forward(i,route_index);					
							break;
						}
						/*将本进路的始端对应的另一架信号机作为后方进路的终端*/
						if ((i != route_index) && IsTRUE(is_route_exist(i)) &&(gr_forward(i) == NO_INDEX )
							&& (gn_another_signal(gb_node(gr_end_button(i))) == start) && (gr_type(i) == gr_type(route_index))
							&& (gr_direction(route_index) == gr_direction(i)))
						{
							if (IsFALSE(have_successive_route(i)))
							{
								sr_backward(route_index,i);
								sr_forward(i,route_index);					
								break;
							}							
						}
					}
				}				
				/*终端信号机类型是单置调车*/
				if (gn_type(start) == NT_SINGLE_SHUNTING_SIGNAL)
				{
					/*遍历进路表中的所有进路*/
					for (i=0; i < MAX_ROUTE; i++)
					{
						/*将本进路的始端作为后方进路的终端*/
						if ((i != route_index) && IsTRUE(is_route_exist(i)) && (gr_forward(i) == NO_INDEX)  
							&& (gb_node(gr_end_button(i)) == start)  && (gr_type(i) == RT_SHUNTING_ROUTE)
							&& (gr_direction(route_index) == gr_direction(i))
							&& (gr_type(i) == gr_type(route_index)))
						{
							sr_backward(route_index,i);
							sr_forward(i,route_index);					
							break;
						}
					}
				}
				else if (gn_type(start) == NT_DIFF_SHUNTING_SIGNAL || gn_type(start) == NT_JUXTAPOSE_SHUNGTING_SIGNAL
					|| gn_type(start) == NT_OUT_SHUNTING_SIGNAL || gn_type(start) == NT_OUT_SIGNAL )
				{
					/*hjh 2013-3-28 非延续进路*/
					if (IsFALSE(is_successive_route(route_index)))
					{
						/*遍历进路表中的所有进路*/
						for (i=0; i < MAX_ROUTE; i++)
						{
							/*将本进路的始端对应的另一架信号机作为后方进路的终端*/
							if ((i != route_index) && IsTRUE(is_route_exist(i)) && (gr_forward(i) == NO_INDEX )
								&& (gn_another_signal(gb_node(gr_end_button(i))) == start)  && (gr_type(i) == RT_SHUNTING_ROUTE)
								&& (gr_direction(route_index) == gr_direction(i))
								&& (gr_type(i) == gr_type(route_index)))
							{
								sr_backward(route_index,i);
								sr_forward(i,route_index);					
								break;
							}
						}
					}
				}
			}
		}
	}
}



