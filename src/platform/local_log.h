/*********************************************************************
 Copyright (C), 2015,  Co.Hengjun, Ltd.

 版本        : 1.0
 创建日期    : 2015年3月25日
 历史修改记录: v1.0    创建
**********************************************************************/

#ifndef _local_log_h__
#define _local_log_h__
 
#include "util/ci_header.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 功能描述    : 使用日志文件方式记录，默认是不使用的
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2014年7月3日 13:05:13
*/
CIAPI_FUNC(int32_t) CILocalLog_Open(void);
/*
 功能描述    : 关闭本地日志记录
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2014年7月3日 13:05:13
*/
CIAPI_FUNC(int32_t) CILocalLog_Close(void);

CIAPI_FUNC(int32_t) CILocalLog_CachePush(void);
/*
 功能描述    : 初始化
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年3月25日 11:24:21
*/
CIAPI_FUNC(int32_t) CILocalLog_Init(void);

#ifdef __cplusplus
}
#endif

#endif /*!_local_log_h__*/
