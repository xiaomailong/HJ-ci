/***************************************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.
文件名:  cancel_route.h
作者:    WSP
版本 :   1.0	
创建日期:2011/12/14
用途:    取消进路
历史修改记录:         
***************************************************************************************/
#ifndef CANCEL_ROUTE
#define CANCEL_ROUTE

#include "util/ci_header.h"
#include "utility_function.h"
#include "error_process.h"

/****************************************************
函数名:    cancel_route
功能描述:  取消进路
返回值:    无
参数:      route_index

作者  :    LYC
日期  ：   2012/3/26
****************************************************/
void cancel_route(route_t route_index);

/****************************************************
函数名:    command_cancel_route
功能描述:  被命令处理模块调用的取消进路模块
返回值:  
参数:      route_index

作者  :    LYC
日期  ：   2012/4/16
****************************************************/
void command_cancel_route(route_t route_index);

/****************************************************
函数名:    signal_opened_disposal
功能描述:  进路状态为“信号已开放”时的处理
返回值:  
参数:      route_index
作者  :    LYC
日期  ：   2012/3/27
****************************************************/
void signal_opened_disposal(route_t route_index);

/****************************************************
函数名:    CR_signal_closoing_disposal
功能描述:  进路状态为“取消进路模块正在非正常关闭信号”时的处理
返回值:  
参数:      route_index
作者  :    LYC
日期  ：   2012/3/27
****************************************************/
void CR_signal_closoing_disposal(route_t route_index);

/****************************************************
函数名:    CR_A_signal_closed
功能描述:  进路状态为“信号非正常关闭”时的处理
返回值:  
参数:      route_index
作者  :    LYC
日期  ：   2012/3/28
****************************************************/
void CR_A_signal_closed(route_t route_index);

/****************************************************
函数名:    CR_check_condition
功能描述:  取消进路条件检查及处理
返回值:  
参数:      route_index
作者  :    LYC
日期  ：   2012/3/27
****************************************************/
void CR_check_condition(route_t route_index);


#endif
