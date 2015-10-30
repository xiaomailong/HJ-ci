/*********************************************************************
 Copyright (C), 2014,  Co.Hengjun, Ltd.
 
 作者        : 张彦升
 版本        : 1.0
 创建日期    : 2014年8月27日
 历史修改记录:
    v1.0    创建
    v2.0    张彦升 修正向日志记录程序输出的数据量大时会导致程序退出的bug
            该模块不完整的一点是当数据过多时会使用截断发送，肯能发送的时间不完整
**********************************************************************/

#include "remote_log.h"

#ifdef LINUX_ENVRIONMENT

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "util/log.h"
#include "util/config.h"
#include "util/utility.h"
#include "util/sig_str.h"
#include "series_manage.h"
#include "cpu_manage.h"

/*cat /proc/sys/net/ipv4/tcp_wmem*/
/*这里主意一个很重要的问题：每次向日志记录服务器发送的数据最大量是在电子单元未
 * 启动的时候，这个时候，日志记录程序汇报的最大长度为32*16*16=4096个字节
 * 其中32为网关的数量，16为每个网关模块的数量，16为格式01-01FF000000001的长度
 * 实际当中网关的数量只能有8个，所以实际最大值应为32*8*16=4096，但测试时我们
 * 应该按照最大容量来测
 * 另外发送的最大数据量应该由远端服务器的接收缓存所决定，已经测试当BUF_LEN的
 * 长度设置为4096 * 4时远端服务器便丢包严重无法工作
 * 长度设置为4096时远端服务器也存在丢包情况，表现行为为重新建了一个session
 * */
#define SYSTEM_STATE_BUF_LEN 50
#define SOCKET_DEFAULT_BUF_LEN (4096 - SYSTEM_STATE_BUF_LEN)

#define REMOTE_LOG_MAX_BUF 1024

static int remote_log_port = 0;                /*端口地址*/

static int32_t remote_log_sock = 0;            /*向sremote_log记录机发送数据的文件句柄*/

static uint8_t write_buf[SOCKET_DEFAULT_BUF_LEN + SYSTEM_STATE_BUF_LEN] = {0};
static int32_t write_data_len = 0;

static struct sockaddr_in remote_log_addr;

/*
 功能描述    : 将缓存写出
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年4月7日 12:50:21
*/
static int32_t remote_log_write_cache_push(void)
{
    static char system_state_buf[SYSTEM_STATE_BUF_LEN] = {0};
    int32_t system_state_buf_len = 0;
    struct timeval cur_time;

    gettimeofday(&cur_time,NULL);

    if (0 == write_data_len)
    {
        return 0;
    }
    assert(SOCKET_DEFAULT_BUF_LEN >= write_data_len);

    memset(system_state_buf,0,SYSTEM_STATE_BUF_LEN);

    /*将系统状态补在每个remote_log信息的后面发送出去*/
    system_state_buf_len = snprintf(system_state_buf,SYSTEM_STATE_BUF_LEN,"[%d.%06d]system_state:%d:%d",
                                   (int)cur_time.tv_sec,(int)cur_time.tv_usec,
                                    CISeries_GetLocalState(),CICpu_GetLocalState());

    assert(system_state_buf_len < SYSTEM_STATE_BUF_LEN);

    memcpy(write_buf + write_data_len,system_state_buf,system_state_buf_len);
    write_data_len += system_state_buf_len;

    sendto(remote_log_sock,write_buf,write_data_len,0,(struct sockaddr *)&remote_log_addr,sizeof(remote_log_addr));

    memset(write_buf,0,SOCKET_DEFAULT_BUF_LEN + SYSTEM_STATE_BUF_LEN);
    write_data_len = 0;

    return 0;
}
/*
 功能描述    : 将网络的sendto函数封装一次，从而节省带宽
              注意，该函数不是安全函数，心跳信号的中断如果在执行此函数时到来则会出问题
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年7月5日 22:11:39
*/
static int32_t remote_log_write(void* p_data,int32_t len)
{
    if (NULL == p_data || 0 > len)
    {
        return -1;
    }
    if (0 == len)
    {
        return 0;
    }
    /*范围超过*/
    if (SOCKET_DEFAULT_BUF_LEN < write_data_len + len)
    {
        remote_log_write_cache_push();
        /*ensure write_data_len be set to 0*/
        write_data_len = 0;
    }

    /*本次发送的数据比发送缓冲大，改为截断发送*/
    if(SOCKET_DEFAULT_BUF_LEN < len)
    {
        memcpy(write_buf,p_data,SOCKET_DEFAULT_BUF_LEN);
        write_data_len = SOCKET_DEFAULT_BUF_LEN;
        remote_log_write_cache_push();
    }
    else
    {
        memcpy(write_buf + write_data_len,p_data,len);
        write_data_len += len;
    }

    return 0;
}
/*
 功能描述    : 将所有缓存的数据全部发送出去
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年9月18日 14:21:03
*/
int32_t CIRemoteLog_CachePush(void)
{
    int32_t ret = 0;

    ret = remote_log_write_cache_push();
    if (0 > ret)
    {
        return -1;
    }

    return 0;
}
/*
 功能描述    : 向远程日志记录机发送
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年6月5日 15:54:57
*/
int32_t CIRemoteLog_Write(const char* fmt,...)
{
    static char buf[REMOTE_LOG_MAX_BUF] = {0};
    static char buf2[REMOTE_LOG_MAX_BUF] = {0};
    int32_t len = 0;
    va_list ap;
    struct timeval cur_time;

    assert(NULL != fmt);

    memset(buf,0,REMOTE_LOG_MAX_BUF);

    va_start(ap, fmt);
    len = vsnprintf(buf, REMOTE_LOG_MAX_BUF, fmt, ap);
    /*当长度超过时，改为截断发送*/
    if(len > REMOTE_LOG_MAX_BUF)
    {
        len = REMOTE_LOG_MAX_BUF;
        buf[len - 1] = 0;
    }

    va_end(ap);

    /*向CILog_Msg层传递*/
    CILog_Msg("%s",buf);

    gettimeofday(&cur_time,NULL);

    len = snprintf(buf2,REMOTE_LOG_MAX_BUF,"[%d.%06d]%s",
        (int)cur_time.tv_sec,(int)cur_time.tv_usec,buf);

    if(len > REMOTE_LOG_MAX_BUF)
    {
        len = REMOTE_LOG_MAX_BUF;
        buf[len - 1] = 0;
    }
    /*通过remotelog发送*/
    remote_log_write(buf2,len);

    return 0;
}
/*
 功能描述    : socket初始化
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年7月3日 13:41:05
*/
static int32_t remote_log_sock_init(void)
{
    const char* remote_log_ip_str = NULL;
    const char* remote_log_port_str = NULL;
#if 0
    int32_t broadcast = 1;
#endif

    /*get remote log ip*/
    remote_log_ip_str = CIConfig_GetValue("RemoteLogServerIp");
    if (NULL == remote_log_ip_str)
    {
        CILog_Msg("配置文件当中找不到RemoteLogServerIp项");
        return -1;
    }
    if(CI_FALSE == CI_ValidateIpAddress(remote_log_ip_str))
    {
        CILog_Msg("配置文件当中RemoteLogServerIp项不符合规范");
        return -1;
    }
    /*get remote log port*/
    remote_log_port_str = CIConfig_GetValue("RemoteLogServerPort");
    if (NULL == remote_log_port_str)
    {
        CILog_Msg("配置文件当中找不到RemoteLogServerPort项");
        return -1;
    }
    remote_log_port = atoi(remote_log_port_str);
    if(-1 == remote_log_port)
    {
        CILog_Msg("配置文件当中RemoteLogServerPort项应该是数字");
        return -1;
    }
    if(CI_FALSE == CI_ValidatePort(remote_log_port))
    {
        CILog_Msg("配置文件当中RemoteLogServerPort项不符合规范");
        return -1;
    }

    remote_log_sock = socket(AF_INET,SOCK_DGRAM,0);
    if (-1 == remote_log_sock)
    {
        CILog_Errno("socket remote_log_fd failed");

        return -1;
    }

#if 0 /*如果使用广播请使用这一段代码*/
    if (-1 == setsockopt(remote_log_sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)))
    {
        CILog_Errno("设置remote_log为广播失败");
        return -1;
    }
#endif

    remote_log_addr.sin_family = AF_INET;
    remote_log_addr.sin_addr.s_addr = inet_addr(remote_log_ip_str);
    remote_log_addr.sin_port = htons(remote_log_port);

    return 0;
}
/*
 功能描述    : 初始化
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年8月27日 12:58:46
*/
int32_t CIRemoteLog_Init(void)
{
    int32_t ret = 0;

    static CI_BOOL b_initialized = CI_FALSE;

    /*在单元测试的时候为了能够多次测试所以屏蔽该项*/
#ifndef CI_UNIT_TEST
    if (CI_TRUE == b_initialized)
    {
        return -1;
    }
#endif /*CI_UNIT_TEST*/

    ret = remote_log_sock_init();
    if(0 > ret)
    {
        return -1;
    }

    b_initialized = CI_TRUE;

    return 0;
}

#else
/*
 功能描述    : 初始化
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年8月27日 12:58:46
*/
int32_t CIRemoteLog_Init(void)
{
    return 0;
}
/*
 功能描述    : 向远程日志记录机发送
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年6月5日 15:54:57
*/
int32_t CIRemoteLog_Write(const char* fmt, ...)
{
    return 0;
}

#endif /*LINUX_ENVRIONMENT*/
