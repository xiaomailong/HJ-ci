/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年10月15日 9:39:07
用途        : 联锁运算周期管理
历史修改记录: v1.0    创建
**********************************************************************/

#ifndef _cycle_int_h__
#define _cycle_int_h__

#include "util/ci_header.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CI_CYCLE_NANO (CI_CYCLE_MS * 1000000)      /*联锁周期，使用纳秒级，默认为400ms*/
#define CI_CYCLE_RELATIVE ((int)(CI_CYCLE_NANO / TIMER_CYCLE_NANO))  /*相对于主定时器的周期*/
#define EEU_REQUEST_CYCLE_RELATIVE CI_CYCLE_RELATIVE  /*电子单元请求相对于主定时器的周期*/

/*
功能描述    : 一个联锁运算周期
返回值      : 无
参数        : 无
作者        : 张彦升
日期        : 2013年10月16日 14:39:38
*/
CIAPI_FUNC(void) CICycleInt_Once(void);
/*
功能描述    : 获取联锁周期数
返回值      : 当前的联锁周期
参数        : 无
作者        : 何境泰
日期        : 2013年11月4日 9:37:03
*/
CIAPI_FUNC(uint32_t) CICycleInt_GetCounter(void);
/*
功能描述    : 修改联锁周期数
返回值      : 旧的周期值
参数        : @counter 计数值
作者        : 何境泰
日期        : 2013年12月25日 10:05:52
*/
CIAPI_FUNC(uint32_t) CICycleInt_SetCounter(uint32_t counter);
/*
功能描述    : 重置周期计数器，将当前周期号重新更改为0
返回值      : 无
参数        : 无
作者        : 何境泰
日期        : 2013年12月11日 13:57:33
*/
CIAPI_FUNC(void) CICycleInt_ResetCounter(void);
/*
 功能描述    : 是否是第一个周期
 返回值      : 如果是则返回CI_TRUE，否则返回CI_FALSE
 参数        : 无
 作者        : 张彦升
 日期        : 2014年7月18日 14:16:45
*/
CIAPI_FUNC(CI_BOOL) CICycleInt_IsFirstCycle(void);
/*
功能描述    : 联锁运算周期初始化
返回值      : 成功为0，失败为-1
参数        : 无
作者        : 张彦升
日期        : 2013年12月16日 14:09:01
*/
CIAPI_FUNC(int32_t) CICycleInt_Init(void);
/*
 功能描述    : 打开状态机输出
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月11日 15:53:40
*/
CIAPI_FUNC(int32_t) CICycleInt_OpenPrintFsm(void);

#ifdef __cplusplus
}
#endif

#endif /*_cycle_int_h__*/
