/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年10月15日 9:44:45
用途        : 通信状态判断
历史修改记录: v1.0    创建
**********************************************************************/

#ifndef _communicate_state_h__
#define _communicate_state_h__

#include "util/ci_header.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum 
{
    CS_NONE                             = 0x00,        /*通信正常状态*/
    CS_EEU_CHANNEL_A_INT                = 0x01,        /*电子单元a路中断状态*/
    CS_EEU_CHANNEL_B_INT                = 0x02,        /*电子单元b路中断状态*/
    CS_HMI_CHANNEL_A_INT                = 0x04,        /*控显机a路中断状态*/
    CS_HMI_CHANNEL_B_INT                = 0x08,        /*控显机b路中断状态*/
    CS_UART_CHANNEL_A_INT               = 0x10,        /*切换机a路中断状态*/
    CS_UART_CHANNEL_B_INT               = 0x20,        /*切换机b路中断状态*/
    CS_FIBER_CHANNEL_A_INT              = 0x40,        /*双系a路中断状态*/
    CS_FIBER_CHANNEL_B_INT              = 0x80,        /*双系b路中断状态*/
}CommState;

CIAPI_FUNC(int32_t) CICommunicateStateInt_Init(void);

CIAPI_FUNC(int32_t) CICommunicateStateInt_SetState(CommState state);

CIAPI_FUNC(int32_t) CICommunicateStateInt_RecoverState(CommState state);

#define CICommunicateStateInt_SetInfo(state,msg) do                       \
{                                                           \
    CICommunicateStateInt_SetInfoExt(state,msg,__FUNCTION__,__LINE__);\
} while (0);

#define CICommunicateStateInt_RecoverInfo(state,msg) do                       \
{                                                           \
    CICommunicateStateInt_RecoverInfoExt(state,msg,__FUNCTION__,__LINE__);\
} while (0);


CIAPI_FUNC(int32_t) CICommunicateStateInt_SetInfoExt(CommState state,
                                                     const char* p_msg,
                                                     const char* p_function_name,
                                                     int32_t line_num);

CIAPI_FUNC(int32_t) CICommunicateStateInt_RecoverInfoExt(CommState state,
                                                         const char* p_msg,
                                                         const char* p_function_name,
                                                         int32_t line_num);

#ifdef __cplusplus
}
#endif

#endif /*! _communicate_state_h__*/
