/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年10月12日 9:16:38
用途        : 平台层向外部提供的接口
历史修改记录: 
**********************************************************************/

#ifndef _platform_api_h__
#define _platform_api_h__

#include "util/ci_header.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
功能描述    : 联锁机向控显机发送的需要向提示框显示的信息
返回值      : 无
参数        : @fmt 以printf标准格式传入参数并格式化打印输出
作者        : 何境泰
日期        : 2013年12月6日 18:45:27
*/
extern int32_t CIHmi_SendNormalTips(char* fmt,...);
/*
功能描述    : 发送错误提示信息，在控显机当中一般情况下只显示错误提示信息，在调试状态下
            可显示CIHmi_SendTips的调试信息
返回值      : 
参数        : @fmt 以printf标准格式传入参数并格式化打印输出
作者        : 何境泰
日期        : 2013年12月6日 18:45:27
*/
extern int32_t CIHmi_SendDebugTips(char* fmt,...);
/*
功能描述    : 发送timer数据信息
返回值      : 无
参数        : @fmt 标准C格式
作者        : 张彦升
日期        : 2013年12月6日 18:45:27
*/
extern int32_t CIHmi_SendTimerTips(char* fmt,...);
/*
功能描述    : 发送节点命令，在windows下即往模拟机发送信息，在linux下往can板发送
返回值      : 无
参数        : @node_index 节点索引号
             @cmd 控制命令
作者        : 何境泰
日期        : 2013年12月13日 13:21:34
*/
extern void CIEeu_SendNodeIndexCmd(int16_t node_index,uint32_t cmd);
/*
功能描述    : 为电子单元添加数据有效标志
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2014年5月9日 13:47:15
*/
extern void CIEeu_SetDataMask(int16_t ga, int16_t ea);
/*
功能描述    : 获取联锁周期数
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2013年11月4日 9:39:22
*/
extern uint32_t CICycleInt_GetCounter(void);

/*
功能描述     : 设置是否要记录电子单元数据
返回值       : 
参数         : 
作者         : 何境泰
日期         : 2014年7月14日 10:53:08
*/
extern void CIEeu_SetRecord (CI_BOOL is_record);
/*
 功能描述    : 向远程日志记录机发送
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年6月5日 15:54:57
*/
extern int32_t CIRemoteLog_Write(const char* fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /*!_platform_api_h__*/
