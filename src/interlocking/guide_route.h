/***************************************************************************************
Copyright (C), 2012,  Co.Hengjun, Ltd.
文件名:  guide_route.h
作者:    hjh
版本 :   1.0	
创建日期:2012/3/20
用途:    引导进路
历史修改记录:         
***************************************************************************************/
#ifndef GUIDE_ROUTE
#define GUIDE_ROUTE

#include "utility_function.h"
#include "check_ci_condition.h"
#include "keep_signal.h"
#include "global_data.h"
#include "error_process.h"
/****************************************************
函数名:    guide_route
功能描述:  引导进路
返回值:    

作者  :    hjh
日期  ：   2012/3/20
****************************************************/
void guide_route( route_t route_index );

/****************************************************
函数名:    command_guide_route
功能描述:  引导进路（命令处理模块调用）
返回值:  
参数:      index

作者  :    LYC
日期  ：   2012/4/25
****************************************************/
void command_guide_route( node_t index );

/****************************************************
函数名：   is_guide_route_build
功能描述： 检查是否可以建立引导进路
返回值：   CI_BOOL
参数：     int16_t guide_route_index
参数：     int16_t route_index
作者：	   hejh
日期：     2014/04/18
****************************************************/
CI_BOOL is_guide_route_build( route_t guide_route_index,route_t route_index );

/****************************************************
函数名:    search_guide_route
功能描述:  搜索引导进路节点算法
返回值:    
参数:      index

作者  :    hjh
日期  ：   2012/3/20
****************************************************/
int16_t search_guide_route( int16_t guide_route_nodes[], int16_t start_signal_index );

/****************************************************
函数名:    set_guide_route
功能描述:  设置引导进路
返回值:    返回进路索引号
参数:      route_index

作者  :    hjh
日期  ：   2012/3/21
****************************************************/
route_t set_guide_route( int16_t route_index );

/****************************************************
函数名:    check_guide_route_condition1
功能描述:  检查引导进路联锁条件并开放信号
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/3/21
****************************************************/
CI_BOOL check_guide_route_condition( route_t route_index );

/****************************************************
函数名:    compare_nodes
功能描述:  找到引导进路对应的进路索引号
返回值:    找到返回金路索引号，否则返回-1
参数:      guide_nodes
参数:      num

作者  :    hjh
日期  ：   2012/3/21
****************************************************/
int16_t compare_nodes(int16_t guide_nodes[],int16_t num);

/****************************************************
函数名:    get_first_section
功能描述:  获取信号机内方第一个区段
返回值:    返回第一个区段索引号
参数:      index

作者  :    hjh
日期  ：   2012/3/22
****************************************************/
int16_t get_first_section( int16_t index );

/****************************************************
函数名:    guide_signal_opening_process
功能描述:  引导信号正在开放执行过程
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/3/22
****************************************************/
void guide_signal_opening_process( route_t route_index );

/****************************************************
函数名:    guide_signal_opened_process
功能描述:  引导信号已开放的执行过程
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/3/22
****************************************************/
void guide_signal_opened_process( route_t route_index );

/****************************************************
函数名:    guide_signal_closing_process
功能描述:  引导信号正在关闭的执行过程
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/3/22
****************************************************/
void guide_signal_closing_process( route_t route_index );

/****************************************************
函数名:    open_guide_signal
功能描述:  开放引导信号
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/3/28
****************************************************/
void open_guide_signal( route_t route_index );
/****************************************************
函数名:    command_throat_locked_process
功能描述:  进站信号机所在咽喉被引总锁闭时的执行过程(命令处理模块调用)
返回值:    
参数:      index

作者  :    hjh
日期  ：   2012/3/20
****************************************************/
void throat_locked_guide( int16_t index );

/****************************************************
函数名:    throat_unlock_guide
功能描述:  引总解锁时的执行过程
返回值:    
作者  :    hejh
日期  ：   2012/8/7
****************************************************/
void throat_unlock_guide(void);

/****************************************************
函数名：   guide_route_exist
功能描述： 引导进路存在执行流程
返回值：   void
参数：     route_t route_index
作者：	   hejh
日期：     2014/04/18
****************************************************/
void guide_route_exist( route_t route_index );

/****************************************************
函数名：   lock_guide_route
功能描述： 锁闭引导进路
返回值：   CI_BOOL
参数：     int16_t route_index
作者：	   hejh
日期：     2014/06/11
****************************************************/
CI_BOOL lock_guide_route( route_t route_index );



#endif
