/***************************************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.
文件名:  route_control.c
作者:    CY
版本 :   1.0	
创建日期:2011/11/30
用途:    进路控制模块
历史修改记录:         
2014/03/03 V1.2.1 hjh mantis:3433
	调整route_control.c中道岔控制模块的位置，使其在进路的相关操作前面。
***************************************************************************************/
#include "route_control.h"
#include "process_command.h"
#include "check_lock_route.h"
#include "guide_route.h"
#include "section_unlock.h"
#include "cancel_route.h"
#include "relay_cancel_route.h"
#include "open_signal.h"
/****************************************************
函数名:    route_control
功能描述:  进路控制
返回值:    
参数:      
作者  :    CY
日期  ：   2011/12/2
****************************************************/
void route_control(void)
{
	route_t i = 0;
	EN_route_state rs = RS_ERROR;

	FUNCTION_IN;
	/*建立进路*/
	search_route();	
	/*道岔控制模块*/
	switch_control();
	/*引总解锁时关闭没有进路的引导信号*/
	throat_unlock_guide();
	/*进路处理*/
	for (i = 0;i < MAX_ROUTE;i++)
	{
		/*参数检查*/
		if (IsFALSE(is_route_exist(i)))
		{
			continue;
		}
		/*进路处理流程控制*/
		app_sw_check_ci_condition(i);
		check_lock_route(i);
		open_signal(i);		
		keep_signal(i);
		auto_unlock(i);
		section_unlock(i);
		cancel_route(i);
		relay_cancel_route(i);
		repeated_open_signal(i);
		guide_route(i);
		unlock_middle_switch(i);
		unlock_special_switch(i);	
		train_location_state_machine(i);
	}
	/*已解锁进路的处理*/
	for (i = 0; i < MAX_ROUTE; i++)
	{
		if (IsFALSE(is_route_exist(i)))
			continue;
		rs = gr_state(i);
		/*参数检查*/
		if (IsTRUE(is_route_exist(i)) 
			&& (RS_CCIC_OK == rs ||
			RS_SELECTED == rs ||
			RS_FAILURE_TO_BUILD == rs))
		{
			sr_state(i,RS_UNLOCKED);	
			delete_route(i);
		}
	}
	/*道岔控制模块*/
	//switch_control();

	FUNCTION_OUT;
}
