/***************************************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.
文件名:  base_type_definition.$FILE_EXT&
作者:    CY
版本 :   1.0	
创建日期:2011/11/29
用途:    定义基本数据类型
历史修改记录:     
2013/1/28 V1.2.1 hjh
	增加特殊防护道岔的枚举量定义SPT_SPECIAL_SWTICH
***************************************************************************************/
#ifndef BASE_TYPE_DEFINITION
#define BASE_TYPE_DEFINITION

#include "util/ci_header.h"
#include "util/app_config.h"

typedef int16_t index_t;       /*索引号类型*/ 
typedef int16_t node_t;        /*信号点类型*/ 

typedef int8_t route_t;        /*进路类型*/ 
typedef int16_t ILT_t;         /*联锁表类型*/ 
typedef int16_t button_t;      /*按钮类型*/ 
typedef int16_t special_t;     /*特殊联锁类型*/ 

typedef uint32_t CI_TIMER;    /*周期计数器类型*/

typedef enum node_type  /*信号节点类型*/
{
	NT_NO,              /*无类型*/  
	NT_ENTRY_SIGNAL,                /*进站信号机*/
	NT_OUT_SIGNAL,                  /*出站信号机*/
	NT_OUT_SHUNTING_SIGNAL,         /*出站兼调车*/
	NT_ROUTE_SIGNAL,                /*进路信号机*/
	NT_SINGLE_SHUNTING_SIGNAL,      /*单置调车信号机*/
	NT_DIFF_SHUNTING_SIGNAL,        /*差置调车信号机*/
	NT_JUXTAPOSE_SHUNGTING_SIGNAL,  /*并置调车信号机*/
	NT_STUB_END_SHUNTING_SIGNAL,    /*尽头式调车信号机*/
	NT_LOCODEPOT_SHUNTING_SIGNAL,   /*机务段调车信号机*/
	NT_REVIEW_SIGNAL,               /*复视信号机*/
	NT_HUMPING_SIGNAL,              /*驼峰信号机*/
	NT_SIGNAL,            /*小于此，大于NT_NO的为信号机*/   

	NT_SWITCH,                      /*道岔*/

	NT_TRACK,                       /*股道*/
	NT_NON_SWITCH_SECTION,          /*无岔区段*/
	NT_SWITCH_SECTION,              /*道岔区段*/
	NT_STUB_END_SECTION,            /*尽头线*/
	NT_LOCODEPOT_SECTION,           /*机待线*/
	NT_APP_DEP_SECTION,            /*进站信号机外方的接近离去区段*/
	NT_SECTION,                     /*小于此大于等于NT_TRACK的为轨道区段*/

	NT_CHECK_POINT,                 /*照查点*/
	NT_REQUEST_AGREE_POINT,         /*请求同意点*/
	NT_CHANGE_BUTTON,               /*变更按钮点*/
	NT_TRAIN_END_BUTTON,			/*列车终端按钮*/
	NT_SUCCESSIVE_END_BUTTON,		/*延续进路终端按钮*/
	NT_SHUNTING_END_BUTTON,         /*调车终端按钮点*/
	NT_EXCEED_LIMIT,                /*侵限绝缘*/
	NT_SEMI_AUTOMATIC_BLOCK,        /*半自动闭塞*/
	NT_INDICATOR,                   /*表示器*/
	NT_THROAT_GUIDE_LOCK,           /*引总锁闭*/
	NT_ZQD,				            /*总启动*/
	NT_DSBJ,				        /*灯丝报警*/
	NT_JCBJ,				        /*挤岔报警*/
	NT_INPUT,				        /*输入*/
	NT_OUTPUT,				        /*输出*/
	NT_INPUTOUTPUT				    /*输入输出*/
}EN_node_type;

typedef enum lock_type     /*锁闭类型*/
{
	LT_UNLOCK = 0,                        /*未锁闭*/
	LT_LOCKED = 0x55,                     /*已锁闭，包括正常的进路锁闭*/
	LT_SWITCH_CLOSED = 0x5500,            /*道岔封锁*/
	LT_SWITCH_SIGNLE_LOCKED = 0x550000,   /*道岔单锁*/
	LT_SWITCH_THROAT_LOCKED = 0x55000000, /*咽喉区锁闭（引总锁闭）*/
	LT_MIDDLE_SWITCH_LOCKED = 0xAA0000    /*中间道岔锁闭*/

}EN_lock_type;

typedef enum node_direction /*信号点方向*/
{
	DIR_ERROR,         /*错误的方向*/
	DIR_UP,            /*上行*/
	DIR_DOWN,          /*下行*/
	DIR_UP_DOWN,       /*上下行*/
	DIR_RIGHT_UP,      /*右上*/
	DIR_LEFT_UP,       /*左上*/
	DIR_LEFT_DOWN,     /*左下*/
	DIR_RIGHT_DOWN     /*右下*/
}EN_node_direction;

typedef enum conflict_signal_type /*敌对信号类型*/
{
	SST_TRAIN,                /*列车敌对*/
	SST_SHUNTING,             /*调车敌对*/
	SST_TRAIN_SHUNTING		  /*列车和调车均敌对*/
}EN_conflict_signal_type;

typedef enum signal_state  /*信号机状态*/
{
	SGS_ERROR = 0x0000,          /*通信中断*/
	SGS_FILAMENT_BREAK = 0x55AA, /*断丝 */
	SGS_H   = 0x5A5A,			 /*红灯*/
	SGS_U   = 0x6969,            /*黄*/
	SGS_UU  = 0x6996,			 /*双黄*/
	SGS_LU  = 0x6699,            /*绿黄*/
	SGS_L   = 0x6666,            /*绿灯*/
	SGS_LL  = 0x5AA5,            /*双绿*/
	SGS_USU  = 0x9669,           /*黄闪黄*/
	SGS_B   = 0x9966,            /*白灯*/
	SGS_YB  = 0x9696,            /*引导白灯*/
	SGS_A   = 0x9999,             /*蓝灯*/
	SGS_LS   = 0xAAAA,             /*绿闪*/
	SGS_US   = 0xCCCC,             /*黄闪*/
	SGS_HS   = 0xA55A,             /*红闪*/
	SGS_BS   = 0xAA55             /*白闪*/
}EN_signal_state;

typedef enum switch_state  /*道岔状态*/
{
	SWS_ERROR = 0x0000,         /*通信中断*/
	SWS_NO_INDICATE = 0x3333,   /*无表示*/
	SWS_NORMAL = 0x33CC,		/*定位,定操*/
	SWS_REVERSE = 0xCC33		/*反位,反操*/
}EN_switch_state;


typedef enum section_state  /*轨道状态*/
{
	SCS_ERROR = 0x0000,         /*通信中断*/
	SCS_OCCUPIED = 0x3CC3,      /*占用*/
	SCS_CLEARED = 0x5555        /*空闲*/
}EN_section_state;

typedef enum state_machine  /*轨道状态机*/
{
	SCSM_FAULT_BEHIND = -4,		/*后方区段异常空闲*/
	SCSM_FAULT_FRONT = -3,		/*前方区段异常*/
	SCSM_FAULT_CLEARED = -2,	/*本区段异常空闲*/
	SCSM_FAULT = -1,			/*区段故障*/
	SCSM_INIT,					/*设备初始化*/	
	SCSM_BEHIND_OCCUPIED,       /*后方区段占用*/
	SCSM_SELF_OCCUPIED,         /*本区段占用*/
	SCSM_FRONT_OCCUPIED_1,      /*前方区段占用1*/
	SCSM_FRONT_OCCUPIED_2,      /*前方区段占用2*/
	SCSM_BEHIND_CLEARED_1,      /*后方区段空闲1*/
	SCSM_BEHIND_CLEARED_2,      /*后方区段空闲2*/
	SCSM_SELF_UNLOCK,		    /*本区段解锁*/
	SCSM_TRACK_UNLOCK,		    /*股道解锁*/
	SCSM_SELF_RETURN		    /*本区段折返*/
}EN_state_machine;

typedef enum delay_time_type     /*延时类型*/
{
	DTT_INIT,					 /*初始化*/
	DTT_UNLOCK,					 /*解锁延时*/
	DTT_CLOSE_GUIDE,			 /*关闭引导信号延时*/
	DTT_ERROR,					 /*错误计时*/
	DTT_OTHER  					 /*其他延时*/
}EN_delay_time_type;

typedef enum /*零散模块状态*/
{
	SIO_ERROR = 0x0000,         /*通信中断*/
	SIO_HAS_SIGNAL = 0xC33C,    /*零散模块有输入,有输出*/
	SIO_NO_SIGNAL = 0xC3C3,		/*零散模块没有输入,无输出*/

	/*SIO_NO,
	SIO_H,
	SIO_B,
	SIO_BS,
	SIO_L*/
}EN_IO_state;

typedef enum /*总启动状态*/
{
	SZQD_ERROR = 0x0000,         /*通信中断*/
	SZQD_HAS_SIGNAL = 0x3C3C,    /*总启动有输入,有输出*/
	SZQD_NO_SIGNAL = 0xA5A5,	 /*总启动没有输入,无输出*/
}EN_ZQD_state;

typedef enum semi_auto_block_state /*64D半自动闭塞状态*/
{
	SAB_ERROR = 0x0000,                /*设备错误*/
	SAB_NORMAL = 0xAB01,               /*设备正常*/  /*解除闭塞*/ 
	/********************************************************************/
	SAB_REQUEST_BLOCK = 0xAB02,        /*按压闭塞按钮/本站请求闭塞*/	
	SAB_SENT_REQUEST = 0xAB03,         /*请求闭塞信息已发送	*/
	SAB_RECIEVED_AUTO_REPLY = 0xAB04,  /*收到（—）/对方站已收到请求信息*/
	SAB_RECIEVED_AGREEMENT = 0xAB05,   /*收到（+）/已收到对方站同意接车信息*/
	SAB_OUT_SIGNAL_OPENED = 0xAB06,    /*出站信号机已开放/出站信号已开放*/	
	SAB_DEPARTED = 0xAB07,             /*出站信号机正常自动关闭/本站已发车*/	
	SAB_SENT_DEPARTED = 0xAB08,        /*已发送本站通知出发信息*/	
	SAB_RECIEVED_CANCEL = 0xAB09,      /*接收到解除闭塞信息/已收到解除闭塞信息*/	
	/********************************************************************/
	SAB_RECIEVED_REQUEST = 0xAB0A,     /*收到(+)/对方站请求闭塞*/
	SAB_AGREED_RECIEVE_TRAIN = 0xAB0B, /*按压闭塞按钮/本站同意接车*/
	SAB_SENDING_AGREEMENT = 0xAB0C,    /*发送（+）/同意接车信息正在发送*/
	SAB_SENT_AGREEMENT = 0xAB0D,       /*同意接车信息已发送*/
	SAB_RECIEVED_DEPARTED = 0xAB0E,    /*收到（+）/已收到对方站通知出发信息*/
	SAB_TRAIN_APPROACH = 0xAB0F,       /*列车到达接近区段/列车接近*/ /*进站信号机开放/接车信号已开放*/
	SAB_TRAIN_ARRIVING = 0xAB10,       /*列车进入进站信号机内方/列车已到达*/
	SAB_TRAIN_IN_ENTRY_SIGNAL = 0xAB11,/*列车完全进入进站信号机内方/接车进路内方第一个道岔区段解锁*/
	SAB_TRAIN_ARRIVED_TRACK = 0xAB12,  /*接车进路已解锁/列车到达股道*/
	//SAB_CANCEL_BLOCK = 0xAB13,         /*拔出闭塞按钮/取消闭塞手续*/
	//SAB_SENDING_CANCEL = 0xAB13,       /*发送（-）/正在发送解除闭塞信息*/
	//SAB_SENT_CANCEL = 0xAB14,          /*已发送解除闭塞信息*/
	
	SAB_RECOVER = 0xAB15,              /*按压复原或事故按钮/办理事故复原*/	
	SAB_RECIEVED_RECOVER = 0xAB16      /*已接收到（-），复原闭塞*/        
}EN_semi_auto_block_state;

typedef enum semi_auto_state  /*半自动状态*/
{
	SAS_NORMAL,      /*非闭塞状态*/
	SAS_DEPARTURE,   /*发车状态*/
	SAS_RECIEVING    /*接车状态*/
}EN_semi_auto_state;

typedef enum JCBJ_state /*挤岔报警状态*/
{
	JCBJ_NORMAL = 0xAC01,      /*正常状态*/
	JCBJ_ALARM = 0xAC02,		/*挤岔报警状态*/
	JCBJ_KNOW = 0xAC03,		  /*挤岔确认状态*/
	JCBJ_WARNING = 0xAC04      /*挤岔恢复提醒状态*/
}EN_JCBJ_state;

typedef enum request_agree_state /*请求同意状态*/
{
	RAS_ERROR,            /*设备错误*/
	RAS_NORMAL,           /*设备正常状态*/
	RAS_SENDING_REQUEST,  /*正在发送请求*/
	RAS_SENT_REQUEST,     /*已发送请求*/
	RAS_RECIEVING_REPLY,  /*正在接受应答*/
	RAS_RECIEVED_REPLY,   /*已接收到应答*/
	RAS_RECIEVING_REQUEST,/*已接收到请求*/
	RAS_RECIEVED_REQUEST, /*已接收到请求*/
	RAS_SENDING_REPLY,    /*正在发送应答*/
	RAS_SENT_REPLY        /*已发送应答*/
}EN_request_agree_state;


typedef enum check_state  /*照查状态*/
{
	CS_ERROR,   /*设备错误*/
	CS_YES,     /*照查条件满足*/
	CS_NO       /*照查条件不满足*/ 
}EN_check_state;


typedef enum route_type  /*进路类型*/
{
	RT_ERROR,          /*进路类型错误*/
	RT_TRAIN_ROUTE,    /*列车进路*/
	RT_SHUNTING_ROUTE, /*调车进路*/
	RT_SUCCESSIVE_ROUTE/*延续进路*/
}EN_route_type;


typedef enum  route_state  /*进路状态(route state 缩写为RS)*/
{            
	RS_ERROR,                    /*进路错误*/
	RS_SELECTED,                 /*进路已选出*/
	RS_CCIC_OK,                  /*联锁条件检查成功*/
	RS_SWITCHING,                /*正在转岔*/
	RS_SWITCHING_OK,             /*道岔转换到位*/
	RS_SELECT_COMPLETE,          /*选排一致*/
	RS_SIGNAL_CHECKED_OK,        /*信号检查条件成功*/
	RS_SIGNAL_CHECKED,           /*信号检查成功*/
	RS_ROUTE_LOCKED_OK,          /*进路锁闭锁闭条件满足*/
	RS_ROUTE_LOCKED,             /*进路已锁闭*/	
	RS_SO_SIGNAL_OPENING,        /*信号开放模块正在开放信号*/
	RS_SIGNAL_OPENED,            /*信号已开放*/
	RS_SK_N_SIGNAL_CLOSING,      /*信号保持正在正常关闭信号*/
	RS_SK_N_SIGNAL_CLOSED,       /*信号保持正常关闭信号*/
	RS_SK_A_SIGNAL_CLOSING,      /*信号保持正在非正常关闭信号*/ 
	RS_SK_SIGNAL_CHANGING,		 /*信号保持正在转换信号*/
	RS_AUTOMATIC_UNLOCKING,      /*正在自动解锁*/
	RS_UNLOCKED,                 /*进路已解锁*/
	RS_RCR_SUCCESSIVE_RELAY,     /*人工延时解锁模块正在延时解锁*/
	RS_RCR_G_SIGNAL_CLOSING,     /*人工延时解锁模块正在关闭引导信号*/
	RS_A_SIGNAL_CLOSED,          /*信号非正常关闭*/
	RS_SU_SIGANL_CLOSING,		 /*区段故障解锁正在关闭信号*/
	RS_FAILURE,                  /*进路故障*/
	RS_TRACK_POWER_OFF,          /*轨道停电故障*/
	RS_AUTO_UNLOCK_FAILURE,      /*进路自动解锁故障*/
	RS_CRASH_INTO_SIGNAL,        /*车列冒进信号*/
	RS_G_ROUTE_LOCKED,			 /*引导进路已锁闭*/
	RS_G_SO_SIGNAL_OPENING,      /*正在开放引导信号*/
	RS_G_SIGNAL_OPENED,          /*引导信号已开放*/ 
	RS_G_SO_SIGNAL_CLOSING,      /*正在关闭引导信号*/
	RS_G_SIGNAL_CLOSED,			 /*引导信号已关闭*/
	RS_AU_SUCCESSIVE_RELAY,      /*延续进路部分延时解锁*/
	RS_RCR_A_SIGNAL_CLOSING,     /*人工延时解锁模块正在非正常关闭信号*/
	RS_CR_A_SIGNAL_CLOSING,		 /*取消进路正在非正常关闭信号*/
	RS_SRO_SIGNAL_OPENING,       /*信号重复开放模块正在开放信号*/
	RS_SECTION_UNLOCKING,        /*正在区段故障解锁*/
	RS_SU_RELAY	,				 /*正在区段故障解锁延时*/
	RS_FAILURE_TO_BUILD,         /*进路建立失败*/
	RS_MS_UNLOCK_RELAY           /*中间道岔正在延时解锁*/
}EN_route_state;

typedef enum route_state_machine  /*轨道状态机*/
{
	RSM_FAULT = -1,				 /*进路故障*/
	RSM_INIT,					 /*进路初始化*/
	RSM_TRAIN_APPROACH,			 /*列车接近*/	
	RSM_FIRST_SECTION,			 /*列车压入首区段*/
	RSM_SECOND_SECTION,			 /*列车压入第二区段*/
	RSM_TRAIN_IN_ROUTE,			 /*列车完全进入进路*/
	RSM_G_FAULT_FIRST,			 /*引导进路首区段分路不良*/

	RSM_TRAIN_ABNORMAL_APPROACH, /*列车异常接近*/
	RSM_TRAIN_IN_POSSIBLE,		 /*疑似列车驶入*/
	RSM_TRAIN_ABNORMAL_IN_ROUTE  /*列车异常进入进路*/

}EN_route_state_machine;

typedef enum  route_other_flag  /*进路其他标志(route_other_flag 缩写为ROF)*/
{            
	ROF_ERROR,                    /*错误*/
	ROF_GUIDE,                    /*引导进路*/
	ROF_GUIDE_APPROACH,           /*引导进路且接近占用*/
	ROF_SUCCESSIVE,               /*延续进路*/
	ROF_SUCCESSIVE_DELAY,		  /*延续进路，车列进入接车进路*/
	ROF_APPROACH_ADDED,           /*接近延长*/
	ROF_HOLD_ROUTE_SHUNTING		  /*非进路调车*/
}EN_route_other_flag;

typedef EN_node_direction EN_route_direction; /*进路方向*/

typedef enum  function_button /*功能按钮(function button 缩写为FB)*/
{ 
	FB_NO_BUTTON = -1,			    /*没有按钮操作 */
	FB_STOP_OPERATION = -2,		    /*终止操作 */
	FB_CANCEL_ROUTE = -3,		    /*1.取消进路 */
	FB_HUMAN_UNLOCK = -4,		    /*2.人工解锁 */
	FB_REOPEN_SIGNAL = -5,		    /*3.重复开放信号 */
	FB_SECTION_UNLOCK = -6,         /*4.故障解锁 */
	FB_GUIDE_ROUTE = -7,		    /*5.引导进路 */
	FB_THROAT_LOCK = -8,		    /*6.引总锁闭 */
	FB_THROAT_UNLOCK = -9,		    /*7.引总解锁 */
	FB_SWITCH_LOCK = -10,		    /*8.道岔单锁 */
	FB_SWITCH_UNLOCK = -11,		    /*9.道岔单解 */
	FB_SWITCH_CLOSE = -12,		    /*10.道岔封锁 */
	FB_SWITCH_UNCLOSE = -13,        /*11.道岔解封 */
	FB_SWITCH_NORMAL = -14,		    /*12.道岔定操 */
	FB_SWITCH_REVERSE = -15,	    /*13.道岔反操 */
	FB_CLOSE_SIGNAL = -16,		    /*14.关闭信号 */
	FB_SUCCESSIVE_UNLOCK = -17,	    /*15.坡道解锁 */	
	FB_BUTTON_CLOSE = -18,			/*16.按钮封锁 */
	FB_HOLD_ROUTE = -19,			/*17.非进路控制 */
	FB_HOLD_ROUTE_FAULT = -20,		/*18.非进路故障恢复 */
	FB_SWITCH_LOCAL_CONTROL = -21,  /*19.道岔局部控制 */  
	FB_SEMI_AUTO_BLOCK = -22,       /*20.半自动闭塞按钮 */ 
	FB_SEMI_AUTO_CANCEL = -23,      /*21.半自动复原按钮*/ 
	FB_SEMI_AUTO_FAIL = -24,        /*22.半自动故障按钮*/ 
	FB_SYSTEM_MENU = -25,			/*23.系统菜单*/ 
	FB_INIT_DATA1 = -100,			/*初始化系统数据按钮1*/
	FB_INIT_DATA2 = 1000,			/*初始化系统数据按钮1*/
}EN_function_button;

typedef enum button_type /*按钮类型*/
{
	BT_ERROR,              /*错误的按钮*/
	BT_TRAIN,              /*列车按钮*/
	BT_SHUNTING,           /*调车按钮*/
	BT_PASSING,            /*通过按钮*/
	BT_TRAIN_END,          /*列车终端按钮*/
	BT_SHUNTING_END,       /*调车终端按钮*/
	BT_SUCCESSIVE_END,     /*坡道终端按钮*/
	BT_CHANGE,             /*变更按钮*/
	BT_REQUEST,            /*请求按钮*/
	BT_AGREEMENT,          /*同意按钮*/
	BT_JCBJ,			   /*挤岔报警按钮*/
	BT_INPUT,			   /*输入按钮*/
	BT_OUTPUT,			   /*输出按钮*/
	BT_INPUTOUTPUT,		   /*输入输出按钮*/
	BT_SEMIAUTO_BLOCK,     /*半自动闭塞按钮*/
	BT_SEMIAUTO_CANCEL,    /*半自动复原按钮*/
	BT_SEMIAUTO_FAILURE,   /*半自动事故按钮*/
	BT_THROAT_GUIDE_LOCK   /*引总锁闭按钮*/
}EN_button_type;

typedef enum special_type  /*特殊联锁定义*/
{
	SPT_ERROR,                /*类型错误*/
	SPT_AUTO_BLOCK,           /*自动闭塞*/
	SPT_AUTO_BLOCK3,          /*自动闭塞*/
	SPT_SEMI_AUTO_BLOCK,      /*半自动闭塞*/
	SPT_CHANGE_RUN_DIR,		  /*改方电路*/
	SPT_REQUEST_AGREE,        /*请求同意联系*/
	SPT_SUCCESSIVE_ROUTE,     /*延续进路*/
	SPT_YARDS_LIAISION,       /*场间联系*/
	SPT_STATE_COLLECT,		  /*状态采集*/
	SPT_SPECIAL_SWTICH,		  /*特殊防护道岔*/
	SPT_HUMP,                 /*驼峰*/
	SPT_HOLD_SHUNGTING_ROUTE, /*非进路调车*/
	SPT_MIDDLE_SWITCH,        /*中间道岔（股道出岔）*/
	SPT_INDICATION_LAMP,	  /*表示灯*/
	SPT_LOCODEPOT_AGREE_LAMP, /*机务段同意表示灯*/	
	SPT_OTHER_SPECIAL		  /*其他特殊配置*/
}EN_special_type;

typedef enum hold_route_shunting_state  /*非进路调车状态*/
{
	HRSS_ERROR = -1,		 /*初始状态*/
	HRSS_SWITCHING,			 /*转岔*/
	HRSS_CHECK_SELECTED,	 /*选排一致*/
	HRSS_SIGNAL_CONDITION_OK,/*信号条件检查成功*/
	HRSS_LOCKED,			 /*锁闭*/
	HRSS_OPENING_SIGNAL,     /*开放信号*/
	HRSS_KEEP_SIGNAL_FAILURE,/*信号保持失败*/
	HRSS_CLOSING_SIGNAL,     /*关闭信号*/
	HRSS_DELAY_UNLOCK,		 /*延时解锁*/
	HRSS_DELAY_UNLOCK_START, /*延时解锁开始*/
	HRSS_FAULT_UNLOCK,		 /*故障解锁*/
	HRSS_FAULT_DELAY,		 /*故障延时*/
	HRSS_FAULT_DELAY_FINISH	 /*故障延时完成*/
}EN_hold_route_shunting_state;

#endif
