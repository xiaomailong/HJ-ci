/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 何境泰
版本        : 1.0
创建日期    : 2013年11月20日 10:51:13
用途        : 联锁机与控显机的通信
历史修改记录: v1.0    创建
**********************************************************************/
#ifndef _hmi_manage_h__
#define _hmi_manage_h__

#include "util/ci_header.h"

#ifdef __cplusplus
extern "C" {
#endif
/*
 功能描述    : 打开控显机信息输出
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年4月11日 15:45:54
*/
CIAPI_FUNC(int32_t) CIHmi_OpenPrintHmi(void);
/*
 功能描述    : 从控显机接受数据
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年3月5日 15:23:52
*/
CIAPI_FUNC(int32_t) CIHmi_RecvData(void);
/*
 功能描述    : 将缓存当中的数据全部发送
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年9月17日 13:26:35
*/
CIAPI_FUNC(int32_t) CIHmi_WriteCachePush(void);
/*
功能描述    : 联锁机向控显机发送的需要向提示框显示的信息
返回值      : 无
参数        : @fmt 以printf标准格式传入参数并格式化打印输出
作者        : 何境泰
日期        : 2013年12月6日 18:45:27
*/
CIAPI_FUNC(int32_t) CIHmi_SendNormalTips(char* fmt,...);
/*
功能描述    : 发送调试数据信息
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2013年12月6日 18:45:27
*/
CIAPI_FUNC(int32_t) CIHmi_SendDebugTips(char* fmt,...);
/*
功能描述    : 发送timer数据信息
返回值      : 无
参数        : @fmt 标准C格式
作者        : 张彦升
日期        : 2013年12月6日 18:45:27
*/
CIAPI_FUNC(int32_t) CIHmi_SendTimerTips(char* fmt,...);
/*
 功能描述    : 发送错误提示信息，在控显机当中一般情况下只显示错误提示信息，在调试状态下
             可显示CIHmi_SendTips的调试信息
 返回值      : 
 参数        : @fmt 以printf标准格式传入参数并格式化打印输出
 作者        : 何境泰
 日期        : 2013年12月6日 18:45:27
*/
CIAPI_FUNC(int32_t) CIHmi_SendNodeData(void);
/*
 功能描述    : 返回已整数值保存的A口ip地址
 返回值      : 控显机的IP地址
 参数        : 
 作者        : 张彦升
 日期        : 2014年5月12日 10:32:43
*/
CIAPI_FUNC(uint32_t) CIHmi_GetIpA(void);
/*
 功能描述    : 返回控显机A口的端口
 返回值      : 控显机的端口
 参数        : 
 作者        : 张彦升
 日期        : 2014年5月12日 10:33:08
*/
CIAPI_FUNC(uint16_t) CIHmi_GetPortA(void);
/*
 功能描述    : 返回已整数值保存的B口ip地址
 返回值      : 控显机的IP地址
 参数        : 
 作者        : 张彦升
 日期        : 2014年5月12日 10:32:43
*/
CIAPI_FUNC(uint32_t) CIHmi_GetIpB(void);
/*
 功能描述    : 返回控显机B口的端口
 返回值      : 控显机的端口
 参数        : 
 作者        : 张彦升
 日期        : 2014年5月12日 10:33:08
*/
CIAPI_FUNC(uint16_t) CIHmi_GetPortB(void);
/*
 功能描述    : 返回控显机设备id
 返回值      : 控显机的id
 参数        : 
 作者        : 张彦升
 日期        : 2014年5月12日 10:33:08
*/
CIAPI_FUNC(uint8_t) CIHmi_GetDeviceId(void);
/*
 功能描述    : 控显机A通信道是否断开
 返回值      : 若断开则返回CI_TRUE，否则返回CI_FALSE
 参数        : 无
 日期        : 2015年5月29日 18:34:31
*/
CIAPI_FUNC(CI_BOOL) CIHmi_IsChannelAOk(void);
/*
 功能描述    : 控显机B通信道是否断开
 返回值      : 若断开则返回CI_TRUE，否则返回CI_FALSE
 参数        : 无
 日期        : 2015年5月29日 18:34:31
*/
CIAPI_FUNC(CI_BOOL) CIHmi_IsChannelBOk(void);
/*
功能描述    : 初始化控显机相关数据
返回值      : 成功为0，失败为-1
参数        : 
作者        : 张彦升
日期        : 2013年12月4日 8:39:07
*/
CIAPI_FUNC(int32_t) CIHmi_Init(void);

#ifdef __cplusplus
}
#endif

#endif /*!_hmi_manage_h__*/