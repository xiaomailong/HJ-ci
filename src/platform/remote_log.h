/*********************************************************************
 Copyright (C), 2014,  Co.Hengjun, Ltd.
 
 作者        : 张彦升
 版本        : 1.0
 创建日期    : 2014年8月27日
 历史修改记录: v1.0    创建
**********************************************************************/

#ifndef _remote_log_h__
#define _remote_log_h__
 
#include "util/ci_header.h"

#ifdef __cplusplus
extern "C" {
#endif
/*
 功能描述    : 将所有缓存的数据全部发送出去
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年9月18日 14:21:03
*/
CIAPI_FUNC(int32_t) CIRemoteLog_CachePush(void);
/*
 功能描述    : 向远程日志记录机发送
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年6月5日 15:54:57
*/
CIAPI_FUNC(int32_t) CIRemoteLog_Write(const char* fmt,...);
/*
 功能描述    : 初始化
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年8月27日 12:58:46
*/
CIAPI_FUNC(int32_t) CIRemoteLog_Init(void);

#ifdef __cplusplus
}
#endif

#endif /*!_remote_log_h__*/
