/*********************************************************************
 Copyright (C), 2011,  Co.Hengjun, Ltd.

 作者        : 张彦升
 版本        : 1.0
 创建日期    : 2014年4月25日 13:22:11
 用途        : 看门狗管理 
 历史修改记录: v1.0    创建
**********************************************************************/
#ifndef _watchdog_int_h__
#define _watchdog_int_h__

#ifdef __cplusplus
extern "C" {
#endif
/*
 功能描述    : 停止看门狗
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年4月25日 13:50:33
*/
CIAPI_FUNC(int32_t) CIWatchdog_Stop(void);
/*
 功能描述    : 初始化看门狗
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年4月25日 13:50:45
*/
CIAPI_FUNC(int32_t) CIWatchdog_Init(void);

#ifdef __cplusplus
}
#endif

#endif /*!_watchdog_int_h__*/
