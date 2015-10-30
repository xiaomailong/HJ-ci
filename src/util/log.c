/*********************************************************************
Copyright (C), 2015,  Co.Hengjun, Ltd.

文件名      : log.c
版本        : 1.2
创建日期    : 2013年10月12日 10:04:04
用途        : 系统所输出的信息最终是要记录的
历史修改记录: 
    v1.2 张彦升 添加单元测试并修正测试出的部分bug
      在正常情况下，日志记录模型如下：
      interlocking      platform
           |               |
       Hmi_SendTips        |
         /          \      /
       HMI            log
                ----------------
            localLog  RemoteLog  Stdout
      上面的模型指出，联锁相关的信息都会向控显机发送，控显机发送的过程当中再
      调用log模块，由log模块决定是否将日志保存在本地、远程或打印在标准输出。
      其中标准输出默认是打开的，本地与远程需要注册

    v1.3 张彦升 精简远程日志信息，日志模型变成如下
      interlocking      platform
            |                |
           \|/               |
       Hmi_SendTips         \|/
         /          \      /
       HMI          RemoteLog -------> 电务维修机
                        |      
                        |
                    CILog_Msg
           -----------------------------
           | localLog  TailLog  Stdout |
           -----------------------------
      其中，调用Hmi_SendTips会即向控显机发送一份数据，又向RemoteLog发送一份数据。
      调用RemoteLog会即向电务维修机发送数据，也向CILog_Msg传递数据。
      上述这种模型确保发向控显机的数据可以被电务维修机记录，可以被TailLogger记录，
      可以在调试时保存到本地，也可以打印到标准输出。
**********************************************************************/

#ifdef WIN32
#   include <winsock2.h>
#   include <Windows.h>
#   pragma comment(lib, "Ws2_32.lib")
#else
#   include <syslog.h>
#   include <sys/time.h>
#endif /*WIN32*/

#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>

#include "ci_header.h"
#include "sig_str.h"

#include "config.h"
#include "log.h"

#define MAX_HANDLER 10      /*处理错误信息的文件最大个数*/
#define LOG_MAX_BUF 0XFFFF      /*为了避免发送数据溢出，使用最大存储空间*/

#define INCRESE_LEN(ret)        \
    if(ret > LOG_MAX_BUF - len) \
    {                           \
        return -1;              \
    }                           \
    else                        \
    {                           \
        len += ret;             \
    }
/*
 *TODO，有很多地方使用了LOG_MAX_BUF，它们的值不统一，造成发送的字符串长度也不一样
 *容易出现缓冲区溢出问题
 */

/*
 功能描述    : 向标准输出写入数据
 返回值      : 成功为0，失败为-1
 参数        : 
 日期        : 2014年7月3日 12:36:10
*/
static int32_t log_stdout_write(void* p_data,int32_t len);

static CILogHandler stdout_handle = {
    log_stdout_write,
    log_stdout_write,
    "stdout",
#ifdef WIN32
    CI_TRUE,            /*windows下默认打开，Linux下默认关闭*/
#else
    CI_FALSE,
#endif
};

/*默认使用标准输出*/
static CILogHandler const* log_handlers[MAX_HANDLER] = {&stdout_handle,NULL};
static int32_t log_handler_count = 1;

/*
 功能描述    : 向标准输出写入数据
 返回值      : 成功为0，失败为-1
 参数        : 
 日期        : 2014年7月3日 12:36:10
*/
static int32_t log_stdout_write(void* p_data,int32_t len)
{
#ifdef WIN32
    /*在windows下因为不牵扯到实时性问题，所以使用标准库即可*/
    fwrite(p_data,sizeof(char),len,stdout);
#else
    write(1,p_data,len);
#endif /*WIN32*/
    return 0;
}

/*
* Print a message and return to caller.
* Caller specifies "errnoflag".
*/
static int32_t log_err_doit(int errnoflag, int error, const char *fmt, va_list ap)
{
    static char buf[LOG_MAX_BUF] = {0};
    int32_t len = 0;
    int32_t i = 0;
    int32_t ret = 0;

    assert(NULL != fmt);

    memset(buf,0,LOG_MAX_BUF);

#ifdef WIN32
    /*Windows提供的该函数返回值为-1时表示被截断，这与linux的不同*/
    ret = vsnprintf(buf + len, LOG_MAX_BUF - len, fmt, ap);
    if (0 >= ret) 
    {
        return -1; 
    } 
    else
    {
        len += ret;
    }

    if (errnoflag)
    {
        ret = _snprintf(buf + len,LOG_MAX_BUF - len,":%d", error);
        if (0 >= ret) 
        {
            return -1; 
        } 
        else
        {
            len += ret;
        }
    }

#else
    struct timeval cur_time;

    gettimeofday(&cur_time,NULL);

    /*
     * The functions snprintf() and vsnprintf() do not write more than size bytes
     * (including the terminating null byte ('\0')). If the output was truncated 
     * due to this limit then the return value is the number of characters 
     * (excluding the terminating null byte) which would have been written to 
     * the final string if enough space had been available. Thus, a return value 
     * of size or more means that the output was truncated. (See also below under NOTES.)
     * snprintf函数返回的是实际被填装的字符串个数,当被截断时返回本应该被填充的字符个数
     **/
    ret = snprintf(buf + len,LOG_MAX_BUF - len,"[%d",(int)cur_time.tv_sec);
    INCRESE_LEN(ret)
    ret = snprintf(buf + len,LOG_MAX_BUF - len,".%06d]",(int)cur_time.tv_usec);
    INCRESE_LEN(ret)
    ret = vsnprintf(buf + len, LOG_MAX_BUF - len, fmt, ap);
    INCRESE_LEN(ret)

    /*如果还有空间存储的话，记录系统调用错误信息*/
    if (errnoflag && LOG_MAX_BUF != len)
    {
        ret = snprintf(buf + len, LOG_MAX_BUF - len, ":%s", strerror(error));
        INCRESE_LEN(ret)
    }

#endif /*WIN32*/
    if(LOG_MAX_BUF < len)
    {
        return -1;
    }
    else if (0 > len)
    {
        return len;
    }
    else
    {
        buf[len++] = '\n';
    }

    /*
     * 这里直接使用了write系统调用，我们不再考虑性能问题，如果这里改为printf函数，由于
     * printf为不可重入函数，在信号函数内部使用printf会出现数据丢失。
     */
    for (i = 0;i < log_handler_count;i++)
    {
        if (CI_TRUE == log_handlers[i]->b_use && NULL != log_handlers[i]->write)
        {
            log_handlers[i]->write(buf,len);
        }
    }

    return len;
}

#ifdef LINUX_ENVRIONMENT
/*
 功能描述    : 安全打印函数
 返回值      : 成功为0，失败为-1
 参数        : 
 日期        : 2014年4月18日 12:56:02
*/
static int32_t log_sig_err_doit(int errnoflag, int error, const char *fmt, va_list ap)
{
    static char buf[LOG_MAX_BUF] = {0};
    struct timeval cur_time;
    int32_t len = 0;
    int32_t i = 0;
    int32_t ret = 0;

    assert(NULL != fmt);

    memset(buf,0,LOG_MAX_BUF);

    gettimeofday(&cur_time,NULL);

    /*TODO*/
    ret = CISigStr_Snprintf(buf + len,LOG_MAX_BUF - len,"[%d.%06d]",
                                (int)cur_time.tv_sec,
                                (int)cur_time.tv_usec);
    INCRESE_LEN(ret)

    ret = CISigStr_Vsnprintf(buf + len,LOG_MAX_BUF - len, fmt, ap);
    INCRESE_LEN(ret)
    if (errnoflag)
    {
        /*TODO*/
        ret = CISigStr_Snprintf(buf + len,LOG_MAX_BUF - len, ":errno:%d", error);
        INCRESE_LEN(ret)
    }
    if(LOG_MAX_BUF < len)
    {
        return -1;
    }
    else if (0 > len)
    {
        return len;
    }
    else
    {
        buf[len++] = '\n';
    }

    /*
     * 这里直接使用了write系统调用，我们不再考虑性能问题，如果这里改为printf函数，由于
     * printf为不可重入函数，在信号函数内部使用printf会出现数据丢失。
     */
    for (i = 0;i < log_handler_count;i++)
    {
        if (CI_TRUE == log_handlers[i]->b_use && NULL != log_handlers[i]->write)
        {
            log_handlers[i]->sig_write(buf,len);
        }
    }

    return ret;
}

/*
功能描述    : 处理信号中断安全打印信息
返回值      : 无
参数        : 
日期        : 2013年10月12日 10:32:43
*/
int32_t CILog_SigMsg(const char *fmt, ...)
{
    int32_t ret = 0;
    va_list ap;

    if (NULL == fmt)
    {
        return -1;
    }

    va_start(ap, fmt);
    ret = log_sig_err_doit(0, 0, fmt, ap);
    va_end(ap);
    return ret;
}
/*
功能描述    : 处理信号中断安全输出信息
返回值      : 无
参数        : 
日期        : 2013年10月12日 10:32:43
*/
int32_t CILog_SigErrno(const char* fmt, ...)
{
    int32_t ret = 0;
    va_list ap;

    if (NULL == fmt)
    {
        return -1;
    }
    
    va_start(ap, fmt);
    log_sig_err_doit(1, errno, fmt, ap);
#ifdef WIN32
    ret = log_sig_err_doit(1, WSAGetLastError(), fmt, ap);
#else
    ret = log_sig_err_doit(1, errno, fmt, ap);
#endif /*WIN32*/

    va_end(ap);
    return ret;
}
#endif /*LINUX_ENVRIONMENT*/

/*
 功能描述    : 打开使用标准输出
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2014年9月11日 8:43:01
*/
int32_t CILog_OpenStdoutWrite(void)
{
    stdout_handle.b_use = CI_TRUE;

    return 0;
}
/*
 功能描述    : 关闭标准输出
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年3月12日 9:53:01
*/
int32_t CILog_CloseStdoutWrite(void)
{
    stdout_handle.b_use = CI_FALSE;

    return 0;
}
/*
 功能描述    : 向log注册要下发的处理者
 返回值      : 成功为0，失败为-1
 参数        : 
 日期        : 2014年1月6日 9:58:26
*/
int32_t CILog_RegistHandler(const CILogHandler* p_handler)
{
    int32_t i = 0;

    if (NULL == p_handler)
    {
        return -1;
    }

    for (i = 0; i < log_handler_count;i++)
    {
        /*already regist*/
        if (p_handler == log_handlers[i])
        {
            return 0;
        }
    }

    /*如果可容纳的数目已经达到最大，则不再添加*/
    if (MAX_HANDLER <= log_handler_count)
    {
        return -1;
    }

    log_handlers[log_handler_count++] = p_handler;

    return 0;
}

/*
功能描述    : 该函数只是打印消息，并不打印errno信息
返回值      : 无
参数        : 
日期        : 2013年10月12日 10:30:20
*/
int32_t CILog_Msg(const char *fmt, ...)
{
    int32_t ret = 0;
    va_list ap;

    if (NULL == fmt)
    {
        return -1;
    }
    va_start(ap, fmt);
    ret = log_err_doit(0, 0, fmt, ap);
    va_end(ap);

    return ret;
}
/*
功能描述    : 该函数会打印系统调用当中出现得到错误并返回
返回值      : 无
参数        : 
日期        : 2013年10月12日 10:32:43
*/
int32_t CILog_Errno(const char* fmt, ...)
{
    int ret = 0;
    va_list ap;

    if (NULL == fmt)
    {
        return -1;
    }
    va_start(ap, fmt);
#ifdef WIN32
    ret = log_err_doit(1, WSAGetLastError(), fmt, ap);
#else
    ret = log_err_doit(1, errno, fmt, ap);
#endif /*WIN32*/

    va_end(ap);

    return ret;
}
