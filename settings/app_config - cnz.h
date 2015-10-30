/***************************************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.
文件名:  app_config.h
作者:    CY
版本 :   1.0	
创建日期:2011/11/29
用途:    标准站系统数据配置
历史修改记录:         
***************************************************************************************/
#ifndef _app_config_h__
#define _app_config_h__

#include "ci_header.h"

/*
 * 配置项目的路径可以在编译的时候指定
 */
#ifndef APP_PATH
#   define APP_PATH "."                       /*项目所在路径*/
#endif /*!APP_PATH*/
#ifdef WIN32
#   define PATH_SPERATOR "\\"
#   define SETTINGS_PATH APP_PATH "\\settings\\"      /*项目所在路径*/
#   define APP_CONFIG_FILENAME "platform.xml"         /*本项目所使用的配置文件名称*/
#else
#   define PATH_SPERATOR "/"
#   define SETTINGS_PATH APP_PATH "/settings/"        /*项目所在路径*/
#   define APP_CONFIG_FILENAME "platform_linux.xml"         /*本项目所使用的配置文件名称*/
#endif /*WIN32*/

#define APP_CONFIG_PATH (SETTINGS_PATH APP_CONFIG_FILENAME)   /*本项目所使用的配置文件路径*/
/*本系统使用的自定义信号，使用POSIX可靠信号集的第一个，默认为34*/
#define SIG_CI_TIMER SIGRTMIN

#define CI_CYCLE_MS 400				/*联锁周期，单位为毫秒*/
#define MAX_BUTTONS 30				/*站场中最大按钮数量*/
#define MAX_SIGNAL_NODES 50		/*站场中最大信号节点数量*/
#define MAX_ILTS 50				/*站场中最大联锁表数量*/
#define MAX_GETWAYS 31				/*站场中最大网关数量*/
#define MAX_EEU_PER_LAYER 16		/*每层最大电子单元数*/

#define MAX_CNZ_GETWAYS 6				/*站场中最大网关数量*/
#define MAX_CNZ_EEU_PER_LAYER 8		/*每层最大电子单元数*/

#define USING_EEU_PER_LAYER 8

#define MAX_CONCURRENCY_SWITCH 6	/*最大同时转岔数量*/
#define MAX_ROUTE 10				/*最大进路数*/
#define MAX_SWITCH_PS 30			/*进路中最大道岔数*/
#define MAX_SIGNALS_PS 30			/*进路中最大信号机数*/
#define MAX_SECTIONS_PS 30			/*进路中最大轨道区段数*/
#define MAX_NODES_PS 100			/*进路中最大信号点数*/
#define MAX_AUTO_BLOCK 2            /*站场中最大自动闭塞数量*/
#define MAX_AUTO_BLOCK3 1           /*站场中最大三显示自动闭塞数量*/
#define MAX_SEMI_AUTO_BLOCK 5       /*站场中最大半自动闭塞数量*/
#define MAX_CHANGE_RUN_DIR 2		/*站场中最大改方运行数量*/
#define MAX_THROAT_GUIDE_LOCK 2		/*站场中最大引总锁闭数量*/
#define MAX_SPECIAL_SWITCH 20		/*站场中最大特殊防护道岔数量*/
#define MAX_SWITCH_INDICATION_LAMP 5/*站场中最大同意动岔数量*/
#define MAX_SUCCESSIVE_ROUTE 5      /*站场中最大延续进路数量*/
#define MAX_HUMP 1                  /*站场中最大驼峰数量*/
#define MAX_HOLD_SHUNGTING_ROUTE 1  /*站场中最大非进路调车数量*/
#define MAX_MIDLLE_SWITCH 2			/*站场中最大中间道岔数量*/
#define MAX_REQUEST_AGREE 5         /*站场中最大请求同意联系数量*/
#define MAX_YARDS_LIAISION 10       /*站场中最大场间联系数量*/
#define MAX_STATE_COLLECT 5         /*站场中最大状态采集数量*/
#define MAX_INDICATION_LAMP 5       /*站场中最大表示灯数量*/
#define MAX_SWITCH_JCBJ	(MAX_SIGNAL_NODES / 2) /*站场中最大挤岔报警数量*/
#define MAX_SPECIAL_INPUT 10		/*站场中最大特殊输入数量*/
#define MAX_BUTTON_LOCKED (MAX_BUTTONS / 8 + 1)		/*站场中最大按钮封锁数量*/
#define MAX_SIGNAL_SHOW_CHANGE 10		/*站场中最大信号变更显示数量*/
#define MAX_SIGNAL_DELAY_OPEN 10		/*站场中最大信号延时开放数量*/
#define MAX_SECTION_COMPOSE 10		/*站场中最大区段合并数量*/
#define MAX_HIGH_CROSS 20		/*站场中最大道口数量*/

#define MAX_AUTO_BLOCK_SECTION 3     /*自动闭塞中接近区段的最大数量*/
#define MAX_AUTO_BLOCK3_SECTION 2    /*三显示自动闭塞中接近区段的最大数量*/
#define MAX_CHANGE_RUN_DIR_SECTION 3 /*改方运行中离去区段的最大数量*/
#define MAX_SPECIAL_SWITCHES 5       /*特殊防护道岔中道岔的最大数量*/
#define MAX_SUCCESSIVE_END  5        /*延续进路中终端按钮的最大数量*/
#define MAX_HOLD_ROUTE_SECTIONS 5	 /*非进路调车中检查区段的最大数量*/
#define MAX_MIDDLE_SWITCHES 5        /*中间道岔中道岔的最大数量*/
#define MAX_MIDDLE_SECTIONS 10       /*中间道岔中区段的最大数量*/
#define MAX_SPECIAL_POINT 2			 /*整个站场特殊点的最大数量*/
#define MAX_DELAY_30SECONDS 20		 /*列车信号延时30seconds的最大数量*/
#define MAX_RED_FILAMENT 30			 /*检查红灯断丝的最大数量*/
#define MAX_SAFE_LINE_SWITCHS 5      /*安全线道岔的最大数量*/
#define MAX_UNLOCK_SECTIONS 5		 /*不解锁区段的最大数量*/
#define MAX_LOCATION_REVERSE 5		 /*表示相反道岔的最大数量*/
#define MAX_SWITCHS18 5				 /*18#道岔的最大数量*/
#define MAX_HIGH_SPEED_SWITCHS 30	 /*提速道岔的最大数量*/

#define TOTAL_BUTTONS (total_buttons)           /*按钮实际总数*/
#define TOTAL_SIGNAL_NODE (total_signal_nodes)	/*信号节点实际总数*/
#define TOTAL_ILT (total_ILTs)					/*联锁表进路实际总数*/
#define TOTAL_GETWAY (total_getways + 1)			/*网关实际总数 */
#define TOTAL_AUTO_BLOCK (total_auto_block)            /*自动闭塞实际数量*/
#define TOTAL_AUTO_BLOCK3 (total_auto_block3)           /*三显示自动闭塞实际数量*/
#define TOTAL_SEMI_AUTO_BLOCK (total_semi_auto_block)       /*半自动闭塞实际数量*/
#define TOTAL_CHANGE_RUN_DIR (total_change_run_dir)		  /*改方运行实际数量*/
#define TOTAL_THROAT_GUIDE_LOCK (total_throat_guide_lock)	  /*引总锁闭实际数量*/
#define TOTAL_SPECIAL_SWITCH (total_special_switch)		  /*特殊防护道岔实际数量*/
#define TOTAL_SWITCH_INDICATION_LAMP (total_switch_indication) /*同意动岔实际数量*/
#define TOTAL_HIGH_SPEED_SWITCH (total_highspeed_switch)	/*提速动岔实际数量*/
#define TOTAL_SUCCESSIVE_ROUTE (total_successive_route)      /*延续进路实际数量*/
#define TOTAL_HUMP (total_hump)                 /*驼峰实际数量*/
#define TOTAL_HOLD_SHUNGTING_ROUTE (total_hold_shunting_route)  /*非进路调车实际数量*/
#define TOTAL_MIDDLLE_SWITCHES (total_middle_switch)       /*中间道岔实际数量*/
#define TOTAL_REQUEST_AGREE (total_request_agree)         /*请求同意联系数量*/
#define TOTAL_YARDS_LIAISION (total_yards_liaision)        /*场间联系数量*/
#define TOTAL_STATE_COLLECT (total_state_collect)         /*状态采集数量*/
#define TOTAL_INDICATION_LAMP (total_indication_lamp)       /*表示灯数量*/
#define TOTAL_SAFE_LINE_SWITCH (total_safe_line_switch)         /*安全线道岔实际数量 */
#define TOTAL_LOCATION_REVERSE (total_location_reverse)       /*道岔位置相反实际数量 */
#define TOTAL_DELAY_30SECONDS (total_delay_30seconds)         /*列车信号延时30s实际数量 */
#define TOTAL_RED_FILAMENT (total_red_filament)     /*检查红灯断丝实际数量 */
#define TOTAL_HIGH_CROSS (total_high_cross)     /*道口实际数量 */

#define TOTAL_NAMES (MAX_SIGNAL_NODES + MAX_BUTTONS)/*字符串总数,不需要改动*/

#define MAX_NODE_PEEU 4            /*每个电子单元中最大由几路信号点*/
#define MAX_ERROR_PER_COMMAND 5    /*每个了命令执行的最大周期数*/
#define MAX_CONCURRENCY_SWITCH   6                       /*最大同时转岔数量*/
#define MAX_WAIT_SWITCH 10            /*等待转岔队列长度*/
#define MAX_SWITCH_PER_SECTION 4                         /*同一个区段中最大的道岔数*/
#define MAX_SWITCH_TIME (13000 / CI_CYCLE_MS)            /*最大转岔时间*/
#define MAX_CLEARING_SECTION 10      /*进路中最大同时出清区段*/
#define MINUTES_3 (3*60*1000)        /*三分钟*/
#define SECONDS_3 (3*1000)           /*3秒*/
#define SECONDS_10 (10*1000)         /*10秒*/
#define SECONDS_15 (15*1000)		 /*15秒*/
#define SECONDS_30 (30*1000)         /*30秒*/
#define TIME_JCBJ (time_jcbj*1000)	/*挤岔报警时间 */
#define MAX_ROUTE_SELECT_TIME 40000u /*最大选路时间40秒*/
#define SEMI_AUTO_SEND_TIME (2000u / CI_CYCLE_MS) /*半自动信号最大发送时间*/
#define SEMI_AUTO_RECV_TIME (500u / CI_CYCLE_MS) /*半自动信号最大接收时间*/

#define TEST_NAME_LENGTH 60u       /*名字字符串长度*/
#define FILE_NAME_LENGTH 250u      /*文件名称长度*/
#define MAX_STRING_LENGTH 400      /*提示信息字符串最大长度*/ 
#define ITEM_NAME_LENGTH 20U       /*条目名称长度*/
#define BUFFER_SIZE 20480u         /*缓冲区长度*/
#define MAX_COMMUNICATE_BOARD 20u  /*连锁机最大通信板数量*/

#define FILE_PATH_SIZE              256     /*文件路径的长度*/
#define STATION_SETTING_NAME_SIZE   20      /*站场配置文件名长度*/
#define STATION_NAME_SIZE           10      /*站场名长度*/
#define DEVICE_NAME_LEN             5       /*设备名长度*/
#define MAX_COMM_PORT               15      /*最大端口数量*/
#define TOTAL_STATION               1       /*站场数*/

#define TRAIN_BUTTON_INDEX 0       /*列车按钮索引号*/
#define PASSING_BUTTON_INDEX 1     /*通过按钮索引号*/
#define SHUNTING_BUTTON_INDEX 1    /*调车按钮索引号*/

#define MAX_FRAME_SIZE 20480         /*最大帧长度*/
#define MAX_CLEW_INFORMATION 100    /*最大提示信息长度*/
#define MAX_NETWORK_STOP_TIME 1500  /*最大可接受网络中断时间（ms）*/

#define ROUTE_LOCKED_MASK 0xFF                /*正常锁闭掩码位*/
#define SWITCH_CLOSED_MASK 0xFF00             /*道岔封锁*/
#define SWITCH_SIGNLE_LOCKED_MASK 0x550000    /*道岔单锁*/
#define SWITCH_THROAT_LOCKED_MASK 0xFF000000  /*咽喉区锁闭掩码位*/
#define MIDDLE_SWITCH_LOCKED_MASK 0xAA0000	  /*中间道岔锁闭掩码位*/

/*******************************控制命令****************************************/
#define CMD_LED_OPEN  0x01u        /*led灯亮*/
#define CMD_LED_CLOSE 0x00u        /*led灯灭*/

#define NO_INDEX -1                 /*无索引号*/
#define NO_NODE -1                  /*没有节点*/
#define NO_ROUTE -1                 /*没有进路*/              
#define NO_TIMER 0xFFFFFFFFu        /*不需要定时器计时*/

#endif /*_app_config_h__*/