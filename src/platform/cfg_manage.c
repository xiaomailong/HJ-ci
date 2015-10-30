
/*********************************************************************
 Copyright (C), 2014,  Co.Hengjun, Ltd.

 作者        : 张彦升
 版本        : 1.0
 创建日期    : 2014年8月2日 18:32:34
 用途        : 配置比较数据管理
 历史修改记录: v1.0    创建
**********************************************************************/
#include "util/ci_header.h"

#ifdef LINUX_ENVRIONMENT

#include "cfg_manage.h"
#include "app.h"
#include "util/app_config.h"
#include "util/config.h"
#include "util/log.h"

static const char* p_station_name = NULL;
static unsigned char ci_md5[MD5_DIGEST_LENGTH];        /*CI程序的md5值*/
static unsigned char ilt_md5[MD5_DIGEST_LENGTH];       /*联锁表的md5值*/
static unsigned char snt_md5[MD5_DIGEST_LENGTH];       /*信号节点表的md5值*/
static unsigned char ssi_md5[MD5_DIGEST_LENGTH];
static unsigned char cfg_md5[MD5_DIGEST_LENGTH];       /*cfg的md5*/
static unsigned char ko_dpram_md5[MD5_DIGEST_LENGTH];  /*双口ram模块的md5*/
static unsigned char ko_fiber_md5[MD5_DIGEST_LENGTH];  /*光纤模块的md5*/
static unsigned char ko_uart_md5[MD5_DIGEST_LENGTH];   /*串口模块的md5*/
static unsigned char ko_can_md5[MD5_DIGEST_LENGTH];    /*can板模块的md5*/
static unsigned char ko_net_md5[MD5_DIGEST_LENGTH];    /*网口模块的md5*/
static unsigned char ko_wtd_md5[MD5_DIGEST_LENGTH];    /*看门狗模块的md5*/

/*
 功能描述    : 得到本项目程序的md5值
 返回值      : 无
 参数        : @md5_sum 长度为MD5_DIGEST_LENGTH的md5字符串
 作者        : 张彦升
 日期        : 2014年8月2日 18:34:58
*/
void CICfg_GetAppMd5(unsigned char md5_sum[MD5_DIGEST_LENGTH])
{
    int32_t i = 0;

    for (i = 0;i < MD5_DIGEST_LENGTH;i++)
    {
        md5_sum[i] = ci_md5[i];
    }

    return;
}
/*
 功能描述    : 得到所使用联锁表的md5值
 返回值      : 无
 参数        : @md5_sum 长度为MD5_DIGEST_LENGTH的md5字符串
 作者        : 张彦升
 日期        : 2014年8月2日 18:34:58
*/
void CICfg_GetIltMd5(unsigned char md5_sum[MD5_DIGEST_LENGTH])
{
    int32_t i = 0;

    for (i = 0;i < MD5_DIGEST_LENGTH;i++)
    {
        md5_sum[i] = ilt_md5[i];
    }
    return;
}
/*
 功能描述    : 得到所使用信号节点表的md5
 返回值      : 无
 参数        : @md5_sum 长度为MD5_DIGEST_LENGTH的md5字符串
 作者        : 张彦升
 日期        : 2014年8月2日 18:34:58
*/
void CICfg_GetSntMd5(unsigned char md5_sum[MD5_DIGEST_LENGTH])
{
    int32_t i = 0;

    for (i = 0;i < MD5_DIGEST_LENGTH;i++)
    {
        md5_sum[i] = snt_md5[i];
    }

    return;
}
/*
 功能描述    : 得到所SSI文件的md5
 返回值      : 无
 参数        : @md5_sum 长度为MD5_DIGEST_LENGTH的md5字符串
 作者        : 张彦升
 日期        : 2014年8月2日 18:34:58
*/
void CICfg_GetSsiMd5(unsigned char md5_sum[MD5_DIGEST_LENGTH])
{
    int32_t i = 0;

    for (i = 0;i < MD5_DIGEST_LENGTH;i++)
    {
        md5_sum[i] = ssi_md5[i];
    }
    return;
}

/*
功能描述    : 得到所cfg文件的md5
返回值      : 无
参数        : @md5_sum 长度为MD5_DIGEST_LENGTH的md5字符串
作者        : 张彦升
日期        : 2014年8月2日 18:34:58
*/
void CICfg_GetCfgMd5(unsigned char md5_sum[MD5_DIGEST_LENGTH])
{
    int32_t i = 0;

    for (i = 0;i < MD5_DIGEST_LENGTH;i++)
    {
        md5_sum[i] = cfg_md5[i];
    }
    return;
}

/*
 功能描述    : 得到双CPU之间双口ram驱动的md5
 返回值      : 无
 参数        : @md5_sum 长度为MD5_DIGEST_LENGTH的md5字符串
 作者        : 张彦升
 日期        : 2014年8月2日 18:34:58
*/
void CICfg_GetDpramKoMd5(unsigned char md5_sum[MD5_DIGEST_LENGTH])
{
    int32_t i = 0;

    for (i = 0;i < MD5_DIGEST_LENGTH;i++)
    {
        md5_sum[i] = ko_dpram_md5[i];
    }
    return;
}
/*
 功能描述    : 得到光纤驱动的md5
 返回值      : 无
 参数        : @md5_sum 长度为MD5_DIGEST_LENGTH的md5字符串
 作者        : 张彦升
 日期        : 2014年8月2日 18:34:58
*/
void CICfg_GetFiberKoMd5(unsigned char md5_sum[MD5_DIGEST_LENGTH])
{
    int32_t i = 0;

    for (i = 0;i < MD5_DIGEST_LENGTH;i++)
    {
        md5_sum[i] = ko_fiber_md5[i];
    }
    return;
}
/*
 功能描述    : 得到网络驱动的md5
 返回值      : 无
 参数        : @md5_sum 长度为MD5_DIGEST_LENGTH的md5字符串
 作者        : 张彦升
 日期        : 2014年8月2日 18:34:58
*/
void CICfg_GetNetKoMd5(unsigned char md5_sum[MD5_DIGEST_LENGTH])
{
    int32_t i = 0;

    for (i = 0;i < MD5_DIGEST_LENGTH;i++)
    {
        md5_sum[i] = ko_net_md5[i];
    }
    return;
}
/*
 功能描述    : 得到can驱动的md5
 返回值      : 无
 参数        : @md5_sum 长度为MD5_DIGEST_LENGTH的md5字符串
 作者        : 张彦升
 日期        : 2014年8月2日 18:34:58
*/
void CICfg_GetCanKoMd5(unsigned char md5_sum[MD5_DIGEST_LENGTH])
{
    int32_t i = 0;

    for (i = 0;i < MD5_DIGEST_LENGTH;i++)
    {
        md5_sum[i] = ko_can_md5[i];
    }
    return;
}
/*
 功能描述    : 得到can驱动的md5
 返回值      : 无
 参数        : @md5_sum 长度为MD5_DIGEST_LENGTH的md5字符串
 作者        : 张彦升
 日期        : 2014年8月2日 18:34:58
*/
void CICfg_GetUartKoMd5(unsigned char md5_sum[MD5_DIGEST_LENGTH])
{
    int32_t i = 0;

    for (i = 0;i < MD5_DIGEST_LENGTH;i++)
    {
        md5_sum[i] = ko_uart_md5[i];
    }

    return;
}
/*
 功能描述    : 得到看门狗驱动的md5
 返回值      : 无
 参数        : @md5_sum 长度为MD5_DIGEST_LENGTH的md5字符串
 作者        : 张彦升
 日期        : 2014年8月2日 18:34:58
*/
void CICfg_GetWtdKoMd5(unsigned char md5_sum[MD5_DIGEST_LENGTH])
{
    int32_t i = 0;

    for (i = 0;i < MD5_DIGEST_LENGTH;i++)
    {
        md5_sum[i] = ko_wtd_md5[i];
    }
    return;
}
/*
 功能描述    : 得到station名称
 返回值      : 无
 参数        : @md5_sum 长度为MD5_DIGEST_LENGTH的md5字符串
 作者        : 张彦升
 日期        : 2014年8月2日 18:34:58
*/
const char* CICfg_StationName(void)
{
    return p_station_name;
}
/*
 功能描述    : 初始化配置相关数据
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年8月2日 18:33:54
*/
int32_t CICfg_Init(void)
{
    char setting_name[FILE_PATH_SIZE + STATION_SETTING_NAME_SIZE] = SETTINGS_PATH;
    static CI_BOOL b_initialized = CI_FALSE;
    int32_t ret = 0;
    int off = 0;
    const char* path = NULL;

    if (CI_TRUE == b_initialized)
    {
        return -1;
    }

    p_station_name = CIConfig_GetValue("StationName");

    strcat(setting_name,p_station_name);
    off = strlen(setting_name);

    /*计算配置文件的md5*/
    strncpy(setting_name + off,".lsb",4);
    ret = CIAlgorithm_Md5(setting_name,ilt_md5);
    assert(-1 != ret);

    strncpy(setting_name + off,".jdb",4);
    ret = CIAlgorithm_Md5(setting_name,snt_md5);
    assert(-1 != ret);

    strncpy(setting_name + off,".syb",4);
    ret = CIAlgorithm_Md5(setting_name,ssi_md5);
    assert(-1 != ret);

    strncpy(setting_name + off,".cfg",4);
    ret = CIAlgorithm_Md5(setting_name,cfg_md5);
    assert(-1 != ret);

    ret = CIAlgorithm_Md5(CIApp_GetProgramName(),ci_md5);
    assert(-1 != ret);

    path = CIConfig_GetValue("CpuDpramDriverPath");
    if (NULL == path)
    {
        CILog_Msg("配置文件当中找不到CpuDpramDriverPath的配置");
        return -1;
    }
    ret = CIAlgorithm_Md5(path,ko_dpram_md5);
    assert(-1 != ret);

    path = CIConfig_GetValue("FiberDriverPath");
    if (NULL == path)
    {
        CILog_Msg("配置文件当中找不到FiberDriverPath的配置");
        return -1;
    }
    ret = CIAlgorithm_Md5(path,ko_fiber_md5);
    assert(-1 != ret);

    path = CIConfig_GetValue("NetDriverPath");
    if (NULL == path)
    {
        CILog_Msg("配置文件当中找不到NetDriverPath的配置");
        return -1;
    }
    ret = CIAlgorithm_Md5(path,ko_net_md5);
    assert(-1 != ret);

    path = CIConfig_GetValue("CanDriverPath");
    if (NULL == path)
    {
        CILog_Msg("配置文件当中找不到CanDriverPath的配置");
        return -1;
    }
    ret = CIAlgorithm_Md5(path,ko_can_md5);
    assert(-1 != ret);

    path = CIConfig_GetValue("UartDriverPath");
    if (NULL == path)
    {
        CILog_Msg("配置文件当中找不到UartDriverPath的配置");
        return -1;
    }
    ret = CIAlgorithm_Md5(path,ko_uart_md5);
    assert(-1 != ret);

    path = CIConfig_GetValue("WatchdogDriverPath");
    if (NULL == path)
    {
        CILog_Msg("配置文件当中找不到WatchdogDriverPath的配置");
        return -1;
    }
    ret = CIAlgorithm_Md5(path,ko_wtd_md5);
    assert(-1 != ret);

    b_initialized = CI_TRUE;

    return 0;
}

#endif /*!LINUX_ENVRIONMENT*/
