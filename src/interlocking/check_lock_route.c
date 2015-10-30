/***************************************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.
文件名:  check_lock_route.c
作者:    HJH
版本 :   1.2	
创建日期:2012/3/14
用途:    检查并锁闭进路模块（选排一致性检查、信号检查、锁闭进路）
历史修改记录:    
2012/7/14,V1.1，CY:
	1.修改了提示信息。根据实际情况将错误信息，警告信息，提示信息正确分类处理；修改了部分Bug
	2.修改了联锁条件检查模块，选排一致性检查模块，信号检查模块，进路锁闭模块。
	3.修改了长调车进路中间的区段占用时，整条进路都不能建立。
	4.修改了自动解锁中与之相关的bug
	5.修改了错误处理函数中格式化字符串的调用方法。解决发送提示字符处出现错乱的问题
2012/8/10,V1.2，HJH:
	1.解决长调车进路和通过进路分段办理后，解锁不正确的Bug。添加了link_route函数。
	2.修改侵限区段占用时提示错误的信息， 解决了问题跟踪系统上#53 BUG。
	3.修改设置和清除锁闭标志函数
	4.选排一致性检查模块中不应检查带动道岔是否转换到位
	5.实现了基本的股道出岔功能，但是解锁时未作严格的检查
	6.修改改方运行时离去区段占用时信号不关闭的问题，列车进路始端信号断丝时的信号降级显示问题，
	  通过进路的接车进路和发车进路在显示上要有联系
2012/12/5 V1.2.1 hjh
	app_sw_check_select_complete中加入后一进路的的状态介于RS_SWITCHING_OK和RS_SIGNAL_OPENED之间
2012/12/13 V1.2.1 hjh
	check_select_complete中添加检查中间道岔是否到位
2012/12/17 V1.2.1 hjh
	check_switch_location添加检查中间道岔位置是否正确
2013/1/28 V1.2.1 hjh
	1.check_select_complete中增加检查特殊防护道岔是否到位的条件
	2.satisfy_signal_condition中增加函数check_special_switch_location，检查特殊防护道岔位置是否正确
	3.set_route_lock_nodes中给特殊防护道岔加锁闭标志
2013/4/22 V1.2.1 hjh
	app_sw_check_select_complete，app_sw_check_signal_condition，app_sw_lock_route中
	增加延续部分存在时再办理延续进路不检查延续部分的条件
2013/7/30  V1.2.1 LYC 
	set_route_lock_nodes增加了锁闭进路时不锁闭防护道岔
2013/8/1 V1.2.1 hjh
	app_sw_check_select_complete，app_sw_check_signal_condition，app_sw_lock_route
	中增加长调车进路前方或后方进路因故不能建立时，本进路及其前方的进路亦不能建立
2013/8/8 V1.2.1 hjh
	增加前后存在联系的进路若其中一条无法建立则其他亦不能建立的约束条件，只限定于锁闭进路以前
2014/2/14 V1.2.1 hjh
	修改check_select_complete和check_middle_switch_location中间道岔相关的内容
2014/5/23 V1.2.2 LYC 
	app_sw_lock_route增加了检查并开放机务段同意表示灯
***************************************************************************************/
#include "check_lock_route.h"
#include "switch_control.h"
#include "global_data.h"

/****************************************************
函数名:    check_lock_route
功能描述:  检查并锁闭进路
返回值:  
参数:      route_index

作者  :    LYC
日期  ：   2012/9/10
****************************************************/
void check_lock_route(route_t route_index)
{
	FUNCTION_IN;

	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*选排一致性检查*/
		check_select_complete(route_index);
		/*信号检查*/
		check_signal_condition(route_index);
		/*锁闭进路*/
		lock_route(route_index);
	}

	FUNCTION_OUT;
}

/****************************************************
函数名:    app_sw_check_select_complete
功能描述:  选排一致性检查应用软件
返回值:  
参数:      route_index

作者  :    LYC
日期  ：   2012/9/10
****************************************************/
void app_sw_check_select_complete(route_t route_index)
{
	route_t fwr = NO_INDEX,bwr = NO_INDEX,oldr = NO_INDEX;
	route_t i;

	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*判断进路状态*/
		if (RS_SWITCHING_OK == gr_state(route_index))
		{
			sr_stop_timer(route_index);
			fwr = gr_forward(route_index);
			/*检查前方进路是否存在*/
			if (fwr != NO_INDEX)
			{
				check_select_complete(fwr);
			}
			oldr = route_index;
			bwr = gr_backward(route_index);
			/*查看进路中的道岔是否转换到位*/
			while (bwr != NO_INDEX && gr_state(bwr) == RS_SWITCHING_OK)
			{
				oldr = bwr;
				bwr = gr_backward(bwr);
			}
			/*检查进路中的道岔全部转换到位及其他条件*/
			/*hjh 2013-4-22 延续部分存在时设置接车进路*/
			if (((fwr == NO_INDEX) && (gr_state(oldr) == RS_SWITCHING_OK)) ||
				((fwr != NO_INDEX) && (gr_state(fwr) >= RS_SELECT_COMPLETE))
				|| ((fwr != NO_INDEX) && IsTRUE(have_successive_route(route_index)) && (gr_state(fwr) > RS_SWITCHING_OK)))
			{
				/*hjh 2012-12-5 若存在后一进路则需等待后一进路的状态介于RS_SWITCHING_OK和RS_SIGNAL_OPENED之间才能进行信号条件检查*/
				if (bwr != NO_INDEX)
				{
					if ((gr_state(bwr) >= RS_SWITCHING_OK) && (gr_state(bwr) <= RS_SIGNAL_OPENED))
					{
						sr_state(route_index,RS_SELECT_COMPLETE);
					}
				}
				else
				{
					sr_state(route_index,RS_SELECT_COMPLETE);
				}
			}			
		}
		/*hjh 2013-8-1 长调车进路前方或后方进路因故不能建立时，本进路及其前方的进路亦不能建立*/
		if ((gr_state(route_index) == RS_FAILURE_TO_BUILD)
			|| ((fwr != NO_INDEX) && (gr_state(fwr) == RS_FAILURE_TO_BUILD))
			|| ((bwr != NO_INDEX) && (gr_state(bwr) == RS_FAILURE_TO_BUILD)))
		{
			fwr = gr_forward(route_index);
			bwr = gr_backward(route_index);
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
			for (i = bwr; i != NO_INDEX;)
			{
				/*设置进路状态*/
				if ((gr_state(i) != RS_FAILURE_TO_BUILD) && (gr_state(i) < RS_ROUTE_LOCKED))
				{
					sr_state(i,RS_FAILURE_TO_BUILD);
				}
				i = gr_backward(i);
			}
		}
	}	
}

/****************************************************
函数名:    check_select_complete
功能描述:  选排一致检查
返回值:    
参数:    
作者  :		LYC
日期  ：   2011/12/2
****************************************************/
void check_select_complete(route_t route_index)
{	
	int16_t i,j,k,temp;
	int16_t switch_count = 0;
	int16_t another_index = 0;
	int16_t si,ms,section1,section2,section3;
	CI_BOOL result = CI_FALSE;
	node_t switch_index;
	CI_BOOL in_route_flag = CI_FALSE;

	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*查看进路状态**/	
		if (RS_SWITCHING == gr_state(route_index))
		{
			/*查看本进路中道岔的实际位置**/	
			if (IsFALSE(is_timer_run(route_index)))
			{
				sr_start_timer(route_index,MAX_ROUTE_SELECT_TIME,DTT_OTHER);
			}
			switch_count = gr_switches_count(route_index);
			/*检查进路中是否有双动道岔*/	
			for (i = 0; i < switch_count;i++)
			{
				if (IsTRUE(is_follow_switch(route_index,i)))
				{
					/*hjh 2012-12-13 添加检查中间道岔是否到位*/
					/*检查中间道岔是否和进路要求的位置一致*/
					si = gs_middle_switch_index(gr_start_signal(route_index));
					/*检查中间道岔是否存在*/
					if (si != NO_INDEX)
					{
						for (j = 0; j < MAX_MIDDLE_SWITCHES; j++)
						{
							/*找出中间道岔索引号*/
							ms = gs_middle_switch(si,j);
							if ((ms != NO_INDEX) && ((ms == gr_switch(route_index,i))
								|| (ms == gn_another_switch(gr_switch(route_index,i)))))
							{
								/*中间道岔位置和进路要求位置不一致*/
								if (gn_switch_state(ms) != gr_switch_location(route_index,i))
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
												/*检查道岔动作是否超时*/
												if (IsTRUE(is_complete_timer(route_index)))
												{
													process_warning(ERR_SWITCH_OUTTIME,gn_name(ms));
													CIHmi_SendNormalTips("不能转岔：%s",gn_name(ms));
												}	
												result = CI_TRUE;
												break;
											}
										}
										if (gn_switch_section(ms) == section3)
										{
											if ((SCS_CLEARED != gn_section_state(section1))
												&& (SCS_CLEARED != gn_section_state(section2))
												&& (SCS_CLEARED != gn_section_state(section3)))
											{
												continue;
											}
											else
											{
												/*检查道岔动作是否超时*/
												if (IsTRUE(is_complete_timer(route_index)))
												{
													process_warning(ERR_SWITCH_OUTTIME,gn_name(ms));
													CIHmi_SendNormalTips("不能转岔：%s",gn_name(ms));
												}	
												result = CI_TRUE;
												break;
											}
										}
									}
									else
									{
										/*检查道岔动作是否超时*/
										if (IsTRUE(is_complete_timer(route_index)))
										{
											process_warning(ERR_SWITCH_OUTTIME,gn_name(ms));
											CIHmi_SendNormalTips("不能转岔：%s",gn_name(ms));
										}	
										result = CI_TRUE;
										break;
									}
								}
							}
						}
						/*中间道岔不到位*/
						if (IsTRUE(result))
						{
							break;
						}
					}
					/*其他带动道岔*/
					else
					{
						continue;
					}
				}
				/*进路中的道岔和防护道岔*/
				else
				{
					/*hjh 2014-2-20 不检查不在进路上的防护道岔*/
					in_route_flag = CI_FALSE;
					for (j = 0; j < gr_nodes_count(route_index); j++)
					{
						if ((NT_SWITCH == gn_type(gr_node(route_index,j)))
							&& ((gr_switch(route_index,i) == gr_node(route_index,j)) || (gr_switch(route_index,i) == gn_another_switch(gr_node(route_index,j)))))
						{
							in_route_flag = CI_TRUE;
						}
					}
					if (IsTRUE(in_route_flag))
					{
						another_index = gn_another_switch(gr_switch(route_index,i));
						/*若该道岔为双动道岔，则检查双动道岔位置是否一致*/
						if (another_index != NO_INDEX)
						{
							/*检查双动道岔位置是否一致*/
							if (gn_switch_state(gr_switch(route_index,i)) != gn_switch_state(another_index))
							{
								/*检查道岔动作是否超时*/
								if (IsTRUE(is_complete_timer(route_index)))
								{
									process_warning(ERR_SWITCH_OUTTIME,gn_name(gr_switch(route_index,i)));
									CIHmi_SendNormalTips("不能转岔：%s",gn_name(gr_switch(route_index,i)));
								}					
								break;
							}
						}	
						/*检查道岔实际位置和进路要求位置是否一致*/
						if (gn_switch_state(gr_switch(route_index,i)) != gr_switch_location(route_index,i))
						{
							/*检查道岔动作是否超时*/
							if (IsTRUE(is_complete_timer(route_index)))
							{
								process_warning(ERR_SWITCH_OUTTIME,gn_name(gr_switch(route_index,i)));
								CIHmi_SendNormalTips("不能转岔：%s",gn_name(gr_switch(route_index,i)));
							}					
							break;
						}
					}
				}
			}

			/*检查是否存在特殊防护道岔*/
			if ((temp = gs_special(route_index,SPT_SPECIAL_SWTICH)) != NO_INDEX)
			{
				for (k = 0;k < MAX_SPECIAL_SWITCHES; k++)
				{
					/*获取特殊防护道岔的索引号*/
					switch_index = gs_special_switch_index(temp,k);
					/*特殊防护道岔位置不正确*/
					if ((switch_index != NO_INDEX) 
						&& (gs_special_switch_location(temp,k)  != gn_switch_state(switch_index)))
					{
						i = 0;
						break;
					}
				}
			}

			/*判断进路中所有的道岔是否转换到位*/
			if (i == switch_count)
			{
				sr_state(route_index,RS_SWITCHING_OK);
				sr_stop_timer(route_index);
			}
			/*判断道岔转动是否超时*/
			if(IsTRUE(is_timer_run(route_index)) 
				&& IsTRUE(is_complete_timer(route_index)))
			{
				switch_out_time(route_index);
				//sr_state(route_index,RS_FAILURE_TO_BUILD);
				//sr_stop_timer(route_index);				
			}
		}
		app_sw_check_select_complete(route_index);
	}	

	FUNCTION_OUT;
}

/****************************************************
函数名:    app_sw_check_signal_condition
功能描述:  信号检查应用软件
返回值:  
参数:      route_index

作者  :    LYC
日期  ：   2012/9/11
****************************************************/
void app_sw_check_signal_condition(route_t route_index)
{
	route_t fwr = NO_INDEX,bwr = NO_INDEX,oldr = NO_INDEX;
	route_t i;
	char_t tips[TEST_NAME_LENGTH];

	if (IsTRUE(is_route_exist(route_index)))
	{
		/*判断信号检查是否成功*/
		if (RS_SIGNAL_CHECKED_OK == gr_state(route_index))
		{
			sr_stop_timer(route_index);
			fwr = gr_forward(route_index);
			/*判断本进路前方是否还有进路*/
			if (fwr != NO_INDEX)
			{
				check_signal_condition(fwr);
			}
			oldr = route_index;
			bwr = gr_backward(route_index);
			while ((bwr != NO_INDEX) && (gr_state(bwr) == RS_SIGNAL_CHECKED_OK))
			{
				oldr = bwr;
				bwr = gr_backward(bwr);
			}
			/*hjh 2013-4-22 延续部分存在时设置接车进路*/
			if (((fwr == NO_INDEX) && (gr_state(oldr) == RS_SIGNAL_CHECKED_OK)) ||
				((fwr != NO_INDEX) && (gr_state(fwr) >= RS_SIGNAL_CHECKED))
				|| ((fwr != NO_INDEX) && IsTRUE(have_successive_route(route_index)) && (gr_state(fwr) > RS_SIGNAL_CHECKED_OK)))
			{
				sr_state(route_index,RS_SIGNAL_CHECKED);
				/*输出提示信息*/
				memset(tips,0x00,sizeof(tips));
				strcat_check(tips,"信号条件检查成功：",sizeof(tips));
				OutputHmiNormalTips(tips,route_index);
			}				
		}
		/*hjh 2013-8-1 长调车进路前方或后方进路因故不能建立时，本进路及其前方的进路亦不能建立*/
		if ((gr_state(route_index) == RS_FAILURE_TO_BUILD)
			|| ((fwr != NO_INDEX) && (gr_state(fwr) == RS_FAILURE_TO_BUILD))
			|| ((bwr != NO_INDEX) && (gr_state(bwr) == RS_FAILURE_TO_BUILD)))
		{
			fwr = gr_forward(route_index);
			bwr = gr_backward(route_index);
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
			for (i = bwr; i != NO_INDEX;)
			{
				/*设置进路状态*/
				if ((gr_state(i) != RS_FAILURE_TO_BUILD) && (gr_state(i) < RS_ROUTE_LOCKED))
				{
					sr_state(i,RS_FAILURE_TO_BUILD);
				}
				i = gr_backward(i);
			}
		}
	}
	
}

/****************************************************
函数名:    check_signal_condition
功能描述:  信号检查
返回值:    
参数:      route_index 进路索引号
作者  :    CY
日期  ：   2011/12/2
****************************************************/
void check_signal_condition(route_t route_index)
{	
	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*检查进路状态*/
		if (RS_SELECT_COMPLETE == gr_state(route_index))
		{
			/*检查进路是否满足信号检查条件*/
			if(IsTRUE(satisfy_signal_condition(route_index)))
			{
				 sr_state(route_index,RS_SIGNAL_CHECKED_OK);
			}
			else
			{
				sr_state(route_index, RS_FAILURE_TO_BUILD);
			}
		}	
		app_sw_check_signal_condition(route_index);
	}	

	FUNCTION_OUT;
}

/****************************************************
函数名:    satisfy_signal_condition
功能描述:  检查进路是否满足信号检查条件
返回值:    
参数:      route_index
作者  :    CY
日期  ：   2011/12/27
****************************************************/
CI_BOOL satisfy_signal_condition( route_t route_index) 
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

		/*检查进路上的联系条件*/
		if (IsFALSE(is_successive_route(route_index)) && IsFALSE(check_relation_condition(route_index)))
		{
			result = CI_FALSE;
		}

		/*道岔位置检查*/	
		if (IsFALSE(check_switch_location(route_index)) 
			|| IsFALSE(check_middle_switch_location(route_index))
			|| IsFALSE(check_special_switch_location(route_index)))
		{
			process_warning(ERR_SWITCH_LOCATION, gr_name(route_index));
			result = CI_FALSE;
		}

		/*检查进路相关的侵限条件是否满足*/
		if (IsTRUE(check_exceed_limit(route_index)))
		{
			process_warning(ERR_EXCEED_LIMIT_OCCUPIED, gr_name(route_index));
			result = CI_FALSE;
		}
	}	

	FUNCTION_OUT;
	return result;
}

/****************************************************
函数名:    app_sw_lock_route
功能描述:  锁闭进路应用软件
返回值:  
参数:      route_index

作者  :    LYC
日期  ：   2012/9/11
****************************************************/
void app_sw_lock_route(route_t route_index)
{
	route_t fwr = NO_INDEX,bwr = NO_INDEX,oldr = NO_INDEX;
	route_t i;
	char_t tips[TEST_NAME_LENGTH];

	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		if (RS_ROUTE_LOCKED_OK == gr_state(route_index))
		{
			sr_stop_timer(route_index);
			/*判断该进路的进路状态*/
			if (gr_state(route_index) == RS_ROUTE_LOCKED_OK)
			{
				fwr = gr_forward(route_index);
				/*判断该进路前方是否有进路存在*/
				if (fwr != NO_INDEX)
				{
					lock_route(fwr);
				}
				oldr = route_index;
				bwr = gr_backward(route_index);
				/*判断本进路后方进路是否也满足本进路锁闭条件*/
				while ((bwr != NO_INDEX) && (gr_state(bwr) == RS_ROUTE_LOCKED_OK))
				{
					oldr = bwr;
					bwr = gr_backward(bwr);
				}
				/*判断该进路可以锁闭的条件*/
				/*hjh 2013-4-22 延续部分存在时设置接车进路*/
				if (((fwr == NO_INDEX) && (gr_state(oldr) == RS_ROUTE_LOCKED_OK)) ||
					((fwr != NO_INDEX) && (gr_state(fwr) == RS_ROUTE_LOCKED))
					|| ((fwr != NO_INDEX) && IsTRUE(have_successive_route(route_index)) && (gr_state(fwr) > RS_ROUTE_LOCKED_OK)))
				{
					/*设置进路中及与进路相关的信号节点锁闭标志*/
					set_route_lock_nodes(route_index);						
					/*清除进路中及与进路相关信号节点的征用标志*/
					clear_ci_used_flag(route_index);
					/*设置进路状态为进路已锁闭*/
					sr_state(route_index,RS_ROUTE_LOCKED);
					/*记录当前处理周期*/
					sr_updata_cycle(route_index);
					/*输出提示信息*/
					memset(tips,0x00,sizeof(tips));
					strcat_check(tips,"进路锁闭：",sizeof(tips));
					OutputHmiNormalTips(tips,route_index);
				}			
			} 
		}
		/*hjh 2013-8-1 长调车进路前方或后方进路因故不能建立时，本进路及其前方的进路亦不能建立*/
		if ((gr_state(route_index) == RS_FAILURE_TO_BUILD)
			|| ((fwr != NO_INDEX) && (gr_state(fwr) == RS_FAILURE_TO_BUILD))
			|| ((bwr != NO_INDEX) && (gr_state(bwr) == RS_FAILURE_TO_BUILD)))
		{
			fwr = gr_forward(route_index);
			bwr = gr_backward(route_index);
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
			for (i = bwr; i != NO_INDEX;)
			{
				/*设置进路状态*/
				if ((gr_state(i) != RS_FAILURE_TO_BUILD) && (gr_state(i) < RS_ROUTE_LOCKED))
				{
					sr_state(i,RS_FAILURE_TO_BUILD);
				}
				i = gr_backward(i);
			}
		}
	}	
}

/****************************************************
函数名:    lock_route
功能描述:  锁闭进路模块
返回值:    
参数:      route_index
作者  :    LYC
日期  ：   2011/12/2
****************************************************/
void lock_route(route_t route_index)
{	
	FUNCTION_IN;

	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		/*检查进路状态为信号检查成功*/
		if (RS_SIGNAL_CHECKED == gr_state(route_index))
		{				
			/*检查进路中和与进路相关的所有信号节点都处于被征用且未锁闭状态*/
			if (IsTRUE(is_all_node_unlock(route_index)))
			{	
				sr_state(route_index,RS_ROUTE_LOCKED_OK) ;
			}
			else
			{
				sr_state(route_index,RS_FAILURE_TO_BUILD) ;
			}
		}
		app_sw_lock_route(route_index);
	}	
	
	FUNCTION_OUT;
}

/****************************************************
函数名:    set_route_lock_nodes
功能描述:  给进路中的所有节点设置锁闭标志
返回值:  
参数：	   route_index
作者  :    LYC
日期  ：   2011/12/7
****************************************************/
void set_route_lock_nodes(route_t route_index)
{
	int16_t i = 0, j = 0, count = 0, index = NO_INDEX;
	int16_t si = NO_INDEX,ms = NO_INDEX,start_signal = NO_INDEX,end_signal = NO_INDEX;
	int16_t temp;
	node_t switch_index;

	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{
		count = gr_nodes_count(route_index);
		/*给进路中所有的信号点设置锁闭标志*/
		for (i = 0; i < count; i++)
		{
			/*不检查进路中的单置调车信号机的节点状态*/
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
			/*锁闭进路时不锁闭防护道岔*/
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
							/*判断节点是否处于征用且未锁闭状态*/
							if (IsFALSE(is_node_locked(index,LT_LOCKED)) && IsTRUE(gn_used_state(index)))
							{
								sn_used_state(index, CI_FALSE);
								sn_locked_state(index,LT_LOCKED);
							}
						}
						break;
					}
				}				
			}
			else
			{
				/*判断节点是否处于征用且未锁闭状态*/
				if (IsFALSE(is_node_locked(index,LT_LOCKED)) && IsTRUE(gn_used_state(index)))
				{
					sn_used_state(index, CI_FALSE);
					sn_locked_state(index,LT_LOCKED);
				}
			}
		
			///*判断节点是否处于征用且未锁闭状态*/
			//if (IsFALSE(is_node_locked(index,LT_LOCKED)) && IsTRUE(gn_used_state(index)))
			//{
			//	sn_used_state(index, CI_FALSE);
			//	sn_locked_state(index,LT_LOCKED);
			//}
		}

		/*非进路调车接近区段锁闭*/
		if ((gr_other_flag(route_index) == ROF_HOLD_ROUTE_SHUNTING) && IsFALSE(is_node_locked(gr_approach(route_index),LT_LOCKED)))
		{
			sn_locked_state(gr_approach(route_index),LT_LOCKED);
		}

		start_signal = gr_start_signal(route_index);
		end_signal = gb_node(gr_end_button(route_index));
		if ((NT_ENTRY_SIGNAL == gn_type(end_signal)) || (NT_ROUTE_SIGNAL == gn_type(end_signal)) || (NT_TRAIN_END_BUTTON == gn_type(end_signal)))
		{
			si = gs_middle_switch_index(start_signal);
			/*判断进路中是否有中间道岔*/
			if (si != NO_INDEX)
			{
				for (i = 0; i < MAX_MIDDLE_SWITCHES; i++)
				{
					ms = gs_middle_switch(si,i);
					/*判断进路中存在中间道岔*/
					if (ms != NO_INDEX)
					{
						/*判断该中间道岔未被单锁*/
						if (IsFALSE(is_node_locked(ms,LT_MIDDLE_SWITCH_LOCKED)))
						{
							sn_locked_state(ms,LT_MIDDLE_SWITCH_LOCKED);
							/*判断该道岔是否是双动道岔*/
							if (gn_another_switch(ms) != NO_INDEX)
							{
								sn_locked_state(gn_another_switch(ms),LT_MIDDLE_SWITCH_LOCKED);
							}
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
					sn_locked_state(switch_index,LT_MIDDLE_SWITCH_LOCKED);
					/*判断该道岔是否是双动道岔*/
					if (gn_another_switch(switch_index) != NO_INDEX)
					{
						sn_locked_state(gn_another_switch(switch_index),LT_MIDDLE_SWITCH_LOCKED);
					}
				}
			}
		}
	}	

	FUNCTION_OUT;
}
/****************************************************
函数名:    check_switch_location
功能描述:  道岔位置一致性检查
返回值:    一致返回真，反之返回假
参数:      route_index
作者  :    WSP
日期  ：   2011/12/14
****************************************************/
CI_BOOL check_switch_location(route_t route_index)
{
	int16_t i,j, count = 0, index = 0;
	CI_BOOL judge_result = CI_TRUE;

	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{		
		count = gr_switches_count(route_index);
		for (i = 0; i < count && judge_result; i++)
		{
			index = gr_switch(route_index, i);
			if ((IsFALSE(is_follow_switch(route_index,i))) 
				&& (gr_switch_location(route_index,i) != gn_switch_state(index)))
			{
				/*hjh 2014-2-20 不检查不在进路上的防护道岔*/
				for (j = 0; j < gr_nodes_count(route_index); j++)
				{
					if ((NT_SWITCH == gn_type(gr_node(route_index,j)))
						&& ((index == gr_node(route_index,j)) || (index == gn_another_switch(gr_node(route_index,j)))))
					{
						judge_result = CI_FALSE;
						CIHmi_SendNormalTips("道岔位置错误：%s",gn_name(index));
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
函数名:    check_middle_switch_location
功能描述:  检查中间道岔位置一致
返回值:    
参数:      int16_t route_index
作者  :    hejh
日期  ：   2012/12/17
****************************************************/
CI_BOOL check_middle_switch_location(route_t route_index)
{
	int16_t i = 0,j = 0,index = 0;
	CI_BOOL judge_result = CI_TRUE;
	int16_t si,ms,section1,section2,section3;

	FUNCTION_IN;
	/*参数检查*/
	if (IsTRUE(is_route_exist(route_index)))
	{		
		si = gs_middle_switch_index(gr_start_signal(route_index));
		/*检查中间道岔是否存在*/
		if (si != NO_INDEX)
		{
			for (i = 0; i < gr_switches_count(route_index); i++)
			{
				index = gr_switch(route_index, i);
				if (gr_switch_location(route_index,i) != gn_switch_state(index))
				{
					if (IsTRUE(is_follow_switch(route_index,i)))
					{
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
									if (gn_switch_section(ms) == section2)
									{
										if ((SCS_CLEARED != gn_section_state(section1))
											&& (SCS_CLEARED != gn_section_state(section2)))
										{
											continue;
										}
										else
										{
											/*中间道岔位置错误*/
											judge_result = CI_FALSE;
											break;
										}
									}
									if (gn_switch_section(ms) == section3)
									{
										if ((SCS_CLEARED != gn_section_state(section1))
											&& (SCS_CLEARED != gn_section_state(section2))
											&& (SCS_CLEARED != gn_section_state(section3)))
										{
											continue;
										}
										else
										{
											/*中间道岔位置错误*/
											judge_result = CI_FALSE;
											break;
										}
									}
								}
								else
								{
									/*中间道岔位置错误*/
									judge_result = CI_FALSE;
									break;
								}
							}
						}
						/*中间道岔位置错误*/
						if (IsFALSE(judge_result))
						{
							CIHmi_SendNormalTips("中间道岔位置错误：%s",gn_name(index));
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
函数名:    check_special_switch_location
功能描述:  检查特殊防护道岔位置正确
返回值:    
参数:      int16_t route_index
作者  :    hejh
日期  ：   2013/1/28
****************************************************/
CI_BOOL check_special_switch_location(route_t route_index)
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
					result = CI_FALSE;
					CIHmi_SendNormalTips("特殊防护道岔位置错误：%s",gn_name(switch_index));
					break;
				}
			}
		}
	}

	return result;
}