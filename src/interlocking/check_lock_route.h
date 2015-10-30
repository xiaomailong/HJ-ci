/***************************************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.
文件名:  check_select_complete.c
作者:    CY
版本 :   1.0	
创建日期:2011/12/2
用途:    检查并锁闭进路
历史修改记录:         
***************************************************************************************/
#ifndef CHECK_SELECT_COMPLETE
#define CHECK_SELECT_COMPLETE
#include "utility_function.h"
#include "error_process.h"
#include "check_ci_condition.h"

/****************************************************
函数名:    check_lock_route
功能描述:  检查并锁闭进路
返回值:  
参数:      route_index

作者  :    LYC
日期  ：   2012/9/10
****************************************************/
void check_lock_route(route_t route_index);

/****************************************************
函数名:    app_sw_check_select_complete
功能描述:  选排一致性检查应用软件
返回值:  
参数:      route_index

作者  :    LYC
日期  ：   2012/9/10
****************************************************/
void app_sw_check_select_complete(route_t route_index);

/****************************************************
函数名:    check_select_complete
功能描述:  选排一致检查
返回值:    
参数:    
作者  :		LYC
日期  ：   2011/12/2
****************************************************/
void check_select_complete(route_t route_index);

/****************************************************
函数名:    app_sw_check_signal_condition
功能描述:  信号检查应用软件
返回值:  
参数:      route_index

作者  :    LYC
日期  ：   2012/9/11
****************************************************/
void app_sw_check_signal_condition(route_t route_index);

/****************************************************
函数名:    check_signal_condition
功能描述:  信号检查
返回值:    
参数:      route_index 进路索引号
作者  :    CY
日期  ：   2011/12/2
****************************************************/
void check_signal_condition(route_t route_index);

/****************************************************
函数名:    satisfy_signal_condition
功能描述:  检查进路是否满足信号检查条件
返回值:    
参数:      route_index
作者  :    CY
日期  ：   2011/12/27
****************************************************/
CI_BOOL satisfy_signal_condition(route_t route_index);

/****************************************************
函数名:    app_sw_lock_route
功能描述:  锁闭进路应用软件
返回值:  
参数:      route_index

作者  :    LYC
日期  ：   2012/9/11
****************************************************/
void app_sw_lock_route(route_t route_index);

/****************************************************
函数名:    lock_route
功能描述:  锁闭进路模块
返回值:    
参数:      route_index
作者  :    CY
日期  ：   2011/12/2
****************************************************/
void lock_route(route_t route_index);

/****************************************************
函数名:    check_switch_location
功能描述:  道岔位置一致性检查
返回值:    一致返回真，反之返回假
参数:      route_index
作者  :    WSP
日期  ：   2011/12/14
****************************************************/
CI_BOOL check_switch_location( route_t route_index);

/****************************************************
函数名:    check_protective_switch_location
功能描述:  防护道岔位置一致性检查
返回值:    一致返回真，反之返回假
参数:      route_index
作者  :    WSP
日期  ：   2011/12/14
****************************************************/
CI_BOOL check_protective_switch_location(route_t route_index);
/****************************************************
函数名:    check_middle_switch_location
功能描述:  检查中间道岔位置一致
返回值:    
参数:      int16_t route_index
作者  :    hejh
日期  ：   2012/12/17
****************************************************/
CI_BOOL check_middle_switch_location(route_t route_index);

/****************************************************
函数名:    check_special_switch_location
功能描述:  检查特殊防护道岔位置正确
返回值:    
参数:      int16_t route_index
作者  :    hejh
日期  ：   2013/1/28
****************************************************/
CI_BOOL check_special_switch_location(route_t route_index);


/*调用函数声明**/
CI_BOOL find_mutuality_route_nodes(route_t route_index);		/*查看不在进路中的与进路相关节点的征用、锁闭标志*/
void set_route_lock_nodes(route_t route_index);		/*设置进路中所有节点的的锁闭标志**/
void set_mutuality_route_lock_nodes(route_t route_index);	/*设置不在进路中的与进路相关的节点锁闭标志*/

#endif
