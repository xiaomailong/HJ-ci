/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年10月15日 9:47:30
用途        : 故障检测
历史修改记录: v1.0    创建
**********************************************************************/

#ifndef _failures_int_h__
#define _failures_int_h__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _FailureType
{
    FAILURE_NONE                        = 1,       /*无故障*/
    FAILURE_SERIOUS                     = 2,       /*严重故障*/
    FAILURE_MANUL                       = 3,       /*需人工解决故障*/
    FAILURE_INTERRUPT                   = 4,       /*系统中断*/
    FAILURE_SYNRESULT                   = 6,       /*同步输出结果错误*/
    FAILURE_STANDBY_SERIES_SYN_REQUEST  = 8,       /*备系请求同步错误*/
    FAILURE_DUAL_MASTER_SERIES          = 9,       /*双主系严重错误*/
    FAILURE_DUAL_STANDBY_SERIES         = 10,      /*双备系严重错误*/
    FAILURE_DUAL_MASTER_CPU             = 11,      /*双主CPU故障*/
    FAILURE_DUAL_SLAVE_CPU              = 12,      /*双从CPU故障*/
    FAILURE_CPU_INPUT_CMP_FAIL          = 13,      /*双CPU输入比较错误*/
    FAILURE_SERIES_INPUT_CMP_FAIL       = 14,      /*双系输入比较错误*/
    FAILURE_HMI_CHANNEL_HALT            = 16,      /*控显机通信信道断开连接*/
    FAILURE_EEU_CHANNEL_HALT            = 17,      /*电子单元通信信道断开连接*/
    FAILURE_DUAL_SERIES_CHANNEL_HALT    = 18,      /*双系通信信道断开连接*/
    FAILURE_DUAL_CPU_CHANNEL_HALT       = 19,      /*双CPU通信信道断开连接*/
    FAILURE_DUAL_SERIES_SWITCH_HALT     = 20,      /*双系切换版通信信道断开连接*/
    FAILURE_SWITCH_COMMUNICATE_DATA     = 21,      /*切换版通信故障*/
    FAILURE_EEU_STATUS_FAULT            = 22,      /*电子单元状态错误故障*/
    FAILURE_SERIES_HEARTBEAT_LOST       = 23,      /*双系心跳信号丢失*/
    FAILURE_SWITCH_BOARD_COMMUNICATE    = 24,      /*双系切换板通信错误*/
}FailureType;

#define CIFailureInt_SetInfo(type,msg) do                       \
    {                                                           \
        CIFailureInt_SetInfoExt(type,msg,__FUNCTION__,__LINE__);\
    } while (0);

CIAPI_FUNC(int32_t) CIFailureInt_SetInfoExt(FailureType type,
                                            const char* p_msg,
                                            const char* p_function_name,
                                            int32_t line_num);

/*
 功能描述    : 检验是否可以终止程序
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2013年12月20日 11:22:04
*/
CIAPI_FUNC(CI_BOOL) CIFailureInt_BeTerminate(void);

/*
功能描述    : 故障检测初始化
返回值      : 成功为0，失败为-1
参数        : 无
作者        : 张彦升
日期        : 2013年10月28日 15:21:58
*/
CIAPI_FUNC(int32_t) CIFailureInt_Init(void);

#ifdef __cplusplus
}
#endif

#endif /*!_failures_int_h__*/
