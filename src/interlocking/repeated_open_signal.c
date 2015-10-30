/***************************************************************************************
Copyright (C), 2012,  Co.Hengjun, Ltd.
文件名:  repeated_open_signal.c
作者:    hjh
版本 :   1.0	
创建日期:2012/3/28
用途:    重复开放信号
历史修改记录:      
2012/7/5,V1.1,CY:
	1.修改了提示信息。根据实际情况将错误信息，警告信息，提示信息正确分类处理；修改了部分Bug
2012/8/10,V1.2,HJH:
	1.修改改方运行时离去区段占用时信号不关闭的问题，列车进路始端信号断丝时的信号降级显示问题，通过进路的接车进路和发车进路在显示上要有联系
2012/11/26 V1.2.1 hjh
	1.re_open_signal_process增加了重复开放信号时对进路上区段的历史信息的清除操作；
	2.repeated_open_train_signal增加了重复开放信号时对引导进路的历史信息的清除操作
2013/8/12 V1.2.1 hjh
	command_repeated_open_signal延续进路的终端不是调车信号和延续进路终端按钮时，不停止计时
2013/8/20 V1.2.1 hjh
	重复开放列车信号时增加检查延续进路的条件
2014/2/24 V1.2.1 hjh mantis:3393
	re_open_signal_process延续部分判断终端是进站信号机则开放信号
2014/4/11 LYC mantis:3582
	re_open_signal_process延续进路在重复开放信号时检查延续部分进路的区段是否故障
***************************************************************************************/
#include "open_signal.h"

/****************************************************
函数名:    repested_open_train_signal
功能描述:  重复开放列车信号
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/3/28
****************************************************/
void repeated_open_train_signal( route_t route_index );
/****************************************************
函数名:    re_open_signal_process
功能描述:  重复开放信号过程
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/4/16
****************************************************/
void re_open_signal_process( route_t route_index ) ;



/****************************************************
函数名:    repeated_open_signal
功能描述:  重复开放信号（进路控制模块调用）
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/3/29
****************************************************/
void repeated_open_signal( route_t route_index )
{
	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*进路状态为信号重复开放模块正在重复开放信号*/
		if (RS_SRO_SIGNAL_OPENING == gr_state(route_index))
		{
			/*执行开放信号流程*/
			signal_opening(route_index);
		}
	}
	FUNCTION_OUT;
}

/****************************************************
函数名:    command_repeated_open_signal
功能描述:  重复开放信号（命令处理模块调用）
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/4/18
****************************************************/
void command_repeated_open_signal( route_t route_index )
{
	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*hjh 20150611 重复开放信号时需必须检查列车位置状态机*/
		if ((gr_state_machine(route_index) == RSM_INIT)
			|| (gr_state_machine(route_index) == RSM_TRAIN_APPROACH)
			|| (gr_state_machine(route_index) == RSM_TRAIN_ABNORMAL_APPROACH)
			|| (gr_state_machine(route_index) == RSM_TRAIN_IN_POSSIBLE))
		{
			switch(gr_state(route_index))
			{
				case RS_A_SIGNAL_CLOSED:
					/*检查信号是否可以重复开放流程*/
					re_open_signal_process(route_index);
					break;
				case RS_ROUTE_LOCKED:
					/*判断是延续进路*/
					if (IsTRUE(is_successive_route(route_index)))
					{	
						/*检查信号是否可以重复开放流程*/
						re_open_signal_process(route_index);
					}
					break;
				case RS_AU_SUCCESSIVE_RELAY:
					/*判断是延续进路*/
					if (IsTRUE(is_successive_route(route_index)))
					{
						/*延续进路延时过程中重复开放信号时停止延时*/
						/*hjh 2013-8-12 延续进路的终端不是调车信号和延续进路终端按钮时，不停止计时*/
						if (IsTRUE(is_timer_run(route_index)) 
							&& (IsFALSE(is_shunting_signal(gb_node(gr_end_button(route_index)))))
							&& (gn_type(gb_node(gr_end_button(route_index))) != NT_SUCCESSIVE_END_BUTTON))
						{
							sr_stop_timer(route_index);
						}					
						re_open_signal_process(route_index);
					}
					break;
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
	FUNCTION_OUT;
}

/****************************************************
函数名:    re_open_signal_process
功能描述:  重复开放信号过程
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/4/16
****************************************************/
void re_open_signal_process( route_t route_index ) 
{
	route_t fwr = NO_INDEX;
	EN_node_type signal_type;

	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*调车进路*/
		if (RT_SHUNTING_ROUTE == gr_type(route_index))
		{
			/*检查开放条件*/
			if (IsTRUE(check_open_signal(route_index)))
			{
				/*hjh 2014-6-10 车列进入进路分录不良时不能重复开放*/
				if ((RSM_FIRST_SECTION != gr_state_machine(route_index))
					&& (RSM_SECOND_SECTION != gr_state_machine(route_index)))
				{
					/*hjh 2012-11-26 重复开放信号时清除进路上区段的历史状态信息*/
					clear_section_history_state(route_index);
					/*开放信号*/
					send_signal_command(gr_start_signal(route_index),SGS_B);
					sr_state(route_index,RS_SRO_SIGNAL_OPENING);
					sr_clear_error_count(route_index);
				}
				else
				{
					process_warning(ERR_TRAIN_IN_ROUTE,gr_name(route_index));
					CIHmi_SendNormalTips("车在进路中");
				}
			}
		}
		/*判断是延续进路*/
		else if (IsTRUE(is_successive_route(route_index)))
		{
			/*hjh 2014-2-24 延续部分判断终端是进站信号机则开放信号*/
			signal_type = gn_type(gb_node(gr_end_button(route_index)));
			if (signal_type == NT_ENTRY_SIGNAL)
			{
				repeated_open_train_signal(route_index);
			}
			else
			{
				process_warning(ERR_OPERATION,gn_name(gr_start_signal(route_index)));
				CIHmi_SendNormalTips("错误办理");
			}
		}
		/*列车信号*/
		else
		{
			/*hjh 2013/8/20 增加检查延续进路的联锁条件*/
			fwr = gr_forward(route_index);
			/*前方进路是延续进路*/			
			if ((fwr != NO_INDEX) && IsTRUE(is_successive_route(fwr)))
			{
				/*检查延续进路的联锁条件是否满足要求*/
				if (IsTRUE(check_successive_ci_condition(fwr)))
				{
					repeated_open_train_signal(route_index);
				}
			}
			else
			{
				repeated_open_train_signal(route_index);
			}
		}
	}
	FUNCTION_OUT;
}

/****************************************************
函数名:    repested_open_train_signal
功能描述:  重复开放列车信号
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/3/28
****************************************************/
void repeated_open_train_signal( route_t route_index )
{
	CI_BOOL result = CI_TRUE;
	EN_signal_state change_result;

	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*检查红灯断丝*/
		if (IsTRUE(is_red_filament(route_index)))
		{
			result = CI_FALSE;
			/*若信号机红灯断丝则将进路状态置为进路已锁闭*/
			//sr_state(route_index,RS_ROUTE_LOCKED);
		}
		
		if (IsTRUE(result))
		{
			/*检查联锁条件*/
			if (IsTRUE(check_open_signal(route_index)))
			{
				/*开放信号*/
				change_result = change_signal_color(route_index);
				if (change_result != SGS_H)
				{
					/*hjh 2012-10-26 重复开放信号时若检查此进路是引导进路则需清除引导标志*/
					if (IsTRUE(is_guide_route(route_index)))
					{
						clear_other_flag(route_index);
					}					
					/*发送命令及修改进路状态*/
					send_signal_command(gr_start_signal(route_index),change_result);
					sr_state(route_index,RS_SRO_SIGNAL_OPENING);
					sr_clear_error_count(route_index);
					sn_signal_expect_state(gr_start_signal(route_index),change_result);
					sn_signal_history_state(gr_start_signal(route_index),gn_signal_state(gr_start_signal(route_index)));
				}
			}
		}
	}
	FUNCTION_OUT;
}