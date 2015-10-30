/***************************************************************************************
Copyright (C), 2012,  Co.Hengjun, Ltd.
文件名:  app_config.c
作者:    CY
版本 :   1.0	
创建日期:2012/2/17
用途:    定义配置数据
历史修改记录:    
2013/1/28 V1.2.1 hjh
	增加特殊防护道岔的相关配置
2014/2/14 V1.2.1 hjh
	修改股道出岔配置
***************************************************************************************/

#include "base_type_definition.h"
#include "data_struct_definition.h"

///*特殊配置表*/
//ST_special_interlocking special_config[TOTAL_SPECIAL_ITEM] =
//{
//	{"Y_SX_XI", SPT_SUCCESSIVE_ROUTE,0},
//	{"Y_SX_XII",SPT_SUCCESSIVE_ROUTE,1},
//	{"Y_SX_XIV",SPT_SUCCESSIVE_ROUTE,2},
//	{"Y_SX_X6", SPT_SUCCESSIVE_ROUTE,3},
//	{"BZD_SX",SPT_SEMI_AUTO_BLOCK,0},
//	{"ZD_SF",SPT_AUTO_BLOCK,0},
//	{"ZD_XF",SPT_AUTO_BLOCK,1},
//	{"3ZD_XD",SPT_AUTO_BLOCK3,0},
//	{"GF_S",SPT_CHANGE_RUN_DIR,0},
//	{"GF_X",SPT_CHANGE_RUN_DIR,1},
//	{"MS_X6",SPT_MIDDLE_SWITCH,0},
//	{"MS_S6",SPT_MIDDLE_SWITCH,1},
//	{"JT",  SPT_REQUEST_AGREE,0},
//	{"TFBD",SPT_REQUEST_AGREE,1},
//	{"TJBD",SPT_INDICATION_LAMP,0},	
//	{"CL_XLD", SPT_YARDS_LIAISION,0},
//	{"CL_D101",SPT_YARDS_LIAISION,1},
//	{"CL_D32", SPT_YARDS_LIAISION,2},
//	{"TCJ",    SPT_YARDS_LIAISION,3},
//	{"CL_T1",  SPT_YARDS_LIAISION,4},
//	{"CL_T2",  SPT_YARDS_LIAISION,5},
//	{"X_TSRAJ",SPT_STATE_COLLECT,0},
//	{"S_TSRAJ",SPT_STATE_COLLECT,1},
//	{"JG_SL",  SPT_STATE_COLLECT,2},
//	{"JG_D32", SPT_STATE_COLLECT,3},
//	{"SPS_S_XIV", SPT_SPECIAL_SWTICH,0},
//	{"SPS_S_X6",  SPT_SPECIAL_SWTICH,1},
//	{"SPS_XIV_S", SPT_SPECIAL_SWTICH,2},
//	{"SPS_X6_S",  SPT_SPECIAL_SWTICH,3},
//	{"SPS_SF_XIV",SPT_SPECIAL_SWTICH,4},
//	{"SPS_SF_X6", SPT_SPECIAL_SWTICH,5},
//	{"SPS_XIV_SF",SPT_SPECIAL_SWTICH,6},
//	{"SPS_XIV_SF1",SPT_SPECIAL_SWTICH,7},
//	{"SPS_X6_SF", SPT_SPECIAL_SWTICH,8},
//	{"SPS_D12_D4",SPT_SPECIAL_SWTICH,9},
//	{"SPS_D12_D6",SPT_SPECIAL_SWTICH,10},
//	{"SPS_XIV_D4",SPT_SPECIAL_SWTICH,11},
//	{"SPS_XIV_D6",SPT_SPECIAL_SWTICH,12},
//	{"SPS_X6_D4", SPT_SPECIAL_SWTICH,13},
//	{"SPS_X6_D6", SPT_SPECIAL_SWTICH,14}
//};
//
// /*延续进路相关配置*/
//successive_t successive_config[TOTAL_SUCCESSIVE_ROUTE]=
//{
//	{node_D7,node_XD,node_X,node_XF,node_D9}, 
//	{node_XD,node_X,node_XF,node_D9,NO_NODE},	
//	{node_XD,node_X,node_XF,node_D9,NO_NODE},	
//	{node_YZA,node_XD,node_X,node_XF,node_D9}
//}; 
//
///*半自动闭塞配置*/
//semi_auto_block_t semi_auto_block_config[TOTAL_SEMI_AUTO_BLOCK]= /*半自动闭塞配置*/
//{
//	{node_SX_BS,node_SX_FU,node_SX_SG,node_SX_BZD_Z,node_SX_BZD_F,node_SX_BZD,node_SX,-1,SAB_NORMAL,SAS_NORMAL,(CI_TIMER)-1 ,(CI_TIMER)-1}
//};
//
///*自动闭塞配置*/
//auto_block_t auto_block_config[TOTAL_AUTO_BLOCK]=
//{
//	{node_X1LQ,node_X2LQ,node_X3LQ},
//	{node_S1LQ,node_S2LQ,node_S3LQ}
//};
//
///*三显示自动闭塞配置*/
//auto_block3_t auto_block3_config[TOTAL_AUTO_BLOCK3]=
//{
//	{
//		node_XD1JG,node_XD2JG
//	}
//};
//
///*改方运行配置*/
//change_dir_run_t change_run_dir_config[TOTAL_CHANGE_RUN_DIR]=
//{
//	{node_STY,node_S1JG,node_S2JG,node_S3JG},
//	{node_XTY,node_X1JG,node_X2JG,node_X3JG}
//};
//
///*股道出岔配置*/
//middle_switch_t middle_switch_config[TOTAL_MIDLLE_SWITCHES]=
//{
//	{node_X6,{node_50,node_52},{node_48_50WG,node_50DG,node_52DG,node_6G},CI_TRUE},
//	{node_S6,{node_52,node_50},{node_6G,node_52DG,node_50DG,node_48_50WG},CI_FALSE}
//};
//
///*请求同意配置*/
//request_agree request_agree_config[TOTAL_REQUEST_AGREE]=
//{
//	node_JTY,node_TFBD
//};
//
///*表示灯*/
//indication_lamp indication_lamp_config[TOTAL_INDICATION_LAMP]=
//{
//	node_TJBD
//};
//
///*场间联系配置*/
//yards_liaision yards_liaision_config[TOTAL_YARDS_LIAISION]=
//{
//	node_XLDZCJF,node_D101ZCJF,node_D32JGJ,node_TCJF,node_T1ZCJF,node_T2ZCJF
//};
//
///*状态采集配置*/
//state_collect_t state_collect_config[TOTAL_STATE_COLLECT]=
//{	
//	{node_X_TSRAJ,NO_NODE},
//	{node_S_TSRAJ,NO_NODE},
//	{node_SLJGJ,node_L1G},
//	{node_D32JGJ,node_L2G}
//};
//
///*特殊防护道岔*/
//special_switch_t special_switch_config[TOTAL_SPECIAL_SWITCH]=
//{
//	{node_8,SWS_REVERSE,NO_NODE,SWS_ERROR,  node_46DG},
//	{node_2,SWS_NORMAL, NO_NODE,SWS_ERROR,  node_48DG},
//	{node_8,SWS_REVERSE,NO_NODE,SWS_ERROR,  node_6_16DG},
//	{node_2,SWS_NORMAL, NO_NODE,SWS_ERROR,  node_44DG},
//	{node_4,SWS_NORMAL, node_8, SWS_REVERSE,node_46DG},
//	{node_2,SWS_NORMAL, NO_NODE,SWS_ERROR,  node_48DG},
//	{node_4,SWS_NORMAL, node_8, SWS_REVERSE,node_38_40DG},
//	{node_4,SWS_NORMAL, node_8, SWS_REVERSE,node_20_22DG},
//	{node_2,SWS_NORMAL, NO_NODE,SWS_ERROR,  node_44DG},
//	{node_8,SWS_REVERSE,NO_NODE,SWS_ERROR,  node_20_22DG},
//	{node_4,SWS_NORMAL, node_8, SWS_REVERSE,node_6_16DG},
//	{node_8,SWS_REVERSE,NO_NODE,SWS_ERROR,  node_6_16DG},
//	{node_4,SWS_NORMAL, node_8, SWS_REVERSE,node_38_40DG},
//	{node_2,SWS_NORMAL, NO_NODE,SWS_ERROR,  node_44DG},
//	{node_2,SWS_NORMAL, NO_NODE,SWS_ERROR,  node_44DG}
//};
//
///*同意动岔表示灯*/
//agree_opreater_switch_t agree_opreater_switch_config[TOTAL_SWITCH_INDICATION_LAMP]=
//{	
//    {node_54_56DBJF,{node_54,SWS_NORMAL}}
//};
//
///*非进路调车*/
//hold_route_shunting_t hold_route_shunting_config[TOTAL_HOLD_SHUNGTING_ROUTE]=
//{
//	{HRSS_ERROR,node_D7,node_D47,node_FJL,node_FJLGZ,{node_67_69DG,node_71DG}}
//};
//
///*引总锁闭*/
//throat_guide_lock throat_guide_lock_config[TOTAL_THROAT_GUIDE_LOCK]=
//{
//	node_X_YDZS,node_S_YDZS
//};
//
///*其他特殊配置*/
//other_special_t other_special_config=
//{
//	/*特殊点*/
//	{node_GTJ,node_SNJJ},
//	/*延时30秒*/
//	{node_XIV,node_X5,node_X6,node_S5,node_S6},
//	/*检查红灯断丝*/
//	{node_X,node_XF,node_XD,node_S,node_SF,node_SX,node_XL,node_XLIII,node_SL,node_SI,node_SII,node_SIII,node_XI,node_XII},
//	/*通向安全线的道岔*/
//	{{node_2,SWS_NORMAL},{node_63,SWS_NORMAL}},
//	/*不能解锁的区段*/
//	{node_2DG},
//	/*位置表示相反道岔*/
//	{node_2},
//	/*18号道岔，允许反位通过*/
//	{node_4},
//	/*高速道岔，同时只允许一组道岔动作*/
//	{node_1,node_7,node_11,node_23,node_29,node_31,node_55,node_61,node_40,node_38,node_28,node_26,node_16,node_14,node_6},
//};
//
///*信号显示列表*/
//signal_show_t signal_show_config[TOTAL_SIGNAL_SHOW]=
//{				/*绿，绿黄，黄，双黄，黄闪/双绿，引白，白*/
//	/*进站信号机显示*/
//	{node_X,    {SGS_L,SGS_LU,SGS_U,SGS_UU,SGS_ERROR,SGS_YB,SGS_ERROR}},
//	{node_XD,   {SGS_L,SGS_LU,SGS_U,SGS_UU,SGS_ERROR,SGS_YB,SGS_ERROR}},
//	{node_XF,   {SGS_L,SGS_LU,SGS_U,SGS_UU,SGS_ERROR,SGS_YB,SGS_ERROR}},
//	{node_S,    {SGS_L,SGS_LU,SGS_U,SGS_UU,SGS_ERROR,SGS_YB,SGS_ERROR}},
//	{node_SF,   {SGS_L,SGS_LU,SGS_U,SGS_UU,SGS_ERROR,SGS_YB,SGS_ERROR}},
//	{node_SX,   {SGS_ERROR,SGS_LU,SGS_U,SGS_UU,SGS_USU,SGS_YB,SGS_ERROR}},
//	/*进路信号机显示*/
//	{node_XL,	{SGS_L,SGS_LU,SGS_U,SGS_UU,SGS_ERROR,SGS_YB,SGS_B}},
//	{node_XLIII,{SGS_L,SGS_LU,SGS_U,SGS_UU,SGS_ERROR,SGS_YB,SGS_B}},
//	{node_SL,	{SGS_L,SGS_LU,SGS_U,SGS_UU,SGS_ERROR,SGS_YB,SGS_B}},
//	/*出站信号机显示*/
//	{node_SI,   {SGS_L,SGS_LU,SGS_U,SGS_ERROR,SGS_ERROR,SGS_ERROR,SGS_B}},
//	{node_SII,  {SGS_L,SGS_LU,SGS_U,SGS_ERROR,SGS_ERROR,SGS_ERROR,SGS_B}},
//	{node_SIII, {SGS_L,SGS_LU,SGS_U,SGS_ERROR,SGS_ERROR,SGS_ERROR,SGS_B}},
//	{node_SIV,  {SGS_L,SGS_LU,SGS_U,SGS_ERROR,SGS_ERROR,SGS_ERROR,SGS_B}},
//	{node_S5,   {SGS_L,SGS_LU,SGS_U,SGS_ERROR,SGS_ERROR,SGS_ERROR,SGS_B}},
//	{node_S6,   {SGS_L,SGS_LU,SGS_U,SGS_ERROR,SGS_ERROR,SGS_ERROR,SGS_B}},
//	{node_XI,   {SGS_L,SGS_LU,SGS_U,SGS_ERROR,SGS_ERROR,SGS_ERROR,SGS_B}},
//	{node_XII,  {SGS_L,SGS_LU,SGS_U,SGS_ERROR,SGS_ERROR,SGS_ERROR,SGS_B}},
//	{node_XIV,	{SGS_L,SGS_LU,SGS_U,SGS_ERROR,SGS_ERROR,SGS_ERROR,SGS_B}},
//	{node_X5,	{SGS_L,SGS_LU,SGS_U,SGS_ERROR,SGS_ERROR,SGS_ERROR,SGS_B}},
//	{node_X6,	{SGS_L,SGS_LU,SGS_U,SGS_ERROR,SGS_ERROR,SGS_ERROR,SGS_B}}
//};

