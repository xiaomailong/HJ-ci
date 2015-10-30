/***************************************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.
文件名:  keep_signal.c
作者:    CY
版本 :   1.0	
创建日期:2011/12/2
用途:    信号保持模块
历史修改记录:         
2012/7/2,V1.1，CY:
	1.修改了部分警告信息，添加了关闭非正常开放的信号的函数(keep_signal.c  void abnormal_signal_close())
2012/8/1,V1.2，HJH:
	1.修改X5至D32列车进路的BUG
	2.增加判断接近区段曾占用过条件
	3.添加改方运行条件检查功能。 解决 Issue #63 closes #63
	4.修改了 延续进路 SX向XI的接车进路无法办理的问题 Issue #57.修改了进站信号显示颜色的代码，开放信号主要依据联锁表进行。
	5.调试并修改半自动闭塞功能
2012/11/27 V1.2.1 hjh
	1.train_signal_opened增加了信号开放后模拟点其他颜色灯光的处理
	2.abnormal_signal_close增加了信号非正常关闭后模拟点其他颜色灯光的处理
2012/12/5 V1.2.1 hjh
	entry_signal_color_definition增加对进路信号机部分的处理
2012/12/19 V1.2.1 hjh
	judge_train_route_sections中判断接近区段的函数将is_approach_cleard修改为直接判断始端信号机外方的第一个区段的状态
2013/1/6 V1.2.1 hjh
	train_signal_opened增加检查延续进路的联锁条件
2013/1/22 V1.2.1 hjh
	entry_signal_color_definition限速时无论前方出站开放任何信号，进站均点黄灯
2013/1/25 V1.2.1 hjh
	entry_signal_color_definition加强限速的条件，必须是进站信号机
2013/1/28 V1.2.1 hjh
	judge_interlocking_condition中增加检查特殊防护道岔的条件
2013/1/29 V1.2.1 hjh
	judge_shunting_route_sections修改折返信号机的判断方法
2013/3/8 V1.2.1 hjh
	is_relation_condition_ok增加检查半自动手续已办好的条件
2013/3/25 V1.2.1 hjh
	judge_shunting_route_sections配合调车中途折返时全部未解锁检查车列顺序退出本进路和其接近区段
	后方存在进路且进路故障时，则非正常关闭信号
2013/4/24 V1.2.1 hjh
	judge_shunting_route_sections修改多条牵出进路时寻找折返信号算法
2013/5/15 V1.2.1 hjh
	entry_signal_color_definition往D32办理列车进路开放信号时错误，已修正
2013/7/30 V1.2.1 LYC 
	check_node_route_locked增加了不检查防护道岔的锁闭情况
2013/8/2 V1.2.1 hjh
	1.train_signal_opened增加检查延续进路的轨道空闲
	2.keep_train_signal 除延续进路之外的其他进路的处理
2013/8/2 V1.2.1 hjh
	signal_changing中增加信号断丝（未转换成功）后将进路状态置为故障
2013/8/13 	LYC 
	abnormal_signal_close增加了当进路故障时进路始端信号应保持关闭状态
2013/8/12 V1.2.1 hjh
	out_signal_color_definition延续进路的终端的类型是延续进路终端按钮时亦不能开放信号
2013/8/20 V1.2.1 hjh
	is_relation_condition_ok增加场间联系的条件
2014/2/20 V1.2.1 hjh
	entry_signal_color_definition中增加若前方存在进路而该进路始端是进站信号机时亮黄灯，否则红灯
2014/2/25 V1.2.1 hjh
	judge_shunting_route_sections增加进路信号机类型
2014/2/28 V1.2.1 hjh mantis:3434
	check_node_route_locked增加检查中间道岔和特殊防护道岔的锁闭标志
2014/3/7 V1.2.1 hjh mantis:3416
	judge_route_section_failure中按照区段的状态机是故障则认为区段故障
2014/4/11 LYC V1.2.2 hjh mantis:3667
	shunting_signal_opened增加了调车信号开放时持续检查信号机状态
2014/5/8 V1.2.3 hjh
	修改调车正常和非正常关系信号的逻辑。
***************************************************************************************/
#include "keep_signal.h"
#include "guide_route.h"
#include "auto_unlock.h"

/****************************************************
函数名:    keep_signal
功能描述:  信号保持
返回值:    
参数:      int16_t route_index
作者  :    hejh
日期  ：   2012/7/23
****************************************************/
void keep_signal(route_t route_index)
{
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*调车信号保持*/
		if (RT_SHUNTING_ROUTE == gr_type(route_index))
		{
			keep_shunting_signal(route_index);
		}
		/*列车信号保持*/
		else
		{
			keep_train_signal(route_index);
		}
	}
}

/****************************************************
函数名:    keep_train_signal
功能描述:  列车信号保持模块
返回值:    
参数:      route_index
作者  :    CY
日期  ：   2011/12/2
****************************************************/
void keep_train_signal(route_t route_index)
{
	CI_BOOL error_flag = CI_FALSE;
	EN_route_state rs;
	int16_t temp;

	FUNCTION_IN;

	/*判断该进路是否存在*/
	if (IsFALSE(is_route_exist(route_index)))
	{
		error_flag = CI_TRUE;
	}

	if (IsFALSE(error_flag))
	{
		/*判断进路类型为列车*/
		if ((gr_type(route_index)) != RT_TRAIN_ROUTE)
		{
			error_flag = CI_TRUE;
		}
	}

	if (IsFALSE(error_flag))
	{
		/*根据不同的进路状态执行操作*/
		rs = gr_state(route_index);
		switch(rs)
		{
			/*进路状态为信号已开放*/
			case RS_SIGNAL_OPENED:
				train_signal_opened(route_index);
				break;

			/*进路状态为信号保持正在正常关闭信号*/
			case RS_SK_N_SIGNAL_CLOSING:
				signal_closing(route_index,RS_SK_N_SIGNAL_CLOSED);
				break;

			/*进路状态为信号保持正在非正常关闭信号*/
			case RS_SK_A_SIGNAL_CLOSING:
				signal_closing(route_index,RS_A_SIGNAL_CLOSED);
				break;

			case RS_G_SO_SIGNAL_CLOSING:
				signal_closing(route_index,RS_G_SIGNAL_CLOSED);
				break;

			/*进路状态为信号保持正在转换信号*/
			case RS_SK_SIGNAL_CHANGING:
				signal_changing(route_index);
				break;

			case RS_ROUTE_LOCKED:
				/*hjh 2013-8-2 除延续进路之外的其他进路的处理*/
				if (IsFALSE(is_successive_route(route_index)))
				{
					/*联锁条件正确*/
					if(IsFALSE(judge_interlocking_condition(route_index)))
					{	
						/*区段故障检查*/
						if (IsTRUE(judge_route_section_failure(route_index)))
						{
							process_warning(ERR_SECTION_OCCUPIED,gr_name(route_index));
							error_flag = CI_TRUE;
						} 
						/*侵限检查*/
						if (IsTRUE(check_exceed_limit(route_index))) 
						{
							process_warning(ERR_EXCEED_LIMIT_OCCUPIED,gr_name(route_index));
							error_flag = CI_TRUE;
						}

						if (IsTRUE(error_flag))
						{
							/*检查是否存在特殊联锁，表示灯*/
							if ((temp = gs_special(route_index,SPT_INDICATION_LAMP)) != NO_INDEX)
							{
								/*获取表示灯的状态*/
								if (gn_state(gs_indication_lamp(temp)) == SIO_HAS_SIGNAL)
								{
									/*设置表示灯的状态*/
									sn_state(gs_indication_lamp(temp),SIO_NO_SIGNAL);
									sr_state(route_index,RS_SK_A_SIGNAL_CLOSING);
									sr_clear_error_count(route_index);
								}
							}
						}
					}
					else
					{
						/*检查是否存在特殊联锁，表示灯*/
						if ((temp = gs_special(route_index,SPT_INDICATION_LAMP)) != NO_INDEX)
						{
							/*获取表示灯的状态*/
							if (gn_state(gs_indication_lamp(temp)) == SIO_HAS_SIGNAL)
							{
								/*设置表示灯的状态*/
								sn_state(gs_indication_lamp(temp),SIO_NO_SIGNAL);
								sr_state(route_index,RS_SK_A_SIGNAL_CLOSING);
								sr_clear_error_count(route_index);
							}
						}
					}
				}				
			default:
				break;
		}
	}
	FUNCTION_OUT;
}		

/****************************************************
函数名:    keep_shunting_signal
功能描述:  调车信号保持模块
返回值:    
参数:      route_index
作者  :    CY
日期  ：   2011/12/2
****************************************************/
void keep_shunting_signal(route_t route_index)
{
	CI_BOOL error_flag = CI_FALSE;
	EN_route_state rs;

	FUNCTION_IN;

	/*判断该进路是否存在*/
	if (IsFALSE(is_route_exist(route_index)))
	{
		error_flag = CI_TRUE;
	}	

	if (IsFALSE(error_flag))
	{
		/*判断进路类型为调车*/
		if ((gr_type(route_index)) != RT_SHUNTING_ROUTE)
		{
			error_flag = CI_TRUE;
		}
	}

	if (IsFALSE(error_flag))
	{
		/*根据不同的进路状态执行操作*/
		rs = gr_state(route_index);
		switch(rs)
		{
			/*进路状态为信号已开放*/
			case RS_SIGNAL_OPENED:
				shunting_signal_opened(route_index);
				break;

			/*进路状态为信号保持正在正常关闭信号*/
			case RS_SK_N_SIGNAL_CLOSING:
				signal_closing(route_index,RS_SK_N_SIGNAL_CLOSED);
				break;

			/*进路状态为信号保持正在非正常关闭信号*/
			case RS_SK_A_SIGNAL_CLOSING:
				signal_closing(route_index,RS_A_SIGNAL_CLOSED);
				break;

			default:
				break;
		}
	}
	FUNCTION_OUT;
}

/****************************************************
函数名:    train_signal_opened
功能描述:  进路状态为列车信号已开放时的执行过程
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/3/5
****************************************************/
void train_signal_opened( route_t route_index )
{
	CI_BOOL result = CI_TRUE;
	route_t fwr;
	route_t i;
	node_t start_signal = gr_start_signal(route_index);
	char_t tips[TEST_NAME_LENGTH];

	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)) && IsTRUE(is_guide_route(route_index)))
	{
		result = CI_FALSE;
	}

	if (IsTRUE(result))
	{
		if (gn_signal_state(start_signal) != SGS_ERROR)
		{
			/*检查允许信号断丝*/
			check_signal_allow_filament(route_index);	
			/*检查信号保持的联锁条件*/
			if(IsFALSE(judge_interlocking_condition(route_index)))
			{
				/*检查存在前一进路的信号保持联锁条件*/
				fwr = gr_forward(route_index);
				/*前方进路是延续进路*/
				/*hjh 2013/1/6 增加检查延续进路的联锁条件*/
				if ((fwr != NO_INDEX) && IsTRUE(is_successive_route(fwr)))
				{
					/*检查延续进路的联锁条件是否满足要求*/
					if (IsTRUE(judge_interlocking_condition(fwr)))
					{
						/*非正常关闭信号，设置进路状态为信号保持正在非正常关闭信号*/
						close_signal(route_index);
						sr_state(route_index,RS_SK_A_SIGNAL_CLOSING);
						sr_clear_error_count(route_index);
					}
					/*hjh 2013/8/2 增加检查延续进路的轨道空闲，是否要检查？？？*/
					/*区段故障检查*/
					//if (IsTRUE(judge_route_section_failure(fwr)))
					//{
					//	process_warning(ERR_SECTION_OCCUPIED,gr_name(fwr));
					//	/*非正常关闭信号，设置进路状态为信号保持正在非正常关闭信号*/
					//	close_signal(route_index);
					//	sr_state(route_index,RS_SK_A_SIGNAL_CLOSING);
					//} 
					///*侵限检查*/
					//if (IsTRUE(check_exceed_limit(fwr))) 
					//{
					//	process_warning(ERR_EXCEED_LIMIT_OCCUPIED,gr_name(fwr));
					//	/*非正常关闭信号，设置进路状态为信号保持正在非正常关闭信号*/
					//	close_signal(route_index);
					//	sr_state(route_index,RS_SK_A_SIGNAL_CLOSING);
					//}
				}
				else
				{
					/*延续部分不存在*/
					if ((fwr == NO_INDEX) && IsTRUE(have_successive_route(route_index)))
					{
						/*非正常关闭信号，设置进路状态为信号保持正在非正常关闭信号*/
						close_signal(route_index);
						process_warning(ERR_SUCCESSIVE_ROUTE,gr_name(route_index));
						/*输出提示信息*/
						memset(tips,0x00,sizeof(tips));
						strcat_check(tips,"延续进路条件不满足：",sizeof(tips));
						OutputHmiNormalTips(tips,route_index);
						sr_state(route_index,RS_SK_A_SIGNAL_CLOSING);
						sr_clear_error_count(route_index);
					}
				}

				/*普通进路*/
				if ((fwr != NO_INDEX) && IsFALSE(is_successive_route(fwr)))
				{
					/*若仍然存在前一进路则继续检查下去*/
					for (i = fwr; i != NO_INDEX;)
					{
						/*hjh 2012-12-5 前一进路的状态必须是信号已开放*/
						if ((fwr != NO_INDEX) && (gr_state(fwr) == RS_SIGNAL_OPENED))
						{
							/*检查前一进路允许信号断丝*/
							check_signal_allow_filament(fwr);
							if (IsFALSE(judge_interlocking_condition(fwr)))
							{
								/*检查区段状态*/
								judge_train_route_sections(fwr);
								/*hjh 2012-11-27 信号开放后模拟改点其他灯光时应立即关闭该信号*/
								if (gn_signal_history_state(gr_start_signal(fwr)) != gn_signal_state(gr_start_signal(fwr)))
								{
									/*非正常关闭信号，设置进路状态为信号保持正在非正常关闭信号*/
									close_signal(fwr);
									sr_state(fwr,RS_SK_A_SIGNAL_CLOSING);
									sr_clear_error_count(fwr);
								}
							}
						}
						fwr = gr_forward(i);
						i = fwr;
					}
				}

				/*本进路状态为信号已开放*/
				if (RS_SIGNAL_OPENED == gr_state(route_index))
				{
					/*判断本列车进路或通过进路区段状态*/
					judge_train_route_sections(route_index);
					/*hjh 2012-11-27 信号开放后模拟改点其他灯光时应立即关闭该信号*/
					if (gn_signal_history_state(start_signal) != gn_signal_state(start_signal))
					{
						/*非正常关闭信号，设置进路状态为信号保持正在非正常关闭信号*/
						close_signal(route_index);
						sr_state(route_index,RS_SK_A_SIGNAL_CLOSING);
						sr_clear_error_count(route_index);
					}
				}
			}
			/*联锁条件不成立，则非正常关闭信号*/
			else
			{
				/*非正常关闭信号，设置进路状态为信号保持正在非正常关闭信号*/
				close_signal(route_index);
				sr_state(route_index,RS_SK_A_SIGNAL_CLOSING);
				sr_clear_error_count(route_index);
			}
		}
		else
		{
			/*非正常关闭信号，设置进路状态为信号保持正在非正常关闭信号*/
			close_signal(route_index);
			sr_state(route_index,RS_SK_A_SIGNAL_CLOSING);
			sr_clear_error_count(route_index);
		}		
	}
	FUNCTION_OUT;
}
		
/****************************************************
函数名：   signal_closing
功能描述： 关闭信号时的处理过程
返回值：   void
参数：     int16_t route_index
参数：     EN_route_state route_state
作者：	   hejh
日期：     2013/04/25
****************************************************/
void signal_closing( route_t route_index,EN_route_state route_state ) 
{
	int16_t temp;

	FUNCTION_IN;
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
				sr_state(route_index,route_state);
				sr_clear_error_count(route_index);
				if (IsTRUE(is_guide_route(route_index)))
				{
					sn_signal_expect_state(gr_start_signal(route_index),SGS_ERROR);
				}
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
			/*检查信号已关闭*/
			if(IsTRUE(is_signal_close(gr_start_signal(route_index))))
			{
				CIHmi_SendNormalTips("信号关闭：%s",gn_name(gr_start_signal(route_index)));
				sr_state(route_index,route_state);
				sr_clear_error_count(route_index);
				if (IsTRUE(is_guide_route(route_index)))
				{
					sn_signal_expect_state(gr_start_signal(route_index),SGS_ERROR);
				}
			}
			/*信号未关闭，错误计数*/
			else
			{
				sr_increament_error_count(route_index);
				if (gr_error_count(route_index) > MAX_ERROR_PER_COMMAND)
				{
					sr_state(route_index,RS_FAILURE);
					sr_clear_error_count(route_index);
					process_warning(ERR_COMMAND_EXECUTE,gr_name(route_index));
				}
			}
		}
	}
	FUNCTION_OUT;
}

/****************************************************
函数名:    signal_changing
功能描述:  信号灯光转换执行过程
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/3/5
****************************************************/
void signal_changing( route_t route_index )
{
	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*判断信号机是否按照预期的颜色转换*/
		if (gn_signal_state(gr_start_signal(route_index)) == gn_signal_expect_state(gr_start_signal(route_index)))
		{
			sr_state(route_index,RS_SIGNAL_OPENED);
			sr_clear_error_count(route_index);
			CIHmi_SendNormalTips("信号开放：%s，%s",gn_name(gr_start_signal(route_index)),hmi_signal_tips(gn_signal_state(gr_start_signal(route_index))));
		}
		else
		{
			/*信号和预期的颜色不一致*/
			/*2013-08-05 hjh 信号断丝（未转换成功）后将进路状态置为故障*/
			if ((gn_signal_state(gr_start_signal(route_index)) != SGS_H)
				&& (gn_signal_state(gr_start_signal(route_index)) != SGS_FILAMENT_BREAK))
			{
				sr_increament_error_count(route_index);
				if (gr_error_count(route_index) > MAX_ERROR_PER_COMMAND)
				{
					close_signal(route_index);
					sr_state(route_index,RS_SK_A_SIGNAL_CLOSING);
					sr_clear_error_count(route_index);
					process_warning(ERR_SIGNAL_OPEN,gr_name(route_index));
				}
			}
			/*信号一直未转换成功*/
			else
			{
				sr_state(route_index,RS_FAILURE);
				close_signal(route_index);
				sr_clear_error_count(route_index);
				process_warning(ERR_COMMAND_EXECUTE,gr_name(route_index));
			}		
		}
		/*设置信号机的历史状态*/
		sn_signal_history_state(gr_start_signal(route_index),gn_signal_state(gr_start_signal(route_index)));
	}
	FUNCTION_OUT;
}

/****************************************************
函数名:    shunting_signal_opened
功能描述:  进路状态为信号已开放的执行过程
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/3/14
****************************************************/
void shunting_signal_opened(route_t route_index)
{
	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*判断允许信号断丝*/
		if (gn_signal_state(gr_start_signal(route_index)) == SGS_FILAMENT_BREAK)
		{
			/*输出提示信息*/
			//CIHmi_SendNormalTips("断丝：%s",gn_name(gr_start_signal(route_index)));
			close_signal(route_index);
			/*非正常关闭信号，设置进路状态为信号保持正在非正常关闭信号*/
			sr_state(route_index,RS_SK_A_SIGNAL_CLOSING);
			sr_clear_error_count(route_index);
		}
		else
		{
			if (gn_signal_state(gr_start_signal(route_index)) != SGS_ERROR)
			{
				/*2014/4/11 LYC 检查进路中信号开放的条件和信号机状态*/
				if ((IsFALSE(judge_interlocking_condition(route_index))) && (IsFALSE(is_signal_close(gr_start_signal(route_index)))))
				{
					/*判断调车进路区段状态*/
					judge_shunting_route_sections(route_index);
				}
				else
				{
					close_signal(route_index);
					/*非正常关闭信号，设置进路状态为信号保持正在非正常关闭信号*/
					sr_state(route_index,RS_SK_A_SIGNAL_CLOSING);
					sr_clear_error_count(route_index);
				}
			}
			else
			{
				close_signal(route_index);
				/*非正常关闭信号，设置进路状态为信号保持正在非正常关闭信号*/
				sr_state(route_index,RS_SK_A_SIGNAL_CLOSING);
				sr_clear_error_count(route_index);
			}
			
		}		
	}
	FUNCTION_OUT;
}

/****************************************************
函数名:    judge_interlocking_condition
功能描述:  联锁条件判断
返回值:    TRUE:联锁条件判断失败
		   FALSE：联锁条件判断成功
参数:      route_index

作者  :    hjh
日期  ：   2012/2/24
****************************************************/
CI_BOOL judge_interlocking_condition( route_t route_index ) 
{
	CI_BOOL error_flag = CI_FALSE;
	
	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*检查进路上的信号点没有设备错误状态*/
		if (IsFALSE(is_all_nodes_device_ok(route_index))) 
		{
			//error_flag = CI_TRUE;
		}
		/*检查始端信号机未通信中断*/
		if (gn_signal_state(gr_start_signal(route_index)) == SGS_ERROR)
		{
			error_flag = CI_FALSE;
			CIHmi_SendNormalTips("通信中断：%s",gn_name(gr_start_signal(route_index)));
		}
		
		/*进路上道岔位置检查*/
		if (IsFALSE(check_switch_location(route_index)) 
			|| IsFALSE(check_middle_switch_location(route_index))
			|| IsFALSE(keep_signal_middle_switch_ok(route_index))
			|| IsFALSE(check_special_switch_location(route_index)))
		{
			process_warning(ERR_SWITCH_LOCATION,gr_name(route_index));
			error_flag = CI_TRUE;
		}

		/*进路上信号点锁闭标志检查*/
		if (IsFALSE(check_node_route_locked(route_index)))
		{
			process_warning(ERR_NODE_UNLOCK,gr_name(route_index));
			error_flag = CI_TRUE;
		}

		/*敌对信号检查*/
		if (IsTRUE(check_conflict_signal(route_index))) 
		{
			process_warning(ERR_CONFLICT_ROUTE,gr_name(route_index));
			error_flag = CI_TRUE;
		}

		/*联系条件检查*/
		if (IsFALSE(is_relation_condition_ok(route_index)))
		{
			process_warning(ERR_RELATION_CONDITION,gr_name(route_index));
			error_flag = CI_TRUE;
		}
	}
	FUNCTION_OUT;
	return error_flag;
}

/****************************************************
函数名:    judge_train_route_sections
功能描述:  对列车进路上区段的状态进行判断
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/3/5
****************************************************/
void judge_train_route_sections( route_t route_index) 
{
	FUNCTION_IN;
	/*该进路接近区段空闲检查*/
	if (IsTRUE(is_approach_cleared(route_index,CI_FALSE)))
	{
		judge_train_route_remain_sections(route_index);
	}
	/*接近区段占用*/
	else
	{
		/*进路内第一区段占用*/
		if ((SCS_CLEARED != gn_section_state(gr_approach(route_index))) 
			&& (SCS_CLEARED != gn_section_state(gr_first_section(route_index))))
		{
			/*关闭信号，设置进路状态为信号保持正在正常关闭信号*/
			close_signal(route_index);
			sr_state(route_index,RS_SK_N_SIGNAL_CLOSING);
			sr_clear_error_count(route_index);
		}
		/*判断其他区段状态*/
		else
		{
			judge_train_route_remain_sections(route_index);
		}
	}
	FUNCTION_OUT;
}

/****************************************************
函数名:    judge_train_route_remain_sections
功能描述:  检查列车进路剩余区段空闲
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/3/8
****************************************************/
void judge_train_route_remain_sections( route_t route_index) 
{
	CI_BOOL error_flag = CI_FALSE;
	EN_signal_state pro_state;
	EN_signal_state change_result;
	
	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*区段故障检查*/
		if (IsTRUE(judge_route_section_failure(route_index)))
		{
			process_warning(ERR_SECTION_OCCUPIED,gr_name(route_index));
			error_flag = CI_TRUE;
		} 
		/*侵限检查*/
		if (IsTRUE(check_exceed_limit(route_index))) 
		{
			process_warning(ERR_EXCEED_LIMIT_OCCUPIED,gr_name(route_index));
			error_flag = CI_TRUE;
		}

		if (IsFALSE(error_flag))
		{
			/*获取信号机当前状态*/
			pro_state = gn_signal_state(gr_start_signal(route_index));
			/*灯光转换*/
			change_result = change_signal_color(route_index);
			/*信号机曾断丝过*/
			if (IsTRUE(has_signal_filamented(gr_start_signal(route_index))))
			{
				/*转换后的信号等级比当前状态等级低时才能改变信号机的状态*/
				if (IsTRUE(is_low_signal(pro_state,change_result)))
				{
					send_signal_command(gr_start_signal(route_index),change_result);
					sr_state(route_index,RS_SK_SIGNAL_CHANGING);
					process_warning(ERR_FILAMENT_BREAK,gr_name(route_index));
					sn_signal_expect_state(gr_start_signal(route_index),change_result);
					sr_clear_error_count(route_index);
				}
			}
			/*信号机未曾断丝过*/
			else
			{
				/*根据转换后的结果显示*/
				send_signal_command(gr_start_signal(route_index),change_result);
				if (SGS_H == change_result)
				{
					close_signal(route_index);
					sr_state(route_index,RS_SK_A_SIGNAL_CLOSING);
					sr_clear_error_count(route_index);
				}
				else
				{
					if (pro_state != change_result)
					{
						sr_state(route_index,RS_SK_SIGNAL_CHANGING);
						sn_signal_expect_state(gr_start_signal(route_index),change_result);
						sr_clear_error_count(route_index);
					}
				}				
			}
		}
		/*联锁条件检查不成立则关闭信号*/
		else
		{
			close_signal(route_index);
			sr_state(route_index,RS_SK_A_SIGNAL_CLOSING);
			sr_clear_error_count(route_index);
		}
	}	
	FUNCTION_OUT;
}

/****************************************************
函数名:    has_signal_filamented
功能描述:  信号机是否断丝过
返回值:    
参数:      node_t index
作者  :    hejh
日期  ：   2012/8/10
****************************************************/
CI_BOOL has_signal_filamented(node_t index)
{
	CI_BOOL result = CI_FALSE;
	
	/*参数检查*/
	if (index )
	{
	}
	/*获取信号机的历史状态，并右移16位获取信号机是否断丝过*/
	if ((gn_history_state(index) >> 16) == SGS_FILAMENT_BREAK)
	{
		result = CI_TRUE;
	}
	return result;
}

/****************************************************
函数名:    judge_shunting_route_sections
功能描述:  对调车进路上区段的状态进行判断
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/3/5
****************************************************/
void judge_shunting_route_sections( route_t route_index ) 
{
	int16_t i;

	FUNCTION_IN;

	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*车列接近*/
		if (RSM_TRAIN_APPROACH == gr_state_machine(route_index))
		{
			/*判断其他区段状态*/
			judge_shunting_route_remain_sections(route_index);
		}		
		/*车列压入首区段或第二区段*/
		else if ((RSM_FIRST_SECTION == gr_state_machine(route_index))
			|| (RSM_SECOND_SECTION == gr_state_machine(route_index)))
		{
			/*判断其他区段状态*/
			judge_shunting_route_remain_sections(route_index);

			if (RS_SK_A_SIGNAL_CLOSING != gr_state(route_index))
			{
				/*机务段调车信号机*/
				if (gn_type(gr_start_signal(route_index)) == NT_LOCODEPOT_SHUNTING_SIGNAL)			
				{
					/*关闭信号，设置进路状态为信号保持正在正常关闭信号*/
					close_signal(route_index);
					sr_state(route_index,RS_SK_N_SIGNAL_CLOSING);
					sr_clear_error_count(route_index);
				}
				/*调车中途折返*/
				if (SCSM_SELF_RETURN == gn_state_machine(gr_first_section(route_index)))
				{
					/*关闭信号，设置进路状态为信号保持正在正常关闭信号*/
					close_signal(route_index);
					sr_state(route_index,RS_SK_N_SIGNAL_CLOSING);
					sr_clear_error_count(route_index);
				}
				/*不存在折返*/
				for ( i = gr_nodes_count(route_index) - 1; i >= 0; i-- )
				{
					if (IsTRUE(is_section(gr_node(route_index,i))) && (SCSM_FAULT_CLEARED == gn_state_machine(gr_node(route_index,i))))
					{
						/*关闭信号，设置进路状态为信号保持正在非正常关闭信号*/
						close_signal(route_index);
						sr_state(route_index,RS_SK_A_SIGNAL_CLOSING);
						sr_clear_error_count(route_index);
						break;
					}
				}
			}			
		}
		/*车列完全进入进路*/
		else if (RSM_TRAIN_IN_ROUTE == gr_state_machine(route_index))
		{
			/*关闭信号，设置进路状态为信号保持正在正常关闭信号*/
			close_signal(route_index);
			sr_state(route_index,RS_SK_N_SIGNAL_CLOSING);
			sr_clear_error_count(route_index);
		}
		else
		{
			judge_shunting_route_remain_sections(route_index);
		}
	}
	FUNCTION_OUT;
}

/****************************************************
函数名:    judge_shunting_route_remain_sections
功能描述:  检查调车进路其他区段空闲
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/3/8
****************************************************/
void judge_shunting_route_remain_sections( route_t route_index ) 
{
	CI_BOOL error_flag = CI_FALSE;
	
	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*区段故障检查*/
		if (IsTRUE(judge_route_section_failure(route_index)))
		{
			process_warning(ERR_SECTION_OCCUPIED,gr_name(route_index));
			error_flag = CI_TRUE;
		} 
		/*侵限检查*/
		if (IsTRUE(check_exceed_limit(route_index))) 
		{
			process_warning(ERR_EXCEED_LIMIT_OCCUPIED,gr_name(route_index));
			error_flag = CI_TRUE;
		}

		/*调车联锁条件检查不成立*/
		if (IsTRUE(error_flag))
		{
			close_signal(route_index);
			sr_state(route_index,RS_SK_A_SIGNAL_CLOSING);
			sr_clear_error_count(route_index);
		}
		
		//CIHmi_SendDebugTips("周期号:%d, 进路%s正常, 状态：%d",CICycleInt_GetCounter(),gn_name(gr_start_signal(route_index)),gn_signal_state(gr_start_signal(route_index)));
	}	
	FUNCTION_OUT;
}

/****************************************************
函数名:    change_signal_color
功能描述:  信号机显示颜色转换
返回值:    
参数:      route_index
作者  :    hjh
日期  ：   2011/12/21
****************************************************/
EN_signal_state change_signal_color(route_t route_index)
{
	EN_signal_state result = SGS_H;
	EN_node_type signal_type;
	int16_t temp;
	
	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		signal_type = gn_type(gr_start_signal(route_index));
		/*信号机类型为进站信号机或进路信号机*/
		if ((signal_type == NT_ENTRY_SIGNAL) || (signal_type == NT_ROUTE_SIGNAL))
		{
			result = entry_signal_color_definition(route_index);
		}
		/*信号机类型为出站信号机或出站兼调车信号机*/
		else if ((signal_type == NT_OUT_SIGNAL) || (signal_type == NT_OUT_SHUNTING_SIGNAL))
		{
			result = out_signal_color_definition(route_index);
		}
		/*以D32为始端办理进路*/
		else
		{
			if ((temp = gs_special(route_index,SPT_INDICATION_LAMP)) != NO_INDEX)
			{
				/*设置表示灯的状态*/
				sn_state(gs_indication_lamp(temp),SIO_HAS_SIGNAL);
			}
		}
	}
	FUNCTION_OUT;
	return result;
}

/****************************************************
函数名:    auto_block_check
功能描述:  自动闭塞条件检查
返回值:    
参数:      LQ1
参数:      LQ2
参数:      LQ3
作者  :    hjh
日期  ：   2011/12/27
****************************************************/
EN_signal_state auto_block_check(int16_t special_index)
{
	int16_t LQ1,LQ2,LQ3;
	EN_signal_state result = SGS_H;
	
	FUNCTION_IN;
	/*参数检查*/
	if ((special_index >= 0) && (special_index < TOTAL_AUTO_BLOCK))
	{
		LQ1 = gs_auto_block(special_index,0);
		LQ2 = gs_auto_block(special_index,1);
		LQ3 = gs_auto_block(special_index,2);
		/*离去区段均空闲*/
		if ((SCS_CLEARED == gn_section_state(LQ1))
			&& (SCS_CLEARED == gn_section_state(LQ2))
			&& (SCS_CLEARED == gn_section_state(LQ3)))
		{
			result = SGS_L;
		}
		/*三离去占用*/
		else if ((SCS_CLEARED == gn_section_state(LQ1))
			&& (SCS_CLEARED == gn_section_state(LQ2))
			&& (SCS_CLEARED != gn_section_state(LQ3)))
		{
			result = SGS_LU;
		}
		/*二离去占用*/
		else if ((SCS_CLEARED == gn_section_state(LQ1))
			&& (SCS_CLEARED != gn_section_state(LQ2)))
		{
			result = SGS_U;
		}
		/*一离去占用*/
		else
		{
			result = SGS_H;
		}
	}
	FUNCTION_OUT;
	return result;
}

/****************************************************
函数名:    auto_block3_check
功能描述:  三显示自动闭塞条件检查
返回值:    
参数:      LQ1
参数:      LQ2
作者  :    hjh
日期  ：   2011/12/27
****************************************************/
EN_signal_state auto_block3_check(int16_t special_index)
{
	int16_t LQ1,LQ2;
	EN_signal_state result =  SGS_H;
	
	FUNCTION_IN;
	/*参数检查*/
	if ((special_index >= 0) && (special_index < TOTAL_AUTO_BLOCK3))
	{
		LQ1 = gs_auto_block3(special_index,0);
		LQ2 = gs_auto_block3(special_index,1);
		/*只有一个离去区段*/
		if (LQ2 == NO_INDEX)
		{
			/*离去区段空闲*/
			if (SCS_CLEARED == gn_section_state(LQ1))
			{
				result = SGS_L;
			}
			else
			{
				result =  SGS_H;
			}
		}
		else
		{
			/*离去区段均空闲*/
			if ((SCS_CLEARED == gn_section_state(LQ1))
				&& (SCS_CLEARED == gn_section_state(LQ2)))
			{
				result =  SGS_L;
			}
			/*二离去区段占用*/
			else if ((SCS_CLEARED == gn_section_state(LQ1))
				&& (SCS_CLEARED != gn_section_state(LQ2)))
			{
				result =  SGS_U;
			}
			/*一离去区段占用*/
			else
			{
				result =  SGS_H;
			}
		}
	}
	FUNCTION_OUT;
	return result;
}

/****************************************************
函数名:    entry_signal_color_definition
功能描述:  进站信号机信号颜色定义
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/3/6
****************************************************/
EN_signal_state entry_signal_color_definition( route_t route_index )
{
	EN_signal_state result = SGS_H;
	int16_t fw_signal,i,index;
	route_t fwr;
	EN_node_direction route_dir = DIR_ERROR;
	node_t last_node = NO_INDEX, search_node = NO_INDEX,next_index = NO_INDEX;
	int16_t temp;
	CI_BOOL judge_result = CI_TRUE;

	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		result = gn_signal_state(gr_start_signal(route_index));
		fwr = gr_forward(route_index);
		/*存在前一进路且不是延续进路*/
		if (NO_INDEX != fwr)
		{
			fw_signal = gr_start_signal(fwr);
			/*前一进路和本进路始端信号机的方向相同*/
			if (gn_direction(fw_signal) == gn_direction(gr_start_signal(route_index)))
			{
				/*前一进路始端是进路信号机*/
				if ((gn_type(fw_signal) == NT_ROUTE_SIGNAL) && ((gn_type(gr_start_signal(route_index)) == NT_ROUTE_SIGNAL) 
					|| (gn_type(gr_start_signal(route_index)) == NT_OUT_SHUNTING_SIGNAL) || (gn_type(gr_start_signal(route_index)) == NT_OUT_SIGNAL)))
				{
					/*根据前一信号机的状态进行显示*/
					switch (gn_signal_state(fw_signal))
					{
						case SGS_H:
							result = SGS_H;						
							break;
						case SGS_U:
							result = SGS_LU;
							break;
						case SGS_UU:
							result = SGS_U;
							break;
						case SGS_LU:
							result = SGS_L;
							break;
						case SGS_L:
							result = SGS_L;
							break;
						default:
							break;
					}
				}

				/*前一进路始端是进路信号机*/
				if ((gn_type(fw_signal) == NT_ROUTE_SIGNAL) && (gn_type(gr_start_signal(route_index)) == NT_ENTRY_SIGNAL))
				{
					if (IsTRUE(all_switchs_is_normal(route_index)))
					{
						/*根据前一信号机的状态进行显示*/
						switch (gn_signal_state(fw_signal))
						{
						case SGS_H:
							result = SGS_U;						
							break;
						case SGS_U:
							result = SGS_LU;
							break;
						case SGS_UU:
							result = SGS_U;
							break;
						case SGS_LU:
							result = SGS_L;
							break;
						case SGS_L:
							result = SGS_L;
							break;
						default:
							break;
						}
					}
					else
					{
						result = SGS_UU;
					}					
				}

				/*前一进路始端是出站信号机*/
				if(IsTRUE(is_out_signal(fw_signal)))
				{
					/*正线接车进路*/
					if (IsTRUE(all_switchs_is_normal(route_index)))
					{
						/*正线发车进路*/
						if (IsTRUE(all_switchs_is_normal(fwr)))
						{
							/*hjh 2013-1-22 限速时无论前方出站开放任何信号，进站均点黄灯*/
							/*进站信号机是否限速,获取限速的状态*/
							if ((gn_type(gr_start_signal(route_index)) == NT_ENTRY_SIGNAL)
								&&((temp = gs_special(route_index,SPT_STATE_COLLECT)) != NO_INDEX) 
								&& (gn_state(gs_state_collect(temp)) != SIO_HAS_SIGNAL))
							{
								/*限速时进站亮黄灯*/
								result = SGS_U;
							}
							/*无限速*/
							else
							{
								/*根据前一信号机的状态进行显示*/
								switch (gn_signal_state(fw_signal))
								{
									case SGS_H:
										result = SGS_U;
										break;
									case SGS_U:
										result = SGS_LU;
										break;
									case SGS_LU:
										result = SGS_L;
										break;
									case SGS_L:
										result = SGS_L;
										break;
									default:
										break;
								}
							}
						}
						else
						{
							result = SGS_U;
						}
					}
					/*侧线接车进路*/
					else
					{
						fw_signal = gr_start_signal(fwr);
						/*前一进路和本进路始端信号机的方向相同*/
						if (gn_direction(fw_signal) == gn_direction(gr_start_signal(route_index)))
						{
							/*前一进路始端是出站信号机*/
							if(IsTRUE(is_out_signal(fw_signal)))
							{
								/*正线发车进路*/
								if (IsTRUE(all_switchs_is_normal(fwr)))
								{
									/*经由18#道岔反位的通过进路开放黄闪黄*/
									/*检查进路中所有道岔位置*/
									for (i = 0; i < gr_nodes_count(route_index); i++)
									{
										index = gr_node(route_index,i);
										if (IsTRUE(is_switch(index)) && (gn_switch_state(index) == SWS_REVERSE) 
											&& IsFALSE(is_switch_location_reverse(index)) && IsFALSE(is_switchs18(index)))
										{
											if (gn_another_switch(index) != NO_INDEX)
											{
												if (IsFALSE(is_switchs18(gn_another_switch(index))))
												{
													judge_result = CI_FALSE;
													break;
												}										
											}
											else
											{
												judge_result = CI_FALSE;
												break;
											}
										}
									}
									if (IsTRUE(judge_result))
									{
										/*根据前一信号机的状态进行显示*/
										if ((SGS_H == gn_signal_state(fw_signal))
											&& (SGS_FILAMENT_BREAK == gn_signal_state(fw_signal))
											&& (SGS_ERROR == gn_signal_state(fw_signal)))
										{
											result = SGS_UU;
										}
										/*前方是允许信号则亮黄闪黄*/
										else
										{
											/*闪光电源正常则亮黄闪黄，否则亮双黄*/
											if (IsTRUE(is_flash_power_on()))
											{
												result = SGS_USU;
											}
											else
											{
												result = SGS_UU;
											}
										}
									}
									else
									{
										result = SGS_UU;
									}							
								}
								/*侧线接车进路*/
								else
								{							
									result = SGS_UU;
								}
							}
						}
					}
				}
			}	
		}
		/*不存在前一进路*/
		else
		{
			/*hjh 2012-12-4*/
			/*获取本进路的方向和最后一个信号点*/
			route_dir = gr_direction(route_index);
			last_node = gr_node(route_index,(gr_nodes_count(route_index) - 1));
			/*找出连续的下一个信号点*/
			for (i = 0; i < TOTAL_SIGNAL_NODE; i++)
			{
				/*上行找出左节点*/
				if (route_dir == DIR_UP)
				{
					search_node = gn_previous(last_node);
					if (search_node != NO_INDEX)
					{
						next_index = gn_previous(search_node);
					}
				}
				/*下行找出右节点*/
				if (route_dir == DIR_DOWN)
				{
					search_node = gn_next(last_node);
					if (search_node != NO_INDEX)
					{
						next_index = gn_next(search_node);
					}
				}
				/*信号点与进路的方向一致且为进路信号机*/
				if ((search_node == NO_INDEX) 
				||((gn_direction(search_node) == route_dir)
				&& ((gn_type(search_node) == NT_ROUTE_SIGNAL)
				|| (gn_type(search_node) == NT_ENTRY_SIGNAL)
				|| (gn_type(search_node) == NT_OUT_SIGNAL)
				|| (gn_type(search_node) == NT_OUT_SHUNTING_SIGNAL))))
				{
					break;
				}
				else
				{
					last_node = search_node;
				}
			}
			/*找到的节点是出站信号机或进路信号机且为死节点*/
			if ((gn_type(gr_start_signal(route_index)) == NT_ENTRY_SIGNAL)
			|| (search_node == NO_INDEX)
			|| ((next_index == NO_INDEX) && (gn_type(search_node) == NT_ROUTE_SIGNAL))
			|| (gn_type(search_node) == NT_OUT_SHUNTING_SIGNAL)
			|| (gn_type(search_node) == NT_OUT_SIGNAL))
			{
				/*正线接车进路*/
				if ((search_node!= NO_INDEX) && IsTRUE(all_switchs_is_normal(route_index)))
				{
					/*搜索到的节点与本进路在同一咽喉时*/
					if (gn_throat(gr_start_signal(route_index)) == gn_throat(search_node))
					{
						/*根据前一信号机的状态进行显示*/
						switch (gn_signal_state(search_node))
						{
							case SGS_H:
								result = SGS_U;
								break;
							case SGS_U:
								result = SGS_LU;
								break;
							case SGS_UU:
								result = SGS_LU;
								break;
							case SGS_LU:
								result = SGS_L;
								break;
							case SGS_L:
								result = SGS_L;
								break;
							default:
								break;
						}
					}
					/*搜索到的节点与本进路不在同一咽喉*/
					else
					{
						result = SGS_U;
					}				
				}
				/*侧线接车进路*/
				else
				{
					/*往D32办理列车进路时开放黄灯*/
					if ((gn_type(gr_start_signal(route_index)) == NT_OUT_SIGNAL)
						|| (gn_type(gr_start_signal(route_index)) == NT_OUT_SHUNTING_SIGNAL))
					{
						result = SGS_U;
					}
					/*hjh 2013-5-15 进站和进路信号机执行下面代码*/
					if ((gn_type(gr_start_signal(route_index)) == NT_ENTRY_SIGNAL)
						|| (gn_type(gr_start_signal(route_index)) == NT_ROUTE_SIGNAL))
					{
						result = SGS_UU;
					}					
				}
			}
			/*办理以D38为列车终端的进路时不开放信号*/
			else
			{
				result = SGS_H;
			}
		}
	}
	FUNCTION_OUT;
	return result;
}

/****************************************************
函数名:    change_run_dir_color
功能描述:  获取改方运行时的出站信号灯光颜色
返回值:    
参数:      int16_t special_index
参数:      int16_t route_index
作者  :    CY
日期  ：   2012/7/25
****************************************************/
EN_signal_state change_run_dir_color(int16_t special_index, route_t route_index)
{
	EN_signal_state result = SGS_H;
	node_t check_node = NO_INDEX;
	
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)) 
	&& (special_index >= 0) && (special_index < TOTAL_CHANGE_RUN_DIR))
	{
		/*获取改方运行的同意点*/
		check_node = gs_change_run_dir(special_index,0);
		if (check_node != NO_INDEX)
		{
			/*改方运行同意*/
			if (gn_state(check_node) == SIO_HAS_SIGNAL)
			{
				/*必须所有离去区段空闲才能开放信号*/
				if ((gn_section_state(gs_change_run_dir(special_index,1)) == SCS_CLEARED)
					&& (gn_section_state(gs_change_run_dir(special_index,2)) == SCS_CLEARED)
					&& (gn_section_state(gs_change_run_dir(special_index,3)) == SCS_CLEARED))
				{
					result = SGS_L;
				}
				else
				{
					process_warning(ERR_DEPARTURE_OCCUPIED,gr_name(route_index));
				}
			}
			/*改方运行未同意*/
			else
			{
				process_warning(ERR_RELATION_CONDITION,gr_name(route_index));
			}
		}
	}
	return result;
}

/****************************************************
函数名:    out_signal_color_definition
功能描述:  出站信号机信号颜色定义
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/3/6
****************************************************/
EN_signal_state out_signal_color_definition( route_t route_index )
{
	EN_signal_state result = SGS_H;
	int16_t end_signal = NO_INDEX,section = NO_INDEX;
	int16_t temp;
	
	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*根据区间闭塞方式分别处理*/
		end_signal = gb_node(gr_end_button(route_index));
		/*进路终端为进站信号机*/
		if ((gn_type(end_signal) == NT_ENTRY_SIGNAL)
			|| (gn_type(end_signal) == NT_ROUTE_SIGNAL))
		{
			/*自动闭塞*/
			if ((temp = gs_special(route_index,SPT_AUTO_BLOCK)) != NO_INDEX)
			{
				/*自动闭塞条件检查*/
				result = auto_block_check(temp);
				if (result == SGS_H)
				{
					process_warning(ERR_DEPARTURE_OCCUPIED,gr_name(route_index));
				}
			}
			/*三显示自动闭塞*/
			else if ((temp = gs_special(route_index,SPT_AUTO_BLOCK3)) != NO_INDEX)
			{
				/*三显示自动闭塞条件检查*/
				result = auto_block3_check(temp);
				if (result == SGS_H)
				{
					process_warning(ERR_DEPARTURE_OCCUPIED,gr_name(route_index));
				}
			}
			/*半自动闭塞*/
			else if ((temp = gs_special(route_index,SPT_SEMI_AUTO_BLOCK)) != NO_INDEX)
			{								
				/*对方站同意接车或出站信号允许开放*/
				if (SAB_RECIEVED_AGREEMENT == gsab_state(temp) || SAB_OUT_SIGNAL_OPENED  == gsab_state(temp))
				{
					result = SGS_L;
				}
				else
				{
					result = SGS_H;
				}
			}
			/*改方运行*/
			else if ((temp = gs_special(route_index,SPT_CHANGE_RUN_DIR)) != NO_INDEX)
			{
				result = change_run_dir_color(temp,route_index);
			}
			else
			{				
				if (gr_type(route_index) == RT_TRAIN_ROUTE)
				{
					section = gr_forward_section(route_index,gr_node_index_route(route_index,end_signal));
					if (SCS_CLEARED == gn_section_state(section))
					{
						result = SGS_L;
					}
					else
					{
						CIHmi_SendNormalTips("区段占用");
					}
				}
				else
				{
					result = SGS_H;
				}
			}
		}
		/*延续进路的终端是调车信号时，不允许开放信号*/
		/*hjh 2013-8-12 延续进路的终端的类型是延续进路终端按钮时亦不能开放信号*/
		else if ((IsTRUE(is_successive_route(route_index)) 
			&& (IsTRUE(is_shunting_signal(end_signal)))) || (gn_type(end_signal) == NT_SUCCESSIVE_END_BUTTON))
		{
			result = SGS_H;
			process_warning(ERR_OPERATION,gr_name(route_index));
		}
		/*进路信号机部分的处理*/
		else
		{
			//result = entry_signal_color_definition(route_index);
		}
	}
	FUNCTION_OUT;
	return result;
}

/****************************************************
函数名:    check_node_route_locked
功能描述:  检查所有信号点均被进路锁闭
返回值:    均被锁闭返回真，否则返回假
参数:      route_index

作者  :    hjh
日期  ：   2012/2/27
****************************************************/
CI_BOOL check_node_route_locked(route_t route_index)
{
	int16_t i,j, count, index,end_signal;
	CI_BOOL judge_result = CI_TRUE;
	int16_t si,ms,special_switch;

	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		count = gr_nodes_count(route_index);
		for (i = 0; i < count; i++)
		{
			/*不检查单置信号机*/
			index = gr_node(route_index, i);
			if (i == (count - 1) && (gn_type(gr_node(route_index, i)) == NT_SINGLE_SHUNTING_SIGNAL))
			{
				continue;
			}
			/*hjh 2015-8-11 进路信号机处的调车进路，终端是同方向的尽头线信号机*/
			if (i == (count - 1) 
				&& (gn_direction(gr_node(route_index, i)) == gn_direction(gr_start_signal(route_index)))
				&& (gr_type(route_index) == RT_SHUNTING_ROUTE)
				&& (gn_type(gr_start_signal(route_index)) == NT_ROUTE_SIGNAL)
				&& (gn_type(gr_node(route_index, i)) == NT_STUB_END_SHUNTING_SIGNAL))
			{
				continue;
			}
			/*不检查防护道岔的锁闭情况*/
			if (gn_type(gr_node(route_index, i)) == NT_SWITCH)
			{	
				/*判断是防护道岔则不设置锁闭标志*/
				for (j = 0; j < gr_switches_count(route_index); j++)
				{
					if ((gr_node(route_index, i) == gr_switch(route_index,j))
						|| (gn_another_switch(gr_node(route_index, i)) == gr_switch(route_index,j)))
					{
						/*防护道岔不添加锁闭标志*/
						if (IsFALSE(is_protective_switch(route_index,j)))
						{
							/*信号点未锁闭*/
							if (IsFALSE(is_node_locked(index,LT_LOCKED)))
							{
								judge_result = CI_FALSE;
							}
						}
						break;
					}
				}				
			}
			else
			{
				/*信号点未锁闭*/
				if (IsFALSE(is_node_locked(index,LT_LOCKED)))
				{
					judge_result = CI_FALSE;
					if (IsTRUE(is_section(index)))
					{
						CIHmi_SendNormalTips("区段未锁闭：%s",gn_name(index));
					}
					break;
				}
			}
			///*信号点未锁闭*/
			//if (IsFALSE(is_node_locked(index,LT_LOCKED)))
			//{
			//	judge_result = CI_FALSE;
			//	if (IsTRUE(is_section(index)))
			//	{
			//		CIHmi_SendNormalTips("区段未锁闭：%s",gn_name(index));
			//	}
			//	break;
			//}
		}
		end_signal = gb_node(gr_end_button(route_index));
		if ((NT_ENTRY_SIGNAL == gn_type(end_signal)) || (NT_ROUTE_SIGNAL == gn_type(end_signal)) || (NT_TRAIN_END_BUTTON == gn_type(end_signal)))
		{
			/*hjh 2014-2-28 检查中间道岔的锁闭标志*/
			si = gs_middle_switch_index(gr_start_signal(route_index));
			/*检查中间道岔是否存在*/
			if (si != NO_INDEX)
			{
				for (j = 0; j < MAX_MIDDLE_SWITCHES; j++)
				{
					/*找出中间道岔索引号*/
					ms = gs_middle_switch(si,j);
					if ((ms != NO_INDEX) && (gn_belong_route(ms) == NO_INDEX) && IsFALSE(is_node_locked(ms,LT_MIDDLE_SWITCH_LOCKED)))
					{
						/*中间道岔未锁闭*/
						judge_result = CI_FALSE;
						CIHmi_SendNormalTips("中间道岔未锁闭：%s",gn_name(ms));
						break;
					}
				}
			}
		}

		/*hjh 2014-2-28 检查特殊防护道岔的锁闭标志*/
		/*检查是否存在特殊防护道岔*/
		if ((si = gs_special(route_index,SPT_SPECIAL_SWTICH)) != NO_INDEX)
		{
			for (i = 0;i < MAX_SPECIAL_SWITCHES; i++)
			{
				/*获取特殊防护道岔的索引号*/
				special_switch = gs_special_switch_index(si,i);
				if ((special_switch != NO_INDEX) && IsFALSE(is_node_locked(special_switch,LT_MIDDLE_SWITCH_LOCKED)))
				{
					/*特殊防护道岔未锁闭*/
					judge_result = CI_FALSE;
					CIHmi_SendNormalTips("特殊防护道岔未锁闭：%s",gn_name(special_switch));
					break;
				}
			}
		}
	}
	FUNCTION_OUT;
	return judge_result;
}

/****************************************************
函数名:    judge_route_section_failure
功能描述:  判断进路上区段是否故障
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/3/8
****************************************************/
CI_BOOL judge_route_section_failure(route_t route_index)
{
	CI_BOOL result = CI_FALSE;
	int16_t count,i,index;

	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		count = gr_nodes_count(route_index);
		for (i = 0; i < count; i++)
		{
			index = gr_node(route_index, i);
			if (is_section(index))
			{
				/*向股道的调车进路不检查股道占用*/
				if ((gn_type(index) == NT_TRACK || (gn_type(index) == NT_STUB_END_SECTION) 
					|| (gn_type(index) == NT_LOCODEPOT_SECTION)) 
					&& gr_type(route_index) == RT_SHUNTING_ROUTE)
				{
					continue;
				}
				/*向无岔区段的调车进路不检查无岔区段占用*/
				else if (gn_type(index) == NT_NON_SWITCH_SECTION 
					&& gr_type(route_index) == RT_SHUNTING_ROUTE 
					&& gr_forward(route_index)==NO_INDEX )
				{
					continue;
				}
				/*其他区段均要检查*/
				else
				{
					if (IsTRUE(is_section_failure(route_index,i)))
					{
						result = CI_TRUE;
						CIHmi_SendNormalTips("区段故障：%s",gn_name(index));
						break;
					}
				}
			}			
		}
	}	
	FUNCTION_OUT;
	return result;
}

/****************************************************
函数名:    close_signal
功能描述:  关闭信号
返回值:    
参数:      node_t signal
作者  :    CY
日期  ：   2012/5/11
****************************************************/
void close_signal(route_t route_index)
{
	node_t signal_node;

	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		signal_node = gr_start_signal(route_index);
		/*单置、差置、并置、尽头线、机务段调车信号机关闭时点蓝灯*/
		if (IsTRUE(is_shunting_signal(signal_node)))
		{
			send_signal_command(signal_node,SGS_A);
		}
		/*列车信号机点红灯*/
		else
		{
			send_signal_command(signal_node,SGS_H);
		}
	}
}

/****************************************************
函数名:    check_signal_allow_filament
功能描述:  检查允许信号断丝
返回值:    
参数:      route_t route_index
作者  :    hejh
日期  ：   2012/8/9
****************************************************/
void check_signal_allow_filament(route_t route_index)
{
	node_t signal_node;
	EN_node_type signal_type;
	int32_t history_show;
	EN_signal_state change_result = SGS_H;

	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		signal_node = gr_start_signal(route_index);
		signal_type = gn_type(signal_node);
		history_show = gn_signal_history_state(signal_node);
		/*判断允许信号断丝*/
		if (gn_signal_state(signal_node) == SGS_FILAMENT_BREAK)
		{
			/*输出提示信息*/
			//CIHmi_SendNormalTips("断丝：%s",gn_name(signal_node));
			/*列车进路*/
			if (gr_type(route_index) != RT_SHUNTING_ROUTE)
			{
				/*不存在自动闭塞，断丝直接显示禁止信号*/
				if (TOTAL_AUTO_BLOCK <= 0)
				{
					change_result = SGS_H;
				}
				/*存在自动闭塞，断丝显示降级信号*/
				else
				{
					/*进站或进路信号机*/
					if ((signal_type == NT_ENTRY_SIGNAL) || (signal_type == NT_ROUTE_SIGNAL))
					{
						/*信号降级显示*/
						if (history_show == SGS_L)
						{
							change_result = SGS_U;
						}
						else
						{
							change_result = SGS_H;
						}
					}
					/*出站信号机*/
					else if ((signal_type == NT_OUT_SHUNTING_SIGNAL) ||(signal_type == NT_OUT_SIGNAL))
					{
						/*自动闭塞*/
						if (gs_special(route_index,SPT_AUTO_BLOCK) != NO_INDEX)
						{
							/*信号降级显示*/
							if (history_show == SGS_L)
							{
								change_result = SGS_U;
							}
							else
							{
								change_result = SGS_H;
							}
						}
						/*三显示自动闭塞*/
						else if (gs_special(route_index,SPT_AUTO_BLOCK3) != NO_INDEX)
						{
							/*信号降级显示*/
							if (history_show == SGS_L)
							{
								change_result = SGS_U;
							}
							else
							{
								change_result = SGS_H;
							}
						}
						/*半自动闭塞*/
						else
						{
							change_result = SGS_H;
						}

					}
				}				
			}
			/*设置历史状态为断丝*/
			sn_signal_history_state(signal_node,SGS_FILAMENT_BREAK);
			process_warning(ERR_FILAMENT_BREAK,gr_name(route_index));
			send_signal_command(gr_start_signal(route_index),change_result);
			/*若信号机实际状态和转换结果一致则进路状态不改变*/
			if (gn_signal_state(signal_node) != change_result)
			{
				/*hjh 2014-2-24 信号转换后是红灯则进路状态设置为正在非正常关闭信号*/
				if (change_result == SGS_H)
				{
					sr_state(route_index,RS_SK_A_SIGNAL_CLOSING);
					sr_clear_error_count(route_index);
				}
				else
				{
					sr_state(route_index,RS_SK_SIGNAL_CHANGING);
					sr_clear_error_count(route_index);
				}				
				sn_signal_expect_state(gr_start_signal(route_index),change_result);
			}
		}
	}
}

/****************************************************
函数名:    is_low_signal
功能描述:  信号是否降级
返回值:    TRUE:ss2比ss1级别低
参数:      EN_signal_state ss1
参数:      EN_signal_state ss2
作者  :    hejh
日期  ：   2012/8/10
****************************************************/
CI_BOOL is_low_signal(EN_signal_state ss1,EN_signal_state ss2)
{
	CI_BOOL result = CI_FALSE;

	/*比绿灯等级低的有：黄灯、绿黄、双黄、红灯*/
	if ((ss1 == SGS_L) && ((ss2 == SGS_U) || (ss2 == SGS_LU) || (ss2 == SGS_UU) || (ss2 == SGS_H) ))
	{
		result = CI_TRUE;
	}
	/*比双绿等级低的有：黄灯、绿黄、双黄、红灯*/
	else if ((ss1 == SGS_LL) && ((ss2 == SGS_U) || (ss2 == SGS_LU) || (ss2 == SGS_UU) || (ss2 == SGS_H) ))
	{
		result = CI_TRUE;
	}
	/*比绿黄等级低的有：黄灯、红灯*/
	else if ((ss1 == SGS_LU) && ((ss2 == SGS_U) || (ss2 == SGS_H) ))
	{
		result = CI_TRUE;
	}
	/*比黄灯等级低的有：红灯*/
	else if ((ss1 == SGS_U) && (ss2 == SGS_H ))
	{
		result = CI_TRUE;
	}
	/*比双黄绿灯等级低的有：红灯*/
	else if ((ss1 == SGS_UU) && (ss2 == SGS_H ))
	{
		result = CI_TRUE;
	}
	return result;
}

/****************************************************
函数名:    close_signal
功能描述:  检查信号是否关闭
返回值:    
参数:      node_t signal
作者  :    CY
日期  ：   2012/5/11
****************************************************/
CI_BOOL is_signal_close(node_t signal_node)
{
	CI_BOOL result = CI_FALSE;

	/*参数检查*/
	if ((signal_node >= 0) && (signal_node < TOTAL_SIGNAL_NODE))
	{
		/*hjh 204150610 信号模块断丝或通信终端均认为是信号已关闭*/
		if ((gn_signal_state(signal_node) == SGS_FILAMENT_BREAK)
			|| (gn_signal_state(signal_node) == SGS_ERROR))
		{
			result = CI_TRUE;
		}
		else
		{
			/*调车信号机禁止信号为蓝灯*/
			if (IsTRUE(is_shunting_signal(signal_node)))
			{
				if (gn_signal_state(signal_node) == SGS_A)
				{
					result = CI_TRUE;
				}
			}
			/*列车信号机禁止信号为红灯*/
			else
			{
				if (gn_signal_state(signal_node) == SGS_H)
				{
					result = CI_TRUE;
				}
			}
		}		
	}
	return result;
}

/****************************************************
函数名:    abnormal_signal_close
功能描述:  关闭错误开放的信号机
返回值:    
作者  :    CY
日期  ：   2012/7/2
****************************************************/
void abnormal_signal_close()
{
	int16_t i;
	route_t route_index;
	node_t next_node = NO_INDEX;

	for (i = 0 ; i < TOTAL_SIGNAL_NODE; i++)
	{
		/*遍历所有的信号机*/
		if (IsTRUE(is_signal(i)) 
			&& ((gn_signal_state(i) != SGS_A) 
			&& (gn_signal_state(i) != SGS_H) 
			&& (gn_signal_state(i) != SGS_YB)
			&& (gn_signal_state(i) != SGS_ERROR) 
			&& (gn_signal_state(i) != SGS_FILAMENT_BREAK)))
		{
			route_index = gn_belong_route(i);
			/*不属于任何进路*/
			if (route_index == NO_INDEX)
			{
				/*调车信号机点蓝灯*/
				if (IsTRUE(is_shunting_signal(i)))
				{
					send_signal_command(i,SGS_A);
				}
				/*列车信号机点红灯*/
				else
				{
					if (gn_signal_state(i) == SGS_YB)
					{
						if (IsFALSE(is_throat_lock(gn_throat(i))))
						{
							send_signal_command(i,SGS_H);
						}
					}
					else
					{
						/*hjh 2012-12-10 单独开放XLD绿黄*/
						/*上行找出左节点*/
						if (gn_direction(i) == DIR_UP)
						{
							next_node = gn_previous(i);
						}
						/*下行找出右节点*/
						else
						{
							next_node = gn_next(i);
						}
						if ((next_node != NO_INDEX) || (gn_type(i) != NT_ROUTE_SIGNAL) || (gn_signal_state(i) != SGS_LU))
						{
							send_signal_command(i,SGS_H);
						}
					}					
				}
			}
			/*该信号机在进路中*/
			else
			{
				/*此信号机不是该进路的始端信号机*/
				if (i != gr_start_signal(route_index))
				{
					/*调车信号机点蓝灯*/
					if (IsTRUE(is_shunting_signal(i)))
					{
						/*不检查非进路调车开放的信号*/
						if (gr_other_flag(route_index) != ROF_HOLD_ROUTE_SHUNTING)
						{
							send_signal_command(i,SGS_A);
						}						
					}
					/*列车信号机点红灯*/
					else
					{
						send_signal_command(i,SGS_H);
					}
				}

				/*hjh 2012-12-27 信号非正常关闭后模拟点其他颜色灯光时立即关闭*/
				if (RS_A_SIGNAL_CLOSED == gr_state(route_index))
				{
					close_signal(route_index);
				}
				/*LYC 2013/8/13 当进路故障时进路始端信号应保持关闭状态*/
				if ((RS_FAILURE == gr_state(route_index)) || (RS_AUTO_UNLOCK_FAILURE == gr_state(route_index)))
				{
					close_signal(route_index);
				}
			}
		}
		
		/*引总锁闭时开放引导信号的执行过程*/
		if (((gn_type(i) == NT_ROUTE_SIGNAL) || (gn_type(i) == NT_ENTRY_SIGNAL)) 
			//&& (gn_belong_route(i) == NO_ROUTE)
			//&& (IsTRUE(is_throat_lock(gn_throat(i)))) 
			//&& ((gn_signal_state(i) == SGS_YB) || (gn_signal_history_state(i) == SGS_YB))
			)
		{
			throat_locked_guide(i);
		}
	}	
}

/****************************************************
函数名:    command_close_signal
功能描述:  关闭信号命令处理
返回值:    
参数:      node_t signal
作者  :    CY
日期  ：   2012/7/24
****************************************************/
void command_close_signal(node_t signal)
{	
	if (((signal >= 0) && signal < (TOTAL_SIGNAL_NODE)) && IsTRUE(is_signal(signal)))
	{
		/*关闭信号*/
		if (IsTRUE(is_shunting_signal(signal)))
		{
			send_signal_command(signal,SGS_A);
		}
		/*列车信号机点红灯*/
		else
		{
			send_signal_command(signal,SGS_H);
		}

		/*设置进路状态*/
		if (gn_belong_route(signal) != NO_INDEX)
		{
			if (IsTRUE(is_guide_route(gn_belong_route(signal))))
			{
				sr_state(gn_belong_route(signal),RS_G_SO_SIGNAL_CLOSING);
			}
			else
			{
				sr_state(gn_belong_route(signal),RS_SK_A_SIGNAL_CLOSING);
			}
			sr_clear_error_count(gn_belong_route(signal));
		}
	}
}

/****************************************************
函数名:    is_relation_condition_ok
功能描述:  判断联系条件是否成立
返回值:    TRUE:成立
			FALSE：不成立
参数:      route_t route_index
作者  :    hejh
日期  ：   2012/9/6
****************************************************/
CI_BOOL is_relation_condition_ok(route_t route_index)
{
	int16_t temp,i;
	node_t check_node;
	CI_BOOL result = CI_TRUE;

	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*宝钢付原料站65#道口*/
		if (dk_bgfylz65_config.section1 != NO_INDEX)
		{
			for (i = 0; i < gr_nodes_count(route_index); i++)
			{
				if (dk_bgfylz65_config.section1 == gr_node(route_index,i))
				{
					if (gn_state(dk_bgfylz65_config.XD) == SIO_HAS_SIGNAL)
					{
						result = CI_FALSE;
					}
					break;
				}
			}
		}
		if ((dk_bgfylz65_config.section2 != NO_INDEX)
			&& (IsTRUE(result)))
		{
			for (i = 0; i < gr_nodes_count(route_index); i++)
			{
				if (dk_bgfylz65_config.section2 == gr_node(route_index,i))
				{
					if (gn_state(dk_bgfylz65_config.XD) == SIO_HAS_SIGNAL)
					{
						result = CI_FALSE;
					}
					break;
				}
			}
		}
		







		/*请求同意点*/
		if ((temp = gs_special(route_index,SPT_REQUEST_AGREE)) != NO_INDEX)
		{
			result = CI_FALSE;
			/*获取请求同意点的状态*/
			check_node = gs_request_agree(temp);
			if (gn_state(check_node) == SIO_HAS_SIGNAL)
			{
				result = CI_TRUE;
			}
			else
			{
				/*输出提示信息*/
				CIHmi_SendNormalTips("未经同意");
			}
		}
		/*半自动闭塞*/
		else if ((temp = gs_special(route_index,SPT_SEMI_AUTO_BLOCK)) != NO_INDEX)
		{
			/*离去区段占用*/
			if (SCS_CLEARED != gn_section_state(gr_forward_section(route_index,gr_node_index_route(route_index,gb_node(gr_end_button(route_index))))))
			{
				CIHmi_SendNormalTips("区段占用");
				process_warning(ERR_DEPARTURE_OCCUPIED,gr_name(route_index));
				result = CI_FALSE;
			}
			/*半自动闭塞条件不成立*/
			if (SAB_RECIEVED_AGREEMENT != gsab_state(temp) && SAB_OUT_SIGNAL_OPENED != gsab_state(temp))
			{
				process_warning(ERR_RELATION_CONDITION,gr_name(route_index));
				/*输出提示信息*/
				CIHmi_SendNormalTips("未办理半自动");
			}
		}
		/*hjh 2013-8-20 场间联系*/
		else if ((temp = gs_special(route_index,SPT_YARDS_LIAISION)) != NO_INDEX)
		{
			result = CI_FALSE;
			/*获取照查点的状态*/
			check_node = gs_yards_liaision(temp);
			if (gn_state(check_node) == SIO_HAS_SIGNAL)
			{
				result = CI_TRUE;
			}
			else
			{
				/*输出提示信息*/
				CIHmi_SendNormalTips("照查不对");
			}
		}
		else
		{
			//result = CI_TRUE;
		}
	}
	return result;
}

/****************************************************
函数名：   keep_signal_middle_switch_ok
功能描述： 检查中间道岔位置正确
返回值：   CI_BOOL
参数：     route_t route_index
作者：	   hejh
日期：     2014/06/11
****************************************************/
CI_BOOL keep_signal_middle_switch_ok(route_t route_index)
{
	int16_t i = 0,si,ms;
	CI_BOOL judge_result = CI_TRUE;

	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
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
					if (gn_section_state(gn_switch_section(ms)) == SCS_CLEARED)
					{
						if ((gn_history_state(ms) != SWS_ERROR) 
							&& (gn_history_state(ms) != (uint32_t)gn_switch_state(ms)))
						{
							CIHmi_SendNormalTips("中间道岔位置错误：%s",gn_name(gs_middle_switch(si,i)));
							judge_result = CI_FALSE;
							break;
						}
					}					
				}
			}
		}
	}

	FUNCTION_OUT;
	return judge_result;
}

/****************************************************
函数名：   hmi_signal_tips
功能描述： 控显机信号机开放颜色
返回值：   char_t*
参数：     EN_signal_state ss
作者：	   hejh
日期：     2015/07/03
****************************************************/
char_t* hmi_signal_tips(EN_signal_state ss)
{
	char_t* result = "";

	switch (ss)
	{
		case SGS_U:
			result = "黄";
			break;
		case SGS_UU:
			result = "双黄";
			break;
		case SGS_LU:
			result = "绿黄";
			break;
		case SGS_L:
			result = "绿";
			break;
		case SGS_LL:
			result = "双绿";
			break;
		case SGS_USU:
			result = "黄闪黄";
			break;
		case SGS_B:
			result = "调白";
			break;
		case SGS_YB:
			result = "引白";
			break;
		case SGS_LS:
			result = "绿闪";
			break;
		case SGS_US:
			result = "黄闪";
			break;
		case SGS_HS:
			result = "红闪";
			break;
		case SGS_BS:
			result = "白闪";
			break;
		default:
			break;
	}
	return result;
}