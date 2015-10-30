/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年10月15日 9:42:50
用途        : 
历史修改记录: v1.0    创建
**********************************************************************/

#ifndef _series_heartbeat_manage_h__
#define _series_heartbeat_manage_h__

#ifdef __cplusplus
extern "C" {
#endif
/*
 功能描述    : 心跳信号是否丢失，表示通讯已中断
 返回值      : 心跳信号丢失则返回CI_TRUE，否则返回CI_FALSE
 参数        : 无
 日期        : 2015年5月29日 19:37:39
*/
CIAPI_FUNC(CI_BOOL) CISeriesHeartbeatInt_IsLost(void);
/*
 功能描述    : 双系心跳信号初始化
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年4月11日 15:43:16
*/
CIAPI_FUNC(int32_t) CISeriesHeartbeatInt_Init(void);

#ifdef __cplusplus
}
#endif

#endif /*!_series_heartbeat_manage_h__*/

