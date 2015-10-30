/***************************************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.
文件名:  auto_unlock.h
作者:    CY
版本 :   1.0	
创建日期:2011/12/2
用途:    自动解锁模块
历史修改记录:         
***************************************************************************************/
#ifndef AUTO_UNLOCK
#define AUTO_UNLOCK
#include "utility_function.h"
#include "error_process.h"

/****************************************************
函数名:    auto_unlock
功能描述:  自动解锁
返回值:    
参数:      route_index
作者  :    CY
日期  ：   2011/12/2
****************************************************/
void auto_unlock(route_t current_route);

/****************************************************
函数名:    unlock_section
功能描述:  解锁信号点
返回值:    
参数:      route_t current_route
参数:      node_t current_section
作者  :    CY
日期  ：   2011/12/26
****************************************************/
void unlock_section(route_t current_route,node_t current_section); 

/****************************************************
函数名:    command_succesive_route_unlock
功能描述:  坡道解锁过程（命令处理模块调用）
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/4/16
****************************************************/
void command_succesive_route_unlock( route_t route_index );

/****************************************************
函数名:    unlock_middle_switch
功能描述:  解锁中间道岔
返回值:    
参数:      route_t current_route
作者  :    CY
日期  ：   2012/8/6
****************************************************/
void unlock_middle_switch(route_t current_route);

/****************************************************
函数名:    is_section_failure
功能描述:  检查是否有轨道故障
返回值:    故障返回真，否则为假
参数:      route_t current_route
参数:      index_t node_ordinal
作者  :    CY
日期  ：   2012/3/5
****************************************************/
CI_BOOL is_section_failure(route_t current_route, index_t node_ordinal);
/****************************************************
函数名：   check_3points_condition
功能描述： 三点检查条件
返回值：   void
参数：     int16_t route_index
参数：     int16_t node_ordinal
作者：	   hejh
日期：     2013/04/17
****************************************************/
void check_3points_condition(route_t route_index,int16_t node_ordinal);

/****************************************************
函数名：   train_location_state_machine
功能描述： 列车位置状态机
返回值：   void
参数：     route_t route_index
作者：	   hejh
日期：     2014/04/17
****************************************************/
void train_location_state_machine(route_t route_index);

/****************************************************
函数名：   unlock_special_switch
功能描述： 解锁特殊防护道岔
返回值：   void
参数：     route_t current_route
作者：	   hejh
日期：     2014/04/28
****************************************************/
void unlock_special_switch(route_t current_route);

#endif
