/***************************************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.
文件名:  check_ci_condition.h
作者:    CY
版本 :   1.0	
创建日期:2011/12/2
用途:    联锁条件检查模块
历史修改记录:         
***************************************************************************************/
#ifndef CHECK_CI_CONDITION
#define CHECK_CI_CONDITION

#include "util/ci_header.h"
#include "base_type_definition.h"

/****************************************************
函数名:    app_sw_check_ci_condition
功能描述:  联锁条件检查应用软件
返回值:    
参数:      route_t route_index
作者  :    hejh
日期  ：   2012/9/10
****************************************************/
void app_sw_check_ci_condition(route_t route_index);

/****************************************************
函数名:    check_ci_condition
功能描述:  联锁条件检查
返回值:    
参数:      route_index
作者  :    CY
日期  ：   2011/12/2
****************************************************/
void check_ci_condition(route_t route_index);

/****************************************************
函数名:    check_throat_shungting
功能描述:  跨咽喉调车条件检查
返回值:    
参数:      route_index
作者  :    WSP
日期  ：   2011/12/6
****************************************************/
CI_BOOL check_throat_shungting(route_t route_index);

/****************************************************
函数名:    set_used_flag
功能描述:  检查联锁关系条件时，检查并设置征用标志
返回值:    返回是否成功设置征用标志结果
参数:      route_index
作者  :    WSP
日期  ：   2011/12/6
****************************************************/
CI_BOOL judge_and_set_used_flag(route_t route_index);

/****************************************************
函数名:    set_used_flag
功能描述:  检查联锁关系条件时，检查锁闭标志
返回值:    锁闭返回真，反之返回假
参数:      route_index
作者  :    WSP
日期  ：   2011/12/6
****************************************************/
CI_BOOL is_any_node_route_locked(route_t route_index);

/****************************************************
函数名:    is_node_locked
功能描述:  检查联锁关系条件时，检查锁闭标志
返回值:    TRUE：进路上所有信号点已经被设置锁闭标志
		   FALSE：进路上有某个信号点未设置征用标志
参数:      route_index
作者  :    CY
日期  ：   2012/6/15
****************************************************/
CI_BOOL is_any_node_close_locked(route_t route_index);

/****************************************************
函数名:    is_node_locked
功能描述:  检查联锁关系条件时，检查锁闭标志
返回值:    TRUE：进路上所有信号点已经被设置锁闭标志
		   FALSE：进路上有某个信号点未设置征用标志
参数:      route_index
作者  :    CY
日期  ：   2012/6/15
****************************************************/
CI_BOOL is_any_node_single_locked(route_t route_index);

/****************************************************
函数名:    check_track_cleared
功能描述:  区段空闲检查
返回值:    
参数:      route_index
作者  :    WSP
日期  ：   2011/12/6
****************************************************/
CI_BOOL check_track_cleared(route_t route_index,CI_BOOL buildRouteFlag);

/****************************************************
函数名:    check_conflict_signal
功能描述:  敌对信号检查
返回值:    有敌对信号返回真，反之返回假
参数:      route_index
作者  :    WSP
日期  ：   2011/12/6
****************************************************/
CI_BOOL check_conflict_signal(route_t route_index);

/****************************************************
函数名:    check_face_conflict
功能描述:  迎面敌对检查
返回值:    敌对返回真，反之返回假
参数:      route_index
作者  :    WSP
日期  ：   2011/12/6
****************************************************/
CI_BOOL check_face_conflict(route_t route_index);

/****************************************************
函数名:    check_throat_locked
功能描述:  引总锁闭锁闭检查
返回值:    锁闭返回真，反之返回假
参数:      route_index
作者  :    WSP
日期  ：   2011/12/6
****************************************************/
CI_BOOL check_throat_locked(route_t route_index);

/****************************************************
函数名:    check_exceed_limit
功能描述:  检查侵限区段空闲；
	      若侵限区段占用，检查侵限区段道岔在规定位置上；
返回值:    侵限返回真，反之返回假
参数:      route_index
作者  :    WSP
日期  ：   2011/12/6
****************************************************/
CI_BOOL	check_exceed_limit(route_t route_index);

/****************************************************
函数名:    check_relation_condition
功能描述:  联系条件检查
返回值:    联系条件成立返回真，反之返回假
参数:      route_index
作者  :    WSP
日期  ：   2011/12/6
****************************************************/
CI_BOOL check_relation_condition (route_t route_index);

/****************************************************
函数名:    clear_ci_used_flag
功能描述:  清除联锁条件检查中设置的征用标志
返回值:    
参数:      route_index
作者  :    WSP
日期  ：   2011/12/14
****************************************************/
void clear_ci_used_flag(route_t route_index);


/****************************************************
函数名:    set_route_used_flag
功能描述:  设置进路中信号点的征用标志
返回值:    
参数:      route_index
作者  :    WSP
日期  ：   2011/12/22
****************************************************/
void set_route_used_flag(route_t route_index, CI_BOOL used_flag);

/****************************************************
函数名:    is_switch_can_drive
功能描述:  判断道岔是否具备转动条件
返回值:    能驱动返回真，反之返回假
参数:      node_index
作者  :    WSP
日期  ：   2011/12/22
****************************************************/
CI_BOOL is_switch_can_operate(int16_t node_index);

/****************************************************
函数名:    is_all_nodes_device_ok
功能描述:  检查进路中的信号点是否存在，是否设备故障
返回值:    正确返回真，反之返回假
参数:      route_index
作者  :    WSP
日期  ：   2011/12/22
****************************************************/
CI_BOOL is_all_nodes_device_ok(route_t route_index);

/****************************************************
函数名：   is_all_nodes_belong_route_ok
功能描述： 检查进路上的信号点所属进路正确
返回值：   CI_BOOL
参数：     route_t route_index
作者：	   hejh
日期：     2014/08/13
****************************************************/
CI_BOOL is_all_nodes_belong_route_ok(route_t route_index);

/****************************************************
函数名:    drive_switch
功能描述:  检查进路内道岔位置是否与进路的要求一致，不一致则将该道岔添加到道岔待转换队列
返回值:    
参数:      route_index
作者  :    WSP
日期  ：   2011/12/26
****************************************************/
void drive_switch(route_t route_index);

/****************************************************
函数名:    is_all_node_unlock
功能描述:  所有的信号点都未锁闭
返回值:    
参数:      int16_t route_index
作者  :    CY
日期  ：   2012/6/15
****************************************************/
CI_BOOL is_all_node_unlock(route_t route_index);

/****************************************************
函数名:    is_middle_switch_ok
功能描述:  判断中间道岔的条件是否满足
返回值:    
参数:      int16_t route_index
作者  :    hejh
日期  ：   2012/12/14
****************************************************/
CI_BOOL is_middle_switch_ok(route_t route_index);

/****************************************************
函数名:    is_special_switch_ok
功能描述:  检查特殊防护道岔位置正确
返回值:    
参数:      int16_t route_index
作者  :    hejh
日期  ：   2013/1/28
****************************************************/
CI_BOOL is_special_switch_ok(route_t route_index);

/****************************************************
函数名:    is_switch_ok
功能描述:  检查进路上的道岔都OK
返回值:    
参数:      int16_t route_index
作者  :    hejh
日期  ：   2012/12/14
****************************************************/
CI_BOOL is_switch_ok(route_t route_index);

/****************************************************
函数名：   check_successive_ci_condition
功能描述： 检查延续进路条件
返回值：   CI_BOOL
参数：     int16_t route_index
作者：	   hejh
日期：     2014/04/23
****************************************************/
CI_BOOL check_successive_ci_condition(route_t route_index);

/****************************************************
函数名：   is_route_signal_conflict
功能描述： 判断准备建立的进路与其他进路是否敌对
返回值：   CI_BOOL
参数：     route_t route_index
参数：     int16_t signal_ordinal
参数：     route_t current_route
作者：	   hejh
日期：     2015/08/14
****************************************************/
CI_BOOL is_route_signal_conflict(route_t route_index, int16_t signal_ordinal,route_t current_route);

#endif
