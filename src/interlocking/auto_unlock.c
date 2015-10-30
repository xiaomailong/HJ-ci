/***************************************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.
文件名:  auto_unlock.c
作者:    CY
版本 :   1.0	
创建日期:2011/12/2
用途:    自动解锁模块
历史修改记录: 
	V1.1  修改引导进路的判断方法，由gr_type修改为gr_other_flag，并删除RT_GUIDE_ROUTE和RT_SUCCESSIVE_ROUTE。
		  修改函数，发现原来有is_guide_route（），故删除gr_other_flag（）
		  解决了Bug 36-39，32
	V1.2  实现了基本的股道出岔功能，但是解锁时未作严格的检查
		  解决 Issue #94
		  解决 Issue #98
		  解决 Issue #99 
2012/11/26 V1.2.1 hjh
	is_section_failure中增加了进路状态是信号已开放时的轨道故障判断条件
2012/11/28 V1.2.1 hjh
	auto_unlock中258行-264行代码并置信号机折返时的解锁且条件太宽松，已修正
2012/12/4 V1.2.1 hjh
	auto_unlock中267增加折返时进路最后一个区段的解锁
2012/12/19 V1.2.1 hjh
	recieve_route_ums修改接车进路中间道岔的解锁
2013/1/7 V1.2.1 hjh
	process_successive_route将延续进路延时过程中进路故障时修改为停止计时
2013/3/21 V1.2.1 hjh
	is_section_failure中增加判断出清本进路但未进入下一进路的情况
2013/4/2 V1.2.1 hjh
	auto_unlock中调车中途折返解锁时给全部未解锁区段加判断接近区段出清的条件
2013/4/3 V1.2.1 hjh
	1.auto_unlock中增加带有中间道岔的延续进路的处理
	2.recieve_route_ums延时解锁中间道岔时检查进路中最后一区段已解锁则解锁整个进路
2013/4/17 V1.2.1 hjh
	1.check_3points_condition修改三点检查的逻辑（采用状态机）
	2.增加函数shuntting_middle_return_unlock
2013/4/19 V1.2.1 hjh
	shuntting_middle_return_unlock修改调车中途折返解锁的算法（采用状态机）
2013/4/24 V1.2.1 hjh
	check_all_unlock_condition修改多条牵出进路的中途折返解锁算法
2013/8/20 V1.2.1 LYC 
	is_section_failure增加了调车中途折返解锁时，差置或并置信号机的另一架信号开放，这种情况下不应该判断为区段故障
2013/9/4 V1.2.1 hjh
	search_bar_signal增加末端是进路信号机时的处理
2013/9/5 V1.2.1 hjh
	修改代码使得只要满足三点检查条件的区段均可解锁
2014/2/14 V1.2.1 hjh
	departure_route_ums增加检查中间道岔区段的状态
2014/2/20 V1.2.1 hjh
	check_3points_condition中增加本区段空闲时将其故障状态设置为初始化状态
2014/2/21 V1.2.1 hjh mantis:3370
	check_3points_condition判断本区段空闲前方区段占用时将状态机设置为“设备故障”
2014/2/25 V1.2.1 hjh mantis:3394/5
	unlock_section对于道岔区段中的道岔只要属于本进路，不用检查其是否锁闭就能解锁
	unlock_section若解锁区段是进路内第一区段则也复位接近区段
2014/3/5 V1.2.1 hjh
	auto_unlock中增加判断条件，解锁区段序号必须小于故障区段序号
2014/3/11 V1.2.1 hjh mantis:3508
	check_3points_condition增加信号正常关闭证明是调车中途折返，否则是区段故障
2014/4/11 V1.2.2 LYC mantis:3607
	unlock_section如果股道或无岔区段的前一区段在另一进路中则不解锁，且把股道或无岔区段加到另一进路中
2014/4/21 V1.2.2 hjh
	根据需求完成区段三点检查的状态机机列车位置状态机。
2014/5/8 V1.2.3 hjh
	根据调车的情况调整车列位置转移图。
***************************************************************************************/
#include "auto_unlock.h"
#include "cancel_route.h"
#include "check_lock_route.h"
#include "keep_signal.h"
#include "section_unlock.h"
#include "global_data.h"

/****************************************************
函数名:    find_unlock_section
功能描述:  查找待解锁区段
返回值:    
参数:      current_route
作者  :    CY
日期  ：   2011/12/26
****************************************************/
static node_t find_unlock_section(route_t current_route); 

/****************************************************
函数名:    search_bar_signal
功能描述:  找到以该点位接近区段的阻拦信号机的索引号
返回值:    
参数:      route_t current_route
作者  :    CY
日期  ：   2012/3/5
****************************************************/
static node_t search_bar_signal(route_t current_route);

/****************************************************
函数名:    search_return_signal
功能描述:  找到以该点位接近区段的折返信号机的索引号
返回值:    
参数:      route_t current_route
作者  :    CY
日期  ：   2012/3/5
****************************************************/
static int16_t search_return_signal(route_t route_index,CI_BOOL is_all_unlock_route);

/****************************************************
函数名:    is_switch_location_right
功能描述:  检查道岔位置是否正确
返回值:    
参数:      route_t current_route
参数:      node_t current_node
参数:      index_t current_ordinal
作者  :    CY
日期  ：   2012/3/5
****************************************************/
static node_t is_switch_location_right(route_t current_route,node_t current_node,index_t current_ordinal);

/****************************************************
函数名:    process_successive_route
功能描述:  处理延续进路
返回值:    
参数:      route_t current_route
参数:      route_t successive_route
参数:      index_t current_ordinal
作者  :    CY
日期  ：   2012/3/26
****************************************************/
static void process_successive_route(route_t current_route,route_t successive_route);

/****************************************************
函数名：   shuntting_middle_return_unlock
功能描述： 调车中途折返解锁
返回值：   void
参数：     route_t current_route
作者：	   hejh
日期：     2013/04/17
****************************************************/
static void shuntting_middle_return_unlock(route_t current_route);

/****************************************************
函数名：   check_juxtapose_unlock_condition
功能描述： 并置信号机处的折返解锁
返回值：   CI_BOOL
参数：     route_t route_index
作者：	   hejh
日期：     2013/04/19
****************************************************/
static CI_BOOL check_juxtapose_unlock_condition(route_t route_index);

/****************************************************
函数名：   check_part_unlock_condition
功能描述： 部分未解锁区段的折返解锁
返回值：   CI_BOOL
参数：     route_t route_index
作者：	   hejh
日期：     2013/04/19
****************************************************/
static CI_BOOL check_part_unlock_condition(route_t route_index);

/****************************************************
函数名：   check_all_unlock_condition
功能描述： 全部未解锁区段的折返解锁
返回值：   route_t
参数：     route_t route_index
作者：	   hejh
日期：     2013/04/19
****************************************************/
static route_t check_all_unlock_condition(route_t route_index);

/****************************************************
函数名：   auto_unlock_successive
功能描述： 延续进路的解锁
返回值：   void
参数：     route_t current_route
作者：	   hejh
日期：     2013/09/09
****************************************************/
void auto_unlock_successive( route_t current_route );
/****************************************************
函数名：   judge_shuntting_middle_return
功能描述： 判断是否调车中途折返
返回值：   void
参数：     route_t route_index
参数：     node_t current_section
作者：	   hejh
日期：     2014/06/06
****************************************************/
void judge_shuntting_middle_return(route_t route_index, node_t current_section);


/****************************************************
函数名:    auto_unlock
功能描述:  自动解锁
返回值:    
参数:      route_index
作者  :    CY
日期  ：   2011/12/2
****************************************************/
void auto_unlock(route_t current_route)
{	
	EN_route_state rs = gr_state(current_route);
	CI_BOOL error = CI_FALSE;
	index_t i,current_ordinal;
	node_t current_section = NO_NODE;	
	node_t error_switch = NO_NODE;
	char_t tips[TEST_NAME_LENGTH];

	FUNCTION_IN;

	/*参数检查*/
	if (IsFALSE(is_route_exist(current_route)) || IsTRUE(is_guide_route(current_route)))
	{
		error = CI_TRUE;
	}

	/*三点检查状态机*/
	for (i = 0; i < gr_nodes_count(current_route); i++)
	{
		if (IsTRUE(is_section(gr_node(current_route,i)))
			&& IsTRUE(is_node_locked(gr_node(current_route,i),LT_LOCKED))
			&& (gn_belong_route(gr_node(current_route,i)) == current_route))
		{
			check_3points_condition(current_route,i);
		}
	}	
	
	/*只处理处于以下四种状态的进路*/
	if ((RS_SK_N_SIGNAL_CLOSED != rs) && (RS_AUTOMATIC_UNLOCKING != rs) 
		&& (RS_AU_SUCCESSIVE_RELAY != rs) && (RS_AUTO_UNLOCK_FAILURE != rs))
	{
		error = CI_TRUE;
	}

	/*检查轨道停电继电器的状态*/
	if (IsFALSE(error) && IsFALSE(is_track_power_on()))
	{
		error = CI_TRUE;
		if (RS_TRACK_POWER_OFF != rs)
		{
			sr_state(current_route,RS_TRACK_POWER_OFF);
			/*输出提示信息*/
			memset(tips,0x00,sizeof(tips));
			strcat_check(tips,"轨道停电：",sizeof(tips));
			OutputHmiNormalTips(tips,current_route);
		}
	}

	/*延续进路的解锁过程，车接近后转为引导时也要延时*/
	if ((IsFALSE(error)) || (gr_other_flag(current_route) == ROF_GUIDE_APPROACH))
	{
		auto_unlock_successive(current_route);
	}
	
	/*找出待解锁区段*/
	if (IsFALSE(error))
	{
		current_section = find_unlock_section(current_route);
		/*检查找到的带解锁区段是否正确*/
		if (NO_NODE == current_section || IsFALSE(is_section(current_section)))
		{
			error = CI_TRUE;
		}
	}

	/*只处理处于以下四种状态的进路*/
	if ((RS_SK_N_SIGNAL_CLOSED != rs) && (RS_AUTOMATIC_UNLOCKING != rs) 
		&& (RS_AU_SUCCESSIVE_RELAY != rs) && (RS_AUTO_UNLOCK_FAILURE != rs))
	{
		error = CI_TRUE;
	}

	/*自动解锁进路*/
	if (IsFALSE(error))
	{		
		current_ordinal = gr_node_index_route(current_route,current_section);		
		/*检查是否有轨道故障*/
		for (i = current_ordinal; i < gr_nodes_count(current_route); i++)
		{
			/*轨道故障判断*/
			if (IsTRUE(is_section(gr_node(current_route,i))) && IsTRUE(is_section_failure(current_route,i)))
			{
				/*车列未进入进路则置为故障*/
				if (RSM_INIT == gr_state_machine(current_route))
				{
					if (RS_FAILURE != gr_state(current_route))
					{
						process_warning(ERR_SECTION_FAIL,gn_name(gr_node(current_route,i)));
						/*输出提示信息*/
						//CIHmi_SendNormalTips("区段故障：%s",gn_name(gr_node(current_route,i)));
						sr_state(current_route,RS_FAILURE);
					}
				}
				/*车列进入进路则置为自动解锁故障*/
				else
				{
					if (RS_AUTO_UNLOCK_FAILURE != gr_state(current_route))
					{
						process_warning(ERR_SECTION_FAIL,gn_name(gr_node(current_route,i)));
						/*输出提示信息*/
						//CIHmi_SendNormalTips("区段故障：%s",gn_name(gr_node(current_route,i)));
						sr_state(current_route,RS_AUTO_UNLOCK_FAILURE);
					}
				}				
			}
		}
		/*道岔位置检查*/
		error_switch = is_switch_location_right(current_route,current_section,current_ordinal);
		if (NO_NODE != error_switch)
		{			
			/*车列未进入进路则置为故障*/
			if (RSM_INIT == gr_state_machine(current_route))
			{
				if (RS_FAILURE != gr_state(current_route))
				{
					sn_state_machine(gn_switch_section(error_switch),SCSM_FAULT);
					/*输出提示信息*/
					CIHmi_SendNormalTips("道岔位置错误：%s",gn_name(error_switch));
					sr_state(current_route,RS_FAILURE);
				}
			}
			/*车列进入进路则置为自动解锁故障*/
			else
			{
				if (RS_AUTO_UNLOCK_FAILURE != gr_state(current_route))
				{
					sn_state_machine(gn_switch_section(error_switch),SCSM_FAULT);
					/*输出提示信息*/
					CIHmi_SendNormalTips("道岔位置错误：%s",gn_name(error_switch));
					sr_state(current_route,RS_AUTO_UNLOCK_FAILURE);
				}
			}
		}

		/*判断该区段的状态机是区段解锁*/
		if (SCSM_SELF_UNLOCK == gn_state_machine(current_section))
		{
			if (IsFALSE(is_unlock_sections(current_section)) || (RT_SHUNTING_ROUTE != gr_type(current_route)))
			{
				/*解锁本区段*/
				unlock_section(current_route,current_ordinal);
				/*如果正在在解锁区段，则进路必须是RS_AUTOMATIC_UNLOCKING状态*/
				if ((RS_AUTOMATIC_UNLOCKING != rs) && (RS_AUTO_UNLOCK_FAILURE != rs))
				{
					sr_state(current_route,RS_AUTOMATIC_UNLOCKING);					
				}
				/*2014/6/12 LYC matis：3929 解锁的区段是调车进路的第一个区段则检查该进路的接近区段是否锁闭，如果锁闭则将该进路的接近区段状态机设置成区段解锁*/
				if ((current_section == gr_first_section(current_route)) 
					&& (IsTRUE(is_node_locked(gr_approach(current_route),LT_LOCKED)))
					&& (RT_SHUNTING_ROUTE == gr_type(current_route)))
				{
					sn_state_machine(gr_approach(current_route),SCSM_SELF_UNLOCK);
				}
			}						
		}
		/*不满足三点检查条件*/
		else
		{
			if ((SCSM_BEHIND_CLEARED_1 != gn_state_machine(current_section))
				&& (SCSM_BEHIND_CLEARED_2 != gn_state_machine(current_section)))
			{
				/*调车中途折返解锁*/
				shuntting_middle_return_unlock(current_route);
			}			
		}

		/*进路解锁后将其删除*//*检查进路上所有区段均解锁时删除进路*/
		if ((IsTRUE(is_route_exist(current_route))) && (IsTRUE(check_all_sections_unlock(current_route))))
		{
			sr_state(current_route,RS_UNLOCKED);	
			CIHmi_SendNormalTips("进路解锁：%s",gr_name(current_route));
			delete_route(current_route);
		}
	}

	FUNCTION_OUT;
}

/****************************************************
函数名：   auto_unlock_successive
功能描述： 延续进路的解锁
返回值：   void
参数：     route_t current_route
作者：	   hejh
日期：     2013/09/09
****************************************************/
void auto_unlock_successive( route_t current_route ) 
{
	route_t fwr = NO_ROUTE;
	int16_t i;	

	/*处理延续进路*/
	if (IsTRUE(have_successive_route(current_route)))
	{
		fwr = gr_forward(current_route);
		if (NO_ROUTE != fwr)
		{
			/*hjh 2013-4-3 普通延续进路，即总共有2条进路*/
			if ((gr_type(fwr) == RT_TRAIN_ROUTE) || (gr_type(fwr) == RT_SUCCESSIVE_ROUTE))
			{
				process_successive_route(current_route,fwr);
			}
			/*带有中间道岔的延续进路的处理*/
			else
			{
				/*找出延续部分*/
				for (i = 0; i < TOTAL_MIDDLLE_SWITCHES; i++)
				{
					fwr = gr_forward(fwr);
					/*延续部分的进路类型是列车进路*/
					if ((fwr != NO_INDEX) && ((gr_type(fwr) == RT_TRAIN_ROUTE) || (gr_type(fwr) == RT_SUCCESSIVE_ROUTE)))
					{
						process_successive_route(current_route,fwr);
						break;
					}
				}
			}	
		}			
	}
	/*延续进路延时过程的处理*/
	if (gr_state(current_route) == RS_AU_SUCCESSIVE_RELAY)
	{
		process_successive_route(NO_ROUTE,current_route);
	}
}

/****************************************************
函数名：   shuntting_middle_return_unlock
功能描述： 调车中途折返解锁
返回值：   void
参数：     route_t current_route
作者：	   hejh
日期：     2013/04/17
****************************************************/
static void shuntting_middle_return_unlock(route_t current_route)
{
	route_t lead_route = NO_ROUTE;
	CI_BOOL error = CI_FALSE;
	
	/*参数检查*/
	if ((IsFALSE(is_route_exist(current_route))) || (RT_SHUNTING_ROUTE != gr_type(current_route))
		|| (RS_AUTO_UNLOCK_FAILURE == gr_state(current_route)) || (RS_FAILURE == gr_state(current_route)) 
		|| (RS_CRASH_INTO_SIGNAL == gr_state(current_route)))
	{
		error = CI_TRUE;
	}

	if (IsFALSE(error))
	{		
		/*并置信号机处折返解锁*/
		if ((gr_state(current_route) == RS_SK_N_SIGNAL_CLOSED)         
			&& (gn_type(gr_start_signal(current_route)) == NT_JUXTAPOSE_SHUNGTING_SIGNAL)
			&& IsTRUE(check_juxtapose_unlock_condition(current_route)))
		{
			sr_state(current_route,RS_UNLOCKED);
			CIHmi_SendNormalTips("进路解锁：%s",gr_name(current_route));
			delete_route(current_route);
		}
		/*全部未解锁区段解锁*/
		if (IsTRUE(is_route_exist(current_route)))
		{
			lead_route = check_all_unlock_condition(current_route);
			if (lead_route != NO_INDEX)
			{
				sr_state(lead_route,RS_UNLOCKED);
				CIHmi_SendNormalTips("进路解锁：%s",gr_name(current_route));
				delete_route(lead_route);
			}
		}				
		/*部分未解锁区段解锁*/
		if (IsTRUE(is_route_exist(current_route))
			&& (gr_state(current_route) == RS_AUTOMATIC_UNLOCKING) 
			&& IsTRUE(check_part_unlock_condition(current_route)))
		{
			sr_state(current_route,RS_UNLOCKED);
			CIHmi_SendNormalTips("进路解锁：%s",gr_name(current_route));
			delete_route(current_route);
		}			
	}
}

/****************************************************
函数名：   check_juxtapose_unlock_condition
功能描述： 并置信号机处的折返解锁
返回值：   CI_BOOL
参数：     route_t route_index
作者：	   hejh
日期：     2013/04/19
****************************************************/
static CI_BOOL check_juxtapose_unlock_condition(route_t route_index)
{
	CI_BOOL result = CI_TRUE;
	int16_t i;
	node_t return_signal = NO_INDEX;
	node_t start_signal = NO_INDEX;
	route_t return_route = NO_INDEX;

	/*参数检查*/
	if (IsFALSE(is_route_exist(route_index)))
	{
		process_warning(ERR_INDEX,"");
		result = CI_FALSE;
	}

	if (RS_SK_N_SIGNAL_CLOSED != gr_state(route_index))
	{
		result = CI_FALSE;
	}
	/*获取折返信号机及折返进路*/
	start_signal = gr_start_signal(route_index);
	if (NT_JUXTAPOSE_SHUNGTING_SIGNAL == gn_type(start_signal))
	{
		return_signal = gn_another_signal(start_signal);
		/*折返信号已关闭*/
		if (SGS_B != gn_signal_state(return_signal))
		{
			return_route = gn_belong_route(return_signal);;
		}
		else
		{
			return_route = NO_INDEX;
		}		
	}
	
	if (return_route == NO_INDEX)
	{
		result = CI_FALSE;
	}
	else
	{
		/*车列进入折返进路*/
		if (!((return_signal == gr_start_signal(return_route))
			&& (RS_SK_N_SIGNAL_CLOSED == gr_state(return_route))
			&& (SCS_CLEARED != gn_section_state(gr_first_section(return_route)))))
		{
			result = CI_FALSE;
		}
		/*确认道岔位置正确*/
		if (IsFALSE(check_switch_location(route_index)))
		{
			result = CI_FALSE;
		}
		/*确认区段都是空闲的*/
		for (i = 0; i < gr_nodes_count(route_index); i++)
		{
			/*逐个检查区段*/
			if (IsTRUE(is_section(gr_node(route_index,i))) && (gn_belong_route(gr_node(route_index,i)) == route_index))
			{
				/*检查区段的空闲情况*/
				if (gn_section_state(gr_node(route_index,i)) != SCS_CLEARED)
				{
					result = CI_FALSE;
					CIHmi_SendNormalTips("区段占用：%s",gn_name(gr_node(route_index,i)));
					break;
				}
				/*检查区段锁闭*/
				if (IsFALSE(is_node_locked(gr_node(route_index,i),LT_LOCKED)))
				{
					result = CI_FALSE;
					CIHmi_SendNormalTips("区段锁闭：%s",gn_name(gr_node(route_index,i)));
					break;
				}
			}
		}
	}
	return result;
}

/****************************************************
函数名：   check_part_unlock_condition
功能描述： 部分未解锁区段的折返解锁
返回值：   CI_BOOL
参数：     route_t route_index
作者：	   hejh
日期：     2013/04/19
****************************************************/
static CI_BOOL check_part_unlock_condition(route_t route_index)
{
	CI_BOOL result = CI_TRUE;
	node_t return_signal = NO_INDEX;
	route_t return_route = NO_INDEX;
	int16_t i,index,node;
	int16_t node_count = gr_nodes_count(route_index);

	/*参数检查*/
	if (IsFALSE(is_route_exist(route_index)))
	{
		process_warning(ERR_INDEX,"");
		result = CI_FALSE;
	}
	/*获取折返信号*/
	return_signal = search_return_signal(route_index,CI_FALSE);	
	/*折返信号已关闭*/
	if ((return_signal != NO_INDEX) && (SGS_B != gn_signal_state(return_signal)))
	{
		return_route = gn_belong_route(return_signal);;
	}
	else
	{
		return_route = NO_INDEX;
	}	
	
	if (return_route == NO_INDEX)
	{
		result = CI_FALSE;
	}
	else
	{
		/*车列进入折返进路*/
		if (!((return_signal == gr_start_signal(return_route))
			&& (RS_SK_N_SIGNAL_CLOSED == gr_state(return_route))
			&& (SCS_CLEARED != gn_section_state(gr_first_section(return_route)))))
		{
			result = CI_FALSE;
		}		
		/*区段空闲检查*/
		index = gr_node_index_route(route_index,return_signal);		
		for(i = node_count - 1 ;  i > index ; i-- )
		{
			node = gr_node(route_index,i);
			/*过滤单置信号机*/
			if (i == node_count - 1 && gn_type(node) == NT_SINGLE_SHUNTING_SIGNAL)
			{
				continue;
			}
			/*数据检查*/
			if (route_index != gn_belong_route(node))
			{
				process_warning(ERR_ROUTE_DATA,"进路数据错误！");
				sr_state(route_index,RS_FAILURE);
			}
			/*检查区段*/
			if (IsTRUE(is_section(node)) && (SCS_CLEARED != gn_section_state(node)))
			{
				result = CI_FALSE;
				CIHmi_SendNormalTips("区段占用：%s",gn_name(gr_node(route_index,i)));
				break;
			}	
			/*检查区段锁闭*/
			if (IsTRUE(is_section(node)) && IsFALSE(is_node_locked(node,LT_LOCKED)) && (gn_belong_route(node) == route_index))
			{
				result = CI_FALSE;
				CIHmi_SendNormalTips("区段锁闭：%s",gn_name(gr_node(route_index,i)));
				break;
			}
			/*道岔位置检查*/
			if (IsTRUE(is_section(node))) 
			{
				if (NO_NODE != is_switch_location_right(return_route,node,i))
				{
					result = CI_FALSE;
				}				
			}
		}		
	}
	return result;
}

/****************************************************
函数名：   check_all_unlock_condition
功能描述： 全部未解锁区段的折返解锁
返回值：   route_t
参数：     route_t route_index
作者：	   hejh
日期：     2013/04/19
****************************************************/
static route_t check_all_unlock_condition(route_t route_index)
{
	CI_BOOL result = CI_TRUE;
	route_t lead_route = NO_INDEX,return_route = NO_INDEX,temp_route = NO_INDEX;
	int16_t node,i,j,return_signal = NO_INDEX;

	node_t bar_signal = NO_INDEX;

	/*参数检查*/
	if (IsFALSE(is_route_exist(route_index)))
	{
		process_warning(ERR_INDEX,"");
		result = CI_FALSE;
	}
	temp_route = route_index;
	/*hjh 2013-4-24 多条牵出进路时的折返解锁*/
	for (j = 0; j < MAX_ROUTE; j++)
	{
		/*查找阻拦信号机*/
		bar_signal = search_bar_signal(temp_route);
		/*如果找到了阻拦信号机，则获取阻拦信号机所在的进路*/
		if (NO_NODE != bar_signal) //&& (SGS_B != gn_signal_state(bar_signal)))
		{
			lead_route = gn_belong_route(bar_signal);
			/*继续寻找阻拦信号机*/
			if ((lead_route != NO_INDEX) && (RT_SHUNTING_ROUTE == gr_type(lead_route)) && (RS_FAILURE != gr_state(lead_route)))
			{
				temp_route = lead_route;
				j = 0;
			}
			else
			{
				lead_route = temp_route;
				break;
			}
		}
		else
		{
			lead_route = temp_route;
			break;
		}
	}

	if ((lead_route == NO_INDEX) || (SGS_B == gn_signal_state(gr_start_signal(lead_route))) 
		|| (RS_SK_N_SIGNAL_CLOSED != gr_state(lead_route)))
	{
		result = CI_FALSE;
	}
	else
	{
		/*获取折返信号*/
		return_signal = search_return_signal(route_index,CI_TRUE);	
		/*折返信号未关闭*/
		if ((return_signal != NO_INDEX) && (SGS_B == gn_signal_state(return_signal)))
		{
			return_route = gn_belong_route(return_signal);;
		}
		else
		{
			return_route = NO_INDEX;
		}	

		if (return_route == NO_INDEX)
		{
			result = CI_FALSE;
		}
		else
		{
			/*不处理并置信号机处的折返*/
			if ((gn_type(return_signal) == NT_JUXTAPOSE_SHUNGTING_SIGNAL) 
				&& (gr_start_signal(lead_route) == gn_another_signal(return_signal)))
			{
				result = CI_FALSE;
			}

			/*检查接近区段是否出清*/
			if (IsTRUE(result) && (SCS_CLEARED == gn_section_state(gr_approach(lead_route))))
			{
				/*车列进入折返进路*/
				if ((return_signal == gr_start_signal(return_route))
					&& (RS_SIGNAL_OPENED == gr_state(return_route))
					&& (SCS_CLEARED != gn_section_state(gr_first_section(return_route))))
				{
					/*车列退出进路*/
					for (i = gr_nodes_count(lead_route) - 1; i >= 0; i--)
					{
						node = gr_node(lead_route,i);
						if (IsTRUE(is_section(node)) 
							&& ((SCS_CLEARED != gn_section_state(node))
							|| (SCSM_SELF_RETURN != gn_state_machine(node))))
						{
							result = CI_FALSE;
							break;
						}
					}
					if (SCS_CLEARED != gn_section_state(gr_approach(lead_route)))
					{
						result = CI_FALSE;
					}

					/*确认道岔位置正确*/
					if (IsFALSE(check_switch_location(lead_route)))
					{
						result = CI_FALSE;
					}
					/*确认区段都是空闲的*/
					for (i = 0; i < gr_nodes_count(lead_route); i++)
					{
						/*逐个检查区段*/
						if (IsTRUE(is_section(gr_node(lead_route,i))))
						{
							/*检查区段的空闲情况*/
							if (gn_section_state(gr_node(lead_route,i)) != SCS_CLEARED)
							{
								result = CI_FALSE;
								CIHmi_SendNormalTips("区段占用：%s",gn_name(gr_node(lead_route,i)));
								break;
							}
							/*检查区段锁闭*/
							if (IsFALSE(is_node_locked(gr_node(lead_route,i),LT_LOCKED)) && (gn_belong_route(gr_node(lead_route,i)) == lead_route))
							{
								result = CI_FALSE;
								CIHmi_SendNormalTips("区段锁闭：%s",gn_name(gr_node(lead_route,i)));
								break;
							}
						}
					}
				}
				else
				{
					if (RS_FAILURE != gr_state(lead_route))
					{
						sr_state(lead_route,RS_FAILURE);
					}					
					result = CI_FALSE;
				}
			}
			else
			{
				result = CI_FALSE;
			}
		}		
	}

	if (IsFALSE(result))
	{
		lead_route = NO_INDEX;
	}
	return lead_route;
}

/****************************************************
函数名:    is_switch_location_right
功能描述:  检查道岔位置是否正确
返回值:    CI_TRUE:		道岔位置正确
			CI_FALSE:	道岔位置不正确
参数:      route_t current_route
参数:      node_t current_node
参数:      index_t current_ordinal
作者  :    CY
日期  ：   2012/3/5
****************************************************/
static node_t is_switch_location_right(route_t current_route,node_t current_node,index_t current_ordinal)
{
	node_t result = NO_NODE;
	index_t i,j,k;
	node_t node_count = gr_nodes_count(current_route);
	node_t switch_section,current_switch;

	/*参数检查*/
	if((gn_belong_route(current_node) == current_route) && 
		IsTRUE(is_node_locked(current_node, LT_LOCKED)))
	{
		/*道岔未经检查需要从当前的信号点开始往后检查，即对未解锁的进路上的道岔位置的检查*/
		for (i = current_ordinal; i < node_count; i++)
		{
			switch_section = gr_node(current_route,i);	
			/*首先要确定道岔是在本进路上，避免数据错误导致错误的解锁*/
			if ((gn_type(switch_section) == NT_SWITCH_SECTION) && 
				(gn_belong_route(switch_section) == current_route))
			{				
				/*对道岔区段的多个道岔逐个判断*/
				for (j = 0; j < MAX_SWITCH_PER_SECTION; j ++)
				{
					current_switch = gn_section_switch(switch_section,j);
					/*返回值检查*/
					if (current_switch != NO_INDEX)
					{
						for (k = 0 ; k < gr_switches_count(current_route); k++ )
						{	
							/*必须是本进路上的道岔*/
							/*hjh 2014-7-16 判断不是带动道岔*/
							if ( (gr_switch(current_route,k) == current_switch)
								&& (gn_switch_state(current_switch) != gr_switch_location(current_route,k))
								&& (IsFALSE(is_follow_switch(current_route,k))))
							{
								result = current_switch;
								CIHmi_SendNormalTips("道岔位置错误：%s",gn_name(current_switch));
							}
						}
					}
				}
			}
		}
	}
	return result;
}

/****************************************************
函数名：   check_3points_condition
功能描述： 三点检查条件
返回值：   void
参数：     int16_t route_index
参数：     int16_t node_ordinal
作者：	   hejh
日期：     2013/04/17
****************************************************/
void check_3points_condition(route_t route_index,int16_t node_ordinal)
{
	int16_t current_section, fw_section = NO_INDEX, bw_section = NO_INDEX, i;
	node_t node,return_signal = NO_INDEX;

	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)) 
		&& (node_ordinal < gr_nodes_count(route_index))
		&& (route_index == gn_belong_route(gr_node(route_index,node_ordinal))))
	{
		current_section = gr_node(route_index,node_ordinal);
		fw_section = gr_forward_section(route_index,node_ordinal);
		bw_section = gr_backward_section(route_index,node_ordinal);
		
		/*本区段未解锁*/
		if ((NO_INDEX != current_section) && (IsTRUE(is_section(current_section))) 
			&& IsTRUE(is_node_locked(current_section,LT_LOCKED)))
		{
			/***************************************************************
			!!!注意：三点检查状态机中区段通信终端不能认为是占用 hjh 2015-8-6			
			***************************************************************/
			if (gn_section_state(current_section) == SCS_ERROR)
			{
				sn_state_machine(current_section,SCSM_FAULT);
			}
			
			/*通过区段不同的状态机执行不同的操作*/
			switch (gn_state_machine(current_section))
			{
				/*1.初始化状态*/
				case SCSM_INIT:
					/*后方区段不存在或后方区段占用时将状态机设置为“后方区段占用”*/
					if ((bw_section == NO_INDEX) 
						|| ((bw_section != NO_INDEX) && (SCS_CLEARED != gn_section_state(bw_section))))
					{
						sn_state_machine(current_section,SCSM_BEHIND_OCCUPIED);
						break;
					}
					/*后方区段折返时将状态机设置为“折返”*/
					if ((bw_section != NO_INDEX) && (SCSM_SELF_RETURN == gn_state_machine(bw_section))
						&& (RT_SHUNTING_ROUTE == gr_type(route_index)))
					{
						sn_state_machine(current_section,SCSM_SELF_RETURN);
						break;
					}

					/*最后区段是道岔区段且前方区段占用时将状态机设置为“前方区段异常”*/
					if ((current_section == gr_last_section(route_index)) && (NT_SWITCH_SECTION == gn_type(current_section))
						&& (fw_section != NO_INDEX) && (SCS_CLEARED != gn_section_state(fw_section))
						&& (RT_SHUNTING_ROUTE == gr_type(route_index)))
					{
						sn_state_machine(current_section,SCSM_FAULT_FRONT);
						break;
					}
					
					/*本区段占用，列车进路和调车进路非股道、无岔区段、尽头线时将状态机置为“区段故障”*/
					if((bw_section != NO_INDEX) && (SCS_CLEARED != gn_section_state(current_section)))
					{
						if (RT_SHUNTING_ROUTE == gr_type(route_index))
						{
							if ((NT_TRACK != gn_type(current_section)) 
								&& (NT_NON_SWITCH_SECTION != gn_type(current_section)) 
								&& (NT_STUB_END_SECTION != gn_type(current_section))
								&& (NT_LOCODEPOT_SECTION != gn_type(current_section)))
							{
								sn_state_machine(current_section,SCSM_FAULT);
								CIHmi_SendNormalTips("区段故障：%s",gn_name(current_section));
							}
						}
						else
						{
							sn_state_machine(current_section,SCSM_FAULT);
							CIHmi_SendNormalTips("区段故障：%s",gn_name(current_section));
						}	
					}
					break;

				/*2.后方区段占用*/
				case SCSM_BEHIND_OCCUPIED:
					/*当前区段是第一区段时判断后方区段出清时将状态机设置为“初始化”*/
					if ((bw_section != NO_INDEX) && (SCS_CLEARED == gn_section_state(bw_section)) 
						&& (current_section == gr_first_section(route_index)))
					{
						sn_state_machine(current_section,SCSM_INIT);
						break;
					}
					/*判断本区段占用时将状态机设置为“本区段占用”*/
					if (SCS_CLEARED != gn_section_state(current_section))
					{
						sn_state_machine(current_section,SCSM_SELF_OCCUPIED);
						break;
					}
					/*当前区段非第一区段时判断后方区段出清且折返时将状态机设置为“折返”*/
					if ((bw_section != NO_INDEX) && (SCS_CLEARED == gn_section_state(bw_section)) 
						&& (current_section != gr_first_section(route_index))
						&& (SCSM_SELF_RETURN == gn_state_machine(bw_section))
						&& (RT_SHUNTING_ROUTE == gr_type(route_index)))
					{
						sn_state_machine(current_section,SCSM_SELF_RETURN);
						break;
					}
					/*当前区段非第一区段时判断后方区段出清且不是折返时将状态机设置为“后方区段异常空闲”*/
					if ((bw_section != NO_INDEX) && (SCS_CLEARED == gn_section_state(bw_section)) 
						&& (current_section != gr_first_section(route_index))
						&& (SCSM_SELF_RETURN != gn_state_machine(bw_section)))
					{
						sn_state_machine(current_section,SCSM_FAULT_BEHIND);
						CIHmi_SendNormalTips("区段故障：%s",gn_name(current_section));
					}			
					break;

				/*3.本区段占用*/
				case SCSM_SELF_OCCUPIED:
					/*前方区段占用时将状态机设置为“前方区段占用”*/
					if ((fw_section != NO_INDEX) && (SCS_CLEARED != gn_section_state(fw_section)))
					{
						sn_state_machine(current_section,SCSM_FRONT_OCCUPIED_1);
						break;
					}
					/*后方区段空闲时将状态机置为“后方区段空闲”*/
					if (SCS_CLEARED == gn_section_state(bw_section))
					{
						sn_state_machine(current_section,SCSM_BEHIND_CLEARED_2);
						break;
					}
					/*本区段空闲时（股道或无岔区段）将状态机置为“股道空闲且占用过”*/
					if ((SCS_CLEARED == gn_section_state(current_section))
						&& ((NT_TRACK == gn_type(current_section)) || (NT_NON_SWITCH_SECTION == gn_type(current_section))))
					{
						sn_state_machine(current_section,SCSM_TRACK_UNLOCK);
						break;
					}
					/*本区段空闲时将状态机置为“本区段异常空闲”或折返*/
					if (SCS_CLEARED == gn_section_state(current_section))
					{
						/*判断是否调车中途折返并设置相应的状态机*/
						judge_shuntting_middle_return(route_index,current_section);
					}								
					break;

				/*4.前方区段占用1*/
				case SCSM_FRONT_OCCUPIED_1:
					/*本区段空闲且前方区段折返时将状态机设置为“折返”*/
					if ((SCS_CLEARED == gn_section_state(current_section)) 
						&& (fw_section != NO_INDEX) && (SCSM_SELF_RETURN == gn_state_machine(fw_section))
						&& (RT_SHUNTING_ROUTE == gr_type(route_index)))
					{
						sn_state_machine(current_section,SCSM_SELF_RETURN);
						break;
					}
					/*本区段空闲且是最后区段时将状态机设置为“折返”*/
					//if ((SCS_CLEARED == gn_section_state(current_section)) 
					//	&& (current_section == gr_last_section(route_index))
					//	&& (current_section != gr_first_section(route_index))
					//	&& (RT_SHUNTING_ROUTE == gr_type(route_index)))
					//{
					//	sn_state_machine(current_section,SCSM_SELF_RETURN);
					//	break;
					//}
					/*后方区段空闲时将状态机置为“后方区段空闲”*/
					if (SCS_CLEARED == gn_section_state(bw_section))
					{
						sn_state_machine(current_section,SCSM_BEHIND_CLEARED_1);
						break;
					}
					/*本区段空闲且是首区段时将状态机设置为“本区段解锁”*/
					if ((SCS_CLEARED == gn_section_state(current_section)) 
						//&& (current_section == gr_first_section(route_index))
						)
					{
						if (bw_section == NO_INDEX)
						{
							sn_state_machine(current_section,SCSM_SELF_UNLOCK);
							break;
						}
						else
						{
							/*调车进路*/
							if (RT_SHUNTING_ROUTE == gr_type(route_index))
							{
								if (((NT_TRACK == gn_type(bw_section)) 
									|| (NT_NON_SWITCH_SECTION == gn_type(bw_section)) 
									|| (NT_STUB_END_SECTION == gn_type(bw_section))
									|| (NT_LOCODEPOT_SECTION == gn_type(bw_section)))
									&& (SCS_CLEARED != gn_section_state(fw_section)))
								{
									sn_state_machine(current_section,SCSM_SELF_UNLOCK);
									break;
								}
								else
								{
									/*判断是否调车中途折返并设置相应的状态机*/
									judge_shuntting_middle_return(route_index,current_section);
									break;
								}
							}
							/*列车进路*/
							else
							{
								if ((NT_TRACK == gn_type(bw_section)) || (NT_NON_SWITCH_SECTION == gn_type(bw_section)) || (NT_STUB_END_SECTION == gn_type(bw_section))
									|| (NT_APP_DEP_SECTION == gn_type(bw_section))|| (NT_LOCODEPOT_SECTION == gn_type(bw_section)))
								{
									sn_state_machine(current_section,SCSM_SELF_UNLOCK);
									break;
								}
								else
								{
									sn_state_machine(current_section,SCSM_FAULT_CLEARED);
									CIHmi_SendNormalTips("区段故障：%s",gn_name(current_section));
									break;
								}
							}
						}						
					}
					/*本区段空闲且非首区段时将状态机设置为异常空闲*/
					//if ((SCS_CLEARED == gn_section_state(current_section)) 
					//	&& (current_section != gr_first_section(route_index)))
					//{
					//	sn_state_machine(current_section,SCSM_FAULT_CLEARED);
					//	break;
					//}
					break;

				/*5.后方区段空闲1*/
				case SCSM_BEHIND_CLEARED_1:
					/*本区段出清时将状态机设置为“本区段解锁”*/
					if ((SCS_CLEARED == gn_section_state(current_section)) 
						&& (SCS_CLEARED != gn_section_state(fw_section)))
					{
						if ((RT_SHUNTING_ROUTE == gr_type(route_index))
							&& (NT_JUXTAPOSE_SHUNGTING_SIGNAL == gn_type(gr_start_signal(route_index)))
							&& (SGS_B == gn_signal_state(gn_another_signal(gr_start_signal(route_index)))))
						{
							sn_state_machine(current_section,SCSM_FAULT_CLEARED);
							CIHmi_SendNormalTips("区段故障：%s",gn_name(current_section));
						}
						else
						{
							sn_state_machine(current_section,SCSM_SELF_UNLOCK);
						}
					}

					break;					
				
				/*6.后方区段空闲2*/
				case SCSM_BEHIND_CLEARED_2:
					/*前方区段占用时将状态机设置为“前方区段占用”*/
					if ((fw_section != NO_INDEX) && (SCS_CLEARED != gn_section_state(fw_section)))
					{
						sn_state_machine(current_section,SCSM_FRONT_OCCUPIED_2);
						break;
					}
					/*本区段空闲时将状态机设置为“本区段解锁”或异常空闲*/
					if (SCS_CLEARED == gn_section_state(current_section))
					{
						/*调车进路*/
						if (RT_SHUNTING_ROUTE == gr_type(route_index))
						{
							if ((NT_TRACK == gn_type(current_section)) 
								|| (NT_NON_SWITCH_SECTION == gn_type(current_section)) 
								|| (NT_STUB_END_SECTION == gn_type(current_section))
								|| (NT_LOCODEPOT_SECTION == gn_type(current_section)))
							{
								sn_state_machine(current_section,SCSM_SELF_UNLOCK);
							}
							else
							{
								/*并置信号机查找折返信号机*/
								if (NT_JUXTAPOSE_SHUNGTING_SIGNAL == gn_type(gr_start_signal(route_index)))
								{
									node = gn_another_signal(gr_start_signal(route_index));
									/*折返信号机在后一进路上，折返信号机已开放,折返信号机的方向和本进路的方向必须是相反的*/
									if (IsTRUE(is_signal(node)) 
										&& (gn_belong_route(node) != NO_ROUTE)
										&& (gn_belong_route(node) != route_index)
										&& (gr_state(gn_belong_route(node)) == RS_SIGNAL_OPENED)
										&& (((gn_direction(node) == DIR_DOWN) && (gr_direction(route_index) == DIR_UP))
										|| ((gn_direction(node) == DIR_UP) && (gr_direction(route_index) == DIR_DOWN))))
									{
										return_signal = node;
									}
								}
								if (return_signal == NO_INDEX)
								{								
									/*查找折返信号机*/
									for (i = 0; i < gr_nodes_count(route_index); i++)
									{
										node = gr_node(route_index,i);
										/*折返信号机在后一进路上，折返信号机已开放,折返信号机的方向和本进路的方向必须是相反的*/
										if (IsTRUE(is_signal(node)) 
											&& (gn_belong_route(node) != NO_ROUTE)
											&& (gn_belong_route(node) != route_index)
											&& (gr_state(gn_belong_route(node)) == RS_SIGNAL_OPENED)
											&& (((gn_direction(node) == DIR_DOWN) && (gr_direction(route_index) == DIR_UP))
											|| ((gn_direction(node) == DIR_UP) && (gr_direction(route_index) == DIR_DOWN))))
										{
											return_signal = node;
											break;
										}
									}
								}
								/*存在折返信号机*/
								if (return_signal != NO_NODE)
								{
									sn_state_machine(current_section,SCSM_SELF_RETURN);
								}
								/*不存在折返信号机*/
								else
								{
									sn_state_machine(current_section,SCSM_FAULT_CLEARED);
									CIHmi_SendNormalTips("区段故障：%s",gn_name(current_section));
								}
							}
						}
						/*列车进路*/
						else
						{
							if ((NT_TRACK == gn_type(current_section)) 
								|| ((NT_STUB_END_SECTION == gn_type(current_section)) && (current_section == gr_last_section(route_index)))
								|| ((NT_LOCODEPOT_SECTION == gn_type(current_section)) && (current_section == gr_last_section(route_index))))
							{
								sn_state_machine(current_section,SCSM_SELF_UNLOCK);
							}
							else
							{
								sn_state_machine(current_section,SCSM_FAULT_CLEARED);
								CIHmi_SendNormalTips("区段故障：%s",gn_name(current_section));
							}
						}
					}
					break;

				/*7.前方区段占用2*/
				case SCSM_FRONT_OCCUPIED_2:
					/*本区段出清时将状态机设置为“本区段解锁”*/
					if ((SCS_CLEARED == gn_section_state(current_section)) 
						&& (SCS_CLEARED != gn_section_state(fw_section)))
					{
						sn_state_machine(current_section,SCSM_SELF_UNLOCK);
						break;
					}
					break;

				/*8.区段故障*/
				case SCSM_FAULT:
					/*区段空闲且有下一步操作时将状态机设置为初始状态*/
					if (SCS_CLEARED == gn_section_state(current_section))
					{
						/*进路状态为信号已开放*/
						if (RS_SIGNAL_OPENED == gr_state(route_index))
						{
							sn_state_machine(current_section,SCSM_INIT);	
						}
					}
					break;

				case SCSM_FAULT_FRONT:
					/*道岔区段占用且前方区段占用时将状态机设置为“故障”*/
					if ((NT_SWITCH_SECTION == gn_type(current_section)) && (SCS_CLEARED != gn_section_state(current_section)))
					{
						sn_state_machine(current_section,SCSM_FAULT);
						break;
					}
					break;

				default:
					break;
			}
		}
	}
}

/****************************************************
函数名：   judge_shuntting_middle_return
功能描述： 判断是否调车中途折返
返回值：   void
参数：     route_t route_index
参数：     node_t current_section
作者：	   hejh
日期：     2014/06/06
****************************************************/
void judge_shuntting_middle_return(route_t route_index, node_t current_section)
{
	route_t temp = NO_INDEX, bw_route = NO_INDEX;
	node_t node,return_signal = NO_INDEX;
	int16_t i;

	/*调车中途折返作业*/
	if (RT_SHUNTING_ROUTE == gr_type(route_index))
	{							
		/*多条牵出进路时寻找折返进路*/
		temp = route_index;
		for (i = 0; i < MAX_ROUTE; i++)
		{
			/*获取本进路后方的进路*/
			bw_route = gr_backward(temp);
			/*继续寻找后方进路*/
			if ((bw_route != NO_INDEX) 
				&& (gr_direction(bw_route) == gr_direction(route_index))
				&& (gr_state(bw_route) == RS_SIGNAL_OPENED))
			{
				temp = bw_route;
			}
			else if ((bw_route != NO_INDEX) 
				&& (gr_direction(bw_route) == gr_direction(route_index))
				&& (gr_state(bw_route) != RS_SIGNAL_OPENED)
				&& (gr_state(bw_route) != RS_SK_N_SIGNAL_CLOSED)
				&& (gr_state(bw_route) != RS_AUTOMATIC_UNLOCKING))
			{
				bw_route = temp;
				break;
			}
			else
			{
				/*未查找到连续的后方进路时，若中间夹着无岔区段则以当前进路为下一进路*/
				if ((bw_route == NO_INDEX) && ((NT_DIFF_SHUNTING_SIGNAL == gn_type(gr_start_signal(temp)))
					|| (NT_ROUTE_SIGNAL == gn_type(gr_start_signal(temp)))))
				{
					bw_route = temp;
				}
				break;
			}
		}
		/*查找折返信号机*/
		if (bw_route != NO_ROUTE)
		{
			/*并置信号机查找折返信号机*/
			if ((NT_DIFF_SHUNTING_SIGNAL == gn_type(gr_start_signal(bw_route)))
				|| (NT_JUXTAPOSE_SHUNGTING_SIGNAL == gn_type(gr_start_signal(bw_route)))
				|| (NT_ROUTE_SIGNAL == gn_type(gr_start_signal(bw_route))))
			{
				node = gn_another_signal(gr_start_signal(bw_route));
				/*折返信号机在后一进路上，折返信号机已开放,折返信号机的方向和本进路的方向必须是相反的*/
				if (IsTRUE(is_signal(node)) 
					&& (gn_belong_route(node) != NO_ROUTE)
					&& (gn_belong_route(node) != bw_route)
					&& (gr_state(gn_belong_route(node)) == RS_SIGNAL_OPENED)
					&& (((gn_direction(node) == DIR_DOWN) && (gr_direction(bw_route) == DIR_UP))
					|| ((gn_direction(node) == DIR_UP) && (gr_direction(bw_route) == DIR_DOWN))))
				{
					return_signal = node;
				}
			}
			if (return_signal == NO_INDEX)
			{
				/*查找折返信号机*/
				for ( i = gr_nodes_count(bw_route) - 1; i > 0 ; i--)
				{
					node = gr_node(bw_route,i);
					/*折返信号机在后一进路上，折返信号机已开放,折返信号机的方向和本进路的方向必须是相反的*/
					if (IsTRUE(is_signal(node)) 
						&& (gn_belong_route(node) != NO_ROUTE)
						&& (gn_belong_route(node) != bw_route)
						&& (gr_state(gn_belong_route(node)) == RS_SIGNAL_OPENED)
						&& (((gn_direction(node) == DIR_DOWN) && (gr_direction(bw_route) == DIR_UP))
						|| ((gn_direction(node) == DIR_UP) && (gr_direction(bw_route) == DIR_DOWN))))
					{
						return_signal = node;
						break;
					}
				}
			}
			/*存在折返信号机*/
			if (return_signal != NO_NODE)
			{
				sn_state_machine(current_section,SCSM_SELF_RETURN);
			}
			/*不存在折返信号机*/
			else
			{
				sn_state_machine(current_section,SCSM_FAULT_CLEARED);
				CIHmi_SendNormalTips("区段故障：%s",gn_name(current_section));
			}
		}
		/*后方不存在进路*/
		else
		{
			/*并置信号机查找折返信号机*/
			if (NT_JUXTAPOSE_SHUNGTING_SIGNAL == gn_type(gr_start_signal(route_index)))
			{
				node = gn_another_signal(gr_start_signal(route_index));
				/*折返信号机在后一进路上，折返信号机已开放,折返信号机的方向和本进路的方向必须是相反的*/
				if (IsTRUE(is_signal(node)) 
					&& (gn_belong_route(node) != NO_ROUTE)
					&& (gn_belong_route(node) != route_index)
					&& (gr_state(gn_belong_route(node)) == RS_SIGNAL_OPENED)
					&& (((gn_direction(node) == DIR_DOWN) && (gr_direction(route_index) == DIR_UP))
					|| ((gn_direction(node) == DIR_UP) && (gr_direction(route_index) == DIR_DOWN))))
				{
					return_signal = node;
				}
			}

			if (return_signal == NO_INDEX)
			{
				/*查找折返信号机*/
				for ( i = gr_nodes_count(route_index) - 1; i>0 ; i--)
				{
					node = gr_node(route_index,i);
					/*折返信号机在本进路上，折返信号机已经正常关闭*/
					if (IsTRUE(is_signal(node)) 
						&& (gn_belong_route(node) != NO_ROUTE)
						&& (gn_belong_route(node) != route_index)
						&& (gr_state(gn_belong_route(node)) == RS_SIGNAL_OPENED)
						&& (((gn_direction(node) == DIR_DOWN) && (gr_direction(route_index) == DIR_UP))
						|| ((gn_direction(node) == DIR_UP) && (gr_direction(route_index) == DIR_DOWN))))
					{
						/*折返信号机的方向和本进路的方向必须是相反的*/
						if (((gn_direction(node) == DIR_DOWN) && (gr_direction(route_index) == DIR_UP))
							|| ((gn_direction(node) == DIR_UP) && (gr_direction(route_index) == DIR_DOWN)))
						{
							return_signal = node;
							break;
						}

					}
				}
			}
			
			/*存在折返信号机*/
			if (return_signal != NO_NODE)
			{
				sn_state_machine(current_section,SCSM_SELF_RETURN);
			}
			/*不存在折返信号机*/
			else
			{
				sn_state_machine(current_section,SCSM_FAULT_CLEARED);
				CIHmi_SendNormalTips("区段故障：%s",gn_name(current_section));
			}
		}							
	}
	else
	{
		sn_state_machine(current_section,SCSM_FAULT_CLEARED);
		CIHmi_SendNormalTips("区段故障：%s",gn_name(current_section));
	}
}

/****************************************************
函数名:    find_unlock_section
功能描述:  查找待解锁区段
返回值:   待解锁区段索引号
参数:      route_index
作者  :    CY
日期  ：   2011/12/20
****************************************************/
static int16_t find_unlock_section(route_t route_index)
{
	int16_t result = NO_INDEX,i;

	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*通过区段锁闭情况来继续找*/
		for (i =0; i < gr_nodes_count(route_index); i++)
		{
			result = gr_node(route_index,i);
			/*第一个还是处于锁闭状态的区段*/
			/*hjh 2012-12-3 判断条件增加&& (gn_belong_route(result) == route_index)*/
			if (IsTRUE(is_section(result)) && IsTRUE(is_node_locked(result,LT_LOCKED))
				&& (gn_belong_route(result) == route_index))
			{
				break;
			}
			result = NO_INDEX;
		}
	}
	return result;
}

/****************************************************
函数名:    unlock_section
功能描述:  解锁信号点
返回值:    无
参数:      route_index
参数:      node_ordinal
作者  :    CY
日期  ：   2011/12/20
****************************************************/
void unlock_section( route_t route_index,int16_t node_ordinal )
{
	int16_t i,j;
	int16_t node_count = gr_nodes_count(route_index);
	int16_t current_node,section_switch;
	node_t another_signal;
	if (IsTRUE(is_route_exist(route_index)))
	{
		for (i = 0; i < node_count; i++)
		{
			current_node = gr_node(route_index,i);
			/*从当前区段开始解锁*/
			if ( i > node_ordinal)
			{
				/*解锁最后一个区段*/
				if ((i == node_count - 1) && 
					(gn_belong_route(current_node) == route_index))
				{
					/*2014/4/11 LYC 股道和无岔区段*/
					if ((gn_type(current_node) == NT_TRACK) 
						|| (gn_type(current_node) == NT_NON_SWITCH_SECTION))
					{
						/*如果股道或无岔区段的前一区段在另一进路中则不解锁，且把股道或无岔区段加到另一进路中*/
						if (gr_type(route_index) == RT_SHUNTING_ROUTE)
						{
							/*获取股道另一端的信号机*/
							another_signal = gn_forword(gr_direction(route_index),current_node);
							/*另一端的信号机在某条进路中*/
							if ((another_signal != NO_INDEX) && (gn_belong_route(another_signal) != NO_ROUTE)
								&& (gr_type(gn_belong_route(another_signal)) == RT_SHUNTING_ROUTE)
								&& another_signal != gr_start_signal(gn_belong_route(another_signal)))
							{
								sn_belong_route(current_node,gn_belong_route(another_signal));
							}
							else
							{
								reset_node(current_node);
								break;
							}
						}
						/*列车进路将最后的股道或无岔区段解锁*/
						else
						{
							reset_node(current_node);
							break;
						}						
					}
					/*如果是尽头线的话就可以只讲将尽头线解锁了*/
					if ((gn_type(current_node) == NT_STUB_END_SECTION ) || ((gn_type(current_node) == NT_LOCODEPOT_SECTION )))
					{
						reset_node(current_node);
						break;
					}
				}
				/*确认信号点属于本进路*/
				if (gn_belong_route(current_node) == route_index)
				{
					/*通过进路中股道的解锁*/
					if ((gn_type(current_node) == NT_TRACK) && (current_node == gr_last_section(route_index)))
					{
						reset_node(current_node);
					}
				}
				/*进路一直从当前节点开始解锁，直到下一个轨道停止解锁*/
				if ( IsTRUE(is_section(current_node)))
				{
					break;
				}
				/*如果下一个当前信号点是道岔，且道岔在本信号点前方，则停止解锁*/
				if ( (gn_type(current_node) == NT_SWITCH) && 
					(gr_node_index_route(route_index,gn_switch_section(current_node)) > i))
				{
					break;
				}	
			}
			/*确认信号点未解锁，且属于本进路*/
			if (IsTRUE(is_node_locked(current_node,LT_LOCKED)) && 
				(route_index == gn_belong_route(current_node)))
			{
				/*解锁当前道岔区段*/
				if ((i == node_ordinal) && ( NT_SWITCH_SECTION == gn_type(current_node)))
				{
					/*多个道岔在一个区段时需要，如果要解锁区段，则多个道岔都要解锁*/
					for (j = 0; j < MAX_SWITCH_PER_SECTION; j++)
					{
						section_switch = gn_section_switch(current_node,j);
						/*hjh 2014-2-25 只要属于本进路，不用检查其是否锁闭就能解锁*/
						if ((NO_INDEX != section_switch) && 
							(route_index == gn_belong_route(section_switch)))
						{
							reset_node(section_switch);
						}
					}
					/*解锁道岔区段*/
					reset_node (current_node);
				}				

				/*解锁除道岔和道岔区段之外的信号点*/
				if (( NT_SWITCH != gn_type(current_node)) && 
					( NT_SWITCH_SECTION != gn_type(current_node)))
				{
					if (gn_type(current_node) == NT_STUB_END_SECTION )
					{
						if (i == node_ordinal)
						{
							reset_node(current_node);
						}
					}
					else
					{
						reset_node(current_node);
					}
				}			

				/*输出提示信息*/
				if ((IsFALSE(is_node_locked(current_node,LT_LOCKED)))
					&& (( NT_STUB_END_SECTION == gn_type(current_node))
					|| ( NT_LOCODEPOT_SECTION == gn_type(current_node))
					|| ( NT_SWITCH_SECTION == gn_type(current_node))
					|| ( NT_NON_SWITCH_SECTION == gn_type(current_node))))
				{
					/*hjh 2014-2-25 若解锁区段是进路内第一区段则也复位接近区段*/
					if ((current_node == gr_first_section(route_index))
						&& IsFALSE(is_node_locked(gr_approach(route_index),LT_LOCKED)))
					{
						reset_node(gr_approach(route_index));
					}	
					CIHmi_SendDebugTips("%s 已解锁！",gn_name(current_node));
				}
			}			
		}
	}
}

/****************************************************
函数名:    is_section_failure
功能描述:  判断区段是否故障
返回值:     CI_TRUE:	区段故障	
			CI_FALSE:	区段不故障
参数:      route_index
参数:      node_ordinal
作者  :    CY
日期  ：   2011/12/20
****************************************************/
CI_BOOL is_section_failure(route_t route_index, int16_t node_ordinal)
{
	CI_BOOL result = CI_FALSE;
	int16_t current_section = gr_node(route_index,node_ordinal);

	/*判断区段故障*/
	if ((gn_state_machine(current_section) == SCSM_FAULT)
		|| (gn_state_machine(current_section) == SCSM_FAULT_CLEARED)
		|| (gn_state_machine(current_section) == SCSM_FAULT_BEHIND))
	{
		result = CI_TRUE;
	}
	
	return result;
}

/****************************************************
函数名:    search_bar_signal
功能描述:  找到以该点位接近区段的阻拦信号机的索引号
返回值:    该点位接近区段的阻拦信号机的索引号
参数:      route_index
参数:      node_ordinal
作者  :    CY
日期  ：   2011/12/20
****************************************************/
static int16_t search_bar_signal(route_t route_index)
{
	int16_t end_signal = gb_node(gr_end_button(route_index));
	int16_t result = NO_NODE;

	/*参数检查*/
	if(IsFALSE(is_route_exist(route_index)))
	{
		result = NO_NODE;
	}
	/*确认是调车进路*/
	if (RT_SHUNTING_ROUTE == gr_type(route_index))
	{	
		/*本进路末端的单置信号机*/
		if (NT_SINGLE_SHUNTING_SIGNAL == gn_type(end_signal))
		{
			result = end_signal;
		} 
		/*末端的差置和并置信号机*/
		else if ((NT_JUXTAPOSE_SHUNGTING_SIGNAL == gn_type(end_signal)) ||
				 (NT_DIFF_SHUNTING_SIGNAL == gn_type(end_signal)))
		{
			result = gn_another_signal(end_signal);
		}
		/*2013-9-4 hjh 末端是进路信号机时的处理*/
		else if (NT_ROUTE_SIGNAL == gn_type(end_signal))
		{
			result = gn_another_signal(end_signal);
			/*获取到的节点不是信号机*/
			if ((result != NO_NODE) && IsFALSE(is_signal(result)))
			{
				if (gr_direction(route_index) == DIR_UP)
				{
					result = gn_previous(result);
				}
				else
				{
					result = gn_next(result);
				}
			}
		}
		else
		{
			result = NO_NODE;
		}
	}
	
	if (NO_NODE != result)
	{
		/*阻拦信号机必须是本进路前方的进路*/
		if (gn_belong_route(result) != gr_forward(route_index))
		{
			result = NO_NODE;
		}
	}
	return result;
}
/****************************************************
函数名:    search_return_signal
功能描述:  找到以该点位接近区段的折返信号机的索引号
返回值:    该点位接近区段的折返信号机的索引号
参数:      route_index
参数:      node_ordinal
作者  :    CY
日期  ：   2011/12/20
****************************************************/
static int16_t search_return_signal(route_t route_index,CI_BOOL is_all_unlock_route)
{
	int16_t i,j,node = NO_INDEX,start_signal = NO_INDEX,return_signal = NO_INDEX;
	route_t lead_route = NO_INDEX,temp_route = NO_INDEX;

	/*参数检查*/
	if(IsFALSE(is_route_exist(route_index)))
	{
		return_signal = NO_NODE;
	}
	/*确认进路类型*/
	if (RT_SHUNTING_ROUTE == gr_type(route_index))
	{	
		/*全部未解锁进路*/
		if (IsTRUE(is_all_unlock_route))
		{
			temp_route = route_index;
			/*hjh 2013-4-24 多条牵出进路时的折返解锁*/
			for (j = 0; j < MAX_ROUTE; j++)
			{
				/*向后寻找折返进路*/
				lead_route = gr_backward(temp_route);
				if ((lead_route != NO_INDEX) && (RT_SHUNTING_ROUTE == gr_type(lead_route)))
				{
					temp_route = lead_route;
					j = 0;
				}
				else
				{
					lead_route = temp_route;
					break;
				}

			}
			if (lead_route != NO_INDEX)
			{
				start_signal = gr_start_signal(lead_route);
				/*存在部分未解锁进路*/
				if (gn_belong_route(start_signal) != lead_route)
				{
					/*折返信号机在本进路上*/
					for ( i = gr_nodes_count(lead_route) - 1; i > 0 ; i--)
					{
						node = gr_node(lead_route,i);
						/*折返信号机在本进路上*/
						if (IsTRUE(is_signal(node)) 
							&& (gn_belong_route(node) != NO_ROUTE)
							&& (gn_belong_route(node) != lead_route)
							&& (gr_state(gn_belong_route(node)) == RS_SIGNAL_OPENED))
						{
							/*折返信号机的方向和本进路的方向必须是相反的*/
							if (((gn_direction(node) == DIR_DOWN) && (gr_direction(lead_route) == DIR_UP))
								|| ((gn_direction(node) == DIR_UP) && (gr_direction(lead_route) == DIR_DOWN)))
							{
								return_signal = node;
								break;
							}

						}
					}
				}
				/*不存在部分未解锁进路*/
				else
				{
					if ((NT_DIFF_SHUNTING_SIGNAL == gn_type(start_signal)) 
						|| (NT_JUXTAPOSE_SHUNGTING_SIGNAL == gn_type(start_signal))
						|| (NT_ROUTE_SIGNAL == gn_type(start_signal)))
					{
						/*折返信号机不在本进路上*/
						node = gn_another_signal(start_signal);
						/*获取到的节点不是信号机*/
						if ((node != NO_NODE) && IsFALSE(is_signal(node)))
						{
							if (gr_direction(lead_route) == DIR_UP)
							{
								node = gn_previous(node);
							}
							else
							{
								node = gn_next(node);
							}
						}

						if ((node != NO_INDEX)
							&& (gn_belong_route(node) != NO_ROUTE)
							&& (gr_state(gn_belong_route(node) == RS_SIGNAL_OPENED)))
						{
							/*折返信号机的方向和本进路的方向必须是相反的*/
							if (((gn_direction(node) == DIR_DOWN) && (gr_direction(route_index) == DIR_UP))
								|| ((gn_direction(node) == DIR_UP) && (gr_direction(route_index) == DIR_DOWN)))
							{
								return_signal = node;
							}
						}
					}
				}
			}
		}
		/*部分未解锁进路*/
		else
		{
			for ( i = gr_nodes_count(route_index) - 1; i>0 ; i--)
			{
				node = gr_node(route_index,i);
				/*折返信号机在本进路上，折返信号机已经正常关闭*/
				if (IsTRUE(is_signal(node)) 
					&& (gn_belong_route(node) != NO_ROUTE)
					&& (gn_belong_route(node) != route_index)
					&& (gr_state(gn_belong_route(node)) == RS_SK_N_SIGNAL_CLOSED))
				{
					/*折返信号机的方向和本进路的方向必须是相反的*/
					if (((gn_direction(node) == DIR_DOWN) && (gr_direction(route_index) == DIR_UP))
						|| ((gn_direction(node) == DIR_UP) && (gr_direction(route_index) == DIR_DOWN)))
					{
						return_signal = node;
						break;
					}

				}
			}
		}		
	}
	return return_signal;
}

/****************************************************
函数名:    process_successive_route
功能描述:  处理延续进路
返回值:    
参数:      route_t current_route
参数:      route_t successive_route
参数:      index_t current_ordinal
作者  :    CY
日期  ：   2012/3/26
****************************************************/
static void process_successive_route(route_t current_route,route_t successive_route)
{
	CI_BOOL err = CI_FALSE;
	node_t end_signal,last_section,last_switch_section;

	if (successive_route == NO_ROUTE)
	{
		process_warning(ERR_INDEX,"");
		err = CI_TRUE;
	}
	if ((gr_state(successive_route) == RS_FAILURE) || (gr_state(successive_route) == RS_CRASH_INTO_SIGNAL))
	{
		err = CI_TRUE;
	}
	/*检查车列是否冒进信号*/
	if (IsFALSE(err) && (SCS_CLEARED != gn_section_state(gr_first_section(successive_route))) 
		&& (gr_state(successive_route) != RS_SIGNAL_OPENED) && (gr_state(successive_route) != RS_SK_N_SIGNAL_CLOSING) 
		&& (gr_state(successive_route) != RS_SK_N_SIGNAL_CLOSED))
	{
		sr_stop_timer(successive_route);
		sr_state(successive_route,RS_CRASH_INTO_SIGNAL);
		CIHmi_SendNormalTips("冒进信号");
		err = CI_TRUE;
	}
	if (IsFALSE(err))
	{
		/*如果未开始延时计时*/
		if(IsFALSE(is_timer_run(successive_route)))
		{
			end_signal = gb_node(gr_end_button(current_route));
			last_section = gr_forward_section(current_route,gr_node_index_route(current_route,end_signal));
			last_switch_section = gr_backward_section(current_route,gr_node_index_route(current_route,end_signal));
			/*当前的区段是股道，股道占用，进路内方的第一个区段（股道的后方区段）也是占用的*/
			if (((gn_type(last_section) == NT_TRACK) || (gn_type(last_section) == NT_NON_SWITCH_SECTION))
				&& (gn_section_state(last_section) != SCS_CLEARED)
				&& (gn_state_machine(last_section) == SCSM_SELF_OCCUPIED)
				&& (gn_section_state(last_switch_section) != SCS_CLEARED)
				&& ((gn_state_machine(last_switch_section) == SCSM_FRONT_OCCUPIED_1) 
				|| (gn_state_machine(last_switch_section) == SCSM_FRONT_OCCUPIED_2))
				&& (gr_state(successive_route) != RS_SIGNAL_OPENED)
				&& (gr_state(current_route) != RS_AUTO_UNLOCK_FAILURE))
			{
				sr_start_timer(successive_route,MINUTES_3,DTT_UNLOCK);
				sr_state(successive_route,RS_AU_SUCCESSIVE_RELAY);
			}

			///*hjh 2013-1-7只有延续进路的状态是进路已锁闭时才能延时*/
			//if (gr_state(successive_route) == RS_ROUTE_LOCKED)
			//{
			//	section = gr_node(current_route,current_ordinal);
			//	/*当前的区段是股道，股道占用，进路内方的第一个区段（股道的后方区段）也是占用的*/
			//	if ((gn_type(section) == NT_TRACK)
			//		&& (gn_section_state(section) != SCS_CLEARED)
			//		&& (gn_section_state(gr_backward_section(current_route,current_ordinal)) != SCS_CLEARED) )
			//	{
			//		sr_start_timer(successive_route,MINUTES_3);
			//		sr_state(successive_route,RS_AU_SUCCESSIVE_RELAY);
			//	}
			//}		
		}
		else
		{
			/*延时结束*/
			if (IsTRUE(is_complete_timer(successive_route)))
			{
				/*检查延续进路三分钟延时后满足解锁条件*/
				sr_state(successive_route,RS_UNLOCKED);	
				delete_route(successive_route);
			}
			/*延时过程的的检查*/
			else
			{
				/*延续进路延时过程中*/
				if ((gr_state(successive_route) != RS_AU_SUCCESSIVE_RELAY)
					|| IsFALSE(satisfy_signal_condition(successive_route)))
				{
					sr_stop_timer(successive_route);
					sr_state(successive_route,RS_FAILURE);
				}
			}
		}
	}
}

/****************************************************
函数名:    check_succesive_condition
功能描述:  坡道解锁时检查延续进路条件
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/4/16
****************************************************/
static CI_BOOL check_succesive_condition( route_t route_index )
{
	CI_BOOL result = CI_TRUE;
	node_t end_signal,last_section,last_switch_section;
	route_t bw_route;

	FUNCTION_IN;

	/*确定是延续进路*/
	if (IsFALSE(is_successive_route(route_index)))
	{
		result = CI_FALSE;
		process_warning(ERR_OPERATION,gr_name(route_index));
		CIHmi_SendNormalTips("错误办理");
	}

	/*信号已关闭*/
	if ((SGS_H != gn_state(gr_start_signal(route_index)))
		&& (SGS_FILAMENT_BREAK != gn_state(gr_start_signal(route_index)))
		&& (SGS_ERROR != gn_state(gr_start_signal(route_index))))
	{
		result = CI_FALSE;
		process_warning(ERR_SIGNAL_OPEN,gr_name(route_index));
		CIHmi_SendNormalTips("信号未关闭");
	}

	/*本延续进路的状态正在延时*/
	if (RS_AU_SUCCESSIVE_RELAY != gr_state(route_index))
	{
		/*后方接车进路不存在*/
		if (gr_backward(route_index) != NO_INDEX)
		{
			result = CI_FALSE;
			process_warning(ERR_SUCCESSIVE_CANCLE,gr_name(route_index));
			CIHmi_SendNormalTips("接车进路未解锁");
		}		
	}

	/*检查列车顺序进入进路且最后区段已解锁*/
	bw_route = gr_backward(route_index);
	if ((NO_INDEX != bw_route) && (IsTRUE(have_successive_route(bw_route))))
	{
		end_signal = gb_node(gr_end_button(bw_route));
		last_section = gr_forward_section(bw_route,gr_node_index_route(bw_route,end_signal));
		last_switch_section = gr_backward_section(bw_route,gr_node_index_route(bw_route,end_signal));
		/*当前的区段是股道，股道占用，进路内方的第一个区段（股道的后方区段）也是占用的*/
		if (!(((gn_type(last_section) == NT_TRACK) || (gn_type(last_section) == NT_NON_SWITCH_SECTION))
			&& (((gn_section_state(last_section) != SCS_CLEARED) && ((gn_state_machine(last_section) == SCSM_BEHIND_CLEARED_1) || (gn_state_machine(last_section) == SCSM_BEHIND_CLEARED_2)))
			|| ((gn_section_state(last_section) ==  SCS_CLEARED) && (gn_state_machine(last_section) == SCSM_TRACK_UNLOCK)))
			&& IsFALSE(is_node_locked(last_switch_section,LT_LOCKED))))
		{
			result = CI_FALSE;
			process_warning(ERR_SUCCESSIVE_CANCLE,gr_name(route_index));
			CIHmi_SendNormalTips("接车进路未解锁");
		}		
	}

	FUNCTION_OUT;
	return result;
}

/****************************************************
函数名:    command_succesive_route_unlock
功能描述:  坡道解锁过程（命令处理模块调用）
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/4/16
****************************************************/
void command_succesive_route_unlock( route_t route_index )
{
	FUNCTION_IN;

	/*坡道解锁时检查延续进路条件*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*检查条件*/
		if (IsTRUE(satisfy_signal_condition(route_index)) && IsTRUE(check_succesive_condition(route_index)))
		{
			sr_state(route_index,RS_UNLOCKED);	
			delete_route(route_index);
		}
		else
		{
			process_warning(ERR_OPERATION,gr_name(route_index));		
		}		
	}
	FUNCTION_OUT;
}

/****************************************************
函数名:    recieve_route_ums
功能描述:  接车进路的中间道岔解锁
返回值:    
参数:      route_t current_route
作者  :    CY
日期  ：   2012/9/6
****************************************************/
void recieve_route_ums( route_t current_route ) 
{
	index_t ordinal,si;
	node_t timer_switch1,timer_switch2;
	int16_t i;
	node_t end_signal,last_section,last_switch_section;
	
	/*接车进路*/
	end_signal = gb_node(gr_end_button(current_route));
	ordinal = gr_node_index_route(current_route,end_signal);
	last_section = gr_forward_section(current_route,ordinal);
	last_switch_section = gr_backward_section(current_route,ordinal);
	if (IsTRUE(is_out_signal(end_signal)))
	{		
		si = gs_middle_switch_index(end_signal);
		/*检查出站信号机处配置的中间道岔的配置数据*/
		if (si != NO_INDEX)
		{
			/*hjh 2012- 12-19 修改接车进路中间道岔的解锁过程*/
			timer_switch1 = gs_middle_switch(si,0);
			timer_switch2 = gs_middle_switch(si,1);
			if (timer_switch2 != NO_INDEX)
			{
				if (IsTRUE(is_node_locked(gn_switch_section(timer_switch1),LT_LOCKED)) 
					|| IsTRUE(is_node_locked(gn_switch_section(timer_switch2),LT_LOCKED)))
				{
					/*计时判断条件*/
					if (IsFALSE(is_node_timer_run(timer_switch1)) 
						&& IsFALSE(is_node_timer_run(timer_switch2)))
					{
						/*最后区段解锁，车列进入进路*/
						if (((gn_type(last_section) == NT_TRACK) || (gn_type(last_section) == NT_NON_SWITCH_SECTION))
							//&& (gn_section_state(last_section) != SCS_CLEARED)
							//&& ((gn_state_machine(last_section) == SCSM_BEHIND_CLEARED_1) || (gn_state_machine(last_section) == SCSM_BEHIND_CLEARED_2))
							&& (gn_section_state(last_switch_section) == SCS_CLEARED)
							&& IsFALSE(is_node_locked(last_switch_section,LT_LOCKED))
							&& (RSM_TRAIN_IN_ROUTE == gr_state_machine(current_route)))
						{
							sn_start_timer(timer_switch1,MINUTES_3,DTT_UNLOCK);
							sn_start_timer(timer_switch2,MINUTES_3,DTT_UNLOCK);
							if (gr_state(current_route) != RS_MS_UNLOCK_RELAY)
							{
								sr_state(current_route,RS_MS_UNLOCK_RELAY);
							}				
						}
						else
						{
							if ((gr_state(current_route) == RS_MS_UNLOCK_RELAY) && IsTRUE(is_guide_route(current_route)))
							{
								sn_start_timer(timer_switch1,MINUTES_3,DTT_UNLOCK);
								sn_start_timer(timer_switch2,MINUTES_3,DTT_UNLOCK);
							}
						}

					}
					/*计时过程*/
					else if( gr_state(current_route) == RS_MS_UNLOCK_RELAY)
					{
						/*计时结束后解锁进路*/
						if (IsTRUE(is_node_complete_timer(timer_switch1))
							|| IsTRUE(is_node_complete_timer(timer_switch2)))
						{
							sn_stop_timer(timer_switch1);
							sn_stop_timer(timer_switch2);
							/*解锁中间道岔相关的区段*/
							for (i = 0; i < middle_switch_config[si].SectionCount; i++)
							{
								unlock_section(current_route,gr_node_index_route(current_route,gs_middle_section(si,i)));
							}
							/*遍历进路上的区段检查是否已经解锁*/
							if (IsTRUE(check_all_sections_unlock(current_route)))
							{
								sr_state(current_route,RS_UNLOCKED);
								CIHmi_SendNormalTips("进路解锁：%s",gr_name(current_route));
								delete_route(current_route);
							}
						}
						/*计时过程中，车列继续运行时出清的区段立即解锁*/
						else
						{
							if (IsFALSE(is_guide_route(current_route)))
							{
								for ( i = ordinal; i < gr_nodes_count(current_route); i++)
								{
									if (IsTRUE(is_section(gr_node(current_route,i))) && (SCSM_SELF_UNLOCK == gn_state_machine(gr_node(current_route,i))))
									{
										/*解锁本区段*/
										unlock_section(current_route,i);
										/*遍历进路上的区段检查是否已经解锁*/
										if (IsTRUE(check_all_sections_unlock(current_route)))
										{
											sr_state(current_route,RS_UNLOCKED);
											CIHmi_SendNormalTips("进路解锁：%s",gr_name(current_route));
											delete_route(current_route);
											break;
										}
									}
								}
							}					
						}
					}
				}
			}
			/*只有一个中间道岔*/
			else
			{
				if (IsTRUE(is_node_locked(gn_switch_section(timer_switch1),LT_LOCKED)))
				{
					/*计时判断条件*/
					if (IsFALSE(is_node_timer_run(timer_switch1)))
					{
						/*最后区段解锁，车列进入进路*/
						if (((gn_type(last_section) == NT_TRACK) || (gn_type(last_section) == NT_NON_SWITCH_SECTION))
							&& (gn_section_state(last_switch_section) == SCS_CLEARED)
							&& IsFALSE(is_node_locked(last_switch_section,LT_LOCKED))
							&& (RSM_TRAIN_IN_ROUTE == gr_state_machine(current_route)))
						{
							sn_start_timer(timer_switch1,MINUTES_3,DTT_UNLOCK);
							if (gr_state(current_route) != RS_MS_UNLOCK_RELAY)
							{
								sr_state(current_route,RS_MS_UNLOCK_RELAY);
							}				
						}
						else
						{
							if ((gr_state(current_route) == RS_MS_UNLOCK_RELAY) && IsTRUE(is_guide_route(current_route)))
							{
								sn_start_timer(timer_switch1,MINUTES_3,DTT_UNLOCK);
							}
						}

					}
					/*计时过程*/
					else if( gr_state(current_route) == RS_MS_UNLOCK_RELAY)
					{
						/*计时结束后解锁进路*/
						if (IsTRUE(is_node_complete_timer(timer_switch1)))
						{
							sn_stop_timer(timer_switch1);
							/*解锁中间道岔相关的区段*/
							for (i = 0; i < middle_switch_config[si].SectionCount; i++)
							{
								unlock_section(current_route,gr_node_index_route(current_route,gs_middle_section(si,i)));
							}
							/*遍历进路上的区段检查是否已经解锁*/
							if (IsTRUE(check_all_sections_unlock(current_route)))
							{
								sr_state(current_route,RS_UNLOCKED);
								CIHmi_SendNormalTips("进路解锁：%s",gr_name(current_route));
								delete_route(current_route);
							}
						}
						/*计时过程中，车列继续运行时出清的区段立即解锁*/
						else
						{
							if (IsFALSE(is_guide_route(current_route)))
							{
								for ( i = ordinal; i < gr_nodes_count(current_route); i++)
								{
									if (IsTRUE(is_section(gr_node(current_route,i))) && (SCSM_SELF_UNLOCK == gn_state_machine(gr_node(current_route,i))))
									{
										/*解锁本区段*/
										unlock_section(current_route,i);
										/*遍历进路上的区段检查是否已经解锁*/
										if (IsTRUE(check_all_sections_unlock(current_route)))
										{
											sr_state(current_route,RS_UNLOCKED);
											CIHmi_SendNormalTips("进路解锁：%s",gr_name(current_route));
											delete_route(current_route);
											break;
										}
									}
								}
							}					
						}
					}
				}
			}

			//ordinal = gr_node_index_route(current_route,out_signal);
			//backward = gr_backward_section(current_route,ordinal);
			///*出站信号机内方的区段必须是道岔区段，此处的检查为了保证解锁正确*/
			//if (gn_type(backward) == NT_SWITCH_SECTION)
			//{
			//	forward = gr_forward_section(current_route,ordinal);
			//	/*出站信号机外方是无岔区段或股道*/
			//	if ((gn_type(forward) == NT_NON_SWITCH_SECTION) || (gn_type(forward) == NT_TRACK))
			//	{
			//		ordinal = gr_node_index_route(current_route,forward);
			//		forward = gr_forward_section(current_route,ordinal);
			//		/*出站信号机外方的第二个区段必须是道岔区段*/
			//		if (gn_type(forward) == NT_SWITCH_SECTION)
			//		{
			//			/*hjh 2012- 12-19 修改接车进路中间道岔的解锁过程*/
			//			timer_switch1 = gs_middle_switch(si,0);
			//			timer_switch2 = gs_middle_switch(si,1);
			//			/*计时判断条件*/
			//			if (IsFALSE(is_node_timer_run(timer_switch1)) 
			//				&& IsFALSE(is_node_timer_run(timer_switch2)))
			//			{
			//				
			//				if (((gn_type(last_section) == NT_TRACK) || (gn_type(last_section) == NT_NON_SWITCH_SECTION))
			//					&& (gn_section_state(last_section) != SCS_CLEARED)
			//					&& (gn_state_machine(last_section) == SCSM_BEHIND_CLEARED_2)
			//					&& (gn_section_state(last_switch_section) == SCS_CLEARED)
			//					&& IsFALSE(is_node_locked(last_switch_section,LT_LOCKED)))
			//				{
			//					sr_state(current_route,RS_MS_UNLOCK_RELAY);
			//					sn_start_timer(timer_switch1,MINUTES_3);
			//					sn_start_timer(timer_switch2,MINUTES_3);
			//				}
			//			}
			//			/*计时过程*/
			//			else if( gr_state(current_route) == RS_MS_UNLOCK_RELAY)
			//			{
			//				/*计时结束后解锁进路*/
			//				if (IsTRUE(is_node_complete_timer(timer_switch1))
			//					&& IsTRUE(is_node_complete_timer(timer_switch2))	)
			//				{
			//					sn_stop_timer(timer_switch1);
			//					sn_stop_timer(timer_switch2);
			//					sr_state(current_route,RS_UNLOCKED);
			//					delete_route(current_route);								
			//				}
			//				/*计时过程中，车列继续运行时出清的区段立即解锁*/
			//				else
			//				{
			//					for ( i = ordinal; i < gr_nodes_count(current_route); i++)
			//					{
			//						if (IsTRUE(is_section(gr_node(current_route,i))) && (SCSM_SELF_UNLOCK == gn_state_machine(gr_node(current_route,i))))
			//						{
			//							/*解锁本区段*/
			//							unlock_section(current_route,i);
			//							/*hjh 2013-4-3 本区段解锁后检查本进路上的最后一个区段已解锁时删除本进路*/
			//							if (IsFALSE(is_node_locked(gr_last_section(current_route),LT_LOCKED)))
			//							{
			//								sr_state(current_route,RS_UNLOCKED);	
			//								delete_route(current_route);	
			//								break;
			//							}
			//						}
			//					}
			//				}
			//			}							  
			//		}
			//	}
			//}
		}
	}
}

/****************************************************
函数名:    departure_route_ums
功能描述:  发车进路的中间道岔解锁
返回值:    
参数:      route_t current_route
作者  :    CY
日期  ：   2012/9/6
****************************************************/
void departure_route_ums( route_t current_route ) 
{
	node_t out_signal,fw_section,ms;
	index_t ordinal,si,i;
	
	/*发车进路*/
	out_signal = gr_start_signal(current_route);   
	if (IsTRUE(is_out_signal(out_signal)))
	{
		si = gs_middle_switch_index(out_signal);		
		/*检查出站信号机是否设置了中间道岔*/
		if (si != NO_INDEX)
		{			
			if (IsTRUE(is_unlock_middle_switch(out_signal)))
			{
				out_signal = gr_start_signal(current_route); 
				ordinal = gr_node_index_route(current_route,out_signal);
				fw_section = gr_forward_section(current_route,ordinal);
				si = gs_middle_switch_index(out_signal);	
				for (i = 0; i < MAX_MIDDLE_SWITCHES; i++)
				{
					ms = gs_middle_switch(si,i);
					/*出站信号机内方是道岔区段，且是锁闭的，并且是占用状态*/
					if ((ms != NO_INDEX)
						&& (gr_state(current_route) == RS_SK_N_SIGNAL_CLOSED)
						&& (gn_type(fw_section) == NT_SWITCH_SECTION) 
						&& IsTRUE(is_node_locked(fw_section,LT_LOCKED)) 
						&& (gn_section_state(fw_section) != SCS_CLEARED))
					{
						/*hjh 2014-2-14 中间道岔解锁增加检查区段状态*/
						/*如果能找到和出站信号机关联的中间道岔*/
						if (gs_middle_section(si,3) != NO_INDEX)
						{
							if ((SCS_CLEARED == gn_section_state(gs_middle_section(si,0)))
								&& (SCS_CLEARED == gn_section_state(gs_middle_section(si,1)))
								&& (SCS_CLEARED == gn_section_state(gs_middle_section(si,2)))
								&& (SCS_CLEARED == gn_section_state(gs_middle_section(si,3))))
							{
								/*如果道岔是单锁的，则清除中间道岔锁闭标志*/
								if (IsTRUE(is_node_locked(ms,LT_MIDDLE_SWITCH_LOCKED)))
								{
									cn_locked_state(ms,LT_MIDDLE_SWITCH_LOCKED);
								}
								/*清除中间道岔双动道岔的另一动道岔的中间道岔锁闭标志*/
								if (gn_another_switch(ms) != NO_INDEX)
								{
									cn_locked_state(gn_another_switch(ms),LT_MIDDLE_SWITCH_LOCKED);
								}
							}
						}
						else
						{
							if ((SCS_CLEARED == gn_section_state(gs_middle_section(si,0)))
								&& (SCS_CLEARED == gn_section_state(gs_middle_section(si,1)))
								&& (SCS_CLEARED == gn_section_state(gs_middle_section(si,2))))
							{
								/*如果道岔是单锁的，则清除中间道岔锁闭标志*/
								if (IsTRUE(is_node_locked(ms,LT_MIDDLE_SWITCH_LOCKED)))
								{
									cn_locked_state(ms,LT_MIDDLE_SWITCH_LOCKED);
								}
								/*清除中间道岔双动道岔的另一动道岔的中间道岔锁闭标志*/
								if (gn_another_switch(ms) != NO_INDEX)
								{
									cn_locked_state(gn_another_switch(ms),LT_MIDDLE_SWITCH_LOCKED);
								}
							}
						}
					}
					/*进路内方第一区段解锁后才可以解锁*/
					if ((ms != NO_INDEX)
						&& (gr_state(current_route) > RS_ROUTE_LOCKED)
						&& (gn_type(fw_section) == NT_SWITCH_SECTION) 
						&& IsFALSE(is_node_locked(fw_section,LT_LOCKED)) 
						&& (gn_section_state(fw_section) == SCS_CLEARED))
					{
						/*如果道岔是单锁的，则清除中间道岔锁闭标志*/
						if (IsTRUE(is_node_locked(ms,LT_MIDDLE_SWITCH_LOCKED)))
						{
							cn_locked_state(ms,LT_MIDDLE_SWITCH_LOCKED);
						}
						/*清除中间道岔双动道岔的另一动道岔的中间道岔锁闭标志*/
						if (gn_another_switch(ms) != NO_INDEX)
						{
							cn_locked_state(gn_another_switch(ms),LT_MIDDLE_SWITCH_LOCKED);
						}
					}
				}
			}			
		}
	}
}

/****************************************************
函数名:    unlock_middle_switch
功能描述:  解锁中间道岔
返回值:    
参数:      route_t current_route
作者  :    CY
日期  ：   2012/8/6
****************************************************/
void unlock_middle_switch(route_t current_route)
{	
	/*中间道岔解锁都是针对列车进路*/
	if (IsTRUE(is_route_exist(current_route)) && (RS_FAILURE != gr_state(current_route)) 
		&& (RS_AUTO_UNLOCK_FAILURE != gr_state(current_route))
	&& (gr_type(current_route) == RT_TRAIN_ROUTE))
	{
		recieve_route_ums(current_route);
		if (IsTRUE(is_route_exist(current_route)))
		{
			departure_route_ums(current_route);
		}
	}
}

/****************************************************
函数名：   train_location_state_machine
功能描述： 列车位置状态机
返回值：   void
参数：     route_t route_index
作者：	   hejh
日期：     2014/04/17
****************************************************/
void train_location_state_machine(route_t route_index)
{
	int16_t approach_section, first_section, second_section,third_section;

	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		approach_section = gr_approach(route_index);
		first_section = gr_first_section(route_index);
		if (gr_node_index_route(route_index,first_section) != NO_INDEX)
		{
			second_section = gr_forward_section(route_index,gr_node_index_route(route_index,first_section));
		}
		else
		{
			second_section = NO_INDEX;
		}
		if (second_section != NO_INDEX)
		{
			if (gr_node_index_route(route_index,second_section) != NO_INDEX)
			{
				third_section = gr_forward_section(route_index,gr_node_index_route(route_index,second_section));
			}
			else
			{
				third_section = NO_INDEX;
			}
		}				

		/*进路状态正确*/
		if ((RS_SIGNAL_OPENED == gr_state(route_index)) || (RS_SK_N_SIGNAL_CLOSED == gr_state(route_index))
			|| (RS_AUTOMATIC_UNLOCKING == gr_state(route_index)) || (RS_RCR_SUCCESSIVE_RELAY == gr_state(route_index))
			|| (RS_A_SIGNAL_CLOSED == gr_state(route_index)) || (RS_TRACK_POWER_OFF == gr_state(route_index)) 
			|| (RS_FAILURE == gr_state(route_index)) || (RS_AUTO_UNLOCK_FAILURE == gr_state(route_index))
			|| (RS_G_SIGNAL_OPENED == gr_state(route_index)) || (RS_G_SIGNAL_CLOSED == gr_state(route_index))
			|| (RS_SECTION_UNLOCKING == gr_state(route_index)) || (RS_SU_RELAY == gr_state(route_index))
			|| (RS_CRASH_INTO_SIGNAL == gr_state(route_index)))
		{
			/*接近延长*/
			check_approach_added(route_index);

			/*根据不同的状态机执行不同的操作*/
			switch (gr_state_machine(route_index))
			{
				/*1.初始化状态*/
				case RSM_INIT:
					/*始端信号开放，接近区段占用*/
					if (((RS_SIGNAL_OPENED == gr_state(route_index)) || (RS_G_SIGNAL_OPENED == gr_state(route_index)))
						&& (SGS_H != gn_signal_state(gr_start_signal(route_index))) 
						&& (SGS_A != gn_signal_state(gr_start_signal(route_index)))
						&& (SGS_ERROR != gn_signal_state(gr_start_signal(route_index)))
						&& (SGS_FILAMENT_BREAK != gn_signal_state(gr_start_signal(route_index)))
						&& IsFALSE(is_approach_cleared(route_index,CI_TRUE)))
					{
						sr_state_machine(route_index,RSM_TRAIN_APPROACH);
					}
					if ((RT_SHUNTING_ROUTE != gr_type(route_index)) && (IsFALSE(is_approach_cleared(route_index,CI_TRUE))))
					{
						//sr_state_machine(route_index,RSM_TRAIN_APPROACH);
					}
					/*接车进路转引导且车接近*/
					if (IsTRUE(is_guide_route(route_index)) && (ROF_GUIDE_APPROACH == gr_other_flag(route_index)))
					{
						sr_state_machine(route_index,RSM_TRAIN_APPROACH);
					}					

					if (IsTRUE(is_guide_route(route_index)))
					{
						/*引导进路首区段故障，引导延时开放，车列运行*/
						second_section = gr_forward_section(route_index,gr_node_index_route(route_index,gr_first_section(route_index)));
						if (gr_node_index_route(route_index,second_section) != NO_INDEX)
						{
							third_section = gr_forward_section(route_index,gr_node_index_route(route_index,second_section));
							if ((SCS_CLEARED != gn_section_state(gr_first_section(route_index)))
								&& (SCS_CLEARED != gn_section_state(second_section)) 
								&& (SCS_CLEARED != gn_section_state(third_section) && IsFALSE(is_section_fault(third_section))))
							{
								sr_state_machine(route_index,RSM_TRAIN_IN_ROUTE);
							}
						}
						else
						{
							if ((SCS_CLEARED != gn_section_state(gr_first_section(route_index)))
								&& (SCS_CLEARED != gn_section_state(second_section)))
							{
								sr_state_machine(route_index,RSM_TRAIN_IN_ROUTE);
							}
						}
					}

					/*列车异常接近*/
					if (IsTRUE(is_signal_close(gr_start_signal(route_index)))
						&& IsFALSE(is_approach_cleared(route_index,CI_TRUE))
						&& (SCS_CLEARED == gn_section_state(first_section)))
					{
						sr_state_machine(route_index,RSM_TRAIN_ABNORMAL_APPROACH);
					}

					/*列车异常驶入 hjh 2015-8-6*/
					if (IsTRUE(is_signal_close(gr_start_signal(route_index)))
						&& IsFALSE(is_approach_cleared(route_index,CI_TRUE))
						&& (SCS_CLEARED != gn_section_state(first_section)))
					{
						sr_state_machine(route_index,RSM_TRAIN_IN_POSSIBLE);
					}

					break;

				/*2.列车接近*/
				case RSM_TRAIN_APPROACH:
					/*接近区段占用，首区段占用*/
					if (((approach_section == NO_INDEX) || (SCS_CLEARED != gn_section_state(approach_section)))
						&& ((SCS_CLEARED != gn_section_state(first_section)) || (IsTRUE(is_section_fault(first_section)) && IsTRUE(is_guide_route(route_index)))))
					{
						sr_state_machine(route_index,RSM_FIRST_SECTION);
						if (RS_A_SIGNAL_CLOSED == gr_state(route_index))
						{
							sr_state(route_index,RS_CRASH_INTO_SIGNAL);
						}
						break;
					}
					/*hjh 2014-6-13信号非正常关闭，首区段占用(接近区段分路不良时接近延长)*/
					if ((RS_A_SIGNAL_CLOSED == gr_state(route_index))
						&& (SCS_CLEARED != gn_section_state(first_section)))
					{
						sr_state_machine(route_index,RSM_FIRST_SECTION);
						break;
					}
					/*引导进路接近区段空闲且曾占用过，首区段占用*/
					if (IsTRUE(is_guide_route(route_index))
						&& ((SCS_CLEARED != gn_section_state(first_section)) || (IsTRUE(is_section_fault(first_section)))))
					{
						sr_state_machine(route_index,RSM_G_FAULT_FIRST);
						break;
					}
					/*接近区段占用，首区段空闲，第二区段占用(不考虑两点故障的情况)*/
					if (((approach_section == NO_INDEX) || (SCS_CLEARED != gn_section_state(approach_section)))
						&& IsTRUE(is_guide_route(route_index))
						&& (SCS_CLEARED == gn_section_state(first_section))
						&& (SCS_CLEARED != gn_section_state(second_section))
						&& (IsFALSE(is_section_fault(second_section))))
					{
						sr_state_machine(route_index,RSM_G_FAULT_FIRST);
						break;
					}
					/*接近区段占用，首区段空闲，第二区段占用*/
					if (IsFALSE(is_approach_cleared(route_index,CI_TRUE))
						&& (SCS_CLEARED == gn_section_state(first_section))
						&& (SCS_CLEARED != gn_section_state(second_section)))
					{
						/*车列异常驶入*/						
						if (RT_SHUNTING_ROUTE == gr_type(route_index))
						{
							if (NT_SWITCH_SECTION == gn_type(second_section))
							{
								sr_state_machine(route_index,RSM_TRAIN_IN_POSSIBLE);
							}
						}
						else
						{
							sr_state_machine(route_index,RSM_TRAIN_IN_POSSIBLE);
						}
					}

					break;

				/*3.列车压入首区段*/
				case RSM_FIRST_SECTION:
					/*接近区段占用，首区段占用，第二区段占用*/
					if (((approach_section == NO_INDEX) || (SCS_CLEARED != gn_section_state(approach_section)))
						&& (SCS_CLEARED != gn_section_state(first_section)) //&& IsFALSE(is_section_fault(first_section))
						&& ((SCS_CLEARED != gn_section_state(second_section)) || (IsTRUE(is_section_fault(first_section) && IsTRUE(is_guide_route(route_index))))))
					{
						sr_state_machine(route_index,RSM_SECOND_SECTION);
						break;
					}
					/*接近区段出清，首区段占用*/
					if (((approach_section == NO_INDEX) || (SCS_CLEARED == gn_section_state(approach_section)))
						&& (SCS_CLEARED != gn_section_state(first_section)))
					{
						sr_state_machine(route_index,RSM_TRAIN_IN_ROUTE);
						break;
					}
					break;

				/*4.列车压入第二区段*/
				case RSM_SECOND_SECTION:
					/*接近区段出清，首区段占用，第二区段占用*/
					if (((approach_section == NO_INDEX) || (SCS_CLEARED == gn_section_state(approach_section)))
						&& (SCS_CLEARED != gn_section_state(first_section)) && IsFALSE(is_section_fault(first_section))
						&& (SCS_CLEARED != gn_section_state(second_section)))
					{
						sr_state_machine(route_index,RSM_TRAIN_IN_ROUTE);	
						break;
					}
					/*接近区段不是道岔区段，接近区段占用，首区段出清，第二区段占用*/
					if (((approach_section == NO_INDEX) || ((SCS_CLEARED != gn_section_state(approach_section)) && (NT_SWITCH_SECTION != gn_type(approach_section))))
						&& (SCS_CLEARED == gn_section_state(first_section)) && IsFALSE(is_section_fault(first_section))
						&& (SCS_CLEARED != gn_section_state(second_section)))
					{
						sr_state_machine(route_index,RSM_TRAIN_IN_ROUTE);	
						break;
					}
					break;

				/*5.引导进路首区段分路不良*/
				case RSM_G_FAULT_FIRST:
					/*首区段空闲，第二区段占用，第三区段占用*/
					if ((SCS_CLEARED != gn_section_state(second_section))
						&& (SCS_CLEARED != gn_section_state(gr_forward_section(route_index,gr_node_index_route(route_index,second_section))))
						 && IsFALSE(is_section_fault(gr_forward_section(route_index,gr_node_index_route(route_index,second_section))))
						&& IsTRUE(is_guide_route(route_index)))
					{
						sr_state_machine(route_index,RSM_TRAIN_IN_ROUTE);
						break;
					}
					break;
				/*6.列车异常接近*/
				case RSM_TRAIN_ABNORMAL_APPROACH:
					/*接近区段空闲*/
					if (IsTRUE(is_approach_cleared(route_index,CI_TRUE)))
					{
						sr_state_machine(route_index,RSM_INIT);
					}
					else
					{
						/*车列异常驶入*/
						if (SCS_CLEARED != gn_section_state(first_section))
						{
							sr_state_machine(route_index,RSM_TRAIN_ABNORMAL_IN_ROUTE);
						}
					}

					break;
				/*7.疑似车类驶入*/
				case RSM_TRAIN_IN_POSSIBLE:
					/*接近区段占用，首区段空闲，第二区段空闲*/
					if (IsFALSE(is_approach_cleared(route_index,CI_TRUE))
						&& (SCS_CLEARED == gn_section_state(first_section))
						&& (SCS_CLEARED == gn_section_state(second_section)))
					{
						sr_state_machine(route_index,RSM_TRAIN_APPROACH);
					}
					/*接近区段占用，首区段占用，第二区段占用*/
					if (IsFALSE(is_approach_cleared(route_index,CI_TRUE))
						&& (SCS_CLEARED != gn_section_state(first_section))
						&& (SCS_CLEARED != gn_section_state(second_section)))
					{
						/*车列异常驶入*/
						sr_state_machine(route_index,RSM_TRAIN_ABNORMAL_IN_ROUTE);
					}

					/*接近区段占用，首区段空闲，第二区段占用，第三区段占用*/
					if (IsFALSE(is_approach_cleared(route_index,CI_TRUE))
						&& (SCS_CLEARED == gn_section_state(first_section))
						&& (SCS_CLEARED != gn_section_state(second_section))
						&& (SCS_CLEARED != gn_section_state(third_button)))
					{
						/*车列异常驶入*/						
						if (RT_SHUNTING_ROUTE == gr_type(route_index))
						{
							if (NT_SWITCH_SECTION == gn_type(third_button))
							{
								sr_state_machine(route_index,RSM_TRAIN_IN_POSSIBLE);
							}
						}
						else
						{
							sr_state_machine(route_index,RSM_TRAIN_IN_POSSIBLE);
						}
					}
					break;

				default:
					break;
			}

			/*延续进路判断车列是否压入接车进路*/
			if (IsTRUE(have_successive_route(route_index)) 
				&& ((RSM_TRAIN_IN_ROUTE == gr_state_machine(route_index))
				|| (RSM_SECOND_SECTION == gr_state_machine(route_index))))
			{
				if ((gr_forward(route_index) != NO_INDEX) && (gr_other_flag(gr_forward(route_index)) == ROF_SUCCESSIVE))
				{
					sr_other_flag(gr_forward(route_index),ROF_SUCCESSIVE_DELAY);
				}
			}
		}
	}
}

/****************************************************
函数名：   unlock_special_switch
功能描述： 解锁特殊防护道岔
返回值：   void
参数：     route_t current_route
作者：	   hejh
日期：     2014/04/28
****************************************************/
void unlock_special_switch(route_t current_route)
{
	int16_t temp,i;
	node_t switch_index,section_index;	

	if (IsTRUE(is_route_exist(current_route)))
	{
		/*检查是否存在特殊防护道岔*/
		if ((temp = gs_special(current_route,SPT_SPECIAL_SWTICH)) != NO_INDEX)
		{
			section_index = gs_special_switch_unlock_section(temp);
			if ((section_index != NO_INDEX) && IsFALSE(is_node_locked(section_index,LT_LOCKED)))
			{
				for (i = 0;i < MAX_SPECIAL_SWITCHES; i++)
				{
					/*获取特殊防护道岔的索引号*/
					switch_index = gs_special_switch_index(temp,i);
					if (switch_index != NO_INDEX)
					{
						if (IsTRUE(is_node_locked(switch_index,LT_MIDDLE_SWITCH_LOCKED)))
						{
							cn_locked_state(switch_index,LT_MIDDLE_SWITCH_LOCKED);
						}
						/*检查道岔是否双动道岔*/
						if (gn_another_switch(switch_index) != NO_INDEX)
						{
							cn_locked_state(gn_another_switch(switch_index),LT_MIDDLE_SWITCH_LOCKED);
						}
					}
				}
			}				
		}		
	}
}

