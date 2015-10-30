/*********************************************************************
 Copyright (C), 2015,  Co.Hengjun, Ltd.

 版本        : 1.0
 创建日期    : 2015年3月25日
 历史修改记录: v1.0    创建
    v2.0  张彦升 20150421
          为能在系统宕机后在本地保留一份最后的日志信息，为该模块增加缓存
          并在系统terminate的时候将缓存的信息保存的本地。
          目前在本地记录日志的时候记录的数据量大小由系统退出时留在缓存里面
          的数据的大小决定，所以记录的数据有时候多有时候少
**********************************************************************/
#include <time.h>

#include "local_log.h"
#include "util/log.h"
#include "util/config.h"

#define LOCAL_LOG_FILE_NAME_SIZE 100
#define LOCAL_LOG_CACHE_SIZE (4096 * 10)

static char local_log_file_name[LOCAL_LOG_FILE_NAME_SIZE] = {0};

static char cache_buf[LOCAL_LOG_CACHE_SIZE];
static int32_t cache_size = 0;

/*
 功能描述    : 日志写入函数
 返回值      : 成功为0，失败为-1
 参数        : @data 写入数据的地址
              @len 写入数据的长度
 作者        : 张彦升
 日期        : 2014年7月3日 12:36:10
*/
static int32_t local_log_file_write(void* data,int32_t len);

#ifdef WIN32
/*Windows下使用标准库，在linux下由于标准库使用互斥锁的原因不可使用*/
static FILE* log_file_fp = NULL;
#else
static int32_t log_file_fd = 0;
#endif /* WIN32*/

static CILogHandler local_log_file_handle = {
    local_log_file_write,
    local_log_file_write,
    "local_log",
    CI_FALSE,
};
int32_t CILocalLog_CachePush(void)
{
#ifndef WIN32
    if(CI_TRUE == local_log_file_handle.b_use)
    {
        /*谨慎实现该函数，linux下write函数的实现不可调用标准库，只许直接调用系统调用*/
        write(log_file_fd,cache_buf,cache_size);
        cache_size = 0;
    }
    else
    {
        cache_size = 0;
    }
#endif /*!WIN32*/

    return 0;
}
/*
 功能描述    : 日志写入函数
 返回值      : 成功为0，失败为-1
 参数        : @data 写入数据的指针
              @len 写入数据的长度
 作者        : 张彦升
 日期        : 2014年7月3日 12:36:10
*/
static int32_t local_log_file_write(void* data,int32_t len)
{
    if(NULL == data || 0 > len)
    {
        return -1;
    }
    else if(0 == len)
    {
        return 0;
    }
#ifdef WIN32
    /*
     * 在windows下该函数可以不是直接Windows API，因为它不再考虑同步等问题，只是作为
     * 一个模拟的平台
     */
    fwrite(data,sizeof(char),len,log_file_fp);
#else
    if(LOCAL_LOG_CACHE_SIZE < cache_size + len)
    {
        CILocalLog_CachePush();
    }
    if(LOCAL_LOG_CACHE_SIZE < len)
    {
        memcpy(cache_buf,data,LOCAL_LOG_CACHE_SIZE);
        cache_size = LOCAL_LOG_CACHE_SIZE;
        CILocalLog_CachePush();
    }
    else
    {
        memcpy(cache_buf + cache_size,data,len);
        cache_size += len;
    }
#endif /* WIN32*/

    return 0;
}
/*
 功能描述    : 使用日志文件方式记录，默认是不使用的
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2014年7月3日 13:05:13
*/
int32_t CILocalLog_Open(void)
{
    local_log_file_handle.b_use = CI_TRUE;

    return 0;
}
/*
 功能描述    : 关闭本地日志记录
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2014年7月3日 13:05:13
*/
int32_t CILocalLog_Close(void)
{
    local_log_file_handle.b_use = CI_FALSE;

    return 0;
}
/*
 功能描述    : 打开使用日志文件记录方式
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月18日 10:56:29
*/
static int32_t local_log_file_open(void)
{
    const char* log_file_name = NULL;
    struct tm* tm = NULL;
    time_t cur_time;
    int32_t len = 0;

    /*得到日志文件名称*/
    log_file_name = CIConfig_GetValue("LocalLogFileName");

    if (NULL == log_file_name)
    {
        CILog_Msg("从配置文件当中找不到LocalLogFileName配置项");

        return -1;
    }
    if(strlen(log_file_name) == 0)
    {
        CILog_Msg("配置文件当中LocalLogFileName配置项为空");

        return -1;
    }

    cur_time = time(NULL);
    tm = localtime(&cur_time);

    /*windows下使用标准库，linux下使用系统调用*/
#ifdef WIN32
    len = _snprintf(local_log_file_name,LOCAL_LOG_FILE_NAME_SIZE,"%s_%04d%02d%02d_%02d%02d%02d.txt",
                    log_file_name,
                    tm->tm_year + 1900,
                    tm->tm_mon + 1,
                    tm->tm_mday,
                    tm->tm_hour,
                    tm->tm_min,
                    tm->tm_sec );

    if(0 > len)
    {
        CILog_Msg("生成配置文件名称字符串被截断，请检测文件名长度");
        local_log_file_name[LOCAL_LOG_FILE_NAME_SIZE - 1] = 0;
    }
    log_file_fp = fopen(local_log_file_name,"w+");

    if (NULL == log_file_fp)
    {
        CILog_Errno("打开%s文件失败",local_log_file_name);

        return -1;
    }
#else
    len = snprintf(local_log_file_name,LOCAL_LOG_FILE_NAME_SIZE,"%s_%04d%02d%02d_%02d%02d%02d",
                    log_file_name,
                    tm->tm_year + 1900,
                    tm->tm_mon + 1,
                    tm->tm_mday,
                    tm->tm_hour,
                    tm->tm_min,
                    tm->tm_sec );

    if(len > LOCAL_LOG_FILE_NAME_SIZE || 0 > len)
    {
        CILog_Msg("生成配置文件名称字符串被截断，请检测文件名长度");
        local_log_file_name[LOCAL_LOG_FILE_NAME_SIZE - 1] = 0;
    }

    log_file_fd = open(local_log_file_name,O_WRONLY | O_CREAT,S_IRUSR);

    if (-1 == log_file_fd)
    {
        CILog_Errno("打开%s文件失败",local_log_file_name);

        return -1;
    }

#endif /* WIN32*/
    local_log_file_handle.name = local_log_file_name;

    return 0;
}
/*
 功能描述    : 初始化
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年3月25日 11:24:21
*/
int32_t CILocalLog_Init(void)
{
    int32_t ret = 0;
    static CI_BOOL b_initialized = CI_FALSE;

#ifndef CI_UNIT_TEST
    if (CI_TRUE == b_initialized)
    {
        return -1;
    }

    if (CI_FALSE == local_log_file_handle.b_use)
    {
        return 0;
    }
#endif /*CI_UNIT_TEST*/

    ret = local_log_file_open();
    if(0 > ret)
    {
        return -1;
    }

    ret = CILog_RegistHandler(&local_log_file_handle);
    if(0 > ret)
    {
        return -1;
    }

    b_initialized = CI_TRUE;

    return 0;
}
