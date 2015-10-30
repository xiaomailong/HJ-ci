/***************************************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.
文件名:  global_data.h
作者:    CY
版本 :   1.0	
创建日期:2011/11/29
用途:    定义全局变量
历史修改记录:         
***************************************************************************************/

#ifndef GLOBAL_DATA
#define GLOBAL_DATA

#include "util/ci_header.h"
#include "base_type_definition.h"
#include "data_struct_definition.h"

CIAPI_DATA(int16_t) pressed_button;             /*当前按下的按钮*/
CIAPI_DATA(int16_t) first_button;               /*当前按下的第一个按钮*/
CIAPI_DATA(int16_t) second_button;              /*当前按下的第二个按钮*/
CIAPI_DATA(int16_t) third_button;               /*当前按下的第三个按钮*/
CIAPI_DATA(int16_t) fourth_button;              /*当前按下的第四个按钮*/
CIAPI_DATA(int16_t) fifth_button;              /*当前按下的第五个按钮*/
CIAPI_DATA(CI_BOOL) can_build_route;            /*构成进路建立命令，开始建立进路*/
CIAPI_DATA(CI_BOOL) passing_route;              /*是否为通过进路*/
CIAPI_DATA(int16_t) wait_switches_count;        /*待转换道岔数*/
CIAPI_DATA(int16_t) switching_count;            /*正在转换道岔数*/

CIAPI_DATA(int16_t) total_buttons;				/*按钮实际总数*/
CIAPI_DATA(int16_t) total_signal_nodes;		/*信号节点实际总数*/
CIAPI_DATA(int16_t) total_ILTs;				/*联锁表进路实际总数*/
CIAPI_DATA(int16_t) total_getways;				/*网关实际总数 */
CIAPI_DATA(int16_t) total_auto_block;				/*自动闭塞实际数量 */
CIAPI_DATA(int16_t) total_auto_block3;				/*三显示自动闭塞实际数量 */
CIAPI_DATA(int16_t) total_semi_auto_block;				/*半自动闭塞实际数量 */
CIAPI_DATA(int16_t) total_change_run_dir;				/*改方运行实际数量 */
CIAPI_DATA(int16_t) total_throat_guide_lock;				/*引总锁闭实际数量 */
CIAPI_DATA(int16_t) total_special_switch;				/*特殊防护道岔实际数量 */
CIAPI_DATA(int16_t) total_switch_indication;				/*同意动岔实际数量 */
CIAPI_DATA(int16_t) total_highspeed_switch;			/*提速道岔实际数量 */
CIAPI_DATA(int16_t) total_successive_route;				/*延续进路实际数量 */
CIAPI_DATA(int16_t) total_hump;						/*驼峰实际数量 */
CIAPI_DATA(int16_t) total_hold_shunting_route;				/*非进路调车实际数量 */
CIAPI_DATA(int16_t) total_middle_switch;				/*中间道岔实际数量 */
CIAPI_DATA(int16_t) total_request_agree;				/*请求同意联系数量 */
CIAPI_DATA(int16_t) total_yards_liaision;				/*场间联系数量 */
CIAPI_DATA(int16_t) total_state_collect;				/*状态采集数量 */
CIAPI_DATA(int16_t) total_indication_lamp;				/*表示灯数量 */
CIAPI_DATA(int16_t) total_safe_line_switch;				/*安全线道岔实际数量 */
CIAPI_DATA(int16_t) total_location_reverse;				/*道岔位置相反实际数量 */
CIAPI_DATA(int16_t) total_delay_30seconds;				/*列车信号延时30s实际数量 */
CIAPI_DATA(int16_t) total_red_filament;				/*检查红灯断丝实际数量 */
CIAPI_DATA(int16_t) time_jcbj;				/*挤岔报警时间 */
CIAPI_DATA(int16_t) total_special_input;				/*特殊输入数量 */
CIAPI_DATA(int16_t) total_high_cross;				/*道口数量 */

CIAPI_DATA(char_t) device_name[TOTAL_NAMES][ITEM_NAME_LENGTH];        /*设备名称存放数组*/
CIAPI_DATA(ST_button_desc) buttons[MAX_BUTTONS];                    /*按钮所属的信号节点*/
CIAPI_DATA(ST_signal_node) signal_nodes[MAX_SIGNAL_NODES];           /*信号节点表*/
CIAPI_DATA(ST_ILT_item) ILT[MAX_ILTS];                               /*联锁表*/
CIAPI_DATA(ST_route) routes[MAX_ROUTE];                               /*进路表*/
CIAPI_DATA(uint32_t) commands[MAX_GETWAYS][MAX_EEU_PER_LAYER];       /*控制命令数据*/
CIAPI_DATA(ST_cleared_section) clear_sections[MAX_CLEARING_SECTION];  /*出清后正在延时的区段*/
CIAPI_DATA(ST_switching) wait_switches[MAX_WAIT_SWITCH];              /*待转换道岔表*/
CIAPI_DATA(ST_switching) switching[MAX_CONCURRENCY_SWITCH];           /*正在转换道岔表*/
CIAPI_DATA(int16_t) input_address[MAX_GETWAYS][MAX_EEU_PER_LAYER][MAX_NODE_PEEU]; /*输入地址映射区*/
CIAPI_DATA(ST_switch_alarm) switch_alarm[MAX_SWITCH_JCBJ];           /*挤岔报警*/
CIAPI_DATA(uint16_t) button_locked[MAX_BUTTON_LOCKED];           /*按钮封锁*/

CIAPI_DATA(successive_t) successive_route_config[MAX_SUCCESSIVE_ROUTE];  /*延续进路相关配置*/
CIAPI_DATA(semi_auto_block_t) semi_auto_block_config[MAX_SEMI_AUTO_BLOCK]; /*半自动闭塞配置*/
CIAPI_DATA(auto_block_t) auto_block_config[MAX_AUTO_BLOCK];             /*自动闭塞配置*/
CIAPI_DATA(auto_block3_t) auto_block3_config[MAX_AUTO_BLOCK3];			 /*三显示自动给闭塞配置*/
CIAPI_DATA(change_dir_run_t) change_run_dir_config[MAX_CHANGE_RUN_DIR]; /*改方运行配置*/
CIAPI_DATA(middle_switch_t) middle_switch_config[MAX_MIDLLE_SWITCH];  /*中间道岔配置*/
CIAPI_DATA(request_agree) request_agree_config[MAX_REQUEST_AGREE];/*请求同意配置*/
CIAPI_DATA(yards_liaision) yards_liaision_config[MAX_YARDS_LIAISION];/*场间联系配置*/
CIAPI_DATA(state_collect_t) state_collect_config[MAX_STATE_COLLECT];/*状态采集配置*/
CIAPI_DATA(special_switch_t) special_switch_config[MAX_SPECIAL_SWITCH];/*特殊防护道岔*/
CIAPI_DATA(safeline_switch_t) safeline_switch_config[MAX_SAFE_LINE_SWITCHS];/*安全线道岔*/
CIAPI_DATA(highspeed_switch_t) highspeed_switch_config[MAX_HIGH_SPEED_SWITCHS];/*提速道岔*/
CIAPI_DATA(delay_30seconds_t) delay_30seconds_config[MAX_DELAY_30SECONDS];/*列车信号延时30s*/
CIAPI_DATA(red_filament_t) red_filament_config[MAX_RED_FILAMENT];/*检查红灯断丝*/
CIAPI_DATA(location_reverse_t) location_reverse_config[MAX_LOCATION_REVERSE];/*道岔位置相反*/
CIAPI_DATA(indication_lamp) indication_lamp_config[MAX_INDICATION_LAMP];/*表示灯配置*/
CIAPI_DATA(agree_opreater_switch_t) agree_opreater_switch_config[MAX_SWITCH_INDICATION_LAMP];/*同意动岔表示灯配置*/
CIAPI_DATA(hold_route_shunting_t) hold_route_shunting_config[MAX_HOLD_SHUNGTING_ROUTE];/*非进路调车*/
CIAPI_DATA(other_special_t) other_special_config;/*其他特殊配置*/
CIAPI_DATA(throat_guide_lock) throat_guide_lock_config[MAX_THROAT_GUIDE_LOCK];/*引总锁闭*/
CIAPI_DATA(string) function_button_name[];

CIAPI_DATA(dk_t) dk_config[MAX_HIGH_CROSS];/*普通道口*/
CIAPI_DATA(dk_bgfylz65_t) dk_bgfylz65_config;/*宝钢付原料站65#道口*/
CIAPI_DATA(special_input_t) special_input_config[MAX_SPECIAL_INPUT];/*特殊输入配置*/

#endif
