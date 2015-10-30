/***************************************************************************************
Copyright (C), 2012,  Co.Hengjun, Ltd.
文件名:  guide_route.c
作者:    hjh
版本 :   1.0	
创建日期:2012/3/20
用途:    引导进路
历史修改记录:         
2012/6/21,V1.1，HJH:
	1.修改引导进路的判断方法，由gr_type修改为gr_other_flag，并删除RT_GUIDE_ROUTE和RT_SUCCESSIVE_ROUTE。
	2.修改函数，发现原来有is_guide_route（），故删除gr_other_flag（）
	3.修改了检查红灯灯丝断丝条件判断错误的BUG
	4.修改了提示信息。根据实际情况将错误信息，警告信息，提示信息正确分类处理；修改了部分Bug
2012/7/31,V1.2，HJH:
	1.重新实现了引导进路搜索算法 
	2.更新信号节点数据和联锁代码
	3.修改设置和清除锁闭标志函数
2013/4/2 V1.2.1 hjh
	search_guide_route搜索节点结束条件增加进路信号机
2013/9/3 V1.2.1 LYC 
	1.command_guide_route修改了未搜索到进路时提示信息错误的BUG。
	2.command_guide_route搜索到进路后该进路联锁条件检查条件不满足则删除该进路
	3.search_guide_route增加了当前节点是驼峰信号机时搜索结束
2013/9/4 V1.2.1 LYC 
	1.throat_locked_process增加了因红灯断丝无法开放引导信号时的提示信息
	2.标准站为信号机内方第一区段占用，延时15s过程中区段空闲，延时到之后关闭引导信号。
	故先按标准站修改guide_signal_opened_process和throat_locked_guide函数相关代码。
2014/2/27 V1.2.1 hjh mantis：3418 
	command_guide_route搜索到的节点与进路匹配不上则认为搜索失败
2014/4/21 V1.2.2 hjh
	根据需求完成引导进路的建立功能。
2014/5/8 V1.2.3 hjh
	部分进路状态不能办理引导进路。
***************************************************************************************/
#include "guide_route.h"
#include "init_manage.h"
#define MAX_GUIDE_ROUTE_NODES  100
#define SWITCH_LOCATION_MASK 0xFF00             /*道岔表示*/
/****************************************************
函数名:    guide_route
功能描述:  引导进路（进路控制模块）
返回值:    

作者  :    hjh
日期  ：   2012/3/20
****************************************************/
void guide_route( route_t route_index )
{
	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)) && IsTRUE(is_guide_route(route_index)))
	{
		/*检查存在以此信号机为始端的进路*/
		switch (gr_state(route_index))
		{
			case RS_G_SO_SIGNAL_OPENING:
				/*引导信号正在开放*/
				guide_signal_opening_process(route_index);
				break;
			case RS_G_SIGNAL_OPENED:
				/*引导信号已开放*/
				guide_signal_opened_process(route_index);
				break;
			case RS_G_SO_SIGNAL_CLOSING:
				/*引导信号正在关闭*/
				guide_signal_closing_process(route_index);
				break;
			case RS_G_ROUTE_LOCKED:
				/*引导信号由于断丝未曾开放过则可以开放*/
				if (IsTRUE(check_guide_route_condition(route_index)))
				{					
					if (SGS_H == gn_signal_state(gr_start_signal(route_index)))
					{
						open_guide_signal(route_index);
					}
				}

				break;
			default:
				break;
		}
	}
	FUNCTION_OUT;
}

/****************************************************
函数名:    command_guide_route
功能描述:  引导进路（命令处理模块调用）
返回值:  
参数:      index

作者  :    LYC
日期  ：   2012/4/25
****************************************************/
void command_guide_route( node_t index )
{
	int16_t ILT_route_index,nodes_num = 0,node_index;
	int8_t i,j,k;
	int16_t guide_nodes[MAX_GUIDE_ROUTE_NODES];
	CI_BOOL result = CI_TRUE;
	route_t successive_route = NO_INDEX,route_index,current_route = NO_INDEX;

	FUNCTION_IN;
	/*参数检查*/
	if ((index >= 0) || (index <= TOTAL_SIGNAL_NODE))
	{
		/*信号机所在咽喉办理了引总锁闭*/
		if (IsTRUE(result) && IsTRUE(is_throat_lock(gn_throat(index))))
		{
			/*检查条件开放引导信号*/
			if (gn_signal_state(index) == SGS_FILAMENT_BREAK)
			{
				//CIHmi_SendNormalTips("断丝：%s",gn_name(index));
			}
			else if (gn_signal_state(index) == SGS_ERROR)
			{
				CIHmi_SendNormalTips("通信中断：%s",gn_name(index));
			}
			else if (gn_signal_state(index) == SGS_H) //&& (gn_signal_history_state(index) != SGS_YB))
			{
				send_signal_command(index,SGS_YB);
				sn_signal_expect_state(index,SGS_YB);
				CIHmi_SendDebugTips("%s 正在开放引导信号！",gn_name(index));
				sn_signal_history_state(index,SGS_H);
			}
			else if (gn_signal_state(index) == SGS_YB)
			{
				if (IsTRUE(is_node_timer_run(index)))
				{
					/*检查进站信号机内方第一个区段*/
					if (SCS_CLEARED != gn_section_state(get_first_section(index)))
					{
						/*重新开始计时*/
						sn_start_timer(index,SECONDS_15,DTT_CLOSE_GUIDE);
					}
					else
					{
						CIHmi_SendNormalTips("错误办理：%s",gn_name(index));
					}
				}
				else
				{
					CIHmi_SendNormalTips("错误办理：%s",gn_name(index));
				}
			}
			else
			{
				CIHmi_SendNormalTips("错误办理：%s",gn_name(index));
			}
			result = CI_FALSE;
		}

		/*搜索引导进路*/
		if (IsTRUE(result))
		{
			guide_nodes[0] = index;
			/*函数search_guide_route（）的返回值大于0，则证明搜索成功*/
			nodes_num = search_guide_route(guide_nodes,0);
			if (nodes_num > 0)
			{
				/*匹配联锁表中的进路号*/
				ILT_route_index = compare_nodes(guide_nodes,nodes_num);
				/*搜索到引导进路*/
				if (ILT_route_index != NO_INDEX)
				{
					/*设置进路，并匹配进表中的进路号*/
					current_route = set_guide_route(ILT_route_index);
					if (IsTRUE(check_guide_route_condition(current_route)))
					{
						/*轮询进路上所有节点是否被锁闭*/
						for (i = 0; i < gr_nodes_count(current_route); i++)
						{
							node_index = gr_node(current_route,i);
							if (IsTRUE(is_node_locked(node_index,LT_LOCKED)))
							{
								route_index = gn_belong_route(node_index);
								if ((route_index != NO_INDEX) && IsTRUE(is_route_exist(route_index)))
								{
									/*引导进路*/
									if (IsTRUE(is_guide_route(route_index)))
									{
										/*引导进路存在执行流程*/
										guide_route_exist(route_index);
										result = CI_FALSE;
										break;
									}
									else
									{
										/*检查可否建立引导进路*/
										result = is_guide_route_build(current_route,route_index);									
										if (IsFALSE(result))
										{
											break;
										}
									}
								}
							}
						}
					}
					else
					{
						result = CI_FALSE;
					}
					
					/*遍历结束，设置节点所属进路并删除重叠进路*/
					if (IsTRUE(result) && (i == gr_nodes_count(current_route)))
					{
						for (j = 0; j < gr_nodes_count(current_route); j++)
						{
							node_index = gr_node(current_route,j);
							if (IsTRUE(is_node_locked(node_index,LT_LOCKED)))
							{
								route_index = gn_belong_route(node_index);
								/*设置节点所属进路*/
								sn_belong_route(node_index,current_route);

								if ((route_index != NO_INDEX) && IsTRUE(is_route_exist(route_index)))
								{
									for (k = 0; k < gr_nodes_count(route_index); k++)
									{
										if (route_index == gn_belong_route(gr_node(route_index,k)))
										{
											break;
										}											
									}
									if (k == gr_nodes_count(route_index))
									{
										/*延续进路时将其前后关系转移至引导进路*/
										if (IsTRUE(have_successive_route(route_index)))
										{
											successive_route = gr_forward(route_index);												
										}
										/*删除进路*/
										delete_route(route_index);

										if (successive_route != NO_INDEX)
										{
											sr_forward(current_route,successive_route);
											sr_backward(successive_route,current_route);
										}										
									}									
								}
								else
								{
									/*设置节点所属进路*/
									sn_belong_route(node_index,current_route);
								}
							}
							else
							{
								/*设置节点所属进路*/
								sn_belong_route(node_index,current_route);
							}
						}
					}

					/*检查联锁条件，开放引导信号*/
					if (IsTRUE(result) && IsTRUE(check_guide_route_condition(current_route))
						&& IsTRUE(lock_guide_route(current_route)))						
					{
						/*记录故障区段*/
						sr_fault_section(current_route);
						open_guide_signal(current_route);
					}
					else
					{
						/*删除进路*/
						delete_route(current_route);
					}										
				}
				/*hjh 2014-2-27 搜索到的节点与进路匹配不上则认为搜索失败*/
				else
				{
					//PRINTF1("未搜索到以%s为始端的引导进路！",gn_name(index));
				}
			}		
			/*2012/9/3 LYC 未搜索到引导进路*/
			else
			{
				//PRINTF1("未搜索到以%s为始端的引导进路！",gn_name(index));
			}
		}
	}
	FUNCTION_OUT;
}

/****************************************************
函数名：   guide_route_exist
功能描述： 引导进路存在执行流程
返回值：   void
参数：     route_t route_index
作者：	   hejh
日期：     2014/04/18
****************************************************/
void guide_route_exist( route_t route_index )
{
	node_t fault_node = NO_INDEX,last_section,last_switch_section,second_last_switch_section;
	node_t middle_last_section,middle_last_switch_section,middle_second_last_switch_section;
	int16_t i,node;
	CI_BOOL result = CI_TRUE;

	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		if (RS_G_SIGNAL_OPENED == gr_state(route_index))
		{
			/*检查引导进路是否计时，重新开始计时*/
			if (IsTRUE(is_timer_run(route_index)))
			{
				sr_start_timer(route_index,SECONDS_15,DTT_CLOSE_GUIDE);
			}
			else
			{
				/*错误操作*/
				process_warning(ERR_OPERATION,gr_name(route_index));
				CIHmi_SendNormalTips("错误办理");
			}
		}
		else
		{
			last_section = gr_last_section(route_index);		
			last_switch_section = gr_backward_section(route_index,gr_node_index_route(route_index,last_section));
			second_last_switch_section = gr_backward_section(route_index,gr_node_index_route(route_index,last_switch_section));
			middle_last_section = gr_forward_section(route_index,gr_node_index_route(route_index,gb_node(gr_end_button(route_index))));
			middle_last_switch_section = gr_backward_section(route_index,gr_node_index_route(route_index,middle_last_section));
			middle_second_last_switch_section = gr_backward_section(route_index,gr_node_index_route(route_index,middle_last_switch_section));

			/*列车未进入进路且联锁条件正确*/
			if (IsTRUE(check_guide_route_condition(route_index)))
			{
				if ((RSM_INIT == gr_state_machine(route_index)) 
					|| (RSM_TRAIN_APPROACH == gr_state_machine(route_index)) 
					|| (RSM_FIRST_SECTION == gr_state_machine(route_index)))
				{
					sr_fault_section(route_index);
					open_guide_signal(route_index);
				}
				/*车列压入第二区段*/
				else if (RSM_SECOND_SECTION == gr_state_machine(route_index))
				{
					if (SCSM_SELF_UNLOCK == gn_state_machine(last_switch_section))
					{
						sr_fault_section(route_index);
						open_guide_signal(route_index);
					}
					if (SCSM_SELF_UNLOCK == gn_state_machine(middle_last_switch_section))
					{
						sr_fault_section(route_index);
						open_guide_signal(route_index);
					}
				}
				else
				{
					/*立即解锁：1.股道故障，次最后区段和最后区段符合三点检查
								2.无故障区段，最后区段符合三点检查*/
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
							sr_fault_section(route_index);
							open_guide_signal(route_index);
						}
						else if ((SCSM_SELF_UNLOCK == gn_state_machine(middle_last_switch_section)) && (SCS_CLEARED == gn_section_state(middle_last_switch_section)))
						{
							sr_fault_section(route_index);
							open_guide_signal(route_index);
						}
					}			
					else
					{
						/*股道故障*/
						if (fault_node == last_section)
						{
							/*股道故障，次最后区段和最后区段符合三点检查*/
							if ((SCSM_SELF_UNLOCK == gn_state_machine(last_switch_section)) 
								&& (SCSM_SELF_UNLOCK == gn_state_machine(second_last_switch_section)))
							{
								sr_fault_section(route_index);
								open_guide_signal(route_index);
							}
							else if ((SCSM_SELF_UNLOCK == gn_state_machine(middle_last_switch_section)) 
								&& (SCSM_SELF_UNLOCK == gn_state_machine(middle_second_last_switch_section)))
							{
								sr_fault_section(route_index);
								open_guide_signal(route_index);
							}
						}
						else
						{
							if ((fault_node != last_switch_section) && (fault_node != second_last_switch_section))
							{
								/*其他区段故障，非故障区段出清，最后区段和股道顺序占用后最后区段出清*/
								if (((((SCSM_BEHIND_CLEARED_2 == gn_state_machine(last_section)) || (SCSM_SELF_UNLOCK == gn_state_machine(last_section))) && (SCSM_FAULT == gn_state_machine(last_switch_section)))
									|| (((SCSM_BEHIND_CLEARED_2 == gn_state_machine(last_section)) || (SCSM_SELF_UNLOCK == gn_state_machine(last_section))) && (SCSM_SELF_UNLOCK == gn_state_machine(last_switch_section))))
									&& (SCS_CLEARED == gn_section_state(last_switch_section)))
								{
									result = CI_TRUE;
									for (i = 0; i < gr_nodes_count(route_index); i++)
									{
										node = gr_node(route_index,i);
										/*非故障区段出清*/
										if (IsTRUE(is_section(node)) && (node != last_section)
											&& (SCS_CLEARED != gn_section_state(node)) && IsFALSE(is_section_fault(node)))
										{
											result = CI_FALSE;
											break;
										}
										/*有区段未被占用过*/
										if (IsTRUE(is_section(node)) && (node != last_section) && IsFALSE(is_section_fault(node))
											&& ((SCSM_BEHIND_OCCUPIED == gn_state_machine(node)) || (SCSM_FAULT_BEHIND == gn_state_machine(node))))
										{
											result = CI_FALSE;
											break;
										}
									}
									if (IsTRUE(result))
									{
										sr_fault_section(route_index);
										open_guide_signal(route_index);
									}
								}
							}
						}
					}
				}

				if (gr_state(route_index) != RS_G_SO_SIGNAL_OPENING)
				{
					/*列车在运行*/
					process_warning(ERR_TRAIN_IN_ROUTE,gr_name(route_index));
				}
			}
		}
	}
}

/****************************************************
函数名：   is_guide_route_build
功能描述： 检查是否可以建立引导进路
返回值：   CI_BOOL
参数：     int16_t guide_route_index
参数：     int16_t route_index
作者：	   hejh
日期：     2014/04/18
****************************************************/
CI_BOOL is_guide_route_build( route_t guide_route_index,route_t route_index )
{
	CI_BOOL result = CI_TRUE;
	node_t last_section,last_switch_section,second_last_switch_section,node_index;
	int8_t i,oppcuied_section_count = 0,failure_section_count = 0;

	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*检查敌对进路未建立*/
		if (IsTRUE(check_conflict_signal(route_index)))
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

		/*检查始端信号未关闭*/
		if (IsTRUE(result) && (SGS_H != gn_signal_state(gr_start_signal(route_index))) && (SGS_A != gn_signal_state(gr_start_signal(route_index)))
			 && (SGS_FILAMENT_BREAK != gn_signal_state(gr_start_signal(route_index))) && (SGS_ERROR != gn_signal_state(gr_start_signal(route_index))))
		{
			process_warning(ERR_SIGNAL_OPEN, gr_name(route_index));	
			CIHmi_SendNormalTips("信号未关闭");
			result = CI_FALSE;
		}

		/*检查进路状态正确，以下进路状态均不能建立引导进路*/
		if (IsTRUE(result) && ((gr_state(route_index) == RS_SIGNAL_OPENED)
			|| (gr_state(route_index) == RS_RCR_SUCCESSIVE_RELAY)
			|| (gr_state(route_index) == RS_SU_RELAY)
			|| (gr_state(route_index) == RS_MS_UNLOCK_RELAY)))
		{
			process_warning(ERR_OPERATION,gr_name(route_index));
			CIHmi_SendNormalTips("错误办理");
			result = CI_FALSE;
		}
		
		if (IsTRUE(result))
		{
			last_section = gr_last_section(route_index);
			/*调车进路或发车进路*/
			if ((RT_SHUNTING_ROUTE == gr_type(route_index)) 
				|| ((NT_ENTRY_SIGNAL != gn_type(gr_start_signal(route_index)))
				&& (NT_ROUTE_SIGNAL != gn_type(gr_start_signal(route_index)))))
			{
				/*敌对进路*/
				if (IsTRUE(is_node_locked(last_section,LT_LOCKED)))
				{
					for (i = 0; i < gr_nodes_count(guide_route_index); i++)
					{
						node_index = gr_node(guide_route_index,i);
						if (IsTRUE(is_section(node_index)) && (last_section == node_index))
						{
							process_warning(ERR_CONFLICT_ROUTE,gr_name(route_index));
							CIHmi_SendNormalTips("敌对进路");
							result = CI_FALSE;
							break;
						}
					}				
				}
			}
			else
			{
				/*车列未进入进路*/
				if ((RSM_INIT == gr_state_machine(route_index)) 
					|| (RSM_TRAIN_APPROACH == gr_state_machine(route_index)) 
					|| (RSM_FIRST_SECTION == gr_state_machine(route_index)))
				{				
					/*接车进路转引导且列车接近*/
					if (RSM_TRAIN_APPROACH == gr_state_machine(route_index))
					{
						sr_other_flag(guide_route_index,ROF_GUIDE_APPROACH);
					}
					/*接车进路列车压入第一区段后出清转引导*/
					if ((RSM_FIRST_SECTION == gr_state_machine(route_index)) && (RS_AUTO_UNLOCK_FAILURE == gr_state(route_index)))
					{
						sr_other_flag(guide_route_index,ROF_GUIDE_APPROACH);
					}
					/*可以建立引导进路*/
					result = CI_TRUE;				
				}
				else
				{
					/*车列到达股道*/
					if ((RSM_SECOND_SECTION == gr_state_machine(route_index)) 
						|| (RSM_TRAIN_IN_ROUTE == gr_state_machine(route_index)))
					{
						last_switch_section = gr_backward_section(route_index,gr_node_index_route(route_index,last_section));
						second_last_switch_section = gr_backward_section(route_index,gr_node_index_route(route_index,last_switch_section));
												
						for (i = 0; i < gr_nodes_count(route_index); i++)
						{
							node_index = gr_node(route_index,i);
							/*统计占用过且未出清区段个数*/
							if (IsTRUE(is_section(node_index)) && IsTRUE(is_node_locked(node_index,LT_LOCKED))
								&& ((SCSM_BEHIND_CLEARED_1 == gn_state_machine(node_index)) || (SCSM_FRONT_OCCUPIED_2 == gn_state_machine(node_index)))
								&& (NT_TRACK != gn_type(node_index)) && (SCS_CLEARED != gn_section_state(node_index)))
							{
								oppcuied_section_count++;
							}
							/*统计未占用过区段个数*/
							if (IsTRUE(is_section(node_index)) && IsTRUE(is_node_locked(node_index,LT_LOCKED))
								&& (SCSM_FAULT == gn_state_machine(node_index)))
							{
								failure_section_count++;
							}
						}

						/*1.最后区段占用，股道占用后出清*/
						if ((SCS_CLEARED != gn_section_state(last_switch_section)) 
							&& (SCSM_TRACK_UNLOCK == gn_state_machine(last_section)) && (SCS_CLEARED == gn_section_state(last_section)))
						{
							/*可以建立引导进路*/
							result = CI_TRUE;
						}
						/*2.最后区段和股道依次占用后出清*/
						else if ((SCSM_SELF_UNLOCK == gn_state_machine(last_section)) && (SCS_CLEARED == gn_section_state(last_section))
							&& (SCS_CLEARED == gn_section_state(last_switch_section)))
						{
							/*可以建立引导进路*/
							result = CI_TRUE;
						}
						/*3.最后区段符合三点检查，股道占用后出清（有区段未占用过）*/
						else if ((SCSM_SELF_UNLOCK == gn_state_machine(last_switch_section))  && (SCS_CLEARED == gn_section_state(last_switch_section))
							&& (SCSM_SELF_UNLOCK == gn_state_machine(last_section)) && (SCS_CLEARED == gn_section_state(last_section))
							&& (failure_section_count >= 1))
						{
							/*可以建立引导进路*/
							result = CI_TRUE;
						}
						/*4.最后两个道岔区段符合三点检查（有区段未占用过，非第一区段）*/
						else if ((SCSM_SELF_UNLOCK == gn_state_machine(second_last_switch_section)) && (SCS_CLEARED == gn_section_state(second_last_switch_section))
							&& (SCSM_SELF_UNLOCK == gn_state_machine(last_switch_section))&& (SCS_CLEARED == gn_section_state(last_switch_section))
							&& (failure_section_count >= 1) && (SCSM_FAULT != gn_state_machine(gr_first_section(route_index))))
						{
							/*可以建立引导进路*/
							result = CI_TRUE;
						}
						/*5.最后道岔区段符合三点检查（只有一个区段未出清，非第一区段）*/
						else if ((SCSM_SELF_UNLOCK == gn_state_machine(last_switch_section)) && (SCS_CLEARED == gn_section_state(last_switch_section))
							&& (oppcuied_section_count == 1)&& (SCS_CLEARED == gn_section_state(gr_first_section(route_index))))
						{
							/*可以建立引导进路*/
							result = CI_TRUE;
						}
						/*6.接近区段分路不良，压入进路首区段*/
						else if ((RS_A_SIGNAL_CLOSED == gr_state(route_index)) && (SCS_CLEARED != gn_section_state(gr_first_section(route_index)))
							&& (SCS_CLEARED == gn_section_state(gr_forward_section(route_index,gr_node_index_route(route_index,gr_first_section(route_index))))))
						{
							/*可以建立引导进路*/
							result = CI_TRUE;
						}
						else
						{
							process_warning(ERR_TRAIN_IN_ROUTE,gr_name(route_index));
							result = CI_FALSE;
							CIHmi_SendNormalTips("车在进路中");
						}					
					}
				}
			}
		}		
	}
	FUNCTION_OUT;
	return result;
}

/****************************************************
函数名:    guide_signal_opening_process
功能描述:  引导信号正在开放执行过程
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/3/22
****************************************************/
void guide_signal_opening_process( route_t route_index )
{
	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*判断信号机按照预期的颜色开放*/
		if (gn_signal_state(gr_start_signal(route_index)) 
		== gn_signal_expect_state(gr_start_signal(route_index)))
		{
			sr_state(route_index,RS_G_SIGNAL_OPENED);
			sr_clear_error_count(route_index);

			CIHmi_SendNormalTips("引导信号开放：%s，%s",gn_name(gr_start_signal(route_index)),"引白");
			/*检查进站信号机内方第一个区段,若占用则开始15s计时*/
			if (SCS_CLEARED != gn_section_state(gr_forward_section(route_index,0)))
			{
				/*检查引导进路是否计时*/
				if (IsFALSE(is_timer_run(route_index)))
				{
					/*开始计时*/
					sr_start_timer(route_index,SECONDS_15,DTT_CLOSE_GUIDE);
				}
			}
			else
			{
				sr_stop_timer(route_index);
			}
		}
		/*信号未按预期的开放*/
		else
		{
			/*信号已开放，但和预期的颜色不一致*/
			if (gn_signal_state(gr_start_signal(route_index)) != SGS_H)
			{
				send_signal_command(route_index,SGS_H);
				sr_state(route_index,RS_G_SO_SIGNAL_CLOSING);
				sr_clear_error_count(route_index);
				process_warning(ERR_SIGNAL_OPEN,gr_name(route_index));
			}
			else
			{
				/*超过MAX_ERROR_PER_COMMAND个周期，信号仍然没有开放*/
				sr_increament_error_count(route_index);
				if (gr_error_count(route_index) > MAX_ERROR_PER_COMMAND)
				{
					sr_clear_error_count(route_index);
					process_warning(ERR_COMMAND_EXECUTE,gr_name(route_index));
				}
			}
		}
	}
	FUNCTION_OUT;
}

/****************************************************
函数名:    guide_signal_opened_process
功能描述:  引导信号已开放的执行过程
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/3/22
****************************************************/
void guide_signal_opened_process( route_t route_index )
{
	CI_BOOL result = CI_TRUE;
	int16_t start_signal,second_section,third_section;
	
	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		start_signal = gr_start_signal(route_index);
		if (gn_signal_state(start_signal) != SGS_ERROR)
		{
			/*检查引导进路是否计时*/
			if (IsTRUE(is_timer_run(route_index)))
			{
				/*2013/9/4 LYC 标准站为信号机内方第一区段占用延时15s过程中区段空闲依然延时到之后关闭引导信号*/
				if (IsTRUE(is_complete_timer(route_index)))
				{
					result = CI_FALSE;
					sr_stop_timer(route_index);
				}
				else
				{
					/*延时过程中列车进入进路则停止计时*/
					if ((RSM_SECOND_SECTION == gr_state_machine(route_index))
						|| (RSM_TRAIN_IN_ROUTE == gr_state_machine(route_index)))
					{
						result = CI_FALSE;
						sr_stop_timer(route_index);
					}
				}
			}
			else
			{
				/*引导进路首区段分路不良*/
				if (RSM_G_FAULT_FIRST == gr_state_machine(route_index))
				{
					second_section = gr_forward_section(route_index,gr_node_index_route(route_index,gr_first_section(route_index)));
					third_section = gr_forward_section(route_index,gr_node_index_route(route_index,second_section));
					if ((SCS_CLEARED != gn_section_state(second_section)) && (SCS_CLEARED != gn_section_state(third_section) && IsFALSE(is_section_fault(third_section))))
					{
						result = CI_FALSE;
					}				
				}

				/*检查进站信号机内方第一个区段*/
				if (SCS_CLEARED != gn_section_state(gr_first_section(route_index)))
				{
					result = CI_FALSE;
				}
			}

			/*判断允许信号断丝*/
			if (gn_signal_state(start_signal) == SGS_FILAMENT_BREAK)
			{
				process_warning(ERR_FILAMENT_BREAK,gr_name(route_index));
				CIHmi_SendNormalTips("断丝");
				result = CI_FALSE;
			}

			/*进路上道岔位置检查*/
			if (IsFALSE(check_switch_location(route_index)))
			{
				process_warning(ERR_SWITCH_LOCATION,gr_name(route_index));
				result = CI_FALSE;
			}

			/*敌对信号检查*/
			if (IsTRUE(check_conflict_signal(route_index))) 
			{
				process_warning(ERR_CONFLICT_ROUTE,gr_name(route_index));
				result = CI_FALSE;
			}

			/*进路上信号点锁闭标志检查*/
			if (IsFALSE(check_node_route_locked(route_index)))
			{
				process_warning(ERR_NODE_UNLOCK,gr_name(route_index));
				result = CI_FALSE;
			}
		}
		else
		{
			result = CI_FALSE;
		}		

		/*关闭引导信号*/
		if (IsFALSE(result))
		{
			sr_stop_timer(route_index);
			send_signal_command(start_signal,SGS_H);
			sr_state(route_index,RS_G_SO_SIGNAL_CLOSING);
			sn_signal_expect_state(start_signal,SGS_H);
			sr_clear_error_count(route_index);
		}
	}	
	FUNCTION_OUT;
}

/****************************************************
函数名:    guide_signal_closing_process
功能描述:  引导信号正在关闭的执行过程
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/3/22
****************************************************/
void guide_signal_closing_process( route_t route_index )
{
	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*信号已关闭*/
		if ((gn_signal_state(gr_start_signal(route_index)) == SGS_H)
			|| (gn_signal_state(gr_start_signal(route_index)) == SGS_ERROR)
			|| (gn_signal_state(gr_start_signal(route_index)) == SGS_FILAMENT_BREAK))
		{
			sr_state(route_index,RS_G_SIGNAL_CLOSED);
			sr_clear_error_count(route_index);
			CIHmi_SendNormalTips("引导信号关闭：%s",gn_name(gr_start_signal(route_index)));
		}
		/*信号未关闭*/
		else
		{
			/*计数，大于MAX_ERROR_PER_COMMAND则认为进路故障*/
			sr_increament_error_count(route_index);
			if (gr_error_count(route_index) > MAX_ERROR_PER_COMMAND)
			{
				sr_state(route_index,RS_FAILURE);
				sr_clear_error_count(route_index);
				process_warning(ERR_CLOSE_SIGNAL,gr_name(route_index));
			}
		}
	}
	FUNCTION_OUT;
}

/****************************************************
函数名:    throat_locked_process
功能描述:  进站信号机所在咽喉被引总锁闭时的执行过程(命令处理模块调用)
返回值:    
参数:      index

作者  :    hjh
日期  ：   2012/3/20
****************************************************/
void throat_locked_guide( int16_t index ) 
{
	CI_BOOL result = CI_TRUE;
	EN_signal_state state,expect_state,histroy_state;

	FUNCTION_IN;
	/*参数检查*/
	if ((index >= 0) && (index < TOTAL_SIGNAL_NODE))
	{
		/*获取信号机的状态和预期状态*/
		state = gn_signal_state(index);
		expect_state = gn_signal_expect_state(index);
		histroy_state = gn_signal_history_state(index);

		/*正在开放引导信号*/
		if ((state == SGS_H) && (histroy_state == SGS_H) && (expect_state == SGS_YB))
		{
			/*hjh 2014-6-12 由于目前计时全部是依附在信号节点上的，有时会发生冲突，故将其屏蔽*/
			/*错误计数3个周期*/
			//if (IsTRUE(is_node_timer_run(index)))
			//{
			//	/*是否完成计时*/
			//	if (IsTRUE(is_node_complete_timer(index)))
			//	{
			//		sn_signal_state(index,SGS_ERROR);
			//		sn_stop_timer(index);
			//		process_warning(ERR_SIGNAL_OPEN,gn_name(index));
			//	}
			//}
			//else
			//{
			//	sn_start_timer(index,MAX_ERROR_PER_COMMAND*CI_CYCLE_MS,DTT_ERROR);
			//}
		}

		/*引导信号已开放*/
		/*hjh 20150609 增加检查已引总锁闭*/
		if ((state == SGS_YB) && (expect_state == SGS_YB) && ((histroy_state == SGS_H) || (histroy_state == SGS_YB))
			&& (IsTRUE(is_node_locked(index,LT_SWITCH_THROAT_LOCKED))))
		{
			/*获取历史状态是禁止信号*/
			if (histroy_state == SGS_H)
			{
				//sn_stop_timer(index);
				CIHmi_SendDebugTips("%s 引导信号已开放！",gn_name(index));
				CIHmi_SendNormalTips("引导信号开放：%s，%s",gn_name(index),"引白");
				/*车正常压入进路，信号正常关闭后开放引导信号时*/
				if (gn_belong_route(index) != NO_INDEX)
				{
					if ((gr_state(gn_belong_route(index)) == RS_SK_N_SIGNAL_CLOSED)
						||(RS_AUTOMATIC_UNLOCKING == gr_state(gn_belong_route(index))))
					{
						sr_state(gn_belong_route(index),RS_FAILURE);
					}
				}
			}

			/*检查进站信号机内方第一个区段*/
			if (SCS_CLEARED != gn_section_state(get_first_section(index)))
			{
				if (gn_signal_history_state(index) == SGS_H)
				{
					/*引导信号开放之前进路内第一区段故障*/
					if (IsFALSE(is_node_timer_run(index)))
					{
						/*开始计时*/
						sn_start_timer(index,SECONDS_15,DTT_CLOSE_GUIDE);
					}
				}

				if (IsTRUE(is_node_timer_run(index)))
				{
					/*是否完成计时*/
					if (IsTRUE(is_node_complete_timer(index)))
					{
						result = CI_FALSE;
					}
					else
					{
						/*延时过程中列车进入进路则停止计时*/
						if ((gn_belong_route(index) != NO_INDEX)
							&& ((RSM_SECOND_SECTION == gr_state_machine(gn_belong_route(index)))
							|| (RSM_TRAIN_IN_ROUTE == gr_state_machine(gn_belong_route(index)))))
						{
							//result = CI_FALSE;
						}
					}	
				}
				/*引导信号开放后，车列驶入进路*/
				else
				{
					result = CI_FALSE;
				}
			}
			/*2013/9/4 LYC 标准站为信号机内方第一区段占用延时15s过程中区段空闲依然延时到之后关闭引导信号*/
			else if ((IsTRUE(is_node_timer_run(index))) && (IsTRUE(is_node_complete_timer(index))))
			{
				result = CI_FALSE;
			}

			/*引总解锁时判断引导信号所在进路的状态*/
			if (IsFALSE(is_throat_lock(gn_throat(index))))
			{
				if ((gn_belong_route(index) != NO_ROUTE) && (RS_G_SIGNAL_OPENED != gr_state(gn_belong_route(index))))
				{
					result = CI_FALSE;
				}				
			}

			if (IsFALSE(result))
			{
				if (IsTRUE(is_node_timer_run(index)) && (signal_nodes[index].time_type == DTT_CLOSE_GUIDE))
				{
					sn_stop_timer(index);
				}			
				send_signal_command(index,SGS_H);
				sn_signal_expect_state(index,SGS_H);
				CIHmi_SendDebugTips("%s 正在关闭引导信号！",gn_name(index));
			}

			if (histroy_state != SGS_YB)
			{
				sn_signal_history_state(index,SGS_YB);
			}			
		}

		/*正在关闭引导信号*/
		if ((state == SGS_YB) && (histroy_state == SGS_YB) && (expect_state == SGS_H))
		{
			/*hjh 2014-6-12 由于目前计时全部是依附在信号节点上的，有时会发生冲突，故将其屏蔽*/
			/*错误计数3个周期*/
			/*检查引导进路是否计时*/
			//if (IsTRUE(is_node_timer_run(index)))
			//{
			//	/*是否完成计时*/
			//	if (IsTRUE(is_node_complete_timer(index)))
			//	{
			//		sn_signal_state(index,SGS_ERROR);
			//		sn_stop_timer(index);
			//		process_warning(ERR_CLOSE_SIGNAL,gn_name(index));
			//	}
			//}
			//else
			//{
			//	sn_start_timer(index,MAX_ERROR_PER_COMMAND*CI_CYCLE_MS,DTT_ERROR);
			//}
		}

		/*引导信号已关闭*/
		if ((state == SGS_H) && (histroy_state == SGS_YB) && (expect_state == SGS_H))
		{
			/*获取历史状态是引导信号*/
			if (gn_signal_history_state(index) == SGS_YB)
			{
				//sn_stop_timer(index);
				sn_signal_history_state(index,SGS_ERROR);
				sn_signal_expect_state(index,SGS_ERROR);
				CIHmi_SendDebugTips("%s 引导信号已关闭！",gn_name(index));
				CIHmi_SendNormalTips("引导信号关闭：%s",gn_name(index));
			}
		}

		/*引导信号开放后红灯断丝*/
		if ((gn_signal_history_state(index) == SGS_YB) && (SGS_FILAMENT_BREAK == gn_signal_state(index)) && (expect_state == SGS_YB))
		{
			//sn_stop_timer(index);
			process_warning(ERR_FILAMENT_BREAK,gn_name(index));
			send_signal_command(index,SGS_H);
			sn_signal_expect_state(index,SGS_H);
			CIHmi_SendDebugTips("%s 正在关闭引导信号！",gn_name(index));
			sn_signal_history_state(index,SGS_YB);
		}

		/*引导信号非法开放*/
		if ((state == SGS_YB) && (expect_state != SGS_YB) && (histroy_state != SGS_YB))
		{
			send_signal_command(index,SGS_H);
		}
	}
	FUNCTION_OUT;
}

/****************************************************
函数名:    throat_unlock_guide
功能描述:  引总解锁时的执行过程
返回值:    
作者  :    hejh
日期  ：   2012/8/7
****************************************************/
void throat_unlock_guide(void)
{
	int16_t i;

	/*关闭引导信号*/
	for (i = 0; i < TOTAL_SIGNAL_NODE; i++)
	{
		/*找出没有所属进路的进站信号机，且其所在咽喉未引总锁闭*/
		if (((gn_type(i) == NT_ROUTE_SIGNAL) || (gn_type(i) == NT_ENTRY_SIGNAL))
		&& (gn_belong_route(i) == NO_ROUTE) && IsFALSE(is_throat_lock(gn_throat(i))))
		{
			/*预期状态是红灯，现在状态是引白*/
			if (gn_state(i) == SGS_YB)
			{
				if (SGS_H == gn_signal_expect_state(i))
				{
					/*检查引导进路是否计时*/
					if (IsTRUE(is_node_timer_run(i)))
					{
						/*是否完成计时*/
						if (IsTRUE(is_node_complete_timer(i)))
						{
							sn_signal_state(i,SGS_ERROR);
							sn_stop_timer(i);
							process_warning(ERR_CLOSE_SIGNAL,gn_name(i));
						}
					}
					/*开始计时*/
					else
					{
						sn_start_timer(i,MAX_ERROR_PER_COMMAND*CI_CYCLE_MS,DTT_ERROR);
					}
				}
			}
			/*引导信号已关闭*/
			if ((gn_state(i) == SGS_H)
				&& (gn_signal_history_state(i) == SGS_YB)
				&& (gn_belong_route(i) == NO_ROUTE))
			{
				sn_stop_timer(i);
				sn_signal_history_state(i,SGS_H);
				CIHmi_SendDebugTips("%s 引导信号已关闭！",gn_name(i));
				CIHmi_SendNormalTips("引导信号关闭：%s",gn_name(i));
			}
		}
	}
}

/****************************************************
函数名:    search_guide_route
功能描述:  搜索引导进路节点算法
返回值:    返回搜索成功时的节点数
参数:      index

作者  :    hjh
日期  ：   2012/3/20
****************************************************/
int16_t search_guide_route( int16_t guide_route_nodes[],int16_t point)
{
	int16_t result = NO_INDEX;
	int16_t temp = NO_INDEX;
	int16_t i;

	/*参数检查*/
	if (guide_route_nodes[point]!=NO_INDEX )
	{
		for (i = 0; i < MAX_NODES_PS; i++)
		{
			if (point == 0)
			{
				/*获取沿进站信号机方向的下一个节点，并放入数组guide_route_nodes*/
				temp = gn_forword(gn_direction(guide_route_nodes[0]),guide_route_nodes[0]);
				//PRINTF2("%-8s  %d",gn_name(temp),temp);
				guide_route_nodes[++point] = temp;
			}
			/*当前节点和进站信号机的方向一致，并且是出站信号机或进路信号机时搜索结束*/
			/*2013/9/3 LYC 增加了当前节点是驼峰信号机时搜索结束*/
			if ((gn_direction(guide_route_nodes[point]) == gn_direction(guide_route_nodes[0])) 
				&& (IsTRUE(is_out_signal(guide_route_nodes[point])) || (NT_ROUTE_SIGNAL == gn_type(guide_route_nodes[point]))|| (NT_HUMPING_SIGNAL == gn_type(guide_route_nodes[point]))))
			{
				guide_route_nodes[point] = NO_INDEX;
				result = point;
				break;
			}
			/*当前节点是道岔*/
			if (gn_type(guide_route_nodes[point]) == NT_SWITCH)
			{
				/*是对向道岔*/
				if (IsTRUE(is_face_switch(gn_direction(guide_route_nodes[0]),guide_route_nodes[point])))
				{
					/*根据道岔实际位置搜索下一节点*/
					if (gn_switch_state(guide_route_nodes[point]) == SWS_NORMAL)
					{
						temp = gn_guide_forword(gn_direction(guide_route_nodes[0]),guide_route_nodes[point]);
					}
					else if (gn_switch_state(guide_route_nodes[point]) == SWS_REVERSE)
					{
						temp = gn_reverse(guide_route_nodes[point]);
					}
					/*道岔在四开位置*/
					else
					{					
						CIHmi_SendNormalTips("道岔位置错误：%s",gn_name(guide_route_nodes[point]));
						result = NO_INDEX;
						break;
					}
				}
				/*是顺向道岔*/
				else
				{
					/*根据道岔实际位置搜索下一节点*/
					if (gn_switch_state(guide_route_nodes[point]) == SWS_NORMAL)
					{
						temp = gn_guide_backword(gn_direction(guide_route_nodes[0]),guide_route_nodes[point]);
					}
					else if (gn_switch_state(guide_route_nodes[point]) == SWS_REVERSE)
					{
						temp = gn_reverse(guide_route_nodes[point]);
					}
					/*道岔在四开位置*/
					else
					{
						CIHmi_SendNormalTips("道岔位置错误：%s",gn_name(guide_route_nodes[point]));
						result = NO_INDEX;
						break;
					}
					/*当前节点和道岔所在位置对应的节点不一致.2013/9/10 LYC point增加了= 1的条件*/
					if ((point >= 1) && (guide_route_nodes[point - 1] != temp))
					{
						CIHmi_SendNormalTips("道岔位置错误：%s",gn_name(guide_route_nodes[point]));
						result = NO_INDEX;
						break;
					}
					else
					{
						temp = gn_guide_forword(gn_direction(guide_route_nodes[0]),guide_route_nodes[point]);	
					}
				}
			}
			/*其他类型节点*/
			else
			{
				temp = gn_guide_forword(gn_direction(guide_route_nodes[0]),guide_route_nodes[point]);			
			}
			/*搜索到死节点*/
			if (temp == NO_INDEX)
			{
				result = point + 1;
				break;
			}
			//PRINTF2("%-8s  %d",gn_name(temp),temp);
			/*将搜索到的节点与数组中的已有节点进行比较，若有重复则搜索失败*/
			for(i=0; i <= point; i++)
			{
				if (temp == guide_route_nodes[i])
				{
					result = point + 1;
					break;
				}
			}
			/*将搜索到的节点放入数组*/
			guide_route_nodes[++point] = temp;
		}
	}
	return result;
}

/****************************************************
函数名:    compare_nodes
功能描述:  找到引导进路对应的进路索引号
返回值:    找到返回进路索引号，否则返回-1
参数:      guide_nodes
参数:      num

作者  :    hjh
日期  ：   2012/3/21
****************************************************/
int16_t compare_nodes(int16_t guide_nodes[],int16_t num)
{
	int16_t i,j,result = NO_INDEX;

	/*遍历联锁表中的所有进路*/
	for (j= 0; j< TOTAL_ILT;j++)
	{
		/*找出列车进路且节点数与num相等的*/
		if ((ILT[j].route_kind == RT_TRAIN_ROUTE) && (num == ILT[j].nodes_count))
		{
			/*逐个比对节点*/
			for (i = 0; i < num; i++)
			{
				if (guide_nodes[i] != ILT[j].nodes[i])
				{
					break;
				}
			}
			if (i == num)
			{
				result = j;
				break;
			}
		}
	}
	return result;
}

/****************************************************
函数名:    set_guide_route
功能描述:  设置引导进路
返回值:    返回进路索引号
参数:      route_index

作者  :    hjh
日期  ：   2012/3/21
****************************************************/
route_t set_guide_route( int16_t route_index )
{
	int16_t i;
	route_t temp_index = NO_INDEX;

	FUNCTION_IN;
	/*参数检查*/
	if ((route_index >= 0) && (route_index < TOTAL_ILT))
	{
		for (i=0; i < MAX_ROUTE; i++)
		{            
			/*设置相关标志位*/
			if(NO_INDEX == routes[i].ILT_index)
			{
				routes[i].ILT_index = route_index;
				routes[i].state = RS_ERROR;
				routes[i].state_machine = RSM_INIT;
				routes[i].current_cycle = CICycleInt_GetCounter();
				routes[i].other_flag = ROF_GUIDE;
				temp_index = (route_t)i;	
				//PRINTF1("搜索到以%s为始端的引导进路！",gn_name(gr_start_signal(temp_index)));
				break;
			}
		}

		/*设置节点所属进路*/
		for ( i = 0; i < gr_nodes_count(temp_index); i++)
		{
			if (NO_INDEX == gn_belong_route(gr_node(temp_index,i)))
			{
				sn_belong_route(gr_node(temp_index,i),temp_index);
			}			
		}
	}
	FUNCTION_OUT;
	return temp_index;
}

/****************************************************
函数名:    check_guide_route_condition
功能描述:  检查引导进路联锁条件并开放信号
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/3/21
****************************************************/
CI_BOOL check_guide_route_condition( route_t route_index )
{
	CI_BOOL result = CI_TRUE;
	
	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
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

		/*道岔位置检查*/	
		if (IsFALSE(check_switch_location(route_index)))
		{
			process_warning(ERR_SWITCH_LOCATION, gr_name(route_index));
			result = CI_FALSE;
		}

		/*红灯灯丝检查*/
		if (SGS_FILAMENT_BREAK == gn_signal_state(gr_start_signal(route_index)))
		{
			//process_warning(ERR_FILAMENT_BREAK,gn_name(gr_start_signal(route_index)));
			//result = CI_FALSE;
		}

		/*检查始端信号机未通信中断*/
		if (gn_signal_state(gr_start_signal(route_index)) == SGS_ERROR)
		{
			result = CI_FALSE;
			CIHmi_SendNormalTips("通信中断：%s",gn_name(gr_start_signal(route_index)));
		}

		/*检查进路上的联系条件*/
		if (IsTRUE(result) && IsFALSE(check_relation_condition(route_index)))
		{
			process_warning(ERR_RELATION_CONDITION, gr_name(route_index));
			result = CI_FALSE;
		}
	}
	else
	{
		result = CI_FALSE;
	}

	FUNCTION_OUT;
	return result;
}

/****************************************************
函数名：   lock_guide_route
功能描述： 锁闭引导进路
返回值：   CI_BOOL
参数：     int16_t route_index
作者：	   hejh
日期：     2014/06/11
****************************************************/
CI_BOOL lock_guide_route( route_t route_index )
{
	CI_BOOL result = CI_TRUE;
	int16_t i,temp;
	node_t switch_index;

	/*锁闭引导进路*/
	for (i = 0; i< gr_nodes_count(route_index); i++)
	{
		sn_locked_state(gr_node(route_index,i),LT_LOCKED);
		/*设置节点所属进路*/
		sn_belong_route(gr_node(route_index,i),route_index);
	}

	/*检查是否存在特殊防护道岔*/
	if ((temp = gs_special(route_index,SPT_SPECIAL_SWTICH)) != NO_INDEX)
	{
		for (i = 0;i < MAX_SPECIAL_SWITCHES; i++)
		{
			/*获取特殊防护道岔的索引号*/
			switch_index = gs_special_switch_index(temp,i);
			if (switch_index != NO_INDEX)
			{
				sn_locked_state(switch_index,LT_MIDDLE_SWITCH_LOCKED);
				/*判断该道岔是否是双动道岔*/
				if (gn_another_switch(switch_index) != NO_INDEX)
				{
					sn_locked_state(gn_another_switch(switch_index),LT_MIDDLE_SWITCH_LOCKED);
				}
			}
		}
	}

	/*检查进路上的节点是否均被进路锁闭*/
	if (IsFALSE(check_node_route_locked(route_index)))
	{
		process_warning(ERR_NODE_UNLOCK,gr_name(route_index));
		result = CI_FALSE;
	}
	else
	{
		CIHmi_SendNormalTips("进路锁闭");
	}

	return result;
}

/****************************************************
函数名:    get_first_section
功能描述:  获取信号机内方第一个区段
返回值:    返回第一个区段索引号
参数:      index

作者  :    hjh
日期  ：   2012/3/22
****************************************************/
int16_t get_first_section( int16_t index )
{
	node_t result = NO_INDEX;
	int16_t i;
	
	FUNCTION_IN;
	/*参数检查*/
	if ((index >= 0) && (index < TOTAL_SIGNAL_NODE))
	{
		/*信号机方向为上行*/
		if (DIR_UP == gn_direction(index))
		{
			/*获取前节点*/
			result = gn_previous(index);
			for (i = 0; i < MAX_SWITCH_PS; i++)
			{
				/*result是区段则跳出循环*/
				if (IsTRUE(is_section(result)))
				{
					break;
				}
				/*result是道岔则根据道岔的方向获取前一节点*/
				else if (IsTRUE(is_switch(result)))
				{
					if ((DIR_RIGHT_UP == gn_direction(result)) ||(DIR_RIGHT_DOWN == gn_direction(result)))
					{
						result = gn_previous(result);
					}

				}
				/*result是信号机或其他则直接取前一节点*/
				else
				{
					result = gn_previous(result);
				}
			}
		}
		/*信号机方向为下行*/
		else if (DIR_DOWN == gn_direction(index))
		{
			/*获取后节点*/
			result = gn_next(index);
			for (i = 0; i < MAX_SWITCH_PS; i++)
			{
				/*result是区段则跳出循环*/
				if (IsTRUE(is_section(result)))
				{
					break;
				}
				/*result是道岔则根据道岔的方向获取前一节点*/
				else if (IsTRUE(is_switch(result)))
				{
					if ((DIR_LEFT_UP == gn_direction(result)) ||(DIR_LEFT_DOWN == gn_direction(result)))
					{
						result = gn_previous(result);
					}

				}
				/*result是信号机或其他则直接取前一节点*/
				else
				{
					result = gn_next(result);
				}
			}
		}
		else
		{
			result = NO_INDEX;
		}
	}
	FUNCTION_OUT;
	return result;
}

/****************************************************
函数名:    open_guide_signal
功能描述:  开放引导信号
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/3/28
****************************************************/
void open_guide_signal( route_t route_index )
{
	node_t index;

	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*红灯灯丝检查*/
		if (SGS_FILAMENT_BREAK == gn_signal_state(gr_start_signal(route_index)))
		{
			process_warning(ERR_FILAMENT_BREAK,gn_name(gr_start_signal(route_index)));
			sr_state(route_index,RS_G_ROUTE_LOCKED);
		}
		/*检查始端信号机未通信中断*/
		else if (gn_signal_state(gr_start_signal(route_index)) == SGS_ERROR)
		{
			process_warning(ERR_SIGNAL_DEVICE_ERR,gn_name(gr_start_signal(route_index)));
		}
		else
		{
			/*开放引导信号*/
			index = gr_start_signal(route_index);
			send_signal_command(index,SGS_YB);
			sr_state(route_index,RS_G_SO_SIGNAL_OPENING);
			sn_signal_expect_state(index,SGS_YB);
			sr_clear_error_count(route_index);
		}		
	}
	FUNCTION_OUT;
}


