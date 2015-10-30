/***************************************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.
文件名:  relay_cancel_route.h
作者:    WSP
版本 :   1.0	
创建日期:2011/12/14
用途:    人工延时解锁
历史修改记录:         
***************************************************************************************/
#ifndef RELAY_CANCEL_ROUTE
#define RELAY_CANCEL_ROUTE

#include "util/ci_header.h"
#include "utility_function.h"

/****************************************************
函数名:    relay_cancel_route
功能描述:  人工延迟解锁
返回值:    无
参数:      route_index

作者  :    LYC
日期  ：   2012/3/28
****************************************************/
void relay_cancel_route(route_t route_index);

/****************************************************
函数名:    command_relay_cancel_route
功能描述:  命令处理模块调用的人工解锁
返回值:  
参数:      route_index

作者  :    LYC
日期  ：   2012/4/16
****************************************************/
void command_relay_cancel_route(route_t route_index);

/****************************************************
函数名:    RCR_signal_opened_disposal
功能描述:  进路状态为信号开放的处理过程
返回值:  
参数:      route_index

作者  :    LYC
日期  ：   2012/3/28
****************************************************/
void RCR_signal_opened_disposal(route_t route_index);

/****************************************************
函数名:    delay_route_check_conditiion
功能描述:  进路延时过程
返回值:    
参数:      int16_t route_index
作者  :    hejh
日期  ：   2012/8/1
****************************************************/
void delay_route_check_conditiion(route_t route_index);

/********************************************************************
函数名:    RCR_singal_closing
功能描述:  进路状态为“人工延时取消进路模块正在关闭信号”时的处理过程
返回值:		无
参数:      route_index
作者  :    LYC
日期  ：   2012/3/28
********************************************************************/
void RCR_singal_closing(route_t route_index);

/****************************************************
函数名:    RCR_singal_closed
功能描述:  进路状态为“信号非正常关闭”时的处理过程
返回值:		无
参数:      route_index
作者  :    LYC
日期  ：   2012/3/28
****************************************************/
void RCR_singal_closed(route_t route_index);

/****************************************************
函数名:    after_singal_closed_disposal
功能描述:  信号关闭后处理过程
返回值:  
参数:      route_index
作者  :    LYC
日期  ：   2012/3/28
****************************************************/
void after_singal_closed_disposal(route_t route_index);

/****************************************************
函数名:    guide_route_check_conditiion
功能描述:  引导进路条件检查
返回值:  CI_TRUE:引导进路信号检查成功
		 CI_FALSE：引导进路信号检查失败
参数:      route_index
作者  :    LYC
日期  ：   2012/3/29
****************************************************/
CI_BOOL guide_route_check_conditiion(route_t route_index);

/****************************************************
函数名:    RCR_route_disposal
功能描述:  人工延时解锁过程
返回值:  
参数:      route_index
作者  :    LYC
日期  ：   2012/3/29
****************************************************/
void RCR_route_disposal(route_t route_index);

/****************************************************
函数名:    RCR_G_singal_closing
功能描述:  进路状态为“人工解锁模块正在关闭引导信号”时的处理过程
返回值:    
参数:      int16_t route_index
作者  :    hejh
日期  ：   2012/6/21
****************************************************/
void RCR_G_singal_closing(route_t route_index);

/****************************************************
函数名:    check_relay_cancel_route_condition
功能描述:  检查进路是否满足人工延时解锁检查条件
返回值:    
参数:      route_index
作者  :    CY
日期  ：   2011/12/27
****************************************************/
CI_BOOL check_relay_cancel_route_condition( route_t route_index);

/****************************************************
函数名：   guide_route_unlock
功能描述： 引导进路解锁
返回值：   void
参数：     route_t route_index
作者：	   hejh
日期：     2014/04/18
****************************************************/
void guide_route_unlock(route_t route_index);

#endif