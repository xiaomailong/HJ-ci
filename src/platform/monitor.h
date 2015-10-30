/*********************************************************************
 Copyright (C), 2015,  Co.Hengjun, Ltd.

 版本        : 1.0
 创建日期    : 2015年3月27日
              该模块用来向检测机发送数据
 历史修改记录: v1.0    创建
**********************************************************************/

#ifndef _monitor_h__
#define _monitor_h__
 
#include "util/ci_header.h"

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus*/

/*
 功能描述    : 检测机发送数据。
            注意，这个函数本来应该定义为私有的，20150330王伟龙拒绝使用新定检测协议
            而改用套用控显机逻辑处理，所以暂时暴漏出来
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年3月30日 11:57:28
*/
CIAPI_FUNC(int32_t) CIMonitor_Write(const void* buf,int32_t data_len);
/*
 功能描述    : 初始化检测机通信
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年3月27日 12:34:53
*/
CIAPI_FUNC(int32_t) CIMonitor_Init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus*/

#endif /*!_monitor_h__*/
