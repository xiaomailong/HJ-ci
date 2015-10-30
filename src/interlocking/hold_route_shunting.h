/***************************************************************************************
Copyright (C), 2012,  Co.Hengjun, Ltd.
文件名:  guide_route.h
作者:    hjh
版本 :   1.0	
创建日期:2012/3/20
用途:    引导进路
历史修改记录:         
***************************************************************************************/
#ifndef HOLD_ROUTE_SHUNTING
#define HOLD_ROUTE_SHUNTING

#define MAX_CHANGE_ROUTE 15
#define MAX_ILT_PER_ROUTES 6

/****************************************************
函数名：   command_hold_route_shunting
功能描述： 非进路调车
返回值：   void
参数：     int16_t special_index
作者：	   hejh
日期：     2014/08/11
****************************************************/
void command_hold_route_shunting(int16_t special_index);

/****************************************************
函数名：   hold_route_shunting_fault
功能描述： 非进路调车故障恢复
返回值：   void
参数：     int16_t special_index
作者：	   hejh
日期：     2014/08/12
****************************************************/
void hold_route_shunting_fault(int16_t special_index);

/****************************************************
函数名：   hold_route_shunting
功能描述： 非进路调车
返回值：   void
参数：     void
作者：	   hejh
日期：     2014/08/11
****************************************************/
void hold_route_shunting(void);

/****************************************************
函数名：   hold_route_shunting_process
功能描述： 非进路调车执行流程
返回值：   void
参数：     int16_t special_index
作者：	   hejh
日期：     2014/08/12
****************************************************/
void hold_route_shunting_process(int16_t special_index);

/****************************************************
函数名：   search_hold_route
功能描述： 搜索非进路调车
返回值：   void
参数：     int16_t searched_routes[MAX_CHANGE_ROUTE][MAX_ILT_PER_ROUTES]
参数：     node_t start_node
参数：     node_t end_node
作者：	   hejh
日期：     2014/08/11
****************************************************/
void search_hold_route (int16_t searched_routes[MAX_CHANGE_ROUTE][MAX_ILT_PER_ROUTES],node_t start_node,node_t end_node);

/****************************************************
函数名：   get_next_node_index
功能描述： 获取下一个起始节点
返回值：   int16_t
参数：     int16_t ILT_index
作者：	   hejh
日期：     2014/08/11
****************************************************/
int16_t get_next_node_index(int16_t ILT_index);

/****************************************************
函数名：   is_dead_ILTroute
功能描述： 不存在后续进路
返回值：   CI_BOOL
参数：     int16_t ILT_index
作者：	   hejh
日期：     2014/08/11
****************************************************/
CI_BOOL is_dead_ILTroute( int16_t ILT_index ) ;

/****************************************************
函数名：   select_hold_route
功能描述： 选择合理的进路
返回值：   int16_t
参数：     int16_t ILTs[MAX_CHANGE_ROUTE][MAX_ILT_PER_ROUTES]
作者：	   hejh
日期：     2014/08/11
****************************************************/
int16_t select_hold_route(int16_t ILTs[MAX_CHANGE_ROUTE][MAX_ILT_PER_ROUTES]);

/****************************************************
函数名：   hold_route_check_ci_condition
功能描述： 非进路调车检查联锁条件
返回值：   CI_BOOL
参数：     route_t route_index
作者：	   hejh
日期：     2014/08/11
****************************************************/
CI_BOOL hold_route_check_ci_condition(route_t route_index);

/****************************************************
函数名：   hold_route_check_select_complete
功能描述： 非进路调车选排一致
返回值：   CI_BOOL
参数：     route_t route_index
作者：	   hejh
日期：     2014/08/11
****************************************************/
CI_BOOL hold_route_check_select_complete(route_t route_index);

/****************************************************
函数名：   hold_route_check_signal_condition
功能描述： 非进路调车信号条件检查
返回值：   CI_BOOL
参数：     route_t route_index
作者：	   hejh
日期：     2014/08/11
****************************************************/
CI_BOOL hold_route_check_signal_condition(route_t route_index);

/****************************************************
函数名：   hold_route_check_open_signal
功能描述： 非进路调车检查开放信号条件
返回值：   CI_BOOL
参数：     route_t route_index
作者：	   hejh
日期：     2014/08/11
****************************************************/
CI_BOOL hold_route_check_open_signal(route_t route_index);

/****************************************************
函数名：   hold_route_keep_signal
功能描述： 非进路调车信号保持
返回值：   CI_BOOL
参数：     route_t route_index
作者：	   hejh
日期：     2014/08/12
****************************************************/
CI_BOOL hold_route_keep_signal(route_t route_index);

/****************************************************
函数名：   hold_route_signal_closed
功能描述： 取消非进路调车时检查信号关闭
返回值：   CI_BOOL
作者：	   hejh
日期：     2014/08/12
****************************************************/
CI_BOOL hold_route_signal_closed();

/****************************************************
函数名：   hold_route_section_cleared
功能描述： 取消非进路调车时检查区段出清
返回值：   CI_BOOL
参数：     int16_t special_index
作者：	   hejh
日期：     2014/08/12
****************************************************/
CI_BOOL hold_route_section_cleared(int16_t special_index);

/****************************************************
函数名：   gs_hrs_state
功能描述： 获取非进路调车状态
返回值：   EN_hold_route_shunting_state
参数：     int16_t index
作者：	   hejh
日期：     2014/08/12
****************************************************/
EN_hold_route_shunting_state gs_hrs_state(int16_t index);

/****************************************************
函数名：   ss_hrs_state
功能描述： 设置非进路调车状态
返回值：   void
参数：     int16_t index
参数：     EN_hold_route_shunting_state state
作者：	   hejh
日期：     2014/08/12
****************************************************/
void ss_hrs_state(int16_t index,EN_hold_route_shunting_state state);

/****************************************************
函数名：   gs_hold_route_shunting_index
功能描述： 获取非进路调车特殊索引号
返回值：   int16_t
参数：     node_t node_index
作者：	   hejh
日期：     2014/08/11
****************************************************/
int16_t gs_hold_route_shunting_index(node_t node_index);

/****************************************************
函数名：   gs_hold_route_shunting_start_signal
功能描述： 获取非进路调车始端信号的索引号
返回值：   node_t
参数：     special_t special
作者：	   hejh
日期：     2014/08/11
****************************************************/
node_t gs_hold_route_shunting_start_signal(special_t special);

/****************************************************
函数名：   gs_hold_route_shunting_end_signal
功能描述： 获取非进路调车终端信号的索引号
返回值：   node_t
参数：     special_t special
作者：	   hejh
日期：     2014/08/11
****************************************************/
node_t gs_hold_route_shunting_end_signal(special_t special);

/****************************************************
函数名：   gs_hold_route_shunting_FJL
功能描述： 获取非进路调车FJL表示灯的索引号
返回值：   node_t
参数：     special_t special
作者：	   hejh
日期：     2014/08/11
****************************************************/
node_t gs_hold_route_shunting_FJL(special_t special);

/****************************************************
函数名：   gs_hold_route_shunting_FJLGZ
功能描述： 获取非进路调车FJLGZ表示灯的索引号
返回值：   node_t
参数：     special_t special
作者：	   hejh
日期：     2014/08/11
****************************************************/
node_t gs_hold_route_shunting_FJLGZ(special_t special);

/****************************************************
函数名：   gs_hold_route_shunting_section
功能描述： 获取非进路调车检查区段的索引号
返回值：   node_t
参数：     special_t special
参数：     uint8_t i
作者：	   hejh
日期：     2014/08/11
****************************************************/
node_t gs_hold_route_shunting_section(special_t special,uint8_t i);


#endif
