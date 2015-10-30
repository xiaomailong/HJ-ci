/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年10月11日 10:42:29
用途        : 
历史修改记录: 
**********************************************************************/

#ifndef _cpu_heartbeat_int_h__
#define _cpu_heartbeat_int_h__

#include "util/ci_header.h"

#ifdef __cplusplus
extern "C" {
#endif
/*
功能描述    : CPU心跳的管理的初始化
返回值      : 成功为0，失败为-1
参数        : 无
作者        : 张彦升
日期        : 2013年10月28日 9:16:36
*/
CIAPI_FUNC(int32_t) CICpuHeartbeatInt_init(void);

#ifdef __cplusplus
}
#endif

#endif /*! _cpu_heartbeat_int_h__*/
