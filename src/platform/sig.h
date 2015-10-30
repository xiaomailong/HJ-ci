/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年10月12日 13:16:31
用途        : 管理信号
历史修改记录: 
**********************************************************************/

#ifndef _sig_h__
#define _sig_h__

#include "util/ci_header.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*sighandler_t)(int);

/*
功能描述    : 得到信号处理函数，该函数进行了一部分的包装
返回值      : 
参数        : 
作者        : 张彦升
日期        : 2013年10月12日 13:30:43
*/
CIAPI_FUNC(sighandler_t) CISig_Get(int sig);
/*
功能描述    : 为指定信号设置触发调用函数
返回值      : 
参数        : 
作者        : 张彦升
日期        : 2013年10月12日 14:09:14
*/
CIAPI_FUNC(sighandler_t) CISsig_Set(int sig, sighandler_t handler);
/*
 功能描述    : 将信号处理函数复位成默认
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2013年12月16日 14:53:45
*/
CIAPI_FUNC(int32_t) CISig_Restore(void);
/*
 功能描述    : 阻塞本系统当中使用到的信号SIG_CI_TIMER
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2013年12月16日 14:51:10
*/
CIAPI_FUNC(int32_t) CISig_Block(void);
/*
 功能描述    : 取消已经阻塞的信号SIG_CI_TIMER
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2013年12月16日 14:51:45
*/
CIAPI_FUNC(int32_t) CISig_UnBlock(void);
/*
 功能描述    : 初始化信号信号管理模块
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2013年12月16日 14:48:47
*/
CIAPI_FUNC(int32_t) CISig_Init(void);

#ifdef __cplusplus
}
#endif

#endif /*!_sig_h__*/
