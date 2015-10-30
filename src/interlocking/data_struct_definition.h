/***************************************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.
文件名:  data_struct_definition.$FILE_EXT&
作者:    CY
版本 :   1.0	
创建日期:2011/11/29
用途:    定义联锁软件用到的数据结构
历史修改记录:   
2013/1/28 V1.2.1 hjh
	增加特殊防护道岔的结构体
2013/3/7 V1.2.1 hjh
	信号节点表中direction的类型由uint8修改为EN_node_direction
2014/2/14 V1.2.1 hjh
	中间道岔结构体增加CI_BOOL allow_reverse;
***************************************************************************************/

#ifndef DATA_STRUCT_DEFINITION
#define DATA_STRUCT_DEFINITION

#if defined WIN32
#include <Windows.h>
#endif
#include "base_type_definition.h"

typedef struct   /*信号节点表*/
{
	uint8_t type;                   /*节点类型*/
	node_t previous_node_index;     /*前节点*/
	node_t next_node_index;         /*后节点*/
	node_t reverse_node_index;      /*岔后反位节点*/
	int16_t property;              /*节点属性*/
	uint32_t additional_property;   /*附加属性*/
	button_t buttons[2];            /*信号点按钮表*/
	uint16_t input_address;         /*输入地址*/
	uint16_t output_address;        /*输出地址*/	
	uint8_t throat;                 /*节点所在咽喉区*/
	EN_node_direction direction;    /*设备方向*/

	//#if defined WIN32
	//	SYSTEMTIME last_sc_time;        /*最后更新节点状态时间，用于确定电子单元是否正常*/
	//#else
	//	struct timeval last_sc_time;
	//#endif
	EN_signal_state expect_state;   /*预期状态*/	
	uint32_t state;                 /*状态*/
	CI_BOOL fault_flag;             /*故障状态*/
	EN_state_machine state_machine;  /*状态机*/
	uint32_t history_state;         /*历史状态*/	
	uint32_t locked_flag;           /*锁闭标志*/
	route_t belong_route;           /*节点所属进路*/
	CI_TIMER time_count;            /*节点计时*/
	EN_delay_time_type time_type;   /*节点计时类型*/
	CI_BOOL used;                   /*征用标志*/
	//CI_BOOL state_changed;          /*状态已改变*/	
}ST_signal_node;

typedef struct /*进路表*/
{
	ILT_t ILT_index;          /*本进路在联锁表中的索引号*/
	EN_route_state state;     /*进路状态*/
	EN_route_state_machine state_machine;  /*状态机*/
	route_t backward_route;   /*后一条进路*/
	route_t forward_route;    /*前一条进路*/
	CI_TIMER current_cycle;   /*当前进路处理周期*/
	route_t error_count;      /*该进路的错误计数*/
	EN_route_other_flag other_flag;   /*其他标志*/
	CI_BOOL approach_unlock;
}ST_route;

typedef struct  /*描述进路中的道岔*/
{
	node_t index;    /*道岔索引号*/
	uint16_t state;   /*高字节表示防护（0xAA）或带动（0x55）或者正常（0x00），低字节表示定位（0x55）或反位（0x00）*/
}ST_switch_description;

typedef struct  /*描述进路中的信号机*/
{
	node_t index; 
	uint8_t conflict_signal_type;          /*敌对信号类型，值为（ST_TRAIN,ST_SHUNTING）*/
	ST_switch_description  conditon_switch;   /*条件检查道岔*/
}ST_signal_description;

typedef struct  /*描述进路中的轨道*/
{
	node_t index; 
	ST_switch_description  conditon_switch;   /*条件检查道岔*/
}ST_track_description;

typedef struct  /*联锁表*/
{
	button_t start_button;                 /*始端按钮*/
	button_t change_button;                /*变更按钮*/
	button_t change_button1;               /*变更按钮1*/
	button_t end_button;                   /*终端按钮*/
	uint8_t route_kind;                    /*进路类型*/
	//uint8_t direction;                     /*进路方向*/
	//uint8_t route_method;                  /*进路方式*/
	//CI_BOOL straight_route;                /*是否直股*/	
	//uint32_t start_signal_show;            /*始端信号显示*/
	//node_t start_signal;                   /*始端信号*/
	node_t approach_section;               /*本进路的接近区段 */
	//node_t indicator;                      /*表示器*/
	//ST_switch_description resolve_switch1; /*决定进路方式的关键道岔1*/
	//ST_switch_description resolve_switch2; /*决定进路方式的关键道岔2*/ 
	node_t nodes_count;                    /*进路中实际的信号点数*/	
	int8_t switch_count;                   /*进路中的道岔数*/
	int8_t signal_count;                   /*进路的敌对信号机数*/
	int8_t track_count;                    /*进路中的轨道数*/
	node_t nodes[MAX_NODES_PS];            /*进路中的信号点*/
	ST_switch_description switches[MAX_SWITCH_PS];   /*进路中的道岔*/	
	ST_signal_description signals[MAX_SIGNALS_PS];   /*该进路的敌对信号*/	
	ST_track_description tracks[MAX_SECTIONS_PS];    /*进路中的轨道*/
	node_t face_train_signal;              /*迎面列车敌对，填写出站信号机*/
	node_t face_shunting_signal;           /*迎面调车敌对，填写调车信号机*/
}ST_ILT_item;

typedef struct /*按钮描述*/
{	
	node_t node_index;  /*按钮所属的信号节点*/
	node_t button_index;/*按钮索引号*/
	int8_t button_type;  /*按钮类型*/
}ST_button_desc;

typedef struct /*道岔控制相关数据*/
{
	node_t switch_index;          /*待转换道岔索引号*/
	EN_switch_state need_location; /*需要转换的位置*/
	CI_TIMER timer_counter;        /*道岔转换计时*/
}ST_switching;

typedef struct /*挤岔报警相关数据*/
{
	node_t switch_index;          /*无表示道岔索引号*/
	CI_TIMER timer_counter;        /*道岔挤岔计时*/
}ST_switch_alarm;

typedef struct/*特殊联锁定义*/
{
	char_t name[ITEM_NAME_LENGTH];
	EN_special_type type;      /*特殊联锁类型*/
	index_t special;           /*特殊联锁项索引号*/
}ST_special_interlocking;

typedef node_t request_agree; /*请求同意*/
typedef node_t yards_liaision; /*场间联系*/
typedef node_t indication_lamp; /*表示灯*/
typedef node_t throat_guide_lock;/*引总锁闭*/

typedef struct  /*同意动岔表示灯配置*/
{
	indication_lamp switch_indicatton_lamp; /*道岔位置表示灯*/
	ST_switch_description  conditon_switch_location;   /*条件检查道岔*/
}agree_opreater_switch_t;

typedef struct  /*非进路调车配置*/
{
	EN_hold_route_shunting_state state;
	node_t start_signal;
	node_t end_signal;
	indication_lamp FJL_indicatton_lamp;
	indication_lamp FJLGZ_indicatton_lamp;
	node_t check_sections[MAX_HOLD_ROUTE_SECTIONS];

}hold_route_shunting_t;

typedef struct /*状态采集*/
{
	node_t check_node;
	node_t section;
}state_collect_t;

typedef struct /*其他特殊配置*/
{
	node_t special_point[MAX_SPECIAL_POINT];
	//node_t delay_30seconds[MAX_DELAY_30SECONDS];
	//node_t check_red_filament[MAX_CHECK_RED_FILAMENT];
	//ST_switch_description safe_line_switchs[MAX_SAFE_LINE_SWITCHS];
	node_t unlock_sections[MAX_UNLOCK_SECTIONS];
	//node_t location_reverse[MAX_LOCATION_REVERSE];
	node_t switchs18[MAX_SWITCHS18];
	//node_t high_speed_switchs[MAX_HIGH_SPEED_SWITCHS];
}other_special_t; 

typedef struct  /*区段延时出清*/
{
	node_t section;
	CI_TIMER start_time;
}ST_cleared_section;


typedef struct  /*中间道岔*/
{
	node_t SignalIndex;
	uint8_t SwitchCount;
	node_t SwitchIndex[MAX_MIDDLE_SWITCHES];
	uint8_t SectionCount;
	node_t SectionIndex[MAX_MIDDLE_SECTIONS];
	CI_BOOL AllowReverseDepature;
}middle_switch_t;

typedef struct  /*延续进路*/
{
	node_t StartSignalIndex;
	node_t EndSignalIndex;
	uint8_t SuccessiveEndCount;
	node_t SuccessiveEnd[MAX_SUCCESSIVE_END];
}successive_t;

typedef struct  /*自动闭塞结构*/
{
	node_t EntrySignal;
	uint8_t SectionCount;
	node_t Sections[MAX_AUTO_BLOCK_SECTION];
}auto_block_t;

typedef struct  /*三显示自动闭塞结构*/
{
	node_t EntrySignal;   
	uint8_t SectionCount;
	node_t Sections[MAX_AUTO_BLOCK3_SECTION];
}auto_block3_t;

typedef struct  /*改方运行*/
{
	node_t EntrySignal;
	node_t CheckNode;
	uint8_t SectionCount;
	node_t Sections[MAX_CHANGE_RUN_DIR_SECTION];
}change_dir_run_t;

typedef struct  /*半自动闭塞*/
{
	node_t entry_signal;
	node_t mem_node;
	node_t block_button;
	node_t cancel_button;
	node_t failuer_button;
	node_t ZJ;   
	node_t FJ;
	route_t belong_route;
	EN_semi_auto_block_state state;
	EN_semi_auto_state block_state;
	CI_TIMER send_cycle_count;
	CI_TIMER ZD_recv_cycle_count;
	CI_TIMER FD_recv_cycle_count;
}semi_auto_block_t;

typedef struct /*特殊防护道岔*/
{
	node_t StartSignalIndex;/*始端信号索引号*/
	node_t EndSignalIndex;/*终端信号索引号*/
	node_t UnlockSectionIndex;	/*解锁区段索引号*/
	uint8_t SwitchCount;/*特殊防护道岔数量*/
	node_t SwitchIndex[MAX_SPECIAL_SWITCHES];/*特殊防护道岔索引号*/
	EN_switch_state SwitchLocation[MAX_SPECIAL_SWITCHES];/*特殊防护道岔位置*/
}special_switch_t;

typedef struct /*安全线相关数据*/
{
	node_t SwitchIndex;          /*道岔索引号*/
	EN_switch_state Location; /*需要转换的位置*/
	CI_TIMER SwitchingTimer;        /*道岔转换计时*/
}safeline_switch_t;

typedef struct /*提速道岔相关数据*/
{
	node_t SwitchIndex;          /*道岔索引号*/
}highspeed_switch_t;

typedef struct /*列车信号机延时30s解锁相关数据*/
{
	node_t SignalIndex;          /*信号机索引号*/
}delay_30seconds_t;

typedef struct /*列车信号检查红灯断丝相关数据*/
{
	node_t SignalIndex;          /*信号机索引号*/
}red_filament_t;

typedef struct /*道岔位置相反相关数据*/
{
	node_t SwitchIndex;          /*道岔索引号*/
}location_reverse_t;

typedef struct /*道口中道岔位置相关数据*/
{
	node_t SwitchIndex;          /*道岔索引号*/
	EN_switch_state Location; /*开通道口方向的位置*/
}switch_location_t;

typedef struct /*宝钢付原料站65#道口*/
{
	node_t section1;      /*道口区段1*/
	node_t section2;      /*道口区段2*/
	node_t QQD;           /*请求表示灯*/
	node_t TYD;           /*同意表示灯*/
	node_t XD;            /*公路信号灯*/
	node_t SQD;           /*收权表示灯*/
	node_t TYA;           /*同意按钮*/
	node_t SQA;           /*收权按钮*/
	CI_TIMER send_cycle_count;/*输出计时*/
}dk_bgfylz65_t;

typedef struct /*宝钢成品库站53#道口*/
{
	node_t AlarmIndex;      /*报警设备*/
	uint16_t AlarmState;      /*平常状态*/
	node_t SignalIndex;     /*检查信号机*/
	uint16_t SignalState;      /*信号机状态*/
	node_t Section1Index;   /*区段1*/
	uint16_t Section1State;   /*区段1状态*/
	node_t Section2Index;   /*区段2*/
	uint16_t Section2State;   /*区段2状态*/
}dk_bgcpkz53_t;

typedef struct /*普通道口*/
{
	node_t AlarmIndex;      /*报警设备*/
	node_t SignalIndex;     /*检查信号机*/
	EN_route_type RouteType;  /*进路类型*/
	node_t StartSection;   /*开始报警区段*/
	node_t StopSection;   /*停止报警区段*/
	switch_location_t Switchs[10];/*道岔*/
	uint8_t SwitchsCount;/*道岔计数*/
	CI_BOOL AlarmState;/*道口状态*/
}dk_t;

typedef struct /*特殊输入相关数据*/
{
	node_t InputNodeIndex;          /*输入索引号*/
	uint16_t InputNodeState; /*输入状态*/
	node_t OutputNodeIndex;          /*本站索引号*/
	uint16_t OutputNodeState; /*本站状态*/
}special_input_t;

typedef struct /*信号机显示变更相关数据*/
{
	node_t StartSignal;          /*始端信号机索引号*/
	node_t EndSignal;          /*终端信号机索引号*/
	EN_signal_state OldShow;        /*变更前显示*/
	EN_signal_state NewShow;        /*变更后显示*/
}signal_show_change_t;

typedef struct /*信号延时开放相关数据*/
{
	node_t SignalIndex;          /*信号机索引号*/
	CI_TIMER TimerCounter;        /*延时时间*/
}signa_delay_open_t;

typedef struct /*轨道合并相关数据*/
{
	node_t Index0;          /*索引号*/
	uint16_t State0;        /*状态*/
	node_t Index1;          /*索引号*/
	uint16_t State1;        /*状态*/
	node_t Index2;          /*索引号*/
	uint16_t State2;        /*状态*/
	node_t Index3;          /*索引号*/
	uint16_t State3;        /*状态*/
}section_compose_t;

#endif
