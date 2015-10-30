/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年10月11日 10:42:29
用途        : CPU管理
历史修改记录: 
**********************************************************************/

#ifndef _cpu_manage_h__
#define _cpu_manage_h__

#include "series_manage.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _CpuState/*CPU工作状态*/
{
    CPU_STATE_NONE      = 0,        /*未知状态*/
    CPU_STATE_MASTER    = 1,        /*主CPU*/
    CPU_STATE_SLAVE     = 2,        /*从CPU*/
}CpuState;

/*
功能描述    : 得到CPU状态
返回值      : 当前CPU的状态
参数        : 无
作者        : 张彦升
日期        : 2013年10月28日 9:15:36
*/
CIAPI_FUNC(CpuState) CICpu_GetLocalState(void);
/*
 功能描述    : 得到同伴CPU的双系状态
 返回值      : 另外一个cpu的状态信息
 参数        : 误
 作者        : 张彦升
 日期        : 2014年5月9日 14:51:23
*/
CIAPI_FUNC(SeriesState) CICpu_GetPeerSeriesState(void);
/*
 功能描述    : 得到CPU状态的名称
 返回值      : 根据状态类型得到的状态名称字符串指针
 参数        : 枚举类型的CPU状态
 作者        : 张彦升
 日期        : 2014年4月11日 15:28:26
*/
CIAPI_FUNC(const) char* CICpu_GetStateName(CpuState state);
/*
 功能描述    : 发送CPU配置比较信息
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月11日 15:29:15
*/
CIAPI_FUNC(int32_t) CICpu_SendCfgSyn(void);
/*
 功能描述    : 接受CPU配置比较信息
 返回值      : 成功为0，失败为-1
 参数        : 误
 作者        : 张彦升
 日期        : 2014年4月11日 15:29:15
*/
CIAPI_FUNC(int32_t) CICpu_RecvCfgSyn(void);
/*
 功能描述    : 检查CPU配置比较信息
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月11日 15:29:15
*/
CIAPI_FUNC(int32_t) CICpu_CheckCfgSyn(void);
/*
 功能描述    : 双CPU之间的比较
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月30日 7:58:15
*/
CIAPI_FUNC(int32_t) CICpu_CfgSyn(void);
/*
 功能描述    : 打开配置比较信息的输出
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月11日 15:29:15
*/
CIAPI_FUNC(int32_t) CICpu_OpenPrintCfgSyn(void);
/*
 功能描述    : 打开新周期同步信息输出
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月11日 15:29:15
*/
CIAPI_FUNC(int32_t) CICpu_OpenPrintNewCycleSyn(void);
/*
 功能描述    : 发送CPU新周期同步信息
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月11日 15:29:15
*/
CIAPI_FUNC(int32_t) CICpu_SendNewCycleSyn(void);
/*
 功能描述    : 接受CPU新周期同步信息
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月11日 15:29:15
*/
CIAPI_FUNC(int32_t) CICpu_RecvNewCycleSyn(void);
/*
 功能描述    : 检查CPU新周期同步信息
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月11日 15:29:15
*/
CIAPI_FUNC(int32_t) CICpu_CheckNewCycleSyn(void);
/*
 功能描述    : 是否打印输入信息
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月11日 15:29:15
*/
CIAPI_FUNC(int32_t) CICpu_OpenPrintInputSyn(void);
/*
 功能描述    : 发送CPU输入同步信息
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月11日 15:29:15
*/
CIAPI_FUNC(int32_t) CICpu_SendInputSyn(void);
/*
 功能描述    : 接受CPU输入同步信息
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月11日 15:29:15
*/
CIAPI_FUNC(int32_t) CICpu_RecvInputSyn(void);
/*
 功能描述    : 检查CPU输入同步信息
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月11日 15:29:15
*/
CIAPI_FUNC(int32_t) CICpu_CheckInputSyn(void);
/*
 功能描述    : 对CPU输入数据进行容错
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月11日 15:29:15
*/
CIAPI_FUNC(int32_t) CICpu_FaultTolerantInputSyn(void);
/*
 功能描述    : 打开结果信息的输出
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月11日 15:29:15
*/
CIAPI_FUNC(int32_t) CICpu_OpenPrintResultSyn(void);
/*
 功能描述    : 发送CPU结果数据
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月11日 15:29:15
*/
CIAPI_FUNC(int32_t) CICpu_SendResultSyn(void);
/*
 功能描述    : 接受CPU结果数据
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月11日 15:29:15
*/
CIAPI_FUNC(int32_t) CICpu_RecvResultSyn(void);
/*
 功能描述    : 检查CPU结果数据
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月11日 15:29:15
*/
CIAPI_FUNC(int32_t) CICpu_CheckResultSyn(void);
/*
 功能描述    : 打开调试CPU心跳信息
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月11日 15:27:25
*/
CIAPI_FUNC(int32_t) CICpu_OpenPrintHeartbeat(void);
/*
 功能描述    : 发送CPU心跳信号
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月11日 15:28:55
*/
CIAPI_FUNC(int32_t) CICpu_SendHeartbeat(void);
/*
 功能描述    : 接受CPU心跳信号
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月11日 15:29:04
*/
CIAPI_FUNC(int32_t) CICpu_RecvHeartbeat(void);
/*
 功能描述    : 检查CPU心跳信号
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月11日 15:29:15
*/
CIAPI_FUNC(int32_t) CICpu_CheckHearbeat(void);
/*
功能描述    : CPU初始化
返回值      : 成功为0，失败为-1
参数        : 无
作者        : 张彦升
日期        : 2013年10月29日 15:36:23
*/
CIAPI_FUNC(int32_t) CICpu_Init(void);

#ifdef __cplusplus
}
#endif

#endif /*!_cpu_manage_h__*/
