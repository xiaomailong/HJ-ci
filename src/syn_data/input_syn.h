/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 何境泰
版本        : 1.0
创建日期    : 2013年10月29日 10:21:43
用途        : 双系输入数据同步
历史修改记录: v1.0    创建（目前和双cpu数据结构一致）
**********************************************************************/
#ifndef _input_syn_h__
#define _input_syn_h__
 
#include "util/ci_header.h"

#include "interlocking/base_type_definition.h"
#include "interlocking/data_struct_definition.h"

/*
 * 双系校核阶段输入同步数据
 */
typedef struct _InputSntFrame
{
    uint8_t expect_state;           /*预期状态*/
    uint8_t state;                  /*状态*/
    int8_t state_machine;           /*状态机*/
    uint8_t history_state;          /*历史状态*/
    uint32_t locked_flag;           /*锁闭标志*/
    int8_t belong_route;            /*节点所属进路*/
    int32_t time_count;             /*节点计时*/
    CI_BOOL used;                   /*征用标志*/
}InputSntFrame;

/*进路表*/
typedef ST_route InputRouteTable;

typedef ST_switching InputSwitching;
typedef ST_switch_alarm InputSwitchAlarm;
typedef uint16_t InputButtonLocked;

/*双系校核输入数据清除区段数据*/
typedef ST_cleared_section InputClearedSection;

/*
 * 双系输入半自动同步信息
 */
typedef struct _InputSemiAutoBlock
{
    uint16_t state;
    uint8_t block_state;
	CI_TIMER send_cycle_count;
	CI_TIMER ZD_recv_cycle_count;
	CI_TIMER FD_recv_cycle_count;
}InputSemiAutoBlock;

/*宝钢付原料站65#道口同步数据*/
typedef struct _InputHignCross_bgfylz65
{
	CI_TIMER send_cycle_count;
}InputHignCross_bgfylz65;

/*校核状态下的输入同步数据*/
typedef struct _CIInputSynFrame
{
    uint16_t type;                      /*数据类型*/
    uint16_t hash;                      /*hash值*/

    uint32_t time_stamp;                /*时间戳*/
    uint32_t cycle_counter;             /*联锁周期号*/

    int16_t button1;                    /*按钮1*/
    int16_t button2;                    /*按钮2*/
    int16_t button3;                    /*按钮3*/
    int16_t button4;                    /*按钮4*/
    int16_t button5;                    /*按钮5*/

    CI_BOOL eeu_channel_a_ok;    CI_BOOL eeu_channel_b_ok;           /*电子单元通信状态*/    CI_BOOL hmi_channel_a_ok;    CI_BOOL hmi_channel_b_ok;           /*控显机通信状态*/

    CI_BOOL b_switch;                    /*是否切换标志*/

    int16_t wait_switches_count;                                    /*待转换道岔数*/
    int16_t switching_count;                                        /*正在转换道岔数*/

    uint16_t commands[MAX_GETWAYS][USING_EEU_PER_LAYER];              /*控制命令数据*/

    InputSntFrame node[MAX_SIGNAL_NODES];               /*信号节点表动态数据*/
    InputRouteTable route[MAX_ROUTE];                    /*进路表*/
    InputSwitching wait_switches[MAX_WAIT_SWITCH];       /*待转换道岔表*/
    InputSwitching switching[MAX_CONCURRENCY_SWITCH];    /*正在转换道岔表*/
    InputClearedSection clear_section[MAX_CLEARING_SECTION];/*出清后延时区段*/
    InputSemiAutoBlock block[MAX_SEMI_AUTO_BLOCK];       /*半自动闭塞动态数据*/
	InputSwitchAlarm switch_alarm[MAX_SWITCH_JCBJ];      /*挤岔报警*/
	InputButtonLocked button_locked[MAX_BUTTON_LOCKED];  /*按钮封锁*/
	InputHignCross_bgfylz65 dk_bgfylz65_config;			 /*宝钢付原料站65#道口*/
}CIInputSynFrame;

/*
功能描述    : 打印输入同步数据，方便调试
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2013年10月29日 10:28:13
*/
CIAPI_FUNC(void) CIInputSyn_Print(CIInputSynFrame* p_frame);
/*
功能描述    : 为输入同步数据生成哈希值
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2013年10月29日 10:29:27
*/
CIAPI_FUNC(uint16_t) CIInputSyn_CalcHash(CIInputSynFrame* p_frame);
/*
功能描述    : 检测帧类型和数据校验码错误
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2014年5月12日 16:08:46
*/
CIAPI_FUNC(CI_BOOL) CIInputSyn_Verify(CIInputSynFrame* p_frame);
/*
功能描述    : 初始化输入同步数据
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2013年10月29日 10:27:52
*/
CIAPI_FUNC(int32_t) CIInputSyn_Init(CIInputSynFrame* p_frame);
/*
功能描述    : 输入同步数据装配
返回值      :
参数        :
作者        : 何境泰
日期        : 2013年10月29日 10:27:52
*/
CIAPI_FUNC(int32_t) CIInputSyn_Assemble(CIInputSynFrame* p_frame);
/*
功能描述    : 输入同步数据比较
返回值      :
参数        :
作者        : 何境泰
日期        : 2013年10月29日 10:27:52
*/
CIAPI_FUNC(int32_t)  CIinputSyn_Compare(CIInputSynFrame selfFrame, CIInputSynFrame peerFrame);
/*
功能描述    : 输入同步数据替换
返回值      :
参数        :
作者        : 何境泰
日期        : 2013年10月29日 10:27:52
*/
CIAPI_FUNC(int32_t) CIinputSyn_Replace(const CIInputSynFrame* p_frame);


#endif /*!_series_input_syn_h__*/
