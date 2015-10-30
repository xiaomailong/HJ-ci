/*********************************************************************
 Copyright (C), 2014,  Co.Hengjun, Ltd.

 作者        : 张彦升
 版本        : 1.0
 创建日期    : 2014年7月3日 10:46:53
 用途        : 用于侦听系统性能。
              该模块会通过网络将系统性能数据发送到远端，远端服务器可通过可视化程序
              将其实时显示出来
 历史修改记录: v1.0    创建
    v2.0    张彦升  将文件名从monitor转移为performance，monitor留给检测机使用
**********************************************************************/

#ifndef _performance_h__
#define _performance_h__

#ifdef __cplusplus
extern "C" {
#endif

/*向检测发送的数据类型，其中T表示发送，R表示接收*/
typedef enum _PERFORMANCE_DATA_TYPE
{
    PDT_CPU_CFG_T_BEGIN                     = 1,
    PDT_CPU_CFG_T_END                       = 2,
    PDT_CPU_CFG_R_BEGIN                     = 3,
    PDT_CPU_CFG_R_END                       = 4,

    PDT_CPU_NEW_CYCLE_T_BEGIN               = 5,
    PDT_CPU_NEW_CYCLE_T_END                 = 6,
    PDT_CPU_NEW_CYCLE_R_BEGIN               = 7,
    PDT_CPU_NEW_CYCLE_R_END                 = 8,

    PDT_CPU_INPUT_T_BEGIN                   = 9,
    PDT_CPU_INPUT_T_END                     = 10,
    PDT_CPU_INPUT_R_BEGIN                   = 11,
    PDT_CPU_INPUT_R_END                     = 12,

    PDT_CPU_RESULT_T_BEGIN                  = 13,
    PDT_CPU_RESULT_T_END                    = 14,
    PDT_CPU_RESULT_R_BEGIN                  = 15,
    PDT_CPU_RESULT_R_END                    = 16,

    PDT_CPU_HEARTBEAT_T_BEGIN               = 17,
    PDT_CPU_HEARTBEAT_T_END                 = 18,
    PDT_CPU_HEARTBEAT_R_BEGIN               = 19,
    PDT_CPU_HEARTBEAT_R_END                 = 20,

    PDT_SERIES_CFG_T_BEGIN                  = 21,
    PDT_SERIES_CFG_T_END                    = 22,
    PDT_SERIES_CFG_R_BEGIN                  = 23,
    PDT_SERIES_CFG_R_END                    = 24,

    PDT_SERIES_NEW_CYCLE_T_BEGIN            = 25,
    PDT_SERIES_NEW_CYCLE_T_END              = 26,
    PDT_SERIES_NEW_CYCLE_R_BEGIN            = 27,
    PDT_SERIES_NEW_CYCLE_R_END              = 28,

    PDT_SERIES_INPUT_T_BEGIN                = 29,
    PDT_SERIES_INPUT_T_END                  = 30,
    PDT_SERIES_INPUT_R_BEGIN                = 31,
    PDT_SERIES_INPUT_R_END                  = 32,

    PDT_SERIES_RESULT_T_BEGIN               = 33,
    PDT_SERIES_RESULT_T_END                 = 34,
    PDT_SERIES_RESULT_R_BEGIN               = 35,
    PDT_SERIES_RESULT_R_END                 = 36,

    PDT_SERIES_HEARTBEAT_T_BEGIN           = 37,
    PDT_SERIES_HEARTBEAT_T_END             = 38,
    PDT_SERIES_HEARTBEAT_R_BEGIN           = 39,
    PDT_SERIES_HEARTBEAT_R_END             = 40,

    PDT_INTRRUPT_BEGIN                      = 41, /*用来检测定时中断*/

    PDT_CI_CYCLE_BEGIN                      = 42, /*联锁周期开始*/
    PDT_CI_CYCLE_END                        = 43, /*联锁周期结束*/

    PDT_EEU_T_BEGIN                         = 44,
    PDT_EEU_T_END                           = 45,

    PDT_EEU_R_BEGIN                         = 46,
    PDT_EEU_R_END                           = 47,

}PERFORMANCE_DATA_TYPE;

typedef struct _PerformanceFrameHead
{
    uint16_t frame_head_tag;                /*帧头标志*/
    uint16_t frame_sn;                      /*帧编号*/
    uint32_t time_stamp;                    /*时间戳*/
    uint16_t data_type;                     /*数据类型*/
    uint8_t series_state;                   /*双系状态*/
    uint8_t cpu_state;                      /*双CPU状态*/
    uint16_t data_length;                   /*数据域数据长度*/	
}PerformanceFrameHead;

typedef struct _PerformanceFrame
{
    PerformanceFrameHead head;                      /*帧头*/
    /*在以后的扩展当中可以添加数据一项，但是目前为了节省网络带宽，暂且将其删除*/
    uint16_t crc;                               /*crc校验码*/
}PerformanceFrame;

/*
 功能描述    : 向检测机发送数据
 返回值      : 成功为0，失败为-1
 参数        : @mdt，发送数据类型
 作者        : 张彦升
 日期        : 2014年7月3日 19:19:19
*/
CIAPI_FUNC(int32_t) CIPerformance_Send(PERFORMANCE_DATA_TYPE mdt);
/*
 功能描述    : 向检测机发送数据，该函数为相对应的安全函数，只可在中断响应函数内部调用
 返回值      : 成功为0，失败为-1
 参数        : @mdt，发送数据类型
 作者        : 张彦升
 日期        : 2014年7月3日 19:19:19
*/
CIAPI_FUNC(int32_t) CIPerformance_SigSend(PERFORMANCE_DATA_TYPE mdt);
/*
 功能描述    : 检测管理初始化
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年7月3日 10:46:38
*/
CIAPI_FUNC(int32_t) CIPerformance_Init(void);

#ifdef __cplusplus
}
#endif

#endif /*!_performance_h__*/