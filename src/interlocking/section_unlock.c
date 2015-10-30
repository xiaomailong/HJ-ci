/***************************************************************************************
Copyright (C), 2012,  Co.Hengjun, Ltd.
文件名:  section_failure_unlock.c
作者:    hjh
版本 :   1.0	
创建日期:2012/3/27
用途:    区段故障解锁
历史修改记录:  
2012/7/31,V1.1,HJH:
	1.修改了接近区段占用时，无法判断其状态
	2.修改设置和清除锁闭标志函数
2012/8/14,V1.2,HJH:
	1.修改区段故障解锁中连续解锁第二个区段时不能解锁的BUG
	2.修改改方运行时离去区段占用时信号不关闭的问题，列车进路始端信号断丝时的信号降级显示问题，通过进路的接车进路和发车进路在显示上要有联系
	3.修改接近区段的判断BUG
2012/12/13 V1.2.1 hjh
	修改区段故障解锁的逻辑
 2013/8/13 V1.2.1 LYC 
	signal_closed_section_unlock增加了区段前方区段空闲且未曾占用过（或不存在），本区段曾占用过，后方区段空闲且占用过或已解锁时能够延时区段故障解锁
2013/9/18 V1.2.1 LYC mantis1600
	1.check_approach_clear_process修改了通过进路中的接车进路轨道故障时发车进路可以立即故障解锁；
	2.command_section_unlock_relay增加了对正在延时区故解的区段再次进行区故解时需检查前后方区段已解锁且空闲后才能解锁；
	3.check_approach_clear_process增加了检查接近区段曾经是否被占用过
	4.command_section_unlock增加了延续进路不能解锁检查条件
	5.signal_closed_section_unlock增加了多个区段分路不良时的延时故障解锁
2013/9/23 V1.2.1 LYC mantis1599
	1.command_section_unlock增加了检查被解锁对象不是股道的条件
2013/9/23 V1.2.1 LYC mantis1573-3
	1.section_unlock_relay增加了带有中间道岔的发车进路故障解锁也延时30s
2013/9/25 V1.2.1 LYC mantis1561
	1.section_unlock_relay增加了延时过程中检查解锁区段后方区段空闲
2014/2/12 V1.2.1 LYC mantis1996
	unlock_sections重写了该函数，使该函数在解锁区段的同时，解锁该区段两端的其他信号点
2014/2/12 V1.2.1 hjh mantis3302
	signal_closed_section_unlock增加前方区段是股道。无岔区段或尽头线时立即解锁的条件。
2014/2/24 V1.2.1 hjh mantis3348
	command_section_unlock_relay当前区段未占用时亦需要延时解锁
2014/4/10 V1.2.2 LYC mantis3677/3606
	unlock_sections1.如果下个区段是股道、无岔区段或尽头线，则下个区段也解锁的条件增加判断下个区段是本进路的才能解锁
	2.如果股道或无岔区段的前一区段在另一进路中则不解锁，且把股道或无岔区段加到另一进路中
***************************************************************************************/
#include "section_unlock.h"
#include "guide_route.h"
#include "relay_cancel_route.h"


/****************************************************
函数名:    section_unlock
功能描述:  区段故障解锁(进路控制模块调用)
返回值:    
参数:      index

作者  :    hjh
日期  ：   2012/3/28
****************************************************/
void section_unlock( route_t route_index )
{
	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*根据不同的进路类型执行不同的操作*/
		switch (gr_state(route_index))
		{
			case RS_SU_SIGANL_CLOSING:
				/*区故解正在关闭信号*/
				signal_closing(route_index,RS_A_SIGNAL_CLOSED);
				break;
			case RS_G_SO_SIGNAL_CLOSING:
				/*正在关闭引导信号*/
				RCR_G_singal_closing(route_index);
				break;				
			case RS_SU_RELAY:
				/*区故解正在延时*/
				section_unlock_relay(route_index);
				break;
			case RS_CRASH_INTO_SIGNAL:
				/*冒进信号*/
				crash_into_signal_section_unlock(route_index,NO_INDEX);
				break;
			default:
				break;		
		}
	}
	FUNCTION_OUT;
}

/****************************************************
函数名:    command_section_unlock
功能描述:  区段故障解锁（命令处理模块调用）
返回值:    
参数:      index

作者  :    hjh
日期  ：   2012/4/16
****************************************************/
void command_section_unlock( node_t index )
{
	CI_BOOL result = CI_TRUE;
	route_t route_index = gn_belong_route(index);
	char_t tips[TEST_NAME_LENGTH];

	FUNCTION_IN;
	/*参数检查 LYC 2013/9/23 增加了检查被解锁对象不是股道的条件*/
	if (((index >= 0) && (index < (TOTAL_SIGNAL_NODE))) && IsTRUE(is_section(index)))
	{
		/*检查解锁区段不是股道*/
		if (NT_TRACK == gn_type(index))
		{
			CIHmi_SendNormalTips("错误办理：%s",gn_name(index));
			result = CI_FALSE;
		}
		/*检查解锁区段不是股道*/
		if ((NT_NON_SWITCH_SECTION == gn_type(index))
			|| (NT_STUB_END_SECTION == gn_type(index)) || (NT_LOCODEPOT_SECTION == gn_type(index)))
		{
			if (RT_SHUNTING_ROUTE == gr_type(route_index))
			{
				/*只有一个区段*/
				if (gr_sections_count(route_index) > 1)
				{
					CIHmi_SendNormalTips("错误办理：%s",gn_name(index));
					result = CI_FALSE;
				}				
			}			
		}
		/*检查区段空闲*/
		if (SCS_CLEARED != gn_section_state(index))
		{
			process_warning(ERR_SECTION_OCCUPIED, gn_name(index));
			result = CI_FALSE;
			/*输出提示信息*/
			CIHmi_SendNormalTips("区段占用：%s",gn_name(index));
		}
		/*检查区段被进路锁闭*/
		if (IsFALSE(is_node_locked(index,LT_LOCKED)))
		{
			process_warning(ERR_NODE_UNLOCK, gn_name(index));
			result = CI_FALSE;
		}
		/*延续进路可以解锁条件检查*/
		if (IsFALSE(section_unlock_successive(route_index,index))
			&& IsTRUE(is_successive_route(route_index))
			&& (RS_SIGNAL_OPENED != gr_state(route_index)))
		{
			process_warning(ERR_OPERATION, gn_name(index));
			/*输出提示信息*/
			CIHmi_SendNormalTips("错误办理：%s",gn_name(index));
			result = CI_FALSE;
		}
		
		if (IsTRUE(result))
		{
			/*根据不同的进路类型执行不同的操作*/
			switch (gr_state(route_index))
			{
				case RS_ROUTE_LOCKED:
					/*进路锁闭（只有延续进路部分的进路状态是进路锁闭）*/
					section_unlock_approach_process(route_index,index);
					break;
				case RS_SO_SIGNAL_OPENING:
					/*关闭信号，设置进路状态为区段故障解锁正在正常关闭信号*/
					close_signal(route_index);
					sr_state(route_index,RS_SU_SIGANL_CLOSING);
					sr_clear_error_count(route_index);
					break;
				case RS_SK_SIGNAL_CHANGING:
					/*关闭信号，设置进路状态为区段故障解锁正在正常关闭信号*/
					close_signal(route_index);
					sr_state(route_index,RS_SU_SIGANL_CLOSING);
					sr_clear_error_count(route_index);
					break;
				case RS_G_SO_SIGNAL_OPENING:
					/*关闭信号，设置进路状态为区段故障解锁正在正常关闭信号*/
					close_signal(route_index);
					sr_state(route_index,RS_SU_SIGANL_CLOSING);
					sr_clear_error_count(route_index);
					break;
				case RS_SIGNAL_OPENED:
					/*关闭信号，设置进路状态为区段故障解锁正在正常关闭信号*/
					close_signal(route_index);
					sr_state(route_index,RS_SU_SIGANL_CLOSING);
					sr_clear_error_count(route_index);
					break;
				case RS_G_SIGNAL_OPENED:
					/*关闭引导信号，设置进路状态为区段故障解锁正在正常关闭信号*/
					sr_stop_timer(route_index);
					send_signal_command(gr_start_signal(route_index),SGS_H);
					sr_state(route_index,RS_G_SO_SIGNAL_CLOSING);
					sn_signal_expect_state(gr_start_signal(route_index),SGS_H);
					sr_clear_error_count(route_index);
					break;
				/*区故解正在延时*/
				case RS_SU_RELAY:
					/*待解锁区段未在延时解锁*/
					if (IsFALSE(is_node_timer_run(index)))
					{
						section_unlock_approach_process(route_index,index);
						break;
					}
					else
					{
						process_warning(ERR_OPERATION,gn_name(index));
						CIHmi_SendNormalTips("延时未到：%s",gn_name(index));
						break;
					}

					break;
				case RS_SK_N_SIGNAL_CLOSED: 
					/*信号保持正常关闭信号*/
					section_unlock_approach_process(route_index,index);
					break;
				case RS_AUTOMATIC_UNLOCKING:   
				/*正在自动解锁*/
					section_unlock_approach_process(route_index,index);
					break;
				case RS_A_SIGNAL_CLOSED: 
				/*信号非正常关闭*/
					section_unlock_approach_process(route_index,index);
					break;
				case RS_FAILURE:
					/*进路故障*/
					if (is_successive_route(route_index))
					{
						section_unlock_successive(route_index,index);
					}
					else
					{
						section_unlock_approach_process(route_index,index);
					}
					
					break;
				case RS_AUTO_UNLOCK_FAILURE:
					/*进路故障*/
					section_unlock_approach_process(route_index,index);
					break;
				case RS_G_SIGNAL_CLOSED:	
				/*引导信号已关闭*/
					section_unlock_approach_process(route_index,index);
					break;
				case RS_SECTION_UNLOCKING:	
					/*正在进行区段故障解锁*/
					section_unlock_approach_process(route_index,index);
				case RS_TRACK_POWER_OFF:	
					/*轨道停电故障*/
					section_unlock_approach_process(route_index,index);
					break;
				case RS_MS_UNLOCK_RELAY:
					/*中间道岔正在延时解锁*/
					section_unlock_approach_process(route_index,index);
					break;
				case RS_CRASH_INTO_SIGNAL:
					/*冒进信号*/
					crash_into_signal_section_unlock(route_index,index);
					break;
				default:
					process_warning(ERR_OPERATION,gn_name(index));
					CIHmi_SendNormalTips("错误办理：%s",gn_name(second_button));
					break;
			}
		}

		/*检查进路上的区段是否全部解锁*/
		if (IsTRUE(check_all_sections_unlock(route_index)))
		{
			/*进路已经解锁*/
			sr_state(route_index,RS_UNLOCKED);	
			/*输出提示信息*/
			memset(tips,0x00,sizeof(tips));
			strcat_check(tips,"进路解锁：",sizeof(tips));
			OutputHmiNormalTips(tips,route_index);
			delete_route(route_index);
			delete_route(route_index);
		}
	}
	FUNCTION_OUT;
}

/****************************************************
函数名:    section_unlock_approach_process
功能描述:  按列车位置对区段分类处理
返回值:    
参数1:      route_index，待解锁区段所在进路索引号
参数2:      index，待解锁区段的节点索引号
作者  :    hejh
日期  ：   2012/8/8
****************************************************/
void section_unlock_approach_process( route_t route_index ,node_t index )
{
	CI_TIMER time_interval = 0;
	int16_t i,j,ordinal;	
	CI_BOOL result = CI_FALSE,time_run_result = CI_FALSE,result_unlock = CI_FALSE,result_whole = CI_FALSE,result_init = CI_FALSE;
	node_t node_index,fws,bws,last_switch_section,end_signal;
	route_t bwr = gr_backward(route_index); 

	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)) && ((index >= 0) && (index < TOTAL_SIGNAL_NODE)))
	{
		time_interval = gr_delay_times(route_index);
		ordinal = gr_node_index_route(route_index,index);

		/*检查待解锁进路内是否有属于其他进路的区段*/
		for (i = 0; i < gr_nodes_count(route_index); i++)
		{	
			if (IsTRUE(is_section(gr_node(route_index,i))) 
				&& (gn_belong_route(gr_node(route_index,i)) != route_index) 
				&& (gn_belong_route(gr_node(route_index,i)) != NO_INDEX))
			{
				result_whole = CI_TRUE;
				break;
			}	
		}
		/*进路上有属于其他进路的区段*/
		if (IsTRUE(result_whole))
		{
			/*本进路前方有符合三点检查的区段；前方有已解锁的区段；本区段符合三点检查的立即解锁*/
			for (i = ordinal; i < gr_nodes_count(route_index); i++)
			{
				if (IsTRUE(is_section(gr_node(route_index,i)))
					&& (gn_belong_route(gr_node(route_index,i)) == route_index)
					&& (IsFALSE(is_node_locked(gr_node(route_index,i),LT_LOCKED))
					|| (SCSM_SELF_UNLOCK == gn_state_machine(gr_node(route_index,i)))))
				{					
					unlock_sections(route_index,index);
					if (gr_state(route_index) != RS_SECTION_UNLOCKING)
					{
						sr_state(route_index,RS_SECTION_UNLOCKING);
					}
					break;
				}
			}
			
			for (i = ordinal; i >= 0; i--)
			{
				/*进路后方区段全部初始化的立即解锁*/
				if (IsTRUE(is_section(gr_node(route_index,i)))
					&& (gn_belong_route(gr_node(route_index,i)) == route_index)
					&& (SCSM_FAULT != gn_state_machine(gr_node(route_index,i)))
					&& (SCSM_INIT != gn_state_machine(gr_node(route_index,i))))
				{					
					result_init = CI_TRUE;
					break;
				}

			}
			if (IsFALSE(result_init))
			{
				unlock_sections(route_index,index);
				if (gr_state(route_index) != RS_SECTION_UNLOCKING)
				{
					sr_state(route_index,RS_SECTION_UNLOCKING);
				}
			}
			else
			{
				/*其余的延时解锁*/
				if (IsFALSE(is_node_timer_run(index)))
				{
					/*开始计时*/
					sr_fault_section(route_index);
					sn_start_timer(index,time_interval,DTT_UNLOCK);
					sr_state(route_index,RS_SU_RELAY);
				}
			}

		}
		/*进路上没有属于其他进路的区段*/
		if (IsFALSE(result_whole))
		{
			switch(gr_state_machine(route_index))
			{
				/*无车进路上有区段故障时立即解锁*/
				case RSM_FAULT:	
					unlock_sections(route_index,index);
					if (gr_state(route_index) != RS_SECTION_UNLOCKING)
					{
						sr_state(route_index,RS_SECTION_UNLOCKING);
					}
					break;	

				/*初始化*/
				case RSM_INIT:	
					/**/
					if ((bwr != NO_INDEX) && IsTRUE(have_successive_route(bwr)))
					{
						end_signal = gb_node(gr_end_button(bwr));
						last_switch_section = gr_backward_section(bwr,gr_node_index_route(bwr,end_signal));
						/*接车进路接车到股道但进路区段故障时延时解锁*/
						if ((RSM_TRAIN_IN_ROUTE == gr_state_machine(bwr))  && IsFALSE(is_node_locked(last_switch_section,LT_LOCKED)))
						{
							if (IsFALSE(is_node_timer_run(index)))
							{
								/*开始计时*/
								sr_fault_section(route_index);
								sn_start_timer(index,time_interval,DTT_UNLOCK);
								sr_state(route_index,RS_SU_RELAY);							
							}
						}
						else
						{
							process_warning(ERR_TRAIN_IN_ROUTE,gr_name(route_index));
							CIHmi_SendNormalTips("车在进路中");
						}
						break;
					}
					else
					{
						/*普通进路无车时立即解锁*/
						unlock_sections(route_index,index);
						if (gr_state(route_index) != RS_SECTION_UNLOCKING)
						{
							sr_state(route_index,RS_SECTION_UNLOCKING);
						}						
						break;
					}


				/*列车接近*/
				case RSM_TRAIN_APPROACH:

					/*正常、完整的进路延时解锁,引导进路和不完整的进路立即解锁*/
					if (IsTRUE(check_node_route_locked(route_index)) 
						&& IsFALSE(is_guide_route(route_index)))
					{
						if (IsFALSE(is_node_timer_run(index)))
						{
							/*开始计时*/
							sr_fault_section(route_index);
							sn_start_timer(index,time_interval,DTT_UNLOCK);
							sr_state(route_index,RS_SU_RELAY);
						}
					}
					else
					{
						unlock_sections(route_index,index);
						if (gr_state(route_index) != RS_SECTION_UNLOCKING)
						{
							sr_state(route_index,RS_SECTION_UNLOCKING);
						}
					}
					break;

				/*列车压入首区段*/
				case RSM_FIRST_SECTION:
					/*首区段空闲进路上所有区段均可以延时解锁*/
					if (SCS_CLEARED == gn_section_state(gr_first_section(route_index)))
					{
						if (IsFALSE(is_node_timer_run(index)))
						{
							/*开始计时*/
							sr_fault_section(route_index);
							sn_start_timer(index,time_interval,DTT_UNLOCK);
							sr_state(route_index,RS_SU_RELAY);
						}
					}
					else
					{
						process_warning(ERR_TRAIN_IN_ROUTE,gr_name(route_index));
						CIHmi_SendNormalTips("车在进路中");
					}		
					break;

				/*引导进路首区段分路不良延时解锁*/
				case RSM_G_FAULT_FIRST:
					if (IsFALSE(is_node_timer_run(index)))
					{
						/*开始计时*/
						sr_fault_section(route_index);
						sn_start_timer(index,time_interval,DTT_UNLOCK);
						sr_state(route_index,RS_SU_RELAY);
					}
					break;

				/*列车压入第二区段*/
				case RSM_SECOND_SECTION:
					/*首区段可以延时解锁*/
					if (index == gr_first_section(route_index))
					{
						if (IsFALSE(is_node_timer_run(index)))
						{
							/*开始计时*/
							sr_fault_section(route_index);
							sn_start_timer(index,time_interval,DTT_UNLOCK);
							sr_state(route_index,RS_SU_RELAY);
						}
					}
					/*后方紧邻区段解锁的延时解锁*/
					else if (IsFALSE(is_node_locked(gr_backward_section(route_index,ordinal),LT_LOCKED))
						&& (SCS_CLEARED == gn_section_state(gr_backward_section(route_index,ordinal))))
					{
						if (IsFALSE(is_node_timer_run(index)))
						{
							/*开始计时*/
							sr_fault_section(route_index);
							sn_start_timer(index,time_interval,DTT_UNLOCK);
							sr_state(route_index,RS_SU_RELAY);
						}
					}
					/*如果后方紧邻区段已解锁或正在延时解锁则本区段可以延时解锁*/
					/*不能解锁*/
					else
					{
						process_warning(ERR_TRAIN_IN_ROUTE,gr_name(route_index));
						CIHmi_SendNormalTips("车在进路中");
					}
					break;

				/*列车完全进入进路*/
				case RSM_TRAIN_IN_ROUTE:
					/*前方有符合三点检查的区段；前方有已解锁的区段；本区段符合三点检查的立即解锁*/
					for (i = ordinal; i < gr_nodes_count(route_index); i++)
					{
						node_index = gr_node(route_index,i);
						if (IsTRUE(is_section(node_index))
							&& (NT_TRACK != gn_type(node_index))
							&& (IsFALSE(is_node_locked(node_index,LT_LOCKED))
							|| (SCSM_SELF_UNLOCK == gn_state_machine(node_index))))
						{					
							unlock_sections(route_index,index);
							if (gr_state(route_index) != RS_SECTION_UNLOCKING)
							{
								sr_state(route_index,RS_SECTION_UNLOCKING);
							}
							result_unlock = CI_TRUE;
							break;
						}
						fws = gr_forward_section(route_index,i);
						bws = gr_backward_section(route_index,i);
						/*进路上故障的区段满足三点检查时可以立即解锁*/
						if ((fws != NO_INDEX) && (bws != NO_INDEX) && IsTRUE(is_section(node_index)) && (IsTRUE(is_node_locked(node_index,LT_LOCKED)))
							&& ((SCSM_FAULT == gn_state_machine(bws)) || (IsFALSE(is_node_locked(bws,LT_LOCKED))))
							&& (SCSM_FAULT_BEHIND == gn_state_machine(node_index))
							&& (SCS_CLEARED == gn_section_state(node_index)) && (SCS_CLEARED != gn_section_state(fws)))
						{
							unlock_sections(route_index,index);
							if (gr_state(route_index) != RS_SECTION_UNLOCKING)
							{
								sr_state(route_index,RS_SECTION_UNLOCKING);
							}
							result_unlock = CI_TRUE;
							break;
						}
					}

					if (IsFALSE(result_unlock))
					{	
						/*列车压入中岔区段时，前方区段可以延时解锁*/
						if (NO_INDEX != gs_middle_switch_index(gb_node(gr_end_button(route_index))))
						{
							/*待解锁区段是中间道岔，可以延时解锁*/
							//si = gs_middle_switch_index(gb_node(gr_end_button(route_index)));
							//if ((RS_AUTOMATIC_UNLOCKING != gr_state(route_index)) && ((gs_middle_section(si,0) == index)	|| (gs_middle_section(si,1) == index) || (gs_middle_section(si,2) == index)	|| (gs_middle_section(si,3) == index))
							//	&& ((SCS_CLEARED != gn_section_state(gs_middle_section(si,0))) || (SCS_CLEARED != gn_section_state(gs_middle_section(si,1)))
							//	|| (SCS_CLEARED != gn_section_state(gs_middle_section(si,2))) || (SCS_CLEARED != gn_section_state(gs_middle_section(si,3)))))
							//{
							//	if (IsFALSE(is_node_timer_run(index)))
							//	{
							//		/*开始计时*/
							//		sr_fault_section(route_index);
							//		sn_start_timer(index,time_interval,DTT_UNLOCK);
							//		sr_state(route_index,RS_SU_RELAY);
							//	}
							//	break;
							//}
						}
						/*列车到达股道，股道前一区段分路不良时延时解锁*/
						fws = gr_forward_section(route_index,ordinal);
						if ((NO_INDEX != fws) && (NT_TRACK == gn_type(fws)) && (SCS_CLEARED != gn_section_state(fws))
							&& (SCSM_FAULT_CLEARED == gn_state_machine(index)))
						{
							if (IsFALSE(is_node_timer_run(index)))
							{
								/*开始计时*/
								sr_fault_section(route_index);
								sn_start_timer(index,time_interval,DTT_UNLOCK);
								sr_state(route_index,RS_SU_RELAY);
							}
							break;
						}	

						for (j = ordinal - 1; j >= 0; j--)
						{
							node_index = gr_node(route_index,j);
							/*后方任意区段有车占用；后方任意区段被车占用过未在延时的不能解锁*/
							if (IsTRUE(is_section(node_index))
								&& ((SCSM_SELF_OCCUPIED == gn_state_machine(node_index))
								|| (SCSM_FAULT_CLEARED == gn_state_machine(node_index))
								|| (SCS_CLEARED != gn_section_state(node_index)))
								&& (IsFALSE(is_node_timer_run(node_index))))
							{
								process_warning(ERR_TRAIN_IN_ROUTE,gr_name(route_index));							
								result = CI_TRUE;
								break;
							}
							/*后方区段空闲检查*/
							if (IsTRUE(is_section(node_index))
								&& (SCS_CLEARED != gn_section_state(node_index)))
							{
								result = CI_TRUE;
							}
							/*后方区段有延时解锁区段检查*/
							if ((IsTRUE(is_node_timer_run(node_index))))
							{
								time_run_result = CI_TRUE;
							}
						}
						/*后方全部区段空闲且有正在延时解锁的区段，待解锁区段延时解锁*/
						if (IsFALSE(result) && IsTRUE(time_run_result))
						{
							if (IsFALSE(is_node_timer_run(index)))
							{
								/*开始计时*/
								sr_fault_section(route_index);
								sn_start_timer(index,time_interval,DTT_UNLOCK);
								sr_state(route_index,RS_SU_RELAY);
							}
							break;
						}		
				
						/*后方紧邻区段解锁的延时解锁*/
						if (IsFALSE(is_node_locked(gr_backward_section(route_index,ordinal),LT_LOCKED)) 
							&& (SCS_CLEARED == gn_section_state(gr_backward_section(route_index,ordinal))))
						{
							if (IsFALSE(is_node_timer_run(index)))
							{
								/*开始计时*/
								sr_fault_section(route_index);
								sn_start_timer(index,time_interval,DTT_UNLOCK);
								sr_state(route_index,RS_SU_RELAY);
							}
							break;
						}
						/*执行到此处还没有满足的条件的提示车在进路中*/
						process_warning(ERR_TRAIN_IN_ROUTE,gr_name(route_index));	
						CIHmi_SendNormalTips("车在进路中");
						break;
					}
					break;

				/*列车异常接近*/
				case RSM_TRAIN_ABNORMAL_APPROACH:
					unlock_sections(route_index,index);
					if (gr_state(route_index) != RS_SECTION_UNLOCKING)
					{
						sr_state(route_index,RS_SECTION_UNLOCKING);
					}
					break;

				/*疑似列车驶入*/
				case RSM_TRAIN_IN_POSSIBLE:
					if (IsFALSE(is_node_timer_run(index)))
					{
						/*开始计时*/
						sr_fault_section(route_index);
						sn_start_timer(index,time_interval,DTT_UNLOCK);
						if (gr_state(route_index) != RS_SU_RELAY)
						{
							sr_state(route_index,RS_SU_RELAY);
						}
					}
					break;

				/*列车异常进入进路*/
				case RSM_TRAIN_ABNORMAL_IN_ROUTE:
					if (IsFALSE(is_node_timer_run(index)))
					{
						/*开始计时*/
						sr_fault_section(route_index);
						sn_start_timer(index,time_interval,DTT_UNLOCK);
						if (gr_state(route_index) != RS_SU_RELAY)
						{
							sr_state(route_index,RS_SU_RELAY);
						}
					}
					break;
				default:
					process_warning(ERR_ROUTE_STATE,gr_name(route_index));
					CIHmi_SendNormalTips("错误办理：%s",gn_name(index));
					break;
			}
		}
	}
}

/****************************************************
函数名:    section_unlock_relay
功能描述:  正在区段故障解锁延时
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/4/16
****************************************************/
void section_unlock_relay( route_t route_index )
{
	int16_t i,k;
	node_t node_index = NO_INDEX,index = NO_INDEX;
	CI_BOOL result = CI_TRUE,result_unlock = CI_TRUE;
	int16_t last_section = gr_last_section(route_index);
	char_t tips[TEST_NAME_LENGTH];
	
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		for (i = 0;i < gr_nodes_count(route_index); i++)
		{
			index = gr_node(route_index,i);
			/*检查延时前进路上所有空闲的区段未占用*/
			if (IsTRUE(is_section(index)) && (SCS_CLEARED != gn_section_state(index))
				&& IsFALSE(is_section_fault(index)) && (route_index == gn_belong_route(index)))
			{
				if (index == gr_first_section(route_index))
				{
					process_warning(ERR_CRASH_INTO_SIGNAL, gr_name(route_index));
					sr_state(route_index,RS_CRASH_INTO_SIGNAL);
					CIHmi_SendNormalTips("冒进信号");
				}
				/*不检查股道和调车进路最后一个无岔区段*/
				if((index == last_section) && 
					((NT_TRACK == gn_type(last_section)) 
					|| ((NT_NON_SWITCH_SECTION == gn_type(last_section)) && (RT_SHUNTING_ROUTE == gr_type(route_index)))
					|| ((NT_LOCODEPOT_SECTION == gn_type(last_section)) && (RT_SHUNTING_ROUTE == gr_type(route_index)))
					|| ((NT_STUB_END_SECTION == gn_type(last_section)) && (RT_SHUNTING_ROUTE == gr_type(route_index)))))
				{
					continue;
				}
				else
				{
					process_warning(ERR_SECTION_OCCUPIED, gn_name(index));
					CIHmi_SendNormalTips("区段占用：%s",gn_name(index));
					result = CI_FALSE;
					break;
				}

			}
			/*检查延时前进路上占用的区段未空闲*/
			if (IsTRUE(is_section(index)) && (SCS_CLEARED == gn_section_state(index)) 
				&& IsTRUE(is_section_fault(index)) && (route_index == gn_belong_route(index)))
			{
				/*不检查股道和调车进路最后一个无岔区段*/
				if((index == last_section) &&  ((NT_TRACK == gn_type(last_section)) 
					|| ((NT_NON_SWITCH_SECTION == gn_type(last_section)) && (RT_SHUNTING_ROUTE == gr_type(route_index)))
					|| ((NT_LOCODEPOT_SECTION == gn_type(last_section)) && (RT_SHUNTING_ROUTE == gr_type(route_index)))
					|| ((NT_STUB_END_SECTION == gn_type(last_section)) && (RT_SHUNTING_ROUTE == gr_type(route_index)))))
				{
					continue;
				}
				else
				{
					process_warning(ERR_TRAIN_IN_ROUTE, gn_name(index));
					result = CI_FALSE;
					CIHmi_SendNormalTips("车在进路中");
					break;
				}

			}
		}
		for (i = 0;i < gr_nodes_count(route_index); i++)
		{		
			node_index = gr_node(route_index,i);
			/*找出进路中延时解锁的区段*/
			if (IsTRUE(is_section(node_index)) && IsTRUE(is_node_timer_run(node_index)))
			{
				if (IsTRUE(result))
				{
					/*完成3min或30s计时*/
					if (IsTRUE(is_node_complete_timer(node_index)))
					{
						unlock_sections(route_index,node_index);
						sn_stop_timer(node_index);
						/*所有区段均解锁*/
						if (IsTRUE(check_all_sections_unlock(route_index)))
						{
							/*进路已经解锁*/
							sr_state(route_index,RS_UNLOCKED);	
							/*输出提示信息*/
							memset(tips,0x00,sizeof(tips));
							strcat_check(tips,"进路解锁：",sizeof(tips));
							OutputHmiNormalTips(tips,route_index);
							delete_route(route_index);
							break;
						}
						else
						{
							/*检查进路上没有区段延时解锁则设置进路状态为“正在区段故障解锁”*/
							for (k = 0; k < gr_nodes_count(route_index); k++)
							{
								if (IsTRUE(is_section(gr_node(route_index,k))) 
									&& IsTRUE(is_node_locked(gr_node(route_index,k),LT_LOCKED))
									&& IsTRUE(is_node_timer_run(gr_node(route_index,k)))
									)
								{
									result_unlock = CI_FALSE;
									break;
								}
							}
							if (IsTRUE(result_unlock))
							{
								/*进路正在区段故障解锁*/
								if (gr_state(route_index) != RS_SECTION_UNLOCKING)
								{
									sr_state(route_index,RS_SECTION_UNLOCKING);
								}						
							}
							break;
						}
					}
				}
				else
				{
					sn_stop_timer(node_index);
					if ((RS_FAILURE != gr_state(route_index)) && (RS_CRASH_INTO_SIGNAL != gr_state(route_index)))
					{
						sr_state(route_index,RS_FAILURE);	
					}
				}
			}
		}
	}
}

/****************************************************
函数名:    unlock_sections
功能描述:  解锁区段
返回值:    
参数:      route_index
参数:      index
作者  :    hjh
日期  ：   2012/3/28
****************************************************/
void unlock_sections( route_t route_index,node_t index )
{
	/*2014.2.12 LYC重写了该函数，使该函数在解锁区段的同时，解锁该区段两端的其他信号点*/
	int16_t i,j,k;
	int16_t bw_current_node,fw_current_node,section_switch,section;
	int16_t	node_ordinal = gr_node_index_route(route_index,index);
	int16_t node_count = gr_nodes_count(route_index);
	node_t another_signal;

	if (IsTRUE(is_route_exist(route_index)))
	{
		/*解锁当前区段之后的信号点*/
		for (i = node_ordinal + 1; i < node_count; i++)
		{
			bw_current_node = gr_node(route_index,i);
			/*解锁到下个区段停止*/
			if (IsTRUE(is_section(bw_current_node)))
			{
				/*2014/04/10 LYC 如果股道或无岔区段的前一区段在另一进路中则不解锁，且把股道或无岔区段加到另一进路中*/
				if ((gr_type(route_index) == RT_SHUNTING_ROUTE) && (gn_type(bw_current_node) == NT_TRACK))
				{
					/*获取股道另一端的信号机*/
					another_signal = gn_forword(gr_direction(route_index),bw_current_node);
					/*另一端的信号机在某条进路中*/
					if ((another_signal != NO_INDEX) && (gn_belong_route(another_signal) != NO_ROUTE)
						&& (gr_type(gn_belong_route(another_signal)) == RT_SHUNTING_ROUTE)
						&& another_signal != gr_start_signal(gn_belong_route(another_signal)))
					{
						sn_belong_route(bw_current_node,gn_belong_route(another_signal));
						break;
					}
				}
				/*如果下个区段是本进路的股道、无岔区段或尽头线，则下个区段也解锁*/
				else if (((gn_type(bw_current_node) == NT_TRACK)
					|| (gn_type(bw_current_node) == NT_NON_SWITCH_SECTION)
					|| (gn_type(bw_current_node) == NT_STUB_END_SECTION)
					|| (gn_type(bw_current_node) == NT_LOCODEPOT_SECTION))
					&& (gn_belong_route(bw_current_node) == route_index))
				{
					reset_node(bw_current_node);
					break;
				}				
				else
				{
					break;
				}
			}	
			/*解锁除道岔和道岔区段之外的信号点*/
			if (( NT_SWITCH != gn_type(bw_current_node)) && 
				( NT_SWITCH_SECTION != gn_type(bw_current_node))
				&& (gn_belong_route(bw_current_node) == route_index))
			{
				reset_node(bw_current_node);
			}				
		}
		/*解锁当前区段及该区段上的道岔*/
		if (IsTRUE(is_node_locked(index,LT_LOCKED)) && 
			(route_index == gn_belong_route(index)))
		{	
			/*解锁当前道岔区段*/
			if (NT_SWITCH_SECTION == gn_type(index))
			{
				/*多个道岔在一个区段时，如果要解锁区段，则多个道岔都要解锁*/
				for (j = 0; j < MAX_SWITCH_PER_SECTION; j++)
				{
					section_switch = gn_section_switch(index,j);
					if ((NO_INDEX != section_switch) && 
						(route_index == gn_belong_route(section_switch)) //&&
						//IsTRUE(is_node_locked(section_switch,LT_LOCKED))hjh2014-4-18屏蔽，否则防护道岔不能解锁
						)
					{
						reset_node(section_switch);
					}
				}
				/*解锁道岔区段*/
				reset_node(index);
				CIHmi_SendNormalTips("%s解锁！",gn_name(index));
			}
			else
			{
				reset_node(index);
				CIHmi_SendNormalTips("%s解锁！",gn_name(index));
			}
		}	
		/*解锁当前区段之前的信号点*/
		for (k = node_ordinal - 1; k >= 0 ; k--)
		{
			fw_current_node = gr_node(route_index,k);
			/*进路一直从当前节点开始解锁，直到前一个轨道停止解锁*/
			if ( IsTRUE(is_section(fw_current_node)))
			{
				break;
			}
			else
			{
				if (gn_type(fw_current_node) == NT_SWITCH)
				{
					section = gn_switch_section(fw_current_node);
					if (IsTRUE(is_node_locked(section,LT_LOCKED)) && 
						(route_index == gn_belong_route(section)))
					{
						/*hjh 2015-7-22 当道岔区段锁闭时，该道岔区段内的道岔也不解锁*/
					}
					else
					{
						/*删除前方节点时需检查是本进路上的节点*/
						if (gn_belong_route(fw_current_node) == route_index)
						{
							reset_node(fw_current_node);
						}
					}
				}
				else
				{
					/*删除前方节点时需检查是本进路上的节点*/
					if (gn_belong_route(fw_current_node) == route_index)
					{
						reset_node(fw_current_node);
					}
				}							
			}		
		}
	}
}

/****************************************************
函数名:    check_all_sections_unlock
功能描述:  检查所有区段均解锁
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/4/16
****************************************************/
CI_BOOL check_all_sections_unlock( route_t route_index )
{
	CI_BOOL result = CI_TRUE;
	int16_t i,count,index;
	
	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*检查所有区段已解锁*/
		count = gr_nodes_count(route_index);
		for (i = 0; i<count; i++)
		{
			index = gr_node(route_index,i);
			/*若有一个区段未解锁则返货FALSE*/
			if (IsTRUE(is_section(index)) 
				&& IsTRUE(is_node_locked(index,LT_LOCKED))
				&& (route_index == gn_belong_route(index)))
			{
				result = CI_FALSE;
				break;
			}
		}
	}
	FUNCTION_OUT;
	return result;
}

/****************************************************
函数名：   section_unlock_successive
功能描述： 判断延续进路是否可以进行故障解锁
返回值：   CI_TURE：可以解锁
			CI_FALSE：不能解锁
参数：     route_t route_index
参数：     node_t index
作者：	   hejh
日期：     2014/03/07
****************************************************/
CI_BOOL section_unlock_successive( route_t route_index,node_t index )
{
	int16_t temp,i,bwr_node = NO_INDEX,end_signal,si;
	route_t bwr;
	CI_BOOL result = CI_FALSE;

	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)) && ((index >= 0) && (index < TOTAL_SIGNAL_NODE)))
	{
		/*检查是否存在特殊联锁，表示灯*/
		if ((temp = gs_special(route_index,SPT_INDICATION_LAMP)) != NO_INDEX)
		{
			/*获取表示灯的状态*/
			if (gn_state(gs_indication_lamp(temp)) == SIO_HAS_SIGNAL)
			{
				/*设置表示灯的状态*/
				sn_state(gs_indication_lamp(temp),SIO_NO_SIGNAL);
				sr_state(route_index,RS_SU_SIGANL_CLOSING);
				sr_clear_error_count(route_index);
			}
		}
		else
		{			
			/*接车进路不存在的可以故障解锁*/
			bwr = gr_backward(route_index);
			if (bwr == NO_INDEX)
			{
				result = CI_TRUE;
			}
			
			if (IsTRUE(is_successive_route(route_index)) && (bwr != NO_INDEX))
			{			
				/*接车进路是引导进路的可以故障解锁*/
				if (IsTRUE(is_guide_route(bwr)))
				{
					result = CI_TRUE;
				}
				end_signal = gb_node(gr_end_button(bwr));
				/*查找接车进路终端信号机外方的最后一个道岔区段*/
				for (i = gr_node_index_route(bwr,end_signal) - 1; i >= 0; i--)
				{					
					bwr_node = gr_node(bwr,i);
					if (IsTRUE(is_section(bwr_node)) && (NT_SWITCH_SECTION == gn_type(bwr_node)))
					{
						break;
					}					
				}
				/*接车进路的最后一个道岔区段解锁时延续进路可以故障解锁*/
				if ((bwr_node != NO_INDEX) && IsFALSE(is_node_locked(bwr_node,LT_LOCKED)))
				{
					result = CI_TRUE;
				}
				/*如果存在中间道岔则中间道岔任意区段解锁延续进路也可以解锁*/
				if (NO_INDEX != gs_middle_switch_index(gr_start_signal(route_index)))
				{
					si = gs_middle_switch_index(gr_start_signal(route_index));	
					/*中间道岔任意区段解锁的延续进路也可以解锁*/
					if ((IsFALSE(is_node_locked((gs_middle_section(si,0)),LT_LOCKED)))
						|| (IsFALSE(is_node_locked((gs_middle_section(si,1)),LT_LOCKED)))
						|| (IsFALSE(is_node_locked((gs_middle_section(si,2)),LT_LOCKED)))
						|| (IsFALSE(is_node_locked((gs_middle_section(si,3)),LT_LOCKED))))						
					{
						result = CI_TRUE;
					}
				}
			}
		}
				
	}	
	FUNCTION_OUT;
	return result;
}

/****************************************************
函数名：   crash_into_signal_section_unlock
功能描述： 进路状态为冒进信号时故障解锁处理
返回值：  无
参数：     route_index 进路索引号
参数：     node_t index 待解锁区段索引号
作者：	   LYC
日期：     2014/05/29
****************************************************/
void crash_into_signal_section_unlock(route_t route_index,node_t index)
{
	int16_t i,node_index;
	CI_BOOL result  = CI_FALSE,time_result = CI_FALSE;
	CI_TIMER time_interval = gr_delay_times(route_index);
	int16_t last_section = gr_last_section(route_index);

	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)) && ((index >= 0) && (index < TOTAL_SIGNAL_NODE)) && (gr_state(route_index) == RS_CRASH_INTO_SIGNAL))
	{
		/*检查进路上全部区段空闲*/
		for (i = 0;i < gr_nodes_count(route_index); i++)
		{			
			/*检查进路上所有空闲的区段未占用*/
			if (IsTRUE(is_section(gr_node(route_index,i))) && (SCS_CLEARED != gn_section_state(gr_node(route_index,i))) && (route_index == gn_belong_route(index)))
			{
				/*最后区段是股道或调车进路的无岔区段不检查*/
				if ((NT_TRACK == gn_type(last_section)) 
					|| ((NT_NON_SWITCH_SECTION == gn_type(last_section)) && (RT_SHUNTING_ROUTE == gr_type(route_index)))
					|| ((NT_LOCODEPOT_SECTION == gn_type(last_section)) && (RT_SHUNTING_ROUTE == gr_type(route_index)))
					|| ((NT_STUB_END_SECTION == gn_type(last_section)) && (RT_SHUNTING_ROUTE == gr_type(route_index))))
				{
					continue;
				}
				else
				{
					result = CI_TRUE;
					break;
				}			
			}
		}
		/*进路上全部区段空闲的开始延时解锁*/
		if (IsFALSE(result) && (IsFALSE(is_node_timer_run(index))))
		{
			/*开始计时*/			
			sn_start_timer(index,time_interval,DTT_UNLOCK);
		}
		else
		{
			process_warning(ERR_TRAIN_IN_ROUTE,gr_name(route_index));
			CIHmi_SendNormalTips("车在进路中");
		}
	}

	if (IsTRUE(is_route_exist(route_index)) && (NO_INDEX == index) && (gr_state(route_index) == RS_CRASH_INTO_SIGNAL))
	{
		/*检查进路上全部区段空闲*/
		for (i = 0;i < gr_nodes_count(route_index); i++)
		{			
			/*检查进路上所有空闲的区段未占用*/
			if (IsTRUE(is_section(gr_node(route_index,i))) && (SCS_CLEARED != gn_section_state(gr_node(route_index,i))) 
				&& ((NO_INDEX == index) || ((NO_INDEX != index) && (route_index == gn_belong_route(index)))))
			{
				/*最后区段是股道或调车进路的无岔区段不检查*/
				if (((gr_node(route_index,i)) == last_section) && 
					((NT_TRACK == gn_type(last_section)) || ((NT_NON_SWITCH_SECTION == gn_type(last_section)) && (RT_SHUNTING_ROUTE == gr_type(route_index)))
					|| ((NT_LOCODEPOT_SECTION == gn_type(last_section)) && (RT_SHUNTING_ROUTE == gr_type(route_index)))
					|| ((NT_STUB_END_SECTION == gn_type(last_section)) && (RT_SHUNTING_ROUTE == gr_type(route_index)))))
				{
					continue;
				}
				else
				{
					time_result = CI_TRUE;
					break;
				}			
			}
		}	
		/*找出进路中延时解锁的区段*/
		for (i = 0;i < gr_nodes_count(route_index); i++)
		{
			node_index = gr_node(route_index,i);
			if (IsTRUE(is_section(node_index)) && IsTRUE(is_node_timer_run(node_index)))
			{
				/*进路内有区段占用则停止延时*/
				if (IsTRUE(time_result))
				{
					sn_stop_timer(node_index);
					process_warning(ERR_SECTION_OCCUPIED,gn_name(node_index));
					/*输出提示信息*/
					CIHmi_SendNormalTips("区段占用：%s",gn_name(node_index));
				} 
				else
				{
					/*完成3min或30s计时*/
					if (IsTRUE(is_node_complete_timer(node_index)))
					{
						unlock_sections(route_index,node_index);
						sn_stop_timer(node_index);
						/*所有区段均解锁*/
						if (IsTRUE(check_all_sections_unlock(route_index)))
						{
							/*进路已经解锁*/
							sr_state(route_index,RS_UNLOCKED);	
							delete_route(route_index);
							break;
						}
					}
				}
			}
		}
	}
}