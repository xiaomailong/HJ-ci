/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年10月12日 13:11:36
用途        : 
历史修改记录: 
**********************************************************************/

#ifndef _app_h__
#define _app_h__

#include "util/ci_header.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 关于工程项目操作入口的定义
 */
typedef struct _AppOps
{
    int32_t (*init) (void);
    void    (*run) (void);
    void    (*terminate) (void);
    int32_t (*decode_arguments) (int argc, char **argv);
}CIAppOps;
/*
 功能描述    : 保存当前程序的文件路径
 返回值      : 无
 参数        : @p_tmp_program_name 程序名称指针
 作者        : 张彦升
 日期        : 2014年5月6日 11:04:48
*/
CIAPI_FUNC(void) CIApp_SaveProgramName(const char* p_tmp_program_name);
/*
 功能描述    : 得到当前程序的文件名称
 返回值      : 本程序的文件名名称指针
 参数        : 无
 作者        : 张彦升
 日期        : 2014年5月6日 11:07:48
*/
CIAPI_FUNC(const char*) CIApp_GetProgramName(void);

#ifdef __cplusplus
}
#endif

#endif /*!_app_h__*/
