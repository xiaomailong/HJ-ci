/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年10月12日 13:16:55
用途        : 管理信号
历史修改记录: 
**********************************************************************/
#include "util/ci_header.h"

#ifdef LINUX_ENVRIONMENT

#include "sig.h"
#include "app.h"
#include "timer.h"

#include "util/app_config.h"
#include "util/log.h"

/*
功能描述    : 得到信号处理函数，该函数进行了一部分的包装
返回值      : 
参数        : 
作者        : 张彦升
日期        : 2013年10月12日 13:30:43
*/
sighandler_t CISig_Get(int sig)
{
    struct sigaction context;

    if (sigaction(sig, NULL, &context) == -1)
    {
        return SIG_ERR;
    }
    return context.sa_handler;
}
/*
功能描述    : 为指定信号设置触发调用函数
返回值      : 
参数        : 
作者        : 张彦升
日期        : 2013年10月12日 14:09:14
*/
sighandler_t CISsig_Set(int sig, sighandler_t handler)
{
    struct sigaction context, ocontext;

    context.sa_handler = handler;
    sigemptyset(&context.sa_mask);
    context.sa_flags = 0;

    if (sigaction(sig, &context, &ocontext) == -1)
    {
        return SIG_ERR;
    }
    return ocontext.sa_handler;
}
/*
功能描述    : 复位信号处理方式
返回值      : 
参数        : 
作者        : 张彦升
日期        : 2013年10月12日 14:11:16
*/
int32_t CISig_Restore(void)
{
#ifndef _DEBUG

#ifdef SIGPIPE
    CISsig_Set(SIGPIPE, SIG_DFL);
#endif

#ifdef SIGINT
    CISsig_Set(SIGINT, SIG_DFL);        /*中断键被按下*/
#endif

#ifdef SIGQUIT
    CISsig_Set(SIGQUIT, SIG_DFL);       /*停止键被按下*/
#endif

#endif /*!_DEBUG*/

    return 0;
}
/*
 功能描述    : 阻塞本系统当中使用的信号
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2013年12月16日 14:52:52
*/
int32_t CISig_Block(void)
{
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIG_CI_TIMER);
    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
    {
        CILog_Errno("阻塞信号失败");
        return -1;
    }

    return 0;
}
/*
 功能描述    : 取消阻塞的信号
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2013年12月16日 14:52:40
*/
int32_t CISig_UnBlock(void)
{
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIG_CI_TIMER);
    if (-1 == sigprocmask(SIG_UNBLOCK, &mask, NULL))
    {
        CILog_Errno("取消阻塞信号失败");
        return -1;
    }

    return 0;
}

/*
功能描述    : 本软件应将大部分不安全的信号全部屏蔽，以免意外使程序异常终止，但是保留了
             TERM信号，方便调试
返回值      : 无
参数        : 
作者        : 张彦升
日期        : 2013年10月12日 13:51:32
*/
int32_t CISig_Init(void)
{
    static CI_BOOL b_initialized = CI_FALSE;
    struct sigaction sa;

    if (CI_TRUE == b_initialized)
    {
        return -1;
    }

    /*初始化自定义信号*/
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = CITimer_GetHandler();
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIG_CI_TIMER, &sa, NULL) == -1)
    {
        CILog_Errno("初始化SIG_CI_TIMER信号失败");
        return -1;
    }

    b_initialized = CI_TRUE;

    return 0;
}

#endif /*!LINUX_ENVRIONMENT*/
