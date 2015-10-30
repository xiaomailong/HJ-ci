/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年12月9日 13:57:42
用途        : 联锁表管理
历史修改记录: v1.0    创建
**********************************************************************/

#include "throat_lock.h"
#include "init_manage.h"
#include "utility_function.h"
#include "global_data.h"

extern CI_BOOL read_SNT(char_t* StationName);
extern CI_BOOL read_ILT(char_t* StationName);
extern CI_BOOL read_CFG(char_t* StationName);
/****************************************************
函数名:    is_face_switch
功能描述:  判断是否为对向道岔
返回值:    
参数:      route_direction 进路方向
           node_index      节点索引号
作者  :    CY
日期  ：   2011/12/2
****************************************************/
CI_BOOL is_face_switch(uint8_t route_direction,int16_t node_index)
{
    CI_BOOL result = CI_FALSE;
	EN_node_direction nd = gn_direction(node_index);
    FUNCTION_IN;
    /*进路为下行方向时，左上，左下方向的道岔为顺向道岔*/
    if(DIR_DOWN == route_direction && 
        ((DIR_LEFT_DOWN == nd ) || (DIR_LEFT_UP == nd)))
        result = CI_TRUE;
    /*进路为上行方向时，右上，右下方向的道岔为顺向道岔*/
    if(DIR_UP == route_direction && 
        ((DIR_RIGHT_DOWN == nd) || (DIR_RIGHT_UP == nd)))
        result = CI_TRUE;
	result = (IsTRUE(result)?CI_FALSE:CI_TRUE);
    FUNCTION_OUT;
    return result;
}

/****************************************************
功能描述:  初始化联锁表
返回值:    
作者  :    CY
日期  ：   2011/12/9
****************************************************/
int32_t ilt_manage_init(void)
{
    register int16_t i,j;
    int16_t nodes[MAX_NODES_PS];
    int16_t node_count=0;
    uint16_t temp;

    /*下面这些其实要在信号机信息初始化好之后才能执行*/
    for (i = 0; i < TOTAL_ILT; i++)
    {
        modify_resolve_switch(i);
        memset(nodes,0xFF,sizeof(nodes));
        /*搜索进路上的节点*/
        node_count = graphic_search_route(i,nodes);
        /*初始化联锁表总节点表*/
        ILT[i].nodes_count = node_count;
        for (j = 0; j < node_count; j++)
        {
            if (j ==  node_count - 1 && gn_type(nodes[j]) == NT_SINGLE_SHUNTING_SIGNAL)
            {
                ILT[i].nodes_count--;
                ILT[i].nodes[j] = NO_INDEX;
                continue;
            }
            ILT[i].nodes[j] = nodes[j];
        }
        //log_msg("\n");
        /*修改联锁表中道岔列表的索引号，将双动道岔进路中的那一动填在此表中*/
        modify_switch(i);
        check_straight_route(i);
    }
    for (i = 0; i< TOTAL_SIGNAL_NODE; i++)
    {
        temp = signal_nodes[i].input_address;
        input_address[(temp>>8)&0xFF][(temp>>2)&0x3F][temp&0x03] = (int16_t)i;
    }
	return 0;
}

/****************************************************
函数名:    init_system_var
功能描述:  初始化全局变量
返回值:    
作者  :    CY
日期  ：   2011/11/30
****************************************************/
void init_system_var()
{
	int32_t i;

	for (i=0; i< TOTAL_BUTTONS; i++)   /*初始化按钮表*/
	{		
		buttons[i].node_index = NO_INDEX;
		buttons[i].button_index = NO_INDEX;
		buttons[i].button_type = BT_ERROR;
	}

	memset(signal_nodes,0xFF,sizeof(signal_nodes));/*初始化信号节点表*/
	memset(ILT,0xFF,sizeof(ILT));/*初始化联锁表*/

	for(i=0; i < MAX_ROUTE; i ++)/*初始化进路表*/
	{
		routes[i].ILT_index = NO_INDEX;
		routes[i].forward_route = NO_INDEX;
		routes[i].backward_route = NO_INDEX;
		routes[i].state = RS_ERROR;
		routes[i].state_machine = RSM_INIT;
		routes[i].other_flag = ROF_ERROR;
	}

	/*输入地址映射区*/
	memset(input_address,0xff,sizeof(input_address));

	memset(clear_sections,0xff,sizeof(clear_sections));

    can_build_route = CI_FALSE;          /*构成进路建立命令，开始建立进路*/
	wait_switches_count = 0;		/*待转换道岔数*/
	switching_count = 0;			/*正在转换道岔数*/
}

/****************************************************
函数名:    is_dead_node
功能描述: 判断当前节点是否为死节点
返回值:    
参数:      route_direction 进路方向
           node_index      节点索引号
作者  :    CY
日期  ：   2011/12/2
****************************************************/
CI_BOOL is_dead_node( int16_t node_index,EN_node_direction route_direction,CI_BOOL track_stop )
{
    CI_BOOL result = CI_FALSE;
	EN_node_type nt;

	if(NO_INDEX != node_index)
	{
		nt = gn_type(node_index);

		/*股道为死节点*/
		if(IsFALSE(track_stop) && (NT_TRACK == nt))
			result = CI_TRUE;
		if (NO_INDEX == gn_forword(route_direction,node_index)
			&& (NT_SWITCH == nt && NO_INDEX == gn_reverse_dir(route_direction,node_index)))
		{
			result = CI_TRUE;
		}
		///*进路为下行方向时,本节点无下一个节点*/
		//if(DIR_DOWN == route_direction &&
		//	(NT_SWITCH != nt && NO_INDEX == gn_next(node_index) ||
		//	(NT_SWITCH == nt && NO_INDEX == gn_reverse(node_index))))
		//	result = CI_TRUE;
		///*进路为上行方向时，本节点无前一个节点*/
		//if(DIR_UP == route_direction && 
		//	((NO_INDEX == gn_previous(node_index)) ||
		//	(NT_SWITCH == nt && NO_INDEX == gn_reverse(node_index))))
		//	result = CI_TRUE;
	}
	else
	{
		result = CI_TRUE;
	}
    return result;
}

/****************************************************
函数名:    get_nodes_switch_state
功能描述:  获取道岔节点状态（用以判断决定进路方向的关键道岔的位置是否满足条件）
返回值:	   result = 0		不满足条件
		   result = 0x55	满足条件
参数:      nodes
参数:      node_count
参数:      i

作者  :    LYC
日期  ：   2012/6/21
****************************************************/
uint16_t get_nodes_switch_state(int16_t nodes[],int32_t node_count,int16_t i)
{
	uint16_t result = 0; 
	if (node_count > 1 + i)
	{
		if (gn_reverse(nodes[i]) == nodes[i+1])
		{
			result = 0;
		}
		if (gn_next(nodes[i]) == nodes[i+1])
		{
			result = 0x55;
		}
	}
	return result;
}

/****************************************************
函数名:    is_target_nodes
功能描述: 判断当前搜索出的节点是否满足进路要求
返回值:    
参数:      nodes      节点数组
           node_count  节点数
作者  :    CY
日期  ：   2011/12/2
****************************************************/
//CI_BOOL is_target_nodes(int16_t ILT_index,int16_t button[],int16_t nodes[],int32_t node_count)
//{
//    CI_BOOL result = CI_FALSE;
//    int16_t i,state = 0;
//    FUNCTION_IN;
//	if (node_count > 0 && nodes[0] == gb_node(button[0]))
//		state = 1;
//    for( i = 1; i < node_count; i++)
//    {
//        if (1 == state && nodes[i] == gb_node(button[1]))
//			state = 2;
//		if (2 == state && nodes[i] == gb_node(button[2]))
//			state = 3;
//		if (3 == state && nodes[i] == gb_node(button[3]))
//			state = 4;
//    }
//	if (2 == state && NO_INDEX == button[2])
//		result = CI_TRUE;
//	if (3 == state && NO_INDEX == button[3])
//		result = CI_TRUE;
//	if (4 == state)
//		result = CI_TRUE;
//	if (IsTRUE(result))
//	{
//		if (ILT[ILT_index].resolve_switch1.index != NO_INDEX)
//		{
//			for( i = 0; i < node_count; i++)
//			{
//				if (nodes[i] == ILT[ILT_index].resolve_switch1.index 
//					&& IsTRUE(is_face_switch(gn_direction(gb_node(ILT[ILT_index].start_button)),nodes[i]))
//					&& ILT[ILT_index].resolve_switch1.state == get_nodes_switch_state(nodes,node_count,i))
//					break;
//			}
//			if ( i == node_count)
//			{
//				result = CI_FALSE;
//			}
//		}
//		if (ILT[ILT_index].resolve_switch2.index != NO_INDEX)
//		{
//			for( i = 0; i < node_count; i++)
//			{
//				if (nodes[i] == ILT[ILT_index].resolve_switch2.index 
//					&& IsTRUE(is_face_switch(gn_direction(gb_node(ILT[ILT_index].start_button)),nodes[i]))
//					&& ILT[ILT_index].resolve_switch2.state == get_nodes_switch_state(nodes,node_count,i))
//					break;
//			}
//			if ( i == node_count)
//			{
//				result = CI_FALSE;
//			}
//		}
//	}
//    FUNCTION_OUT;
//    return result;
//}

/****************************************************
函数名:    graphic_search_route
功能描述:  站场型搜索进路函数
返回值:    
参数:      button[]  按钮数组
           nodes[]  用于返回搜索出的节点
作者  :    CY
日期  ：   2011/12/2
****************************************************/
//int16_t graphic_search_route (int16_t ILT_index,int16_t button[],int16_t nodes[])
//{
//	int16_t sp = 0,i;
//    int16_t next_index;
//    EN_node_direction route_direction;
//	EN_node_type nt;
//    CI_BOOL poped, passing_route;
//
//	FUNCTION_IN;
//	
//	if ( NO_INDEX != button[0] && NO_INDEX != button[1])
//	{	
//
//		nodes[sp] = gb_node(button[0]);
//		route_direction = gn_direction(nodes[sp]);
//		passing_route = is_passing_ILT(ILT_index);
//		
//		/*循环搜索整条进路*/
//		while(IsFALSE(is_target_nodes(ILT_index, button,nodes,sp+1)))
//		{
//			next_index = NO_INDEX;
//			if (IsTRUE(is_dead_node(nodes[sp],route_direction,passing_route)))
//			{
//				poped = CI_FALSE;
//				while(IsFALSE(poped))
//				{    
//					if (sp < 1)  /*未搜索到进路*/
//					{
//						PRINTF1("检查进路:%d",ILT_index);
//						FUNCTION_OUT;
//						return sp;
//					}
//					sp--;     
//					//退栈条件判断。最后一个条件用来判断是从哪个方向退回来的
//					if(sp >=1 && NT_SWITCH == gn_type(nodes[sp]) 
//						&& IsTRUE(is_face_switch(route_direction,nodes[sp]))  //对象道岔
//						&& gn_reverse(nodes[sp]) != nodes[sp+1])
//					{
//						
//						next_index = gn_reverse(nodes[sp]);
//						for (i = 0;i <= sp; i++)
//						{
//							if (nodes[i] == next_index)
//								break;
//						}
//						if (i > sp)
//							poped = CI_TRUE;
//					}
//				}
//
//			}
//			else
//			{/*非死节点，继续往前搜索*/
//				if (NT_SWITCH == gn_type(nodes[sp]))
//				{/*道岔节点的下一个节点入栈*/
//					if(IsFALSE(is_face_switch(route_direction,nodes[sp])))
//					{/*顺向道岔*/
//						next_index = gn_previous(nodes[sp]);
//					}
//					else
//					{/*对向道岔*/
//						next_index = gn_next(nodes[sp]);
//						for (i = sp;i >= 0; i--)
//						{
//							if (nodes[i] == next_index)
//								break;
//						}
//						if (i >= 0)
//							next_index =  gn_reverse(nodes[sp]);
//					}
//				} 
//				else
//				{/*非道岔的信号点的下一个节点入栈*/
//					next_index = (DIR_DOWN == route_direction)?gn_next(nodes[sp]):gn_previous(nodes[sp]);
//				}            
//			}
//			if (sp == MAX_NODES_PS - 1)
//			{	
//				PRINTF("进路搜索错误");
//				for (i = 0; i <= sp; i++)
//				{
//					PRINTF1("%s ",device_name[nodes[i]]);
//				}
//				PRINTF("");
//				break;
//			}
//			nodes[++sp] = next_index;
//		}
//		nt = gn_type(nodes[sp]);
//		if ( nt == NT_OUT_SHUNTING_SIGNAL || nt == NT_OUT_SIGNAL || nt == NT_DIFF_SHUNTING_SIGNAL
//			|| nt == NT_LOCODEPOT_SHUNTING_SIGNAL || nt == NT_STUB_END_SHUNTING_SIGNAL)
//				nodes[++sp]= (DIR_DOWN == route_direction)?gn_next(nodes[sp]):gn_previous(nodes[sp]);
//    }
//
//	FUNCTION_OUT;
//	for (i = 0; nodes[i] != NO_INDEX && i < MAX_NODES_PS && i <=sp ; i++)
//	{
//	}
//    return i;
//}

/****************************************************
函数名:    get_next_node
功能描述:  按照联锁表，获取下一个节点
返回值:    
参数:      index_t ILT_index
参数:      node_t nodes[]
参数:      index_t sp
作者  :    CY
日期  ：   2012/7/19
****************************************************/
node_t get_next_node(index_t ILT_index, node_t nodes[],index_t sp)
{
	node_t result = NO_INDEX;
	int16_t i;
	EN_node_direction nd = (EN_node_direction)signal_nodes[gb_node(ILT[ILT_index].start_button)].direction;
	int16_t current_node = nodes[sp];

	if (signal_nodes[current_node].type == NT_SWITCH && IsTRUE(is_face_switch(nd,current_node)))
	{
		for (i = 0; i < ILT[ILT_index].switch_count; i ++)
		{
			if (ILT[ILT_index].switches[i].index == current_node && (ILT[ILT_index].switches[i].state & 0xFF00) != 0x5500)
			{
				break;
			}
			if (gn_another_switch(ILT[ILT_index].switches[i].index) == current_node && (ILT[ILT_index].switches[i].state & 0xFF00) != 0x5500)
			{
				break;
			}
		}
		if (i < ILT[ILT_index].switch_count)
		{
			if ((ILT[ILT_index].switches[i].state & 0xFF) == 0x55)
			{
				result = gn_forword(nd,current_node);
			}
			else if ((ILT[ILT_index].switches[i].state & 0xFF) == 0x00)
			{
				result = gn_reverse(current_node);
			}
		}
	}
	else
	{
		result = gn_forword(nd,current_node);
	}
	return result;
}

CI_BOOL is_target_ILT(int16_t ILT_index, int16_t next_node)
{	
	if(next_node != NO_INDEX && gb_node(ILT[ILT_index].end_button) == next_node)
	{
		if (gn_type(gb_node(ILT[ILT_index].start_button)) == NT_ENTRY_SIGNAL )
		{
			if (!(gn_direction(gb_node(ILT[ILT_index].start_button)) == gn_direction(next_node) && IsTRUE(is_out_signal(next_node))))
			{
				return CI_FALSE;
			}
		}
		return CI_TRUE;
	}
	if (next_node != NO_INDEX && gn_type(next_node) == NT_TRAIN_END_BUTTON)
	{
		return CI_TRUE;
	}
	return CI_FALSE;
}

/****************************************************
函数名:    graphic_search_route
功能描述:  按照联锁表搜索节点
返回值:    
参数:      int16_t ILT_index
参数:      int16_t button[]
参数:      int16_t nodes[]
作者  :    CY
日期  ：   2012/7/19
****************************************************/
int16_t graphic_search_route(int16_t ILT_index,int16_t nodes[])
{
	node_t next_node = gb_node(ILT[ILT_index].start_button);
	index_t sp=0;
	CI_BOOL stop = CI_FALSE;

	while(NO_INDEX != next_node && (MAX_NODES_PS > (sp+2)))
	{
		nodes[sp++] = next_node;

		if (IsTRUE(stop))
		{
			/*本进路始端信号机是进站或进路信号机*/
			if ((gn_type(gb_node(ILT[ILT_index].start_button)) == NT_ENTRY_SIGNAL 
				|| gn_type(gb_node(ILT[ILT_index].start_button)) == NT_ROUTE_SIGNAL) )
			{
				/*hjh 2013-4-1 本判断条件增加同方向的进路信号机*/
				/*hjh 2014-2-25 增加同方向驼峰信号机*/
				/*下一节点是同方向的出站信号机或进路信号机则删除数组中的最后一个节点并停止*/
				if (gn_direction(gb_node(ILT[ILT_index].start_button)) == gn_direction(next_node) 
					&& (IsTRUE(is_out_signal(next_node)) || (gn_type(next_node) == NT_ROUTE_SIGNAL) || (gn_type(next_node) == NT_HUMPING_SIGNAL))
					&& (ILT[ILT_index].route_kind == RT_TRAIN_ROUTE))
				{
					nodes[--sp] = NO_INDEX;
				    break;
				}
				/*hjh 2014-2-27 增加进路信号机处的调车进路搜索*/
				if (gn_direction(gb_node(ILT[ILT_index].start_button)) == gn_direction(next_node) 
					&& (IsTRUE(is_out_signal(next_node)) || (gn_type(next_node) == NT_ROUTE_SIGNAL) || IsTRUE(is_shunting_signal(next_node)))
					&& (ILT[ILT_index].route_kind == RT_SHUNTING_ROUTE))
				{
					nodes[--sp] = NO_INDEX;
					break;
				}
				/*hjh 2013-1-25 添加处理搜索中遇到列车终端按钮的情况*/
				if (gn_type(nodes[sp - 2]) == NT_TRAIN_END_BUTTON)
				{
					/*若列车终端按钮的下一节点是股道或无岔区段，则将其也添加到nodes数组中*/
					if (next_node != NO_INDEX && ((gn_type(next_node) ==  NT_TRACK) || (gn_type(next_node) == NT_NON_SWITCH_SECTION)))
					{
						break;
					}
					else
					{
						nodes[--sp] = NO_INDEX;
						break;
					}
				}
				/*hjh 2015-08-11 尽头线信号机处的特殊处理*/
				if ((ILT[ILT_index].route_kind == RT_SHUNTING_ROUTE) 
					&& (gn_type(nodes[sp - 2]) == NT_STUB_END_SHUNTING_SIGNAL))
				{
					if ((gn_type(nodes[sp - 1]) != NT_STUB_END_SECTION)
						&& (gn_type(nodes[sp - 1]) != NT_NON_SWITCH_SECTION))
					{
						nodes[--sp] = NO_INDEX;
						break;
					}
				}
			}
			/*否则停止搜索*/
			else
			{
				break;
			}
			
		}
		/*搜索到终端按钮处*/
		else if(gb_node(ILT[ILT_index].end_button) == next_node)
		{
			stop = CI_TRUE;	
			/*终端按钮是单置或并置调车信号机或进站信号机时停止*/
			if (gn_type(next_node) == NT_SINGLE_SHUNTING_SIGNAL || gn_type(next_node) == NT_JUXTAPOSE_SHUNGTING_SIGNAL|| gn_type(next_node) == NT_ENTRY_SIGNAL)
			{
				break;
			}
		}
		/*获取下一节点*/
		next_node = get_next_node(ILT_index,nodes,sp - 1);
	}
	/*获取下一节点*/
	next_node = get_next_node(ILT_index,nodes,sp - 1);
	/*若下一节点是股道或无岔区段，则将其也添加到nodes数组中*/
	if (next_node != NO_INDEX && ((gn_type(next_node) ==  NT_TRACK) || (gn_type(next_node) == NT_NON_SWITCH_SECTION)))
	{
		nodes[sp++] = next_node;
	}

	return sp;
}
/****************************************************
函数名:    modify_switch
功能描述:  修正联锁表中进路上的双动道岔
返回值:    
参数:      route_index
作者  :    CY
日期  ：   2011/12/20
****************************************************/
void modify_switch(int16_t ILT_index)
{
	int16_t i,j,k,another;
	for (i = 0; i < ILT[ILT_index].switch_count; i++ )
	{
		another = gn_another_switch(ILT[ILT_index].switches[i].index);
		if(NO_INDEX != another)
		{/*判断双动道岔*/
			for (j = 0; j < ILT[ILT_index].nodes_count; j++)
			{
			    if (NT_SWITCH_SECTION == gn_type(ILT[ILT_index].nodes[j]))
			    {
					for (k = 0; k < 4; k++)
					{
						if (another == gn_section_switch(ILT[ILT_index].nodes[j],k))
							ILT[ILT_index].switches[i].index = another;
					}
			    }
			}
		}
	}
}

/****************************************************
函数名:    modify_resolve_switch
功能描述:  添加决定进路方向的关键道岔及位置
返回值:  
参数:      i

作者  :    LYC
日期  ：   2012/6/21
****************************************************/
void modify_resolve_switch( int16_t i ) 
{
	//node_t another;
	//if (ILT[i].resolve_switch1.index != NO_INDEX)
	//{
	//	if ( IsFALSE(is_face_switch(gn_direction(gb_node(ILT[i].start_button)),ILT[i].resolve_switch1.index)))
	//	{
	//		another = gn_another_switch(ILT[i].resolve_switch1.index);
	//		if (another != NO_INDEX &&  IsTRUE(is_face_switch(gn_direction(gb_node(ILT[i].start_button)),another)))
	//		{
	//			ILT[i].resolve_switch1.index = another;
	//		}
	//	}
	//}
	//if (ILT[i].resolve_switch2.index != NO_INDEX)
	//{
	//	if ( IsFALSE(is_face_switch(gn_direction(gb_node(ILT[i].start_button)),ILT[i].resolve_switch2.index)))
	//	{
	//		another = gn_another_switch(ILT[i].resolve_switch2.index);
	//		if (another != NO_INDEX &&  IsTRUE(is_face_switch(gn_direction(gb_node(ILT[i].start_button)),another)))
	//		{
	//			ILT[i].resolve_switch2.index = another;
	//		}
	//	}
	//}
}

/****************************************************
函数名:    init_ILT
功能描述:  初始化联锁表
返回值:    
作者  :    CY
日期  ：   2011/12/9
****************************************************/
void init_ILT()
{
	int16_t i,j;
	int16_t nodes[MAX_NODES_PS];
	int16_t node_count=0;
	uint16_t temp;

	for (i = 0; i < TOTAL_ILT; i++)
	{
		modify_resolve_switch(i);
		memset(nodes,0xFF,sizeof(nodes));
		/*搜索进路上的节点*/
		node_count = graphic_search_route(i,nodes);
		/*初始化联锁表总节点表*/
		ILT[i].nodes_count = node_count;
		for (j = 0; j < node_count; j++)
		{
			//PRINTF1("%s ",device_name[nodes[j]]);
			if (j ==  node_count - 1 && gn_type(nodes[j]) == NT_SINGLE_SHUNTING_SIGNAL)
			{
				ILT[i].nodes_count--;
				ILT[i].nodes[j] = NO_INDEX;
				continue;
			}
			ILT[i].nodes[j] = nodes[j];
		}
		//PRINTF("");
		/*修改联锁表中道岔列表的索引号，将双动道岔进路中的那一动填在此表中*/
		modify_switch(i);
		check_straight_route(i);
	}
	for (i = 0; i< TOTAL_SIGNAL_NODE; i++)
	{
		temp = signal_nodes[i].input_address;
		input_address[(temp>>8)&0xFF][(temp>>2)&0x3F][temp&0x03] = (int16_t)i;
	}
}

/****************************************************
函数名:    check_straight_route
功能描述:  检查进路是否为直股
返回值:  
参数:      ILT_index

作者  :    CY
日期  ：   2011/12/9
****************************************************/
void check_straight_route(index_t ILT_index)
{
	//CI_BOOL result = CI_TRUE;
	//index_t i,j,node ;
	//for (i = 0 ; i < ILT[ILT_index].nodes_count ; i++)
	//{
	//	node = ILT[ILT_index].nodes[i];
	//	if (signal_nodes[node].type == NT_SWITCH)
	//	{
	//		for (j = 0; j < ILT[ILT_index].switch_count ;j++)
	//		{
	//			if (ILT[ILT_index].switches[j].index == node)
	//			{
	//				if ((ILT[ILT_index].switches[j].state & 0xFF) != 0x55)
	//				{
	//					result = CI_FALSE;
	//				}
	//			}
	//		}
	//	}
	//}
	//ILT[ILT_index].straight_route = result;
}

/****************************************************
函数名:    reset_signal_nodes
功能描述:  复位信号点数据
返回值:  
参数:      

作者  :   CY
日期  ：   2011/12/9
****************************************************/
void reset_signal_nodes(void)
{
	int i;
	for (i=0; i < TOTAL_SIGNAL_NODE; i++)  /*初始化信号节点表*/
	{
#ifdef WIN32
		if(signal_nodes[i].type == (uint8_t)NT_SWITCH)
		{
			signal_nodes[i].state= (uint32_t)SWS_NORMAL;
		}
		if (signal_nodes[i].type > 0 && (signal_nodes[i].type <= (uint8_t)NT_ROUTE_SIGNAL || signal_nodes[i].type == NT_HUMPING_SIGNAL))
		{
			signal_nodes[i].state= (uint32_t)SGS_H;
		}
		if (signal_nodes[i].type >= (uint8_t)NT_SINGLE_SHUNTING_SIGNAL && signal_nodes[i].type <= (uint8_t)NT_LOCODEPOT_SHUNTING_SIGNAL)
		{
			signal_nodes[i].state= (uint32_t)SGS_A;
		}
		if (signal_nodes[i].type >= (uint8_t)NT_TRACK && signal_nodes[i].type < (uint8_t)NT_SECTION)
		{
			signal_nodes[i].state = (uint32_t)SCS_CLEARED;
		}
		if (signal_nodes[i].type == NT_CHECK_POINT || signal_nodes[i].type == NT_INDICATOR || signal_nodes[i].type == NT_REQUEST_AGREE_POINT
			|| signal_nodes[i].type == NT_INPUT || signal_nodes[i].type == NT_OUTPUT || signal_nodes[i].type == NT_INPUTOUTPUT)
		{
			signal_nodes[i].state = (uint32_t)SIO_NO_SIGNAL;
		}
		if (signal_nodes[i].type == NT_SEMI_AUTOMATIC_BLOCK)
		{
			signal_nodes[i].state = 0;
		}
		signal_nodes[i].state_machine = SCSM_INIT;
		signal_nodes[i].belong_route = NO_INDEX;
		signal_nodes[i].history_state = 0;
		signal_nodes[i].locked_flag = LT_UNLOCK;
		//signal_nodes[i].state_changed = CI_FALSE;
		signal_nodes[i].used = CI_FALSE;
		signal_nodes[i].time_count = NO_TIMER;
		signal_nodes[i].time_type = DTT_INIT;
#else
		if(signal_nodes[i].type == (uint8_t)NT_SWITCH)
		{
			signal_nodes[i].state= (uint32_t)SWS_NO_INDICATE;
		}
		if (signal_nodes[i].type > 0 && (signal_nodes[i].type <= (uint8_t)NT_ROUTE_SIGNAL || signal_nodes[i].type == NT_HUMPING_SIGNAL))
		{
			signal_nodes[i].state= (uint32_t)SGS_FILAMENT_BREAK;
		}
		if (signal_nodes[i].type >= (uint8_t)NT_SINGLE_SHUNTING_SIGNAL && signal_nodes[i].type <= (uint8_t)NT_LOCODEPOT_SHUNTING_SIGNAL)
		{
			signal_nodes[i].state= (uint32_t)SGS_FILAMENT_BREAK;
		}
		if (signal_nodes[i].type >= (uint8_t)NT_TRACK && signal_nodes[i].type < (uint8_t)NT_SECTION)
		{
			signal_nodes[i].state = (uint32_t)SCS_CLEARED;
		}
		if (signal_nodes[i].type == NT_CHECK_POINT || signal_nodes[i].type == NT_INDICATOR || signal_nodes[i].type == NT_REQUEST_AGREE_POINT)
		{
			signal_nodes[i].state = (uint32_t)SIO_NO_SIGNAL;
		}
		if (signal_nodes[i].type == NT_SEMI_AUTOMATIC_BLOCK)
		{
			signal_nodes[i].state = 0;
		}
		signal_nodes[i].state_machine = SCSM_INIT;
		signal_nodes[i].belong_route = NO_INDEX;
		signal_nodes[i].history_state = 0;
		signal_nodes[i].locked_flag = LT_UNLOCK;
		//signal_nodes[i].state_changed = CI_FALSE;
		signal_nodes[i].used = CI_FALSE;
		signal_nodes[i].time_count = NO_TIMER;
		signal_nodes[i].time_type = DTT_INIT;
#endif		
	}
}

/****************************************************
函数名:    reset_routes
功能描述:  初始化进路表
返回值:    
参数:      

作者  :    CY
日期  ：   2012/2/10
****************************************************/
void reset_routes(void)
{
	int16_t i;
	for (i=0 ; i < MAX_ROUTE; i ++)
	{
		routes[i].ILT_index = NO_INDEX;
		routes[i].state = RS_ERROR;
		routes[i].backward_route = NO_INDEX;
		routes[i].error_count = 0;
		routes[i].forward_route = NO_INDEX;
		routes[i].state_machine = RSM_INIT;
		routes[i].other_flag = ROF_ERROR;
	}
}