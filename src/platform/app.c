/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年10月12日 13:11:32
用途        : 管理本程序实例
历史修改记录: 
**********************************************************************/
#include "app.h"

static const char* p_program_name = NULL;

/*
 功能描述    : 设置程序名称
 返回值      : 无
 参数        : @p_tmp_program_name 程序文件名指针
 作者        : 张彦升
 日期        : 2014年4月11日 14:13:20
*/
void CIApp_SaveProgramName (const char* p_tmp_program_name)
{
    p_program_name = p_tmp_program_name;

    return;
}
/*
 功能描述    : 得到本程序的名称
 返回值      : 本项目程序的文件名指针
 参数        : 无
 作者        : 张彦升
 日期        : 2014年5月6日 11:12:29
*/
const char* CIApp_GetProgramName(void)
{
    return p_program_name;
}

