/*********************************************************************
 Copyright (C), 2014,  Co.Hengjun, Ltd.

 作者        : 张彦升
 版本        : 1.0
 创建日期    : 2014年8月2日 18:32:34
 用途        : 配置比较数据管理
 历史修改记录: v1.0    创建
**********************************************************************/

#ifndef _cfg_manage_h__
#define _cfg_manage_h__

#include "util/ci_header.h"
#include "util/algorithms.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 功能描述    : 得到本项目程序的md5值
 返回值      : 无
 参数        : @md5_sum 长度为MD5_DIGEST_LENGTH的md5字符串
 作者        : 张彦升
 日期        : 2014年8月2日 18:34:58
*/
CIAPI_FUNC(void) CICfg_GetAppMd5(unsigned char md5_sum[MD5_DIGEST_LENGTH]);
/*
 功能描述    : 得到所使用联锁表的md5值
 返回值      : 无
 参数        : @md5_sum 长度为MD5_DIGEST_LENGTH的md5字符串
 作者        : 张彦升
 日期        : 2014年8月2日 18:34:58
*/
CIAPI_FUNC(void) CICfg_GetIltMd5(unsigned char md5_sum[MD5_DIGEST_LENGTH]);
/*
 功能描述    : 得到所使用信号节点表的md5
 返回值      : 无
 参数        : @md5_sum 长度为MD5_DIGEST_LENGTH的md5字符串
 作者        : 张彦升
 日期        : 2014年8月2日 18:34:58
*/
CIAPI_FUNC(void) CICfg_GetSntMd5(unsigned char md5_sum[MD5_DIGEST_LENGTH]);
/*
 功能描述    : 得到所SSI文件的md5
 返回值      : 无
 参数        : @md5_sum 长度为MD5_DIGEST_LENGTH的md5字符串
 作者        : 张彦升
 日期        : 2014年8月2日 18:34:58
*/

//CICfg_GetCfgMd5
CIAPI_FUNC(void) CICfg_GetSsiMd5(unsigned char md5_sum[MD5_DIGEST_LENGTH]);

/*
功能描述    : 得到所SSI文件的md5
返回值      : 无
参数        : @md5_sum 长度为MD5_DIGEST_LENGTH的md5字符串
作者        : 张彦升
日期        : 2014年8月2日 18:34:58
*/

CIAPI_FUNC(void) CICfg_GetCfgMd5(unsigned char md5_sum[MD5_DIGEST_LENGTH]);

/*
 功能描述    : 得到双CPU之间双口ram驱动的md5
 返回值      : 无
 参数        : @md5_sum 长度为MD5_DIGEST_LENGTH的md5字符串
 作者        : 张彦升
 日期        : 2014年8月2日 18:34:58
*/
CIAPI_FUNC(void) CICfg_GetDpramKoMd5(unsigned char md5_sum[MD5_DIGEST_LENGTH]);
/*
 功能描述    : 得到光纤驱动的md5
 返回值      : 无
 参数        : @md5_sum 长度为MD5_DIGEST_LENGTH的md5字符串
 作者        : 张彦升
 日期        : 2014年8月2日 18:34:58
*/
CIAPI_FUNC(void) CICfg_GetFiberKoMd5(unsigned char md5_sum[MD5_DIGEST_LENGTH]);
/*
 功能描述    : 得到网络驱动的md5
 返回值      : 无
 参数        : @md5_sum 长度为MD5_DIGEST_LENGTH的md5字符串
 作者        : 张彦升
 日期        : 2014年8月2日 18:34:58
*/
CIAPI_FUNC(void) CICfg_GetNetKoMd5(unsigned char md5_sum[MD5_DIGEST_LENGTH]);
/*
 功能描述    : 得到can驱动的md5
 返回值      : 无
 参数        : @md5_sum 长度为MD5_DIGEST_LENGTH的md5字符串
 作者        : 张彦升
 日期        : 2014年8月2日 18:34:58
*/
CIAPI_FUNC(void) CICfg_GetCanKoMd5(unsigned char md5_sum[MD5_DIGEST_LENGTH]);
/*
 功能描述    : 得到can驱动的md5
 返回值      : 无
 参数        : @md5_sum 长度为MD5_DIGEST_LENGTH的md5字符串
 作者        : 张彦升
 日期        : 2014年8月2日 18:34:58
*/
CIAPI_FUNC(void) CICfg_GetUartKoMd5(unsigned char md5_sum[MD5_DIGEST_LENGTH]);
/*
 功能描述    : 得到看门狗驱动的md5
 返回值      : 无
 参数        : @md5_sum 长度为MD5_DIGEST_LENGTH的md5字符串
 作者        : 张彦升
 日期        : 2014年8月2日 18:34:58
*/
CIAPI_FUNC(void) CICfg_GetWtdKoMd5(unsigned char md5_sum[MD5_DIGEST_LENGTH]);
/*
 功能描述    : 得到station名称
 返回值      : 无
 参数        : @md5_sum 长度为MD5_DIGEST_LENGTH的md5字符串
 作者        : 张彦升
 日期        : 2014年8月2日 18:34:58
*/
CIAPI_FUNC(const char*) CICfg_StationName(void);
/*
 功能描述    : 初始化配置相关数据
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年8月2日 18:33:54
*/
CIAPI_FUNC(int32_t) CICfg_Init(void);

#ifdef __cplusplus
}
#endif

#endif /*!_cfg_manage_h__*/
