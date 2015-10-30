/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年10月29日 15:41:05
用途        : 双系管理
历史修改记录: v1.0    创建
**********************************************************************/
#ifndef _series_manage_h__
#define _series_manage_h__

#include "util/ci_header.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    SERIES_PENDING  = 0,       /*停机状态*/
    SERIES_CHECK    = 1,       /*校核状态*/
    SERIES_MASTER   = 2,       /*主机状态*/
    SERIES_STANDBY  = 3,       /*热备状态*/
}SeriesState;
/*
 功能描述    : 得到本地双系的状态
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年7月28日 11:09:00
*/
CIAPI_FUNC(SeriesState) CISeries_GetLocalState(void);
/*
 功能描述    : 设置另外一系的双系状态
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年7月28日 12:52:04
*/
CIAPI_FUNC(void) CISeries_SetPeerState(SeriesState state);
/*
 功能描述    : 得到另外一系的状态
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年7月28日 11:09:00
*/
CIAPI_FUNC(SeriesState) CISeries_GetPeerState(void);
/*
功能描述    : 双系切换至主系
返回值      : 成功为0，失败为-1
参数        : 
作者        : 张彦升
日期        : 2013年10月29日 14:14:54
*/
CIAPI_FUNC(int32_t) CISeries_SwitchToMaster(void);
/*
 功能描述    : 切换到备系
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年5月23日 13:31:04
*/
CIAPI_FUNC(int32_t) CISeries_SwitchToCheck(void);
/*
 功能描述    : 切换到热备状态
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年5月23日 13:31:04
*/
CIAPI_FUNC(int32_t) CISeries_SwitchToStandby(void);
/*
 功能描述    : 从CPU双系状态调整
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年7月28日 12:56:29
*/
CIAPI_FUNC(int32_t) CISeries_SlaveCpuAdjustState(void);
/*
功能描述    : 发送备系校核同步数据(主备系)
返回值      : 成功为0，失败为-1
参数        : 
作者        : 张彦升
日期        : 2013年10月30日 13:01:57
*/
CIAPI_FUNC(int32_t) CISeries_SendCfgSyn(void);
/*
功能描述    : 接受备系校核同步数据(备系)
返回值      : 收到数据返回0，未收到数据返回-1
参数        : 
作者        : 张彦升
日期        : 2013年10月30日 13:02:17
*/
CIAPI_FUNC(int32_t) CISeries_RecvCfgSyn(void);
/*
 功能描述    : 检查备系校核同步数据
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年5月4日 14:16:39
*/
CIAPI_FUNC(int32_t) CISeries_CheckCfgSyn(void);
/*
 功能描述    : 打开备系校核同步数据开关
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年5月14日 18:38:59
*/
CIAPI_FUNC(int32_t) CISeries_OpenPrintCfgSyn(void);
/*
 功能描述    : 关闭启动时比较配置详细信息
 返回值      : 无
 参数        : 无
 作者        : 张彦升
 日期        : 2014年8月21日 14:53:50
*/
CIAPI_FUNC(void) CISeries_CloseCfgCompare(void);
/*
 功能描述    : 打开启动时比较配置详细信息
 返回值      : 无
 参数        : 无
 作者        : 张彦升
 日期        : 2014年8月21日 14:53:50
*/
CIAPI_FUNC(void) CISeries_OpenCfgCompare(void);
/*
 功能描述    : 进行配置比较
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年5月4日 13:57:50
*/
CIAPI_FUNC(int32_t) CISeries_CfgSyn(void);
/*
功能描述    : 发送双系新周期同步消息
返回值      : 成功为0，失败为-1
参数        : 
作者        : 张彦升
日期        : 2013年10月30日 13:02:29
*/
CIAPI_FUNC(int32_t) CISeries_SendNewCycleSyn(void);
/*
功能描述    : 接受新周期同步消息
返回值      : 成功为0，失败为-1
参数        : 
作者        : 张彦升
日期        : 2013年10月30日 13:02:55
*/
CIAPI_FUNC(int32_t) CISeries_RecvNewCycleSyn(void);
/*
 功能描述    : 检查新周期同步的数据是否一致
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年5月6日 8:03:18
*/
CIAPI_FUNC(int32_t) CISeries_CheckNewCycleSyn(void);
/*
 功能描述    : 打开新周期同步信息输出
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年4月11日 15:29:15
*/
CIAPI_FUNC(int32_t) CISeries_OpenPrintNewCycleSyn(void);
/*
功能描述    : 发送本系输入同步数据
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2013年10月29日 14:45:09
*/
CIAPI_FUNC(int32_t) CISeries_SendInputSyn(void);
/*
 功能描述    : 发送校核输入数据
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年7月31日 14:11:51
*/
CIAPI_FUNC(int32_t) CISeries_SendCheckInputSyn(void);
/*
功能描述    : 接收它系输入同步数据
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2013年10月29日 14:45:45
*/
CIAPI_FUNC(int32_t) CISeries_RecvInputSyn(void);
/*
功能描述    : 检查双系输入数据是否一致
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2013年10月29日 14:49:36
*/
CIAPI_FUNC(int32_t) CISeries_CheckInputSyn(void);
/*
 功能描述    : 是否打印输入信息
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年4月11日 15:29:15
*/
CIAPI_FUNC(int32_t) CISeries_OpenPrintInputSyn(void);
/*
功能描述    : 发送本系输出结果
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2013年10月29日 14:46:11
*/
CIAPI_FUNC(int32_t) CISeries_SendResultSyn(void);
/*
功能描述    : 发送他系输出结果
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2013年10月29日 14:47:28
*/
CIAPI_FUNC(int32_t) CISeries_RecvResultSyn(void);
/*
功能描述    : 检查双系输出结果是否出错
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2013年10月29日 14:50:53
*/
CIAPI_FUNC(int32_t) CISeries_CheckResultSyn(void);
/*
 功能描述    : 打开结果信息的输出
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年4月11日 15:29:15
*/
CIAPI_FUNC(int32_t) CISeries_OpenPrintResultSyn(void);
/*
功能描述    : 发送双系心跳
返回值      : 成功为0，失败为-1
参数        : 
作者        : 张彦升
日期        : 2013年10月30日 13:01:57
*/
CIAPI_FUNC(int32_t) CISeries_SendHeartbeat(void);
/*
功能描述    : 接受双系心跳
返回值      : 成功为0，失败为-1
参数        : 
作者        : 张彦升
日期        : 2013年10月30日 13:02:17
*/
CIAPI_FUNC(int32_t) CISeries_RecvHeartbeat(void);
/*
 功能描述    : 检查双系心跳信号
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年4月30日 13:54:19
*/
CIAPI_FUNC(int32_t) CISeries_CheckHeartbeat(void);
/*
 功能描述    : 打开调试双系心跳信息
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年4月11日 15:27:25
*/
CIAPI_FUNC(int32_t) CISeries_OpenPrintHeartbeat(void);
/*
 功能描述    : 得到双系状态的名称以便打印
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年4月30日 13:53:50
*/
CIAPI_FUNC(const) char* CISeries_GetStateName(SeriesState state);
/*
 功能描述    : 得到双系的id（I或者II）
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年3月14日 18:14:27
*/
CIAPI_FUNC(const char*) CISeries_GetId(void);
/*
功能描述    : 数据容错函数，该函数很少会执行到，所以需要仔细测试
返回值      : 成功为0，失败为-1
参数        : 
作者        : 何境泰
日期        : 2014年5月12日 20:49:28
*/
CIAPI_FUNC(int32_t) CISeries_FaultTolerantInputSyn(void);
/*
 功能描述    : 清除光纤驱动中存留的所有缓存数据
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年6月16日 10:56:57
*/
CIAPI_FUNC(int32_t) CISeries_CleanRecvBuffer(void);
/*
 * 返回主系电子单元是否断开标志
 */
CIAPI_FUNC(CI_BOOL) CISeries_IsPeerEeuBroke(void);
/*
功能描述    : 初始化双系
返回值      : 成功为0，失败为-1
参数        : 
作者        : 张彦升
日期        : 2013年10月29日 15:43:55
*/
CIAPI_FUNC(int32_t) CISeries_Init(void);

#ifdef __cplusplus
}
#endif

#endif /*!_series_manage_h__*/
