/*********************************************************************
 Copyright (C), 2015,  Co.Hengjun, Ltd.

 版本        : 1.0
 创建日期    : 2015年5月27日
 历史修改记录: v1.0    创建
**********************************************************************/

#ifndef _tail_log_h__
#define _tail_log_h__
 
#include "util/ci_header.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 功能描述    : 初始化
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年5月27日 13:37:06
*/
CIAPI_FUNC(int32_t) CITailLog_Init(void);


#ifdef __cplusplus
}
#endif

#endif /*!_tail_log_h__*/