/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年10月24日 10:43:14
用途        : 同步数据类型定义
历史修改记录: v1.0    创建
**********************************************************************/

#ifndef _syn_type_h__
#define _syn_type_h__

#include "util/ci_header.h"

/*同步的类型定义*/
typedef enum
{
    ST_NEW_CYCLE        = 1,           /*新周期同步*/
    ST_INPUT            = 2,           /*输入数据同步*/
    ST_RESULT           = 3,           /*结果数据同步*/
    ST_CFG              = 4,           /*配置比较同步*/
    ST_HEARTBEAT        = 5,           /*心跳信号同步*/
}SynType;

#endif /*!_syn_type_h__*/
    