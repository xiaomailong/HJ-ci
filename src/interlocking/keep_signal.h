/***************************************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.
文件名:  keep_signal.h
作者:    CY
版本 :   1.0	
创建日期:2011/12/2
用途:    信号保持模块
历史修改记录:         
***************************************************************************************/
#ifndef KEEP_SIGNAL
#define KEEP_SIGNAL
#include "check_ci_condition.h"
#include "check_lock_route.h"
#include "semi_auto_block.h"

/****************************************************
函数名:    command_close_signal
功能描述:  关闭信号命令处理
返回值:    
参数:      node_t signal
作者  :    CY
日期  ：   2012/7/24
****************************************************/
void command_close_signal(node_t signal);

/****************************************************
函数名:    keep_signal
功能描述:  信号保持
返回值:    
参数:      int16_t route_index
作者  :    hejh
日期  ：   2012/7/23
****************************************************/
void keep_signal(route_t route_index);

/****************************************************
函数名:    keep_train_signal
功能描述:  列车信号保持模块
返回值:    
参数:      route_index
作者  :    CY
日期  ：   2011/12/2
****************************************************/
void keep_train_signal(route_t route_index);

/****************************************************
函数名:    kkep_shunting_signal
功能描述:  调车信号保持模块
返回值:    
参数:      route_index
作者  :    CY
日期  ：   2011/12/2
****************************************************/
void keep_shunting_signal(route_t route_index);

/****************************************************
函数名:    train_signal_opened
功能描述:  进路状态为信号已开放时的执行过程
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/3/5
****************************************************/
void train_signal_opened( route_t route_index );

/****************************************************
函数名:    signal_changing
功能描述:  信号灯光转换执行过程
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/3/5
****************************************************/
void signal_changing( route_t route_index );

/****************************************************
函数名:    judge_train_route_sections
功能描述:  对进路上区段的状态进行判断
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/3/5
****************************************************/
void judge_train_route_sections( route_t route_index) ;

/****************************************************
函数名:    judge_train_route_remain_sections
功能描述:  检查列车进路剩余区段空闲
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/3/8
****************************************************/
void judge_train_route_remain_sections( route_t route_index) ;

/****************************************************
函数名:    judge_shunting_route_sections
功能描述:  对调车进路上区段的状态进行判断
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/3/5
****************************************************/
void judge_shunting_route_sections( route_t route_index ) ;

/****************************************************
函数名:    judge_shunting_route_remain_sections
功能描述:  检查调车进路其他区段空闲
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/3/8
****************************************************/
void judge_shunting_route_remain_sections( route_t route_index ) ;

/****************************************************
函数名:    judge_interlocking_condition
功能描述:  联锁条件判断
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/2/24
****************************************************/
CI_BOOL judge_interlocking_condition( route_t route_index );

/****************************************************
函数名:    change_signal_color
功能描述:  信号机显示颜色转换
返回值:    
参数:      route_index
作者  :    hjh
日期  ：   2011/12/21
****************************************************/
EN_signal_state change_signal_color(route_t route_index);
/****************************************************
函数名:    auto_block_check
功能描述:  自动闭塞条件检查
返回值:    
参数:      special_index
作者  :    hjh
日期  ：   2011/12/27
****************************************************/
EN_signal_state auto_block_check(int16_t special_index);

/****************************************************
函数名:    semi_auto_block_check
功能描述:  三显示自动闭塞条件检查
返回值:    
参数:      special_index
作者  :    hjh
日期  ：   2011/12/27
****************************************************/
EN_signal_state auto_block3_check(int16_t special_index);

/****************************************************
函数名:    judge_shunting_interlocking_condition
功能描述:  检查调车联锁条件
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/3/5
****************************************************/
void judge_shunting_interlocking_condition( route_t route_index ) ;


/****************************************************
函数名:    entry_signal_color_definition
功能描述:  进站信号机信号颜色定义
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/3/6
****************************************************/
EN_signal_state entry_signal_color_definition( route_t route_index );

/****************************************************
函数名:    out_signal_color_definition
功能描述:  出站信号机信号颜色定义
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/3/6
****************************************************/
EN_signal_state out_signal_color_definition( route_t route_index );

/****************************************************
函数名:    check_node_route_locked
功能描述:  检查所有信号点均被进路锁闭
返回值:    均被锁闭返回真，否则返回假
参数:      route_index

作者  :    hjh
日期  ：   2012/2/27
****************************************************/
CI_BOOL check_node_route_locked(route_t route_index);

/****************************************************
函数名:    judge_route_section_failure
功能描述:  判断进路上区段是否故障
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/3/8
****************************************************/
CI_BOOL judge_route_section_failure(route_t route_index);

/****************************************************
函数名:    shunting_signal_opened
功能描述:  进路状态为信号已开放的执行过程
返回值:    
参数:      route_index

作者  :    hjh
日期  ：   2012/3/14
****************************************************/
void shunting_signal_opened(route_t route_index);

/****************************************************
函数名:    close_signal
功能描述:  关闭信号
返回值:    
参数:      node_t signal
作者  :    CY
日期  ：   2012/5/11
****************************************************/
void close_signal(route_t route_index);

/****************************************************
函数名:    close_signal
功能描述:  判断信号是否关闭
返回值:    
参数:      node_t signal
作者  :    CY
日期  ：   2012/5/11
****************************************************/
CI_BOOL is_signal_close(node_t signal_node);

/****************************************************
函数名:    abnormal_signal_close
功能描述:  关闭错误开放的信号机
返回值:    
作者  :    CY
日期  ：   2012/7/2
****************************************************/
void abnormal_signal_close();

/****************************************************
函数名:    check_signal_allow_filament
功能描述:  检查允许信号断丝
返回值:    
参数:      route_t route_index
作者  :    hejh
日期  ：   2012/8/9
****************************************************/
void check_signal_allow_filament(route_t route_index);

/****************************************************
函数名:    is_low_signal
功能描述:  信号是否降级
返回值:    TRUE:ss2比ss1级别低
参数:      EN_signal_state ss1
参数:      EN_signal_state ss2
作者  :    hejh
日期  ：   2012/8/10
****************************************************/
CI_BOOL is_low_signal(EN_signal_state ss1,EN_signal_state ss2);

/****************************************************
函数名:    has_signal_filamented
功能描述:  信号机是否断丝过
返回值:    
参数:      node_t index
作者  :    hejh
日期  ：   2012/8/10
****************************************************/
CI_BOOL has_signal_filamented(node_t index);

/****************************************************
函数名:    is_relation_condition_ok
功能描述:  判断联系条件是否成立
返回值:    TRUE:成立
			FALSE：不成立
参数:      route_t route_index
作者  :    hejh
日期  ：   2012/9/6
****************************************************/
CI_BOOL is_relation_condition_ok(route_t route_index);

/****************************************************
函数名：   signal_closing
功能描述： 关闭信号时的处理过程
返回值：   void
参数：     int16_t route_index
参数：     EN_route_state route_state
作者：	   hejh
日期：     2013/04/25
****************************************************/
void signal_closing( route_t route_index,EN_route_state route_state );

/****************************************************
函数名：   keep_signal_middle_switch_ok
功能描述： 检查中间道岔位置正确
返回值：   CI_BOOL
参数：     route_t route_index
作者：	   hejh
日期：     2014/06/11
****************************************************/
CI_BOOL keep_signal_middle_switch_ok(route_t route_index);

/****************************************************
函数名：   hmi_signal_tips
功能描述： 控显机信号机开放颜色
返回值：   char_t*
参数：     EN_signal_state ss
作者：	   hejh
日期：     2015/07/03
****************************************************/
char_t* hmi_signal_tips(EN_signal_state ss);

#endif
