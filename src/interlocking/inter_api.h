/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年10月17日 20:26:56
用途        : 联锁运算程序运行入口
历史修改记录: v1.0    创建
**********************************************************************/

#ifndef _inter_api_h__
#define _inter_api_h__

#ifdef __cplusplus
extern "C"{
#endif /*__cplusplus*/

#include "util/ci_header.h"
#include "global_data.h"

/*
 功能描述    : 设置节点状态
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2013年12月17日 9:30:47
*/
CIAPI_FUNC(void) CINode_SetState(node_t node_index, uint32_t status);
/*
 功能描述    : 获取节点数据
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2013年12月17日 9:45:38
*/
CIAPI_FUNC(uint16_t) CINode_GetData(node_t node_index);
/*
 功能描述    : 平台层要使用运算层当中使用的button信息，通过此函数获得
 返回值      : 记录的当前按下的按钮pressed_button
 参数        : 
 作者        : 何境泰
 日期        : 2014年6月20日 12:40:09
*/
CIAPI_FUNC(int16_t) CICommand_GetButton(int16_t *button1,
                                        int16_t *button2, 
                                        int16_t *button3,
                                        int16_t *button4,
                                        int16_t *button5);
/*
 功能描述    : 对按钮信息进行更改
 返回值      : 记录的当前按下的按钮pressed_button
 参数        : 
 作者        : 何境泰
 日期        : 2014年6月20日 12:40:09
*/
CIAPI_FUNC(int16_t) CICommand_SetButton(int16_t button1,
                                        int16_t button2, 
                                        int16_t button3,
                                        int16_t button4,
                                        int16_t button5);

CIAPI_FUNC(int32_t) CIInter_ClearSection(void);

/****************************************************
功能描述    : 将禁止信号清空，防止每次都发送该命令，
             放在这里比较符合逻辑分层的思想。
返回值      : 
作者        : 何境泰
日期        : 2014年4月16日 14:59:20
****************************************************/
extern CIAPI_FUNC(int32_t) CIInter_ClearCommands(void);

/*
 功能描述    : 判断是否为道岔
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年6月27日 11:13:34
*/
CIAPI_FUNC(CI_BOOL) CIInter_IsSwitch(int16_t node_index);
/*
 功能描述    : 判断是否为道岔
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年6月27日 11:13:34
*/
CIAPI_FUNC(CI_BOOL) CIInter_IsSignal(int16_t node_index);

CIAPI_FUNC(CI_BOOL) CIInter_IsSection(int16_t node_index);

CIAPI_FUNC(void) CIInter_InputNodeState(uint8_t ga,uint8_t ea,uint32_t state);
/*
 功能描述    : 进行联锁运算
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2013年12月20日 13:42:40
*/
CIAPI_FUNC(int32_t) CIInter_Calculate(void);
/*
功能描述    : 联锁运算层的数据初始化
返回值      : 成功为0，失败为-1
参数        : 
作者        : 张彦升
日期        : 2013年12月6日 9:49:43
*/
CIAPI_FUNC(int32_t) CIInter_Init(void);

/*
功能描述     : 故障—安全处理
返回值       : 
参数         : 
作者         : 何境泰
日期         : 2014年7月22日 9:53:46
*/
CIAPI_FUNC(void) CIInter_DefaultSafeProcess(uint16_t ga,uint16_t ea);

#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /*_inter_api_h__*/
