/***************************************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.
文件名:  check_ci_condition.c
作者:    CY
版本 :   1.0	
创建日期:2011/12/2
用途:    联锁条件检查
历史修改记录:      
2012/7/20, V1.1, CY:
	1.修改了提示信息。根据实际情况将错误信息，警告信息，提示信息正确分类处理；修改了部分Bug
	2.修改了联锁条件检查模块，选排一致性检查模块，信号检查模块，进路锁闭模块。长调车进路中间的区段占用时，整条进路都不能建立。
	3.修改了X6 向 D6进路选择不正确的Bug
	4.修改了同时向股道排列调车进路时不能排出的Bug
2012/8/10, V1.2, CY:
	1.修改check_ci_condition中的错误提示信息，keep_signal中的BUG
	2.修改侵限区段占用时提示错误的信息， Issue #53 closes #53
	3.修改锁闭标志，及清除锁闭标志的代码
	4.修改检查道岔单锁的代码
	5.调试并修改半自动闭塞功能
2012/12/4 V1.2.1 hjh
	drive_switch中有条件是“带动道岔必须是双动道岔，否则认为数据错误”，50#道岔为单动带动道岔，故将其屏蔽
2012/12/10 V1.2.1 hjh
	check_relation_condition添加对于XLD处的照查条件
2012/12/18 V1.2.1 hjh
	drive_switch添加中间道岔的检查
2013/1/24 V1.2.1 hjh
	app_sw_check_ci_condition长调车进路，本进路因故不能建立时，其前方的进路亦不能建立
2013/1/28 V1.2.1 hjh
	check_ci_condition中增加函数is_special_switch_ok，检查特殊防护道岔是否可以操纵
2013/3/15 V1.2.1 hjh
	1.is_signal_node_used延续进路上的调车进路不检查征用标志
	2.judge_conflict_signal_condition延续进路上的调车进路不检查敌对进路
2013/3/21 V1.2.1 LYC
	is_switch_can_drive判断进路上的道岔是否可转动条件中增加了检查道岔单锁、封锁的条件
2013/4/22 V1.2.1 hjh
	1.app_sw_check_ci_condition增加延续部分存在时再办理延续进路不检查延续部分的条件
	2.删除2013年3月15日增加的内容
2013/7/18V1.2.1 LYC
	修改了check_conflict_signal函数：增加了列车敌对条件中设备错误不是敌对；
	增加了列车与调车敌对条件中设备错误、断丝不是敌对
2013/7/24V1.2.1 LYC
	修改了check_ci_condition联锁条件检查不成功时，清除进路上信号点的征用标志
2013/7/31 V1.2.1 hjh
	app_sw_check_ci_condition长调车进路，前方进路因故不能建立时，本进路及其前方的进路亦不能建立
2013/8/8 V1.2.1 hjh
	增加前后存在联系的进路若其中一条无法建立则其他亦不能建立的约束条件，只限定于锁闭进路以前
2013/8/28 V1.2.1 LYC 
	在check_relation_condition函数中增加了改方运行条件检查
2013/9/2 V1.2.1 hjh
	check_relation_condition增加调车进路的照查条件
2014/2/14 V1.2.1 hjh
	修改drive_switch中间道岔相关的内容
2014/3/14 V1.2.1 hjh
	is_face_conflict增加判断index所属进路不为空的条件
2014/3/24 V1.2.2 hjh
	check_exceed_limit增加判断侵限区段锁闭的条件
2014/4/22 V1.2.2 hjh
	check_exceed_limit根据需求完成侵限区段的检查条件
2014/4/11 V1.2.2 LYC
	is_switch_can_operate检查道岔及双动道岔另一动是否被中间道岔锁闭
***************************************************************************************/
#include "check_ci_condition.h"
#include "utility_function.h"
#include "switch_control.h"
#include "keep_signal.h"
#include "global_data.h"

/****************************************************
函数名:    app_sw_check_ci_condition
功能描述:  联锁条件检查应用软件
返回值:    
参数:      route_t route_index
作者  :    hejh
日期  ：   2012/9/10
****************************************************/
void app_sw_check_ci_condition(route_t route_index)
{
	route_t fwr = NO_INDEX,bwr,oldr;
	int8_t i,j;
	route_t routes_index[MAX_ROUTE];
	int8_t routes_count = 1;
	char_t tips[TEST_NAME_LENGTH];
	
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)) && (gr_state(route_index) == RS_SELECTED))
	{
		/*对本进路进行联锁条件检查*/
		check_ci_condition(route_index);
		if (gr_state(route_index) == RS_CCIC_OK)
		{
			/*将本进路索引号放入数组*/
			routes_index[1] = route_index;
			fwr = gr_forward(route_index);
			/*检查关联的长调车进路，并将进路号放入数组*/
			for (i = fwr; i != NO_INDEX;)
			{
				routes_count++;
				routes_index[routes_count] = i;				
				/*检查延续部分联锁条件*/
				if (IsTRUE(have_successive_route(route_index)))
				{
					if ((gr_state(i) == RS_RCR_SUCCESSIVE_RELAY)
						|| (gr_state(i) == RS_AU_SUCCESSIVE_RELAY)
						|| (gr_state(i) == RS_SU_RELAY)
						|| (gr_state(i) == RS_AUTO_UNLOCK_FAILURE)
						|| (gr_state(i) == RS_CRASH_INTO_SIGNAL))
					{
						sr_state(route_index,RS_FAILURE_TO_BUILD);
						break;
					}
					if (IsTRUE(check_successive_ci_condition(i)))
					{
						if (RS_SELECTED == gr_state(i))
						{
							sr_state(i,RS_CCIC_OK);		
						}										
					}
					else
					{
						if (RS_SELECTED == gr_state(i))
						{
							/*清除进路中及与进路相关信号节点的征用标志*/
							clear_ci_used_flag(i);
							sr_state(i,RS_FAILURE_TO_BUILD);
						}						
						break;
					}
					fwr = gr_forward(i);
					i = fwr;
				}
				else
				{
					check_ci_condition(i);
					/*关联的长进路联锁条件检查完毕*/
					if ((gr_state(i) >= RS_CCIC_OK) && (gr_state(i) <= RS_ROUTE_LOCKED))
					{
						fwr = gr_forward(i);
						i = fwr;
					}
					/*联锁条件检查不成功则直接退出*/
					else
					{	
						routes_count = 0;
						break;
					}
				}
				//check_ci_condition(i);
				///*关联的长进路联锁条件检查完毕*/
				//if (gr_state(i) == RS_CCIC_OK)
				//{
				//	fwr = gr_forward(i);
				//	i = fwr;
				//}
				///*hjh 2014-4-23 延续部分存在时检查延续部分联锁条件*/
				//else if (IsTRUE(have_successive_route(route_index)))
				//{
				//	if ((RS_AU_SUCCESSIVE_RELAY == gr_state(i)) || IsFALSE(check_successive_ci_condition(i)))
				//	{
				//		sr_state(route_index,RS_FAILURE_TO_BUILD);
				//		break;
				//	}
				//	fwr = gr_forward(i);
				//	i = fwr;
				//}
				///*联锁条件检查不成功则直接退出*/
				//else
				//{	
				//	routes_count = 0;
				//	break;
				//}
			}

			/*由远及近设置进路状态*/
			for (j = routes_count; j > 0; j--)
			{
				oldr = routes_index[j];
				bwr = gr_backward(oldr);
				/*检查整个长进路上都联锁条件检查完毕*/
				while ((bwr != NO_INDEX) && (gr_state(bwr) == RS_CCIC_OK))
				{
					oldr = bwr;
					bwr = gr_backward(bwr);
				}
				if (((fwr == NO_INDEX) && (gr_state(oldr) == RS_CCIC_OK))
					|| ((fwr != NO_INDEX) && (gr_state(fwr) == RS_SWITCHING)))
				{
					/*hjh 2013-4-22 延续部分存在时不设置其进路状态*/
					if ((gr_state(routes_index[j]) == RS_CCIC_OK) 
						|| (oldr == NO_INDEX)
						|| IsFALSE(have_successive_route(oldr)))
					{
						if (gr_state(routes_index[j]) <= RS_CCIC_OK)
						{
							/*检查进路内道岔位置是否与进路的要求一致，不一致则将该道岔添加到道岔待转换队列*/
							drive_switch(routes_index[j]);
							/*设置进路状态*/
							sr_state(routes_index[j],RS_SWITCHING);
							/*输出提示信息*/
							memset(tips,0x00,sizeof(tips));
							strcat_check(tips,"正在转岔：",sizeof(tips));
							OutputHmiNormalTips(tips,routes_index[j]);
						}
					}				
				}
			}
		}
		/*hjh 2013-1-24 长调车进路，本进路因故不能建立时，其前方的进路亦不能建立*/
		/*hjh 2013-7-31 长调车进路，前方进路因故不能建立时，本进路及其前方的进路亦不能建立*/
		if ((gr_state(route_index) == RS_FAILURE_TO_BUILD)
			|| ((fwr != NO_INDEX) && (gr_state(fwr) == RS_FAILURE_TO_BUILD)))
		{
			fwr = gr_forward(route_index);
			/*检查关联的长调车进路*/
			for (i = fwr; i != NO_INDEX;)
			{
				/*设置进路状态*/
				if ((gr_state(i) != RS_FAILURE_TO_BUILD) && (gr_state(i) < RS_ROUTE_LOCKED))
				{
					sr_state(i,RS_FAILURE_TO_BUILD);
				}
				i = gr_forward(i);
			}			
		}
	}
}

/****************************************************
函数名:    check_ci_condition
功能描述:  联锁条件检查
返回值:    
参数:      route_index
作者  :    CY
日期  ：   2011/12/2
****************************************************/
void check_ci_condition(route_t route_index)
{
	CI_BOOL result = CI_TRUE;

	FUNCTION_IN;

	/*判断该进路已选出*/
	if (IsTRUE(is_route_exist(route_index)) && gr_state(route_index) == RS_SELECTED )
	{
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

		/*检查进路相关的侵限条件是否满足*/
		if (IsTRUE(result) && IsTRUE(check_exceed_limit(route_index)))
		{
			process_warning(ERR_EXCEED_LIMIT_OCCUPIED, gr_name(route_index));
			result = CI_FALSE;
		}

		/*跨咽喉调车条件检查*/
		if (IsTRUE(result) && IsFALSE(check_throat_shungting(route_index)))
		{
			process_warning(ERR_THROAT_SHUNTING, gr_name(route_index));	
			result = CI_FALSE;
		}		

		/*确认办理进路的咽喉区未办理咽喉区总锁闭*/
		if (IsTRUE(result) && IsTRUE(check_throat_locked(route_index)))
		{
			process_warning(ERR_GUIDE_ALL_LOCKED, gr_name(route_index));
			result = CI_FALSE;
		}	
			
		/*检查进路上的联系条件*/
		if (IsTRUE(result) && IsFALSE(check_relation_condition(route_index)))
		{
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

		/*检查并设置征用标志*/
		if (IsTRUE(result) && IsFALSE(judge_and_set_used_flag(route_index)))
		{
			process_warning(ERR_SIGNAL_NODE_USED, gr_name(route_index));
			result = CI_FALSE;
		}

		/*hjh 2014-08-13检查进路上的信号点所属进路正确*/
		if (IsFALSE(is_all_nodes_belong_route_ok(route_index)))
		{
			result = CI_FALSE;
		}

		/*检查进路上的信号点没有设备错误状态*/
		if (IsFALSE(is_all_nodes_device_ok(route_index))) 
		{
			//result = CI_FALSE;
		}

		/*检查始端信号机未通信中断*/
		if (gn_signal_state(gr_start_signal(route_index)) == SGS_ERROR)
		{
			result = CI_FALSE;
			CIHmi_SendNormalTips("通信中断：%s",gn_name(gr_start_signal(route_index)));
		}
		
		/*设置进路状态为联锁条件检查完毕，否则建立进路失败*/
		if(IsTRUE(result))
		{
			sr_state(route_index,RS_CCIC_OK);
		}
		else
		{
			/*清除进路中及与进路相关信号节点的征用标志*/
			clear_ci_used_flag(route_index);
			sr_state(route_index,RS_FAILURE_TO_BUILD);
		}
	}
	FUNCTION_OUT;
}

/****************************************************
函数名：   check_successive_ci_condition
功能描述： 检查延续进路条件
返回值：   CI_BOOL
参数：     route_t route_index
作者：	   hejh
日期：     2014/04/23
****************************************************/
CI_BOOL check_successive_ci_condition(route_t route_index)
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

		/*检查道岔OK*/
		if (IsTRUE(result) && IsFALSE(is_switch_ok(route_index)))
		{
			process_warning(ERR_SWITCH_LOCATION, gr_name(route_index));
			result = CI_FALSE;
		}

		/*确认区段处于空闲状态*/
		if (IsTRUE(result) && IsFALSE(check_track_cleared(route_index,CI_TRUE)))
		{
			process_warning(ERR_SECTION_OCCUPIED, gr_name(route_index));
			result = CI_FALSE;
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
				
		/*确认办理进路的咽喉区未办理咽喉区总锁闭*/
		if (IsTRUE(result) && IsTRUE(check_throat_locked(route_index)))
		{
			process_warning(ERR_GUIDE_ALL_LOCKED, gr_name(route_index));
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

		/*检查并设置征用标志*/
		if (IsTRUE(result) && (RS_SELECTED == gr_state(route_index)) && IsFALSE(judge_and_set_used_flag(route_index)))
		{
			process_warning(ERR_SIGNAL_NODE_USED, gr_name(route_index));
			result = CI_FALSE;
		}
	}
	FUNCTION_OUT;
	return result;
}

/****************************************************
函数名:    is_all_nodes_device_ok
功能描述:  检查进路中的信号点均存在，检查信号点设备故障
返回值:    TRUE：信号点存在且无故障
		   FALSE：信号点不存在或设备有故障
参数:      route_index
作者  :    WSP
日期  ：   2011/12/22
****************************************************/
CI_BOOL is_all_nodes_device_ok(route_t route_index)
{
	CI_BOOL judge_result = CI_TRUE;
	int16_t i, count, index;
	uint8_t n_type;

	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		count = gr_nodes_count(route_index);
	}
	else
	{
		count = 0;
		judge_result = CI_FALSE;
	}
	
	for (i = 0; i < count; i++)
	{
		/*找出设备类型并判断是否设备故障*/
		index = gr_node(route_index ,i);
		if (index != NO_INDEX)
		{
			n_type = gn_type(index);
			if ((n_type > NT_NO) && (n_type < NT_SIGNAL))
			{
				/*信号机设备状态错误*/
				if (gn_signal_state(index) == SGS_ERROR)
				{					
					if (index == gr_start_signal(route_index))
					{
						judge_result = CI_FALSE;
						process_warning(ERR_SIGNAL_DEVICE_ERR, gn_name(index));
						//CIHmi_SendNormalTips("通信中断：%s",gn_name(index));
						break;
					}
					else
					{
						process_warning(ERR_SIGNAL_DEVICE_ERR, gn_name(index));
						//CIHmi_SendNormalTips("通信中断：%s",gn_name(index));
					}
				}
			}
			else if (n_type == NT_SWITCH)
			{
				/*道岔设备状态错误*/
				if (gn_switch_state(index) == SWS_ERROR)
				{
					judge_result = CI_FALSE;
					process_warning(ERR_SWITCH_DEVICE_ERR,gn_name(index));
					//CIHmi_SendNormalTips("通信中断：%s",gn_name(index));
					break;
				}
			}
			else if (IsTRUE(is_section(index)))
			{
				/*轨道设备状态错误*/
				if (gn_section_state(index) == SCS_ERROR)
				{
					judge_result = CI_FALSE;
					process_warning(ERR_SECTION_DEVICE_ERR, gn_name(index));
					//CIHmi_SendNormalTips("通信中断：%s",gn_name(index));
					break;
				}
			}
			else
			{
				judge_result = CI_TRUE;
			}
		}
	}
	FUNCTION_OUT;
	return judge_result;
}

/****************************************************
函数名：   is_all_nodes_belong_route_ok
功能描述： 检查进路上的信号点所属进路正确
返回值：   CI_BOOL
参数：     route_t route_index
作者：	   hejh
日期：     2014/08/13
****************************************************/
CI_BOOL is_all_nodes_belong_route_ok(route_t route_index)
{
	CI_BOOL judge_result = CI_TRUE;
	int16_t i, count, index;
	node_t another_signal = NO_INDEX;

	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		count = gr_nodes_count(route_index);
	}
	else
	{
		count = 0;
		judge_result = CI_FALSE;
	}

	for (i = 0; i < count; i++)
	{
		/*找出设备类型并判断是否设备故障*/
		index = gr_node(route_index ,i);
		if (index != NO_INDEX)
		{
			if (gn_belong_route(index) != route_index)
			{
				/*hjh 2015-8-11 进路信号机处的调车进路，终端是同方向的尽头线信号机*/
				if (i == (count - 1) 
					&& (gn_direction(gr_node(route_index, i)) == gn_direction(gr_start_signal(route_index)))
					&& (gr_type(route_index) == RT_SHUNTING_ROUTE)
					&& (gn_type(gr_start_signal(route_index)) == NT_ROUTE_SIGNAL)
					&& (gn_type(gr_node(route_index, i)) == NT_STUB_END_SHUNTING_SIGNAL))
				{
					continue;
				}
				/*不是单置信号机*/
				if (!((gn_type(index) == NT_SINGLE_SHUNTING_SIGNAL) && (i + 1 == count)))
				{
					if ((gr_type(route_index) == RT_SHUNTING_ROUTE) && (gn_type(index) == NT_TRACK))
					{
						/*获取股道另一端的信号机*/
						another_signal = gn_forword(gr_direction(route_index),index);
						/*不存在以另一端信号机为终端的进路*/
						if (!((another_signal != NO_INDEX) && (gn_belong_route(another_signal) != NO_ROUTE)
							&& (gr_type(gn_belong_route(another_signal)) == RT_SHUNTING_ROUTE)
							&& another_signal != gr_start_signal(gn_belong_route(another_signal))))
						{
							judge_result = CI_FALSE;
							process_warning(ERR_ROUTE_DATA, gr_name(route_index));
							//CIHmi_SendNormalTips("所属进路错误：%s",gn_name(index));
						}
					}
					else
					{
						judge_result = CI_FALSE;
						process_warning(ERR_ROUTE_DATA, gr_name(route_index));
						//CIHmi_SendNormalTips("所属进路错误：%s",gn_name(index));
					}
				}				
			}
		}
	}
	FUNCTION_OUT;
	return judge_result;
}

/****************************************************
函数名:    drive_switch
功能描述:  检查进路内道岔位置是否与进路的要求一致，不一致则将该道岔添加到道岔待转换队列
返回值:    
参数:      route_index
作者  :    WSP
日期  ：   2011/12/26
****************************************************/
void drive_switch(route_t route_index)
{
	int16_t i,j, count;
	int16_t	index;
	int16_t si,ms,section1,section2,section3;
	CI_BOOL result = CI_FALSE;

	FUNCTION_IN;

	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		count = gr_switches_count(route_index);
	}
	else
	{
		count = 0;
	}

	/*驱动道岔动作*/	
	for (i = 0; i < count; i++)
	{
		index = gr_switch(route_index, i);
		/*找出不在进路要求位置上的道岔*/
		if (gn_switch_state(index) != gr_switch_location(route_index, i))
		{
			/*进路上的道岔或防护道岔*/
			if (IsFALSE(is_follow_switch(route_index, i)))
			{
				/*判断道岔是否可以操纵*/
				/*if (IsTRUE(is_switch_can_operate(index)))
				{
					start_switch(index, gr_switch_location(route_index, i));
				}*/
				/*hjh 2014-2-20 不检查不在进路上的防护道岔*/
				for (j = 0; j < gr_nodes_count(route_index); j++)
				{
					if ((NT_SWITCH == gn_type(gr_node(route_index,j)))
						&& ((index == gr_node(route_index,j)) || (index == gn_another_switch(gr_node(route_index,j)))))
					{
						/*判断道岔是否可以操纵*/
						if (IsTRUE(is_switch_can_operate(index)))
						{
							start_switch(index, gr_switch_location(route_index, i));
						}
						break;
					}
				}
			}
			else
			{
				/*hjh 2012-12-18 添加中间道岔相关代码*/
				si = gs_middle_switch_index(gr_start_signal(route_index));
				/*检查中间道岔是否存在*/
				if (si != NO_INDEX)
				{
					result = CI_FALSE;
					for (j = 0; j < MAX_MIDDLE_SWITCHES; j++)
					{
						/*找出中间道岔索引号*/
						ms = gs_middle_switch(si,j);
						if ((ms != NO_INDEX) && ((ms == index) || (ms == gn_another_switch(index))))
						{
							section1 = gs_middle_section(si,0);
							section2 = gs_middle_section(si,1);
							section3 = gs_middle_section(si,2);
							/*三个区段均占用时，T/52可以在反位*/
							if ((CI_TRUE == gs_middle_allow_reverse(si))
								&& (j == middle_switch_config[si].SwitchCount - 1) 
								&& (section1 != NO_NODE) && (section2 != NO_NODE) && (section3 != NO_NODE))
							{
								/*result = CI_TRUE;
								continue;*/

								if (gn_switch_section(ms) == section2)
								{
									if ((SCS_CLEARED != gn_section_state(section1))
										&& (SCS_CLEARED != gn_section_state(section2)))
									{
										result = CI_TRUE;
										continue;
									}
									else
									{
										/*判断道岔是否可以操纵*/
										if (IsFALSE(is_switch_can_operate(index)) 
											|| IsTRUE(is_node_locked(index,LT_SWITCH_SIGNLE_LOCKED))
											|| IsTRUE(is_node_locked(index,LT_SWITCH_CLOSED)))
										{
											start_switch(index, gr_switch_location(route_index, i));
											result = CI_TRUE;
											break;
										}
										else
										{
											/*判断道岔是否可以操纵*/
											if (IsFALSE(is_switch_can_operate(index)) 
												|| IsTRUE(is_node_locked(index,LT_SWITCH_SIGNLE_LOCKED))
												|| IsTRUE(is_node_locked(index,LT_SWITCH_CLOSED)))
											{
												start_switch(index, gr_switch_location(route_index, i));
												result = CI_TRUE;
												break;
											}
										}
									}
								}
								if (gn_switch_section(ms) == section3)
								{
									if ((SCS_CLEARED != gn_section_state(section1))
										&& (SCS_CLEARED != gn_section_state(section2))
										&& (SCS_CLEARED != gn_section_state(section3)))
									{
										result = CI_TRUE;
										continue;
									}
								}
							}
							else
							{
								/*判断道岔是否可以操纵*/
								if (IsFALSE(is_switch_can_operate(index)) 
									|| IsTRUE(is_node_locked(index,LT_SWITCH_SIGNLE_LOCKED))
									|| IsTRUE(is_node_locked(index,LT_SWITCH_CLOSED)))
								{
									start_switch(index, gr_switch_location(route_index, i));
									result = CI_TRUE;
									break;
								}
							}
							
						}
					}
					/*该条进路存在中间道岔，但此道岔不是中间道岔*/
					if (IsFALSE(result))
					{
						start_switch(index, gr_switch_location(route_index, i));
					}
				}
				/*带动道岔能带则带*/	
				else
				{
					start_switch(index, gr_switch_location(route_index, i));
				}
			}			
		}
	}
	FUNCTION_OUT;
}

/****************************************************
函数名:    check_throat_shungting
功能描述:  跨咽喉调车条件检查
返回值:    TRUE：进路上所有信号点与进路始端信号机在同一咽喉
		   FALSE：进路上某个信号点与进路始端信号机不在同一咽喉
参数:      route_index 进路表中的进路索引号
作者  :    WSP
日期  ：   2011/12/6
****************************************************/
CI_BOOL check_throat_shungting(route_t route_index)
{
	int16_t i;
	uint8_t current_throat;
	CI_BOOL judge_result = CI_TRUE;

	FUNCTION_IN;
	/*判断配置文件是否允许跨咽喉调车*/
	if (IsTRUE(is_route_exist(route_index))) 
	{
		if (RT_SHUNTING_ROUTE == gr_type(route_index))
		{
			/*确定进路咽喉*/
			current_throat = gn_throat(gr_start_signal(route_index)); 
			for (i = 0; i < gr_nodes_count(route_index); i++)
			{
				/*找出不在同一咽喉的信号点，股道不检查其所属咽喉*/
				if ((current_throat != gn_throat(gr_node(route_index, i)))
					&& (gn_type(gr_node(route_index, i)) != NT_TRACK))
				{
					judge_result = CI_FALSE;
					break;
				}
			}
		}
	}
	else
	{
		judge_result = CI_FALSE;
	}
	FUNCTION_OUT;
	return judge_result;
}

/****************************************************
函数名:    is_signal_node_used
功能描述:  检查进路中信号未设置征用标志
返回值:    TRUE：进路上某个信号点已设置征用标志
		   FALSE：进路上所有信号点未设置征用标志
参数:      route_index
作者  :    WSP
日期  ：   2011/12/6
****************************************************/
CI_BOOL is_signal_node_used(route_t route_index)
{
	int16_t i,count;
	int16_t index;
	CI_BOOL judge_result = CI_TRUE;

	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		count = gr_nodes_count(route_index);	
	}
	else
	{
		count = 0;
		judge_result = CI_FALSE;
	}
		
	for (i = 0; i < count; i++)
	{
		index = gr_node(route_index, i);
		/*不检查末端单置信号*/
		if ((i == (count - 1)) && (gn_type(index) == NT_SINGLE_SHUNTING_SIGNAL))
		{
			continue;
		}
		/*检查征用标志*/
		if (IsTRUE(gn_used_state(index)))
		{
			/*除过两个咽喉同时向股道排列调车进路之外，其他均无法办理进路*/
			if(!((gn_type(index) == NT_TRACK)  && (gn_belong_route(index) != route_index) && 
				(gr_type(gn_belong_route(index)) == RT_SHUNTING_ROUTE) && (gr_type(route_index) ==  RT_SHUNTING_ROUTE)))
			{
				judge_result = CI_FALSE;
				process_warning(ERR_SIGNAL_NODE_USED,gn_name(index));
			}
		}
	}
	FUNCTION_OUT;
	return judge_result;
}

/****************************************************
函数名:    set_route_used_flag
功能描述:  设置进路中信号点的征用标志
返回值:    
参数:      route_index
作者  :    WSP
日期  ：   2011/12/22
****************************************************/
void set_route_used_flag(route_t route_index, CI_BOOL used_flag)
{
	int16_t i, count;
	int16_t index;

	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)) && (IsTRUE(used_flag) || IsFALSE(used_flag)))
	{
		count = gr_nodes_count(route_index);
	}
	else
	{
		count = 0;
	}
	
	for (i = 0; i < count; i++)
	{
		index = gr_node(route_index, i);
		/*不设置末端单置信号*/
		if (i == (count - 1) && (gn_type(index) == NT_SINGLE_SHUNTING_SIGNAL))
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
		if (IsTRUE(gn_used_state(index)))
		{
			/*除过两个咽喉同时向股道排列调车进路之外，其他均设置征用标志*/
			if((gn_type(index) == NT_TRACK) && (gn_belong_route(index) != route_index) && 
				(gr_type(gn_belong_route(index)) == RT_SHUNTING_ROUTE) && (gr_type(route_index) ==  RT_SHUNTING_ROUTE))
			{
				continue;
			}
			/*hjh 2014-7-23 属于本进路的节点才设置征用标志*/
			if (gn_belong_route(index) != route_index)
			{
				continue;
			}
		}
		/*设置征用标志*/
		sn_used_state(index, used_flag);
	}
	FUNCTION_OUT;
}
/****************************************************
函数名:    set_used_flag
功能描述:  检查联锁关系条件时，检查并设置征用标志
返回值:    TRUE：检查进路上的信号点未设置征用标志，并设置征用标志成功
		   FALSE：检查进路上的信号点已经被设置了征用标志，设置征用标志失败
参数:      route_index
作者  :    WSP
日期  ：   2011/12/6
****************************************************/
CI_BOOL judge_and_set_used_flag(route_t route_index)
{
	CI_BOOL judge_result = CI_FALSE;
	
	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*检查进路中信号点未设置征用标志*/
		if (IsTRUE(is_signal_node_used(route_index)))
		{
			set_route_used_flag(route_index, CI_TRUE);
			judge_result = CI_TRUE;
		}
	}
	
	FUNCTION_OUT;
	return judge_result;
}

/****************************************************
函数名:    is_node_locked
功能描述:  检查联锁关系条件时，检查进路锁闭标志
返回值:    TRUE：进路上所有信号点已经被设置进路锁闭标志
		   FALSE：进路上有某个信号点未设置进路锁闭标志
参数:      route_index
作者  :    CY
日期  ：   2012/6/15
****************************************************/
CI_BOOL is_any_node_route_locked(route_t route_index)
{
	int16_t i,count;
	int16_t index;
	CI_BOOL judge_result = CI_FALSE;

	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		count = gr_nodes_count(route_index);	
	}
	else
	{
		count = 0;
	}
	/*通过遍历所有进路中节点的方式达到对信号机锁闭状态的检测*/
	for (i = 0; i < count; i++)
	{
		index = gr_node(route_index, i);
		/*不检查末端单置信号*/
		if ((i == (count - 1)) && (gn_type(index) == NT_SINGLE_SHUNTING_SIGNAL))
		{
			continue;
		}
		/*不检查两个咽喉同时向股道排列调车进路*/
		if (IsTRUE(is_node_locked(index,LT_LOCKED)))
		{
			if(!((gn_type(index) == NT_TRACK)
				&& (gn_belong_route(index) != route_index) 
				&& (gr_type(gn_belong_route(index)) == RT_SHUNTING_ROUTE) 
				&& (gr_type(route_index) ==  RT_SHUNTING_ROUTE)))
			{
				judge_result = CI_TRUE;
				if (IsTRUE(is_section(index)))
				{
					CIHmi_SendNormalTips("区段锁闭：%s",gn_name(index));
					break;
				}
				//break;
			}
		}
	}
	FUNCTION_OUT;
	return judge_result;
}

/****************************************************
函数名:    is_node_locked
功能描述:  检查联锁关系条件时，检查道岔单锁锁闭标志
返回值:    TRUE：进路上所有信号点已经被设置道岔单锁锁闭标志
		   FALSE：进路上有某个信号点未设置道岔单锁锁闭标志
参数:      route_index
作者  :    CY
日期  ：   2012/6/15
****************************************************/
CI_BOOL is_any_node_single_locked(route_t route_index)
{
	int16_t i,j,count;
	int16_t index,index1;
	CI_BOOL judge_result = CI_FALSE;

	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		count = gr_nodes_count(route_index);	
	}
	else
	{
		count = 0;
	}
	/*通过遍历所有进路中节点的方式达到对道岔单锁状态的检测*/
	for (i = 0; i < count; i++)
	{
		index = gr_node(route_index, i);
		if (gn_type(index) == NT_SWITCH)
		{
			for (j = 0; j < gr_switches_count(route_index); j++)
			{
				/*检查单锁的道岔位置是否在进路要求的位置上*/
				index1 = gr_switch(route_index,j);
				if ((index1 == index) 
				&& IsTRUE(is_node_locked(index,LT_SWITCH_SIGNLE_LOCKED)) 
				&& (gr_switch_location(route_index,j) != gn_switch_state(index)))
				{
					CIHmi_SendNormalTips("道岔单锁：%s",gn_name(index));
					judge_result = CI_TRUE;
					break;
				}
			}
		}
	}
	FUNCTION_OUT;
	return judge_result;
}

/****************************************************
函数名:    is_node_locked
功能描述:  检查联锁关系条件时，检查锁闭标志
返回值:    TRUE：进路上所有信号点已经被设置道岔封锁标志
		   FALSE：进路上有某个信号点未设置道岔封锁标志
参数:      route_index
作者  :    CY
日期  ：   2012/6/15
****************************************************/
CI_BOOL is_any_node_close_locked(route_t route_index)
{
	int16_t i,count;
	int16_t index;
	CI_BOOL judge_result = CI_FALSE;

	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		count = gr_nodes_count(route_index);	
	}
	else
	{
		count = 0;
	}
	/*通过遍历所有进路中节点的方式达到对道岔锁闭状态的检测*/
	for (i = 0; i < count; i++)
	{
		index = gr_node(route_index, i);
		if (gn_type(index) == NT_SWITCH)
		{
			/*检查道岔封锁标志*/
			if (IsTRUE(is_node_locked(index,LT_SWITCH_CLOSED)))
			{
				judge_result = CI_TRUE;
				CIHmi_SendNormalTips("道岔封锁：%s",gn_name(index));
				break;
			}
		}
	}
	FUNCTION_OUT;
	return judge_result;
}

/****************************************************
函数名:    is_all_node_unlock
功能描述:  所有的信号点都未锁闭
返回值:    
参数:      route_t route_index
作者  :    CY
日期  ：   2012/6/15
****************************************************/
CI_BOOL is_all_node_unlock(route_t route_index)
{
	int16_t i,count;
	int16_t index;
	CI_BOOL judge_result = CI_TRUE;

	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		count = gr_nodes_count(route_index);	
	}
	else
	{
		count = 0;
		judge_result = CI_FALSE;
	}
	/*通过遍历所有进路中节点的方式达到对信号点锁闭状态的检测*/
	for (i = 0; i < count; i++)
	{
		index = gr_node(route_index, i);

		/*不检查末端单置信号*/
		if ((i == (count - 1)) && (gn_type(index) == NT_SINGLE_SHUNTING_SIGNAL))
		{
			continue;
		}

		if (IsTRUE(is_node_locked(index,LT_LOCKED)))
		{
			/*不检查两个咽喉同时向股道排列调车进路*/
			if(!((gn_type(index) == NT_TRACK)// && (gn_belong_route(index) != route_index)
			&& (gr_type(gn_belong_route(index)) == RT_SHUNTING_ROUTE) 
			&& (gr_type(route_index) ==  RT_SHUNTING_ROUTE)))
			{
				judge_result = CI_FALSE;
				break;
			}

		}
	}
	FUNCTION_OUT;
	return judge_result;
}
/****************************************************
函数名:    check_track_cleared
功能描述:  进路上所有区段空闲检查
返回值:    TRUE：进路上所有区段空闲
		   FALSE：进路上有某个区段占用
参数:      route_index
作者  :    WSP
日期  ：   2011/12/6
****************************************************/
CI_BOOL check_track_cleared(route_t route_index,CI_BOOL buildRouteFlag)
{
	int16_t i, count;
	int16_t index;
	CI_BOOL judge_result = CI_TRUE;
	
	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		count = gr_nodes_count(route_index);	
	}
	else
	{
		count = 0;
		judge_result = CI_FALSE;
	}

	for (i = 0; i < count; i++)
	{
		index = gr_node(route_index, i);
		if (IsFALSE(is_section(index)))
		{
			continue;
		}
		/*向股道或尽头线的调车进路不检查股道占用*/
		if (((gn_type(index) == NT_TRACK) && (gr_type(route_index) == RT_SHUNTING_ROUTE))
		|| ((gn_type(index) == NT_STUB_END_SECTION) && (gr_type(route_index) == RT_SHUNTING_ROUTE))
		|| ((gn_type(index) == NT_LOCODEPOT_SECTION) && (gr_type(route_index) == RT_SHUNTING_ROUTE)))
		{
			continue;
		}
		if (IsTRUE(buildRouteFlag))
		{
			/*向无岔区段的调车进路不检查无岔区段占用，长进路需检查*/
			if ((gn_type(index) == NT_NON_SWITCH_SECTION) 
				&& (gr_type(route_index) == RT_SHUNTING_ROUTE) 
				&& (gr_forward(route_index) == NO_INDEX) )
			{
				continue;
			}
		}
		else
		{
			/*向无岔区段的调车进路不检查无岔区段占用*/
			if ((gn_type(index) == NT_NON_SWITCH_SECTION) 
				&& (gr_type(route_index) == RT_SHUNTING_ROUTE))
			{
				continue;
			}
		}		
		/*区段占用时返回FALSE*/
		if (gn_section_state(index) != SCS_CLEARED)
		{
			judge_result = CI_FALSE;
			CIHmi_SendNormalTips("区段占用：%s",gn_name(index));
			break;
		}
	}
	FUNCTION_OUT;
	return judge_result;
}
/****************************************************
函数名:    judge_conflict_signal_condition
功能描述:  判断信号条件敌对
返回值:    TRUE：信号条件敌对成立
		   FALSE：条件敌对不成立
参数:      route_index
参数:      signal_ordinal    进路上所有信号点的序号
参数:      signal_index
作者  :    WSP
日期  ：   2011/12/14
****************************************************/
CI_BOOL judge_conflict_signal_condition(route_t route_index, int16_t signal_ordinal)
{
	int16_t index, switch_index;
	EN_switch_state switch_state;
	CI_BOOL judge_result = CI_FALSE;

	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{	
		/*获取敌对信号索引号*/
		index = gr_conflict_signal(route_index, signal_ordinal);
		/*是条件敌对*/
		if (IsTRUE(gr_conflict_signal_condition(route_index, signal_ordinal)))
		{
			/*获取条件道岔的索引号和位置*/
			switch_index = gr_conflict_signal_switch(route_index, signal_ordinal);
			switch_state = gr_conflict_signal_switch_location(route_index, signal_ordinal);
			/*条件道岔位置和进路规定位置是否一致*/
			if (gn_switch_state(switch_index) == switch_state)
			{
				/*条件道岔和敌对信号是否在同一个进路中*/
				if ((NO_INDEX != gn_belong_route(index))
					&& (gn_belong_route(index) == gn_belong_route(switch_index)))
				{
					judge_result = CI_TRUE;
				}						
			}
		}
	}
	FUNCTION_OUT;
	return judge_result;
}
/****************************************************
函数名:    check_conflict_signal
功能描述:  敌对信号检查
返回值:    TRUE：敌对信号开放
		   FALSE：敌对信号未开放
参数:      route_index
作者  :    WSP
日期  ：   2011/12/6
修改记录:  2012-2-13
****************************************************/
CI_BOOL check_conflict_signal(route_t route_index)
{
	uint8_t i,j;
	int16_t count, index;
	CI_BOOL judge_result = CI_FALSE;
	uint8_t c_type;
	EN_signal_state state;

	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		count = gr_conflict_signals_count(route_index);
	}
	else
	{
		count = 0;
	}
	
	for (i = 0; i < count; i++)
	{
		/*获取敌对信号的索引号，状态及类型*/
		index = gr_conflict_signal(route_index, i);
		state = gn_signal_state(index);
		c_type = gr_conflict_signal_type(route_index, i);

		/*列车敌对时，开放红灯和白灯和蓝灯时不敌对*/
		/*增加了设备错误时列车信号不敌对,断丝不敌对*/
		if ((c_type == SST_TRAIN) && (state != SGS_H) && (state != SGS_B) && (state != SGS_A) && (state != SGS_ERROR) && (state != SGS_FILAMENT_BREAK))
		{
			/*hjh 2014-6-6 引总锁闭时进站信号机开放引导信号不构成敌对*/
			if (SGS_YB == state)
			{
				if (IsFALSE(is_node_locked(gr_start_signal(route_index),LT_SWITCH_THROAT_LOCKED)))
				{
					if (SGS_H != gn_signal_expect_state(index))
					{
						/*是条件敌对*/
						if (IsTRUE(gr_conflict_signal_condition(route_index,i)))
						{
							/*条件敌对成立*/
							if (IsTRUE(judge_conflict_signal_condition(route_index, i)))
							{
								judge_result = CI_TRUE;
								CIHmi_SendNormalTips("敌对进路：%s",gn_name(index));
								break;
							}
						}
						else
						{
							judge_result = CI_TRUE;
							CIHmi_SendNormalTips("敌对进路：%s",gn_name(index));
							break;
						}
					}					
				}
			}
		}

		/*调车敌对时，开放白灯敌对*/
		if ((c_type == SST_SHUNTING) && (state == SGS_B))
		{
			/*是条件敌对*/
			if (IsTRUE(gr_conflict_signal_condition(route_index,i)))
			{
				/*条件敌对成立*/
				if (IsTRUE(judge_conflict_signal_condition(route_index, i)))
				{
					judge_result = CI_TRUE;
					CIHmi_SendNormalTips("敌对进路：%s",gn_name(index));
					break;
				}
			}
			else
			{
				judge_result = CI_TRUE;
				CIHmi_SendNormalTips("敌对进路：%s",gn_name(index));
				break;
			}
		}
		/*列车和调车敌对时，开放的不是红灯且不是蓝灯敌对*/
		if((c_type == SST_TRAIN_SHUNTING) && (state != SGS_H) && (state != SGS_A) && (state != SGS_ERROR) && (state != SGS_FILAMENT_BREAK))
		{
			/*是条件敌对*/
			if (IsTRUE(gr_conflict_signal_condition(route_index,i)))
			{
				/*条件敌对成立*/
				if (IsTRUE(judge_conflict_signal_condition(route_index, i)))
				{
					judge_result = CI_TRUE;
					CIHmi_SendNormalTips("敌对进路：%s",gn_name(index));
					break;
				}
			}
			else
			{
				judge_result = CI_TRUE;
				CIHmi_SendNormalTips("敌对进路：%s",gn_name(index));
				break;
			}
		}
	}

	/*hjh 2015-8-14 本进路与其他进路敌对时不能建立本进路*/
	if ((gr_state(route_index) == RS_SELECTED)
		&& (IsFALSE(judge_result)))
	{
		for (i = 0; i < MAX_ROUTE; i++)
		{
			if (IsFALSE(is_route_exist(i)))
			{
				continue;
			}

			for (j = 0; j < gr_conflict_signals_count(i); j++)
			{
				if ((gr_start_signal(route_index) == gr_conflict_signal(i, j))
					&& (IsFALSE(is_signal_close(i))))
				{
					c_type = gr_conflict_signal_type(i, j);

					if ((c_type == SST_TRAIN) && ((gr_type(route_index) == RT_TRAIN_ROUTE) 
						|| (gr_other_flag(route_index) == ROF_GUIDE) || (gr_other_flag(route_index) == ROF_GUIDE_APPROACH)))
					{
						/*是条件敌对*/
						if (IsTRUE(gr_conflict_signal_condition(i,j)))
						{
							/*条件敌对成立*/
							if (IsTRUE(is_route_signal_conflict(i, j,route_index)))
							{
								judge_result = CI_TRUE;
								CIHmi_SendNormalTips("敌对进路：%s",gn_name(gr_start_signal(i)));
								break;
							}
						}
						else
						{
							judge_result = CI_TRUE;
							CIHmi_SendNormalTips("敌对进路：%s",gn_name(gr_start_signal(i)));
							break;
						}
					}

					/*调车敌对时，开放白灯敌对*/
					if ((c_type == SST_SHUNTING) && (gr_type(route_index) == RT_SHUNTING_ROUTE))
					{
						/*是条件敌对*/
						if (IsTRUE(gr_conflict_signal_condition(i,j)))
						{
							/*条件敌对成立*/
							if (IsTRUE(is_route_signal_conflict(i, j,route_index)))
							{
								judge_result = CI_TRUE;
								CIHmi_SendNormalTips("敌对进路：%s",gn_name(gr_start_signal(i)));
								break;
							}
						}
						else
						{
							judge_result = CI_TRUE;
							CIHmi_SendNormalTips("敌对进路：%s",gn_name(gr_start_signal(i)));
							break;
						}
					}
					/*列车和调车敌对时，开放的不是红灯且不是蓝灯敌对*/
					if(c_type == SST_TRAIN_SHUNTING)
					{
						/*是条件敌对*/
						if (IsTRUE(gr_conflict_signal_condition(route_index,i)))
						{
							/*条件敌对成立*/
							if (IsTRUE(is_route_signal_conflict(i, j,route_index)))
							{
								judge_result = CI_TRUE;
								CIHmi_SendNormalTips("敌对进路：%s",gn_name(gr_start_signal(i)));
								break;
							}
						}
						else
						{
							judge_result = CI_TRUE;
							CIHmi_SendNormalTips("敌对进路：%s",gn_name(gr_start_signal(i)));
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
函数名:    is_face_conflict
功能描述:  是否构成迎面敌对检查
返回值:    TRUE：构成迎面敌对
		   FALSE：不构成迎面敌对
参数:      type    进路类型
参数:      ID      该进路敌对信号的索引号
参数:      self    进路索引号

作者  :    hjh
日期  ：   2012/3/14
****************************************************/
CI_BOOL is_face_conflict(EN_route_type type,node_t index,route_t route_index)
{
	route_t i;
	CI_BOOL result = CI_FALSE;

	FUNCTION_IN;
	/*参数检查*/
	/*hjh 2014-3-14 增加判断index所属金进路不为空的条件*/
	if (((type == RT_TRAIN_ROUTE) || (type == RT_SHUNTING_ROUTE) || (type == RT_SUCCESSIVE_ROUTE))
	&& (index != NO_INDEX) && (IsTRUE(is_route_exist(route_index))) && (NO_INDEX != gn_belong_route(index)))
	{
		for (i = 0; i < MAX_ROUTE; i++)
		{
			/*存在以另一咽喉的出站信号机为终端的进路*/
			if (IsTRUE(is_route_exist(i)) && (i != route_index) 
			&& (gb_node(gr_end_button(i)) == index) && (RS_SELECTED < gr_state(i)))
			{
				/*本进路是列车进路*/
				if (type == RT_TRAIN_ROUTE)
				{
					result = CI_TRUE;
					break;
				}
				/*本进路是调车进路，对方咽喉是列车进路*/
				if (type == RT_SHUNTING_ROUTE && gr_type(i) == RT_TRAIN_ROUTE)
				{
					result = CI_TRUE;
					break;
				}
			}
		}
	}	
	FUNCTION_OUT;
	return result;
}
/****************************************************
函数名:    check_face_conflict
功能描述:  迎面敌对检查
返回值:    敌对返回TRUE，不敌对返回FALSE
参数:      route_index
作者  :    WSP
日期  ：   2011/12/6
****************************************************/
CI_BOOL check_face_conflict(route_t route_index)
{
	int16_t index;
	CI_BOOL judge_result = CI_TRUE;
	EN_route_type tp;

	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*检查列车敌对*/
		tp = gr_type(route_index);
		index = gr_face_train_signal(route_index);
		judge_result = is_face_conflict(tp,index,route_index);	

		if (IsFALSE(judge_result))
		{
			/*检查调车敌对*/
			index = gr_face_shunting_signal(route_index);
			judge_result = is_face_conflict(tp,index,route_index);	
		}
		if (IsTRUE(judge_result))
		{
			CIHmi_SendNormalTips("敌对进路：%s",gn_name(index));
		}
	}
	else
	{
		judge_result = CI_FALSE;
	}
		
	FUNCTION_OUT;
	return judge_result;
}

/****************************************************
函数名:    check_throat_locked
功能描述:  引总锁闭锁闭检查
返回值:    锁闭返回真，反之返回假
参数:      route_index
作者  :    WSP
日期  ：   2011/12/6
****************************************************/
CI_BOOL check_throat_locked(route_t route_index)
{
	int16_t i, index;
	uint8_t current_throat;
	CI_BOOL judge_result = CI_FALSE;

	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*确定进路咽喉*/
		index = gr_start_signal(route_index);
		current_throat = gn_throat(index);
		/*检查咽喉区的道岔状态均为引总锁闭*/
		for (i = 0; i < TOTAL_SIGNAL_NODE; i++)
		{
			if (gn_type(i) == NT_SWITCH)
			{
				if (gn_throat(i) == current_throat)
				{
					/*检查引总锁闭*/
					if (IsTRUE(is_node_locked(i, LT_SWITCH_THROAT_LOCKED)))
					{
						judge_result = CI_TRUE;
						CIHmi_SendNormalTips("咽喉区引总锁闭：%s",gn_name(index));
						break;
					}
				}
			}
		}
	}

	FUNCTION_OUT;
	return judge_result;
}

/****************************************************
函数名:    check_exceed_limit
功能描述:  检查侵限区段空闲；
	      若侵限区段占用，检查侵限区段道岔在规定位置上；

返回值:    侵限返回真，反之返回假
参数:      route_index
作者  :    WSP
日期  ：   2011/12/6
****************************************************/
CI_BOOL check_exceed_limit(route_t route_index)
{
	int16_t i, j,k,count;
	route_t index;
	CI_BOOL judge_result = CI_FALSE;
	node_t condition_switch,fw_section,bw_section,limit_section,cross_switch;

	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		count = gr_sections_count(route_index);
	}
	else
	{
		count = 0;
	}

	for (i = 0; i < count; i++)
	{
		/*侵限区段*/
		if (IsTRUE(is_exceed_limit_section(route_index,i)))
		{
			limit_section = gr_section(route_index,i);
			/*侵限区段占用*/
			if ((gn_section_state(limit_section) != SCS_CLEARED))
			{
				/*有条件道岔*/
				condition_switch = gr_section_condition_switch(route_index, i);
				if (condition_switch != NO_INDEX)
				{
					/*条件道岔在进路要求检查的位置*/
					if ((gn_switch_state(condition_switch) == gr_section_condition_switch_location(route_index,i)) 
						|| (SWS_NO_INDICATE == gn_switch_state(condition_switch)))
					{
						judge_result = CI_TRUE;
						CIHmi_SendNormalTips("侵限占用：%s",gn_name(limit_section));
						break;
					}
					/*交叉渡线处的侵限*/
					for (j = 0; j < MAX_SWITCH_PER_SECTION; j++)
					{
						cross_switch = gn_section_switch(limit_section,j);
						if (cross_switch != NO_INDEX)
						{
							if (cross_switch == gn_another_switch(condition_switch))
							{
								condition_switch = gn_another_switch(condition_switch);
							}
							if ((cross_switch != NO_INDEX) && (condition_switch != cross_switch) 
								&& (((gn_direction(condition_switch) == DIR_LEFT_UP) && (gn_direction(cross_switch) == DIR_RIGHT_UP))
								|| ((gn_direction(condition_switch) == DIR_RIGHT_UP) && (gn_direction(cross_switch) == DIR_LEFT_UP))
								|| ((gn_direction(condition_switch) == DIR_LEFT_DOWN) && (gn_direction(cross_switch) == DIR_RIGHT_DOWN))
								|| ((gn_direction(condition_switch) == DIR_RIGHT_DOWN) && (gn_direction(cross_switch) == DIR_LEFT_DOWN))))
							{
								if (((gr_section_condition_switch_location(route_index,i) == SWS_NORMAL)
									&& (gn_switch_state(condition_switch) != SWS_NORMAL)
									&& (gn_switch_state(cross_switch) != SWS_NORMAL))								
									|| (SWS_NO_INDICATE == gn_switch_state(cross_switch)))
								{
									judge_result = CI_TRUE;
									CIHmi_SendNormalTips("侵限占用：%s",gn_name(limit_section));
									break;
								}
							}
						}						
					}
				}
				else
				{
					judge_result = CI_TRUE;
					CIHmi_SendNormalTips("侵限占用：%s",gn_name(limit_section));
					break;
				}
			}			

			/*hjh 2014-3-24 增加侵限区段锁闭时也认为构成侵限*/
			if (IsFALSE(judge_result) && IsTRUE(is_exceed_limit_section(route_index,i))
				&& IsTRUE(is_node_locked(gr_section(route_index,i),LT_LOCKED)))
			{
				condition_switch = gr_section_condition_switch(route_index, i);
				/*不存在条件道岔或条件道岔不在构成侵限的位置*/
				if ((condition_switch == NO_INDEX)
					|| ((condition_switch != NO_INDEX) && (gn_switch_state(condition_switch) == gr_section_condition_switch_location(route_index,i))))
				{
					/*获取侵限区段所在进路*/
					index = gn_belong_route(gr_section(route_index,i));
					if ((index != NO_INDEX) && (route_index != index))
					{
						judge_result = CI_TRUE;
						fw_section = gr_forward_section(index,gr_node_index_route(index,gr_section(route_index,i)));					
						bw_section = gr_backward_section(index,gr_node_index_route(index,gr_section(route_index,i)));
						for (j = 0; j < count; j++)
						{
							/*侵限区段所在进路的接近区段在本进路中则不认为是侵限*/
							if ((IsTRUE(is_section(gr_section(route_index,j)))) && (gr_section(route_index,j) == gr_approach(index)))
							{
								judge_result = CI_FALSE;
								break;
							}
							/*侵限区段所在进路中与其相邻的区段（在本进路中）已解锁，则认为不构成侵限*/
							if ((IsTRUE(is_section(gr_section(route_index,j)))) && (gr_section(route_index,j) == fw_section) 
								&& (IsFALSE(is_node_locked(fw_section,LT_LOCKED)) || (IsTRUE(is_node_locked(fw_section,LT_LOCKED)) && (gn_belong_route(fw_section) == route_index))))
							{
								for (k = 0; k < gr_nodes_count(index); k++)
								{
									if (IsTRUE(is_section(gr_node(index,k))) && (gr_node(index,k) == fw_section) )
									{
										judge_result = CI_FALSE;
										break;
									}
								}
								if (IsFALSE(judge_result))
								{
									break;
								}
							}
							if ((IsTRUE(is_section(gr_section(route_index,j)))) && (gr_section(route_index,j) == bw_section) 
								&& (IsFALSE(is_node_locked(bw_section,LT_LOCKED)) || (IsTRUE(is_node_locked(bw_section,LT_LOCKED)) && (gn_belong_route(bw_section) == route_index))))
							{
								for (k = 0; k < gr_nodes_count(index); k++)
								{
									if (IsTRUE(is_section(gr_node(index,k))) && (gr_node(index,k) == bw_section) )
									{
										judge_result = CI_FALSE;
										break;
									}
								}
								if (IsFALSE(judge_result))
								{
									break;
								}
							}
						}
						if (IsTRUE(judge_result))
						{
							CIHmi_SendNormalTips("侵限占用：%s",gn_name(limit_section));
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
函数名:    check_relation_condition
功能描述:  联系条件检查
返回值:    联系条件成立返回真，反之返回假
参数:      route_index
作者  :    WSP
日期  ：   2011/12/6
****************************************************/
CI_BOOL check_relation_condition (route_t route_index)
{
	int16_t temp,section = NO_INDEX;
	CI_BOOL result = CI_TRUE;

	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*2013-9-2 hjh 增加调车进路的照查*/
		if (gr_type(route_index) == RT_SHUNTING_ROUTE)
		{
			/*场间联系*/
			if ((temp = gs_special(route_index,SPT_YARDS_LIAISION)) != NO_INDEX)
			{
				/*获取场间联系的状态*/
				if (gn_state(gs_yards_liaision(temp)) != SIO_HAS_SIGNAL)
				{
					result = CI_FALSE;
					process_warning(ERR_RELATION_CONDITION,gr_name(route_index));
					/*输出提示信息*/
					CIHmi_SendNormalTips("照查不对");
				}
			}
			/*检查区间空闲*/
			if (IsFALSE(is_block_cleared(route_index)))
			{
				result = CI_FALSE;
				process_warning(ERR_BLOCK_OCCUPIED,gr_name(route_index));
				/*输出提示信息*/
				CIHmi_SendNormalTips("区间占用");
			}
		}
		/*列车进路*/
		else
		{
			/*列车进路终端为进站信号机*/
			if ((gn_type(gb_node(gr_end_button(route_index))) == NT_ENTRY_SIGNAL)
				|| (gn_type(gb_node(gr_end_button(route_index))) == NT_ROUTE_SIGNAL))
			{
				/*自动闭塞条件检查*/
				if ((temp = gs_special(route_index,SPT_AUTO_BLOCK)) != NO_INDEX)
				{
					/*自动闭塞条件不成立*/
					if (auto_block_check(temp) == SGS_H)
					{
						result = CI_FALSE;
						process_warning(ERR_DEPARTURE_OCCUPIED,gr_name(route_index));
						/*输出提示信息*/
						CIHmi_SendNormalTips("离去区段占用");
					}
				}
				/*三显示自动闭塞条件检查*/
				else if ((temp = gs_special(route_index,SPT_AUTO_BLOCK3)) != NO_INDEX)
				{
					/*三显示自动闭塞条件不成立*/
					if (auto_block3_check(temp) == SGS_H)
					{
						result = CI_FALSE;
						process_warning(ERR_DEPARTURE_OCCUPIED,gr_name(route_index));
						/*输出提示信息*/
						CIHmi_SendNormalTips("离去区段占用");
					}
				}
				/*半自动闭塞条件检查*/
				//else if ((temp = gs_special(route_index,SPT_SEMI_AUTO_BLOCK)) != NO_INDEX)
				//{
				//	/*半自动闭塞条件不成立*/
				//	if (gn_section_state(gn_approach_section(gb_node(gr_end_button(route_index)),1)) != SCS_CLEARED)
				//	{
				//		result = CI_FALSE;
				//		process_warning(ERR_DEPARTURE_OCCUPIED,gr_name(route_index));
				//		/*输出提示信息*/
				//		CIHmi_SendNormalTips("离去区段占用");
				//	}
				//}
				/*2013/8/28 LYC 增加了改方运行条件检查*/
				else if ((temp = gs_special(route_index,SPT_CHANGE_RUN_DIR)) != NO_INDEX)
				{
					if (IsFALSE(is_successive_route(route_index)))
					{
						/*获取改方运行的同意点*/
						if (gs_change_run_dir(temp,0) != NO_INDEX)
						{
							/*改方运行未同意*/
							if (gn_state(gs_change_run_dir(temp,0)) != SIO_HAS_SIGNAL)
							{
								result = CI_FALSE;
								process_warning(ERR_RELATION_CONDITION,gr_name(route_index));
								/*输出提示信息*/
								CIHmi_SendNormalTips("未经同意");
							}
						
							else
							{
								/*检查离去区段状态*/
								if ((gn_section_state(gs_change_run_dir(temp,1)) != SCS_CLEARED)
									|| (gn_section_state(gs_change_run_dir(temp,2)) != SCS_CLEARED)
									|| (gn_section_state(gs_change_run_dir(temp,3)) != SCS_CLEARED))
								{
									result = CI_FALSE;
									process_warning(ERR_DEPARTURE_OCCUPIED,gr_name(route_index));
									/*输出提示信息*/
									CIHmi_SendNormalTips("离去区段占用");
								}
							}
						}
					}										
				}
				else
				{
					section = gr_forward_section(route_index,gr_node_index_route(route_index,gb_node(gr_end_button(route_index))));
					if (SCS_CLEARED != gn_section_state(section))
					{
						CIHmi_SendNormalTips("区段占用");
						result = CI_FALSE;
					}
				}
			}
			/*hjh 2012-12-10 添加对于XLD处的照查条件*/
			/*进路终端不是进站信号机*/
			else
			{
				/*场间联系*/
				if ((temp = gs_special(route_index,SPT_YARDS_LIAISION)) != NO_INDEX)
				{
					/*获取场间联系的状态*/
					if (gn_state(gs_yards_liaision(temp)) != SIO_HAS_SIGNAL)
					{
						result = CI_FALSE;
						process_warning(ERR_RELATION_CONDITION,gr_name(route_index));
						CIHmi_SendNormalTips("照查不对");
					}
				}
			}
		}
	}
	/*进路不存在*/
	else
	{
		result = CI_FALSE;
	}
	FUNCTION_OUT;
	return result;
}
 
/****************************************************
函数名:    is_switch_can_operate
功能描述:  判断道岔是否具备转动条件
返回值:    能驱动返回真，反之返回假
参数:      node_index
作者  :    WSP
日期  ：   2011/12/22
****************************************************/
CI_BOOL is_switch_can_operate(int16_t node_index)
{
	int16_t switch1,switch2,section1,section2;
	CI_BOOL judge_result = CI_TRUE;

	FUNCTION_IN;
	/*参数检查*/
	if ((node_index >= 0) && (node_index < TOTAL_SIGNAL_NODE))
	{
		switch1 = node_index;
		section1 = gn_switch_section(switch1);
		switch2 = gn_another_switch(switch1);
		section2 = (switch2 != NO_INDEX) ?gn_switch_section(switch2):NO_INDEX;

		/*检查道岔及双动道岔另一动所在区段空闲*/
		if (gn_section_state(section1) != SCS_CLEARED)
		{
			process_warning(ERR_SECTION_OCCUPIED,gn_name(section1));
			CIHmi_SendNormalTips("区段占用：%s",gn_name(section1));
			judge_result = CI_FALSE;
		}
		if (IsTRUE(judge_result) && (switch2 != NO_INDEX) && (section1 != section2) && (gn_section_state(section2) != SCS_CLEARED))
		{
			process_warning(ERR_SECTION_OCCUPIED,gn_name(section2));
			CIHmi_SendNormalTips("区段占用：%s",gn_name(section2));
			judge_result = CI_FALSE;
		}
		/*检查道岔及双动道岔的另一动未被进路锁闭*/
		if(IsTRUE(judge_result) && IsTRUE(is_node_locked(switch1,LT_LOCKED)))
		{
			process_warning(ERR_SWITCH_ROUTE_LOCKED,gn_name(switch1));
			CIHmi_SendNormalTips("道岔锁闭：%s",gn_name(switch1));
			judge_result = CI_FALSE;		
		}
		if (IsTRUE(judge_result) && (switch2 != NO_INDEX) && IsTRUE(is_node_locked(switch2,LT_LOCKED)))
		{
			process_warning(ERR_SWITCH_ROUTE_LOCKED,gn_name(switch2));
			CIHmi_SendNormalTips("道岔锁闭：%s",gn_name(switch2));
			judge_result = CI_FALSE;	
		}		
		/*检查道岔及双动道岔另一动所在区段是否被锁闭*/
		if(IsTRUE(judge_result) && IsTRUE(is_node_locked(section1,LT_LOCKED)))
		{
			process_warning(ERR_NODE_LOCKED,gn_name(section1));
			judge_result = CI_FALSE;	
			CIHmi_SendNormalTips("区段锁闭：%s",gn_name(section1));
		}
		if (IsTRUE(judge_result) && (switch2 != NO_INDEX) && IsTRUE(is_node_locked(section2,LT_LOCKED)))
		{
			process_warning(ERR_NODE_LOCKED,gn_name(section2));
			judge_result = CI_FALSE;	
			CIHmi_SendNormalTips("区段锁闭：%s",gn_name(section2));
		}
		/*LYC2013/3/21检查道岔及双动道岔另一动是否被单锁*/
		if(IsTRUE(judge_result) && IsTRUE(is_node_locked(switch1,LT_SWITCH_SIGNLE_LOCKED)))
		{
			process_warning(ERR_SWITCH_SINGLE_LOCKED,gn_name(switch1));
			judge_result = CI_FALSE;	
			CIHmi_SendNormalTips("道岔单锁：%s",gn_name(switch1));
		}
		if (IsTRUE(judge_result) && (switch2 != NO_INDEX) && IsTRUE(is_node_locked(switch2,LT_SWITCH_SIGNLE_LOCKED)))
		{
			process_warning(ERR_SWITCH_SINGLE_LOCKED,gn_name(switch2));
			judge_result = CI_FALSE;	
			/*输出提示信息*/
			CIHmi_SendNormalTips("道岔单锁：%s",gn_name(switch2));
		}
		/*LYC2013/3/21检查道岔及双动道岔另一动是否被封锁*/
		if(IsTRUE(judge_result) && IsTRUE(is_node_locked(switch1,LT_SWITCH_CLOSED)))
		{
			process_warning(ERR_SWITCH_LOCKED,gn_name(switch1));
			judge_result = CI_FALSE;			
			CIHmi_SendNormalTips("道岔封锁：%s",gn_name(switch1));
		}
		if (IsTRUE(judge_result) && (switch2 != NO_INDEX) && IsTRUE(is_node_locked(switch2,LT_SWITCH_CLOSED)))
		{
			process_warning(ERR_SWITCH_LOCKED,gn_name(switch2));
			judge_result = CI_FALSE;	
			CIHmi_SendNormalTips("道岔封锁：%s",gn_name(switch2));
		}
		/*LYC 2014/4/11检查道岔及双动道岔另一动是否被中间道岔锁闭*/
		if(IsTRUE(judge_result) && IsTRUE(is_node_locked(switch1,LT_MIDDLE_SWITCH_LOCKED)))
		{
			process_warning(ERR_SWITCH_LOCKED,gn_name(switch1));
			CIHmi_SendNormalTips("道岔锁闭：%s",gn_name(switch1));
			judge_result = CI_FALSE;			
		}
		if (IsTRUE(judge_result) && (switch2 != NO_INDEX) && IsTRUE(is_node_locked(switch2,LT_MIDDLE_SWITCH_LOCKED)))
		{
			process_warning(ERR_SWITCH_LOCKED,gn_name(switch2));
			judge_result = CI_FALSE;	
			CIHmi_SendNormalTips("道岔锁闭：%s",gn_name(switch2));
		}
	}
	else
	{
		judge_result = CI_FALSE;
	}
	FUNCTION_OUT;
	return judge_result;
}
/****************************************************
函数名:    clear_ci_used_flag
功能描述:  清除联锁条件检查中设置的征用标志
返回值:    
参数:      route_index
作者  :    WSP
日期  ：   2011/12/14
****************************************************/
void clear_ci_used_flag(route_t route_index)
{
	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*设置进路上的信号点为未锁闭状态*/
		set_route_used_flag(route_index, CI_FALSE);
	}
	FUNCTION_OUT;
}

/****************************************************
函数名:    is_middle_switch_ok
功能描述:  判断中间道岔的条件是否满足
返回值:    
参数:      route_t route_index
作者  :    hejh
日期  ：   2012/12/14
****************************************************/
CI_BOOL is_middle_switch_ok(route_t route_index)
{
	CI_BOOL result = CI_TRUE;
	int16_t si,ms,i,j,index;
	node_t section1,section2,section3;

	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*检查是否存在中间道岔*/
		si = gs_middle_switch_index(gr_start_signal(route_index));
		/*检查中间道岔是否存在*/
		if (si != NO_INDEX)
		{
			for (i = 0; i < gr_switches_count(route_index); i++)
			{
				index = gr_switch(route_index, i);
				for (j = 0; j < MAX_MIDDLE_SWITCHES; j++)
				{
					/*找出中间道岔索引号*/
					ms = gs_middle_switch(si,j);
					if ((ms != NO_INDEX) && ((ms == index) || (ms == gn_another_switch(index))))
					{
						/*找出不在进路要求位置上的道岔*/
						if (gn_switch_state(index) != gr_switch_location(route_index, i))
						{
							section1 = gs_middle_section(si,0);
							section2 = gs_middle_section(si,1);
							section3 = gs_middle_section(si,2);
							/*三个区段均占用时，T/52可以在反位*/
							if ((CI_TRUE == gs_middle_allow_reverse(si))
								&& (j == middle_switch_config[si].SwitchCount - 1) 
								&& (section1 != NO_NODE) && (section2 != NO_NODE) && (section3 != NO_NODE))
							{
								if (gn_switch_section(ms) == section2)
								{
									if ((SCS_CLEARED != gn_section_state(section1))
										&& (SCS_CLEARED != gn_section_state(section2)))
									{
										continue;
									}
									else
									{
										/*判断道岔是否可以操纵*/
										if (IsFALSE(is_switch_can_operate(index)) 
											|| IsTRUE(is_node_locked(index,LT_SWITCH_SIGNLE_LOCKED))
											|| IsTRUE(is_node_locked(index,LT_SWITCH_CLOSED))
											|| IsTRUE(is_node_locked(index,LT_MIDDLE_SWITCH_LOCKED)))
										{
											CIHmi_SendNormalTips("中间道岔位置错误：%s",gn_name(index));
											result = CI_FALSE;
											break;
										}
										else
										{
											/*hjh 2015-7-28 判断进路上的道岔正在转岔则不能建立该进路*/
											for (j = 0; j < switching_count; j++)
											{
												/*检查道岔索引号是否存在*/
												if ((switching[j].switch_index == index)
													|| (switching[j].switch_index == gn_another_switch(index)))
												{
													result = CI_FALSE;
													CIHmi_SendNormalTips("道岔正在转岔：%s",gn_name(index));
													break;
												}
											}
										}
									}
								}
								if (gn_switch_section(ms) == section3)
								{
									if ((SCS_CLEARED != gn_section_state(section1))
										&& (SCS_CLEARED != gn_section_state(section2))
										&& (SCS_CLEARED != gn_section_state(section3)))
									{
										/*hjh 2015-7-28 判断进路上的道岔正在转岔则不能建立该进路*/
										for (j = 0; j < switching_count; j++)
										{
											/*检查道岔索引号是否存在*/
											if ((switching[j].switch_index == index)
												|| (switching[j].switch_index == gn_another_switch(index)))
											{
												result = CI_FALSE;
												CIHmi_SendNormalTips("道岔正在转岔：%s",gn_name(index));
												break;
											}
										}
										if (IsTRUE(result))
										{
											continue;
										}
									}
									else
									{
										/*判断道岔是否可以操纵*/
										if (IsFALSE(is_switch_can_operate(index)) 
											|| IsTRUE(is_node_locked(index,LT_SWITCH_SIGNLE_LOCKED))
											|| IsTRUE(is_node_locked(index,LT_SWITCH_CLOSED))
											|| IsTRUE(is_node_locked(index,LT_MIDDLE_SWITCH_LOCKED)))
										{
											CIHmi_SendNormalTips("中间道岔位置错误：%s",gn_name(index));
											result = CI_FALSE;
											break;
										}
										else
										{
											/*hjh 2015-7-28 判断进路上的道岔正在转岔则不能建立该进路*/
											for (j = 0; j < switching_count; j++)
											{
												/*检查道岔索引号是否存在*/
												if ((switching[j].switch_index == index)
													|| (switching[j].switch_index == gn_another_switch(index)))
												{
													result = CI_FALSE;
													CIHmi_SendNormalTips("道岔正在转岔：%s",gn_name(index));
													break;
												}
											}
										}
									}
								}
							}
							else
							{
								/*判断道岔是否可以操纵*/
								if (IsFALSE(is_switch_can_operate(index)) 
									|| IsTRUE(is_node_locked(index,LT_SWITCH_SIGNLE_LOCKED))
									|| IsTRUE(is_node_locked(index,LT_SWITCH_CLOSED))
									|| IsTRUE(is_node_locked(index,LT_MIDDLE_SWITCH_LOCKED)))
								{
									CIHmi_SendNormalTips("中间道岔位置错误：%s",gn_name(index));
									result = CI_FALSE;
									break;
								}
							}
						}
					}
				}
				if (IsFALSE(result))
				{
					break;
				}
			}
		}
	}

	return result;
}

/****************************************************
函数名:    is_special_switch_ok
功能描述:  检查特殊防护道岔位置正确
返回值:    
参数:      route_t route_index
作者  :    hejh
日期  ：   2013/1/28
****************************************************/
CI_BOOL is_special_switch_ok(route_t route_index)
{
	CI_BOOL result = CI_TRUE;
	int16_t temp,i;
	node_t switch_index;

	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*检查是否存在特殊防护道岔*/
		if ((temp = gs_special(route_index,SPT_SPECIAL_SWTICH)) != NO_INDEX)
		{
			for (i = 0;i < MAX_SPECIAL_SWITCHES; i++)
			{
				/*获取特殊防护道岔的索引号*/
				switch_index = gs_special_switch_index(temp,i);
				if ((switch_index != NO_INDEX) 
					&& (gs_special_switch_location(temp,i)  != gn_switch_state(switch_index)))
				{
					/*判断道岔是否可以操纵*/
					if (IsFALSE(is_switch_can_operate(switch_index)) 
						|| IsTRUE(is_node_locked(switch_index,LT_SWITCH_SIGNLE_LOCKED))
						|| IsTRUE(is_node_locked(switch_index,LT_SWITCH_CLOSED))
						|| IsTRUE(is_node_locked(switch_index,LT_MIDDLE_SWITCH_LOCKED)))
					{
						result = CI_FALSE;
						CIHmi_SendNormalTips("特殊防护道岔位置错误：%s",gn_name(switch_index));
						break;
					}
					/*添加到道岔待转换队列*/
					else
					{
						start_switch(switch_index, gs_special_switch_location(temp,i));
					}
				}
			}
		}
	}

	return result;
}

/****************************************************
函数名:    is_switch_ok
功能描述:  检查进路上的道岔都OK
返回值:    
参数:      route_t route_index
作者  :    hejh
日期  ：   2012/12/14
****************************************************/
CI_BOOL is_switch_ok(route_t route_index)
{
	CI_BOOL result = CI_TRUE;
	int16_t i,j,index;

	/*检查进路上的道岔是否能够操纵*/
	for (i = 0; i < gr_switches_count(route_index) && IsTRUE(result); i++)
	{
		index = gr_switch(route_index, i);
		/*进路上的道岔或防护道岔*/
		if (IsFALSE(is_follow_switch(route_index, i)))
		{
			/*找出不在进路要求位置上的道岔*/
			if (gn_switch_state(index) != gr_switch_location(route_index, i))
			{
				/*hjh 2014-2-20 不检查不在进路上的防护道岔*/
				for (j = 0; j < gr_nodes_count(route_index); j++)
				{
					if ((NT_SWITCH == gn_type(gr_node(route_index,j)))
						&& ((index == gr_node(route_index,j)) || (index == gn_another_switch(gr_node(route_index,j)))))
					{
						/*判断道岔是否可以操纵*/
						if (IsFALSE(is_switch_can_operate(index)))
						{
							result = CI_FALSE;
						}
						break;
					}
				}

				/*hjh 2015-7-28 判断进路上的道岔正在转岔则不能建立该进路*/
				for (j = 0; j < switching_count; j++)
				{
					/*检查道岔索引号是否存在*/
					if ((switching[j].switch_index == index)
						|| (switching[j].switch_index == gn_another_switch(index)))
					{
						result = CI_FALSE;
						CIHmi_SendNormalTips("道岔正在转岔：%s",gn_name(index));
						break;
					}
				}
			}
		}
	}

	/*检查进路上所有的道岔未单独锁闭*/
	if (IsTRUE(is_any_node_single_locked(route_index)))
	{
		process_warning(ERR_SWITCH_SINGLE_LOCKED, gr_name(route_index));		
		result = CI_FALSE;
	}

	/*检查进路上所有的道岔未封锁*/
	if (IsTRUE(is_any_node_close_locked(route_index)))
	{
		process_warning(ERR_SWITCH_LOCKED, gr_name(route_index));
		result = CI_FALSE;
	}

	return result;
}

/****************************************************
函数名：   is_route_signal_conflict
功能描述： 判断准备建立的进路与其他进路是否敌对
返回值：   CI_BOOL
参数：     route_t route_index
参数：     int16_t signal_ordinal
参数：     route_t current_route
作者：	   hejh
日期：     2015/08/14
****************************************************/
CI_BOOL is_route_signal_conflict(route_t route_index, int16_t signal_ordinal,route_t current_route)
{
	int16_t index, switch_index,i;
	EN_switch_state switch_state;
	CI_BOOL judge_result = CI_FALSE;

	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{	
		/*获取敌对信号索引号*/
		index = gr_conflict_signal(route_index, signal_ordinal);
		/*是条件敌对*/
		if (IsTRUE(gr_conflict_signal_condition(route_index, signal_ordinal)))
		{
			/*获取条件道岔的索引号和位置*/
			switch_index = gr_conflict_signal_switch(route_index, signal_ordinal);
			switch_state = gr_conflict_signal_switch_location(route_index, signal_ordinal);
			/*条件道岔位置和进路规定位置是否一致*/
			if (index == gr_start_signal(current_route))
			{
				for (i = 0; i < gr_switches_count(current_route); i++)
				{
					if ((switch_index == gr_switch(current_route,i))
						&& (switch_state == gr_switch_location(current_route,i)))
					{						
						judge_result = CI_TRUE;
						break;
					}
				}
			}
		}
	}
	FUNCTION_OUT;
	return judge_result;
}

