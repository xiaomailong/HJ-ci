/***************************************************************************************
Copyright (C), 2012,  Co.Hengjun, Ltd.
文件名:  relay_cancel_route.c
作者:    LYC
版本 :   1.2	
创建日期:2012/3/28
用途:    人工延时解锁
历史修改记录: 
2012/7/19,V1.1，HJH:
	1.修改引导进路的判断方法，由gr_type修改为gr_other_flag，并删除RT_GUIDE_ROUTE和RT_SUCCESSIVE_ROUTE；
	2.修改函数，发现原来有is_guide_route（），故删除gr_other_flag（）；
	3.修改了提示信息。根据实际情况将错误信息，警告信息，提示信息正确分类处理；修改了部分Bug；	
2012/8/14,V1.2，HJH:
	1.增加了取消进路模块和人工延时解锁模块中，进路状态为“进路已锁闭”时也可以取消进路的代码。
	2.修改联锁程序中的BUG，自动解锁中判断区段故障；
	3.修改改方运行时离去区段占用时信号不关闭的问题，列车进路始端信号断丝时的信号降级显示问题，
	  通过进路的接车进路和发车进路在显示上要有联系；
	4.修改接近区段的判断BUG；
	5.修改与信号保持中相关部分的BUG（断丝）；
2013/01/24,v1.2.1,LYC
	修改了侵限区段占用无法人工解锁进路的BUG
2013/08/02,V1.2.1,LYC
	1.增加了delay_route_check_conditiion函数、guide_route_check_conditiion函数、
	check_relay_cancel_route_condition函数中检查中间道岔和特殊防护道岔位置
	2.在delay_route_check_conditiion函数中增加了在延时过程中对进路的延续部分进行信号检查
2013/01/24 V1.2.1 hjh
	delay_route_check_conditiion延时过程中持续检查进路上区段的状态
2013/8/2 V1.2.1 LYC 
	1.delay_route_check_conditiion增加了带有延续进路进路的，延续进路部分也满足信号检查条件
	2.delay_route_check_conditiion增加了特殊防护道岔和中间道岔条件检查
2014/02/26 V1.2.1 hjh mantis:3409
	delay_route_check_conditiion中不检查中间道岔、特殊防护道岔以及延续部分的道岔。
2014/04/21 V1.2.2 hjh
	根据需求完成引导进路的解锁功能。
2014/5/8 V1.2.3 hjh
	修改引导进路延时解锁。
***************************************************************************************/
#include "relay_cancel_route.h"
#include "check_lock_route.h"
#include "keep_signal.h"

/****************************************************
函数名:    relay_cancel_route
功能描述:  人工延时解锁
返回值:    无
参数:      route_index
作者  :    LYC
日期  ：   2012/3/28
****************************************************/
void relay_cancel_route(route_t route_index)
{
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		EN_route_state rs = gr_state(route_index);
		/*判断该进路是否存在*/
		if (IsTRUE(is_route_exist(route_index)))
		{
			/*根据不同的进路状态执行操作*/
			switch(rs)
			{				
				case RS_RCR_A_SIGNAL_CLOSING:    /*进路状态为"人工解锁模块正在非正常关闭信号"*/
					RCR_singal_closing(route_index);
					break;
				case RS_RCR_SUCCESSIVE_RELAY:   /*进路状态为"人工解锁模块正在延时解锁"*/
					delay_route_check_conditiion(route_index);
					break;	
				case RS_RCR_G_SIGNAL_CLOSING:  /*进路状态为"人工延时解锁模块正在关闭引导信号"*/
					RCR_G_singal_closing(route_index);
					break;
				
				default:/*没有符合条件的直接退出本模块*/
					break;
			}
		}
	}
}

/****************************************************
函数名:    command_relay_cancel_route
功能描述:  命令处理模块调用的人工解锁
返回值:  
参数:      route_index

作者  :    LYC
日期  ：   2012/4/16
****************************************************/
void command_relay_cancel_route(route_t route_index)
{
	EN_route_state rs = gr_state(route_index);
	int16_t temp;
	char_t tips[TEST_NAME_LENGTH];

	/*判断该进路是否存在*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*根据不同的进路状态执行操作*/
		switch(rs)
		{
				/*进路状态为"信号开放模块正在开放信号"*/
			case RS_SO_SIGNAL_OPENING:
				close_signal(route_index);
				RCR_singal_closed(route_index);
				break;

			case RS_SK_SIGNAL_CHANGING:
				close_signal(route_index);
				RCR_singal_closed(route_index);
				break;

				/*进路状态为"重复开放信号模块正在开放信号"*/
			case RS_SRO_SIGNAL_OPENING:
				close_signal(route_index);
				RCR_singal_closed(route_index);
				break;

				/*进路状态为"正在开放引导信号"*/
			case RS_G_SO_SIGNAL_OPENING:
				close_signal(route_index);
				RCR_singal_closed(route_index);
				break;

				/*进路状态为"引导信号已开放"*/
			case RS_G_SIGNAL_OPENED:
				RCR_signal_opened_disposal(route_index);
				break;

				/*进路状态为"信号已开放"*/
			case RS_SIGNAL_OPENED:
				RCR_signal_opened_disposal(route_index);
				break;

				/*进路状态为"信号非正常关闭"*/
			case RS_A_SIGNAL_CLOSED:
				RCR_singal_closed(route_index);
				break;
				/*引导信号正常关闭*/
			case RS_G_SIGNAL_CLOSED:
				RCR_singal_closed(route_index);
				break;
				/*引导进路已锁闭*/
			case RS_G_ROUTE_LOCKED:
				after_singal_closed_disposal(route_index);
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
						sr_state(route_index,RS_RCR_A_SIGNAL_CLOSING);
						sr_clear_error_count(route_index);
					}
				}
				else
				{
					/*检查其他条件*/
					after_singal_closed_disposal(route_index);
				}
				break;
			case RS_RCR_SUCCESSIVE_RELAY:   /*进路状态为"人工解锁模块正在延时解锁"*/
				if (IsTRUE(is_guide_route(route_index)))
				{
					/*引导进路条件检查及处理*/
					if (IsTRUE(guide_route_check_conditiion(route_index)))
					{
						guide_route_unlock(route_index);
						if (IsTRUE(is_route_exist(route_index)) && (RS_RCR_SUCCESSIVE_RELAY != gr_state(route_index)))
						{
							process_warning(ERR_TRAIN_IN_ROUTE,gr_name(route_index));
							/*输出提示信息*/
							memset(tips,0x00,sizeof(tips));
							strcat_check(tips,"车在进路中：",sizeof(tips));
							OutputHmiNormalTips(tips,route_index);
						}
					}
				}
				else
				{
					/*输出提示信息*/
					memset(tips,0x00,sizeof(tips));
					strcat_check(tips,"延时未到：",sizeof(tips));
					OutputHmiNormalTips(tips,route_index);
				}
				break;
			case RS_FAILURE:
				RCR_singal_closed(route_index);
				break;
				/*没有符合条件的直接退出本模块*/
			default:
				/*操作错误*/
				process_warning(ERR_OPERATION,gr_name(route_index));
				CIHmi_SendNormalTips("错误办理：%s",gn_name(gr_start_signal(route_index)));
				break;
		}
	}
}
/****************************************************
函数名:    RCR_signal_opened_disposal
功能描述:  进路状态为信号开放的处理过程
返回值:  
参数:      route_index

作者  :    LYC
日期  ：   2012/3/28
****************************************************/
void RCR_signal_opened_disposal(route_t route_index)
{
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*根据进路类型关闭相应进路的信号机,并设置进路状态*/
		/*列车进路时处理方式*/
		if ((gr_type(route_index) == RT_TRAIN_ROUTE) 
			&& (IsFALSE(is_guide_route(route_index))))
		{				
			close_signal(route_index);
			sr_state(route_index,RS_RCR_A_SIGNAL_CLOSING);
			sr_clear_error_count(route_index);
		}
		/*调车进路时处理方式*/
		if (gr_type(route_index) == RT_SHUNTING_ROUTE)
		{				
			close_signal(route_index);
			sr_state(route_index,RS_RCR_A_SIGNAL_CLOSING);
			sr_clear_error_count(route_index);
		}
		/*引导进路时处理方式*/
		if (IsTRUE(is_guide_route(route_index)))
		{				
			close_signal(route_index);
			sr_state(route_index,RS_RCR_G_SIGNAL_CLOSING);
			sr_clear_error_count(route_index);
		}
	}
}

/********************************************************************
函数名:    RCR_singal_closing
功能描述:  进路状态为“人工解锁模块正在非正常关闭信号”时的处理过程
返回值:		无
参数:      route_index
作者  :    LYC
日期  ：   2012/3/28
********************************************************************/
void RCR_singal_closing(route_t route_index)
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
				after_singal_closed_disposal(route_index);	
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
			/*进路始端信号机是否关闭*/
			if(IsTRUE(is_signal_close(gr_start_signal(route_index))))
			{		
				sr_state(route_index,RS_A_SIGNAL_CLOSED);
				sr_clear_error_count(route_index);
				after_singal_closed_disposal(route_index);		
			}
			/*断丝*/
			else if (gn_state(gr_start_signal(route_index)) == SGS_FILAMENT_BREAK)
			{
				process_warning(ERR_FILAMENT_BREAK,gr_name(route_index));
				CIHmi_SendNormalTips("断丝");
				sr_clear_error_count(route_index);
				after_singal_closed_disposal(route_index);
			}
			else
			{
				/*三个周期无法关闭信号则设置进路状态为错误、清除错误计数、提示错误*/
				sr_increament_error_count(route_index);
				/*当前周期错误计数是否达到最大周期数*/
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
函数名:    RCR_G_singal_closing
功能描述:  进路状态为“人工解锁模块正在关闭引导信号”时的处理过程
返回值:    
参数:      int16_t route_index
作者  :    hejh
日期  ：   2012/6/21
****************************************************/
void RCR_G_singal_closing(route_t route_index)
{
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*判断进路始端信号是否关闭*/
		if(IsTRUE(is_signal_close(gr_start_signal(route_index))))
		{	
			sr_state(route_index,RS_G_SIGNAL_CLOSED);
			sn_signal_expect_state(gr_start_signal(route_index),SGS_ERROR);
			sr_clear_error_count(route_index);
			after_singal_closed_disposal(route_index);
		}
		else
		{
			/*三个周期无法关闭信号则设置进路状态为错误、清除错误计数、提示错误*/
			sr_increament_error_count(route_index);
			/*当前周期错误计数是否达到最大周期数*/
			if (gr_error_count(route_index) > MAX_ERROR_PER_COMMAND)
			{
				process_warning(ERR_CLOSE_SIGNAL,gr_name(route_index));
				sr_state(route_index,RS_FAILURE);
				sr_clear_error_count(route_index);
			}
		}
	}
	
}

/****************************************************
函数名:    RCR_singal_closed
功能描述:  进路状态为“信号非正常关闭”时的处理过程
返回值:		无
参数:      route_index
作者  :    LYC
日期  ：   2012/3/28
****************************************************/
void RCR_singal_closed(route_t route_index)
{
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*判断进路始端信号关闭或断丝*/
		if(IsTRUE(is_signal_close(gr_start_signal(route_index))) 
			|| (gn_signal_state(gr_start_signal(route_index)) == SGS_FILAMENT_BREAK))
		{			
			after_singal_closed_disposal(route_index);
		}
	}	
}

/****************************************************
函数名:    after_singal_closed_disposal
功能描述:  信号关闭后处理过程
返回值:  
参数:      route_index
作者  :    LYC
日期  ：   2012/3/28
****************************************************/
void after_singal_closed_disposal(route_t route_index)
{
	CI_BOOL result = CI_TRUE;
	route_t bwr = gr_backward(route_index);
	node_t end_signal,last_section,last_switch_section;

	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*判断该进路是否引导进路*/
		if (IsTRUE(is_guide_route(route_index)))
		{
			/*引导进路条件检查及处理*/
			if (IsTRUE(guide_route_check_conditiion(route_index)))
			{
				guide_route_unlock(route_index);
				if (IsTRUE(is_route_exist(route_index)) && (RS_RCR_SUCCESSIVE_RELAY != gr_state(route_index))
					&& (RS_MS_UNLOCK_RELAY != gr_state(route_index)))
				{
					process_warning(ERR_TRAIN_IN_ROUTE,gr_name(route_index));
					CIHmi_SendNormalTips("车在进路中");
				}
			}			
		} 
		else
		{
			/*判断该进路不是延续进路的延续部分*/
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
							process_warning(ERR_SUCCESSIVE_CANCLE,gr_name(route_index));
							CIHmi_SendNormalTips("接车进路未解锁");
						}
					}
				}
				else
				{
					if (IsTRUE(is_node_locked(last_switch_section,LT_LOCKED)))
					{
						result = CI_FALSE;
						process_warning(ERR_SUCCESSIVE_CANCLE,gr_name(route_index));
						CIHmi_SendNormalTips("接车进路未解锁");
					}
				}
			}

			/*hjh 20150611 人工解锁时需必须检查列车位置状态机*/
			if ((gr_state_machine(route_index) == RSM_INIT)
				|| (gr_state_machine(route_index) == RSM_TRAIN_APPROACH)
				|| (gr_state_machine(route_index) == RSM_TRAIN_ABNORMAL_APPROACH)
				|| (gr_state_machine(route_index) == RSM_TRAIN_IN_POSSIBLE))
			{
				/*检查进路是否满足人工延时解锁检查条件*/
				if (IsFALSE(check_relay_cancel_route_condition(route_index)))
				{
					result = CI_FALSE;
				}
				/*检查信号点的进路锁闭标志*/
				if (IsFALSE(check_node_route_locked(route_index)))
				{
					result = CI_FALSE;
					process_warning(ERR_NODE_UNLOCK,gr_name(route_index));
				}
			}
			else
			{
				result = CI_FALSE;
				CIHmi_SendNormalTips("车在进路中");
			}			
			
			/*正常进路条件检查及处理*/
			if (IsTRUE(result))
			{
				RCR_route_disposal(route_index);
			}
		}
	}	
}

/****************************************************
函数名:    delay_route_check_conditiion
功能描述:  进路延时过程
返回值:    
参数:      int16_t route_index
作者  :    hejh
日期  ：   2012/8/1
****************************************************/
void delay_route_check_conditiion(route_t route_index)
{
	CI_BOOL result = CI_TRUE;
	route_t fwr = gr_forward(route_index);
	int16_t i;
	char_t tips[TEST_NAME_LENGTH];

	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*进路相关道岔锁在规定位置*/	
		if (IsFALSE(check_switch_location(route_index)))
		{
			result = CI_FALSE;
			process_warning(ERR_SWITCH_LOCATION, gr_name(route_index));
		}	
		/*hjh 2014-4-28 人工解锁时不检查中间道岔位置
		2013/8/2 LYC 中间道岔位置检查*/	
		/*if (IsFALSE(check_middle_switch_location(route_index)))
		{
			process_tips(ERR_SWITCH_LOCATION, gr_name(route_index));
			//result = CI_FALSE;
		}*/
		/*2013/8/2 LYC 特殊防护道岔位置检查*/	
		if (IsTRUE(result) && IsFALSE(check_special_switch_location(route_index)))
		{
			process_warning(ERR_SWITCH_LOCATION, gr_name(route_index));
			//result = CI_FALSE;
		}
		/*检查车列是否冒进信号*/
		if ((IsTRUE(result) && IsFALSE(is_guide_route(route_index))) 
			&& (gn_section_state(gr_first_section(route_index)) != SCS_CLEARED))
		{
			result = CI_FALSE;
			//process_warning(ERR_CRASH_INTO_SIGNAL, gr_name(route_index));
			sr_state(route_index,RS_CRASH_INTO_SIGNAL);
			/*输出提示信息*/
			memset(tips,0x00,sizeof(tips));
			strcat_check(tips,"冒进信号：",sizeof(tips));
			OutputHmiNormalTips(tips,route_index);
		}		
		/*hjh 2013-8-5 延时过程中持续检查进路上区段的状态*/
		if (IsTRUE(result))
		{
			for (i = 0; i < gr_nodes_count(route_index); i++)
			{
				if (IsTRUE(is_section(gr_node(route_index,i))))
				{
					/*引导进路*/
					if (IsTRUE(is_guide_route(route_index)))
					{
						/*故障区段判断其是否出清*/
						if (IsTRUE(is_section_fault(gr_node(route_index,i))))
						{
							if (gn_section_state(gr_node(route_index,i)) == SCS_CLEARED)
							{
								result = CI_FALSE;
								process_warning(ERR_TRAIN_IN_ROUTE, gr_name(route_index));
								/*输出提示信息*/
								memset(tips,0x00,sizeof(tips));
								strcat_check(tips,"车在进路中：",sizeof(tips));
								OutputHmiNormalTips(tips,route_index);
								break;
							}
						}
						/*非故障区段判断其是否占用*/
						else
						{
							if (gn_section_state(gr_node(route_index,i)) != SCS_CLEARED)
							{
								result = CI_FALSE;
								process_warning(ERR_TRAIN_IN_ROUTE, gr_name(route_index));
								/*输出提示信息*/
								memset(tips,0x00,sizeof(tips));
								strcat_check(tips,"车在进路中：",sizeof(tips));
								OutputHmiNormalTips(tips,route_index);
								break;
							}
						}					
					}
					else
					{
						/*调车进路*/
						if (gr_type(route_index) == RT_SHUNTING_ROUTE)
						{
							/*股道、无岔区段、尽头线不检查*/
							if ((gn_type(gr_node(route_index,i)) == NT_TRACK)
								|| (gn_type(gr_node(route_index,i)) == NT_NON_SWITCH_SECTION)
								|| (gn_type(gr_node(route_index,i)) == NT_STUB_END_SECTION)
								|| (gn_type(gr_node(route_index,i)) == NT_LOCODEPOT_SECTION))
							{
								continue;
							}
							else
							{
								if (gn_section_state(gr_node(route_index,i)) != SCS_CLEARED)
								{
									result = CI_FALSE;
									process_warning(ERR_SECTION_OCCUPIED, gr_name(route_index));
									/*输出提示信息*/
									CIHmi_SendNormalTips("区段占用：%s",gn_name(gr_node(route_index,i)));
									break;
								}
							}
						}
						/*列车进路*/
						else
						{
							if (gn_section_state(gr_node(route_index,i)) != SCS_CLEARED)
							{
								/*股道不检查*/
								if (gn_type(gr_node(route_index,i)) == NT_TRACK)
								{
									process_warning(ERR_SECTION_OCCUPIED, gr_name(route_index));
								}
								else
								{
									result = CI_FALSE;
									process_warning(ERR_SECTION_OCCUPIED, gr_name(route_index));
									/*输出提示信息*/
									CIHmi_SendNormalTips("区段占用：%s",gn_name(gr_node(route_index,i)));
									break;
								}
								
							}
						}
					}
				}
			}
		}
		
		/*检查进路相关的侵限条件是否满足*/
		if (IsTRUE(result) && IsTRUE(check_exceed_limit(route_index)))
		{
			process_warning(ERR_EXCEED_LIMIT_OCCUPIED, gr_name(route_index));
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
		/* 2013/8/2 LYC 如果是带有延续进路进路的，延续进路部分也满足信号检查条件*/
		if (IsTRUE(result) && IsTRUE(have_successive_route(route_index)))
		{
			if (fwr != NO_INDEX)
			{
				/*进路相关道岔锁在规定位置*/	
				if (IsFALSE(check_switch_location(fwr)))
				{
					//result = CI_FALSE;
					process_warning(ERR_SWITCH_LOCATION, gr_name(fwr));
				}	
				/*2013/8/2 LYC 中间道岔位置检查*/	
				if (IsTRUE(result) && IsFALSE(check_middle_switch_location(fwr)))
				{
					process_warning(ERR_SWITCH_LOCATION, gr_name(fwr));
					result = CI_FALSE;
				}
				/*2013/8/2 LYC 特殊防护道岔位置检查*/	
				if (IsTRUE(result) && IsFALSE(check_special_switch_location(fwr)))
				{
					process_warning(ERR_SWITCH_LOCATION, gr_name(fwr));
					result = CI_FALSE;
				}
				/*检查车列是否冒进信号*/
				if (IsTRUE(result) && gn_section_state(gr_section(fwr,0)) != SCS_CLEARED)
				{
					result = CI_FALSE;
					//process_warning(ERR_CRASH_INTO_SIGNAL, gr_name(fwr));
					sr_state(fwr,RS_CRASH_INTO_SIGNAL);
					CIHmi_SendNormalTips("冒进信号");
				}
				/*检查进路相关的侵限条件是否满足*/
				if (IsTRUE(result) && IsTRUE(check_exceed_limit(fwr)))
				{
					process_warning(ERR_EXCEED_LIMIT_OCCUPIED, gr_name(fwr));
					result = CI_FALSE;
				}
				/*检查敌对进路未建立*/
				if (IsTRUE(result) && IsTRUE(check_conflict_signal(fwr)))
				{
					process_warning(ERR_CONFLICT_ROUTE, gr_name(fwr));
					result = CI_FALSE;
				}
				/*检查迎面敌对信号未开放*/
				if (IsTRUE(result) && IsTRUE(check_face_conflict(fwr)))
				{
					process_warning(ERR_FACE_CONFLICT, gr_name(fwr));
					result = CI_FALSE;
				}
			}			
		}
		/*正常进路条件检查及处理*/
		if (IsTRUE(result))
		{
			RCR_route_disposal(route_index);
		}
		else
		{
			/*如果在延时解锁过程中进路条件检查失败则停止延时解锁,并设置进路状态*/
			if (IsTRUE(is_timer_run(route_index)))
			{
				sr_stop_timer(route_index);
			}
			if ((RS_FAILURE != gr_state(route_index)) && (RS_CRASH_INTO_SIGNAL != gr_state(route_index)))
			{
				sr_state(route_index,RS_FAILURE);	
			}					
		}
	}	
}

/****************************************************
函数名:    guide_route_check_conditiion
功能描述:  引导进路条件检查
返回值:    CI_TRUE:引导进路信号检查成功
		   CI_FALSE：引导进路信号检查失败
参数:      route_index
作者  :    LYC
日期  ：   2012/3/29
****************************************************/
CI_BOOL guide_route_check_conditiion(route_t route_index)
{
	CI_BOOL result = CI_TRUE;

	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*进路相关道岔锁在规定位置*/	
		if (IsFALSE(check_switch_location(route_index)))
		{
			result = CI_FALSE;
			process_warning(ERR_SWITCH_LOCATION, gr_name(route_index));
		}	
		/*2013/8/2 LYC 中间道岔位置检查*/	
		if (IsFALSE(check_middle_switch_location(route_index)))
		{
			process_warning(ERR_SWITCH_LOCATION, gr_name(route_index));
			result = CI_FALSE;
		}
		/*2013/8/2 LYC 特殊防护道岔位置检查*/	
		if (IsFALSE(check_special_switch_location(route_index)))
		{
			process_warning(ERR_SWITCH_LOCATION, gr_name(route_index));
			result = CI_FALSE;
		}
		/*检查进路相关的侵限条件是否满足*/
		/*if (IsTRUE(check_exceed_limit(route_index)))
		{
			process_warning(ERR_EXCEED_LIMIT_OCCUPIED, gr_name(route_index));
			result = CI_FALSE;
		}*/

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
	}
	return result;
}

/****************************************************
函数名:    RCR_route_disposal
功能描述:  人工延时解锁过程处理
返回值:  
参数:      route_index

作者  :    LYC
日期  ：   2012/3/29
****************************************************/
void RCR_route_disposal(route_t route_index)
{
	CI_TIMER time_interval = 0;
	CI_BOOL result = CI_FALSE;
	route_t bwr = NO_INDEX;

	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*检查是否正在延时*/
		if (IsTRUE(is_timer_run(route_index)))
		{			
			/*检查延时时间到？*/
			if (IsTRUE(is_complete_timer(route_index)))
			{
				result = CI_TRUE;
			}	
			else
			{
				/*延时过程中车列是否冒进信号*/
				if (RSM_TRAIN_APPROACH != gr_state_machine(route_index))
				{
					if (IsTRUE(is_guide_route(route_index)))
					{
						/*车列进入进路*/
						if ((RSM_SECOND_SECTION == gr_state_machine(route_index))
							|| (RSM_TRAIN_IN_ROUTE == gr_state_machine(route_index)))
						{
							process_warning(ERR_TRAIN_IN_ROUTE,gr_name(route_index));
							sr_state(route_index,RS_FAILURE);
							sr_stop_timer(route_index);
							result = CI_FALSE;
							CIHmi_SendNormalTips("冒进信号");
						}
					}
					else
					{
						/*车列进入进路*/
						if ((RSM_FIRST_SECTION == gr_state_machine(route_index))
							|| (RSM_SECOND_SECTION == gr_state_machine(route_index))
							|| (RSM_TRAIN_IN_ROUTE == gr_state_machine(route_index)))
						{
							process_warning(ERR_TRAIN_IN_ROUTE,gr_name(route_index));
							sr_state(route_index,RS_CRASH_INTO_SIGNAL);
							sr_stop_timer(route_index);
							result = CI_FALSE;
							CIHmi_SendNormalTips("冒进信号");
						}
					}
				}
			}
		}
		else
		{
			time_interval = gr_delay_times(route_index);
			/*检查接近区段空闲*/
			//if (IsTRUE(is_approach_cleared(route_index,CI_TRUE)))
			/*调车进路无接近记忆功能*/
			if (RT_SHUNTING_ROUTE == gr_type(route_index))
			{
				if ((RS_ROUTE_LOCKED != gr_state(route_index)) && IsFALSE(is_approach_cleared(route_index,CI_TRUE)))
				{
					sr_start_timer(route_index,time_interval,DTT_UNLOCK);
					sr_state(route_index,RS_RCR_SUCCESSIVE_RELAY);
					result = CI_FALSE;
				}
				else
				{
					result = CI_TRUE;
				}
			}
			else
			{
				if ((RSM_TRAIN_APPROACH == gr_state_machine(route_index)) || (ROF_APPROACH_ADDED == gr_other_flag(route_index)))
				{
					sr_start_timer(route_index,time_interval,DTT_UNLOCK);
					sr_state(route_index,RS_RCR_SUCCESSIVE_RELAY);
					result = CI_FALSE;
				}
				else
				{
					bwr = gr_backward(route_index);
					/*判断该进路是延续进路的延续部分*/
					if ((bwr != NO_INDEX) && IsTRUE(have_successive_route(bwr)))
					{
						sr_start_timer(route_index,time_interval,DTT_UNLOCK);
						sr_state(route_index,RS_RCR_SUCCESSIVE_RELAY);
						result = CI_FALSE;
					}
					else
					{
						if ((bwr == NO_INDEX) && IsTRUE(is_successive_route(route_index)))
						{
							/*车列未压入接车进路*/
							if (ROF_SUCCESSIVE == gr_other_flag(route_index))
							{
								result = CI_TRUE;
							}
							/*车列已压入接车进路*/
							if (ROF_SUCCESSIVE_DELAY == gr_other_flag(route_index))
							{
								sr_start_timer(route_index,time_interval,DTT_UNLOCK);
								sr_state(route_index,RS_RCR_SUCCESSIVE_RELAY);
								result = CI_FALSE;
							}
						}
						else
						{
							result = CI_TRUE;
						}
					}
				}				
			}
		}
		if (IsTRUE(result))
		{
			/*解锁进路*/
			sr_state(route_index,RS_UNLOCKED);
			sr_clear_error_count(route_index);
			delete_route(route_index);
		}
	}	
}

/****************************************************
函数名:    check_relay_cancel_route_condition
功能描述:  检查进路是否满足人工延时解锁检查条件
返回值:    
参数:      route_index
作者  :    CY
日期  ：   2011/12/27
****************************************************/
CI_BOOL check_relay_cancel_route_condition( route_t route_index) 
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
		/*hjh 2014-4-29 人工解锁时不检查联系条件*/
		///*检查进路上的联系条件*/
		//if (IsFALSE(check_relation_condition(route_index)))
		//{
		//	result = CI_FALSE;
		//}

		/*道岔位置检查*/	
		if (IsFALSE(check_switch_location(route_index)))
		{
			process_warning(ERR_SWITCH_LOCATION, gr_name(route_index));
			result = CI_FALSE;
		}
		/*hjh 2014-4-28 人工解锁时不检查中间道岔位置
		2013/8/2 LYC 中间道岔位置检查*/	
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

		/*信号非正常关闭时，车列运行*/
		if ((RS_A_SIGNAL_CLOSED == gr_state(route_index))
			&& ((RSM_FIRST_SECTION == gr_state_machine(route_index)) 
			|| (RSM_SECOND_SECTION == gr_state_machine(route_index))
			|| (RSM_TRAIN_IN_ROUTE == gr_state_machine(route_index))))
			//&& (SCSM_SELF_UNLOCK == gn_state_machine(gr_first_section(route_index))))
		{
			process_warning(ERR_TRAIN_IN_ROUTE, gr_name(route_index));
			result = CI_FALSE;
			CIHmi_SendNormalTips("车在进路中");
		}
	}	

	FUNCTION_OUT;
	return result;
}

/****************************************************
函数名：   guide_route_unlock
功能描述： 引导进路解锁
返回值：   void
参数：     route_t route_index
作者：	   hejh
日期：     2014/04/18
****************************************************/
void guide_route_unlock(route_t route_index)
{
	node_t node,fault_node = NO_INDEX,last_section,last_switch_section,second_last_switch_section;
	node_t middle_last_section,middle_last_switch_section,middle_second_last_switch_section;
	int8_t i;
	CI_BOOL result = CI_TRUE;

	/*车列未进入进路*/
	if ((RSM_INIT == gr_state_machine(route_index)) 
		|| (RSM_TRAIN_APPROACH == gr_state_machine(route_index)))
	{
		/*接车进路转引导且车接近*/
		if (ROF_GUIDE_APPROACH == gr_other_flag(route_index))
		{
			/*延时解锁引导进路*/
			if (IsFALSE(is_timer_run(route_index)))
			{			
				sr_start_timer(route_index,MINUTES_3,DTT_UNLOCK);
				sr_state(route_index,RS_RCR_SUCCESSIVE_RELAY);			
			}
		}
		else
		{
			/*立即解锁引导进路*/
			sr_state(route_index,RS_UNLOCKED);
			delete_route(route_index);	
		}			
	}
	/*车列进入进路*/
	else if ((RSM_FIRST_SECTION == gr_state_machine(route_index))
		|| (RSM_G_FAULT_FIRST == gr_state_machine(route_index)))
	{
		/*延时解锁引导进路*/
		if (IsFALSE(is_timer_run(route_index)))
		{
			sr_start_timer(route_index,MINUTES_3,DTT_UNLOCK);
			sr_state(route_index,RS_RCR_SUCCESSIVE_RELAY);		
			sr_fault_section(route_index);		
		}
		else
		{
			/*延时类型不是延时解锁时开始延时解锁*/
			if (DTT_UNLOCK != gr_time_type(route_index))
			{
				sr_start_timer(route_index,MINUTES_3,DTT_UNLOCK);
				sr_state(route_index,RS_RCR_SUCCESSIVE_RELAY);		
				sr_fault_section(route_index);
			}
		}
	}
	/*车列完全进入进路和车列压入第二区段*/
	else
	{
		last_section = gr_last_section(route_index);		
		last_switch_section = gr_backward_section(route_index,gr_node_index_route(route_index,last_section));
		second_last_switch_section = gr_backward_section(route_index,gr_node_index_route(route_index,last_switch_section));
		middle_last_section = gr_forward_section(route_index,gr_node_index_route(route_index,gb_node(gr_end_button(route_index))));
		middle_last_switch_section = gr_backward_section(route_index,gr_node_index_route(route_index,middle_last_section));
		middle_second_last_switch_section = gr_backward_section(route_index,gr_node_index_route(route_index,middle_last_switch_section));

		/*车列压入第二区段*/
		if (RSM_SECOND_SECTION == gr_state_machine(route_index))
		{
			if (SCSM_SELF_UNLOCK == gn_state_machine(last_switch_section))
			{
				/*立即解锁引导进路*/
				sr_state(route_index,RS_UNLOCKED);
				delete_route(route_index);
			}
			if (SCSM_SELF_UNLOCK == gn_state_machine(middle_last_switch_section))
			{
				/*立即解锁引导进路*/
				sr_state(route_index,RS_MS_UNLOCK_RELAY);
				delete_middle_guide_route(route_index);
			}
		}
		/*车列完全进入进路*/
		else
		{
			/*立即解锁：1.股道故障，次最后区段和最后区段符合三点检查
						2.无故障区段，最后区段符合三点检查*/
			/*延时解锁：1.无故障区段，最后区段不符合三点检查，进路上区段出清（不包含股道）
						2.股道故障，次最后区段和最后区段不符合三点检查，进路上区段出清
						3.最后区段故障，则检查除股道占用或空闲且占用过外所有非故障区段均空闲
						4.次最后区段故障，最后区段和股道顺序占用后最后区段出清
						5.其他区段故障，非故障区段出清，最后区段和股道顺序占用后最后区段出清*/

			/*轮询进路上是否有区段故障*/
			for (i = 0; i < gr_nodes_count(route_index); i++)
			{
				fault_node = gr_node(route_index,i);
				if (IsTRUE(is_section(fault_node)) && IsTRUE(is_section_fault(fault_node)))
				{
					result = CI_FALSE;
					break;
				}
			}

			if (IsTRUE(result))
			{
				/*无故障区段，最后区段符合三点检查*/
				if ((SCSM_SELF_UNLOCK == gn_state_machine(last_switch_section)) && (SCS_CLEARED == gn_section_state(last_switch_section)))
				{
					/*立即解锁引导进路*/
					sr_state(route_index,RS_UNLOCKED);
					delete_route(route_index);
				}
				else if ((SCSM_SELF_UNLOCK == gn_state_machine(middle_last_switch_section)) && (SCS_CLEARED == gn_section_state(middle_last_switch_section)))
				{
					/*立即解锁引导进路*/
					sr_state(route_index,RS_MS_UNLOCK_RELAY);
					delete_middle_guide_route(route_index);
				}
				/*无故障区段，最后区段不符合三点检查，进路上区段出清（不包含股道）*/
				else
				{
					for (i = 0; i < gr_nodes_count(route_index); i++)
					{
						node = gr_node(route_index,i);
						if (IsTRUE(is_section(node)) && (node != last_section)
							&& (SCS_CLEARED != gn_section_state(node)) && IsFALSE(is_section_fault(node)))
						{
							result = CI_FALSE;
							break;
						}
					}
					if (IsTRUE(result))
					{
						/*延时解锁引导进路*/
						if (IsFALSE(is_timer_run(route_index)))
						{			
							sr_start_timer(route_index,MINUTES_3,DTT_UNLOCK);
							sr_state(route_index,RS_RCR_SUCCESSIVE_RELAY);	
							sr_fault_section(route_index);
						}
					}
				}
			}			
			else
			{
				/*故障情况下，再次解锁检查进路空闲则立即解锁*/
				if (RS_FAILURE == gr_state(route_index))
				{
					result = CI_TRUE;
					for (i = 0; i < gr_nodes_count(route_index); i++)
					{
						node = gr_node(route_index,i);
						if (IsTRUE(is_section(node)) && (node != last_section) && (SCS_CLEARED != gn_section_state(node)))
						{
							result = CI_FALSE;
							break;
						}
					}
					if (IsTRUE(result))
					{
						/*立即解锁引导进路*/
						sr_state(route_index,RS_UNLOCKED);
						delete_route(route_index);
					}
				}

				/*股道故障*/
				if (fault_node == last_section)
				{
					/*股道故障，次最后区段和最后区段符合三点检查*/
					if ((SCSM_SELF_UNLOCK == gn_state_machine(last_switch_section)) 
						&& (SCSM_SELF_UNLOCK == gn_state_machine(second_last_switch_section)))
					{
						/*立即解锁引导进路*/
						sr_state(route_index,RS_UNLOCKED);
						delete_route(route_index);
					}
					else if ((SCSM_SELF_UNLOCK == gn_state_machine(middle_last_switch_section)) 
						&& (SCSM_SELF_UNLOCK == gn_state_machine(middle_second_last_switch_section)))
					{
						/*立即解锁引导进路*/
						sr_state(route_index,RS_MS_UNLOCK_RELAY);
						delete_middle_guide_route(route_index);
					}
					/*股道故障，次最后区段和最后区段不符合三点检查，进路上区段出清*/
					else
					{
						for (i = 0; i < gr_nodes_count(route_index); i++)
						{
							node = gr_node(route_index,i);
							if (IsTRUE(is_section(node)) && (SCS_CLEARED != gn_section_state(node)) && IsFALSE(is_section_fault(node)))
							{
								result = CI_FALSE;
								break;
							}
						}
						if (IsTRUE(result))
						{
							/*延时解锁引导进路*/
							if (IsFALSE(is_timer_run(route_index)))
							{			
								sr_start_timer(route_index,MINUTES_3,DTT_UNLOCK);
								sr_state(route_index,RS_RCR_SUCCESSIVE_RELAY);		
								sr_fault_section(route_index);
							}
						}
					}
				}
				/*最后区段故障*/
				else if (fault_node == last_switch_section)
				{
					/*最后区段故障，则检查除股道占用或空闲且占用过外所有非故障区段均空闲*/
					if ((SCS_CLEARED != gn_section_state(last_section))
						|| ((SCSM_TRACK_UNLOCK == gn_state_machine(last_section)) && (SCS_CLEARED == gn_section_state(last_section))))
					{
						for (i = 0; i < gr_nodes_count(route_index); i++)
						{
							node = gr_node(route_index,i);
							if (IsTRUE(is_section(node)) && (node != last_section)
								&& (SCS_CLEARED != gn_section_state(node)) && IsFALSE(is_section_fault(node)))
							{
								result = CI_FALSE;
								break;
							}
						}
						if (IsTRUE(result))
						{
							/*延时解锁引导进路*/
							if (IsFALSE(is_timer_run(route_index)))
							{			
								sr_start_timer(route_index,MINUTES_3,DTT_UNLOCK);
								sr_state(route_index,RS_RCR_SUCCESSIVE_RELAY);		
								sr_fault_section(route_index);
							}
						}
					}
				}
				/*次最后区段故障*/
				else if (fault_node == second_last_switch_section)
				{
					/*次最后区段故障，最后区段和股道顺序占用后最后区段出清*/
					if (((SCSM_BEHIND_CLEARED_2 == gn_state_machine(last_section)) || (SCSM_SELF_UNLOCK == gn_state_machine(last_section)))
						&& (SCSM_FAULT_CLEARED == gn_state_machine(last_switch_section))
						&& (SCS_CLEARED == gn_section_state(last_switch_section)))
					{
						/*延时解锁引导进路*/
						if (IsFALSE(is_timer_run(route_index)))
						{			
							sr_start_timer(route_index,MINUTES_3,DTT_UNLOCK);
							sr_state(route_index,RS_RCR_SUCCESSIVE_RELAY);	
							sr_fault_section(route_index);
						}
					}
					/*hjh 2014-9-23 次最后区段故障，最后区段和股道顺序占用后最后区段出清*/
					if (((SCSM_BEHIND_CLEARED_2 == gn_state_machine(last_section)) || (SCSM_SELF_UNLOCK == gn_state_machine(last_section)))
						&& (SCSM_SELF_UNLOCK == gn_state_machine(last_switch_section))
						&& (SCS_CLEARED == gn_section_state(last_switch_section)))
					{
						/*立即解锁引导进路*/
						sr_state(route_index,RS_UNLOCKED);
						delete_route(route_index);
					}
				}
				/*其他区段故障*/
				else
				{
					/*其他区段故障，非故障区段出清，最后区段和股道顺序占用后最后区段出清*/
					if (((((SCSM_BEHIND_CLEARED_2 == gn_state_machine(last_section)) || (SCSM_SELF_UNLOCK == gn_state_machine(last_section))) && (SCSM_FAULT == gn_state_machine(last_switch_section)))
						|| (((SCSM_BEHIND_CLEARED_2 == gn_state_machine(last_section)) || (SCSM_SELF_UNLOCK == gn_state_machine(last_section))) && (SCSM_SELF_UNLOCK == gn_state_machine(last_switch_section))))
						&& (SCS_CLEARED == gn_section_state(last_switch_section)))
					{
						if (RS_FAILURE != gr_state(route_index))
						{
							result = CI_TRUE;
							for (i = 0; i < gr_nodes_count(route_index); i++)
							{
								node = gr_node(route_index,i);
								/*非故障区段出清*/
								if (IsTRUE(is_section(node)) && (node != last_section)
									&& (SCS_CLEARED != gn_section_state(node)) && IsFALSE(is_section_fault(node)))
								{
									//result = CI_FALSE;
									//break;
								}
								/*有区段未被占用过*/
								if (IsTRUE(is_section(node)) && (node != last_section) && IsFALSE(is_section_fault(node))
									&& ((SCSM_BEHIND_OCCUPIED == gn_state_machine(node)) || (SCSM_FAULT_BEHIND == gn_state_machine(node))))
								{
									result = CI_FALSE;
									break;
								}
							}
						}
						
						if (IsTRUE(result))
						{
							/*立即解锁引导进路*/
							sr_state(route_index,RS_UNLOCKED);
							delete_route(route_index);
						}
						else
						{
							/*延时解锁引导进路*/
							if (IsFALSE(is_timer_run(route_index)))
							{			
								sr_start_timer(route_index,MINUTES_3,DTT_UNLOCK);
								sr_state(route_index,RS_RCR_SUCCESSIVE_RELAY);	
								sr_fault_section(route_index);
							}
						}
					}
				}
			}
		}
	}
}


