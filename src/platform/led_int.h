/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年10月15日 10:18:04
用途        : LED点灯
历史修改记录: v1.0    创建
**********************************************************************/

#ifndef _led_h__
#define _led_h__

#ifdef __cplusplus
extern "C" {
#endif
/*
 功能描述    : 点亮初始化成功指示灯
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月28日 16:26:01
*/
CIAPI_FUNC(int32_t) CILedInt_LightInit(void);
/*
 功能描述    : 熄灭初始化成功指示灯
 返回值      : 成功为0，失败为-1
 参数        : 误
 作者        : 张彦升
 日期        : 2014年4月28日 16:26:01
*/
CIAPI_FUNC(int32_t) CILedInt_BlankingInit(void);
/*
 功能描述    : 点亮主系状态指示灯
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月28日 16:26:01
*/
CIAPI_FUNC(int32_t) CILedInt_LightMaster(void);
/*
 功能描述    : 熄灭主系状态指示灯
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月28日 16:26:01
*/
CIAPI_FUNC(int32_t) CILedInt_BlankingMaster(void);
/*
 功能描述    : 点亮备系状态指示灯
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月28日 16:26:01
*/
CIAPI_FUNC(int32_t) CILedInt_LightStandby(void);
/*
 功能描述    : 熄灭备系状态指示灯
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月28日 16:26:01
*/
CIAPI_FUNC(int32_t) CILedInt_BlankingStandby(void);
/*
 功能描述    : 点亮故障显示灯
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月28日 16:26:01
*/
CIAPI_FUNC(int32_t) CILedInt_LightFailure(void);
/*
 功能描述    : 点亮故障显示灯
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月28日 16:26:01
*/
CIAPI_FUNC(int32_t) CILedInt_BlankingFailure(void);
/*
 功能描述    : 熄灭所有显示灯
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月28日 16:26:01
*/
CIAPI_FUNC(int32_t) CILedInt_BlankingAll(void);
/*
 功能描述    : LED灯初始化函数
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月28日 16:25:45
*/
CIAPI_FUNC(int32_t) CILedInt_Init(void);

#ifdef __cplusplus
}
#endif

#endif /*! _led_h__*/

