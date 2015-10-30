/***************************************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.
文件名:  utility_function.c
作者:    CY
版本 :   1.0	
创建日期:2011/12/1
用途:    系统常用工具函数
历史修改记录:         
2012/11/28 V1.2.1 hjh
	1.发现gr_forward_section中错误调用函数gn_next和gn_previous，应调用函数gn_forward
	2.gn_forward中未处理道岔反位时的下一节点，所有道岔直接返回直股节点，已修正
	3.sn_state中对接近区段被占用的时间判断错误，已修正
2012/12/7 V1.2.1 hjh
	all_switchs_is_normal添加延续进路中不判断2#道岔在定位的条件
2012/12/19 V1.2.1 hjh
	is_approach_cleared添加中间道岔部分的区段
2013/1/25 V1.2.1 hjh
	gn_another_signal条件加入进路信号机
2013/1/28 V1.2.1 hjh
	1.增加函数gs_special_switch_index和gs_special_switch_location，用于特殊防护道岔的相关操作	
	2.delete_route中增加删除特殊防护道岔的锁闭标志
2013/3/7 V1.2.1 hjh
	send_switch_command中command == 0，修改为command == SWS_NORMAL
2013/3/15 V1.2.1 hjh
	delete_route增加删除延续进路的接车部分时也删除6G上的调车进路
2013/3/20 V1.2.1 hjh
	1.gn_forward和gn_backward中未处理道岔反位时的下一节点，所有道岔直接返回直股节点，已修正
	2.gr_forward_section中搜索算法错误，已修正
2013/3/22 V1.2.1 hjh		
	delete_route两个咽喉向同一股道办理调车进路，再取消某一进路时会使股道错误解锁的BUG
2013/4/3 V1.2.1 hjh	
	delete_route设置带有中间道岔的进路的前后关系，为了解决延续部分延时过程中办理坡道解锁检查接车进路已解锁的条件
2013/4/22 V1.2.1 hjh
	删除2013年3月15日增加的内容
2013/7/31 V1.2.1 LYC
	删除了is_protective_switch函数中当道岔不是防护道岔时提示错误信息
2013/9/27 V1.2.1 LYC 
	is_approach_cleared增加了检查通过进路的发车进路接近区段时，还需检查接车进路的接近区段是否占用
2014/2/14 V1.2.1 hjh
	增加gs_middle_allow_reverse函数
2014/2/24 V1.2.1 hjh mantis:3375
	is_exceed_limit_section增加联锁表区段表中不在进路上的区段是侵限区段
2014/2/25 V1.2.1 hjh mantis:3349
	修改is_approach_cleared函数，增加参数special_flag，表示是否检查特殊联锁
2014/2/28 V1.2.1 hjh 
	修改send_node_timer和is_node_complete_time,否则会出现倒计时溢出的问题。
2014/3/10 V1.2.1 hjh 
	所有区段从占用恢复至空闲时均延时3s
***************************************************************************************/
#include "utility_function.h"
#include "semi_auto_block.h"
#include "global_data.h"
#include "error_process.h"
#include "global_data.h"
#include "util/app_config.h"
#include "platform/platform_api.h"

char_t route_name[30];
char_t* route_state_name[] =
{
	"进路错误",
	"进路已选出",
	"联锁条件检查成功",
	"正在转岔",
	"道岔转换到位",
	"选排一致",
	"信号检查条件成功",
	"信号检查成功",
	"进路锁闭锁闭条件满足",
	"进路已锁闭",	
	"信号开放模块正在开放信号",
	"信号已开放",
	"信号保持正在正常关闭信号",
	"信号保持正常关闭信号",
	"信号保持正在非正常关闭信号", 
	"信号保持正在转换信号",
	"正在自动解锁",
	"进路已解锁",
	"人工延时解锁模块正在延时解锁",
	"人工延时解锁模块正在关闭引导信号",
	"信号非正常关闭",
	"区段故障解锁正在关闭信号",
	"进路故障",
	"轨道停电故障",
	"进路自动解锁故障",
	"车列冒进信号",
	"引导进路已锁闭",
	"正在开放引导信号",
	"引导信号已开放", 
	"正在关闭引导信号",
	"引导信号已关闭",
	"延续进路部分延时解锁",
	"人工延时解锁模块正在非正常关闭信号",
	"取消进路正在非正常关闭信号",
	"信号重复开放模块正在开放信号",
	"正在区段故障解锁",
	"正在区段故障解锁延时",
	"进路建立失败",
	"中间道岔正在延时解锁",
	"信号开放模块延时开放信号"
};

/****************************************************
函数名:    input_addrees_to_node
功能描述:  从输入地址找出对应的信号节点索引号
返回值:    信号节点索引号
参数:      address 输入地址
作者  :    CY
日期  ：   2011/12/1
****************************************************/
int16_t input_addrees_to_node(uint16_t address)
{
	int16_t result = 0;
	
    result = input_address[GATEWAY(address)][EEU(address)][NODE(address)];
	return result;
}

/****************************************************
函数名:    gb_node
功能描述:  返回按钮所属的信号节点的索引号
返回值:    
参数:      button 按钮索引号
作者  :    CY
日期  ：   2011/12/5
****************************************************/
int16_t gb_node( int16_t button_index )
{
	int16_t result = NO_INDEX,i;

	/*参数检查*/
	if((button_index >= TOTAL_SIGNAL_NODE) && (button_index < TOTAL_NAMES))
	{
		/*列车或调车按钮返回按钮所属的信号点*/
		/*result = buttons[button_index - TOTAL_SIGNAL_NODE].node_index;*/

		for (i = 0; i < TOTAL_BUTTONS; i++)
		{
			if (buttons[i].button_index == button_index)
			{
				result = buttons[i].node_index;
				break;
			}
		}
	}
	else
	{
		/*其它按钮返回原来的索引号*/
		if((button_index >= 0) && (button_index < TOTAL_NAMES))
		{	
			result = button_index;
		}
		else
		{
			result = NO_INDEX;
		}
	}

	return result;
}

/****************************************************
函数名:    gb_type
功能描述:  获取按钮类型
返回值:    
参数:      button_index
作者  :    CY
日期  ：   2011/12/20
****************************************************/
EN_button_type gb_type(int16_t button_index)
{
	EN_button_type result = BT_ERROR;
	int16_t i;

	/*参数检查*/
	if((button_index >= TOTAL_SIGNAL_NODE) && (button_index < TOTAL_NAMES))
	{
		/*列车或调车按钮返回按钮所属的信号点*/
		//result = (EN_button_type)buttons[button_index - TOTAL_SIGNAL_NODE].button_type;
		for (i = 0; i < TOTAL_BUTTONS; i++)
		{
			if (buttons[i].button_index == button_index)
			{
				result = (EN_button_type)buttons[i].button_type;
				break;
			}
		}
	}
	return result;
}

/****************************************************
函数名:    is_follow_switch
功能描述:  检查道岔是否为带动道岔
返回值:    
参数:      state 道岔描述
作者  :    CY
日期  ：   2011/12/7
****************************************************/
CI_BOOL is_follow_switch( route_t route_index, int16_t switch_ordinal )
{
	uint16_t state = 0;
	CI_BOOL result = CI_FALSE;

	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE)
		|| (NO_INDEX == routes[route_index].ILT_index)
		|| (switch_ordinal < 0) 
		|| (switch_ordinal >= ILT[routes[route_index].ILT_index].switch_count))
	{
		process_warning(ERR_INDEX,"");
		return CI_FALSE;
	}
	else
	{
		/*查看该道岔在联锁表中的状态*/
		state = ILT[routes[route_index].ILT_index].switches[switch_ordinal].state;	
		/*判断是否带动道岔*/
		if (0x55 == ((state>>8)&0xFF))
		{
			result = CI_TRUE;
		}
		else
		{
			result = CI_FALSE;
		}
	}
	return result;
}

/****************************************************
函数名:    is_protective_switch
功能描述:  检查道岔是否为防护道岔
返回值:    
参数:      state 道岔描述
作者  :    CY
日期  ：   2011/12/7
****************************************************/
CI_BOOL is_protective_switch( route_t route_index, int16_t switch_ordinal )
{
	uint16_t state = 0;
	CI_BOOL result = CI_FALSE;

	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) 
		|| (NO_INDEX == routes[route_index].ILT_index)
		|| (switch_ordinal < 0) 
		|| (switch_ordinal >= ILT[routes[route_index].ILT_index].switch_count))
	{
		result = CI_FALSE;
	}
	else
	{
		/*查看该道岔在联锁表中的状态*/
		state = ILT[routes[route_index].ILT_index].switches[switch_ordinal].state;	
		/*判断是否防护道岔*/
		if ( PROTECTIVE_SWITCH(state))
		{
			result = CI_TRUE;
		}
		else
		{
			result = CI_FALSE;
		}
	}
	return result;
}

/****************************************************
函数名:    is_ILT_protective_switch
功能描述:  检查道岔是否为防护道岔
返回值:    
参数:      state 道岔描述
作者  :    CY
日期  ：   2011/12/7
****************************************************/
CI_BOOL is_ILT_protective_switch(int16_t ILT_index, int16_t switch_ordinal)
{
	uint16_t state = 0;
	CI_BOOL result = CI_FALSE;

	/*参数检查*/
	if ((ILT_index < 0) || (ILT_index >= TOTAL_ILT))
	{
		process_warning(ERR_INDEX,"");
		result = CI_FALSE;
	}
	else
	{
		/*查看该道岔在联锁表中的状态*/
		state = ILT[ILT_index].switches[switch_ordinal].state;	
		/*判断是否防护道岔*/
		if ( PROTECTIVE_SWITCH(state))
		{
			result = CI_TRUE;
		}
		else
		{
			result = CI_FALSE;
		}
	}	
	return result;
}

/****************************************************
函数名:    gn_switch_state
功能描述:  获取当前道岔的位置
返回值:    
参数:      
作者  :    CY
日期  ：   2011/12/12
****************************************************/
EN_switch_state gn_switch_state( int16_t node_index )
{
	int16_t another = 0;
	EN_switch_state result;

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
		result = SWS_ERROR;
	}
	else
	{
		another = (int16_t)(signal_nodes[node_index].property);
		/*双动道岔两动是否一致检查*/
		if ((another != NO_INDEX) && (signal_nodes[another].state != signal_nodes[node_index].state))
		{
			process_warning(ERR_NODE_DATA,"");
			result = SWS_ERROR;
		}
		else
		{
			result = (EN_switch_state)(signal_nodes[node_index].state);
		}	
	}
	return result;
}

/****************************************************
函数名:    gn_switch_section
功能描述:  从道岔获取道岔所在的道岔区段
返回值:    
参数:      index
作者  :    CY
日期  ：   2011/12/12
****************************************************/
int16_t gn_switch_section( int16_t node_index )
{
	int16_t result = 0;

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
		result = NO_INDEX;
	}
	else
	{
		/*获取道岔所在的道岔区段*/
		result = (int16_t)(signal_nodes[node_index].additional_property);
	}
	return result;
}


/****************************************************
函数名:    gn_section_state
功能描述:  获取轨道占用状态
返回值:    
参数:      index
作者  :    CY
日期  ：   2011/12/12
****************************************************/
EN_section_state gn_section_state( int16_t node_index )
{
	EN_section_state result = SCS_ERROR,DG1_state = SCS_ERROR,DG2_state = SCS_ERROR,DG3_state = SCS_ERROR;
	int16_t i;
	node_t DG1 = NO_INDEX,DG2 = NO_INDEX,DG3 = NO_INDEX;

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE)
		|| IsFALSE(is_section(node_index)))
	{
		process_warning(ERR_INDEX,"");
		result = SCS_ERROR;
	}
	else
	{
		/*hjh 20150420 道岔区段一送多受*/
		if (signal_nodes[node_index].property == NO_INDEX)
		{
			result = (EN_section_state)(signal_nodes[node_index].state);
		}
		else
		{
			result = (EN_section_state)(signal_nodes[node_index].state);
			for ( i = 0; i < TOTAL_SIGNAL_NODE; i++)
			{
				if ((i != node_index) && (signal_nodes[i].type == NT_SWITCH_SECTION) 
					&& ((int16_t)signal_nodes[i].property == node_index))
				{
					if (DG1 == NO_INDEX)
					{
						DG1 = i;
						DG1_state = (EN_section_state)(signal_nodes[i].state);
					}
					else
					{
						if (DG2 == NO_INDEX)
						{
							DG2 = i;
							DG2_state = (EN_section_state)(signal_nodes[i].state);
						}
						else
						{
							DG3 = i;
							DG3_state = (EN_section_state)(signal_nodes[i].state);
						}
					}
				}
			}

			if (result != SCS_ERROR)
			{
				if ((DG1 != NO_INDEX) && (DG2 == NO_INDEX))
				{
					if ((result == SCS_CLEARED) && (DG1_state == SCS_CLEARED))
					{
						result = SCS_CLEARED;
					}
					else
					{
						result = SCS_OCCUPIED;
					}
				}
				if ((DG1 != NO_INDEX) && (DG2 != NO_INDEX) && (DG3 == NO_INDEX))
				{
					if ((result == SCS_CLEARED) && (DG1_state == SCS_CLEARED) && (DG2_state == SCS_CLEARED))
					{
						result = SCS_CLEARED;
					}
					else
					{
						result = SCS_OCCUPIED;
					}
				}
				if ((DG1 != NO_INDEX) && (DG2 != NO_INDEX) && (DG3 != NO_INDEX))
				{
					if ((result == SCS_CLEARED) && (DG1_state == SCS_CLEARED) && (DG2_state == SCS_CLEARED) && (DG3_state == SCS_CLEARED))
					{
						result = SCS_CLEARED;
					}
					else
					{
						result = SCS_OCCUPIED;
					}
				}
			}			
		}
	}
	return result;
}
/****************************************************
函数名:    gn_signal_state
功能描述:  获取信号机显示状态
返回值:    
参数:      index
作者  :    CY
日期  ：   2011/12/12
****************************************************/
EN_signal_state gn_signal_state( int16_t node_index )
{
	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
		return SGS_ERROR;
	}
	return (EN_signal_state)(signal_nodes[node_index].state);
}

/****************************************************
函数名:    sn_signal_state
功能描述:  设置信号机状态
返回值:    
参数:      int16_t node_index
参数:      EN_signal_state ss
作者  :    CY
日期  ：   2012/3/21
****************************************************/
void sn_signal_state( int16_t node_index, EN_signal_state ss )
{
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
		return ;
	}
	signal_nodes[node_index].state = ss;
}

/****************************************************
函数名:    sn_signal_expect_state
功能描述:  设置信号机预期的状态
返回值:    
参数:      node_index
参数:      ss

作者  :    hjh
日期  ：   2012/3/7
****************************************************/
void sn_signal_expect_state( int16_t node_index, EN_signal_state ss )
{
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
		return ;
	}
	signal_nodes[node_index].expect_state = ss;
}

/****************************************************
函数名：   sn_state_machine
功能描述： 设置节点的状态机
返回值：   void
参数：     int16_t node_index
参数：     EN_unlock_state ss
作者：	   hejh
日期：     2013/04/17
****************************************************/
void sn_state_machine( int16_t node_index, EN_state_machine ss )
{
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
		return ;
	}
	signal_nodes[node_index].state_machine = ss;
}

/****************************************************
函数名:    sn_signal_history_state
功能描述:  设置信号机的历史状态
返回值:    
参数:      node_index
参数:      ss

作者  :    hjh
日期  ：   2012/3/22
****************************************************/
void sn_signal_history_state( int16_t node_index, EN_signal_state ss )
{
	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
		return ;
	}
	/*参数检查*/
	if (ss == SGS_FILAMENT_BREAK)
	{
		/*设置信号机的历史状态*/
		signal_nodes[node_index].history_state = (signal_nodes[node_index].history_state & 0x0000FFFF) | (ss << 16);
	}
	else
	{
		/*设置信号机的历史状态*/
		signal_nodes[node_index].history_state = (signal_nodes[node_index].history_state & 0xFFFF0000) | ss;
	}
}

/****************************************************
函数名：   sn_history_state
功能描述： 设置节点的历史状态
返回值：   void
参数：     int16_t node_index
参数：     EN_signal_state ss
作者：	   hejh
日期：     2014/06/11
****************************************************/
void sn_history_state( int16_t node_index, int32_t ss )
{
	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
		return ;
	}
	else
	{
		signal_nodes[node_index].history_state = ss;
	}
}

/****************************************************
函数名:    clear_section_history_state
功能描述:  清除此进路上区段的历史状态
返回值:    
参数:      route_t route_index
作者  :    hejh
日期  ：   2012/11/26
****************************************************/
void clear_section_history_state(route_t route_index)
{
	int16_t i;
	node_t node_index = NO_INDEX;
	
	/*参数有效性检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		for (i = 0; i < gr_nodes_count(route_index); i++)
		{
			node_index = gr_node(route_index,i);
			/*区段清除历史状态*/
			if (IsTRUE(is_section(node_index)))
			{
				signal_nodes[node_index].history_state = 0u;
			}
		}
	}
}

/****************************************************
函数名:    is_node_locked
功能描述:  获取节点锁闭状态
返回值:    
参数:      index
作者  :    CY
日期  ：   2011/12/12
****************************************************/
CI_BOOL is_node_locked( int16_t node_index, uint32_t state )
{
	CI_BOOL result = CI_FALSE;
	int16_t i;

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
		return LT_LOCKED;
	}
	/*获取进路锁闭状态*/
	if ((state & ROUTE_LOCKED_MASK) == LT_LOCKED)
	{
		if ((signal_nodes[node_index].locked_flag & ROUTE_LOCKED_MASK) == LT_LOCKED)
		{
			result = CI_TRUE;
		}
	}
	/*获取道岔锁闭状态*/
	if ((state & SWITCH_CLOSED_MASK) == LT_SWITCH_CLOSED)
	{
		if ((signal_nodes[node_index].locked_flag & SWITCH_CLOSED_MASK) == LT_SWITCH_CLOSED)
		{
			result = CI_TRUE;
		}
	}
	/*获取道岔单锁状态*/
	if ((state & SWITCH_SIGNLE_LOCKED_MASK) == LT_SWITCH_SIGNLE_LOCKED)
	{
		if ((signal_nodes[node_index].locked_flag & SWITCH_SIGNLE_LOCKED_MASK) == LT_SWITCH_SIGNLE_LOCKED)
		{
			result = CI_TRUE;
		}
	}
	/*获取咽喉区锁闭状态*/
	if ((state & SWITCH_THROAT_LOCKED_MASK) == LT_SWITCH_THROAT_LOCKED)
	{
		if (gn_type(node_index) == NT_SWITCH)
		{
			if ((signal_nodes[node_index].locked_flag & SWITCH_THROAT_LOCKED_MASK) == LT_SWITCH_THROAT_LOCKED)
			{
				result = CI_TRUE;
			}
		}
		else
		{
			for (i = 0; i < TOTAL_SIGNAL_NODE; i ++)
			{
				if ((gn_type(i) == NT_SWITCH) && (gn_throat(i) == gn_throat(node_index)))
				{
					if ((signal_nodes[i].locked_flag & SWITCH_THROAT_LOCKED_MASK) == LT_SWITCH_THROAT_LOCKED)
					{
						result = CI_TRUE;
					}
					break;
				}
			}
		}
	}
	/*获取中间道岔锁闭状态*/
	if ((state & MIDDLE_SWITCH_LOCKED_MASK) == LT_MIDDLE_SWITCH_LOCKED)
	{
		if ((signal_nodes[node_index].locked_flag & MIDDLE_SWITCH_LOCKED_MASK) == LT_MIDDLE_SWITCH_LOCKED)
		{
			result = CI_TRUE;
		}
	}
	
	return result;
}

/****************************************************
函数名:    gn_used_state
功能描述:  获取节点征用状态
返回值:    
参数:      index
作者  :    CY
日期  ：   2011/12/12
****************************************************/
CI_BOOL gn_used_state( int16_t node_index )
{
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
		return CI_TRUE;
	}
	return signal_nodes[node_index].used;
}

/****************************************************
函数名:    gn_another_switch
功能描述:  获取双动道岔的另一动
返回值:    返回NO_INDEX说明时单动道岔
参数:      index
作者  :    CY
日期  ：   2011/12/12
****************************************************/
int16_t gn_another_switch( int16_t node_index )
{
	int16_t result = 0;

	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
		result = NO_INDEX;
	}
	else
	{
		result = (int16_t)(signal_nodes[node_index].property);
	}
	return result;
}

/****************************************************
函数名:    gn_another_signal
功能描述:  获取和本并置或差置信号机关联的另一架信号机
返回值:    
参数:      node_index
作者  :    CY
日期  ：   2011/12/26
****************************************************/
int16_t gn_another_signal(int16_t node_index)
{
	int16_t result = 0;

	/*参数检查*/
	if (node_index < 0 || node_index >= TOTAL_SIGNAL_NODE )
	{
		process_warning(ERR_INDEX,"");
		result = NO_INDEX;
	}
	else
	{
		/*获取信号机条件检查*/
		if ((NT_JUXTAPOSE_SHUNGTING_SIGNAL != gn_type(node_index)) 
			&& (NT_DIFF_SHUNTING_SIGNAL != gn_type(node_index))
			&& (gn_type(node_index) != NT_OUT_SHUNTING_SIGNAL) 
			&& (gn_type(node_index) != NT_OUT_SIGNAL)
			&& (gn_type(node_index) != NT_ROUTE_SIGNAL)
			&& (gn_type(node_index) != NT_TRAIN_END_BUTTON))
		{
			result = NO_INDEX;
		}
		else
		{
			result = (int16_t)(signal_nodes[node_index].property);
		}

	}
	return result;
}

/****************************************************
函数名:    gn_throat
功能描述:  获取信号点所属咽喉
返回值:    
参数:      node_index
作者  :    CY
日期  ：   2011/12/13
****************************************************/
uint8_t gn_throat(int16_t node_index)
{
	uint8_t result;

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
		result = SCS_ERROR;
	}
	else
	{
		result = signal_nodes[node_index].throat;
	}
	return result;
}

/****************************************************
函数名:    gr_type
功能描述:  获取进路类型
返回值:    
参数:      index
作者  :    CY
日期  ：   2011/12/12
****************************************************/
EN_route_type gr_type( route_t route_index )
{
	EN_route_type result;

	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) || (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,gr_name(route_index));

		result = RT_ERROR;
	}
	else
	{
		/*返回进路类型*/
		result = (EN_route_type)(ILT[routes[route_index].ILT_index].route_kind);
	}
	return result;
}

/****************************************************
函数名:    gr_state
功能描述:  获取进路状态
返回值:    
参数:      index
作者  :    CY
日期  ：   2011/12/12
****************************************************/
EN_route_state gr_state( route_t route_index )
{
	EN_route_state result;

	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE)
		|| (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
		result = RS_ERROR;
	}
	else
	{
		/*返回进路状态*/
		result = (EN_route_state)(routes[route_index].state);
	}
	return result;
}

/****************************************************
函数名：   gr_state_machine
功能描述： 获取进路的状态机
返回值：   EN_route_state_machine
参数：     route_t route_index
作者：	   hejh
日期：     2014/04/17
****************************************************/
EN_route_state_machine gr_state_machine( route_t route_index )
{
	EN_route_state_machine result;

	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE)
		|| (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
		result = RSM_FAULT;
	}
	else
	{
		/*返回进路状态*/
		result = (EN_route_state_machine)(routes[route_index].state_machine);
	}
	return result;
}

/****************************************************
函数名:    sr_state
功能描述:  设置进路状态
返回值:    
参数:      index
参数:      state
作者  :    CY
日期  ：   2011/12/12
****************************************************/
void sr_state_fun( route_t route_index,EN_route_state state )
{
	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) 
		|| (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		/*设置进路状态*/
		routes[route_index].state = state;
	}	
}

/****************************************************
函数名：   sr_state_machine
功能描述： 设置进路的状态机
返回值：   void
参数：     route_t route_index
参数：     EN_route_state_machine ss
作者：	   hejh
日期：     2014/04/17
****************************************************/
void sr_state_machine( route_t route_index,EN_route_state_machine ss )
{
	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) 
		|| (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		/*设置进路状态*/
		routes[route_index].state_machine = ss;
	}	
}

/****************************************************
函数名：   sr_other_flag
功能描述： 设置进路的特殊标志
返回值：   void
参数：     route_t route_index
参数：     EN_route_other_flag rof
作者：	   hejh
日期：     2014/04/17
****************************************************/
void sr_other_flag( route_t route_index,EN_route_other_flag rof )
{
	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) 
		|| (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		/*设置特殊标志*/
		routes[route_index].other_flag = rof;
	}	
}

/****************************************************
函数名：   gr_other_flag
功能描述： 获取进路的特殊标志
返回值：   EN_route_other_flag
参数：     route_t route_index
作者：	   hejh
日期：     2014/04/18
****************************************************/
EN_route_other_flag gr_other_flag(route_t route_index)
{
	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) 
		|| (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
		return ROF_ERROR;
	}
	else
	{
		return routes[route_index].other_flag;
	}
}

/****************************************************
函数名：   clear_other_flag
功能描述： 清除进路的特殊标志
返回值：   void
参数：     route_t route_index
作者：	   hejh
日期：     2014/04/17
****************************************************/
void clear_other_flag( route_t route_index )
{
	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) 
		|| (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		/*清除特殊标志*/
		routes[route_index].other_flag = ROF_ERROR;
	}	
}

/****************************************************
函数名:    is_guide_route
功能描述:  判断是否引导引路
返回值:    是引导进路返回TRUE
参数:      route_index

作者  :    hjh
日期  ：   2012/3/29
****************************************************/
CI_BOOL is_guide_route( route_t route_index )
{
	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) 
		|| (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
		return CI_FALSE;
	}
	/*返回判断结果*/
	else
	{
		if ((routes[route_index].other_flag == ROF_GUIDE)
			||(routes[route_index].other_flag == ROF_GUIDE_APPROACH))
		{
			return CI_TRUE;
		}
		else
		{
			return CI_FALSE;
		}
	}
}

/****************************************************
函数名:    gr_start_signal
功能描述:  获取始端信号
返回值:    
参数:      index
作者  :    CY
日期  ：   2011/12/12
****************************************************/
int16_t gr_start_signal( route_t route_index )
{
	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) 
		|| (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
		return NO_INDEX;
	}
	else
	{
		/*返回进路始端信号*/
		return gb_node(ILT[routes[route_index].ILT_index].start_button);
	}	
}

/****************************************************
函数名:    gr_forward
功能描述:  获取前方进路
返回值:    
参数:      index
作者  :    CY
日期  ：   2011/12/12
****************************************************/
route_t gr_forward( route_t route_index )
{
	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) 
		||( NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
		return NO_INDEX;
	}
	/*返回前方进路索引号*/
	return routes[route_index].forward_route;
}

/****************************************************
函数名:    gr_backward
功能描述:  获取本进路后方进路
返回值:    
参数:      index
作者  :    CY
日期  ：   2011/12/12
****************************************************/
route_t gr_backward( route_t route_index )
{
	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) 
		|| (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
		return NO_INDEX;
	}
	/*获取本进路后方进路*/
	return routes[route_index].backward_route;
}

/****************************************************
函数名:    send_signal_command
功能描述:  设置信号开放命令
返回值:    
参数:      index
参数:      command
作者  :    CY
日期  ：   2011/12/12
****************************************************/
void send_signal_command( int16_t node_index,uint32_t command )
{
	uint16_t getway,EEU,id;
	route_t route;

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
		return ;
	}
#ifdef WIN32
	CIEeu_SendNodeIndexCmd(node_index,command);
#endif	
	route = gn_belong_route(node_index);
	/*参数检查*/
	if ((route != NO_INDEX) && IsTRUE(is_route_exist(route)))
	{
		/*表示器索引号检查*/
//		if (ILT[routes[route].ILT_index].indicator != NO_INDEX)
//		{
//			/*发送命令*/
//			if (command == SGS_H)
//			{
//#ifdef WIN32
//				CIEeu_SendNodeIndexCmd(ILT[routes[route].ILT_index].indicator,SIO_NO_SIGNAL);
//#endif	
//			}
//			else
//			{
//#ifdef WIN32
//				CIEeu_SendNodeIndexCmd(ILT[routes[route].ILT_index].indicator,SIO_HAS_SIGNAL);
//#endif	
//			}
//		}
	}	
	getway = (signal_nodes[node_index].output_address >> 8) & 0xFF;
	EEU = (signal_nodes[node_index].output_address >> 2) & 0x3F;
	id = signal_nodes[node_index].output_address  & 0x03;
	commands[getway][EEU] = (commands[getway][EEU] & (~(0xFFFF << (id * 16)))) | ((command << id*16) & (0xFFFF << (id * 16)));
}

/****************************************************
函数名:    send_command
功能描述:  发送命令
返回值:    
参数:      node_t node
参数:      uint16_t cmd_code
作者  :    CY
日期  ：   2012/3/28
****************************************************/
void send_command( node_t node, uint16_t cmd_code ) 
{
	uint16_t getway,EEU,id;
	if ((node < 0) ||( node >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
		return ;
	}
#ifdef WIN32
	CIEeu_SendNodeIndexCmd(node,cmd_code);
#endif	
	getway = (signal_nodes[node].output_address >> 8) & 0xFF;
	EEU = (signal_nodes[node].output_address >> 2) & 0x3F;
	id = signal_nodes[node].output_address  & 0x03;
	commands[getway][EEU] = (commands[getway][EEU] & (~(0xFFFF << (id * 16)))) | ((cmd_code << id*16) & (0xFFFF << (id * 16)));
}

/****************************************************
函数名:    is_exist_route
功能描述:  检查是否存在该进路
返回值:    
参数:      route_index
作者  :    CY
日期  ：   2011/12/13
****************************************************/
CI_BOOL is_route_exist(route_t route_index)
{
	CI_BOOL result = CI_FALSE;

	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE))
	{
		process_warning(ERR_INDEX,"");
		result = CI_FALSE;
	}
	/*查看进路是否存在*/
	if ( NO_INDEX != routes[route_index].ILT_index)
	{
		result = CI_TRUE;
	}
	return result;	
}

/****************************************************
函数名:    gr_nodes_count
功能描述:  获取进路中的信号点总数
返回值:    
参数:      route_index
作者  :    CY
日期  ：   2011/12/13
****************************************************/
int16_t gr_nodes_count(route_t route_index)
{
	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) 
		|| (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
		return NO_INDEX;
	}
	/*获取进路中的信号点总数*/
	return ILT[routes[route_index].ILT_index].nodes_count;	
}

/****************************************************
函数名:    gr_switches_count
功能描述:  获取进路中的道岔总数
返回值:    
参数:      route_index
作者  :    CY
日期  ：   2011/12/13
****************************************************/
int16_t gr_switches_count(route_t route_index)
{
	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) 
		|| (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
		return NO_INDEX;
	}
	/*获取进路中的道岔总数*/
	return ILT[routes[route_index].ILT_index].switch_count;	
}

/****************************************************
函数名:    gr_conflict_signals_count
功能描述:  获取进路中信号节点总数
返回值:    
参数:      route_index
作者  :    CY
日期  ：   2011/12/13
****************************************************/
int16_t gr_conflict_signals_count(route_t route_index)
{
	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE)
		|| (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
		return NO_INDEX;
	}
	/*获取进路中信号节点总数*/
	return ILT[routes[route_index].ILT_index].signal_count;	
}

/****************************************************
函数名:    gr_sections_count
功能描述:  获取进路中区段总数
返回值:    
参数:      route_index
作者  :    CY
日期  ：   2011/12/13
****************************************************/
int16_t gr_sections_count(route_t route_index)
{
	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE)
		|| (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
		return NO_INDEX;
	}
	/*获取进路中区段总数*/
	return ILT[routes[route_index].ILT_index].track_count;	
}

/****************************************************
函数名:    gr_node
功能描述:  获取进路中的某个信号节点
返回值:    
参数:      route_index  进路索引号
参数:      node_ordinal  进路中的信号点序号
作者  :    CY
日期  ：   2011/12/13
****************************************************/
int16_t gr_node(route_t route_index, int16_t node_ordinal)
{
	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE )
		||( NO_INDEX == routes[route_index].ILT_index)
		|| (node_ordinal < 0) 
		|| (node_ordinal >= ILT[routes[route_index].ILT_index].nodes_count))
	{
		process_warning(ERR_INDEX,"");
		return NO_INDEX;
	}
	/*获取进路中的某个信号节点*/
	return ILT[routes[route_index].ILT_index].nodes[node_ordinal];	
}

/****************************************************
函数名:    gr_switch
功能描述:  获取进路中的某个道岔
返回值:    
参数:      route_index
参数:      switch_ordinal
作者  :    CY
日期  ：   2011/12/13
****************************************************/
int16_t gr_switch(route_t route_index, int16_t switch_ordinal)
{
	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE)
		|| (NO_INDEX == routes[route_index].ILT_index)
		|| (switch_ordinal < 0) 
		|| (switch_ordinal >= ILT[routes[route_index].ILT_index].switch_count))
	{
		process_warning(ERR_INDEX,"");
		return NO_INDEX;
	}
	/*获取进路中的某个道岔*/
	return ILT[routes[route_index].ILT_index].switches[switch_ordinal].index;	
}

/****************************************************
函数名:    gr_conflict_signal
功能描述:  获取本进路的某个敌对信号
返回值:    
参数:      route_index
参数:      signal_ordinal
作者  :    CY
日期  ：   2011/12/13
****************************************************/
int16_t gr_conflict_signal(route_t route_index, int16_t signal_ordinal)
{
	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) 
		|| (NO_INDEX == routes[route_index].ILT_index)
		|| (signal_ordinal < 0) 
		|| (signal_ordinal >= ILT[routes[route_index].ILT_index].signal_count))
	{
		process_warning(ERR_INDEX,"");
		return NO_INDEX;
	}
	/*获取本进路的某个敌对信号*/
	return ILT[routes[route_index].ILT_index].signals[signal_ordinal].index;	
}



/****************************************************
函数名:    gr_next_section
功能描述:  获取该进路中以当前节点开始的下一个区段
返回值:    
参数:      route_index
参数:      node_ordinal
作者  :    CY
日期  ：   2011/12/20
****************************************************/
int16_t gr_forward_section(route_t route_index,int16_t node_ordinal)
{
	int16_t i,j,fw_node = NO_INDEX;
	int16_t result = NO_INDEX;

	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) 
		|| (NO_INDEX == routes[route_index].ILT_index)
		|| (node_ordinal < 0)
		|| (node_ordinal >= ILT[routes[route_index].ILT_index].nodes_count))
	{
		process_warning(ERR_INDEX,"");
		result = NO_INDEX;
	}
	else
	{
		for (i = node_ordinal + 1; i < ILT[routes[route_index].ILT_index].nodes_count; i++)
		{
			/*检查节点是否为区段*/
			if (IsTRUE(is_section( ILT[routes[route_index].ILT_index].nodes[i])))
			{
				result = ILT[routes[route_index].ILT_index].nodes[i];
				break;
			}
		}

		/*当前节点时进路上最后一个区段*/
		if (result == NO_INDEX)
		{		
			/*进路方向为下行时的处理*/
			if (gr_direction(route_index) == DIR_DOWN)
			{
				/*获取进路上最后一个节点的前方节点*/
				fw_node = gn_forword(gr_direction(route_index),gr_node(route_index,gr_nodes_count(route_index) - 1));
				for (j = 0; j < TOTAL_SIGNAL_NODE; j++)
				{
					/*前一节点为空*/
					if (fw_node == NO_INDEX)
					{
						break;
					}
					else
					{
						/*判断下个节点如果是信号机或其他则继续下个节点寻找*/
						if ((IsTRUE(is_signal(fw_node)))
							|| ((IsFALSE(is_section(fw_node)) &&  gn_type(fw_node) != NT_SWITCH)))
						{
							fw_node = gn_next(fw_node);
						}
						/*判断下个节点如果是区段则返回区段的索引号*/
						if ((fw_node != NO_INDEX)&&(IsTRUE(is_section(fw_node))))
						{
							result = fw_node;
							break;
						}
						/*判断下个节点如果是道岔则返回道岔所在区段的索引号*/
						if((fw_node != NO_INDEX)&&(gn_type(fw_node) == NT_SWITCH))
						{
							result = gn_switch_section(fw_node);
							break;
						}
					}
				}
			}
			/*进路方向为上行时的处理*/
			if (gr_direction(route_index) == DIR_UP)
			{
				/*获取进路上最后一个节点的前方节点*/
				fw_node = gn_forword(gr_direction(route_index),gr_node(route_index,gr_nodes_count(route_index) - 1));
				for (j = 0; j < TOTAL_SIGNAL_NODE; j++)
				{
					/*前一节点为空*/
					if (fw_node == NO_INDEX)
					{
						break;
					}
					else
					{
						/*判断下个节点如果是信号机或其他则继续下个节点寻找*/
						if ((IsTRUE((is_signal(fw_node))) 
							|| (IsFALSE(is_section(fw_node)) &&  gn_type(fw_node) != NT_SWITCH)))
						{
							fw_node = gn_previous(fw_node);
						}
						/*判断下个节点如果是区段则返回区段的索引号*/
						if ((fw_node != NO_INDEX)&&(IsTRUE(is_section(fw_node))))
						{
							result = fw_node;
						}
						/*判断下个节点如果是道岔则返回道岔所在区段的索引号*/
						if((fw_node != NO_INDEX)&&(gn_type(fw_node) == NT_SWITCH))
						{
							result = gn_switch_section(fw_node);
						}
					}
				}
			}
		}

		/*若找到的区段索引号和当前区段索引号一致则返回-1*/
		if (result == gr_node(route_index,node_ordinal))
		{
			result = NO_INDEX;
		}
	}	
	return result;
}

/****************************************************
函数名:    gr_previous_section
功能描述:  获取前一个区段
返回值:    
参数:      route_index
参数:      node_ordinal
作者  :    CY
日期  ：   2011/12/26
****************************************************/
int16_t gr_backward_section(route_t route_index,int16_t node_ordinal)
{
	int16_t i;
	int16_t result = NO_INDEX;

	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) 
		|| (NO_INDEX == routes[route_index].ILT_index)
		||( node_ordinal < 0) 
		|| (node_ordinal > ILT[routes[route_index].ILT_index].nodes_count))
	{
		process_warning(ERR_INDEX,"");
		result = NO_INDEX;
	}
	else
	{
		for (i = node_ordinal - 1; i >= 0; i--)
		{
			/*前一个节点如果是区段则直接返回区段索引号*/
			if (IsTRUE(is_section( ILT[routes[route_index].ILT_index].nodes[i])))
			{
				result = ILT[routes[route_index].ILT_index].nodes[i];
				break;
			}
		}
		if (NO_INDEX == i)
		{
			/*返回该进路接近区段的索引号*/
			result = gr_approach(route_index);
		}
	}
	return result;
}

/****************************************************
函数名:    gr_section
功能描述:  获取进路中的某个区段
返回值:    
参数:      route_index
参数:      section_ordinal
作者  :    CY
日期  ：   2011/12/13
****************************************************/
int16_t gr_section(route_t route_index,int16_t section_ordinal)
{
	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) 
		|| (NO_INDEX == routes[route_index].ILT_index)
		|| (section_ordinal < 0) 
		|| (section_ordinal >= ILT[routes[route_index].ILT_index].track_count))
	{
		process_warning(ERR_INDEX,"");
		return NO_INDEX;
	}
	/*获取进路中的某个区段*/
	return ILT[routes[route_index].ILT_index].tracks[section_ordinal].index;
}

/****************************************************
函数名:    gr_start_button
功能描述:  获取进路始端按钮
返回值:    
参数:      route_index
作者  :    CY
日期  ：   2011/12/13
****************************************************/
int16_t gr_start_button(route_t route_index)
{
	int16_t result = 0;

	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) 
		|| (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
		result = NO_INDEX;
	}
	else
	{
		/*获取进路始端按钮*/
		result = ILT[routes[route_index].ILT_index].start_button;	
	}
	return result;
}
/****************************************************
函数名:    gr_change_button
功能描述:  获取进路的第一个变更按钮
返回值:    
参数:      route_index
作者  :    CY
日期  ：   2011/12/13
****************************************************/
int16_t gr_change_button(route_t route_index)
{
	int16_t result = 0;

	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) 
		|| (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
		result = NO_INDEX;
	}
	else
	{
		/*获取进路的第一个变更按钮*/
		result = ILT[routes[route_index].ILT_index].change_button;
	}
	return result;
}

/****************************************************
函数名:    gr_change_button1
功能描述:  获取进路的第二个变更按钮
返回值:    
参数:      route_index
作者  :    CY
日期  ：   2011/12/13
****************************************************/
int16_t gr_change_button1(route_t route_index)
{
	int16_t result = 0;

	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) 
		|| (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
		result = NO_INDEX;
	}
	else
	{
		/*获取进路的第二个变更按钮*/
		result = ILT[routes[route_index].ILT_index].change_button1;
	}
	return result;
}

/****************************************************
函数名:    gr_end_button
功能描述:  获取进路终端按钮
返回值:    
参数:      route_index
作者  :    CY
日期  ：   2011/12/13
****************************************************/
int16_t gr_end_button(route_t route_index)
{
	int16_t result = 0;

	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) 
		|| (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
		result = NO_INDEX;
	}
	else
	{
		/*获取进路终端按钮*/
		result = ILT[routes[route_index].ILT_index].end_button;
	}
	return result;
}

/****************************************************
函数名:    gr_direction
功能描述:  获取进路方向
返回值:    
参数:      route_index
作者  :    CY
日期  ：   2011/12/13
****************************************************/
EN_route_direction gr_direction(route_t route_index)
{
	EN_route_direction result;

	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE)
		|| (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
		/*错误的方向*/
		result = DIR_ERROR ;
	}
	else
	{
		/*获取进路方向*/
		result = (EN_route_direction)(gn_direction(gb_node(ILT[routes[route_index].ILT_index].start_button)));
	}
	return result;
}

/****************************************************
函数名:    gr_face_train_signal
功能描述:  获取进路的迎面列车敌对信号
返回值:    
参数:      route_index
作者  :    CY
日期  ：   2011/12/13
****************************************************/
int16_t gr_face_train_signal(route_t route_index)
{
	int16_t result = 0;

	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) 
		|| (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
		result = NO_INDEX;
	}
	else
	{
		/*获取进路的迎面列车敌对信号*/
		result = ILT[routes[route_index].ILT_index].face_train_signal;
	}
	return result;
}

/****************************************************
函数名:    gr_face_shunting_signal
功能描述:  获取迎面调车敌对
返回值:    
参数:      route_index
作者  :    CY
日期  ：   2011/12/13
****************************************************/
int16_t gr_face_shunting_signal(route_t route_index)
{
	int16_t result = 0;

	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) 
		|| (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
		result = NO_INDEX;
	}
	else
	{
		/*获取迎面调车敌对*/
		result = ILT[routes[route_index].ILT_index].face_shunting_signal;
	}
	return result;
}

/****************************************************
函数名:    gr_switch_location
功能描述:  获取进路中某个道岔要求的位置
返回值:    
参数:      route_index
参数:      switch_ordinal
作者  :    CY
日期  ：   2011/12/13
****************************************************/
EN_switch_state gr_switch_location(route_t route_index,int16_t switch_ordinal)
{
	uint16_t state;
	EN_switch_state result = SWS_ERROR;

	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE )
		|| (NO_INDEX == routes[route_index].ILT_index)
		|| (switch_ordinal < 0) 
		|| (switch_ordinal >= ILT[routes[route_index].ILT_index].switch_count))
	{
		process_warning(ERR_INDEX,"");
		result = SWS_ERROR;
	}
	else
	{
		state = ILT[routes[route_index].ILT_index].switches[switch_ordinal].state;
		if ( SWITCH_NORMAL(state))
		{
			result = SWS_NORMAL;
		}	
		else
		{
			result = SWS_REVERSE;	
		}			
	}
	return result;
}
/****************************************************
函数名:    is_switch_in_route
功能描述:  判断道岔是否在进路中
返回值:    
参数:      route_index
参数:      node_index
作者  :    CY
日期  ：   2011/12/13
****************************************************/
CI_BOOL is_switch_in_route(route_t route_index, int16_t node_index)
{
	int16_t i;
	int16_t nodes_count;
	CI_BOOL result= CI_FALSE;

	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE)
		|| (NO_INDEX == routes[route_index].ILT_index)
		|| (node_index < 0) ||( node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
		result = CI_FALSE;
	}
	else
	{
		nodes_count = ILT[routes[route_index].ILT_index].nodes_count;
		for(i = 0; i < nodes_count; i++)
		{
			/*参数检查*/
			if (node_index == ILT[routes[route_index].ILT_index].nodes[i])
			{
				break;
			}
		}
		/*判断道岔是否在进路中*/
		if (i == nodes_count)
		{
			result = CI_FALSE;
		}			
		else
		{
			result = CI_TRUE;
		}			
	}
	return result;
}

/****************************************************
函数名:    gr_conflict_signal_condition
功能描述:  是否为条件敌对的信号
返回值:    
参数:      route_index
参数:      signal_ordinal
作者  :    CY
日期  ：   2011/12/13
****************************************************/
CI_BOOL gr_conflict_signal_condition(route_t route_index, int16_t signal_ordinal)
{
	CI_BOOL result = CI_FALSE;

	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) ||
		 (NO_INDEX == routes[route_index].ILT_index) ||
		 (signal_ordinal < 0) || 
		 (signal_ordinal >= ILT[routes[route_index].ILT_index].signal_count))
	{
		process_warning(ERR_INDEX,"");
		result = CI_FALSE;
	}
	else
	{
		/*判断是否为条件敌对的信号*/
		if ( NO_INDEX == ILT[routes[route_index].ILT_index].signals[signal_ordinal].conditon_switch.index)
		{
			result = CI_FALSE;
		}			
		else
		{
			result = CI_TRUE;
		}			
	}
	return result;
}

/****************************************************
函数名:    gr_conflic_signal_switch
功能描述:  条件敌对信号检查的道岔索引号
返回值:    
参数:      route_index
参数:      signal_ordinal
作者  :    CY
日期  ：   2011/12/13
****************************************************/
int16_t gr_conflict_signal_switch(route_t route_index,int16_t signal_ordinal)
{
	int16_t result = 0;

	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) ||
		(NO_INDEX == routes[route_index].ILT_index) ||
		(signal_ordinal < 0) || 
		(signal_ordinal >= ILT[routes[route_index].ILT_index].signal_count))
	{
		process_warning(ERR_INDEX,"");
		result = NO_INDEX;
	}
	else
	{
		/*条件敌对信号检查的道岔索引号*/
		result = ILT[routes[route_index].ILT_index].signals[signal_ordinal].conditon_switch.index;
	}
	return result;
}

/****************************************************
函数名:    gr_conflic_signal_switch_location
功能描述:  条件敌对信号要求检查的道岔位置
返回值:    
参数:      route_index
参数:      signal_ordinal
作者  :    CY
日期  ：   2011/12/13
****************************************************/
EN_switch_state gr_conflict_signal_switch_location(route_t route_index,int16_t signal_ordinal)
{
	uint16_t state;
	EN_switch_state result;

	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) ||
		(NO_INDEX == routes[route_index].ILT_index) ||
		(signal_ordinal < 0) || 
		(signal_ordinal >= ILT[routes[route_index].ILT_index].signal_count))
	{
		process_warning(ERR_INDEX,"");
		result = SWS_NORMAL;
	}
	else
	{
		state = ILT[routes[route_index].ILT_index].signals[signal_ordinal].conditon_switch.state;
		if ( (state & 0xFF) == 0x55)
		{
			result = SWS_NORMAL;
		}
		else
		{
			result = SWS_REVERSE;	
		}
	}
	return result;
}

/****************************************************
函数名:    gr_conflict_signal_type
功能描述:  条件敌对信号的类型
返回值:    
参数:      route_index
参数:      signal_ordinal
作者  :    CY
日期  ：   2011/12/13
****************************************************/
EN_conflict_signal_type gr_conflict_signal_type(route_t route_index,int16_t signal_ordinal)
{
	EN_conflict_signal_type result;

	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) ||
		(NO_INDEX == routes[route_index].ILT_index) ||
		(signal_ordinal < 0) || 
		(signal_ordinal >= ILT[routes[route_index].ILT_index].signal_count))
	{
		process_warning(ERR_INDEX,"");
		result = SST_TRAIN;
	}
	else
	{
		/*返回条件敌对信号的类型*/
		result = (EN_conflict_signal_type)(ILT[routes[route_index].ILT_index].signals[signal_ordinal].conflict_signal_type);
	}
	return result;
}
/****************************************************
函数名:    gr_section_condition
功能描述:  判断进路中的区段是否为有条件的检查
返回值:    
参数:      route_index
参数:      section_ordinal
作者  :    CY
日期  ：   2011/12/13
****************************************************/
CI_BOOL gr_section_condition(route_t route_index,int16_t section_ordinal)
{
	CI_BOOL result = CI_FALSE;

	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) ||
		(NO_INDEX == routes[route_index].ILT_index) ||
		(section_ordinal < 0) || 
		(section_ordinal >= ILT[routes[route_index].ILT_index].track_count))
	{
		process_warning(ERR_INDEX,"");
		result = CI_FALSE;
	}
	else
	{
		/*判断进路中的区段是否为有条件的检查*/
		if ( NO_INDEX == ILT[routes[route_index].ILT_index].tracks[section_ordinal].conditon_switch.index)
		{
			result = CI_FALSE;
		}
		else
		{
			result = CI_TRUE;
		}
	}
	return result;
}

/****************************************************
函数名:    gr_section_switch
功能描述:  获取进路中区段检查条件的道岔索引号
返回值:    
参数:      route_index
参数:      section_ordinal
作者  :    CY
日期  ：   2011/12/13
****************************************************/
int16_t gr_section_condition_switch(route_t route_index,int16_t section_ordinal)
{
	int16_t result = 0;

	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) ||
		(NO_INDEX == routes[route_index].ILT_index) ||
		(section_ordinal < 0) || 
		(section_ordinal >= ILT[routes[route_index].ILT_index].track_count))
	{
		process_warning(ERR_INDEX,"");
		result = NO_INDEX;
	}
	else
	{
		/*获取进路中区段检查条件的道岔索引号*/
		result = ILT[routes[route_index].ILT_index].tracks[section_ordinal].conditon_switch.index;
	}
	return result;
}

/****************************************************
函数名:    gr_section_switch_location
功能描述:  获取进路中有条件检查区段要求检查的道岔位置
返回值:    
参数:      route_index
参数:      section_ordinal
作者  :    CY
日期  ：   2011/12/13
****************************************************/
EN_switch_state gr_section_condition_switch_location(route_t route_index, int16_t section_ordinal)
{
	uint16_t state;
	EN_switch_state result;

	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) ||
		(NO_INDEX == routes[route_index].ILT_index) ||
		(section_ordinal < 0) || 
		(section_ordinal >= ILT[routes[route_index].ILT_index].track_count))
	{
		process_warning(ERR_INDEX,"");
		result = SWS_NORMAL;
	}
	else
	{
		/*获取进路中有条件检查区段要求检查的道岔位置*/
		state = ILT[routes[route_index].ILT_index].tracks[section_ordinal].conditon_switch.state;
		if ( (state & 0xFF) == 0x55)
		{
			result = SWS_NORMAL;
		}
		else
		{
			result = SWS_REVERSE;	
		}
	}
	return result;
}

/****************************************************
函数名:    sn_used_flag
功能描述:  设置征用标志
返回值:    
参数:      node_index
作者  :    WSP
日期  ：   2011/12/13
****************************************************/
void sn_used_state( int16_t node_index, CI_BOOL state )
{
	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		/*设置征用标志*/
		signal_nodes[node_index].used = state;
	}

}

/****************************************************
函数名:    gn_belong_route
功能描述:  获取信号节点所在的进路索引
返回值:    在进路中则返回进路索引号，无则返回NO_INDEX
参数:      node_index
作者  :    WSP
日期  ：   2011/12/14
****************************************************/
route_t gn_belong_route(int16_t node_index)
{
	route_t result = -1;

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		/*获取信号节点所在的进路索引*/
		result = (signal_nodes[node_index].belong_route);
	}
	return result;
}
/****************************************************
函数名:    gn_type
功能描述:  获取信号点类型
返回值:    
参数:      index
作者  :    CY
日期  ：   2011/12/12
****************************************************/
EN_node_type gn_type(int16_t node_index)
{
	EN_node_type result = NT_NO;

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		/*获取信号点类型*/
		result = (EN_node_type)(signal_nodes[node_index].type);
	}
	return result;
}

/****************************************************
函数名:    gn_previous
功能描述:  获取前一个节点
返回值:    
参数:      index
作者  :    CY
日期  ：   2011/12/12
****************************************************/
int16_t gn_previous(int16_t node_index)
{
	int16_t result = NO_INDEX;

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		/*获取前一个节点*/
		result = (signal_nodes[node_index].previous_node_index);
	}
	return result;
}
/****************************************************
函数名:    gn_backword
功能描述:  按照方向，获取当前节点后方的节点
返回值:    
参数:      index
作者  :    CY
日期  ：   2011/12/12
****************************************************/
int16_t gn_backword(EN_node_direction dir, int16_t node_index)
{
	EN_node_direction nd;
	int16_t index = NO_INDEX;
	route_t i,route_index = NO_INDEX;
	EN_switch_state switch_location = SWS_ERROR;

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
		index =  NO_INDEX;
	}
	/*判断信号点不是道岔*/
	if(gn_type(node_index) != NT_SWITCH)
	{
		/*根据信号点方向返回当前节点后方的节点*/
		if (dir == DIR_DOWN)
		{
			index = signal_nodes[node_index].previous_node_index;
		}
		else
		{
			index = signal_nodes[node_index].next_node_index;
		}
	}
	else 
	{
		/*判断信号点方向*/
		nd = gn_direction(node_index);
		if(dir == DIR_DOWN)
		{
			/*根据道岔方向返回当前节点后方的节点*/
			if((nd == DIR_LEFT_DOWN) || (nd == DIR_LEFT_UP))
			{
				/*获取节点所在进路索引号*/
				route_index = gn_belong_route(node_index);
				/*不存在进路号直接返回直股节点*/
				if (route_index == NO_INDEX)
				{
					index =  signal_nodes[node_index].next_node_index;
				}
				else
				{
					/*hjh 2012-11-28 存在进路号则找出该道岔在进路中要求的位置*/
					for (i = 0; i < gr_switches_count(route_index); i++)
					{
						if ((node_index == gr_switch(route_index,i))
							|| (gn_another_switch(node_index) == gr_switch(route_index,i)))
						{
							switch_location = gr_switch_location(route_index,i);
							break;
						}
					}
					/*进路要求的位置在定位则返回直股节点*/
					if (switch_location == SWS_NORMAL)
					{
						index =  signal_nodes[node_index].next_node_index;
					}
					/*进路要求的位置在反位则返回弯股节点*/
					else
					{
						index =  signal_nodes[node_index].reverse_node_index;
					}
				}
			}
			/*根据道岔方向返回当前节点后方的节点*/
			if((nd == DIR_RIGHT_DOWN) || (nd == DIR_RIGHT_UP))
			{
				index =  signal_nodes[node_index].previous_node_index;
			}
		}
		/*判断信号点方向*/
		if(dir == DIR_UP)
		{
			/*根据道岔方向返回当前节点后方的节点*/
			if((nd == DIR_LEFT_DOWN) || (nd == DIR_LEFT_UP))
			{
				index =  signal_nodes[node_index].previous_node_index;
			}
			/*根据道岔方向返回当前节点后方的节点*/
			if((nd == DIR_RIGHT_DOWN) || (nd == DIR_RIGHT_UP))
			{
				/*获取节点所在进路索引号*/
				route_index = gn_belong_route(node_index);
				/*不存在进路号直接返回直股节点*/
				if (route_index == NO_INDEX)
				{
					index =  signal_nodes[node_index].next_node_index;
				}
				else
				{
					/*hjh 2012-11-28 存在进路号则找出该道岔在进路中要求的位置*/
					for (i = 0; i < gr_switches_count(route_index); i++)
					{
						if ((node_index == gr_switch(route_index,i))
							|| (gn_another_switch(node_index) == gr_switch(route_index,i)))
						{
							switch_location = gr_switch_location(route_index,i);
							break;
						}
					}
					/*进路要求的位置在定位则返回直股节点*/
					if (switch_location == SWS_NORMAL)
					{
						index =  signal_nodes[node_index].next_node_index;
					}
					/*进路要求的位置在反位则返回弯股节点*/
					else
					{
						index =  signal_nodes[node_index].reverse_node_index;
					}
				}
			}
		}
	}
	return index;
}

/****************************************************
函数名：   gn_guide_backword
功能描述： 获取引导进路该节点的后方节点
返回值：   int16_t
参数：     EN_node_direction dir
参数：     int16_t node_index
作者：	   hejh
日期：     2014/04/21
****************************************************/
int16_t gn_guide_backword(EN_node_direction dir, int16_t node_index)
{
	EN_node_direction nd;
	int16_t index = NO_INDEX;
	EN_switch_state switch_location = SWS_ERROR;

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
		index =  NO_INDEX;
	}
	/*判断信号点不是道岔*/
	if(gn_type(node_index) != NT_SWITCH)
	{
		/*根据信号点方向返回当前节点后方的节点*/
		if (dir == DIR_DOWN)
		{
			index = signal_nodes[node_index].previous_node_index;
		}
		else
		{
			index = signal_nodes[node_index].next_node_index;
		}
	}
	else 
	{
		/*判断信号点方向*/
		nd = gn_direction(node_index);
		if(dir == DIR_DOWN)
		{
			/*根据道岔方向返回当前节点后方的节点*/
			if((nd == DIR_LEFT_DOWN) || (nd == DIR_LEFT_UP))
			{
				switch_location = gn_switch_state(node_index);
				if (switch_location == SWS_NORMAL)
				{
					index =  signal_nodes[node_index].next_node_index;
				}
				else
				{
					index =  signal_nodes[node_index].reverse_node_index;
				}
			}
			/*根据道岔方向返回当前节点后方的节点*/
			if((nd == DIR_RIGHT_DOWN) || (nd == DIR_RIGHT_UP))
			{
				index =  signal_nodes[node_index].previous_node_index;
			}
		}
		/*判断信号点方向*/
		if(dir == DIR_UP)
		{
			/*根据道岔方向返回当前节点后方的节点*/
			if((nd == DIR_LEFT_DOWN) || (nd == DIR_LEFT_UP))
			{
				index =  signal_nodes[node_index].previous_node_index;
			}
			/*根据道岔方向返回当前节点后方的节点*/
			if((nd == DIR_RIGHT_DOWN) || (nd == DIR_RIGHT_UP))
			{
				switch_location = gn_switch_state(node_index);
				if (switch_location == SWS_NORMAL)
				{
					index =  signal_nodes[node_index].next_node_index;
				}
				else
				{
					index =  signal_nodes[node_index].reverse_node_index;
				}
			}
		}
	}
	return index;
}

/****************************************************
函数名:    get_next_node
功能描述:  获取下一个节点
返回值:    
参数:      index
作者  :    CY
日期  ：   2011/12/12
****************************************************/
int16_t gn_next(int16_t node_index)
{
	int16_t result;

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
		result = NO_INDEX;
	}
	else
	{
		/*获取下一个节点*/
		result = (signal_nodes[node_index].next_node_index);
	}
	return result;
}
/****************************************************
函数名:    gn_forword
功能描述:  按照方向，获取当前节点前方的节点
返回值:    
参数:      index
作者  :    CY
日期  ：   2011/12/12
****************************************************/
int16_t gn_forword(EN_node_direction dir, int16_t node_index)
{
	EN_node_direction nd;
	int16_t index = NO_INDEX;
	route_t route_index = NO_INDEX;
	EN_switch_state switch_location = SWS_ERROR;
	int16_t i;

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		/*判断信号点不是道岔，则直接根据进路方向返回当前节点的前一节点*/
		if(gn_type(node_index) != NT_SWITCH)
		{
			index = dir == DIR_UP ? signal_nodes[node_index].previous_node_index : signal_nodes[node_index].next_node_index;
		}
		else 
		{
			nd = gn_direction(node_index);
			/*判断进路方向*/
			if(dir == DIR_UP)
			{
				/*根据道岔方向返回当前节点的前节点*/
				if((nd == DIR_LEFT_DOWN) || (nd == DIR_LEFT_UP))
				{
					/*获取节点所在进路索引号*/
					route_index = gn_belong_route(node_index);
					/*不存在进路号直接返回直股节点*/
					if (route_index == NO_INDEX)
					{
						index =  signal_nodes[node_index].next_node_index;
					}
					else
					{
						/*hjh 2012-11-28 存在进路号则找出该道岔在进路中要求的位置*/
						for (i = 0; i < gr_switches_count(route_index); i++)
						{
							if ((node_index == gr_switch(route_index,i))
								|| (gn_another_switch(node_index) == gr_switch(route_index,i)))
							{
								switch_location = gr_switch_location(route_index,i);
								break;
							}
						}
						/*进路要求的位置在定位则返回直股节点*/
						if (switch_location == SWS_NORMAL)
						{
							index =  signal_nodes[node_index].next_node_index;
						}
						/*进路要求的位置在反位则返回弯股节点*/
						else
						{
							index =  signal_nodes[node_index].reverse_node_index;
						}
					}
				}
				/*根据道岔方向返回当前节点的前节点*/
				if((nd == DIR_RIGHT_DOWN) || (nd == DIR_RIGHT_UP))
				{
					index =  signal_nodes[node_index].previous_node_index;
				}
			}
			/*判断进路方向*/
			if(dir == DIR_DOWN)
			{
				/*根据道岔方向返回当前节点的前节点*/
				if((nd == DIR_LEFT_DOWN) || (nd == DIR_LEFT_UP))
				{
					index =  signal_nodes[node_index].previous_node_index;
				}
				/*根据道岔方向返回当前节点的后节点*/
				if((nd == DIR_RIGHT_DOWN) || (nd == DIR_RIGHT_UP))
				{
					/*获取节点所在进路索引号*/
					route_index = gn_belong_route(node_index);
					/*不存在进路号直接返回直股节点*/
					if (route_index == NO_INDEX)
					{
						index =  signal_nodes[node_index].next_node_index;
					}
					else
					{
						/*hjh 2012-11-28 存在进路号则找出该道岔在进路中要求的位置*/
						for (i = 0; i < gr_switches_count(route_index); i++)
						{
							if ((node_index == gr_switch(route_index,i))
								|| (gn_another_switch(node_index) == gr_switch(route_index,i)))
							{
								switch_location = gr_switch_location(route_index,i);
								break;
							}
						}
						/*进路要求的位置在定位则返回直股节点*/
						if (switch_location == SWS_NORMAL)
						{
							index =  signal_nodes[node_index].next_node_index;
						}
						/*进路要求的位置在反位则返回弯股节点*/
						else
						{
							index =  signal_nodes[node_index].reverse_node_index;
						}
					}
				}
			}
		}
	}
	return index;
}

/****************************************************
函数名：   gn_guide_forword
功能描述： 获取引导进路该节点前方节点
返回值：   int16_t
参数：     EN_node_direction dir
参数：     int16_t node_index
作者：	   hejh
日期：     2014/04/21
****************************************************/
int16_t gn_guide_forword(EN_node_direction dir, int16_t node_index)
{
	EN_node_direction nd;
	int16_t index = NO_INDEX;
	EN_switch_state switch_location = SWS_ERROR;

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		/*判断信号点不是道岔，则直接根据进路方向返回当前节点的前一节点*/
		if(gn_type(node_index) != NT_SWITCH)
		{
			index = dir == DIR_UP ? signal_nodes[node_index].previous_node_index : signal_nodes[node_index].next_node_index;
		}
		else 
		{
			nd = gn_direction(node_index);
			/*判断进路方向*/
			if(dir == DIR_UP)
			{
				/*根据道岔方向返回当前节点的前节点*/
				if((nd == DIR_LEFT_DOWN) || (nd == DIR_LEFT_UP))
				{
					switch_location = gn_switch_state(node_index);
					if (switch_location == SWS_NORMAL)
					{
						index =  signal_nodes[node_index].next_node_index;
					}
					if (switch_location == SWS_REVERSE)
					{
						index =  signal_nodes[node_index].reverse_node_index;
					}
				}
				/*根据道岔方向返回当前节点的前节点*/
				if((nd == DIR_RIGHT_DOWN) || (nd == DIR_RIGHT_UP))
				{
					index =  signal_nodes[node_index].previous_node_index;
				}
			}
			/*判断进路方向*/
			if(dir == DIR_DOWN)
			{
				/*根据道岔方向返回当前节点的前节点*/
				if((nd == DIR_LEFT_DOWN) || (nd == DIR_LEFT_UP))
				{
					index =  signal_nodes[node_index].previous_node_index;
				}
				/*根据道岔方向返回当前节点的后节点*/
				if((nd == DIR_RIGHT_DOWN) || (nd == DIR_RIGHT_UP))
				{
					switch_location = gn_switch_state(node_index);
					if (switch_location == SWS_NORMAL)
					{
						index =  signal_nodes[node_index].next_node_index;
					}
					if (switch_location == SWS_REVERSE)
					{
						index =  signal_nodes[node_index].reverse_node_index;
					}
				}
			}
		}
	}
	return index;
}

/****************************************************
函数名:    gn_reverse
功能描述:  获取道岔的另外一个节点
返回值:    
参数:      index
作者  :    CY
日期  ：   2011/12/12
****************************************************/
int16_t gn_reverse(int16_t node_index)
{
	int16_t result;

	/*参数检查*/
	if (node_index < 0 || node_index >= TOTAL_SIGNAL_NODE)
	{
		process_warning(ERR_INDEX,"");
		result = NO_INDEX;
	}
	else
	{
		/*获取道岔的另外一个节点*/
		result = (signal_nodes[node_index].reverse_node_index);
	}
	return result;
}

/****************************************************
函数名:    gn_reverse_dir
功能描述:  按照方向，获取道岔的反位节点
返回值:    
参数:      index
作者  :    CY
日期  ：   2011/12/12
****************************************************/
int16_t gn_reverse_dir(EN_node_direction dir, int16_t node_index)
{
	EN_node_direction nd;
	int16_t index = NO_INDEX;

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
		index = NO_INDEX;
	}
	else
	{
		/*判断节点是否是道岔*/
		if(gn_type(node_index) == NT_SWITCH)
		{
			nd = gn_direction(node_index);
			/*判断方向*/
			if(dir == DIR_UP)
			{
				/*根据方向获取道岔反位节点*/
				if((nd == DIR_LEFT_DOWN) || (nd == DIR_LEFT_UP))
				{
					index =  signal_nodes[node_index].reverse_node_index;
				}
			}
			/*判断方向*/
			if(dir == DIR_DOWN)
			{
				/*根据方向获取道岔反位节点*/
				if((nd == DIR_RIGHT_DOWN) || (nd == DIR_RIGHT_UP))
				{
					index =  signal_nodes[node_index].reverse_node_index;
				}
			}
		}
	}
	
	return index;
}

/****************************************************
函数名:    gn_direction
功能描述:  获取信号点方向
返回值:    
参数:      node_index
作者  :    CY
日期  ：   2011/12/14
****************************************************/
EN_node_direction gn_direction(int16_t node_index)
{
	EN_node_direction result;

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
		result = DIR_ERROR;
	}
	else
	{
		/*获取信号点方向*/
		result = (EN_node_direction)(signal_nodes[node_index].direction);
	}
	return result;
}

/****************************************************
函数名:    sn_locked_state
功能描述:  设置信号点的锁闭标志
返回值:    
参数:      node_index
作者  :    WSP
日期  ：   2011/12/14
****************************************************/
void sn_locked_state(int16_t node_index, EN_lock_type state)
{

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		/*根据参数设置信号点锁闭标志*/
		if (state == LT_LOCKED)
		{
			signal_nodes[node_index].locked_flag = (signal_nodes[node_index].locked_flag & (~ROUTE_LOCKED_MASK)) | state;
		}
		if (state == LT_SWITCH_CLOSED)
		{
			signal_nodes[node_index].locked_flag = (signal_nodes[node_index].locked_flag & (~SWITCH_CLOSED_MASK)) | state;
		}
		if (state == LT_SWITCH_SIGNLE_LOCKED)
		{
			signal_nodes[node_index].locked_flag = (signal_nodes[node_index].locked_flag & (~SWITCH_SIGNLE_LOCKED_MASK)) | state;
		}
		if (state == LT_SWITCH_THROAT_LOCKED)
		{
			signal_nodes[node_index].locked_flag = (signal_nodes[node_index].locked_flag & (~SWITCH_THROAT_LOCKED_MASK)) | state;
		}
		if (state == LT_MIDDLE_SWITCH_LOCKED)
		{
			signal_nodes[node_index].locked_flag = (signal_nodes[node_index].locked_flag & (~MIDDLE_SWITCH_LOCKED_MASK)) | state;
		}
	}

}
/****************************************************
函数名:    cn_locked_state
功能描述:  清除信号节点的锁闭标志
返回值:    
参数:      int16_t node_index
参数:      EN_lock_type state
作者  :    hejh
日期  ：   2012/7/31
****************************************************/
void cn_locked_state(int16_t node_index, EN_lock_type state)
{

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		/*根据参数清除信号节点的锁闭标志*/
		if (state == LT_LOCKED)
		{
			signal_nodes[node_index].locked_flag = signal_nodes[node_index].locked_flag & (~ROUTE_LOCKED_MASK);
			signal_nodes[node_index].belong_route = NO_INDEX;
			signal_nodes[node_index].state_machine = SCSM_INIT;
			signal_nodes[node_index].history_state = 0;
			//signal_nodes[node_index].state_changed = 0;
			signal_nodes[node_index].time_count = NO_TIMER;
			signal_nodes[node_index].time_type = DTT_INIT;
			signal_nodes[node_index].used = CI_FALSE;
		}
		if (state == LT_SWITCH_CLOSED)
		{
			signal_nodes[node_index].locked_flag = signal_nodes[node_index].locked_flag & (~SWITCH_CLOSED_MASK);
		}
		if (state == LT_SWITCH_SIGNLE_LOCKED)
		{
			signal_nodes[node_index].locked_flag = signal_nodes[node_index].locked_flag & (~SWITCH_SIGNLE_LOCKED_MASK);
		}
		if (state == LT_SWITCH_THROAT_LOCKED)
		{
			signal_nodes[node_index].locked_flag = signal_nodes[node_index].locked_flag & (~SWITCH_THROAT_LOCKED_MASK);
		}	
		if (state == LT_MIDDLE_SWITCH_LOCKED)
		{
			signal_nodes[node_index].locked_flag = signal_nodes[node_index].locked_flag & (~MIDDLE_SWITCH_LOCKED_MASK);
		}
	}
	
}
/****************************************************
函数名:    is_approach_cleared
功能描述:  接近区段空闲检查
返回值:    空闲返回真，反之返回假
参数:      route_index
			special_flag:检查特殊联锁标志
作者  :    WSP
日期  ：   2011/12/15
****************************************************/
CI_BOOL is_approach_cleared(route_t route_index, CI_BOOL special_flag)
{
	CI_BOOL result = CI_TRUE;
	route_t bwr;
	int16_t temp,si,i,j,k;
	node_t start_signal,end_signal,node_index;
	
	if ((route_index < 0) || (route_index >= MAX_ROUTE)
		|| NO_INDEX == routes[route_index].ILT_index )
	{
		process_warning(ERR_INDEX,"");
		result = CI_FALSE;
	}
	else
	{
		/*调车进路*/
		if (RT_SHUNTING_ROUTE == gr_type(route_index))
		{
			/*调车进路的接近区段是信号机外方邻接轨道电路区段*/
			if ((gr_approach(route_index) == NO_NODE)
				|| (SCS_CLEARED != gn_section_state(gr_approach(route_index))))
			{
				result = CI_FALSE;
			}
		}
		else
		{
			start_signal = gr_start_signal(route_index);
			end_signal = gb_node(gr_end_button(route_index));
			/*接车进路的接近区段为信号机外方的闭塞分区或轨道电路区段*/
			if (NT_ENTRY_SIGNAL == gn_type(start_signal))
			{
				/*自动闭塞*/
				for (i = 0; i < TOTAL_AUTO_BLOCK; i++)
				{
					for (j = 0; j < gs_auto_block_section_count(i); j++)
					{
						if (gr_approach(route_index) == gs_auto_block(i,j))
						{
							if (IsFALSE(all_switchs_is_normal(route_index)))
							{
								if (SCS_CLEARED != gn_section_state(gr_approach(route_index)))
								{
									result = CI_FALSE;
								}
							}
							else
							{
								/*接近区段空闲检查*/
								for (k = 0; k < gs_auto_block_section_count(i); k++)
								{
									if (SCS_CLEARED != gn_section_state(gs_auto_block(i,k)))
									{
										result = CI_FALSE;
										break;
									}
								}							
							}
							if (RSM_TRAIN_APPROACH == gr_state_machine(route_index))
							{
								result = CI_FALSE;
							}
							break;
						}
					}					
				}
				/*3显示自动闭塞*/
				for (i = 0; i < TOTAL_AUTO_BLOCK3; i++)
				{
					for (j = 0; j < gs_auto_block3_section_count(i); j++)
					{
						if (gr_approach(route_index) == gs_auto_block3(i,j))
						{
							/*接近区段只有一个*/
							if (SCS_CLEARED != gn_section_state(gr_approach(route_index)))
							{
								result = CI_FALSE;
							}

							if (RSM_TRAIN_APPROACH == gr_state_machine(route_index))
							{
								result = CI_FALSE;
							}
							break;
						}
					}					
				}
				/*改方运行*/
				for (i = 0; i < TOTAL_CHANGE_RUN_DIR; i++)
				{
					for (j = 0; j < gs_change_run_dir_section_count(i); j++)
					{
						if (gr_approach(route_index) == gs_change_run_dir(i,j))
						{
							if (IsFALSE(all_switchs_is_normal(route_index)))
							{
								if (SCS_CLEARED != gn_section_state(gr_approach(route_index)))
								{
									result = CI_FALSE;
								}
							}
							else
							{
								/*接近区段空闲检查*/
								for (k = 0; k < gs_change_run_dir_section_count(i); k++)
								{
									if (SCS_CLEARED != gn_section_state(gs_change_run_dir(i,k)))
									{
										result = CI_FALSE;
										break;
									}
								}
							}						
							if (RSM_TRAIN_APPROACH == gr_state_machine(route_index))
							{
								result = CI_FALSE;
							}
							break;
						}
					}					
				}
			
				if (IsTRUE(result))
				{
					/*接近区段空闲检查*/
					if ((SCS_CLEARED != gn_section_state(gr_approach(route_index))) || (RSM_TRAIN_APPROACH == gr_state_machine(route_index)))
					{
						result = CI_FALSE;
					}
				}
			
			}
			/*发车进路的接近区段为发车线*/
			else if ((NT_OUT_SHUNTING_SIGNAL == gn_type(start_signal))
			|| (NT_OUT_SIGNAL ==  gn_type(start_signal))
			|| (NT_ROUTE_SIGNAL ==  gn_type(start_signal)))
			{
				/*正线出站*/
				if ((IsTRUE(all_switchs_is_normal(route_index)) && ((NT_ENTRY_SIGNAL == gn_type(end_signal)) 
					|| (NT_ROUTE_SIGNAL == gn_type(end_signal)) || (NT_DIFF_SHUNTING_SIGNAL == gn_type(end_signal))))
					/*hjh 2014-9-22 进路信号机处的特殊处理*/
					|| (NT_ROUTE_SIGNAL ==  gn_type(start_signal)))
				{
					/*hjh 2014-2-25 增加检查特殊标志的条件*/
					if (IsTRUE(special_flag))
					{
						bwr = gr_backward(route_index);
						if ((NO_INDEX != bwr) && (IsFALSE(is_guide_route(bwr))))
						{	
							if (IsTRUE(is_successive_route(route_index)) && (RS_SIGNAL_OPENED == gr_state(route_index)))
							{
								/*接近区段空闲检查*/
								if (((gr_approach(route_index) != NO_NODE)
									&&(SCS_CLEARED != gn_section_state(gr_approach(route_index))))
									|| (RSM_TRAIN_APPROACH == gr_state_machine(route_index)))
								{
									result = CI_FALSE;
								}
							}
						}
						else
						{
							/*接近区段空闲检查*/
							if (((gr_approach(route_index) != NO_NODE)
								&&(SCS_CLEARED != gn_section_state(gr_approach(route_index))))
								|| (RSM_TRAIN_APPROACH == gr_state_machine(route_index)))
							{
								result = CI_FALSE;
							}

							/*存在特殊联锁的状态采集*/
							if (((temp = gs_special(route_index,SPT_STATE_COLLECT)) != NO_INDEX)
								&& (state_collect_config[temp].section != NO_INDEX)
								&& (gr_approach(route_index) != NO_NODE)
								&& (gr_approach(route_index) == state_collect_config[temp].section))
							{
								/*获取特殊联锁的状态采集的状态*/
								if ((gn_state(gs_state_collect(temp)) != SIO_HAS_SIGNAL)
									|| (SCS_CLEARED != gn_section_state(gr_approach(route_index))))
								{
									result = CI_FALSE;
								}
							}
							else
							{
								/*不存在特殊联锁，只检查接近区段的状态*/
								if (((gr_approach(route_index) != NO_NODE)
									&& (SCS_CLEARED != gn_section_state(gr_approach(route_index))))
									|| (RSM_TRAIN_APPROACH == gr_state_machine(route_index)))
								{
									result = CI_FALSE;
								}
							}
						}
					}
					else
					{
						if (((gr_approach(route_index) != NO_NODE)
							&& (SCS_CLEARED != gn_section_state(gr_approach(route_index))))
							|| (RSM_TRAIN_APPROACH == gr_state_machine(route_index)))
						{
							result = CI_FALSE;
						}
					}
				}
				else
				{
					/*hjh 2012-12-19 添加中间道岔部分的区段*/
					node_index = gr_approach(route_index);
					si = gs_middle_switch_index(start_signal);
					/*检查出站信号机处配置的中间道岔的配置数据*/
					if (si != NO_INDEX)
					{
						/*获取进站信号机的方向*/
						if (gn_direction(start_signal) == DIR_DOWN)
						{
							/*寻找下一节点*/
							for (i = 0; i < TOTAL_SIGNAL_NODE; i++)
							{
								/*另一咽喉的出站信号机*/
								if (IsTRUE(is_out_signal(node_index)) && (gn_direction(node_index) == DIR_UP))
								{
									break;
								}
								/*区段并且不空闲*/
								if ((IsTRUE(is_section(node_index)) && (SCS_CLEARED != gn_section_state(node_index)))
									|| (RSM_TRAIN_APPROACH == gr_state_machine(route_index)))
								{
									result = CI_FALSE;
									break;
								}
								/*继续找下一节点*/
								else
								{
									node_index = gn_forword(DIR_UP,node_index);
									if (node_index == NO_INDEX)
									{
										break;
									}
								}
							}
						}
						else
						{
							/*寻找下一节点*/
							for (i = 0; i < TOTAL_SIGNAL_NODE; i++)
							{
								/*另一咽喉的出站信号机*/
								if (IsTRUE(is_out_signal(node_index)) && (gn_direction(node_index) == DIR_DOWN))
								{
									break;
								}
								/*区段并且不空闲*/
								if ((IsTRUE(is_section(node_index)) && (SCS_CLEARED != gn_section_state(node_index)))
									|| (RSM_TRAIN_APPROACH == gr_state_machine(route_index)))
								{
									result = CI_FALSE;
									break;
								}
								/*继续找下一节点*/
								else
								{
									node_index = gn_forword(DIR_DOWN,node_index);
									if (node_index == NO_INDEX)
									{
										break;
									}
								}
							}
						}
					}
					else
					{
						/*接近区段空闲检查*/
						if (((gr_approach(route_index) != NO_NODE)
							&&(SCS_CLEARED != gn_section_state(node_index)))
							|| (RSM_TRAIN_APPROACH == gr_state_machine(route_index)))
						{
							result = CI_FALSE;
						}
					}
				}
			}
			/*以调车信号机为始端的列车进路*/
			else if (NT_DIFF_SHUNTING_SIGNAL == gn_type(gr_start_signal(route_index)))
			{
				/*hjh 2014-2-25 增加检查特殊标志的条件*/
				if (IsTRUE(special_flag))
				{
					/*存在特殊联锁的状态采集*/
					if (((temp = gs_special(route_index,SPT_STATE_COLLECT)) != NO_INDEX)
						&& (state_collect_config[temp].section != NO_INDEX)
						&& (gr_approach(route_index) != NO_NODE)
						&& (gr_approach(route_index) == state_collect_config[temp].section))
					{
						/*获取特殊联锁的状态采集的状态*/
						if ((gn_state(gs_state_collect(temp)) != SIO_HAS_SIGNAL)
							|| (SCS_CLEARED != gn_section_state(gr_approach(route_index))))
						{
							result = CI_FALSE;
						}
					}
					else
					{
						/*不存在特殊联锁，只检查接近区段的状态*/
						if (((gr_approach(route_index) != NO_NODE)
							&& (SCS_CLEARED != gn_section_state(gr_approach(route_index))))
							|| (RSM_TRAIN_APPROACH == gr_state_machine(route_index)))
						{
							result = CI_FALSE;
						}
					}
				}
				else
				{
					/*检查接近区段的状态*/
					if (((gr_approach(route_index) != NO_NODE)
						&& (SCS_CLEARED != gn_section_state(gr_approach(route_index))))
						|| (RSM_TRAIN_APPROACH == gr_state_machine(route_index)))
					{
						result = CI_FALSE;
					}
				}				
			}
			/*其他*/
			else
			{				
				if (((gr_approach(route_index) != NO_NODE)
					&& (SCS_CLEARED != gn_section_state(gr_approach(route_index))))
					|| (RSM_TRAIN_APPROACH == gr_state_machine(route_index)))
				{
					result = CI_FALSE;
				}
			}
		}
	}	
	return result;
}

/****************************************************
函数名：   check_approach_added
功能描述： 判断接近延长
返回值：   CI_BOOL
参数：     route_t route_index
作者：	   hejh
日期：     2014/06/13
****************************************************/
void check_approach_added(route_t route_index)
{
	CI_BOOL result = CI_FALSE;
	route_t bwr;
	int16_t i,j;
	node_t index = NO_NODE;
	node_t start_signal,end_signal;

	if ((route_index < 0) || (route_index >= MAX_ROUTE)
		|| NO_INDEX == routes[route_index].ILT_index )
	{
		process_warning(ERR_INDEX,"");
		result = CI_FALSE;
	}
	else
	{
		/*列车进路*/
		if ((RT_SHUNTING_ROUTE != gr_type(route_index)) && IsFALSE(is_guide_route(route_index)))
		{
			start_signal = gr_start_signal(route_index);
			end_signal = gb_node(gr_end_button(route_index));

			if ((NT_OUT_SHUNTING_SIGNAL == gn_type(start_signal))
				|| (NT_OUT_SIGNAL ==  gn_type(start_signal))
				|| (NT_ROUTE_SIGNAL ==  gn_type(gr_start_signal(route_index))))
			{
				/*正线出站*/
				if (IsTRUE(all_switchs_is_normal(route_index)) && ((NT_ENTRY_SIGNAL == gn_type(end_signal)) 
					|| (NT_ROUTE_SIGNAL == gn_type(end_signal)) || (NT_DIFF_SHUNTING_SIGNAL == gn_type(end_signal))))
				{
					bwr = gr_backward(route_index);
					if ((NO_INDEX != bwr) && (IsFALSE(is_guide_route(bwr))))
					{
						if ((((gn_type(gr_start_signal(bwr)) == NT_ENTRY_SIGNAL) || (gn_type(gr_start_signal(bwr)) == NT_ROUTE_SIGNAL))
							&& IsTRUE(all_switchs_is_normal(bwr))) || (gn_type(gr_start_signal(bwr)) == NT_OUT_SIGNAL) || (gn_type(gr_start_signal(bwr)) == NT_OUT_SHUNTING_SIGNAL))
						{
							/*接近区段延长至整个接车进路和2个闭塞分区*/
							/*自动闭塞*/
							for (i = 0; i < TOTAL_AUTO_BLOCK; i++)
							{
								for (j = 0; j < gs_auto_block_section_count(i); j++)
								{
									if (gr_approach(bwr) == gs_auto_block(i,j))
									{
										/*接近区段空闲检查*/
										if ((SCS_CLEARED != gn_section_state(gs_auto_block(i,0)))
											|| (SCS_CLEARED != gn_section_state(gs_auto_block(i,1))))
										{
											result = CI_TRUE;
										}
										break;
									}
								}
							}
							/*改方运行*/
							for (i = 0; i < TOTAL_CHANGE_RUN_DIR; i++)
							{
								for (j = 0; j < gs_change_run_dir_section_count(i); j++)
								{
									if (gr_approach(bwr) == gs_change_run_dir(i,j))
									{
										/*接近区段空闲检查*/
										if ((SCS_CLEARED != gn_section_state(gs_change_run_dir(i,gs_change_run_dir_section_count(i) - 1)))
											|| (SCS_CLEARED != gn_section_state(gs_change_run_dir(i,gs_change_run_dir_section_count(i) - 2))))
										{
											result = CI_TRUE;
										}
										break;
									}
								}
							}
							/*检查整个接车进路的空闲*/
							if (((RS_AUTOMATIC_UNLOCKING == gr_state(bwr)) || (RS_SK_N_SIGNAL_CLOSED == gr_state(bwr))
								|| (RS_AUTO_UNLOCK_FAILURE == gr_state(bwr))
								|| (RS_TRACK_POWER_OFF == gr_state(bwr)) || (RS_A_SIGNAL_CLOSED == gr_state(bwr)) )
								&& (gr_state_machine(bwr) > RSM_TRAIN_APPROACH))
							{
								for(i = 0; i < gr_nodes_count(bwr); i++)
								{
									/*区段空闲检查*/
									index = gr_node(bwr,i);
									if ((IsTRUE(is_section(index))) && (SCS_CLEARED != gn_section_state(index)))
									{
										result = CI_TRUE;
										break;
									}
								}
							}
						}
					}
					/*后方进路不存在时，清除接近延长标志*/
					if (bwr == NO_INDEX)
					{
						result = CI_FALSE;
						if (gr_other_flag(route_index) == ROF_APPROACH_ADDED)
						{
							sr_other_flag(route_index,ROF_ERROR);
						}
					}
				}
			}

			if (IsTRUE(result))
			{
				if (gr_other_flag(route_index) == ROF_ERROR)
				{
					sr_other_flag(route_index,ROF_APPROACH_ADDED);
				}				
			}
		}
	}
}

/****************************************************
函数名:    gr_first_section
功能描述:  获取进路信号内方第一区段索引
返回值:    存在返回区段索引，反之返回NO_INDEX
参数:      route_index
作者  :    WSP
日期  ：   2011/12/15
****************************************************/
int16_t gr_first_section(route_t route_index)
{
	int16_t i, count, index = NO_INDEX;
	EN_node_type s_type;

	/*参数检查*/
	if (route_index < 0 || route_index >= MAX_ROUTE 
		|| NO_INDEX == routes[route_index].ILT_index )
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		count = gr_nodes_count(route_index);
		for (i = 0;i < count; i++)
		{
			/*获取进路信号内方第一区段索引*/
			index = gr_node(route_index, i);
			s_type = gn_type(index);
			if ((s_type == NT_TRACK)
				|| (s_type == NT_NON_SWITCH_SECTION)
				|| (s_type == NT_SWITCH_SECTION)
				|| (s_type == NT_STUB_END_SECTION)
				|| (s_type == NT_LOCODEPOT_SECTION)
				|| (s_type == NT_SECTION))
			{
				break;
			}
		}
	}
	return index;
}

/****************************************************
函数名：   gr_last_section
功能描述： 获取进路上最后一个区段
返回值：   int16_t
参数：     route_t route_index
作者：	   hejh
日期：     2013/04/17
****************************************************/
int16_t gr_last_section(route_t route_index)
{
	int16_t i, index = NO_INDEX;

	/*参数检查*/
	if (route_index < 0 || route_index >= MAX_ROUTE 
		|| NO_INDEX == routes[route_index].ILT_index )
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		for (i = gr_nodes_count(route_index) - 1;i >= 0; i--)
		{
			/*获取进路信号内方第一区段索引*/
			index = gr_node(route_index, i);
			if (IsTRUE(is_section(index)))
			{
				break;
			}
		}
	}
	return index;
}

/****************************************************
函数名:    sn_belong_route
功能描述:  设置信号节点所属进路
返回值:    
参数:      node_index
参数:      route_index
作者  :    CY
日期  ：   2011/12/16
****************************************************/
void sn_belong_route( int16_t node_index, route_t route_index ) 
{
	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE) ||
		(route_index < 0) || (route_index >= MAX_ROUTE) || (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		/*设置信号节点所属进路*/
		signal_nodes[node_index].belong_route = route_index;
	}

}

/****************************************************
函数名:    sr_backward
功能描述:  设置本进路的后方进路
返回值:    
参数:      route_index
参数:      forward_route
作者  :    CY
日期  ：   2011/12/16
****************************************************/
void sr_backward( route_t route_index, route_t backward_route )
{
	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) 
		|| (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		/*设置本进路的后方进路*/
		routes[route_index].backward_route = backward_route;
	}

}
/****************************************************
函数名:    sr_forward
功能描述:  设置本进路的前方进路
返回值:    
参数:      route_index
参数:      forward_route
作者  :    CY
日期  ：   2011/12/16
****************************************************/
void sr_forward(route_t route_index,route_t forward_route)
{
	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) 
		|| (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		/*设置本进路的前方进路*/
		routes[route_index].forward_route = forward_route;
	}

}
/****************************************************
函数名:    is_signal
功能描述:  是否为信号机节点
返回值:    
参数:      node_index
作者  :    CY
日期  ：   2011/12/19
****************************************************/
CI_BOOL is_signal(int16_t node_index)
{
	CI_BOOL result = CI_FALSE;
	CI_BOOL nt = gn_type(node_index);

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		/*是否为信号机节点*/
		if ((nt > NT_NO) && (nt < NT_SIGNAL))
		{
			result = CI_TRUE;
		}
	}
	return result;
}
/****************************************************
函数名:    is_switch
功能描述:  是否为道岔节点
返回值:    
参数:      node_index
作者  :    CY
日期  ：   2011/12/19
****************************************************/
CI_BOOL is_switch(int16_t node_index)
{	
	CI_BOOL result = CI_FALSE;
	EN_node_type nt = gn_type(node_index);

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		/*判断是否为道岔节点*/
		if (NT_SWITCH == nt)
		{
			result = CI_TRUE;
		}
	}
	return result;
}
/****************************************************
函数名:    is_section
功能描述:  节点是否为区段
返回值:    
参数:      node_index
作者  :    CY
日期  ：   2011/12/19
****************************************************/
CI_BOOL is_section(int16_t node_index)
{
	CI_BOOL result = CI_FALSE;
	CI_BOOL nt = gn_type(node_index);

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		/*判断节点是否为区段*/
		if ((nt >= NT_TRACK) && (nt < NT_SECTION))
		{
			result = CI_TRUE;
		}
	}
	return result;
}

/****************************************************
函数名:    gr_node_index_route
功能描述:  获取信号点在进路中的索引号
返回值:    
参数:      route_index
参数:      node_index
作者  :    CY
日期  ：   2011/12/19
****************************************************/
int16_t gr_node_index_route(route_t route_index,int16_t node_index)
{
	int16_t result = NO_INDEX;
	int16_t i,node_count;

	/*参数检查*/
	if (route_index < 0 || route_index >= MAX_ROUTE || NO_INDEX == routes[route_index].ILT_index ||
		node_index < 0 || node_index >= TOTAL_SIGNAL_NODE )
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		/*获取信号点在进路中的索引号*/
		node_count = gr_nodes_count(route_index);
		for (i = 0; i < node_count; i++)
		{
			if (gr_node(route_index,i) == node_index)
			{
				result = i;
				break;
			}
		}
	}
	return result;
}

/****************************************************
函数名:    gn_section_switch
功能描述:  获取道岔区段中的道岔
返回值:    
参数:      node_index
作者  :    CY
日期  ：   2011/12/20
****************************************************/
int16_t gn_section_switch(int16_t node_index,int16_t ordinal)
{
	int16_t result = NO_INDEX;
	node_t switch0 = NO_INDEX,switch1 = NO_INDEX,switch2 = NO_INDEX,switch3 = NO_INDEX;
	int16_t i = 0;

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE) || 
		IsFALSE(is_section(node_index)) ||
		(ordinal < 0) || (ordinal >= MAX_SWITCH_PER_SECTION))
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{	
		/*获取道岔区段中的道岔*/
		//if (ordinal < 2)
		//{
		//	result = (int16_t)((signal_nodes[node_index].property >> (ordinal * 16)) & 0xFFFF);
		//}
		//else
		//{
		//	result = (int16_t)((signal_nodes[node_index].additional_property >> (( ordinal - 2 ) * 16)) & 0xFFFF);
		//}

		/*hjh 2015-4-20 修改获取道岔区段的道岔算法*/
		for (i = 0; i < TOTAL_SIGNAL_NODE; i++)
		{
			if((signal_nodes[i].type == NT_SWITCH) && ((int16_t)signal_nodes[i].additional_property == node_index))
			{
				if (switch0 == NO_INDEX)
				{
					switch0 = i;
				}
				else
				{
					if (switch1 == NO_INDEX)
					{
						switch1 = i;
					}
					else
					{
						if (switch2 == NO_INDEX)
						{
							switch2 = i;
						}
						else
						{
							switch3 = i;
						}
					}
				}
			}
		}
		if (ordinal == 0)
		{
			result = switch0;
		}
		if (ordinal == 1)
		{
			result = switch1;
		}
		if (ordinal == 2)
		{
			result = switch2;
		}
		if (ordinal == 3)
		{
			result = switch3;
		}
	}
	return result;
}

/****************************************************
函数名:    sr_increament_error_count
功能描述:  增加进路错误计数
返回值:    
参数:      route_index
作者  :    CY
日期  ：   2011/12/20
****************************************************/
void sr_increament_error_count(route_t route_index)
{
	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) 
		|| (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		/*增加进路错误计数*/
		routes[route_index].error_count++;
	}
}
/****************************************************
函数名:    sr_clear_error_count
功能描述:  清除进路错误计数
返回值:    
参数:      route_index
作者  :    CY
日期  ：   2011/12/20
****************************************************/
void sr_clear_error_count(route_t route_index)
{
	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) 
		|| (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		/*清除进路错误计数*/
		routes[route_index].error_count = 0;
	}
}

/****************************************************
函数名:    gr_error_count
功能描述:  获取进路当前的错误计数
返回值:    
参数:      route_index
作者  :    CY
日期  ：   2011/12/20
****************************************************/
int16_t gr_error_count(route_t route_index)
{
	int16_t result;
	
	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE)
		|| (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
		result = NO_INDEX;
	}
	else
	{
		/*获取进路当前的错误计数*/
		result = routes[route_index].error_count;
	}
	return result;
}

/****************************************************
函数名:    gn_state
功能描述:  获取信号点状态
返回值:    
参数:      int16_t node_index
作者  :    CY
日期  ：   2012/3/28
****************************************************/
uint32_t gn_state(int16_t node_index)
{
	uint32_t result;

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
		result = SWS_ERROR;
	}
	else
	{
		/*获取信号点状态*/
		return signal_nodes[node_index].state;
	}
	return result;
}

/****************************************************
函数名:    gn_signal_expect_state
功能描述:  获取信号节点的预期状态
返回值:    
参数:      node_index

作者  :    hjh
日期  ：   2012/3/7
****************************************************/
EN_signal_state gn_signal_expect_state(int16_t node_index)
{
	EN_signal_state result;

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE) )
	{
		process_warning(ERR_INDEX,"");
		result = SGS_ERROR;
	}
	else
	{
		/*获取信号节点的预期状态*/
		result = signal_nodes[node_index].expect_state;
	}
	return result;
}

/****************************************************
函数名：   gn_state_machine
功能描述： 获取节点的状态机
返回值：   EN_unlock_state
参数：     int16_t node_index
作者：	   hejh
日期：     2013/04/17
****************************************************/
EN_state_machine gn_state_machine(int16_t node_index)
{
	EN_state_machine result;

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE) )
	{
		process_warning(ERR_INDEX,"");
		result = SCSM_INIT;
	}
	else
	{
		/*获取节点的状态机*/
		result = signal_nodes[node_index].state_machine;
	}
	return result;
}

/****************************************************
函数名:    gn_signal_history_state
功能描述:  获取信号机的历史状态
返回值:    
参数:      node_index

作者  :    hjh
日期  ：   2012/3/22
****************************************************/
EN_signal_state gn_signal_history_state(int16_t node_index)
{
    EN_signal_state result;

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE) )
	{
		process_warning(ERR_INDEX,"");
		result = SGS_ERROR;
	}
	else
	{	
		/*获取信号机的历史状态*/
		result = signal_nodes[node_index].history_state & 0x0000FFFF;
	}
	return result;
}

/****************************************************
函数名:    gn_signal_node_history_state
功能描述:  获取信号机的历史状态(包括断丝历史状态)
返回值:    
参数:      int16_t node_index
作者  :    hejh
日期  ：   2012/8/10
****************************************************/
uint32_t gn_history_state(int16_t node_index)
{
	uint32_t result;

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE) )
	{
		process_warning(ERR_INDEX,"");
		result = SGS_ERROR;
	}
	else
	{
		/*获取信号机的历史状态(包括断丝历史状态)*/
		result = signal_nodes[node_index].history_state;
	}
	return result;
}

/****************************************************
函数名:    gr_approach
功能描述:  获取进路的接近区段
返回值:    
参数:      route_index
作者  :    CY
日期  ：   2011/12/26
****************************************************/
int16_t gr_approach(route_t route_index)
{
	int16_t result;

	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) 
		|| (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
		result = NO_INDEX;
	}
	else
	{
		/*获取进路的接近区段*/
		result = ILT[routes[route_index].ILT_index].approach_section;
	}
	return result;
}

/****************************************************
函数名:    sn_lock_state
功能描述:  设置信号点锁闭状态
返回值:    
参数:      node_index
作者  :    CY
日期  ：   2011/12/26
****************************************************/
void sn_lock_state(int16_t node_index, EN_lock_type lt)
{
	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE) )
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		/*设置信号点锁闭状态*/
		signal_nodes[node_index].locked_flag = (uint32_t)lt;
	}
}

/****************************************************
函数名:    gn_special_interlocking
功能描述:  获得特殊联锁索引号
返回值:    
参数:      node_index
作者  :    hjh
日期  ：   2011/12/27
****************************************************/
int16_t gn_special_interlocking(int16_t node_index)
{
	int16_t result;

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE) )
	{
		process_warning(ERR_INDEX,"");
		result = SWS_ERROR;
	}
	else
	{
		result = (int16_t)(signal_nodes[node_index].property & 0xFFFF);
	}
	return result;
}

/****************************************************
函数名:    gs_middle_switch_index
功能描述:  根据信号机获取中间道岔的配置索引号
返回值:    
参数:      int16_t signal
作者  :    CY
日期  ：   2012/8/7
****************************************************/
int16_t gs_middle_switch_index(int16_t signal)
{
	int16_t i;
	int16_t result = NO_INDEX;
	
	for (i = 0; i < TOTAL_MIDDLLE_SWITCHES; i++)
	{
		/*根据信号机获取中间道岔的配置索引号*/
		if (middle_switch_config[i].SignalIndex == signal)
		{
			result = i;
			break;
		}
	}
	return result;
}

/****************************************************
函数名:    gs_middle_switch
功能描述:  获取中间道岔
返回值:    
参数:      int16_t signal
参数:      int16_t index
作者  :    CY
日期  ：   2012/8/7
****************************************************/
int16_t gs_middle_switch(int16_t special_index,int16_t index)
{
	int16_t result = NO_INDEX;

	/*参数检查*/
	if ((special_index < 0) || (special_index >= TOTAL_MIDDLLE_SWITCHES))
	{
		process_warning(ERR_INDEX,"");
	} 
	else
	{	
		if (index < middle_switch_config[special_index].SwitchCount)
		{
			/*获取中间道岔*/
			result = middle_switch_config[special_index].SwitchIndex[index];
		}		
	}
	return result;
}

/****************************************************
函数名:    gs_middle_section
功能描述:  获取中间道岔的区段
返回值:    
参数:      int16_t signal
参数:      int16_t index
作者  :    hejh
日期  ：   2012/12/17
****************************************************/
int16_t gs_middle_section(int16_t special_index,int16_t index)
{
	int16_t result = NO_INDEX;

	/*参数检查*/
	if ((special_index < 0) || (special_index >= TOTAL_MIDDLLE_SWITCHES) 
		|| (index >= middle_switch_config[special_index].SectionCount))
	{
		process_warning(ERR_INDEX,"");
	} 
	else
	{	
		if (index < middle_switch_config[special_index].SectionCount)
		{
			/*获取中间道岔*/
			result = middle_switch_config[special_index].SectionIndex[index];
		}		
	}
	return result;
}

/****************************************************
函数名:    gs_allow_reverse
功能描述:  获取允许中间道岔反位的标志
返回值:    
参数:      int16_t signal
作者  :    hejh
日期  ：   2014/2/14
****************************************************/
int16_t gs_middle_allow_reverse(int16_t special_index)
{
	int16_t result = NO_INDEX;

	/*参数检查*/
	if ((special_index < 0) || (special_index >= TOTAL_MIDDLLE_SWITCHES))
	{
		process_warning(ERR_INDEX,"");
	} 
	else
	{	
		/*获取中间道岔*/
		result = middle_switch_config[special_index].AllowReverseDepature;
	}
	return result;
}

/****************************************************
函数名:    gs_middle_switch_signal
功能描述:  中间道岔关联的信号机
返回值:    
参数:      int16_t index
作者  :    CY
日期  ：   2012/8/7
****************************************************/
int16_t gs_middle_switch_signal(int16_t special_index)
{
	int16_t result = NO_INDEX;

	/*参数检查*/
	if ((special_index < 0) || (special_index >= TOTAL_MIDDLLE_SWITCHES))
	{
		process_warning(ERR_INDEX,"");
	} 
	else
	{
		result = middle_switch_config[special_index].SignalIndex;
	}
	return result;
}


/****************************************************
函数名：   is_unlock_sections
功能描述： 是否不解锁区段
返回值：   CI_BOOL
参数：     node_t node_index
作者：	   hejh
日期：     2014/04/24
****************************************************/
CI_BOOL is_unlock_sections(node_t node_index)
{
	int i;
	CI_BOOL result = CI_FALSE;

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		for (i = 0; i < MAX_UNLOCK_SECTIONS; i++)
		{
			if (other_special_config.unlock_sections[i] == node_index)
			{
				result = CI_TRUE;
				break;
			}
		}
	}
	return result;
}

/****************************************************
函数名：   is_switchs18
功能描述： 是否18#道岔
返回值：   CI_BOOL
参数：     node_t node_index
作者：	   hejh
日期：     2014/04/25
****************************************************/
CI_BOOL is_switchs18(node_t node_index)
{
	int i;
	CI_BOOL result = CI_FALSE;

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		for (i = 0; i < MAX_SWITCHS18; i++)
		{
			if (other_special_config.switchs18[i] == node_index)
			{
				result = CI_TRUE;
				break;
			}
		}
	}
	return result;
}

/****************************************************
函数名：   is_high_speed_switch
功能描述： 是否高速道岔
返回值：   CI_BOOL
参数：     node_t node_index
作者：	   hejh
日期：     2014/05/04
****************************************************/
CI_BOOL is_high_speed_switch(node_t node_index)
{
	int i;
	CI_BOOL result = CI_FALSE;
	node_t another_switch = NO_INDEX;

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		another_switch = gn_another_switch(node_index);
		for (i = 0; i < TOTAL_HIGH_SPEED_SWITCH; i++)
		{
			if ((highspeed_switch_config[i].SwitchIndex == node_index)
				|| ((another_switch != NO_INDEX) && (highspeed_switch_config[i].SwitchIndex == another_switch)))
			{
				result = CI_TRUE;
				break;
			}
		}
	}
	return result;
}

/****************************************************
函数名：   is_switch_location_reverse
功能描述： 是否表示相反道岔
返回值：   CI_BOOL
参数：     node_t node_index
作者：	   hejh
日期：     2014/04/25
****************************************************/
CI_BOOL is_switch_location_reverse(node_t node_index)
{
	int i;
	CI_BOOL result = CI_FALSE;

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		for (i = 0; i < TOTAL_LOCATION_REVERSE; i++)
		{
			if (location_reverse_config[i].SwitchIndex == node_index)
			{
				result = CI_TRUE;
				break;
			}
		}
	}
	return result;
}

/****************************************************
函数名：   gr_delay_times
功能描述： 获取进路的初始延时时间
返回值：   CI_TIMER
参数：     route_t route_index
作者：	   hejh
日期：     2014/03/12
****************************************************/
CI_TIMER gr_delay_times(route_t route_index)
{
	int16_t i;
	CI_TIMER result = SECONDS_30;

	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) 
		|| (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		if (RT_SHUNTING_ROUTE != gr_type(route_index))
		{
			result = MINUTES_3;
			for (i = 0; i < TOTAL_DELAY_30SECONDS; i++)
			{
				if (gr_start_signal(route_index) == delay_30seconds_config[i].SignalIndex)
				{
					result = SECONDS_30;
					break;
				}
			}
		}		
	}

	return result;
}

/****************************************************
函数名：   is_red_filament
功能描述： 是否检查红灯断丝
返回值：   CI_BOOL
参数：     route_t route_index
作者：	   hejh
日期：     2014/03/12
****************************************************/
CI_BOOL is_red_filament(route_t route_index)
{
	int16_t i;
	CI_BOOL result = CI_FALSE;
	node_t start_signal;

	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) 
		|| (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		if (RT_SHUNTING_ROUTE != gr_type(route_index))
		{
			start_signal = gr_start_signal(route_index);
			for (i = 0; i < TOTAL_RED_FILAMENT; i++)
			{
				if (start_signal == red_filament_config[i].SignalIndex)
				{
					/*需要检查红灯断丝*/
					/*红灯灯丝检查*/
					if (SGS_FILAMENT_BREAK == gn_signal_state(start_signal))
					{
						process_warning(ERR_FILAMENT_BREAK,gr_name(route_index));
						//CIHmi_SendNormalTips("断丝");
						result = CI_TRUE;
					}
					else if (SGS_H == gn_signal_state(start_signal))
					{
						result = CI_FALSE;
					}
					/*若此时信号机亮其他颜色灯光，则直接关闭信号*/
					else
					{
						send_signal_command(start_signal,SGS_H);
						sr_state(route_index,RS_SK_A_SIGNAL_CLOSING);
						sr_clear_error_count(route_index);
						result = CI_TRUE;
					}
					break;
				}
			}
		}		
	}

	return result;
}

/****************************************************
函数名：   is_track_power_on
功能描述： 轨道电源是否正常
返回值：   CI_BOOL
作者：	   hejh
日期：     2014/05/20
****************************************************/
CI_BOOL is_track_power_on()
{
	CI_BOOL result = CI_TRUE;

	/*轨道停电继电器*/
	/*if ((other_special_config.special_point[0] != NO_INDEX) 
		&& (gn_state(other_special_config.special_point[0]) != SIO_HAS_SIGNAL))
	{
		result = CI_FALSE;
	}*/
	return result;
}

/****************************************************
函数名：   is_flash_power_on
功能描述： 闪光电源是否正常
返回值：   CI_BOOL
作者：	   hejh
日期：     2014/05/20
****************************************************/
CI_BOOL is_flash_power_on()
{
	CI_BOOL result = CI_TRUE;

	/*轨道停电继电器*/
	if ((other_special_config.special_point[1] != NO_INDEX) 
		&& (gn_state(other_special_config.special_point[1]) != SIO_HAS_SIGNAL))
	{
		result = CI_FALSE;
	}
	return result;
}

/****************************************************
函数名:    sr_start_timer
功能描述:  开始计时
返回值:    
参数:      route_index
作者  :    CY
日期  ：   2011/12/28
****************************************************/
void sr_start_timer(route_t route_index,CI_TIMER interval,EN_delay_time_type time_type)
{
	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) 
		|| (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		/*设置开始计时*/
		sn_start_timer(gr_start_signal(route_index),interval,time_type);
	}

}

/****************************************************
函数名:    sr_stop_timer
功能描述:  停止计时
返回值:    
参数:      route_index
作者  :    CY
日期  ：   2011/12/28
****************************************************/
void sr_stop_timer(route_t route_index)
{
	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE)
		||( NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		/*设置停止计时*/
		sn_stop_timer(gr_start_signal(route_index));
	}
}


/****************************************************
函数名:    sn_start_timer
功能描述:  节点开始计时
返回值:    
参数:      node_index

作者  :    hjh
日期  ：   2012/3/22
****************************************************/
void sn_start_timer(int16_t node_index,CI_TIMER interval,EN_delay_time_type time_type)
{

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		/*节点开始计时*/
		signal_nodes[node_index].time_count = CICycleInt_GetCounter() + interval / CI_CYCLE_MS + (1000 / CI_CYCLE_MS - 1);
		if (interval == SECONDS_15)
		{
			signal_nodes[node_index].time_count = CICycleInt_GetCounter() + interval / CI_CYCLE_MS + 2;
		}
		signal_nodes[node_index].time_type = time_type;
	}
}

/****************************************************
函数名:    sn_stop_timer
功能描述:  节点停止计时
返回值:    
参数:      node_index

作者  :    hjh
日期  ：   2012/3/22
****************************************************/
void sn_stop_timer(int16_t node_index)
{
	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		/*节点停止计时*/
		signal_nodes[node_index].time_count = NO_TIMER;
		signal_nodes[node_index].time_type = DTT_INIT;
	}
}

/****************************************************
函数名:    is_node_timer_run
功能描述:  节点计时器是否在计时
返回值:    
参数:      node_index

作者  :    hjh
日期  ：   2012/3/22
****************************************************/
CI_BOOL is_node_timer_run(int16_t node_index)
{
	CI_BOOL result = CI_FALSE;

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
		result = CI_FALSE;
	}
	else
	{
		/*返回节点计时器是否在计时*/
		result = (signal_nodes[node_index].time_count == NO_TIMER) ? CI_FALSE:CI_TRUE;
	}
	return result;
}

/****************************************************
函数名:    is_node_complete_timer
功能描述:  节点计时是否已完成
返回值:    
参数:      node_index

作者  :    hjh
日期  ：   2012/3/22
****************************************************/
CI_BOOL is_node_complete_timer(int16_t node_index)
{
	CI_BOOL result = CI_FALSE;

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
		result = CI_FALSE;
	}
	else
	{
		/*返回节点计时是否已完成*/
		//result = (signal_nodes[node_index].time_count - CICycleInt_GetCounter() == 0 ? CI_TRUE:CI_FALSE;
		/*hjh 2014-2-28 判断计时完成条件*/
		if (signal_nodes[node_index].time_count - CICycleInt_GetCounter()== 1)
		{
			result = CI_TRUE;
		}
		else
		{
			if (signal_nodes[node_index].time_count <= CICycleInt_GetCounter())
			{
				result = CI_TRUE;
			}
		}
	}
	return result;
}

/****************************************************
函数名:    is_timer_run
功能描述:  计时器是否正在计时
返回值:    
参数:      route_index
作者  :    CY
日期  ：   2011/12/28
****************************************************/
CI_BOOL is_timer_run(route_t route_index)
{

	CI_BOOL result = CI_FALSE;

	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) 
		|| (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
		result = CI_FALSE;
	}
	else
	{
		/*返回计时器是否正在计时*/
		result = is_node_timer_run(gr_start_signal(route_index));
	}
	return result;
}

/****************************************************
函数名:    is_complete_timer
功能描述:  是否完成计时
返回值:    
参数:      route_index
作者  :    CY
日期  ：   2011/12/28
****************************************************/
CI_BOOL is_complete_timer( route_t route_index )
{
	CI_BOOL result = CI_FALSE;

	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE)
		|| (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
		result = CI_FALSE;
	}
	else
	{
		/*返回是否完成计时*/
		result = is_node_complete_timer(gr_start_signal(route_index));
	}
	return result;
}

/****************************************************
函数名：   gr_time_type
功能描述： 获取进路延时类型
返回值：   EN_delay_time_type
参数：     route_t route_index
作者：	   hejh
日期：     2014/05/12
****************************************************/
EN_delay_time_type gr_time_type( route_t route_index )
{
	EN_delay_time_type result = DTT_INIT;

	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE)
		|| (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
		result = DTT_INIT;
	}
	else
	{
		/*返回是否完成计时*/
		result = signal_nodes[gr_start_signal(route_index)].time_type;
	}
	return result;
}

/****************************************************
函数名:    is_throat_lock
功能描述:  判断咽喉是否引总锁闭
返回值:    TRUE：引总锁闭
	       FALSE：未引总锁闭
参数:      throat

作者  :    hjh
日期  ：   2012/3/20
****************************************************/
CI_BOOL is_throat_lock(uint8_t throat)
{
	int16_t index = 0;
	CI_BOOL result = CI_FALSE;

	FUNCTION_IN;
    for (index = 0; index < TOTAL_SIGNAL_NODE; index++)
    {
        /*选出道岔*/
        if (IsTRUE(is_switch(index)))
        {
            /*判断道岔所在咽喉*/
            if (throat == gn_throat(index))
            {
                /*判断道岔所在咽喉是否引总锁闭*/
                if (IsTRUE(is_node_locked(index, LT_SWITCH_THROAT_LOCKED)))
                {
                    result = CI_TRUE;
                }
                break;
            }
        }
    }
	
	FUNCTION_OUT;
	return result;
}

/****************************************************
函数名:    gr_name
功能描述:  获取进路名称
返回值:    
参数:      route_index

作者  :    CY
日期  ：   2012/1/5
****************************************************/
char_t *gr_name(route_t route_index)
{
	int16_t ilt_index = routes[route_index].ILT_index;
	char_t *result;

	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) || (NO_INDEX == ilt_index))
	{
		process_warning(ERR_INDEX,"");
		sprintf(route_name,"route index : %d",ilt_index);
		result = 0;
	}
	else
	{
		/*获取进路名称*/
		sprintf(route_name,"route:%d/%s--->%s",ilt_index,device_name[ ILT[ilt_index].start_button],device_name[ILT[ilt_index].end_button]);
		result = route_name;
	}
	return result;
}


/****************************************************
函数名:    sr_updata_cycle
功能描述:  更新记录当前处理周期
返回值:    
参数:      route_index
作者  :    CY
日期  ：   2012/3/20
****************************************************/
void sr_updata_cycle(route_t route_index)
{
	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) 
		|| (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		/*更新记录当前处理周期*/
		routes[route_index].current_cycle = CICycleInt_GetCounter();
	}

}

/****************************************************
函数名:    is_route_cycle_right
功能描述:  判断进路周期是否正确
返回值:    
参数:      route_index
作者  :    CY
日期  ：   2012/3/20
****************************************************/
CI_BOOL is_route_cycle_right(route_t route_index)
{
	CI_BOOL result = CI_FALSE;

	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) 
		|| (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
		result = CI_FALSE;
	}
	else
	{
		/*判断进路周期是否正确*/
		result = routes[route_index].current_cycle < CICycleInt_GetCounter() ? CI_TRUE:CI_FALSE;
	}
	return result;
}

/****************************************************
函数名：   gr_cycle
功能描述： 获取进路周期
返回值：   CI_TIMER
参数：     route_t route_index
作者：	   hejh
日期：     2014/05/08
****************************************************/
CI_TIMER gr_cycle(route_t route_index)
{
	CI_TIMER result = NO_TIMER;

	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) 
		|| (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
		result = NO_TIMER;
	}
	else
	{
		/*判断进路周期是否正确*/
		result = routes[route_index].current_cycle;
	}
	return result;
}

/****************************************************
函数名:    gn_name
功能描述:  信号点名称
返回值:    
参数:      node_index

作者  :    CY
日期  ：   2012/1/5
****************************************************/
char_t* gn_name(int16_t node_index)
{
	char_t* result = 0;

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_NAMES))
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		/*信号点名称*/
		result = device_name[node_index];
	}
	return result;
}

/****************************************************
函数名:    send_switch_command
功能描述:  发送道岔控制命令
返回值:    
参数:      node_index
参数:      command

作者  :    CY
日期  ：   2012/1/6
****************************************************/
void send_switch_command( int16_t node_index,uint32_t command )
{
	/*参数检查*/
    if ((node_index < 0) || (node_index >= TOTAL_NAMES))
    {
        process_warning(ERR_INDEX,"");
    }
	else
	{
		 /*发送道岔控制命令*/
		set_command(node_index,command);
		CIHmi_SendDebugTips("%s:%s",gn_name(node_index),command == SWS_NORMAL?"定操":"反操");
	}
}

/****************************************************
函数名:    ILT_to_route
功能描述:  将联锁表索引号转换为进路号
返回值:    
参数:      ILT_index

作者  :    CY
日期  ：   2012/1/9
****************************************************/
int16_t ILT_to_route(int16_t ILT_index)
{
	int16_t i = 0,result = NO_INDEX;

	/*参数检查*/
	if ((ILT_index < 0) || (ILT_index >= TOTAL_ILT))
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		for (i = 0; i < MAX_ROUTE; i++)
		{
			if (routes[i].ILT_index == ILT_index)
			{
				result = i;
				break;
			}
		}
	}
	return result;
}

/****************************************************
函数名:    is_successive_route
功能描述:  是否为延续进路
返回值:    
参数:      index_t ILT_index
作者  :    CY
日期  ：   2012/3/21
****************************************************/
CI_BOOL is_successive_ILT(index_t ILT_index)
{
	CI_BOOL result = CI_FALSE;
	int16_t i = 0;

	for (i = 0; i < TOTAL_SUCCESSIVE_ROUTE; i++)
	{
		/*是否为延续进路*/
		if (gb_node(ILT[ILT_index].start_button) == successive_route_config[i].StartSignalIndex)
		{
			result = CI_TRUE;
			break;
		}
	}
	return result;
}

/****************************************************
函数名:    is_successive_button
功能描述:  延续进路按钮是否正确
返回值:    
参数:      index_t ILT_index
作者  :    CY
日期  ：   2012/3/22
****************************************************/
CI_BOOL is_successive_button(index_t ILT_index)
{
	CI_BOOL result = CI_FALSE;
	int16_t i = 0,j = 0;

	/*参数检查*/
	if (third_button != NO_INDEX)
	{
		for (i = 0; i < TOTAL_SUCCESSIVE_ROUTE; i++)
		{
			/*基本进路*/
			if ((gb_node(first_button) == successive_route_config[i].StartSignalIndex)
				&& (gb_node(second_button) == successive_route_config[i].EndSignalIndex))
			{
				for (j = 0; j < successive_route_config[i].SuccessiveEndCount; j++)
				{
					/*判断延续进路按钮是否正确*/
					if ((successive_route_config[i].SuccessiveEnd[j] == third_button) 
						|| (successive_route_config[i].SuccessiveEnd[j] == gb_node(third_button)) )
					{
						result = CI_TRUE;
						break;
					}
				}
			}
			/*变通进路判断*/
			if ((gb_node(first_button) == successive_route_config[i].StartSignalIndex)
				&& (ILT[ILT_index].change_button == second_button)
				&& (gb_node(third_button) == successive_route_config[i].EndSignalIndex))
			{
				for (j = 0; j < successive_route_config[i].SuccessiveEndCount; j++)
				{
					/*判断延续进路按钮是否正确*/
					if ((successive_route_config[i].SuccessiveEnd[j] == fourth_button) 
						|| (successive_route_config[i].SuccessiveEnd[j] == gb_node(fourth_button)) )
					{
						result = CI_TRUE;
						break;
					}
				}
			}
		}
	}
	return result;
}

/****************************************************
函数名:    set_command
功能描述:  设置信号点所在的电子单元的发送命令
返回值:    信号节点索引号
参数:      node_index
作者  :    CY
日期  ：   2012/1/9
****************************************************/
void set_command(int16_t node_index,uint32_t cmd)
{
    uint16_t address;

	/*参数检查*/
    if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
    {
        process_warning(ERR_INDEX,"");
    }
	else
	{
		/*设置信号点所在的电子单元的发送命令*/
#ifdef WIN32
		CIEeu_SendNodeIndexCmd(node_index,cmd);
#endif		
		address = signal_nodes[node_index].output_address;
		commands[GATEWAY(address)][EEU(address)] = cmd;
	}
}

/****************************************************
函数名：   clear_switch_command
功能描述： 清除命令
返回值：   void
参数：     int16_t node_index
作者：	   hejh
日期：     2014/04/17
****************************************************/
void clear_switch_command(int16_t node_index)
{
	uint16_t address;

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		/*设置信号点所在的电子单元的发送命令*/
		address = signal_nodes[node_index].output_address;
		commands[GATEWAY(address)][EEU(address)] = SWS_ERROR;
	}
}

/****************************************************
函数名:    get_command
功能描述:  获取信号点所在的电子单元的发送命令
返回值:    信号节点索引号
参数:      node_index
作者  :    CY
日期  ：   2012/1/9
****************************************************/
uint32_t get_command(int16_t node_index)
{
    uint16_t address;
	uint32_t result;

	/*参数检查*/
    if ((node_index < 0) || (node_index >= TOTAL_NAMES))
    {
        process_warning(ERR_INDEX,"");
        result = 0;
    }
	else
	{
		/*获取信号点所在的电子单元的发送命令*/
		address = signal_nodes[node_index].output_address;
		result = commands[GATEWAY(address)][EEU(address)];
	}
	return result;
}

/****************************************************
函数名:    sn_switch_location
功能描述:  设置当前道岔的位置
返回值:    
参数:      
作者  :    CY
日期  ：   2011/12/12
****************************************************/
void sn_switch_location(int16_t node_index, EN_switch_state location)
{
	int16_t another;

	/*参数检查*/
    if (node_index < 0 || node_index >= TOTAL_NAMES)
    {
        process_warning(ERR_INDEX,"");
    }
	else
	{
		/*设置当前道岔的位置*/
		another = (int16_t)(signal_nodes[node_index].property);
		if ((another != NO_INDEX))
		{
			signal_nodes[another].state = location;
		}
		signal_nodes[node_index].state = location;
	}

}

/****************************************************
函数名:    strlen_check
功能描述:  求字符串长度
返回值:    
参数:      str

作者  :    CY
日期  ：   2012/1/13
****************************************************/
size_t strlen_check(char_t * str)
{
	char_t * result = str;
	while( *(str++)) ;
	return str - result - 1;
}
/****************************************************
函数名:    strcmmp_no_case
功能描述:  不区分大小写的字符串比较
返回值:    
参数:      str1
参数:      str2

作者  :    CY
日期  ：   2012/1/11
****************************************************/
int16_t strcmp_no_case(const char_t * str1, const char_t * str2)
{
	while( ((*str1 >= 'A' && *str1 <= 'Z')? (*str1)+ 'a' - 'A' : *str1) ==
			((*str2 >= 'A' && *str2 <= 'Z')? (*str2)+ 'a' - 'A' : *str2) &&
			*str1 && *str2)
	{
		str1++;
		str2++;
	}
	return *str1 || *str2 ;
}
/****************************************************
函数名:    strcpy_check
功能描述:  按照规定的长度复制字符串
返回值:    
参数:      str1
参数:      str2
参数:      len

作者  :    CY
日期  ：   2012/1/11
****************************************************/
char_t * strcpy_check( char_t * str1,char_t * str2, size_t len )
{
	/*参数检查*/
	if (len < 0x7fffffff)
	{	
		/*字符串长度检查*/
		if (strlen_check(str2) > len)
		{
			while( (len--) > 0 && (*(str1++) = *(str2++)));
			*(--str1) = 0;
			CIHmi_SendDebugTips("复制字符串的长度超过了要求的长度\n");
		}
		else
		{
			/*按照规定的长度复制字符串*/
			while( (*(str1++) = *(str2++)));
		}
	}
	return str1;
}

/****************************************************
函数名:    strcat_check
功能描述:  带长度检查的字符串拼接
返回值:    
参数:      str1
参数:      str2
参数:      len

作者  :    CY
日期  ：   2012/1/11
****************************************************/
char_t * strcat_check(char_t * str1,char_t * str2, size_t len)
{
	/*参数检查*/
	if (strlen_check(str1) + strlen_check(str2) > len)
	{
		strcpy_check(str1 + strlen_check(str1),str2,len - strlen_check(str1));
		CIHmi_SendDebugTips("字符串拼接时剩余空间不够\n");
	}
	else
	{
		/*带长度检查的字符串拼接*/
		strcpy_check(str1 + strlen_check(str1),str2,len - strlen_check(str1));
	}
	return str1;
}

/****************************************************
函数名:    reset_route
功能描述:  重置进路数据，删除旧数据
返回值:    

作者  :    CY
日期  ：   2011/12/20
****************************************************/
void reset_route(route_t route_index)
{
	int16_t special_index = 0,i = 0;

	/*参数检查*/
	if ((special_index = gs_special(route_index,SPT_SEMI_AUTO_BLOCK)) != NO_INDEX)
	{
		semi_auto_block_config[special_index].belong_route = -1;
	}

	for ( i = 0; i < TOTAL_SEMI_AUTO_BLOCK; i++)
	{
		if (semi_auto_block_config[i].entry_signal == gr_start_signal(route_index))
		{
			sab_recieving_route_unlock(i);
			semi_auto_block_config[i].belong_route = -1;
		}
	}
	/*重置进路数据,删除旧数据*/
	routes[route_index].ILT_index = NO_INDEX;
	routes[route_index].error_count = 0;
	routes[route_index].forward_route = NO_INDEX;
	routes[route_index].backward_route = NO_INDEX;
	routes[route_index].state = RS_ERROR;
	routes[route_index].current_cycle = NO_TIMER;
	routes[route_index].approach_unlock = CI_FALSE;
	routes[route_index].other_flag = ROF_ERROR;
	routes[route_index].state_machine = RSM_INIT;
}

/****************************************************
函数名:    is_route_approach_unlock
功能描述:  接近区段已解锁
返回值:    
参数:      route_t route_index
作者  :    CY
日期  ：   2012/7/23
****************************************************/
CI_BOOL is_route_approach_unlock(route_t route_index)
{
	CI_BOOL result = CI_FALSE;

	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{	
		/*接近区段已解锁*/
		result = routes[route_index].approach_unlock;
	}
	return result;
}

/****************************************************
函数名:    unlock_approach_section
功能描述:  解锁进路的接近区段
返回值:    
参数:      node_t section
作者  :    CY
日期  ：   2012/7/23
****************************************************/
void unlock_approach_section(node_t section)
{
	route_t i = 0;

	/*参数检查*/
	if ((NO_INDEX != section) && IsTRUE(is_section(section)))
	{
		for (i =0; i < MAX_ROUTE; i++)
		{
			if (IsTRUE(is_route_exist(i)))
			{
				/*解锁进路的接近区段*/
				if(gr_approach(i) == section)
				{
					routes[i].approach_unlock = CI_TRUE;
				}
			}
		}
	}
}

/****************************************************
函数名:    reset_node
功能描述:  重置节点数据，删除旧数据
返回值:    

作者  :    CY
日期  ：   2011/12/20
****************************************************/
void reset_node(int16_t node_index)
{
	unlock_approach_section(node_index);
	cn_locked_state(node_index,LT_LOCKED);
}

/****************************************************
函数名:    print_nodes
功能描述:  打印节点名称
返回值:    

作者  :    CY
日期  ：   2011/12/20
****************************************************/
void print_nodes(void)
{
	int16_t i,j;
	for (i=0 ; i < TOTAL_ILT; i ++)
	{
		CIHmi_SendDebugTips("route:%d / %s / %s :",i,device_name[ ILT[i].start_button],device_name[ILT[i].end_button]);
		for (j=0;j < ILT[i].nodes_count;j++)
		{
			CIHmi_SendDebugTips("%s ",device_name[ILT[i].nodes[j]]);
		}
		CIHmi_SendDebugTips("\n");
	}
}
/****************************************************
函数名:    delete_route
功能描述:  删除进路
返回值:    

作者  :    CY
日期  ：   2011/12/20
****************************************************/
void delete_route(route_t route_index)
{
	int16_t i = 0,node,node_count;
	int16_t si,ms,temp;
	node_t switch_index,another_signal;
	CI_BOOL belong_route = CI_FALSE;

	/*参数检查*/
	/*hjh 2014-5-9 删除进路时不一定要设置进路状态为进路已解锁，否则会造成误解*/
	if ( (route_index >= 0) && (routes[route_index].ILT_index != NO_INDEX))// && (gr_state(route_index) == RS_UNLOCKED))
	{
		/*reset进路上的信号点*/
		node_count = gr_nodes_count(route_index);
		for(i = 0 ;i < node_count; i++)
		{
			node = gr_node(route_index,i);
			/*检查进路索引号*/
			if (gn_belong_route(node) == route_index)
			{
				/*hjh 2013-3-22 两个咽喉向同一股道办理调车进路，再取消某一进路时会使股道解锁*/
				if ((gr_type(route_index) == RT_SHUNTING_ROUTE) && (gn_type(node) == NT_TRACK))
				{
					/*获取股道另一端的信号机*/
					another_signal = gn_forword(gr_direction(route_index),node);
					/*另一端的信号机在某条进路中*/
					if ((another_signal != NO_INDEX) && (gn_belong_route(another_signal) != NO_ROUTE)
						&& (gr_type(gn_belong_route(another_signal)) == RT_SHUNTING_ROUTE)
						&& another_signal != gr_start_signal(gn_belong_route(another_signal)))
					{
						sn_belong_route(node,gn_belong_route(another_signal));
					}
					/*不存在以另一端信号机为终端的进路*/
					else
					{
						reset_node(node);
					}
				}
				else
				{
					reset_node(node);
				}
				belong_route = CI_TRUE;
			}
		}

		if (IsTRUE(belong_route))
		{
			si = gs_middle_switch_index(gr_start_signal(route_index));
			/*检查中间道岔是否存在*/
			if (si != NO_INDEX)
			{
				if (IsTRUE(is_unlock_middle_switch(gr_start_signal(route_index))))
				{
					for (i = 0; i < MAX_MIDDLE_SWITCHES; i++)
					{
						ms = gs_middle_switch(si,i);
						if (ms != NO_INDEX)
						{
							/*检查道岔是否被单锁*/
							if (IsTRUE(is_node_locked(ms,LT_MIDDLE_SWITCH_LOCKED)))
							{
								cn_locked_state(ms,LT_MIDDLE_SWITCH_LOCKED);
							}
							/*检查道岔是否双动道岔*/
							if (gn_another_switch(ms) != NO_INDEX)
							{
								cn_locked_state(gn_another_switch(ms),LT_MIDDLE_SWITCH_LOCKED);
							}
						}
					}
				}			
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
						/*检查道岔是否被单锁*/
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

			/*设置半自动关联的进路*/
			for (i = 0; i < TOTAL_SEMI_AUTO_BLOCK; i++)
			{
				if (semi_auto_block_config[i].belong_route == route_index)
				{
					semi_auto_block_config[i].belong_route = NO_INDEX;
					break;
				}
			}
		}		

		/*检查前方进路是否存在*/
		if(gr_forward(route_index) != NO_ROUTE)
		{
			sr_backward(gr_forward(route_index), NO_ROUTE);
			sr_forward(route_index,NO_ROUTE);
		}
		/*检查后方进路是否存在*/
		if(gr_backward(route_index) != NO_ROUTE)
		{
			sr_forward(gr_backward(route_index),NO_ROUTE);
			sr_backward(route_index,NO_ROUTE);
		}
		/*复位进路中的节点*/
		reset_route(route_index);
	} 

}

/****************************************************
函数名：   delete_middle_guide_route
功能描述： 删除中间道岔处的引导进路
返回值：   void
参数：     route_t route_index
作者：	   hejh
日期：     2014/04/29
****************************************************/
void delete_middle_guide_route(route_t route_index)
{
	int16_t i = 0,j,node,node_count;
	int16_t si,temp;
	node_t switch_index;
	CI_BOOL result = CI_TRUE;

	/*参数检查*/
	if ( (route_index >= 0) && (routes[route_index].ILT_index != NO_INDEX) && IsTRUE(is_guide_route(route_index)))
	{
		/*reset进路上的信号点*/
		node_count = gr_nodes_count(route_index);
		for(i = 0 ;i < node_count; i++)
		{
			node = gr_node(route_index,i);
			/*检查进路索引号*/
			if (gn_belong_route(node) == route_index)
			{
				/*中间道岔处引导进路*/
				si = gs_middle_switch_index(gb_node(gr_end_button(route_index)));
				if (si != NO_INDEX)
				{
					for (j = 0; j < MAX_MIDDLE_SECTIONS; j++)
					{
						if (node == gs_middle_section(si,j))
						{
							result = CI_FALSE;
							break;
						}
					}
				}
				if (IsTRUE(result))
				{
					reset_node(node);
				}
				result = CI_TRUE;
			}
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
					/*检查道岔是否被单锁*/
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

		/*检查前方进路是否存在*/
		if(gr_forward(route_index) != NO_ROUTE)
		{
			sr_backward(gr_forward(route_index), NO_ROUTE);
			sr_forward(route_index,NO_ROUTE);
		}
		/*检查后方进路是否存在*/
		if(gr_backward(route_index) != NO_ROUTE)
		{
			sr_forward(gr_backward(route_index),NO_ROUTE);
			sr_backward(route_index,NO_ROUTE);
		}
		/*复位进路中的节点*/
		//reset_route(route_index);
	} 
}

/****************************************************
函数名：   is_unlock_middle_switch
功能描述： 可否解锁中间道岔
返回值：   CI_BOOL
参数：     node_t state_signal
作者：	   hejh
日期：     2014/04/28
****************************************************/
CI_BOOL is_unlock_middle_switch(node_t start_signal)
{
	node_t fw_section,ms,next_node = NO_INDEX;
	index_t ordinal,si,i;
	CI_BOOL result = CI_FALSE;
	route_t another_route;
	EN_node_direction dir;

	if (IsTRUE(is_out_signal(start_signal)))
	{
		si = gs_middle_switch_index(start_signal);		
		/*检查出站信号机是否设置了中间道岔*/
		if (si != NO_INDEX)
		{
			dir = gn_direction(start_signal);
			/*根据出站信号机找出股道另一端的出站信号机*/
			for (i = 0 ; i < MAX_NODES_PS; i++)
			{
				if (dir == DIR_UP)
				{
					next_node = gn_next(start_signal);
					if (IsTRUE(is_switch(next_node)))
					{
						if ((gn_direction(next_node) == DIR_LEFT_UP) || (gn_direction(next_node) == DIR_LEFT_DOWN))
						{
							next_node = gn_previous(next_node);
						}
						else
						{
							next_node = gn_next(next_node);
						}
					}
				}
				else
				{
					next_node = gn_previous(start_signal);
					if (IsTRUE(is_switch(next_node)))
					{
						if ((gn_direction(next_node) == DIR_LEFT_UP) || (gn_direction(next_node) == DIR_LEFT_DOWN))
						{							
							next_node = gn_next(next_node);
						}
						else
						{
							next_node = gn_previous(next_node);
						}
					}
				}
				if (IsTRUE(is_out_signal(next_node)))
				{
					break;
				}
				start_signal = next_node;
			}
			/*获取另一端信号机所在进路*/
			another_route = gn_belong_route(next_node);
			if ((another_route == NO_INDEX) || (RT_SHUNTING_ROUTE == gn_type(another_route)) || (gr_start_signal(another_route) != next_node))
			{
				result = CI_TRUE;
			}
			else
			{
				ordinal = gr_node_index_route(another_route,next_node);
				fw_section = gr_forward_section(another_route,ordinal);
				si = gs_middle_switch_index(next_node);		
				for (i = 0; i < MAX_MIDDLE_SWITCHES; i++)
				{
					ms = gs_middle_switch(si,i);
					/*出站信号机内方是道岔区段，且是锁闭的，并且是占用状态*/
					if ((ms != NO_INDEX)
						&& (gr_state(another_route) == RS_SK_N_SIGNAL_CLOSED)
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
								result = CI_TRUE;
							}
						}
						else
						{
							if ((SCS_CLEARED == gn_section_state(gs_middle_section(si,0)))
								&& (SCS_CLEARED == gn_section_state(gs_middle_section(si,1)))
								&& (SCS_CLEARED == gn_section_state(gs_middle_section(si,2))))
							{
								result = CI_TRUE;
							}
						}
					}
					/*进路内方第一区段解锁后才可以解锁*/
					if ((ms != NO_INDEX)
						&& (gr_state(another_route) > RS_ROUTE_LOCKED)
						&& (gn_type(fw_section) == NT_SWITCH_SECTION) 
						&& IsFALSE(is_node_locked(fw_section,LT_LOCKED)) 
						&& (gn_section_state(fw_section) == SCS_CLEARED))
					{
						result = CI_TRUE;
					}
				}
			}
		}
	}
	return result;
}

/****************************************************
函数名:    gn_approach_section
功能描述:  获取进站信号机外方的接近区段
返回值:    
参数:      entry_signal
作者  :    CY
日期  ：   2011/12/13
****************************************************/
node_t gn_approach_section(node_t entry_signal,index_t section)
{
	int16_t section1 = NO_NODE,section2 = NO_NODE,section3 = NO_NODE;
	node_t result = NO_NODE;

	/*参数检查*/
	if (entry_signal < 0 || entry_signal >= TOTAL_NAMES || section <= 0 || section > 3)
	{
		process_warning(ERR_INDEX,"");
		result = NO_NODE;
	}
	else
	{
		/*判断进路方向*/
		if (gn_direction(entry_signal) == DIR_UP)
		{
			/*获取接近区段*/
			section1 = signal_nodes[entry_signal].next_node_index;
			if (section1 != NO_NODE)
			{
				section2 = signal_nodes[section1].next_node_index;
			}
			if (section2 != NO_NODE)
			{
				section3 = signal_nodes[section2].next_node_index;
			}
		}
		/*判断进路方向*/
		if (gn_direction(entry_signal) == DIR_DOWN)
		{
			/*获取接近区段*/
			section1 = signal_nodes[entry_signal].previous_node_index;
			if (section1 != NO_NODE)
			{
				section2 = signal_nodes[section1].previous_node_index;
			}
			if (section2 != NO_NODE)
			{
				section3 = signal_nodes[section2].previous_node_index;
			}
		}
		/*获取进站信号机外方的接近区段*/
		if (section == 1)
		{
			result = section1;
		}
		if (section == 2)
		{
			result = section2;
		}
		if (section == 3)
		{
			result = section3;
		}		
	}
	return result;
}


/****************************************************
函数名:    gn_data
功能描述:  获取信号点数据
返回值:    
参数:      node_t node
作者  :    CY
日期  ：   2012/3/21
****************************************************/
uint16_t gn_data(node_t node_index)
{
	uint16_t lock_state = 0;
	uint16_t data = 0;
	node_t section = NO_INDEX;
	route_t route_index = NO_INDEX;
	int16_t i = 0,button1 = NO_INDEX,button2 = NO_INDEX;

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_NAMES) )
	{
		process_warning(ERR_INDEX,"");
		data = 0;
	}
	else
	{
		/*获取信号点数据*/
		/*lock_state = signal_nodes[node_index].locked_flag;
		data = (lock_state&0xF) | (((lock_state>>8)&0xF)<<4) | 
			(((lock_state>>16)&0xF)<<8) | (((lock_state>>24)&0x3)<<12) | (signal_nodes[node_index].used?0xC000:0x0);
		data = (data<<16) | (signal_nodes[node_index].state & 0xFFFF);*/

		/*判断道岔占用*/
		if (gn_type(node_index) == NT_SWITCH)
		{
			section = gn_switch_section(node_index);
			if (gn_section_state(section) != SCS_CLEARED)
			{
				route_index = gn_belong_route(section);
				/*道岔区段在进路中*/
				if (route_index != NO_INDEX)
				{
					for (i = 0; i < gr_switches_count(route_index); i++)
					{
						/*道岔在进路中但不是防护道岔*/
						if ((node_index == gr_switch(route_index,i))
							|| (gn_another_switch(node_index) == gr_switch(route_index,i)))
						{
							if (IsFALSE(is_protective_switch(route_index,i)) && IsFALSE(is_follow_switch(route_index,i)))
							{
								lock_state = lock_state | 0x0001;
							}
							break;
						}
					}
				}
				/*道岔区段不在进路中则直接返回占用*/
				else
				{
					lock_state = lock_state | 0x0001;
				}
			}
		}
		/*锁闭标志*/
		if ((gn_type(node_index) >= NT_SWITCH) && (gn_type(node_index) <= NT_SECTION))
		{
			/*获取进路锁闭状态*/
			if ((signal_nodes[node_index].locked_flag & ROUTE_LOCKED_MASK) == LT_LOCKED)
			{
				lock_state = lock_state | 0x0002;
			}			
			/*获取道岔封锁状态*/
			if ((signal_nodes[node_index].locked_flag & SWITCH_CLOSED_MASK) == LT_SWITCH_CLOSED)
			{
				lock_state = lock_state | 0x0004;
			}
			/*获取道岔单锁状态*/
			if ((signal_nodes[node_index].locked_flag & SWITCH_SIGNLE_LOCKED_MASK) == LT_SWITCH_SIGNLE_LOCKED)
			{
				lock_state = lock_state | 0x0008;
			}
			/*获取咽喉区锁闭状态*/
			if ((signal_nodes[node_index].locked_flag & SWITCH_THROAT_LOCKED_MASK) == LT_SWITCH_THROAT_LOCKED)
			{
				lock_state = lock_state | 0x0010;
			}
			/*获取中间道岔锁闭状态*/
			if ((signal_nodes[node_index].locked_flag & MIDDLE_SWITCH_LOCKED_MASK) == LT_MIDDLE_SWITCH_LOCKED)
			{
				lock_state = lock_state | 0x0020;
			}
			/*将锁闭标志和占用标志置于高位*/
			lock_state = (lock_state << 8) & 0xFF00;
		}
		/*按钮封锁标志*/
		if ((gn_type(node_index) >= NT_ENTRY_SIGNAL) && (gn_type(node_index) <= NT_SIGNAL))
		{
			/*获取信号机处的按钮*/
			button1 = signal_nodes[node_index].buttons[0];
			button2 = signal_nodes[node_index].buttons[1];
			if (button1 != NO_INDEX)
			{
				if (button2 != NO_INDEX)
				{
					if (IsTRUE(is_button_locked(button1)))
					{
						lock_state = lock_state | 0x0001;
					}
					if (IsTRUE(is_button_locked(button2)))
					{
						lock_state = lock_state | 0x0002;
					}
				}
				else
				{
					if (IsTRUE(is_button_locked(button1)))
					{
						lock_state = lock_state | 0x0001;
					}
				}
			}
		}
		/*压缩数据*/
		data = lock_state | (CI_CompressSntState((uint16_t)gn_state(node_index)) & 0x00FF);		
	}
	return data;
}

/****************************************************
函数名:    sn_data
功能描述:  设置信号点数据
返回值:    
参数:      node_t node
参数:      uint32_t data
作者  :    CY
日期  ：   2012/3/21
****************************************************/
void sn_data(node_t node_index,uint32_t data)
{
	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_NAMES) )
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		/*设置信号点数据*/
		signal_nodes[node_index].state = data;
	}
}

/****************************************************
函数名：   sr_fault_section
功能描述： 设置故障区段
返回值：   void
参数：     route_t route_index
作者：	   hejh
日期：     2014/04/17
****************************************************/
void sr_fault_section( route_t route_index )
{
	int8_t i;
	node_t node_index;

	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		for (i = 0; i < gr_nodes_count(route_index); i++)
		{
			node_index = gr_node(route_index,i);
			if (IsTRUE(is_section(node_index)))
			{
				if (SCS_CLEARED == gn_state(node_index))
				{
					signal_nodes[node_index].fault_flag = CI_FALSE;
				}
				else
				{
					signal_nodes[node_index].fault_flag = CI_TRUE;
				}
			}
		}
	}
	FUNCTION_OUT;
}

/****************************************************
函数名：   gn_fault_state
功能描述： 获取区段的故障标志
返回值：   CI_BOOL
参数：     node_t node_index
作者：	   hejh
日期：     2014/04/18
****************************************************/
CI_BOOL is_section_fault(node_t node_index)
{
	CI_BOOL result = CI_FALSE;

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
		result = CI_FALSE;
	}
	else
	{
		result = signal_nodes[node_index].fault_flag;
	}
	return result;
}

/****************************************************
函数名:    have_successive_route
功能描述:  是否有延续进路
返回值:    
参数:      index_t route_index
作者  :    CY
日期  ：   2012/3/22
****************************************************/
CI_BOOL have_successive_route(index_t route_index)
{
	CI_BOOL result = CI_FALSE;

	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) 
		|| (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
		result = CI_FALSE;
	}
	else
	{
		/*是否有延续进路*/
		result = is_successive_ILT(routes[route_index].ILT_index);
	}
	return result;
}

/****************************************************
函数名:    is_successive_route
功能描述:  是否为延续进路
返回值:    
参数:      index_t route_index
作者  :    hjh
日期  ：   2012/4/18
****************************************************/
CI_BOOL is_successive_route(route_t route_index)
{
	CI_BOOL result = CI_FALSE;

	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE)
		|| (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		/*是否为延续进路*/
		if ((routes[route_index].other_flag == ROF_SUCCESSIVE) 
			|| (routes[route_index].other_flag == ROF_SUCCESSIVE_DELAY))
		{
			result = CI_TRUE;
		}
	}
	return result;
}

/****************************************************
函数名：   gs_auto_block_section_count
功能描述： 获取自动闭塞区段的数量
返回值：   node_t
参数：     special_t special
作者：	   hejh
日期：     2015/03/13
****************************************************/
node_t gs_auto_block_section_count(special_t special)
{
	uint8_t result = 0;

	/*参数检查*/
	if ((special < 0) || (special >= TOTAL_AUTO_BLOCK))
	{
		process_warning(ERR_INDEX,"");
		result = 0;
	}
	else
	{
		/*获取自动闭塞某个区段的索引号*/
		result = auto_block_config[special].SectionCount;
	}
	return result;
}

/****************************************************
函数名:    gs_auto_block
功能描述:  获取自动闭塞某个区段的索引号
返回值:    
参数:      special
参数:      LQ

作者  :    hjh
日期  ：   2012/3/23
****************************************************/
node_t gs_auto_block(special_t special,index_t LQ)
{
	node_t result = NO_INDEX;

	/*参数检查*/
	if ((special < 0) || (special >= TOTAL_AUTO_BLOCK) || (LQ >= auto_block_config[special].SectionCount))
	{
		process_warning(ERR_INDEX,"");
		result = NO_INDEX;
	}
	else
	{
		/*获取自动闭塞某个区段的索引号*/
		result = auto_block_config[special].Sections[LQ];
	}
	return result;
}

/****************************************************
函数名：   gs_auto_block_section_count
功能描述： 获取三显示自动闭塞区段的数量
返回值：   node_t
参数：     special_t special
作者：	   hejh
日期：     2015/03/13
****************************************************/
node_t gs_auto_block3_section_count(special_t special)
{
	uint8_t result = 0;

	/*参数检查*/
	if ((special < 0) || (special >= TOTAL_AUTO_BLOCK3))
	{
		process_warning(ERR_INDEX,"");
		result = 0;
	}
	else
	{
		/*获取自动闭塞某个区段的索引号*/
		result = auto_block3_config[special].SectionCount;
	}
	return result;
}

/****************************************************
函数名:    gs_auto_block3
功能描述:  获取三显示自动闭塞某个区段的索引号
返回值:    
参数:      special
参数:      LQ

作者  :    hjh
日期  ：   2012/3/23
****************************************************/
node_t gs_auto_block3(special_t special,index_t JG)
{
	node_t result = NO_INDEX;

	/*参数检查*/
	if ((special < 0) || (special >= TOTAL_AUTO_BLOCK3) || (JG >= auto_block3_config[special].SectionCount))
	{
		process_warning(ERR_INDEX,"");
		result = NO_INDEX;
	}
	else
	{
		/*获取三显示自动闭塞某个区段的索引号*/
		result = auto_block3_config[special].Sections[JG];
	}
	return result;
}

/****************************************************
函数名：   gs_auto_block_section_count
功能描述： 获取改方运行区段的数量
返回值：   node_t
参数：     special_t special
作者：	   hejh
日期：     2015/03/13
****************************************************/
node_t gs_change_run_dir_section_count(special_t special)
{
	uint8_t result = 0;

	/*参数检查*/
	if ((special < 0) || (special >= TOTAL_CHANGE_RUN_DIR))
	{
		process_warning(ERR_INDEX,"");
		result = 0;
	}
	else
	{
		/*获取自动闭塞某个区段的索引号*/
		result = change_run_dir_config[special].SectionCount;
	}
	return result;
}

/****************************************************
函数名:    gs_change_run_dir
功能描述:  获取改方运行某个区段的索引号
返回值:    
参数:      special
参数:      JG

作者  :    hjh
日期  ：   2012/4/11
****************************************************/
node_t gs_change_run_dir(special_t special,index_t JG)
{
	node_t result = NO_INDEX;

	/*参数检查*/
	if ((special < 0) || (special >= TOTAL_CHANGE_RUN_DIR) || (JG >= change_run_dir_config[special].SectionCount))
	{
		process_warning(ERR_INDEX,"");
		result = NO_INDEX;
	}
	else
	{
		/*获取改方运行某个区段的索引号*/
		result = change_run_dir_config[special].Sections[JG];
	}
	return result;
}

/****************************************************
函数名:    gs_request_agree
功能描述:  获取请求同意点的状态
返回值:    
参数:      special_t special
作者  :    hejh
日期  ：   2012/9/6
****************************************************/
node_t gs_request_agree(special_t special)
{
	node_t result = NO_INDEX;

	/*参数检查*/
	if ((special < 0) || (special >= TOTAL_REQUEST_AGREE))
	{
		process_warning(ERR_INDEX,"");
		result = NO_INDEX;
	}
	else
	{
		/*获取请求同意点的状态*/
		result = request_agree_config[special];
	}
	return result;
}

/****************************************************
函数名:    gs_indication_lamp
功能描述:  获取表示灯的索引号
返回值:    
参数:      special_t special
作者  :    hejh
日期  ：   2013/1/7
****************************************************/
node_t gs_indication_lamp(special_t special)
{
	node_t result = NO_INDEX;

	/*参数检查*/
	if ((special < 0) || (special >= TOTAL_INDICATION_LAMP))
	{
		process_warning(ERR_INDEX,"");
		result = NO_INDEX;
	}
	else
	{
		/*获取表示灯的特殊索引号*/
		result = indication_lamp_config[special];
	}
	return result;
}

/****************************************************
函数名:    gs_yards_liaision
功能描述:  获取场间联系的状态
返回值:    
参数:      special_t special
作者  :    hejh
日期  ：   2012/12/10
****************************************************/
node_t gs_yards_liaision(special_t special)
{
	node_t result = NO_INDEX;

	/*参数检查*/
	if ((special < 0) || (special >= TOTAL_YARDS_LIAISION))
	{
		process_warning(ERR_INDEX,"");
		result = NO_INDEX;
	}
	else
	{
		/*获取场间联系的状态*/
		result = yards_liaision_config[special];
	}
	return result;
}

/****************************************************
函数名:    gs_state_collect
功能描述:  获取状态采集的状态
返回值:    
参数:      special_t special
作者  :    hejh
日期  ：   2012/12/10
****************************************************/
node_t gs_state_collect(special_t special)
{
	node_t result = NO_INDEX;

	/*参数检查*/
	if ((special < 0) || (special >= TOTAL_STATE_COLLECT))
	{
		process_warning(ERR_INDEX,"");
		result = NO_INDEX;
	}
	else
	{
		/*获取状态采集的状态*/
		result = state_collect_config[special].check_node;
	}
	return result;
}

/****************************************************
函数名:    gs_special_switch_index
功能描述:  获取特殊防护道岔的索引号
返回值:    
参数:      special_t special
参数:      int16_t num
作者  :    hejh
日期  ：   2013/1/28
****************************************************/
node_t gs_special_switch_index(special_t special,int16_t num)
{
	node_t result = NO_INDEX;

	/*参数检查*/
	if ((special < 0) || (special >= TOTAL_SPECIAL_SWITCH)
		|| (num >= special_switch_config[special].SwitchCount))
	{
		process_warning(ERR_INDEX,"");
		result = NO_INDEX;
	}
	else
	{
		/*获取特殊防护道岔的索引号*/
		result = special_switch_config[special].SwitchIndex[num];
	}
	return result;
}

/****************************************************
函数名:    gs_special_switch_location
功能描述:  获取特殊防护道岔的位置
返回值:    
参数:      special_t special
参数:      int16_t num
作者  :    hejh
日期  ：   2013/1/28
****************************************************/
EN_switch_state gs_special_switch_location(special_t special,int16_t num)
{
	EN_switch_state result = SWS_ERROR;

	/*参数检查*/
	if ((special < 0) || (special >= TOTAL_SPECIAL_SWITCH) 
		|| (num >= special_switch_config[special].SwitchCount))
	{
		process_warning(ERR_INDEX,"");
		result = SWS_ERROR;
	}
	else
	{
		/*获取特殊防护道岔的索引号*/
		result = special_switch_config[special].SwitchLocation[num];
	}
	return result;
}

/****************************************************
函数名：   gs_special_switch_unlock_section
功能描述： 获取特殊防护道岔解锁区段的索引号
返回值：   node_t
参数：     special_t special
作者：	   hejh
日期：     2014/05/04
****************************************************/
node_t gs_special_switch_unlock_section(special_t special )
{
	node_t result = NO_INDEX;

	/*参数检查*/
	if ((special < 0) || (special >= TOTAL_SPECIAL_SWITCH))
	{
		process_warning(ERR_INDEX,"");
		result = NO_INDEX;
	}
	else
	{
		/*获取特殊防护道岔解锁区段的索引号*/
		result = special_switch_config[special].UnlockSectionIndex;
	}
	return result;
}

/****************************************************
函数名:    all_switchs_is_normal
功能描述:  判断进路上所有道岔在定位
返回值:    TRUE表示道岔均在定位，否则FALSE
参数:      route_index

作者  :    hjh
日期  ：   2012/3/6
****************************************************/
CI_BOOL all_switchs_is_normal( route_t route_index )
{
	int16_t i,index;
	CI_BOOL result = CI_TRUE;

	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*检查进路中所有道岔位置*/
		for (i = 0; i <  gr_nodes_count(route_index); i++)
		{
			index = gr_node(route_index, i);
			if (IsTRUE(is_switch(index)) && (gn_switch_state(index) == SWS_REVERSE))
			{
				if (IsFALSE(is_switch_location_reverse(index)))
				{
					result = CI_FALSE;
					break;
				}
			}
		}
	} 
	else
	{
		process_warning(ERR_INDEX,"");
	}
	FUNCTION_OUT;
	return result;
}

/****************************************************
函数名:    input_node_state
功能描述:  打印
返回值:    
参数:      GA，EA，state
作者  :    hjh
日期  ：   2012/3/6
****************************************************/
void input_node_state(uint8_t GA,uint8_t EA,uint32_t state)
{
	uint16_t temp1,temp2;	
	int16_t node_index = NO_INDEX,node_index2 = NO_INDEX;
	int16_t i;
		
	/*第一、二路设备状态*/
	temp2 = (state >> 16) & 0x0000FFFF;
	temp1 = state & 0x0000FFFF;

	 /*检查数据的ID是否超出范围*/
	if ((GA >= TOTAL_GETWAY) || (EA >= MAX_EEU_PER_LAYER))
	{
		CIHmi_SendDebugTips("error:电子模块ID超出范围! GA:%d,EA:%d",GA,EA);
	}
	else
	{
		if ((GA > 0) && (EA > 0))
		{
			node_index = input_address[GA][EA][0];
			//PRINTF4("%d-%d-0,%s的状态是:%d",GA,EA,gn_name(node_index),gn_state(node_index));
			/*第一路不为空*/
			if (node_index != NO_INDEX)
			{
				/*道岔*/
				if (IsTRUE(is_switch(node_index)))
				{
					node_index2 = (int16_t)signal_nodes[node_index].property;
					/*检查状态数据正确*/
					if ((temp1 == SWS_ERROR) || (temp1 == SWS_NO_INDICATE) 
						|| (temp1 == SWS_NORMAL) || (temp1 == SWS_REVERSE))
					{
						if (temp1 != gn_switch_state(node_index))
						{
							CIRemoteLog_Write("%s#状态:%d",gn_name(node_index),temp1);
						}
						if ((temp1 == SWS_ERROR) && (gn_state(node_index) != SWS_ERROR))
						{
							CIRemoteLog_Write("通信中断：%s",gn_name(node_index));
						}
						sn_state(node_index,temp1);
						if (signal_nodes[node_index].time_type == DTT_ERROR)
						{
							sn_stop_timer(node_index);
						}
					}
					/*道岔状态数据错误*/
					else
					{
						/*开始计时并判断是否到达3个周期*/
						if (IsFALSE(is_node_timer_run(node_index)))
						{
							sn_start_timer(node_index,MAX_ERROR_PER_COMMAND*CI_CYCLE_MS,DTT_ERROR);
						}
						if (IsTRUE(is_node_complete_timer(node_index)))
						{
							/*设备转为安全态*/
							sn_state(node_index,SWS_ERROR);
							sn_stop_timer(node_index);
							process_warning(ERR_SWITCH_DEVICE_ERR,gn_name(node_index));
							//CIHmi_SendNormalTips("通信中断：%s",gn_name(node_index));
						}
					}
					if (node_index2 != NO_INDEX)
					{
						/*检查状态数据正确*/
						if ((temp1 == SWS_ERROR) || (temp1 == SWS_NO_INDICATE) 
							|| (temp1 == SWS_NORMAL) || (temp1 == SWS_REVERSE))
						{
							if (temp1 != gn_switch_state(node_index2))
							{
								CIRemoteLog_Write("%s#状态:%d",gn_name(node_index2),temp1);
							}
							if ((temp1 == SWS_ERROR) && (gn_state(node_index2) != SWS_ERROR))
							{
								CIRemoteLog_Write("通信中断：%s",gn_name(node_index2));
							}
							sn_state(node_index2,temp1);
							if (signal_nodes[node_index2].time_type == DTT_ERROR)
							{
								sn_stop_timer(node_index2);
							}
						}
						/*道岔状态数据错误*/
						else
						{
							/*开始计时并判断是否到达3个周期*/
							if (IsFALSE(is_node_timer_run(node_index2)))
							{
								sn_start_timer(node_index2,MAX_ERROR_PER_COMMAND*CI_CYCLE_MS,DTT_ERROR);
							}
							if (IsTRUE(is_node_complete_timer(node_index2)))
							{
								/*设备转为安全态*/
								sn_state(node_index2,SWS_ERROR);
								sn_stop_timer(node_index2);
								process_warning(ERR_SWITCH_DEVICE_ERR,gn_name(node_index2));
								//CIHmi_SendNormalTips("通信中断：%s",gn_name(node_index2));
							}
						}
					}
				}
				/*信号*/
				else if (IsTRUE(is_signal(node_index)))
				{
					/*调车信号*/
					if (IsTRUE(is_shunting_signal(node_index)))
					{
						node_index2 = input_address[GA][EA][1];
						/*检查状态数据正确*/
						if ((temp1 == SGS_ERROR) || (temp1 == SGS_FILAMENT_BREAK) 
							|| (temp1 == SGS_B) || (temp1 == SGS_A))
						{
							if (temp1 != gn_signal_state(node_index))
							{
								CIRemoteLog_Write("%s状态:%d",gn_name(node_index),temp1);
							}
							if ((temp1 == SGS_ERROR) && (gn_state(node_index) != SGS_ERROR))
							{
								CIRemoteLog_Write("通信中断：%s",gn_name(node_index));
							}
							if ((temp1 == SGS_FILAMENT_BREAK) && (gn_state(node_index) != SGS_FILAMENT_BREAK))
							{
								CIHmi_SendNormalTips("断丝：%s",gn_name(node_index));
							}
							sn_state(node_index,temp1);
							if (signal_nodes[node_index].time_type == DTT_ERROR)
							{
								sn_stop_timer(node_index);
							}
						}
						/*状态数据错误*/
						else
						{
							/*开始计时并判断是否到达3个周期*/
							if (IsFALSE(is_node_timer_run(node_index)))
							{
								sn_start_timer(node_index,MAX_ERROR_PER_COMMAND*CI_CYCLE_MS,DTT_ERROR);
							}
							if (IsTRUE(is_node_complete_timer(node_index)))
							{
								/*设备转为安全态*/
								sn_state(node_index,SGS_ERROR);
								sn_stop_timer(node_index);
								process_warning(ERR_SIGNAL_DEVICE_ERR,gn_name(node_index));
								//CIHmi_SendNormalTips("通信中断：%s",gn_name(node_index));
							}
						}

						if (node_index2 != NO_INDEX)
						{
							/*检查第2路状态数据正确*/
							if ((temp2 == SGS_ERROR) || (temp2 == SGS_FILAMENT_BREAK) 
								|| (temp2 == SGS_B) || (temp2 == SGS_A))
							{
								if (temp2 != gn_signal_state(node_index2))
								{
									CIRemoteLog_Write("%s状态:%d",gn_name(node_index2),temp2);
								}
								if ((temp2 == SGS_ERROR) && (gn_state(node_index2) != SGS_ERROR))
								{
									CIRemoteLog_Write("通信中断：%s",gn_name(node_index2));
								}
								if ((temp2 == SGS_FILAMENT_BREAK) && (gn_state(node_index2) != SGS_FILAMENT_BREAK))
								{
									CIHmi_SendNormalTips("断丝：%s",gn_name(node_index2));
								}
								sn_state(node_index2,temp2);
								if (signal_nodes[node_index2].time_type == DTT_ERROR)
								{
									sn_stop_timer(node_index2);
								}
							}
							/*状态数据错误*/
							else
							{
								/*开始计时并判断是否到达3个周期*/
								if (IsFALSE(is_node_timer_run(node_index2)))
								{
									sn_start_timer(node_index2,MAX_ERROR_PER_COMMAND*CI_CYCLE_MS,DTT_ERROR);
								}
								if (IsTRUE(is_node_complete_timer(node_index2)))
								{
									/*设备转为安全态*/
									sn_state(node_index2,SGS_ERROR);
									sn_stop_timer(node_index2);
									process_warning(ERR_SIGNAL_DEVICE_ERR,gn_name(node_index2));
									//CIHmi_SendNormalTips("通信中断：%s",gn_name(node_index2));
								}
							}
						}				
					}
					/*列车信号*/
					else
					{
						/*检查状态数据正确*/
						if ((temp1 == SGS_ERROR) || (temp1 == SGS_FILAMENT_BREAK) 
							|| (temp1 == SGS_H) || (temp1 == SGS_U) || (temp1 == SGS_UU)
							|| (temp1 == SGS_LU) || (temp1 == SGS_L) || (temp1 == SGS_LL)
							|| (temp1 == SGS_USU) || (temp1 == SGS_YB))
						{
							if (temp1 != gn_signal_state(node_index))
							{
								CIRemoteLog_Write("%s状态:%d",gn_name(node_index),temp1);
							}
							if ((temp1 == SGS_ERROR) && (gn_state(node_index) != SGS_ERROR))
							{
								CIRemoteLog_Write("通信中断：%s",gn_name(node_index));
							}
							if ((temp1 == SGS_FILAMENT_BREAK) && (gn_state(node_index) != SGS_FILAMENT_BREAK))
							{
								CIHmi_SendNormalTips("断丝：%s",gn_name(node_index));
							}
							sn_state(node_index,temp1);
							if (signal_nodes[node_index].time_type == DTT_ERROR)
							{
								sn_stop_timer(node_index);
							}
						}
						else if ((temp1 == SGS_B) && ((NT_OUT_SHUNTING_SIGNAL == gn_type(node_index)) 
							|| (NT_ROUTE_SIGNAL == gn_type(node_index)) || (NT_HUMPING_SIGNAL == gn_type(node_index))))
						{
							if (temp1 != gn_signal_state(node_index))
							{
								CIRemoteLog_Write("%s状态:%d",gn_name(node_index),temp1);
							}
							sn_state(node_index,temp1);
							if (signal_nodes[node_index].time_type == DTT_ERROR)
							{
								sn_stop_timer(node_index);
							}
						}
						/*状态数据错误*/
						else
						{
							/*开始计时并判断是否到达3个周期*/
							if (IsFALSE(is_node_timer_run(node_index)))
							{
								sn_start_timer(node_index,MAX_ERROR_PER_COMMAND*CI_CYCLE_MS,DTT_ERROR);
							}
							if (IsTRUE(is_node_complete_timer(node_index)))
							{
								/*设备转为安全态*/
								sn_state(node_index,SGS_ERROR);
								if (signal_nodes[node_index].time_type == DTT_ERROR)
								{
									sn_stop_timer(node_index);
								}
								process_warning(ERR_SIGNAL_DEVICE_ERR,gn_name(node_index));
								//CIHmi_SendNormalTips("通信中断：%s",gn_name(node_index));
							}
						}

						/*hjh 20150429 三显示出站是一个模块两路信号*/
						node_index2 = input_address[GA][EA][1];
						if (node_index2 != NO_INDEX)
						{
							/*检查状态数据正确*/
							if ((temp2 == SGS_ERROR) || (temp2 == SGS_FILAMENT_BREAK) 
								|| (temp2 == SGS_H) || (temp2 == SGS_U) || (temp2 == SGS_UU)
								|| (temp2 == SGS_LU) || (temp2 == SGS_L) || (temp2 == SGS_LL)
								|| (temp2 == SGS_USU) || (temp2 == SGS_YB))
							{
								if (temp2 != gn_signal_state(node_index2))
								{
									CIRemoteLog_Write("%s状态:%d",gn_name(node_index2),temp2);
								}
								if ((temp2 == SGS_ERROR) && (gn_state(node_index2) != SGS_ERROR))
								{
									CIRemoteLog_Write("通信中断：%s",gn_name(node_index2));
								}
								if ((temp2 == SGS_FILAMENT_BREAK) && (gn_state(node_index2) != SGS_FILAMENT_BREAK))
								{
									CIHmi_SendNormalTips("断丝：%s",gn_name(node_index2));
								}
								sn_state(node_index2,temp2);
								if (signal_nodes[node_index2].time_type == DTT_ERROR)
								{
									sn_stop_timer(node_index2);
								}
							}
							else if ((temp2 == SGS_B) && ((NT_OUT_SHUNTING_SIGNAL == gn_type(node_index2)) 
								|| (NT_ROUTE_SIGNAL == gn_type(node_index2)) || (NT_HUMPING_SIGNAL == gn_type(node_index2))))
							{
								if (temp2 != gn_signal_state(node_index2))
								{
									CIRemoteLog_Write("%s状态:%d",gn_name(node_index2),temp2);
								}
								sn_state(node_index2,temp2);
								if (signal_nodes[node_index2].time_type == DTT_ERROR)
								{
									sn_stop_timer(node_index2);
								}
							}
							/*状态数据错误*/
							else
							{
								/*开始计时并判断是否到达3个周期*/
								if (IsFALSE(is_node_timer_run(node_index2)))
								{
									sn_start_timer(node_index2,MAX_ERROR_PER_COMMAND*CI_CYCLE_MS,DTT_ERROR);
								}
								if (IsTRUE(is_node_complete_timer(node_index2)))
								{
									/*设备转为安全态*/
									sn_state(node_index2,SGS_ERROR);
									if (signal_nodes[node_index2].time_type == DTT_ERROR)
									{
										sn_stop_timer(node_index2);
									}
									process_warning(ERR_SIGNAL_DEVICE_ERR,gn_name(node_index2));
									//CIHmi_SendNormalTips("通信中断：%s",gn_name(node_index2));
								}
							}
						}
					}
				}
				/*轨道区段*/
				else if (IsTRUE(is_section(node_index)))
				{
					node_index2 = input_address[GA][EA][1];
					/*检查状态数据正确*/
					if ((temp1 == SCS_ERROR) || (temp1 == SCS_CLEARED) || (temp1 != SCS_CLEARED))
					{
						if (temp1 != gn_section_state(node_index))
						{
							CIRemoteLog_Write("%s状态:%d",gn_name(node_index),temp1);
						}
						if ((temp1 == SCS_ERROR) && (gn_state(node_index) != SCS_ERROR))
						{
							CIRemoteLog_Write("通信中断：%s",gn_name(node_index));
						}
						//sn_state(node_index,temp1);

						/*轨道采用零散模块采集邻站轨道条件*/
						for (i = 0; i < MAX_SPECIAL_INPUT; i++)
						{
							if ((special_input_config[i].OutputNodeIndex != NO_INDEX)
								&& (special_input_config[i].OutputNodeIndex == node_index))
							{
								if (special_input_config[i].InputNodeIndex != NO_INDEX)
								{
									if ((gn_state(special_input_config[i].InputNodeIndex) == special_input_config[i].InputNodeState)
										&& (special_input_config[i].InputNodeState != 0))
									{
										sn_state(node_index,special_input_config[i].OutputNodeState);
									}
									else
									{
										sn_state(node_index,SCS_OCCUPIED);
									}
								}								
								break;
							}
						}
						if (i == MAX_SPECIAL_INPUT)
						{
							sn_state(node_index,temp1);
						}

						if (signal_nodes[node_index].time_type == DTT_ERROR)
						{
							sn_stop_timer(node_index);
						}
					}
					/*状态数据错误*/
					else
					{
						/*开始计时并判断是否到达3个周期*/
						if (IsFALSE(is_node_timer_run(node_index)))
						{
							sn_start_timer(node_index,MAX_ERROR_PER_COMMAND*CI_CYCLE_MS,DTT_ERROR);
						}
						if (IsTRUE(is_node_complete_timer(node_index)))
						{
							/*设备转为安全态*/
							sn_state(node_index,SCS_ERROR);
							sn_stop_timer(node_index);
							process_warning(ERR_SECTION_DEVICE_ERR,gn_name(node_index));
							//CIHmi_SendNormalTips("通信中断：%s",gn_name(node_index));
						}
					}

					if (node_index2 != NO_INDEX)
					{
						/*检查第2路状态数据正确*/
						if ((temp2 == SCS_ERROR) || (temp2 == SCS_CLEARED) || (temp2 != SCS_CLEARED))
						{
							if (temp2 != gn_section_state(node_index2))
							{
								CIRemoteLog_Write("%s状态:%d",gn_name(node_index2),temp2);
							}
							if ((temp2 == SCS_ERROR) && (gn_state(node_index2) != SCS_ERROR))
							{
								CIRemoteLog_Write("通信中断：%s",gn_name(node_index2));
							}
							//sn_state(node_index2,temp2);

							/*轨道采用零散模块采集邻站轨道条件*/
							for (i = 0; i < MAX_SPECIAL_INPUT; i++)
							{
								if ((special_input_config[i].OutputNodeIndex != NO_INDEX)
									&& (special_input_config[i].OutputNodeIndex == node_index2))
								{
									if (special_input_config[i].InputNodeIndex != NO_INDEX)
									{
										if ((gn_state(special_input_config[i].InputNodeIndex) == special_input_config[i].InputNodeState)
											&& (special_input_config[i].InputNodeState != 0))
										{
											sn_state(node_index2,special_input_config[i].OutputNodeState);
										}
										else
										{
											sn_state(node_index2,SCS_OCCUPIED);
										}
									}								
									break;
								}
							}
							if (i == MAX_SPECIAL_INPUT)
							{
								sn_state(node_index2,temp2);
							}

							if (signal_nodes[node_index2].time_type == DTT_ERROR)
							{
								sn_stop_timer(node_index2);
							}
						}
						/*状态数据错误*/
						else
						{
							/*开始计时并判断是否到达3个周期*/
							if (IsFALSE(is_node_timer_run(node_index2)))
							{
								sn_start_timer(node_index2,MAX_ERROR_PER_COMMAND*CI_CYCLE_MS,DTT_ERROR);
							}
							if (IsTRUE(is_node_complete_timer(node_index2)))
							{
								/*设备转为安全态*/
								sn_state(node_index2,SCS_ERROR);
								sn_stop_timer(node_index2);
								process_warning(ERR_SECTION_DEVICE_ERR,gn_name(node_index2));
								//CIHmi_SendNormalTips("通信中断：%s",gn_name(node_index2));
							}
						}
					}
				}
				/*总启动*/
				else if (gn_type(node_index) == NT_ZQD)
				{
					/*检查状态数据正确*/
					if ((temp1 == SZQD_ERROR) || (temp1 == SZQD_HAS_SIGNAL) || (temp1 == SZQD_NO_SIGNAL))
					{
						if (temp1 != gn_state(node_index))
						{
							CIRemoteLog_Write("%s状态:%d",gn_name(node_index),temp1);
						}
						if ((temp1 == SZQD_ERROR) && (gn_state(node_index) != SZQD_ERROR))
						{
							CIRemoteLog_Write("通信中断：%s",gn_name(node_index));
						}
						sn_state(node_index,temp1);
						if (signal_nodes[node_index].time_type == DTT_ERROR)
						{
							sn_stop_timer(node_index);
						}
					}
					/*状态数据错误*/
					else
					{
						/*开始计时并判断是否到达3个周期*/
						if (IsFALSE(is_node_timer_run(node_index)))
						{
							sn_start_timer(node_index,MAX_ERROR_PER_COMMAND*CI_CYCLE_MS,DTT_ERROR);
						}
						if (IsTRUE(is_node_complete_timer(node_index)))
						{
							/*设备转为安全态*/
							sn_state(node_index,SZQD_ERROR);
							if (signal_nodes[node_index].time_type == DTT_ERROR)
							{
								sn_stop_timer(node_index);
							}
							process_warning(ERR_ZQD_ERR,gn_name(node_index));
							//CIHmi_SendNormalTips("通信中断：%s",gn_name(node_index));
						}
					}
				}
				/*照查点*/
				else
				{
					node_index2 = input_address[GA][EA][1];
					/*检查状态数据正确*/
					if ((temp1 == SIO_ERROR) || (temp1 == SIO_HAS_SIGNAL) || (temp1 == SIO_NO_SIGNAL))
					{
						if (temp1 != gn_state(node_index))
						{
							CIRemoteLog_Write("%s状态:%d",gn_name(node_index),temp1);
						}
						if ((temp1 == SIO_ERROR) && (gn_state(node_index) != SIO_ERROR))
						{
							CIRemoteLog_Write("通信中断：%s",gn_name(node_index));
						}
						sn_state(node_index,temp1);

						/*特殊输出*/
						for (i = 0; i < MAX_SPECIAL_INPUT; i++)
						{
							if ((special_output_config[i].InputNodeIndex != NO_INDEX)
								&& (special_output_config[i].InputNodeIndex == node_index))
							{
								if ((temp1 == special_output_config[i].InputNodeState)
									&& (special_output_config[i].InputNodeState != 0))
								{
									if (special_output_config[i].OutputNodeIndex != NO_INDEX)
									{
										send_command(special_output_config[i].OutputNodeIndex,special_output_config[i].OutputNodeState);
									}									
								}
								if (temp1 != special_output_config[i].InputNodeState)
								{
									if (special_output_config[i].OutputNodeIndex != NO_INDEX)
									{
										send_command(special_output_config[i].OutputNodeIndex,special_output_config[i].OutputNodeState);
										if (special_output_config[i].OutputNodeState == SIO_HAS_SIGNAL)
										{
											send_command(special_output_config[i].OutputNodeIndex,SIO_NO_SIGNAL);
										}
										else
										{
											send_command(special_output_config[i].OutputNodeIndex,SIO_HAS_SIGNAL);
										}
									}									
								}
								break;
							}
						}

						/*轨道合并*/
						for (i = 0; i < MAX_SECTION_COMPOSE; i++)
						{
							if ((section_compose_config[i].Index0 != NO_INDEX)
								&& (section_compose_config[i].Index0 == node_index))
							{
								if ((gn_state(section_compose_config[i].Index1) == section_compose_config[i].State1)
									&& (gn_state(section_compose_config[i].Index2) == section_compose_config[i].State2)
									&& (gn_state(section_compose_config[i].Index3) == section_compose_config[i].State3))
								{
									sn_state(node_index,section_compose_config[i].State0);
								}
							}
						}

						if (signal_nodes[node_index].time_type == DTT_ERROR)
						{
							sn_stop_timer(node_index);
						}
					}
					/*状态数据错误*/
					else
					{
						/*开始计时并判断是否到达3个周期*/
						if (IsFALSE(is_node_timer_run(node_index)))
						{
							sn_start_timer(node_index,MAX_ERROR_PER_COMMAND*CI_CYCLE_MS,DTT_ERROR);
						}
						if (IsTRUE(is_node_complete_timer(node_index)))
						{
							/*设备转为安全态*/
							sn_state(node_index,SIO_ERROR);
							sn_stop_timer(node_index);
							process_warning(ERR_LIAISON_DEVICE_ERR,gn_name(node_index));
							//CIHmi_SendNormalTips("通信中断：%s",gn_name(node_index));
						}
					}

					if (node_index2 != NO_INDEX)
					{
						/*检查第2路状态数据正确*/
						if ((temp2 == SIO_ERROR) || (temp2 == SIO_HAS_SIGNAL) || (temp2 == SIO_NO_SIGNAL))
						{
							if (temp2 != gn_state(node_index2))
							{
								CIRemoteLog_Write("%s状态:%d",gn_name(node_index2),temp2);
							}
							if ((temp2 == SIO_ERROR) && (gn_state(node_index2) != SIO_ERROR))
							{
								CIRemoteLog_Write("通信中断：%s",gn_name(node_index2));
							}
							sn_state(node_index2,temp2);
							
							/*特殊输出*/
							for (i = 0; i < MAX_SPECIAL_INPUT; i++)
							{
								if ((special_output_config[i].InputNodeIndex != NO_INDEX)
									&& (special_output_config[i].InputNodeIndex == node_index2))
								{
									if (temp2 == special_input_config[i].InputNodeState)
									{
										if (special_output_config[i].OutputNodeIndex != NO_INDEX)
										{
											send_command(special_output_config[i].OutputNodeIndex,special_output_config[i].OutputNodeState);
										}									
									}								
									break;
								}
							}

							/*轨道合并*/
							for (i = 0; i < MAX_SECTION_COMPOSE; i++)
							{
								if ((section_compose_config[i].Index0 != NO_INDEX)
									&& (section_compose_config[i].Index0 == node_index2))
								{
									if ((gn_state(section_compose_config[i].Index1) == section_compose_config[i].State1)
										&& (gn_state(section_compose_config[i].Index2) == section_compose_config[i].State2)
										&& (gn_state(section_compose_config[i].Index3) == section_compose_config[i].State3))
									{
										sn_state(node_index2,section_compose_config[i].State0);
									}
								}
							}

							if (signal_nodes[node_index2].time_type == DTT_ERROR)
							{
								sn_stop_timer(node_index2);
							}
						}
						/*状态数据错误*/
						else
						{
							/*开始计时并判断是否到达3个周期*/
							if (IsFALSE(is_node_timer_run(node_index2)))
							{
								sn_start_timer(node_index2,MAX_ERROR_PER_COMMAND*CI_CYCLE_MS,DTT_ERROR);
							}
							if (IsTRUE(is_node_complete_timer(node_index2)))
							{
								/*设备转为安全态*/
								sn_state(node_index2,SIO_ERROR);
								sn_stop_timer(node_index2);
								process_warning(ERR_LIAISON_DEVICE_ERR,gn_name(node_index2));
								//CIHmi_SendNormalTips("通信中断：%s",gn_name(node_index2));
							}
						}
					}
				}
			}
			/*第一路为空*/
			else
			{
				node_index2 = input_address[GA][EA][1];
				if (node_index2 != NO_INDEX)
				{
					/*信号*/
					if (IsTRUE(is_signal(node_index2)))
					{
						/*调车信号*/
						if (IsTRUE(is_shunting_signal(node_index2)))
						{
							/*检查第2路状态数据正确*/
							if ((temp2 == SGS_ERROR) || (temp2 == SGS_FILAMENT_BREAK) 
								|| (temp2 == SGS_B) || (temp2 == SGS_A))
							{
								if (temp2 != gn_signal_state(node_index2))
								{
									CIRemoteLog_Write("%s状态:%d",gn_name(node_index2),temp2);
								}
								if ((temp2 == SGS_ERROR) && (gn_state(node_index2) != SGS_ERROR))
								{
									CIRemoteLog_Write("通信中断：%s",gn_name(node_index2));
								}
								if ((temp2 == SGS_FILAMENT_BREAK) && (gn_state(node_index2) != SGS_FILAMENT_BREAK))
								{
									CIHmi_SendNormalTips("断丝：%s",gn_name(node_index2));
								}
								sn_state(node_index2,temp2);
								if (signal_nodes[node_index2].time_type == DTT_ERROR)
								{
									sn_stop_timer(node_index2);
								}
							}
							/*状态数据错误*/
							else
							{
								/*开始计时并判断是否到达3个周期*/
								if (IsFALSE(is_node_timer_run(node_index2)))
								{
									sn_start_timer(node_index2,MAX_ERROR_PER_COMMAND*CI_CYCLE_MS,DTT_ERROR);
								}
								if (IsTRUE(is_node_complete_timer(node_index2)))
								{
									/*设备转为安全态*/
									sn_state(node_index2,SGS_ERROR);
									sn_stop_timer(node_index2);
									process_warning(ERR_SIGNAL_DEVICE_ERR,gn_name(node_index2));
									//CIHmi_SendNormalTips("通信中断：%s",gn_name(node_index2));
								}
							}
						}
						/*列车信号*/
						else
						{
							/*检查状态数据正确*/
							if ((temp2 == SGS_ERROR) || (temp2 == SGS_FILAMENT_BREAK) 
								|| (temp2 == SGS_H) || (temp2 == SGS_U) || (temp2 == SGS_UU)
								|| (temp2 == SGS_LU) || (temp2 == SGS_L) || (temp2 == SGS_LL)
								|| (temp2 == SGS_USU) || (temp2 == SGS_YB))
							{
								if (temp2 != gn_signal_state(node_index2))
								{
									CIRemoteLog_Write("%s状态:%d",gn_name(node_index2),temp2);
								}
								if ((temp2 == SGS_ERROR) && (gn_state(node_index2) != SGS_ERROR))
								{
									CIRemoteLog_Write("通信中断：%s",gn_name(node_index2));
								}
								if ((temp2 == SGS_FILAMENT_BREAK) && (gn_state(node_index2) != SGS_FILAMENT_BREAK))
								{
									CIHmi_SendNormalTips("断丝：%s",gn_name(node_index2));
								}
								sn_state(node_index2,temp2);
								if (signal_nodes[node_index2].time_type == DTT_ERROR)
								{
									sn_stop_timer(node_index2);
								}
							}
							else if ((temp2 == SGS_B) && ((NT_OUT_SHUNTING_SIGNAL == gn_type(node_index2)) 
								|| (NT_ROUTE_SIGNAL == gn_type(node_index2)) || (NT_HUMPING_SIGNAL == gn_type(node_index2))))
							{
								if (temp2 != gn_signal_state(node_index2))
								{
									CIRemoteLog_Write("%s状态:%d",gn_name(node_index2),temp2);
								}
								sn_state(node_index2,temp2);
								if (signal_nodes[node_index2].time_type == DTT_ERROR)
								{
									sn_stop_timer(node_index2);
								}
							}
							/*状态数据错误*/
							else
							{
								/*开始计时并判断是否到达3个周期*/
								if (IsFALSE(is_node_timer_run(node_index2)))
								{
									sn_start_timer(node_index2,MAX_ERROR_PER_COMMAND*CI_CYCLE_MS,DTT_ERROR);
								}
								if (IsTRUE(is_node_complete_timer(node_index2)))
								{
									/*设备转为安全态*/
									sn_state(node_index2,SGS_ERROR);
									if (signal_nodes[node_index2].time_type == DTT_ERROR)
									{
										sn_stop_timer(node_index2);
									}
									process_warning(ERR_SIGNAL_DEVICE_ERR,gn_name(node_index2));
									//CIHmi_SendNormalTips("通信中断：%s",gn_name(node_index2));
								}
							}
						}
					}
					/*轨道区段*/
					else if (IsTRUE(is_section(node_index2)))
					{
						/*检查第2路状态数据正确*/
						if ((temp2 == SCS_ERROR) || (temp2 == SCS_CLEARED) || (temp2 != SCS_CLEARED))
						{
							if (temp2 != gn_section_state(node_index2))
							{
								CIRemoteLog_Write("%s状态:%d",gn_name(node_index2),temp2);
							}
							if ((temp2 == SCS_ERROR) && (gn_state(node_index2) != SCS_ERROR))
							{
								CIRemoteLog_Write("通信中断：%s",gn_name(node_index2));
							}
							sn_state(node_index2,temp2);
							if (signal_nodes[node_index2].time_type == DTT_ERROR)
							{
								sn_stop_timer(node_index2);
							}							
						}
						/*状态数据错误*/
						else
						{
							/*开始计时并判断是否到达3个周期*/
							if (IsFALSE(is_node_timer_run(node_index2)))
							{
								sn_start_timer(node_index2,MAX_ERROR_PER_COMMAND*CI_CYCLE_MS,DTT_ERROR);
							}
							if (IsTRUE(is_node_complete_timer(node_index2)))
							{
								/*设备转为安全态*/
								sn_state(node_index2,SCS_ERROR);
								sn_stop_timer(node_index2);
								process_warning(ERR_SECTION_DEVICE_ERR,gn_name(node_index2));
								//CIHmi_SendNormalTips("通信中断：%s",gn_name(node_index2));
							}
						}
					}
					/*照查点*/
					else
					{
						/*检查第2路状态数据正确*/
						if ((temp2 == SIO_ERROR) || (temp2 == SIO_HAS_SIGNAL) || (temp2 == SIO_NO_SIGNAL))
						{
							if (temp2 != gn_state(node_index2))
							{
								CIRemoteLog_Write("%s状态:%d",gn_name(node_index2),temp2);
							}
							if ((temp2 == SIO_ERROR) && (gn_state(node_index2) != SIO_ERROR))
							{
								CIRemoteLog_Write("通信中断：%s",gn_name(node_index2));
							}
							sn_state(node_index2,temp2);
							if (signal_nodes[node_index2].time_type == DTT_ERROR)
							{
								sn_stop_timer(node_index2);
							}							
						}
						/*状态数据错误*/
						else
						{
							/*开始计时并判断是否到达3个周期*/
							if (IsFALSE(is_node_timer_run(node_index2)))
							{
								sn_start_timer(node_index2,MAX_ERROR_PER_COMMAND*CI_CYCLE_MS,DTT_ERROR);
							}
							if (IsTRUE(is_node_complete_timer(node_index2)))
							{
								/*设备转为安全态*/
								sn_state(node_index2,SIO_ERROR);
								sn_stop_timer(node_index2);
								process_warning(ERR_LIAISON_DEVICE_ERR,gn_name(node_index2));
								//CIHmi_SendNormalTips("通信中断：%s",gn_name(node_index2));
							}
						}
					}
				}				
			}
		}		
	}
}
/****************************************************
函数名:    sn_state
功能描述:  设置节点状态
返回值:    
参数:      node_t node_index
参数:      uint32_t status
作者  :    CY
日期  ：   2012/5/11
****************************************************/
void sn_state(node_t node_index, uint32_t status)
{
	EN_node_type ntype = gn_type(node_index);

	/*参数检查*/
	if ( node_index >= TOTAL_SIGNAL_NODE)
	{
		process_warning(ERR_INDEX,"");
		return ;
	}
	else
	{
#if defined WIN32
#else
		//gettimeofday(&signal_nodes[node_index].last_sc_time, NULL);
#endif
		/*参数检查*/
		if ((ntype == NT_NON_SWITCH_SECTION) || (ntype == NT_SWITCH_SECTION)
			|| (ntype == NT_TRACK) || (ntype ==  NT_STUB_END_SECTION) 
			|| (ntype ==  NT_APP_DEP_SECTION) || (ntype ==  NT_LOCODEPOT_SECTION))
		{
			/*hjh 2014-3-10 所有区段从占用恢复至空闲时均延时3s*/
			if ((signal_nodes[node_index].state != SCS_CLEARED) && (status == SCS_CLEARED))
			{
				set_clear_section(node_index);
			}
			/*hjh 2014-5-5修改*/
			///*检查节点所在的进路存在*/			
			//if (gn_belong_route(node_index) != NO_INDEX)
			//{
			//	/*hjh区段状态检查*/
			//	//if ((signal_nodes[node_index].state != SCS_CLEARED) && (status == SCS_CLEARED))
			//	//{
			//	//	set_clear_section(node_index);
			//	//	//PRINTF2("%s区段出清的周期是-------------------%d\n",gn_name(node_index), CICycleInt_GetCounter());
			//	//}
			//	/*区段状态检查*/
			//	if ((signal_nodes[node_index].state == SCS_CLEARED) && (status != SCS_CLEARED) )
			//	{
			//		signal_nodes[node_index].state = status;
			//		signal_nodes[node_index].history_state = CI_TRUE;
			//		/*hjh 2012-11-28 设置接近区段被占用过的时机是接近区段和进路内第一区段同时被占用*/
			//		route_index = gn_belong_route(node_index);
			//		/*当前区段是某一进路的第一区段*/
			//		if (gr_first_section(route_index) == node_index)
			//		{
			//			if (gr_approach(route_index) != NO_NODE)
			//			{
			//				/*进路的接近区段占用*/
			//				if (gn_section_state(gr_approach(route_index)) != SCS_CLEARED)
			//				{
			//					/*设置进路的接近区段为被占用过*/
			//					signal_nodes[gr_approach(route_index)].history_state = CI_TRUE;
			//				}
			//			}
			//		}
			//	}
			//	return ;
			//} 
			if (((signal_nodes[node_index].state == SCS_CLEARED) && (status != SCS_CLEARED))
				|| (status == SCS_ERROR)
				|| ((signal_nodes[node_index].state == SCS_ERROR) && ((status != SCS_CLEARED) || (status == SCS_CLEARED))))
			{
				signal_nodes[node_index].state = status;
			}
			return;

			//for (i = 0; i < MAX_ROUTE; i++)
			//{
			//	/*参数检查*/
			//	if (IsTRUE(is_route_exist(i)))
			//	{
			//		/*接近区段检查*/
			//		if (gr_approach(i) == node_index)
			//		{
			//			/*hjh设置节点状态*/
			//			/*if ((signal_nodes[node_index].state != SCS_CLEARED) && (status == SCS_CLEARED))
			//			{
			//				set_clear_section(node_index);
			//			}*/
			//			/*设置节点状态*/
			//			if ((signal_nodes[node_index].state == SCS_CLEARED) && (status != SCS_CLEARED) )
			//			{
			//				signal_nodes[node_index].state = status;
			//				//signal_nodes[node_index].history_state = CI_TRUE;							
			//			}
			//		}
			//	}
			//}
		}

		/*不在进路中的双动道岔的另一动处理*/
		if((gn_another_switch(node_index) != NO_INDEX) && (ntype == NT_SWITCH))
		{
			signal_nodes[gn_another_switch(node_index)].state = status;
		}
		if (gn_type(node_index) != NT_SEMI_AUTOMATIC_BLOCK)
		{
			signal_nodes[node_index].state = status;
		}		
	}
	
}


/****************************************************
函数名:    set_clear_section
功能描述:  设置进路中出清区段
返回值:    
参数:      node_t section_node
作者  :    CY
日期  ：   2012/5/11
****************************************************/
void set_clear_section(node_t section_node)
{
	int16_t i;

	/*设置进路中出清区段*/
	for (i = 0; i < MAX_CLEARING_SECTION ; i++ )
	{
		if(clear_sections[i].section == NO_INDEX)
		{
			clear_sections[i].section = section_node;
			clear_sections[i].start_time = CICycleInt_GetCounter();
			break;
		}
	}
}

/****************************************************
函数名:    clear_section
功能描述:  处理出清区段
返回值:    
作者  :    CY
日期  ：   2012/5/11
****************************************************/
void clear_section(void)
{
	int16_t i;

	for (i = 0; i < MAX_CLEARING_SECTION ; i++ )
	{
		/*参数检查*/
		if(clear_sections[i].section != NO_INDEX)
		{
			/*处理出清区段*/
			if ((CICycleInt_GetCounter() - clear_sections[i].start_time) >= (SECONDS_3/CI_CYCLE_MS - 1000/CI_CYCLE_MS + 1))
			{
				signal_nodes[clear_sections[i].section].state = SCS_CLEARED;
				clear_sections[i].section = NO_INDEX;
				clear_sections[i].start_time = NO_TIMER;				
			}
		}
	}
}

/****************************************************
函数名:    send_node_timer
功能描述:  发送节点计时信息
返回值:    
作者  :    CY
日期  ：   2012/7/16
****************************************************/
void send_node_timer(void)
{
	char_t timer_str[1024];
	string pos = timer_str;
	int16_t i;

	sprintf(pos,"Timer:");
	pos += strlen(timer_str);
	for (i = 0; i < TOTAL_SIGNAL_NODE; i++)
	{
		/*参数检查*/
		if (IsTRUE(is_node_timer_run(i)))
		{
			/*hjh 2014-2-28 倒计时显示增加判断条件*/
			if (signal_nodes[i].time_count >= CICycleInt_GetCounter())
			{
				if ((signal_nodes[i].time_type == DTT_UNLOCK || (signal_nodes[i].time_type == DTT_CLOSE_GUIDE)))
				{
					sprintf(pos," %s: %d ; ",gn_name(i), (signal_nodes[i].time_count - CICycleInt_GetCounter()) * CI_CYCLE_MS / 1000);
					pos = timer_str + strlen(timer_str);
				}				
				if (signal_nodes[i].time_count == CICycleInt_GetCounter())
				{
					signal_nodes[i].time_count = 0;
				}
			}
			else
			{
				signal_nodes[i].time_count = 0;
			}			
		}
	}
	/*发送节点计时信息*/
	if (pos > timer_str+strlen("Timer:"))
	{
		*(--pos) = 0;
		*(--pos) = 0;
		CIHmi_SendTimerTips(timer_str);
	}	
}


/****************************************************
函数名:    get_button_name
功能描述:  设置按钮名称
返回值:  
参数:      button
参数:      name

作者  :    LYC
日期  ：   2012/9/18
****************************************************/
void get_button_name(int16_t button, string name)
{
	/*参数检查*/
	if ((button < 0) && (button > FB_INIT_DATA1))
	{
		sprintf(name,"%s",function_button_name[-1-button]);
	} 
	else
	{
		if ((button > 0) && (button < TOTAL_NAMES))
		{
			sprintf(name,"%s",device_name[button]);
		}			
	}
}
/****************************************************
函数名:    send_pressed_buttons
功能描述:  发送当前按下的按钮
返回值:    
作者  :    CY
日期  ：   2012/7/19
****************************************************/
void send_pressed_buttons(void)
{
	char_t timer_str[60] = {0};
	string pos = timer_str;

	/*sprintf(pos,"PressedButtons:");
	pos += strlen(timer_str);*/
	/*判断第一个按钮存在*/
	if (first_button != NO_INDEX)
	{
		get_button_name(first_button,pos);
		pos = timer_str + strlen(timer_str);
		/*判断第二个按钮存在*/
		if (second_button != NO_INDEX)
		{
			*pos = ':';
			pos ++;
			get_button_name(second_button,pos);
			pos = timer_str + strlen(timer_str);
			/*判断第三个按钮存在*/
			if (third_button != NO_INDEX)
			{
				*pos = ':';
				pos ++;
				get_button_name(third_button,pos);
				pos = timer_str + strlen(timer_str);
				/*判断第四个按钮存在*/
				if (fourth_button != NO_INDEX)
				{
					*pos = ':';
					pos ++;
					get_button_name(fourth_button,pos);
					pos = timer_str + strlen(timer_str);

				}
			}
		}

		/*发送数据*/
		CIHmi_SendTimerTips(timer_str);		
	}		
}

/****************************************************
函数名：   default_safe_process
功能描述： 故障—安全处理
返回值：   void
参数：     uint16_t GA
参数：     uint16_t EA
作者：	   hejh
日期：     2014/09/15
****************************************************/
void default_safe_process(uint16_t GA, uint16_t EA)
{
	node_t node_index = NO_INDEX,node_index2 = NO_INDEX;
	uint16_t i;

	/*参数检查*/
	if ((GA <= 0) || (GA >= TOTAL_GETWAY) || (EA <= 0) || (EA >= MAX_EEU_PER_LAYER))
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		for (i = 0; i < TOTAL_SIGNAL_NODE; i++)
		{
			if ((((signal_nodes[i].input_address >> 8) & 0xFF) == GA)
				&& (((signal_nodes[i].input_address >> 2) & 0x3F) == EA))
			{
				node_index = i;
				/*道岔*/
				if (IsTRUE(is_switch(node_index)))
				{
					/*第一路道岔*/
					if (gn_state(node_index) != SWS_ERROR)
					{
						CIRemoteLog_Write("通信中断：%s",gn_name(node_index));
					}
					sn_state(node_index,SWS_ERROR);
					//sn_stop_timer(node_index);
					if (signal_nodes[node_index].time_type == DTT_ERROR)
					{
						sn_stop_timer(node_index);
					}
					/*双动道岔*/
					node_index2 = (int16_t)signal_nodes[node_index].property;
					if (node_index2 != NO_INDEX)
					{
						if (gn_state(node_index2) != SWS_ERROR)
						{
							CIRemoteLog_Write("通信中断：%s",gn_name(node_index2));
						}
						sn_state(node_index2,SWS_ERROR);
						//sn_stop_timer(node_index2);
						if (signal_nodes[node_index2].time_type == DTT_ERROR)
						{
							sn_stop_timer(node_index2);
						}
					}
				}
				/*信号*/
				else if (IsTRUE(is_signal(node_index)))
				{
					/*调车信号*/
					if (IsTRUE(is_shunting_signal(node_index)))
					{
						if (gn_state(node_index) != SWS_ERROR)
						{
							CIRemoteLog_Write("通信中断：%s",gn_name(node_index));
						}
						sn_state(node_index,SGS_ERROR);
						//sn_stop_timer(node_index);
						if (signal_nodes[node_index].time_type == DTT_ERROR)
						{
							sn_stop_timer(node_index);
						}
					}
					/*列车信号*/
					else
					{
						if (gn_state(node_index) != SWS_ERROR)
						{
							CIRemoteLog_Write("通信中断：%s",gn_name(node_index));
						}
						sn_state(node_index,SGS_ERROR);
						//sn_stop_timer(node_index);
						if (signal_nodes[node_index].time_type == DTT_ERROR)
						{
							sn_stop_timer(node_index);
						}
					}
				}
				/*轨道区段*/
				else if (IsTRUE(is_section(node_index)))
				{
					if (gn_state(node_index) != SWS_ERROR)
					{
						CIRemoteLog_Write("通信中断：%s",gn_name(node_index));
					}
					sn_state(node_index,SCS_ERROR);
					//sn_stop_timer(node_index);
					if (signal_nodes[node_index].time_type == DTT_ERROR)
					{
						sn_stop_timer(node_index);
					}
				}
				/*总启动*/
				else if (gn_type(node_index) == NT_ZQD)
				{
					if (gn_state(node_index) != SWS_ERROR)
					{
						CIRemoteLog_Write("通信中断：%s",gn_name(node_index));
					}
					sn_state(node_index,SZQD_ERROR);
					//sn_stop_timer(node_index);
					if (signal_nodes[node_index].time_type == DTT_ERROR)
					{
						sn_stop_timer(node_index);
					}
				}
				else
				{
					if (gn_state(node_index) != SWS_ERROR)
					{
						CIRemoteLog_Write("通信中断：%s",gn_name(node_index));
					}
					sn_state(node_index,SIO_ERROR);
					//sn_stop_timer(node_index);
					if (signal_nodes[node_index].time_type == DTT_ERROR)
					{
						sn_stop_timer(node_index);
					}
				}
			}
		}
	}	
}

/****************************************************
函数名:    gn_inputaddress
功能描述:  获取信号节点的输入地址
返回值:    
参数:      int16_t node_index
作者  :    hejh
日期  ：   2012/10/16
****************************************************/
uint16_t gn_inputaddress(int16_t node_index)
{
	uint16_t result;

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
		result = 0;
	}
	else
	{
		result = signal_nodes[node_index].input_address;
	}
	return result;
}

/****************************************************
函数名:    is_out_signal
功能描述:  是否为出站信号机
返回值:    
参数:      int16_t node_index
作者  :    CY
日期  ：   2012/7/27
****************************************************/
CI_BOOL is_out_signal(int16_t node_index)
{
	CI_BOOL result = CI_FALSE;

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
		result = CI_FALSE;
	}
	else
	{
		/*判断是否为出站信号机*/
		if ((gn_type(node_index) == NT_OUT_SHUNTING_SIGNAL) || (gn_type(node_index) == NT_OUT_SIGNAL))
		{
			result = CI_TRUE;
		}
	}
	return result;
}

/****************************************************
函数名:    is_out_signal
功能描述:  是否为调车信号机
返回值:    
参数:      int16_t node_index
作者  :    CY
日期  ：   2012/7/27
****************************************************/
CI_BOOL is_shunting_signal(int16_t node_index)
{
	CI_BOOL result = CI_FALSE;

	/*参数检查*/
	if ((node_index < 0) || (node_index >= TOTAL_SIGNAL_NODE))
	{
		process_warning(ERR_INDEX,"");
		result = CI_FALSE;
	}
	else
	{
		/*判断是否为调车信号机*/
		if ((gn_type(node_index) ==  NT_SINGLE_SHUNTING_SIGNAL)
			|| (gn_type(node_index) == NT_JUXTAPOSE_SHUNGTING_SIGNAL)
			|| (gn_type(node_index) == NT_DIFF_SHUNTING_SIGNAL)
			|| (gn_type(node_index) == NT_STUB_END_SHUNTING_SIGNAL)
			|| (gn_type(node_index) == NT_LOCODEPOT_SHUNTING_SIGNAL))
		{
			result = CI_TRUE;
		}
	}
	return result;
}
/****************************************************
函数名:    is_exceed_limit_section
功能描述:  判断联锁表中某区段是否为侵限区段
返回值:    
参数:      route_t route_index
参数:      int16_t section_index
作者  :    CY
日期  ：   2012/7/27
****************************************************/
CI_BOOL is_exceed_limit_section(route_t route_index, int16_t section_ordinal)
{
	int16_t i,index;
	CI_BOOL result = CI_FALSE;

	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) ||
		(NO_INDEX == routes[route_index].ILT_index) ||
		(section_ordinal < 0) || (section_ordinal >= ILT[routes[route_index].ILT_index].track_count))
	{
		process_warning(ERR_INDEX,"");
		result = CI_TRUE;
	}
	else
	{
		/*判断联锁表中某区段是否为侵限区段*/
		if(ILT[routes[route_index].ILT_index].tracks[section_ordinal].conditon_switch.index != NO_INDEX)
		{
			return CI_TRUE;
		}
		/*判断联锁表中某区段是否为侵限区段*/
		//for (i = gr_nodes_count(route_index) - 1; i>=0 ; i--)
		//{
		//	index = gr_node(route_index,i);
		//	if ((gn_type(index) == NT_EXCEED_LIMIT) && 
		//		(((int16_t)(signal_nodes[index].property)) == ILT[routes[route_index].ILT_index].tracks[section_ordinal].index))
		//	{
		//		result = CI_TRUE;
		//		break;
		//	}
		//}
		
		/*hjh 2014-2-24 联锁表区段表中不在进路上的区段是侵限区段*/
		if (IsFALSE(result))
		{
			index = gr_section(route_index,section_ordinal);
			for (i = 0; i < gr_nodes_count(route_index); i ++)
			{
				if (IsTRUE(is_section(gr_node(route_index,i))) && (index == gr_node(route_index,i)))
				{
					result = CI_FALSE;
					break;
				}
				result = CI_TRUE;
			}
		}		
	}
	return result;
}

/****************************************************
函数名：   is_block_cleared
功能描述： 检查照查区间空闲
返回值：   CI_BOOL
参数：     route_t route_index
作者：	   hejh
日期：     2014/05/21
****************************************************/
CI_BOOL is_block_cleared( route_t route_index )
{
	int16_t temp,i;
	CI_BOOL result = CI_TRUE;
	node_t section = NO_INDEX;

	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) 
		|| (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
	}
	else
	{
		if ((temp = gs_special(route_index,SPT_STATE_COLLECT)) != NO_INDEX)
		{
			for (i = 0; i < gr_nodes_count(route_index); i++)
			{
				section = gr_node(route_index,i);
				if (IsTRUE(is_section(section)) && (section == state_collect_config[temp].section)
					&& (SCS_CLEARED != gn_section_state(section)))
				{
					result = CI_FALSE;
				}
			}
		}
	}
	return result;
}

/****************************************************
函数名:    memory_set
功能描述:  清除内存
返回值:    
参数:      void * _dst
参数:      int _val
参数:      size_t _size
作者  :    CY
日期  ：   2012/9/5
****************************************************/
void memory_set(void * dst, int32_t val, int32_t size)
{
	int32_t i ;
	uint8_t * c;
	uint8_t v = (uint8_t) val;

	/*参数检查*/
	if ((dst != 0) && (size > 0))
	{
		/*清除内存*/
		c = (uint8_t *)dst;
		for (i = 0; i < size; i++)
		{
			*c = v;
			c++;
		}
	}
}

/****************************************************
功能描述    : 将禁止信号清空，防止每次都发送该命令，
			  放在这里比较符合逻辑分层的思想。
返回值      : 
作者        : 何境泰
日期        : 2014年4月16日 14:59:20
****************************************************/
int32_t CIInter_ClearCommands(void)
{ 
	uint8_t i,j;
	uint16_t command_1, command_2;

	for (i = 0; i < TOTAL_GETWAY; i++)
	{
		for (j = 0; j < MAX_EEU_PER_LAYER; j++)
		{
			command_1 = commands[i][j] & 0x0000ffff;
			command_2 = commands[i][j] >> 16;

			if (SGS_A == command_1 || SGS_H == command_1)
			{
				commands[i][j] &= 0xffff0000;
			}
			if (SGS_A == command_2 || SGS_H == command_2)
			{
				commands[i][j] &= 0x0000ffff;
			}
		}
	}

    return 0;
}

/****************************************************
函数名：   gs_special
功能描述： &&&待删除！！！获取特殊联锁索引号
返回值：   int16_t
参数：     route_t route_index
参数：     EN_special_type type
作者：	   hejh
日期：     2015/03/13
****************************************************/
int16_t gs_special(route_t route_index, EN_special_type type)
{
	int16_t i,result = NO_INDEX;

	/*参数检查*/
	if ((route_index < 0) || (route_index >= MAX_ROUTE) 
		|| (NO_INDEX == routes[route_index].ILT_index))
	{
		process_warning(ERR_INDEX,"");
		result = NO_INDEX;
	}
	else
	{
		///*检查特殊联锁是否存在，若存在，则返回此特殊联锁的索引号*/
		//for (i =0; i < MAX_SPECIAL_PS ; i ++)
		//{
		//	if (ILT[routes[route_index].ILT_index].special_interlocking[i] != NO_INDEX &&
		//		special_config[ILT[routes[route_index].ILT_index].special_interlocking[i]].type == type)
		//	{
		//		result = special_config[ILT[routes[route_index].ILT_index].special_interlocking[i]].special;
		//	}
		//}

		if (type == SPT_SEMI_AUTO_BLOCK)
		{
			for (i = 0; i < total_semi_auto_block; i++)
			{
				if (semi_auto_block_config[i].entry_signal == gb_node(gr_end_button(route_index)))
				{
					result = i;
					break;
				}
			}
		}
	}
	/*若不存在则返回NO_INDEX*/
	return result;
}

/****************************************************
函数名：   OutputHmiNormalTips
功能描述： 输出至控显机提示信息
返回值：   void
参数：     char_t * tips
参数：     route_t route_index
作者：	   hejh
日期：     2015/05/26
****************************************************/
void OutputHmiNormalTips(char_t * tips,route_t route_index)
{
	/*输出提示信息*/
	if (gr_change_button(route_index) != NO_INDEX)
	{
		if (gr_change_button1(route_index) != NO_INDEX)
		{
			strcat_check(tips,"%s --> %s --> %s --> %s",TEST_NAME_LENGTH);
			CIHmi_SendNormalTips(tips,gn_name(gr_start_button(route_index)),gn_name(gr_change_button(route_index)),gn_name(gr_change_button1(route_index)),gn_name(gr_end_button(route_index)));
		}
		else
		{
			
			strcat_check(tips,"%s --> %s --> %s",TEST_NAME_LENGTH);
			CIHmi_SendNormalTips(tips,gn_name(gr_start_button(route_index)),gn_name(gr_change_button(route_index)),gn_name(gr_end_button(route_index)));
		}
	}
	else
	{
		strcat_check(tips,"%s --> %s",TEST_NAME_LENGTH);
		CIHmi_SendNormalTips(tips,gn_name(gr_start_button(route_index)),gn_name(gr_end_button(route_index)));
	}
}


/****************************************************
函数名：   set_lock_button
功能描述： 设置按钮封锁或解封
返回值：   void
参数：     int16_t button_index
参数：     CI_BOOL lock_flag
作者：	   hejh
日期：     2015/08/11
****************************************************/
void set_lock_button(int16_t button_index,CI_BOOL lock_flag)
{
	int16_t byteCount = 0,bitCount = 0;

	byteCount = (button_index - TOTAL_SIGNAL_NODE) / 8;
	bitCount = (button_index - TOTAL_SIGNAL_NODE) % 8;

	/*设置封锁标志*/
	if (IsTRUE(lock_flag))
	{
		switch (bitCount)
		{
			case 0:
				button_locked[byteCount] |= 0x01;
				break;
			case 1:
				button_locked[byteCount] |= 0x02;
				break;
			case 2:
				button_locked[byteCount] |= 0x04;
				break;
			case 3:
				button_locked[byteCount] |= 0x08;
				break;
			case 4:
				button_locked[byteCount] |= 0x10;
				break;
			case 5:
				button_locked[byteCount] |= 0x20;
				break;
			case 6:
				button_locked[byteCount] |= 0x40;
				break;
			case 7:
				button_locked[byteCount] |= 0x80;
				break;
		}
	}
	/*取消封锁标志*/
	else
	{
		switch (bitCount)
		{
			case 0:
				button_locked[byteCount] &= 0xFE;
				break;
			case 1:
				button_locked[byteCount] &= 0xFD;
				break;
			case 2:
				button_locked[byteCount] &= 0xFB;
				break;
			case 3:
				button_locked[byteCount] &= 0xF7;
				break;
			case 4:
				button_locked[byteCount] &= 0xEF;
				break;
			case 5:
				button_locked[byteCount] &= 0xDF;
				break;
			case 6:
				button_locked[byteCount] &= 0xBF;
				break;
			case 7:
				button_locked[byteCount] &= 0x7F;
				break;
		}
	}
}

/****************************************************
函数名：   is_button_locked
功能描述： 获取按钮是否封锁
返回值：   CI_BOOL
参数：     int16_t button_index
作者：	   hejh
日期：     2015/08/11
****************************************************/
CI_BOOL is_button_locked(int16_t button_index)
{
	int16_t byteCount = 0,bitCount = 0;
	CI_BOOL result = CI_FALSE;

	byteCount = (button_index - TOTAL_SIGNAL_NODE) / 8;
	bitCount = (button_index - TOTAL_SIGNAL_NODE) % 8;

	switch (bitCount)
	{
		case 0:
			result = (button_locked[byteCount] & 0x01) == 0x01 ? CI_TRUE : CI_FALSE;
			break;
		case 1:
			result = (button_locked[byteCount] & 0x02) == 0x02 ? CI_TRUE : CI_FALSE;
			break;
		case 2:
			result = (button_locked[byteCount] & 0x04) == 0x04 ? CI_TRUE : CI_FALSE;
			break;
		case 3:
			result = (button_locked[byteCount] & 0x08) == 0x08 ? CI_TRUE : CI_FALSE;
			break;
		case 4:
			result = (button_locked[byteCount] & 0x10) == 0x10 ? CI_TRUE : CI_FALSE;
			break;
		case 5:
			result = (button_locked[byteCount] & 0x20) == 0x20 ? CI_TRUE : CI_FALSE;
			break;
		case 6:
			result = (button_locked[byteCount] & 0x40) == 0x40 ? CI_TRUE : CI_FALSE;
			break;
		case 7:
			result = (button_locked[byteCount] & 0x80) == 0x80 ? CI_TRUE : CI_FALSE;
			break;
	}
	return result;
}

/****************************************************
函数名：   init_auto_test
功能描述： 初始化测试环境
返回值：   void
作者：	   hejh
日期：     2015/04/08
****************************************************/
void init_auto_test(const char* StationName)
{
	if (!strcmp_no_case(StationName,"bzz"))
	{
		//sn_state(309,SIO_HAS_SIGNAL);//GTJ
		//sn_state(310,SIO_HAS_SIGNAL);//SNJJ
		//sn_state(296,SIO_HAS_SIGNAL);//XTY
		//sn_state(297,SIO_HAS_SIGNAL);//STY
		//sn_state(299,SIO_HAS_SIGNAL);//JTY
		//sn_state(294,SIO_HAS_SIGNAL);//X_TSRAJ
		//sn_state(295,SIO_HAS_SIGNAL);//S_TSRAJ
		//sn_state(301,SIO_HAS_SIGNAL);//XLDZCJF
		//sn_state(302,SIO_HAS_SIGNAL);//SLJGJ
		//sn_state(303,SIO_HAS_SIGNAL);//D101ZCJF
		//sn_state(304,SIO_HAS_SIGNAL);//D32JGJ
		//sn_state(305,SIO_HAS_SIGNAL);//TFBD
		//sn_state(298,SIO_HAS_SIGNAL);//TCJF
		//sn_state(307,SIO_HAS_SIGNAL);//T1ZCJF
		//sn_state(308,SIO_HAS_SIGNAL);//T2ZCJF
		//semi_auto_block_config[0].state = SAB_RECIEVED_AGREEMENT;//SX_BZD
	}	
}

