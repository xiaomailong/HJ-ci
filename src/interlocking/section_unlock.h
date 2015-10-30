/***************************************************************************************
Copyright (C), 2012,  Co.Hengjun, Ltd.
文件名:  section_failure_unlock.h
作者:    hjh
版本 :   1.0	
创建日期:2012/3/27
用途:    区段故障解锁
历史修改记录:         
***************************************************************************************/
#ifndef SECTION_FAILURE_UNLOCK
#define SECTION_FAILURE_UNLOCK

#include "check_ci_condition.h"
#include "keep_signal.h"
#include "auto_unlock.h"

/****************************************************
函数名:    section_unlock
功能描述:  区段故障解锁
返回值:    
参数:      index

作者  :    hjh
日期  ：   2012/3/28
****************************************************/
void section_unlock( route_t route_index );

/****************************************************
函数名:    command_section_unlock
功能描述:  区段故障解锁（命令处理模块调用）
返回值:    
参数:      index

作者  :    hjh
日期  ：   2012/4/16
****************************************************/
void command_section_unlock( node_t index );

/****************************************************
函数名:    signal_opened_section_unlock
功能描述:  信号已开放时的执行过程
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/3/28
****************************************************/
void signal_opened_section_unlock( route_t route_index );

/****************************************************
函数名:    unlock_sections
功能描述:  解锁区段，并检查进路是否解锁
返回值:    
参数:      route_index
参数:      index

作者  :    hjh
日期  ：   2012/3/28
****************************************************/
void unlock_sections( route_t route_index,node_t index );

/****************************************************
函数名:    check_all_sections_unlock
功能描述:  检查所有区段均解锁
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/4/16
****************************************************/
CI_BOOL check_all_sections_unlock( route_t route_index );

/****************************************************
函数名:    section_unlock_relay
功能描述:  正在区段故障解锁延时
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/4/16
****************************************************/
void section_unlock_relay( route_t route_index );

/****************************************************
函数名:    check_approach_clear_process
功能描述:  检查接近区段空闲的执行过程
返回值:    
参数:      route_t route_index
参数:      node_t index
作者  :    hejh
日期  ：   2012/8/8
****************************************************/
void section_unlock_approach_process( route_t route_index ,node_t index );

/****************************************************
函数名：   section_unlock_successive
功能描述： 判断延续进路是否可以进行故障解锁
返回值：   CI_TURE：可以解锁
			CI_FALSE：不能解锁
参数：     route_t route_index
参数：     node_t index
作者：	   hejh
日期：     2014/03/07
****************************************************/
CI_BOOL section_unlock_successive( route_t route_index,node_t index );

/****************************************************
函数名：   crash_into_signal_section_unlock
功能描述： 进路状态为冒进信号时故障解锁处理
返回值：  无
参数：     route_index 进路索引号
参数：     node_t index 待解锁区段索引号
作者：	   LYC
日期：     2014/05/29
****************************************************/
void crash_into_signal_section_unlock( route_t route_index,node_t index );

#endif