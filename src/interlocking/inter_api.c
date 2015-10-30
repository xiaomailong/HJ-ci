/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年10月17日 20:28:04
用途        : 该文件的目的是对联锁运算层进行封装，力求不再引用对方的全局数据 
历史修改记录: v1.0    创建
**********************************************************************/

#include "inter_api.h"
#include "global_data.h"
#include "switch_control.h"
#include "process_command.h"
#include "route_control.h"
#include "keep_signal.h"
#include "init_manage.h"
#include "semi_auto_block.h"
#include "hold_route_shunting.h"
#include "special_interlocking.h"

#include "util/config.h"
#include "util/log.h"

extern CI_BOOL read_SNT(const char_t* StationName);
extern CI_BOOL read_ILT(const char_t* StationName);
extern CI_BOOL read_CFG(const char_t* StationName);

/*
 功能描述    : 设置节点状态
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2013年12月17日 9:30:47
*/
void CINode_SetState(node_t node_index, uint32_t status)
{
    sn_state(node_index, status);
}
/*
 功能描述    : 获取节点数据
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2013年12月17日 9:45:38
*/
uint16_t CINode_GetData(node_t node_index)
{
    return gn_data(node_index);
}

int32_t CIInter_ClearSection(void)
{
    clear_section();
    return 0;
}

/*
 功能描述    : 判断是否为道岔
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年6月27日 11:13:34
*/
CI_BOOL CIInter_IsSwitch(int16_t node_index)
{
    return is_switch(node_index);
}
/*
 功能描述    : 判断是否为道岔
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年6月27日 11:13:34
*/
CI_BOOL CIInter_IsSignal(int16_t node_index)
{
    return is_signal(node_index);
}

CI_BOOL CIInter_IsSection(int16_t node_index)
{
    return is_section(node_index);
}
void CIInter_InputNodeState(uint8_t ga,uint8_t ea,uint32_t state)
{
    input_node_state(ga, ea, state);

    return;
}
/*
功能描述     : 故障—安全处理
返回值       : 
参数         : 
作者         : 何境泰
日期         : 2014年7月22日 9:53:46
*/
void CIInter_DefaultSafeProcess(uint16_t ga,uint16_t ea)
{
    default_safe_process(ga,ea);

    return;
}
/*
 功能描述    : 进行联锁运算
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2013年12月20日 13:42:40
*/
int32_t CIInter_Calculate(void)
{
    process_command();
    route_control();
    /*非进路调车*/
    hold_route_shunting();
    semi_auto_block_process();
	sepecial_interlocking();
    abnormal_signal_close();
    send_node_timer();

    return 0;
}

/*
 功能描述    : 联锁运算层的初始化
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2013年12月30日 15:01:54
*/
int32_t CIInter_Init(void)
{
    static CI_BOOL b_initialized = CI_FALSE;
    int16_t i = 0,j;
    const char* p_station_name = CIConfig_GetValue("StationName");

    if (CI_TRUE == b_initialized)
    {
        CILog_Msg("联锁运算层已初始化");
        return -1;
    }

    if (NULL == p_station_name)
    {
        CILog_Msg("未能从配置文件文件当中找到StationName");
        return -1;
    }

    for (i=0; i< TOTAL_BUTTONS; i++)   /*初始化按钮表*/
    {
        buttons[i].node_index = NO_INDEX;
		buttons[i].button_index = NO_INDEX;
        buttons[i].button_type = BT_ERROR;
    }

    memset(signal_nodes,0xFF,sizeof(signal_nodes));/*初始化信号节点表*/
    memset(ILT,0xFF,sizeof(ILT));/*初始化联锁表*/

	for (i = 0; i < MAX_SIGNAL_NODES; i++)
	{
		signal_nodes[i].input_address = 0;
		signal_nodes[i].output_address = 0;
	}
	

    for(i=0; i < MAX_ROUTE; i ++)/*初始化进路表*/
    {
        routes[i].ILT_index = NO_INDEX;
        routes[i].forward_route = NO_INDEX;
        routes[i].backward_route = NO_INDEX;
        routes[i].state = RS_ERROR;
        routes[i].other_flag = ROF_ERROR;
    }

    /*输入地址映射区等*/
    memset(input_address,0xff,sizeof(input_address));
	memset(buttons,0xff,sizeof(buttons));
	memset(device_name,0xff,sizeof(device_name));
	memset(commands,0x00,sizeof(commands));
    memset(clear_sections,0xff,sizeof(clear_sections));
	memset(wait_switches,0xff,sizeof(wait_switches));
	memset(switching,0xff,sizeof(switching));
	memset(switch_alarm,0xff,sizeof(switch_alarm));

	/*特殊联锁*/
	memset(auto_block_config,0xFF,sizeof(auto_block_config));
	memset(auto_block3_config,0xFF,sizeof(auto_block3_config));
	memset(semi_auto_block_config,0xFF,sizeof(semi_auto_block_config));
	for (i = 0; i < MAX_SEMI_AUTO_BLOCK;i++)
	{
		semi_auto_block_config[i].state = SAB_NORMAL;
		semi_auto_block_config[i].block_state = SAS_NORMAL;
		semi_auto_block_config[i].send_cycle_count = NO_TIMER;
		semi_auto_block_config[i].ZD_recv_cycle_count = NO_TIMER;
		semi_auto_block_config[i].FD_recv_cycle_count = NO_TIMER;
	}	
	memset(change_run_dir_config,0xFF,sizeof(change_run_dir_config));
	memset(successive_route_config,0xFF,sizeof(successive_route_config));
	for (i = 0; i < MAX_SUCCESSIVE_ROUTE;i++)
	{
		successive_route_config[i].SuccessiveEndCount = 0;
	}
	memset(middle_switch_config,0xFF,sizeof(middle_switch_config));
	for (i = 0; i < MAX_MIDLLE_SWITCH;i++)
	{
		middle_switch_config[i].SwitchCount = 0;
		middle_switch_config[i].SectionCount = 0;
	}	
	memset(hold_route_shunting_config,0xFF,sizeof(hold_route_shunting_config));
	memset(special_switch_config,0xFF,sizeof(special_switch_config));
	memset(agree_opreater_switch_config,0xFF,sizeof(agree_opreater_switch_config));
	memset(highspeed_switch_config,0xFF,sizeof(highspeed_switch_config));
	memset(safeline_switch_config,0xFF,sizeof(safeline_switch_config));
	memset(location_reverse_config,0xFF,sizeof(location_reverse_config));
	memset(delay_30seconds_config,0xFF,sizeof(delay_30seconds_config));
	memset(red_filament_config,0xFF,sizeof(red_filament_config));
	memset(request_agree_config,0xFF,sizeof(request_agree_config));
	memset(yards_liaision_config,0xFF,sizeof(yards_liaision_config));
	memset(state_collect_config,0xFF,sizeof(state_collect_config));
	memset(indication_lamp_config,0xFF,sizeof(indication_lamp_config));
	memset(button_locked,0x00,sizeof(button_locked));

	for (i = 0; i < MAX_HIGH_CROSS; i++)
	{
		dk_config[i].AlarmIndex = NO_INDEX;
		dk_config[i].SignalIndex = NO_INDEX;
		dk_config[i].RouteType = RT_ERROR;
		dk_config[i].StartSection = NO_INDEX;
		dk_config[i].StopSection = NO_INDEX;
		for (j = 0; j < 10; j++)
		{
			dk_config[i].Switchs[j].SwitchIndex = NO_INDEX;
			dk_config[i].Switchs[j].Location = SWS_ERROR;
		}
		dk_config[i].SwitchsCount = 0;
		dk_config[i].AlarmState = CI_FALSE;
	}
	
	/*初始化宝钢付原料站65#道口*/
	dk_bgfylz65_config.section1 = NO_INDEX;
	dk_bgfylz65_config.section2 = NO_INDEX;
	dk_bgfylz65_config.QQD = NO_INDEX;
	dk_bgfylz65_config.TYD = NO_INDEX;
	dk_bgfylz65_config.XD = NO_INDEX;
	dk_bgfylz65_config.SQD = NO_INDEX;
	dk_bgfylz65_config.TYA = NO_INDEX;
	dk_bgfylz65_config.SQA = NO_INDEX;
	dk_bgfylz65_config.send_cycle_count = 0;
	/*初始化宝钢成品库站53#道口*/
	dk_bgcpkz53_config.AlarmIndex = NO_INDEX;
	dk_bgcpkz53_config.AlarmState = 0;
	dk_bgcpkz53_config.SignalIndex = NO_INDEX;
	dk_bgcpkz53_config.SignalState = 0;
	dk_bgcpkz53_config.Section1Index = NO_INDEX;
	dk_bgcpkz53_config.Section1State = 0;
	dk_bgcpkz53_config.Section2Index = NO_INDEX;
	dk_bgcpkz53_config.Section2State = 0;

	for (i = 0; i < MAX_SPECIAL_INPUT; i++)
	{
		special_input_config[i].InputNodeIndex = NO_INDEX;
		special_input_config[i].InputNodeState = 0;
		special_input_config[i].OutputNodeIndex = NO_INDEX;
		special_input_config[i].OutputNodeState = 0;
		special_output_config[i].InputNodeIndex = NO_INDEX;
		special_output_config[i].InputNodeState = 0;
		special_output_config[i].OutputNodeIndex = NO_INDEX;
		special_output_config[i].OutputNodeState = 0;
	}
	for (i = 0; i < MAX_SIGNAL_SHOW_CHANGE; i++)
	{
		signal_show_change_config[i].StartSignal = NO_INDEX;
		signal_show_change_config[i].EndSignal = NO_INDEX;
		signal_show_change_config[i].OldShow = SGS_ERROR;
		signal_show_change_config[i].NewShow = SGS_ERROR;
	}
	for (i = 0; i < MAX_SIGNAL_DELAY_OPEN; i++)
	{
		signal_delay_open_config[i].SignalIndex = NO_INDEX;
		signal_delay_open_config[i].TimerCounter = NO_TIMER;
	}
	for (i = 0; i < MAX_SECTION_COMPOSE; i++)
	{
		section_compose_config[i].Index0 = NO_INDEX;
		section_compose_config[i].State0 = 0;
		section_compose_config[i].Index1 = NO_INDEX;
		section_compose_config[i].State1 = 0;
		section_compose_config[i].Index2 = NO_INDEX;
		section_compose_config[i].State2= 0;
		section_compose_config[i].Index3 = NO_INDEX;
		section_compose_config[i].State3 = 0;
	}


    can_build_route = CI_FALSE;          /*构成进路建立命令，开始建立进路*/

	for (i = 0; i < MAX_ILTS; i++)
	{
		ILT[i].nodes_count = 0;
		ILT[i].switch_count = 0;
		ILT[i].signal_count = 0;
		ILT[i].track_count = 0;
	}

    if (CI_FALSE == read_SNT(p_station_name))
    {
        return -1;
    }
    if (CI_FALSE == read_ILT(p_station_name))
    {
        return -1;
    }
	if (CI_FALSE == read_CFG(p_station_name))
	{
		return -1;
	}
    if (-1 == ilt_manage_init())
    {
        return -1;
    }

    reset_signal_nodes();
    reset_routes();

    init_switch_control();
		
	/*初始化挤岔报警为正常状态*/
	for (i = 0; i < TOTAL_SIGNAL_NODE; i++)
	{
		if (gn_type(i) == NT_JCBJ)
		{
			sn_state(i,JCBJ_NORMAL);
			break;
		}
	}

	/*初始化引总锁闭为解锁状态*/
	for (i = 0; i < TOTAL_SIGNAL_NODE; i++)
	{
		if (NT_THROAT_GUIDE_LOCK == gn_type(i))
		{
			sn_state(i, SIO_NO_SIGNAL);
		}
	}

    b_initialized = CI_TRUE;

	/*配合测试的照查条件*/
	init_auto_test(p_station_name);

    return 0;
}
