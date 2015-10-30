/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

文件名      : log.h
版本        : 1.0
创建日期    : 2013年10月12日 9:58:57
用途        : 系统的错误管理模块，该模块的思想来源于APUE
历史修改记录: 
**********************************************************************/
#ifndef _log_h__
#define _log_h__

#include "ci_header.h"

typedef struct _CILogHandler 
{
    /*caution!务必在linux下write函数的实现不可调用标准库，只许直接调用系统调用*/
    int32_t (*write) (void* data,int32_t len);
    int32_t (*sig_write) (void* data,int32_t len);
    const char* name;
    CI_BOOL b_use;          /*使用标志*/
} CILogHandler;

#ifdef LINUX_ENVRIONMENT

/*
功能描述    : 处理信号中断安全输出信息
返回值      : 无
参数        : 
日期        : 2013年10月12日 10:32:43
*/
CIAPI_FUNC(int32_t) CILog_SigErrno(const char *, ...);
/*
功能描述    : 处理信号中断安全打印信息
返回值      : 无
参数        : 
日期        : 2013年10月12日 10:32:43
*/
CIAPI_FUNC(int32_t) CILog_SigMsg(const char *, ...);

#endif /*LINUX_ENVRIONMENT*/

/*
 功能描述    : 打开使用标准输出
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2014年9月11日 8:43:01
*/
CIAPI_FUNC(int32_t) CILog_OpenStdoutWrite(void);
/*
 功能描述    : 关闭标准输出
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年3月12日 9:53:01
*/
CIAPI_FUNC(int32_t) CILog_CloseStdoutWrite(void);
/*
 功能描述    : 向log注册要下发的处理者
 返回值      : 成功为0，失败为-1
 参数        : 
 日期        : 2014年1月6日 9:58:26
*/
CIAPI_FUNC(int32_t) CILog_RegistHandler(const CILogHandler* p_handler);
/*
功能描述    : 该函数只是打印错误消息
返回值      : 无
参数        : 
日期        : 2013年10月12日 10:30:20
*/
CIAPI_FUNC(int32_t) CILog_Msg(const char *, ...);
/*
功能描述    : 该函数会打印系统调用当中出现得到错误并返回
返回值      : 无
参数        : 
日期        : 2013年10月12日 10:32:43
*/
CIAPI_FUNC(int32_t) CILog_Errno(const char *, ...);

#endif /*!_log_h__*/
