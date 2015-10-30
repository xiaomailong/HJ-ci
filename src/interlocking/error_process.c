/***************************************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.
文件名:  error_process.c
作者:    CY
版本 :   1.0	
创建日期:2011/12/1
用途:    系统错误处理模块
历史修改记录:         
**************************************************************************************/
#ifndef ERROR_PROCESS
#define ERROR_PROCESS

#include "error_process.h"

char * ErrorInfor[] =
{
    "用户操作错误",
    "命令处理模块按钮错误",
    "进路状态错误",
    "按钮错误，不能搜索出进路",
    "信号节点数据错误",
    "联锁表数据错误",
    "不允许跨咽喉调车",
    "道岔锁闭",
    "区段占用",
    "敌对进路",
    "迎面敌对",
    "引总锁闭",
    "侵限占用",
    "联系条件不满足",
    "区间占用",
    "防护道岔条件不满足",
    "道岔进路锁闭",
    "道岔单锁",
    "道岔引总锁闭",
    "双动道岔不满足条件",
    "道岔动作超时",
    "选路超时",
    "选排一致错误",
    "道岔位置错误",
    "未转安全线",
    "丢失征用标志",
    "节点已锁闭",
    "灯丝断丝",
    "三点检查条件不满足",
    "信号非正常关闭",
    "区段故障",
    "取消延续进路错误",
    "接近区段占用",
    "离去区段占用",
    "进路占用",
    "车辆冒进信号",
    "信号为正常关闭，不允许重复开放",
    "索引号错误或越界",
    "进路类型错误",
    "信号无法关闭",
    "接车进路未解锁",
    "信号错误开放，或信号未关闭错误",
	"信号设备设备通信中断或接收数据错误",
	"轨道设备设备通信中断或接收数据错误",
	"道岔设备设备通信中断或接收数据错误",
	"零散设备设备通信中断或接收数据错误",
	"总启动设备通信中断或接收数据错误",
    "信号点被征用",
    "信号点被进路锁闭",
    "信号点未被锁闭",
    "进路数据错误",
    "列车运行中",
    "延续进路条件不满足",
    "命令执行失败",
};
string ErrorInfoE[]=
{
    "ERR_OPERATION",                /*用户操作错误*/  
    "ERR_BUTTON",                   /*命令处理模块按钮错误*/
    "ERR_ROUTE_STATE",              /*进路状态错误*/
    "ERR_NO_ROUTE",                 /*按钮错误，不能搜索出进路*/
    "ERR_NODE_DATA",                /*信号节点数据错误*/
    "ERR_ILT_DATA",                 /*联锁表数据错误*/
    "ERR_THROAT_SHUNTING",          /*不允许跨咽喉调车*/
    "ERR_SWITCH_LOCKED",            /*道岔锁闭*/
    "ERR_SECTION_OCCUPIED",         /*区段占用*/
    "ERR_CONFLICT_ROUTE",           /*敌对进路*/
    "ERR_FACE_CONFLICT",            /*迎面敌对*/
    "ERR_GUIDE_ALL_LOCKED",         /*引总锁闭*/
    "ERR_EXCEED_LIMIT_OCCUPIED",    /*侵限占用*/
    "ERR_RELATION_CONDITION",       /*联系条件不满足*/
    "ERR_BLOCK_OCCUPIED",           /*区间占用*/    
    "ERR_PROTECTIVE_SWITCH",        /*防护道岔条件不满足*/
    "ERR_SWITCH_ROUTE_LOCKED",      /*道岔进路锁闭*/
    "ERR_SWITCH_SINGLE_LOCKED",     /*道岔单锁*/
    "ERR_SWITCH_GUIDE_LOCKED",      /*道岔引总锁闭*/
    "ERR_DOUBLE_SWITCH",            /*双动道岔不满足条件*/
    "ERR_SWITCH_OUTTIME",           /*道岔动作超时*/
    "ERR_SELECT_ROUTE_SWITCH",      /*选路超时*/
    "ERR_SELECT_COMFORT",           /*选排一致错误*/
    "ERR_SWITCH_LOCATION",          /*道岔位置错误*/
    "ERR_SAFE_LINE",	            /*未转安全线*/
    "ERR_LOSE_USED_FLAG",           /*丢失征用标志*/
    "ERR_NODE_LOCKED",              /*节点已锁闭*/
    "ERR_FILAMENT_BREAK",           /*灯丝断丝*/
    "ERR_3POINT_CHECK",             /*三点检查条件不满足*/
    "ERR_SIGNAL_ABNORMAL_CLOSE",    /*信号非正常关闭*/
    "ERR_SECTION_FAIL",             /*区段故障*/
    "ERR_CANCEL_CONTINUE",          /*取消延续进路错误*/
    "ERR_APPROACH_OCCUPIED",        /*接近区段占用*/
    "ERR_DEPARTURE_OCCUPIED",       /*离去区段占用*/
    "ERR_ROUTE_OCCUPIED",           /*进路占用*/
    "ERR_CRASH_INTO_SIGNAL",        /*车辆冒进信号*/
    "ERR_SIGNAL_NORMAL_CLOSE",      /*信号为正常关闭，不允许重复开放*/
    "ERR_INDEX",                    /*索引号错误或越界*/
    "ERR_ROUTE_TYPE",               /*进路类型错误*/
    "ERR_CLOSE_SIGNAL",             /*信号无法关闭*/
    "ERR_SUCCESSIVE_CANCLE",        /*接车进路未解锁*/
    "ERR_SIGNAL_OPEN",              /*信号错误开放，或信号未关闭错误*/
    "ERR_SIGNAL_DEVICE_ERR",        /*信号设备故障*/
    "ERR_SECTION_DEVICE_ERR",       /*轨道设备故障*/
    "ERR_SWITCH_DEVICE_ERR",        /*道岔设备故障*/
    "ERR_LIAISON_DEVICE_ERR",       /*零散设备故障*/
    "ERR_SIGNAL_NODE_USED",         /*信号点被征用*/
    "ERR_SIGNAL_NODE_LOCKED",       /*信号点被进路锁闭*/
    "ERR_NODE_UNLOCK",              /*信号点未被锁闭*/
    "ERR_ROUTE_DATA",               /*进路数据错误*/
    "ERR_TRAIN_IN_ROUTE",           /*列车运行中*/
    "ERR_SUCCESSIVE_ROUTE",         /*延续进路条件不满足*/
    "ERR_COMMAND_EXECUTE",          /*命令执行失败*/
};

/*
 功能描述    : process_warning宏的扩展函数
 返回值      : 无
 参数        : 
 作者        : 张彦升
 日期        : 2013年12月30日 13:37:55
*/
void process_warning_ext(EN_ERROR err,const char* info,
                         const char* function_name,int32_t line_num)
{
    CIHmi_SendDebugTips("Warning:%s(函数%s第%d行[%s])",ErrorInfor[err],function_name,line_num,info);

    return;
}

#endif