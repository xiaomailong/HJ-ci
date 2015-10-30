/*********************************************************************
Copyright (C), 2013,  Co.Hengjun, Ltd.

作者        : 何境泰
版本        : 1.0
创建日期    : 2013年10月29日 10:32:18
用途        : 双系输入数据同步
历史修改记录: v1.0    创建
**********************************************************************/
#include "util/ci_header.h"

#ifdef LINUX_ENVRIONMENT

#include "input_syn.h"
#include "syn_type.h"

#include "util/algorithms.h"
#include "util/utility.h"

#include "interlocking/inter_api.h"

/*
功能描述     : 双系校核同步信号节点表结构体操作
返回值       : 无
参数         : @node信号节点表指针
作者         : 何境泰
日期         : 2014年6月9日 15:48:01
*/
static inline void series_input_snt_print(const InputSntFrame* node)
{
    printf("\texpect_state              :%#x\n",node->expect_state);
    printf("\tstate                     :%#x\n",node->state);
    printf("\thistory_state             :%#x\n",node->history_state);
    printf("\tlocked_flag               :%d\n",node->locked_flag);
    printf("\tbelong_route              :%d\n",node->belong_route);
    printf("\ttime_count                :%d\n",node->time_count);
    printf("\tused                      :%s\n",CI_BoolPrint(node->used));

    return;
}
/*
 功能描述    : 双系校核输入数据进路表打印
 返回值      : 无
 参数        : @route 进路表指针
 作者        : 张彦升
 日期        : 2014年8月14日 9:13:55
*/
static inline void series_input_route_print(const InputRouteTable* route)
{
    printf("\tILT_index                     :%d\n",route->ILT_index);
    printf("\tstate                         :%#x\n",route->state);
    printf("\tstate_machine                 :%d\n",route->state_machine);
    printf("\tbackward_route                :%d\n",route->backward_route);
    printf("\tforward_route                 :%d\n",route->forward_route);
    printf("\tcurrent_cycle                 :%d\n",route->current_cycle);
    printf("\terror_count                   :%d\n",route->error_count);
    printf("\tother_flag                    :%d\n",route->other_flag);
    printf("\tapproach_unlock               :%s\n",CI_BoolPrint(route->approach_unlock));

    return;
}
/*
 功能描述    : 双系校核输入数据转换道岔打印
 返回值      : 无
 参数        : @switch 转换道岔指针
 作者        : 张彦升
 日期        : 2014年8月14日 9:13:55
*/
static void series_input_switch_print(const InputSwitching* wait_switches)
{
    printf("\tswitch_index                  :%d\n",wait_switches->switch_index);
    printf("\tneed_location                 :%d\n",wait_switches->need_location);
    printf("\ttimer_counter                 :%d\n",wait_switches->timer_counter);

    return;
}
/*
 功能描述    : 清除区段数据打印
 返回值      : 无
 参数        : @section 清除区段指针
 作者        : 张彦升
 日期        : 2014年8月14日 13:24:41
*/
static inline void series_input_cleared_section_print(const InputClearedSection* section)
{
    printf("\tcleared_section             :%d\n",section->section);
    printf("\tsection_start_time          :%d\n",section->start_time);
    return ;
}
/*
 功能描述    : 半自动闭塞数据打印
 返回值      : 无
 参数        : @block半自动闭塞数据指针
 作者        : 张彦升
 日期        : 2014年8月14日 9:13:55
*/
static inline void series_input_semi_auto_block_print(const InputSemiAutoBlock* block)
{
    printf("\tZJ                         :%d\n",block->ZJ);
    printf("\tFJ                         :%d\n",block->FJ);
    printf("\tstate                      :%d\n",block->state);
    printf("\tblock_state                :%d\n",block->block_state);

    return;
}
/*
功能描述    : 打印输入同步数据，方便调试
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2013年10月29日 10:46:07
*/
void CIInputSyn_Print(CIInputSynFrame* p_frame)
{
    int i = 0,j = 0;

    printf("type                        :%d\n",p_frame->type);
    printf("hash                        :%#x\n",p_frame->hash);
    printf("cycle_counter                          :%#x\n",p_frame->cycle_counter);
    printf("time_stamp                  :%#x\n",p_frame->time_stamp);
    printf("button1                     :%d\n",p_frame->button1);
    printf("button2                     :%d\n",p_frame->button2);
    printf("button3                     :%d\n",p_frame->button3);
    printf("button4                     :%d\n",p_frame->button4);
    printf("button5                     :%d\n",p_frame->button5);

    printf("wait_switches_count         :%d\n",p_frame->wait_switches_count);
    printf("switching_count             :%d\n",p_frame->switching_count);

    printf("commands:\n");
    for (i = 0;i < MAX_GETWAYS;i++)
    {
        for (j = 0;j < USING_EEU_PER_LAYER;j++)
        {
            printf("%08x ",p_frame->commands[i][j]);
        }
        printf("\n");
    }

    for (i = 0;i < MAX_SIGNAL_NODES;i++)
    {
        printf("signal node:%d\n",i);
        series_input_snt_print(&p_frame->node[i]);
    }

    for (i = 0;i < MAX_ROUTE;i++)
    {
        printf("route:%d\n",i);
        series_input_route_print(&p_frame->route[i]);
    }
    
    for (i = 0;i < MAX_WAIT_SWITCH;i++)
    {
        printf("wait_switches:%d\n",i);
        series_input_switch_print(&p_frame->wait_switches[i]);
    }

    for (i = 0;i < MAX_CONCURRENCY_SWITCH;i++)
    {
        printf("switching:%d\n",i);
        series_input_switch_print(&p_frame->switching[i]);
    }

    for (i = 0;i < MAX_CLEARING_SECTION;i++)
    {
        printf("clear section:%d\n",i);
        series_input_cleared_section_print(&p_frame->clear_section[i]);
    }

    for (i = 0;i < MAX_SEMI_AUTO_BLOCK;i++)
    {
        printf("semi auto block:%d\n",i);
        series_input_semi_auto_block_print(&p_frame->block[i]);
    }
    return ;
}

/*
功能描述    : 计算hash值，然后将其填充到结构当中并返回该hash值
返回值      : CRC码
参数        : 
作者        : 何境泰
日期        : 2013年10月29日 10:47:55
*/
uint16_t CIInputSyn_CalcHash(CIInputSynFrame* p_frame)
{
    uint16_t old_hash = p_frame->hash;
    uint16_t crc_code = 0;

    /*为了避免该域有值而导致计算结果不一致，而将此置为0*/
    p_frame->hash = 0;

    crc_code = CIAlgorithm_Crc16(p_frame,sizeof(CIInputSynFrame));
    p_frame->hash = crc_code;

    return old_hash;
}
/*
功能描述    : 检测帧类型和数据校验码错误
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2014年5月12日 16:08:46
*/
CI_BOOL CIInputSyn_Verify(CIInputSynFrame* p_frame)
{
    uint16_t old_hash = 0;

    /*检查数据校验码是否正确*/
    old_hash = CIInputSyn_CalcHash(p_frame);
    if (old_hash != p_frame->hash)
    {
#if 0
        CILog_Msg("验证双系输入帧校验码不一致:old_hash(%#x) != new_hash(%#x)",
            old_hash,p_frame->hash);
#endif
        return CI_FALSE;
    }
    /*检查帧数据类型*/
    if (ST_INPUT != p_frame->type)
    {
#if 0
        CILog_Msg("验证双系输入帧类型不一致:type(%d)",p_frame->type);
#endif

        return CI_FALSE;
    }

    return CI_TRUE;
}
/*
功能描述    : 初始化输入同步数据
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2013年10月29日 10:39:09
*/
int32_t CIInputSyn_Init(CIInputSynFrame* p_frame)
{
    p_frame->type = ST_INPUT;
    p_frame->hash = 0;
    p_frame->time_stamp = CI_GetTimeStamp();
    p_frame->cycle_counter = 0;

    p_frame->eeu_channel_a_ok = CI_TRUE;
    p_frame->eeu_channel_b_ok = CI_TRUE; /*电子单元通信状态*/
    p_frame->hmi_channel_a_ok = CI_TRUE;
    p_frame->hmi_channel_b_ok = CI_TRUE; /*控显机通信状态*/

    return 0;
}


/*
功能描述    : 输入数据装配
返回值      : 成功为0，失败为-1
参数        :
作者        : 张彦升
日期        : 2014年4月15日 13:52:49
*/
int32_t CIInputSyn_Assemble(CIInputSynFrame* p_frame)
{
    register int i = 0;
    register int j = 0;

    assert(NULL != p_frame);

    p_frame->type = ST_INPUT;
    
    p_frame->time_stamp = CI_GetTimeStamp();
    p_frame->cycle_counter = CICycleInt_GetCounter();

    /*同步当前按下的button信息*/
    CICommand_GetButton(&p_frame->button1,
        &p_frame->button2,
        &p_frame->button3,
        &p_frame->button4,
        &p_frame->button5);

    p_frame->eeu_channel_a_ok = CIEeu_IsChannelAOk();
    p_frame->eeu_channel_b_ok = CIEeu_IsChannelBOk();           /*电子单元通信状态*/
    p_frame->hmi_channel_a_ok = CIHmi_IsChannelAOk();
    p_frame->hmi_channel_b_ok = CIHmi_IsChannelBOk();           /*控显机通信状态*/

    p_frame->b_switch = CISwitch_CouldSwitch();

    p_frame->wait_switches_count = wait_switches_count;
    p_frame->switching_count = switching_count;

    /*
    * 装配命令控制信息，这里同步的过程当中仍然同步了一部分无用数据，使用稀疏矩阵
    * 可避免该问题
    */
    for (i = 0; i < MAX_GETWAYS; i++)
    {
        for (j = 0; j < USING_EEU_PER_LAYER; j++)
        {
            if (CI_TRUE == CIEeu_IsAddressUsing(i, j))
            {
                p_frame->commands[i][j] = CI_CompressCommandState(commands[i][j]);
            }
            else
            {
                p_frame->commands[i][j] = 0;
            }
        }
    }

    /*装配信号节点表信息*/
    for (i = 0; i < MAX_SIGNAL_NODES; i++)
    {
        p_frame->node[i].expect_state = CI_CompressSntState(signal_nodes[i].expect_state);
        p_frame->node[i].state = CI_CompressSntState(signal_nodes[i].state);
        p_frame->node[i].state_machine = signal_nodes[i].state_machine;
        p_frame->node[i].history_state = CI_CompressSntState(signal_nodes[i].history_state);
        p_frame->node[i].locked_flag = signal_nodes[i].locked_flag;
        p_frame->node[i].belong_route = signal_nodes[i].belong_route;
        p_frame->node[i].time_count = signal_nodes[i].time_count;
        p_frame->node[i].used = signal_nodes[i].used;
    }

    /*装配进路表信息*/
    for (i = 0; i < MAX_ROUTE; i++)
    {
        p_frame->route[i].ILT_index = routes[i].ILT_index;
        p_frame->route[i].state = routes[i].state;
        p_frame->route[i].state_machine = routes[i].state_machine;
        p_frame->route[i].backward_route = routes[i].backward_route;
        p_frame->route[i].forward_route = routes[i].forward_route;
        p_frame->route[i].current_cycle = routes[i].current_cycle;
        p_frame->route[i].error_count = routes[i].error_count;
        p_frame->route[i].other_flag = routes[i].other_flag;
        p_frame->route[i].approach_unlock = routes[i].approach_unlock;
    }

    /*装配正在转换道岔数据*/
    for (i = 0; i < MAX_WAIT_SWITCH; i++)
    {
        p_frame->wait_switches[i].switch_index = wait_switches[i].switch_index;
        p_frame->wait_switches[i].need_location = wait_switches[i].need_location;
        p_frame->wait_switches[i].timer_counter = wait_switches[i].timer_counter;
    }

    /*装配正在转换转换道岔数据*/
    for (i = 0; i < MAX_CONCURRENCY_SWITCH; i++)
    {
        p_frame->switching[i].switch_index = switching[i].switch_index;
        p_frame->switching[i].need_location = switching[i].need_location;
        p_frame->switching[i].timer_counter = switching[i].timer_counter;
    }

    /*装配清除区段数据*/
    for (i = 0; i < MAX_CLEARING_SECTION; i++)
    {
        p_frame->clear_section[i].section = clear_sections[i].section;
        p_frame->clear_section[i].start_time = clear_sections[i].start_time;
    }

    /*拷贝半自动闭塞数据*/
    for (i = 0; i < MAX_SEMI_AUTO_BLOCK; i++)
    {        
        p_frame->block[i].state = semi_auto_block_config[i].state;
        p_frame->block[i].block_state = semi_auto_block_config[i].block_state;
		p_frame->block[i].send_cycle_count = semi_auto_block_config[i].send_cycle_count;
		p_frame->block[i].ZD_recv_cycle_count = semi_auto_block_config[i].ZD_recv_cycle_count;
		p_frame->block[i].FD_recv_cycle_count = semi_auto_block_config[i].FD_recv_cycle_count;
    }

	/*装配挤岔报警数据*/
	for (i = 0; i < MAX_SWITCH_JCBJ; i++)
	{
		p_frame->switch_alarm[i].switch_index = switch_alarm[i].switch_index;
		p_frame->switch_alarm[i].timer_counter = switch_alarm[i].timer_counter;
	}

	/*装配按钮封锁数据*/
	for (i = 0; i < MAX_BUTTON_LOCKED; i++)
	{
		//p_frame->button_locked[i] = button_locked[i];
	}

	/*装配宝钢付原料站65#道口数据*/
	p_frame->dk_bgfylz65_config.send_cycle_count = dk_bgfylz65_config.send_cycle_count;

    /*计算crc*/
    CIInputSyn_CalcHash(p_frame);

    return 0;
}

/*
功能描述    : 比较双系输入数据是否一致
返回值      : 相等则返回0，不相等则返回-1
参数        : 无
作者        : 何境泰
日期        : 2014年5月13日 9:29:14
*/
int32_t CIinputSyn_Compare(CIInputSynFrame self_input_frame, CIInputSynFrame peer_input_frame)
{
    int i = 0, j = 0;

    /*比较按钮信息*/
    if (self_input_frame.button1 != peer_input_frame.button1
        || self_input_frame.button2 != peer_input_frame.button2
        || self_input_frame.button3 != peer_input_frame.button3
        || self_input_frame.button4 != peer_input_frame.button4
        || self_input_frame.button5 != peer_input_frame.button5
        )
    {
        CIRemoteLog_Write("input_cmp_pressed_button_fail:%d-%d-%d-%d-%d,%d-%d-%d-%d-%d",
            self_input_frame.button1,
            self_input_frame.button2,
            self_input_frame.button3,
            self_input_frame.button4,
            self_input_frame.button5,

            peer_input_frame.button1,
            peer_input_frame.button2,
            peer_input_frame.button3,
            peer_input_frame.button4,
            peer_input_frame.button5);

        return -1;
    }

#define SERIES_INPUT_CMP(field)                                             \
    if (self_input_frame.field != peer_input_frame.field) {     \
        CIRemoteLog_Write("input_cmp_"#field"_fail:%d!=%d",        \
                self_input_frame.field,peer_input_frame.field); \
        return -1;                                                          \
            }

    SERIES_INPUT_CMP(switching_count);
    SERIES_INPUT_CMP(wait_switches_count);

#undef SERIES_INPUT_CMP

    /*命令数据*/
    for (i = 0; i < MAX_GETWAYS; i++)
    {
        for (j = 0; j < USING_EEU_PER_LAYER; j++)
        {
            if (self_input_frame.commands[i][j] != peer_input_frame.commands[i][j])
            {
                CIRemoteLog_Write("input_cmp_commands_fail:%d_%d:%#x!=%#x",
                    i, j, self_input_frame.commands[i][j], peer_input_frame.commands[i][j]);
                return -1;
            }
        }
    }

#define SERIES_INPUT_ARRAY_CMP(arr,index,field)                                                     \
    if (self_input_frame.arr[index].field != peer_input_frame.arr[index].field) {                   \
        CIRemoteLog_Write("input_cmp_"#arr"_fail:%d:"#field":%#x!=%#x",                      \
            index,self_input_frame.arr[index].field,peer_input_frame.arr[index].field);             \
        return -1;                                                                                  \
            }

    /*比较信号节点表数据*/
    for (i = 0; i < MAX_SIGNAL_NODES; i++)
    {
        SERIES_INPUT_ARRAY_CMP(node, i, expect_state);
        SERIES_INPUT_ARRAY_CMP(node, i, state);
        SERIES_INPUT_ARRAY_CMP(node, i, history_state);
        SERIES_INPUT_ARRAY_CMP(node, i, locked_flag);
        SERIES_INPUT_ARRAY_CMP(node, i, belong_route);
        SERIES_INPUT_ARRAY_CMP(node, i, time_count);
        SERIES_INPUT_ARRAY_CMP(node, i, used);
    }
	/*比较进路表*/
    for (i = 0; i < MAX_ROUTE; i++)
    {
        SERIES_INPUT_ARRAY_CMP(route, i, ILT_index);
        SERIES_INPUT_ARRAY_CMP(route, i, state);
        SERIES_INPUT_ARRAY_CMP(route, i, backward_route);
        SERIES_INPUT_ARRAY_CMP(route, i, forward_route);
        SERIES_INPUT_ARRAY_CMP(route, i, other_flag);
        SERIES_INPUT_ARRAY_CMP(route, i, approach_unlock);
    }

    /*比较等待转换道岔*/
    for (i = 0; i < MAX_WAIT_SWITCH; i++)
    {
        SERIES_INPUT_ARRAY_CMP(wait_switches, i, switch_index);
        SERIES_INPUT_ARRAY_CMP(wait_switches, i, need_location);
        SERIES_INPUT_ARRAY_CMP(wait_switches, i, timer_counter);
    }

    /*比较正在转换道岔*/
    for (i = 0; i < MAX_CONCURRENCY_SWITCH; i++)
    {
        SERIES_INPUT_ARRAY_CMP(switching, i, switch_index);
        SERIES_INPUT_ARRAY_CMP(switching, i, need_location);
        SERIES_INPUT_ARRAY_CMP(switching, i, timer_counter);
    }

    /*清除区段*/
    for (i = 0; i < MAX_CLEARING_SECTION; i++)
    {
        SERIES_INPUT_ARRAY_CMP(clear_section, i, section);
        SERIES_INPUT_ARRAY_CMP(clear_section, i, start_time);
    }

    /*比较半自动信息*/
    for (i = 0; i < MAX_SEMI_AUTO_BLOCK; i++)
    {        
        SERIES_INPUT_ARRAY_CMP(block, i, state);
        SERIES_INPUT_ARRAY_CMP(block, i, block_state);
		SERIES_INPUT_ARRAY_CMP(block, i, send_cycle_count);
		SERIES_INPUT_ARRAY_CMP(block, i, ZD_recv_cycle_count);
		SERIES_INPUT_ARRAY_CMP(block, i, FD_recv_cycle_count);
    }

	/*比较挤岔报警*/
	for (i = 0; i < MAX_SWITCH_JCBJ; i++)
	{
		SERIES_INPUT_ARRAY_CMP(switch_alarm, i, switch_index);
		SERIES_INPUT_ARRAY_CMP(switch_alarm, i, timer_counter);
	}

	/*比较按钮封锁*/
	for (i = 0; i < MAX_BUTTON_LOCKED; i++)
	{
		if (self_input_frame.button_locked[i] != peer_input_frame.button_locked[i])
		{
			CIRemoteLog_Write("input_cmp_button_locked_fail:%d_%d:%#x!=%#x",
				i, j, self_input_frame.button_locked[i], peer_input_frame.button_locked[i]);
			return -1;
		}		
	}

	/*比较宝钢付原料站65#道口*/
	if (self_input_frame.dk_bgfylz65_config.send_cycle_count != peer_input_frame.dk_bgfylz65_config.send_cycle_count)
	{
		CIRemoteLog_Write("input_cmp_dk_bgfylz65_fail:%d_%d:%#x!=%#x",
			i, j, self_input_frame.dk_bgfylz65_config.send_cycle_count, peer_input_frame.dk_bgfylz65_config.send_cycle_count);
		return -1;
	}

#undef SERIES_INPUT_CMP
#undef SERIES_INPUT_ARRAY_CMP

    return 0;
}

/*
功能描述    : 校核状态数据替换
返回值      : 成功为0，失败为-1
参数        :
作者        : 何境泰
日期        : 2014年5月12日 20:34:10
*/
int32_t CIinputSyn_Replace(const CIInputSynFrame* p_frame)
{
    register int i = 0;
    register int j = 0;

    assert(NULL != p_frame);

    wait_switches_count = p_frame->wait_switches_count;
    switching_count = p_frame->switching_count;

    CICommand_SetButton(p_frame->button1,
        p_frame->button2,
        p_frame->button3,
        p_frame->button4,
        p_frame->button5);

    /*替换命令数据*/
    for (i = 0; i < MAX_GETWAYS; i++)
    {
        for (j = 0; j < USING_EEU_PER_LAYER; j++)
        {
            commands[i][j] = CI_DecompressCommandState(p_frame->commands[i][j]);
        }
    }

    /*替换信号节点表信息*/
    for (i = 0; i < MAX_SIGNAL_NODES; i++)
    {
        signal_nodes[i].expect_state = CI_DecompressSntState(p_frame->node[i].expect_state);
        signal_nodes[i].state = CI_DecompressSntState(p_frame->node[i].state);
        signal_nodes[i].state_machine = p_frame->node[i].state_machine;
        signal_nodes[i].history_state = CI_DecompressSntState(p_frame->node[i].history_state);
        signal_nodes[i].locked_flag = p_frame->node[i].locked_flag;
        signal_nodes[i].belong_route = p_frame->node[i].belong_route;
        signal_nodes[i].time_count = p_frame->node[i].time_count;
        signal_nodes[i].used = p_frame->node[i].used;
    }

    /*替换进路表信息*/
    for (i = 0; i < MAX_ROUTE; i++)
    {
        routes[i].ILT_index = p_frame->route[i].ILT_index;
        routes[i].state = p_frame->route[i].state;
        routes[i].state_machine = p_frame->route[i].state_machine;
        routes[i].backward_route = p_frame->route[i].backward_route;
        routes[i].forward_route = p_frame->route[i].forward_route;
        routes[i].current_cycle = p_frame->route[i].current_cycle;
        routes[i].error_count = p_frame->route[i].error_count;
        routes[i].other_flag = p_frame->route[i].other_flag;
        routes[i].approach_unlock = p_frame->route[i].approach_unlock;
    }

    /*替换正在转换道岔数据*/
    for (i = 0; i < MAX_WAIT_SWITCH; i++)
    {
        wait_switches[i].switch_index = p_frame->wait_switches[i].switch_index;
        wait_switches[i].need_location = p_frame->wait_switches[i].need_location;
        wait_switches[i].timer_counter = p_frame->wait_switches[i].timer_counter;
    }

    /*替换正在转换转换道岔数据*/
    for (i = 0; i < MAX_CONCURRENCY_SWITCH; i++)
    {
        switching[i].switch_index = p_frame->switching[i].switch_index;
        switching[i].need_location = p_frame->switching[i].need_location;
        switching[i].timer_counter = p_frame->switching[i].timer_counter;
    }

    /*替换清除区段数据*/
    for (i = 0; i < MAX_CLEARING_SECTION; i++)
    {
        clear_sections[i].section = p_frame->clear_section[i].section;
        clear_sections[i].start_time = p_frame->clear_section[i].start_time;
    }

    /*替换半自动闭塞数据*/
    for (i = 0; i < MAX_SEMI_AUTO_BLOCK; i++)
    {        
        semi_auto_block_config[i].state = p_frame->block[i].state;
        semi_auto_block_config[i].block_state = p_frame->block[i].block_state;
		semi_auto_block_config[i].send_cycle_count = p_frame->block[i].send_cycle_count;
		semi_auto_block_config[i].ZD_recv_cycle_count = p_frame->block[i].ZD_recv_cycle_count;
		semi_auto_block_config[i].FD_recv_cycle_count = p_frame->block[i].FD_recv_cycle_count;
    }

	/*替换挤岔报警数据*/
	for (i = 0; i < MAX_SWITCH_JCBJ; i++)
	{
		switch_alarm[i].switch_index = p_frame->switch_alarm[i].switch_index;
		switch_alarm[i].timer_counter = p_frame->switch_alarm[i].timer_counter;
	}

	/*替换按钮封锁数据*/
	for (i = 0; i < MAX_BUTTON_LOCKED; i++)
	{
		//button_locked[i] = p_frame->button_locked[i];
	}

	/*替换宝钢付原料站65#道口数据*/
	dk_bgfylz65_config.send_cycle_count = p_frame->dk_bgfylz65_config.send_cycle_count;

    return 0;
}

#endif /*!LINUX_ENVRIONMENT*/
