/***************************************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.
文件名:  route_control.h
作者:    CY
版本 :   1.0	
创建日期:2011/11/30
用途:    进路控制模块
历史修改记录:         
***************************************************************************************/
#ifndef ROUTE_CONTROL
#define ROUTE_CONTROL

#include "search_route.h"
#include "check_ci_condition.h"
#include "check_lock_route.h"
#include "open_signal.h"
#include "keep_signal.h"
#include "auto_unlock.h"
#include "switch_control.h"

/****************************************************
函数名:    route_control
功能描述:  进路控制函数
返回值:    
参数:      
作者  :    CY
日期  ：   2011/12/2
****************************************************/
void route_control(void);

#endif
