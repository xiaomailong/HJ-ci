/***************************************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.
文件名:  search_route.c
作者:    CY
版本 :   1.0	
创建日期:2011/12/2
用途:    进路搜索模块
历史修改记录:         
		V1.1  解决了Bug 36-39，32
			  修改了进路表中进路存放的位置，之前是从第六条开始存放
			  解决进路搜索模块中，进路选择算法中的bug
			  调试并修改半自动闭塞功能
		V1.2  解决 Issue #57 closes #57
			  修改了 延续进路 SX向XI的接车进路无法办理的问题 Issue #57
			  放信号主要依据联锁表进行。
			  解决长调车进路和通过进路分段办理后，解锁不正确的Bug。添加了link_route函数。
			  解决初始化中的Bug，init_system_var 有问题
			  1.修改了X6 向 D6进路选择不正确的Bug
			  2.修改了同时向股道排列调车进路时不能排出的Bug
2013/1/5 V1.2.1 hjh
	successive_route_judge增加检查延续部分是列车进路的条件
	normal_route_judge解决SILA，BA1，XDLA可以办进路的BUG
2013/1/21 V1.2.1 hjh
	normal_route_judge整理搜索逻辑，将始端和终端符合要求的进路均搜索出来
2013/1/25 V1.2.1 hjh
	normal_route_judge加强条件，不搜索通过进路和延续进路
	passing_route_judge亦加强条件，只有始终端按钮匹配的进路再判断进路上道岔的位置是否均在定位
2013/3/5 V1.2.1 hjh
	normal_route_judge变更进路只考虑所有按钮均匹配的情况
2013/3/6 V1.2.1 hjh
	select_ILT长进路选择时优先选择基本进路
2013/3/15 V1.2.1 hjh
	1.successive_route_judge中增加6G上存在中间道岔时办理延续进路的情况
	2.set_route增加6G上存在中间道岔时给延续部分设置延续标志
2013/3/21 V1.2.1 hjh
	select_ILT搜索到多条符合要求的进路时，只选择排在最前面的一条进路，方便配置，
	若搜索到的进路与要求不一致只需更改联锁表中的顺序即可
2013/4/22 V1.2.1 hjh
	1.search_route增加存在延续部分再办理延续进路的问题
	2.get_next_index修改为以进路上最后一个节点开始搜索，解决6G延续进路上存在中间道岔的问题。
	3.删除2013年3月15日增加的内容
2013/4/25 V1.2.1 hjh
	get_next_index中查找到结果的类型不是信号机，则继续沿着进路的方向查找
2014/2/20 V1.2.1 hjh mantis:3303
	select_ILT添加关于进路信号机办理长进路的处理，只选组合进路，不选短进路
***************************************************************************************/
#include "search_route.h"
#include "process_command.h"
#include "global_data.h"
#include "utility_function.h"
#include "error_process.h"
#include "init_manage.h"

#define  MAX_ILT_PER_ROUTES  15 /*进路中的最大短进路数*/
#define  MAX_CHANGE_ROUTE 20   /*进路中的最大变更进路数*/

/****************************************************
函数名:    ILT_search_route
功能描述:  联锁表型进路搜索
返回值:    
参数:      route
作者  :    CY
日期  ：   2011/12/2
****************************************************/
void ILT_search_route (int16_t route[MAX_CHANGE_ROUTE][MAX_ILT_PER_ROUTES]);

/****************************************************
函数名:    set_route
功能描述:  设置进路，将正确的进路添加到进路表中，并设置信号节点的进路属性
返回值:    
参数:      ILT_routes 联锁表索引号数组
作者  :    CY
日期  ：   2011/12/2
****************************************************/
void set_route(int16_t ILT_routes[]);

/****************************************************
函数名:    select_ILT
功能描述:  从多条进路中选出合适的进路
返回值:    
参数:      int16_t ILTs[MAX_CHANGE_ROUTE][MAX_ILT_PER_ROUTES]
作者  :    CY
日期  ：   2012/5/23
****************************************************/
int16_t select_ILT(int16_t ILTs[MAX_CHANGE_ROUTE][MAX_ILT_PER_ROUTES]);

/****************************************************
函数名:   get_next_index
功能描述: 根据当前进路获取下一个起始节点
返回值:    节点索引号
参数:      route_index 进路索引号
作者  :    CY
日期  ：   2011/12/2
****************************************************/
int16_t get_next_index(int16_t ILT_index);

/****************************************************
函数名:    search_route
功能描述:  进路搜索主函数
返回值:    
作者  :    CY
日期  ：   2011/12/2
****************************************************/
void search_route( void )
{
	int16_t ILT_routes[MAX_CHANGE_ROUTE][MAX_ILT_PER_ROUTES];
	int16_t valid_ILT_index = NO_INDEX;
	node_t successive_start_signal = NO_INDEX;
	route_t successive_route = NO_INDEX;
	char_t tips[TEST_NAME_LENGTH];

	FUNCTION_IN;
	if (IsTRUE(can_build_route))
	{
		/*检查按钮*/
		if ((first_button >= 0) && (first_button <= TOTAL_NAMES))  
		{
			/*初始化数据区域，清除数据*/
			memory_set(ILT_routes,NO_INDEX,sizeof(ILT_routes));

			/*联锁表型进路搜索*/
			ILT_search_route(ILT_routes);
			/*从ILT_routes中选择仅有的一条正确进路*/
			valid_ILT_index = select_ILT(ILT_routes);
			if ( valid_ILT_index != NO_INDEX) 
			{
				/*对有延续进路的接车进路分别进行处理*/
				if(IsTRUE(is_successive_ILT(ILT_routes[valid_ILT_index][0])))
				{
					/*获取延续部分始端信号*/
					successive_start_signal = get_next_index(ILT_routes[valid_ILT_index][0]);
					if (IsTRUE(is_out_signal(successive_start_signal)))
					{
						successive_route = gn_belong_route(successive_start_signal);
					}
					/*hjh 2013-4-22 存在以延续部分始端信号为始端的列车进路*/
					if ((successive_route != NO_INDEX)
						&& (gr_start_signal(successive_route) == successive_start_signal)
						/*&& ((gr_state(successive_route) == RS_ROUTE_LOCKED)
						|| (gr_state(successive_route) == RS_SIGNAL_OPENED)
						|| (gr_state(successive_route) == RS_A_SIGNAL_CLOSED))*/
						&& ((gr_type(successive_route) == RT_TRAIN_ROUTE) || (gr_type(successive_route) == RT_SUCCESSIVE_ROUTE)))
					{
						/*检查延续部分条件是否满足*/
						if ((gr_state(successive_route) == RS_CRASH_INTO_SIGNAL) || (gr_state(successive_route) == RS_AUTO_UNLOCK_FAILURE))
						{
							process_warning(ERR_SUCCESSIVE_ROUTE,gr_name(successive_route));
							/*输出提示信息*/
							memset(tips,0x00,sizeof(tips));
							strcat_check(tips,"延续进路条件不满足：",sizeof(tips));
							OutputHmiNormalTips(tips,successive_route);
						}
						else
						{
							set_route(ILT_routes[valid_ILT_index]);  /*设置进路*/
							clean_button_log();
							CIHmi_SendDebugTips("建立延续进路！");
						}						
					}
					else
					{
						/*这个条件的意思是如果当前只按下了接车进路部分的按钮，延续进路相关的按钮还没按下，
						  此时不应该设置进路数据。必须要等到接车进路和延续进路都搜索出来才开始设置进路数据*/
						if (ILT_routes[valid_ILT_index][0] != NO_INDEX && ILT_routes[valid_ILT_index][1] != NO_INDEX)
						{
							set_route(ILT_routes[valid_ILT_index]);  /*设置进路*/
							clean_button_log();
							CIHmi_SendDebugTips("建立延续进路！");
						}
					}					
				}
				else
				{
					/*正常建立进路过程的处理*/
					set_route(ILT_routes[valid_ILT_index]);  /*设置进路*/
					clean_button_log();
					CIHmi_SendDebugTips("建立进路！");
				}
			}
			else
			{
				/*搜索不成功时，说明操作命令是错误的，所以清除了操作命令*/
				CIHmi_SendNormalTips("错误办理：%s --> %s",
					gn_name(gb_node(first_button)),
					gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
				clean_button_log();
				process_warning(ERR_NO_ROUTE,"");
			}
		}		
		can_build_route = CI_FALSE;
	}
	FUNCTION_OUT;
}

/****************************************************
函数名:    select_ILT
功能描述:  从多条进路中选出合适的进路
返回值:    
参数:      int16_t ILTs[MAX_CHANGE_ROUTE][MAX_ILT_PER_ROUTES]
作者  :    CY
日期  ：   2012/5/23
****************************************************/
int16_t select_ILT(int16_t ILTs[MAX_CHANGE_ROUTE][MAX_ILT_PER_ROUTES])
{
	int16_t i,j,m,n;
	uint8_t exclude[MAX_CHANGE_ROUTE];
	int16_t only = NO_INDEX;
	int16_t result = NO_INDEX;

	memory_set(exclude,0,sizeof(exclude));
	/*检查多条进路*/
	for (i = 0; i < MAX_CHANGE_ROUTE && ILTs[i][0] != NO_INDEX; i++)
	{
		/*经过筛选，此时选出正确的延续进路有且仅有一条*/
		if (IsTRUE(is_successive_ILT(ILTs[i][0])))
		{
			only = i;
			break;
		}
		/*短进路*/
		else
		{
			/*两个按钮*/
			if ((third_button == NO_INDEX) && (fourth_button == NO_INDEX))
			{
				/*hjh 2014-2-20 添加关于进路信号机XLIII-SL处办理长进路的特殊处理，只选组合进路，不选短进路*/
				for (j = 1; j < MAX_ILT_PER_ROUTES && ILTs[i][j] != NO_INDEX; j++)
				{
					if ((ILTs[i][j] != NO_INDEX) 
						&& (gn_type(gb_node(ILT[ILTs[i][j]].end_button)) == NT_ROUTE_SIGNAL))
					{
						only = i;
						break;
					}
				}
				if (only != NO_INDEX)
				{
					break;
				}
				
				/*普通进路*/
				if ((ILT[ILTs[i][0]].change_button == NO_INDEX) 
					&& (ILT[ILTs[i][0]].change_button1 == NO_INDEX)
					&& (ILT[ILTs[i][0]].end_button == second_button))
				{
					only = i;
					break;
				}
			}
			/*三个按钮*/
			if ((third_button != NO_INDEX) && (fourth_button == NO_INDEX))
			{
				/*正确进路*/
				if ((ILT[ILTs[i][0]].change_button != NO_INDEX) 
					&& (ILT[ILTs[i][0]].change_button1 == NO_INDEX) 
					&& (ILT[ILTs[i][0]].change_button == second_button)
					&& (ILT[ILTs[i][0]].end_button == third_button))
				{
					only = i;
					break;
				}			
			}
			/*四个按钮*/
			if ((third_button != NO_INDEX) && (fourth_button != NO_INDEX))
			{
				/*正确进路*/
				if ((ILT[ILTs[i][0]].change_button != NO_INDEX) 
					&& (ILT[ILTs[i][0]].change_button1 != NO_INDEX)
					&& (ILT[ILTs[i][0]].change_button == second_button)
					&& (ILT[ILTs[i][0]].change_button1 == third_button)
					&& (ILT[ILTs[i][0]].end_button == fourth_button))
				{
					only = i;
					break;
				}
			}
		}		
	}

	/*多条进路中，如果有且只有一条是短进路，则以此为最合适的进路*/
	if (only > NO_INDEX)
	{
		result = only;
	}
	/*长进路选择*/
	else
	{   
		/*2013-3-6 hjh 长进路选择时优先选择基本进路*/
		/*长进路上的短进路均要求是基本进路，不能选出变更进路*/
		while (ILTs[result + 1][0] != NO_INDEX)
		{
			/*短进路不允许是变更进路*/	
			result++;
			for (n = 0; n < MAX_ILT_PER_ROUTES && ILTs[0][n] != NO_INDEX; n++)
			{
				/*短进路存在变更进路则将其删除*/
				if ((ILT[ILTs[result][n]].change_button != NO_INDEX)
					|| (ILT[ILTs[result][n]].change_button1 != NO_INDEX))
				{
					/*先将本区域清零,-1*/
					for (n = 0; n < MAX_ILT_PER_ROUTES && ILTs[result][n] != NO_INDEX; n++)
					{
						ILTs[result][n] = NO_INDEX;
					}
					/*将后面的数据填充至本区域*/
					for (m = result; m < MAX_CHANGE_ROUTE && ILTs[m + 1][0] != NO_INDEX; m++)
					{
						for (n = 0; n < MAX_ILT_PER_ROUTES && ILTs[m + 1][n] != NO_INDEX; n++)
						{
							ILTs[m][n] = ILTs[m + 1][n];
							ILTs[m + 1][n] = NO_INDEX;
						}
					}
					result--;
					break;
				}
			}
		}

		/*存在多条符合要求的长进路*/
		if (result > 0)
		{
			/*hjh 2013-3-21 只选择排在最前面的一条进路，方便配置*/
			/*若搜索到的进路与要求不一致只需更改联锁表中的顺序即可*/
			if (ILTs[0][0] != NO_INDEX)
			{
				result = 0;
			}
			else
			{
				result = NO_INDEX;
			}
		}
	}
	return result;
}

/****************************************************
函数名:    successive_route_judge
功能描述:  延续进路判断
返回值:    
参数:      int32_t ILT_count
参数:      int16_t * searched_routes
作者  :    CY
日期  ：   2012/6/7
****************************************************/
CI_BOOL successive_route_judge( int32_t ILT_count, int16_t * searched_routes) 
{
	CI_BOOL result = CI_FALSE;

	/*一般延续进路是两条*/
	if ((ILT_count == 2)
		&& IsTRUE(is_successive_ILT(searched_routes[0])) 
		&& IsTRUE(is_successive_button(searched_routes[0]))) 
	{
		/*hjh 2013-1-5 增加检查延续部分是列车进路的条件*/
		result = CI_FALSE;
		/*三个按钮*/
		if ((third_button != NO_INDEX) && (fourth_button == NO_INDEX))
		{
			/*匹配第二个和第三个按钮*/
			if ((ILT[searched_routes[0]].change_button == NO_INDEX) 
				&&(ILT[searched_routes[0]].end_button == second_button) 
				&& ((ILT[searched_routes[1]].route_kind == RT_TRAIN_ROUTE) || (ILT[searched_routes[1]].route_kind == RT_SUCCESSIVE_ROUTE))
				&& (ILT[searched_routes[1]].change_button == NO_INDEX)
				&& (ILT[searched_routes[1]].end_button == third_button))
			{
				result = CI_TRUE;
			}
		}
		/*四个按钮*/
		if ((third_button != NO_INDEX) && (fourth_button != NO_INDEX) && (fifth_button == NO_INDEX))
		{
			/*匹配第二个，第三个和第四个按钮*/
			if ((ILT[searched_routes[0]].change_button == second_button)
				&& (ILT[searched_routes[0]].change_button1 == NO_INDEX)
				&&(ILT[searched_routes[0]].end_button == third_button) 
				&& ((ILT[searched_routes[1]].route_kind == RT_TRAIN_ROUTE) || (ILT[searched_routes[1]].route_kind == RT_SUCCESSIVE_ROUTE))
				&& (ILT[searched_routes[1]].change_button == NO_INDEX)
				&& (ILT[searched_routes[1]].end_button == fourth_button))
			{
				result = CI_TRUE;
			}
		}
		/*五个按钮*/
		if ((third_button != NO_INDEX) && (fourth_button != NO_INDEX) && (fifth_button != NO_INDEX))
		{
			/*匹配第二个，第三个、第四个和第五个按钮*/
			if ((ILT[searched_routes[0]].change_button == second_button)
				&& (ILT[searched_routes[0]].change_button1 == third_button)
				&&(ILT[searched_routes[0]].end_button == fourth_button) 
				&& ((ILT[searched_routes[1]].route_kind == RT_TRAIN_ROUTE) || (ILT[searched_routes[1]].route_kind == RT_SUCCESSIVE_ROUTE))
				&& (ILT[searched_routes[1]].change_button == NO_INDEX)
				&& (ILT[searched_routes[1]].end_button == fifth_button))
			{
				result = CI_TRUE;
			}
		}
	}
	/*检查搜索出的一条进路是否需要办理延续进路*/
	if ((ILT_count == 1) && IsTRUE(is_successive_ILT(searched_routes[0])))
	{
		result = CI_FALSE;
		/*两个按钮*/
		if (third_button == NO_INDEX)
		{
			if (ILT[searched_routes[0]].end_button == second_button)
			{
				result = CI_TRUE;
			}
		}
		/*三个按钮*/
		if ((third_button != NO_INDEX) && (fourth_button == NO_INDEX))
		{
			if ((ILT[searched_routes[0]].end_button == third_button)
				&& (ILT[searched_routes[0]].change_button == second_button)
				&& (ILT[searched_routes[0]].change_button1 == NO_INDEX))
			{
				result = CI_TRUE;
			}
		}
		/*四个按钮*/
		if ((third_button != NO_INDEX) && (fourth_button != NO_INDEX))
		{
			if ((ILT[searched_routes[0]].end_button == fourth_button)
				&& (ILT[searched_routes[0]].change_button == second_button)
				&& (ILT[searched_routes[0]].change_button1 == third_button))
			{
				result = CI_TRUE;
			}
		}
	}	
	return result;
}

/****************************************************
函数名:    passing_route_judge
功能描述:  通过进路判断
返回值:    
参数:      int32_t ILT_count
参数:      int16_t searched_routes[]
作者  :    CY
日期  ：   2012/6/7
****************************************************/
CI_BOOL passing_route_judge( int32_t ILT_count, int16_t  searched_routes[] ) 
{
	int32_t i = 0,j;
	uint32_t switch_state;
	CI_BOOL result = CI_FALSE;

	/*通过进路判断*/
	if (IsTRUE(passing_route) && ILT_count>1) 
	{
		/*匹配按钮*/
		if (ILT[searched_routes[0]].start_button == first_button )
		{
			if (ILT[searched_routes[ILT_count-1]].end_button == second_button)
			{
				result = CI_TRUE;
			}
		}
		if (IsTRUE(result))
		{
			for (i = 0; i < 2; i++)
			{
				/*此处是为了判断道岔状态，要求通过进路上的道岔都必须是定位*/
				for (j = 0;j < ILT[searched_routes[i]].switch_count;j++)
				{
					switch_state = ILT[searched_routes[i]].switches[j].state;				
					if (!FOLLOW_SWITCH(switch_state) && !PROTECTIVE_SWITCH(switch_state))
					{
						if (!SWITCH_NORMAL(switch_state))
						{
							result = CI_FALSE;
							i = 2;
							break;
						}
					}
				}
			}
		}		
	}
	return result;
}

/****************************************************
函数名:    normal_route_judge
功能描述:  正常进路判断
返回值:    
参数:      int32_t ILT_count
参数:      int16_t searched_routes[]
作者  :    CY
日期  ：   2012/6/7
****************************************************/
CI_BOOL normal_route_judge( int32_t ILT_count, int16_t  searched_routes[] )
{
	int32_t i = 0;
	CI_BOOL result = CI_FALSE;

	/*正常的进路按钮匹配，排除有延续进路的接车进路和通过进路*/
	if (ILT_count > 0 
		&& IsFALSE(is_successive_ILT(searched_routes[0])) 
		&& IsFALSE(passing_route)
		&& (ILT[searched_routes[0]].start_button == first_button))
	{
		/*按下两个按钮*/
		if (NO_INDEX == third_button)
		{
			for (i = 0; i < ILT_count; i++)
			{
				/*匹配始端按钮和终端按钮，考虑长进路*/
				if (ILT[searched_routes[i]].end_button == second_button)
				{
					result = CI_TRUE;
					break;
				}
			}
		}
		
		/*2013-3-5 hjh 按钮匹配时，变更进路只考虑所有按钮均匹配的情况*/
		/*按下三个按钮*/
		if ((NO_INDEX != third_button) && (NO_INDEX == fourth_button))
		{
			/*匹配始端按钮和终端按钮，变更进路都是特定的*/
			if ((ILT[searched_routes[0]].change_button != NO_INDEX)
				&& (ILT[searched_routes[0]].change_button1 == NO_INDEX)
				&& (ILT[searched_routes[0]].change_button == second_button)
				&& (ILT[searched_routes[0]].end_button == third_button))
			{
				result = CI_TRUE;
			}
		}
		
		/*按下四个按钮*/
		if ((NO_INDEX != third_button) && (NO_INDEX != fourth_button))
		{
			/*匹配始端按钮和终端按钮，变更进路都是特定的*/
			if ((ILT[searched_routes[0]].change_button != NO_INDEX)
				&& (ILT[searched_routes[0]].change_button1 != NO_INDEX)
				&& (ILT[searched_routes[0]].change_button == second_button)
				&& (ILT[searched_routes[0]].change_button1 == third_button)
				&& (ILT[searched_routes[0]].end_button == fourth_button))
			{
				result = CI_TRUE;
			} 
		}
	}
	return result;
}
/****************************************************
函数名:    is_target_nodes
功能描述: 判断当前搜索出的进路是否满足进路要求
返回值:    
参数:      routes       进路索引号数组
           route_count  进路数
作者  :    CY
日期  ：   2011/12/2
****************************************************/
CI_BOOL is_target_routes(int16_t searched_routes[],int32_t ILT_count)
{
    CI_BOOL result = CI_FALSE;
    FUNCTION_IN;

	/*普通进路判断*/
	result = normal_route_judge(ILT_count, searched_routes);

	if (IsFALSE(result))
	{
		/*延续进路处理*/
		result = successive_route_judge(ILT_count, searched_routes);
	}
	if (IsFALSE(result))
	{
		/*通过进路处理*/
		result = passing_route_judge(ILT_count, searched_routes);
	}
    FUNCTION_OUT;
    return result;
}

/****************************************************
函数名:   get_next_index
功能描述: 根据当前进路获取下一个起始节点
返回值:    节点索引号
参数:      route_index 进路索引号
作者  :    CY
日期  ：   2011/12/2
****************************************************/
int16_t get_next_index(int16_t ILT_index)
{
	int16_t result = NO_INDEX;
	int16_t end_point = ILT[ILT_index].nodes[ILT[ILT_index].nodes_count - 1];
	EN_node_type type = gn_type(end_point);
	int16_t i;
	FUNCTION_IN;

	/*hjh 2013-4-22 进路上最后一个节点是股道或无岔区段，则以倒数第2个节点开始搜索*/
	if ((type == NT_TRACK)|| (type == NT_NON_SWITCH_SECTION))
	{
		end_point = ILT[ILT_index].nodes[ILT[ILT_index].nodes_count - 2];
		type = gn_type(end_point);
	}
	/*进路上最后一个节点不是股道或无岔区段，则按照终端按钮搜索*/
	else
	{
		end_point = gb_node(ILT[ILT_index].end_button);
		type = gn_type(end_point);
	}
	/*并置和差置信号机,出站信号机，出站兼调车*/
	if (NT_JUXTAPOSE_SHUNGTING_SIGNAL == type || NT_DIFF_SHUNTING_SIGNAL == type
		//|| NT_SINGLE_SHUNTING_SIGNAL == type 
		|| NT_OUT_SIGNAL == type || NT_OUT_SHUNTING_SIGNAL == type
		|| NT_ROUTE_SIGNAL == type || NT_TRAIN_END_BUTTON == type)
	{
		result = (int16_t)(signal_nodes[end_point].property & 0xFFFF);
		/*hjh 2013-4-25 判断获取结果的类型是不是信号机*/
		for (i = 0; i < TOTAL_SIGNAL_NODE; i++)
		{
			if ((result != NO_INDEX) && IsFALSE(is_signal(result)))
			{
				/*不是信号机则沿着进路方向继续查找*/
				if (gn_direction(gb_node(ILT[ILT_index].start_button)) == DIR_UP)
				{
					result = gn_previous(result);
				}
				else
				{
					result = gn_next(result);
				}
			}
			else
			{
				break;
			}
		}		
	}
	/*单置信号机*/
	if (NT_SINGLE_SHUNTING_SIGNAL == type)
	{
		result = end_point;
	}
	/*hjh 2015-8-12 进路信号机处的特殊处理*/
	if ((type == NT_STUB_END_SHUNTING_SIGNAL)
		&& ((gn_type(gb_node(ILT[ILT_index].start_button)) == NT_ROUTE_SIGNAL) || (gn_type(gb_node(ILT[ILT_index].start_button)) == NT_JUXTAPOSE_SHUNGTING_SIGNAL) 
		|| (gn_type(gb_node(ILT[ILT_index].start_button)) == NT_DIFF_SHUNTING_SIGNAL) || (gn_type(gb_node(ILT[ILT_index].start_button)) == NT_STUB_END_SHUNTING_SIGNAL))
		&& (gn_direction(end_point) == gn_direction(gb_node(ILT[ILT_index].start_button))))
	{
		result = end_point;
	}
	/*延续进路的始端信号点获取*/
	if(result == NO_INDEX && IsTRUE(is_successive_ILT(ILT_index)))
	{
		if (gn_direction(end_point)== DIR_DOWN)
		{
			result = gn_previous(gn_previous(end_point));
		}
		else
		{
			if (gn_direction(end_point)== DIR_UP)
			{
				result = gn_next(gn_next(end_point));
			}
		}
	}
	/*防护*/
	if ((result == NO_INDEX) || IsFALSE(is_signal(result)))
	{
		result = NO_INDEX;
	}
	FUNCTION_OUT;
	return result;
}

/****************************************************
函数名:    is_dead_route
功能描述:  是否为最后一条进路，改进路的末端为死节点
返回值:    
参数:      route_index
作者  :    CY
日期  ：   2011/12/6
****************************************************/
CI_BOOL is_dead_route( int16_t ILT_index ) 
{
	CI_BOOL result = CI_FALSE;
	result = is_dead_node(gb_node(ILT[ILT_index].end_button),
		signal_nodes[gb_node(ILT[ILT_index].start_button)].direction,CI_FALSE);
	return result;
}

/****************************************************
函数名:    ILT_search_route
功能描述:  联锁表型进路搜索
返回值:    
参数:      route
作者  :    CY
日期  ：   2011/12/2
****************************************************/
void ILT_search_route (int16_t searched_routes[MAX_CHANGE_ROUTE][MAX_ILT_PER_ROUTES])
{
	int16_t sp = 0;
	int16_t ILT_index=0;
	int16_t same_index = 0;
	int16_t start_signal = gb_node(first_button);
	int16_t i;

	FUNCTION_IN;

	while(sp >= 0)
	{
		/*要求的始端信号和进路始端信号相同，并且他们的始端按钮也要相同，减少查找次数*/
		if( gb_node(ILT[ILT_index].start_button) == start_signal &&
			((ILT[ILT_index].start_button == signal_nodes[start_signal].buttons[0]) ||
			(ILT[ILT_index].start_button == signal_nodes[start_signal].buttons[1])))
		{
			searched_routes[same_index][sp] = ILT_index;
			/*匹配始终端按钮*/
			if (IsTRUE(is_target_routes(searched_routes[same_index],sp + 1)))
			{			
				/*搜索到始端按钮和终端按钮能够匹配的进路*/
				for (i = 0; i <= sp; i++)
				{
					searched_routes[same_index + 1][i] = searched_routes[same_index][i];
				}
				same_index ++;
				ILT_index ++;
			}			
			else if (IsTRUE(is_dead_route(ILT_index)) || (NO_INDEX == get_next_index(ILT_index)))
			{
				/*按钮匹配不成功，且进路前方没有进路，则继续往下搜索*/
				ILT_index ++;
			} 	
			else
			{
				/*获取前方进路的始端后，从联锁表的头部开始搜索*/
				start_signal = get_next_index(ILT_index);			
				sp++;
				ILT_index = 0;
			}
		}
		else
		{
			/*完全不匹配，所以继续往下搜索*/
			ILT_index++;
		}
		/*联锁表搜索完了还没有搜索到符合要求的进路，所以要退栈*/
		if (ILT_index >= TOTAL_ILT)
		{
			sp--;
			if (sp >= 0)
			{
				/*以当前堆栈顶的进路的始端信号继续往下搜索*/
				start_signal = gb_node(ILT[searched_routes[same_index][sp]].start_button);
				/*退栈后，从前面一条进路开始，在联锁表中继续往下找*/
				ILT_index = searched_routes[same_index][sp]+1;
			}
			/*退栈*/			
			searched_routes[same_index][sp + 1] = NO_INDEX;
		}
	}

	FUNCTION_OUT;
}

/****************************************************
函数名:    set_route
功能描述:  设置进路，将正确的进路添加到进路表中，并设置信号节点的进路属性
返回值:    
参数:      ILT_routes 联锁表索引号数组
           routes_count 进路数
作者  :    CY
日期  ：   2011/12/2
****************************************************/
void set_route(int16_t ILT_routes[])
{
	int16_t j = 0;
	route_t temp_index[MAX_ILT_PER_ROUTES];
	int16_t cur_index;
	route_t bwr = NO_ROUTE,i = 0;
	int16_t route_count = 0;
	node_t successive_start_signal = NO_INDEX;
	route_t successive_route = NO_INDEX;

	FUNCTION_IN;

	memory_set(temp_index,0xFF,sizeof(temp_index));
	for (j = 0; j < MAX_ILT_PER_ROUTES && ILT_routes[j] != NO_INDEX; j++)
	{
		for (i = 0; i < MAX_ROUTE; i++)
		{     
			/*初始化基本的联锁表数据*/
			if(NO_INDEX == routes[i].ILT_index)
			{
				routes[i].ILT_index = ILT_routes[j];
				routes[i].state = RS_SELECTED;
				routes[i].current_cycle = CICycleInt_GetCounter();
				temp_index[route_count++] = (route_t)i;	
                CIHmi_SendDebugTips("搜索到进路：%s ----> %s",device_name[gr_start_button(i)],device_name[gr_end_button(i)]);
				break;
			}
		}
	}
	/*设置进路关联属性*/
	for (j = 0; j < route_count; j++)
	{
		routes[temp_index[j]].backward_route = (j == 0u) ? NO_INDEX :  temp_index[j-1];
		routes[temp_index[j]].forward_route = temp_index[j+1];

		/*获取延续部分始端信号*/
		successive_start_signal = get_next_index(routes[temp_index[j]].ILT_index);
		if (successive_start_signal != NO_INDEX)
		{
			successive_route = gn_belong_route(successive_start_signal);
		}
		/*hjh 2013-4-22 存在以延续部分始端信号为始端的列车进路*/
		if (IsTRUE(have_successive_route(temp_index[j]))
			&& (successive_route != NO_INDEX)
			&& (gr_start_signal(successive_route) == successive_start_signal)
			//&& (gr_state(successive_route) == RS_ROUTE_LOCKED)
			&& ((gr_type(successive_route) == RT_TRAIN_ROUTE) || (gr_type(successive_route) == RT_SUCCESSIVE_ROUTE)))
		{
			/*设置前后进路的关联属性*/
			routes[successive_route].backward_route = temp_index[j];
			routes[temp_index[j]].forward_route = successive_route;
			routes[successive_route].other_flag = ROF_SUCCESSIVE;
		}
		/*设置延续进路标志*/
		bwr = gr_backward(temp_index[j]);
		/*普通延续进路*/
		if ((bwr != NO_ROUTE)
			&& ((gr_type(temp_index[j]) == RT_TRAIN_ROUTE) || (gr_type(temp_index[j]) == RT_SUCCESSIVE_ROUTE))
			&& (IsTRUE(have_successive_route(bwr))))
		{
			routes[temp_index[j]].other_flag = ROF_SUCCESSIVE;
		}
	}
	/*设置节点所属进路*/
	for (j = 0; j < route_count; j++)
	{
		for ( i = 0; i < gr_nodes_count(temp_index[j]); i++)
		{
			cur_index = gr_node(temp_index[j],i);
			/*hjh 2015-8-11 进路信号机处的调车进路，终端是同方向的尽头线信号机*/
			if (i == (gr_nodes_count(temp_index[j]) - 1) 
				&& (gn_direction(cur_index) == gn_direction(gr_start_signal(temp_index[j])))
				&& (gr_type(temp_index[j]) == RT_SHUNTING_ROUTE)
				&& ((gn_type(gr_start_signal(temp_index[j])) == NT_ROUTE_SIGNAL) || (gn_type(gr_start_signal(temp_index[j])) == NT_JUXTAPOSE_SHUNGTING_SIGNAL)
				|| (gn_type(gr_start_signal(temp_index[j])) == NT_DIFF_SHUNTING_SIGNAL) || (gn_type(gr_start_signal(temp_index[j])) == NT_STUB_END_SHUNTING_SIGNAL))
				&& (gn_type(cur_index) == NT_STUB_END_SHUNTING_SIGNAL))
			{
				continue;
			}
			if ((gn_belong_route(cur_index) == NO_INDEX) && 
				!((gn_type(cur_index) == NT_SINGLE_SHUNTING_SIGNAL) && 
				(i+1 == gr_nodes_count(temp_index[j]))))
			{
				sn_belong_route(cur_index,temp_index[j]);
			}			
		}
	}
	/*设置进路关联的半自动*/
	for (j = 0; j < route_count; j++)
	{
		for (i = 0; i < TOTAL_SEMI_AUTO_BLOCK; i++)
		{
			if (gb_node(ILT[routes[temp_index[j]].ILT_index].end_button) == semi_auto_block_config[i].entry_signal)
			{
				semi_auto_block_config[i].belong_route = temp_index[j];
				break;
			}
		}
		for ( i = 0; i < TOTAL_SEMI_AUTO_BLOCK; i++)
		{
			if (semi_auto_block_config[i].entry_signal == gr_start_signal(temp_index[j]))
			{
				semi_auto_block_config[i].belong_route = temp_index[j];
			}
		}
	}

	FUNCTION_OUT;
}