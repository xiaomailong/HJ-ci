/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年10月23日 16:10:48
用途        : 双系心跳数据管理
历史修改记录: v1.0    创建
**********************************************************************/

#ifndef _series_heartbeat_h__
#define _series_heartbeat_h__

/*
 * 双系心跳数据帧，相比双CPU心跳的数据，少了CPU相关的一些数据
 * 另外我们对心跳检测的数据事实上没有必要这么多，使用tcp的确认机制是不是会节省很多流量?
 */
typedef struct _SeriesHeartBeatFrame
{
    uint16_t type;                  /*数据类型*/
    uint16_t hash;                  /*hash值*/
    uint32_t sn;                    /*帧编号*/
    uint32_t time_stamp;            /*时间戳*/
    uint8_t  series_state;          /*本地双系状态*/
    uint32_t elapse;                /*流逝的周期数*/

    CI_BOOL eeu_channel_a_ok;    CI_BOOL eeu_channel_b_ok;           /*电子单元通信状态*/    CI_BOOL hmi_channel_a_ok;    CI_BOOL hmi_channel_b_ok;           /*控显机通信状态*/
}SeriesHeartBeatFrame;
/*
 功能描述    : cpu同步数据打印
 返回值      : 无
 参数        : @p_frame 双系心跳数据帧指针
 作者        : 张彦升
 日期        : 2013年10月23日 9:27:08
*/
CIAPI_FUNC(void) CISeriesHeartbeat_Print(const SeriesHeartBeatFrame* p_frame);
/*
 功能描述    : 计算hash值
 返回值      : 旧的hash值
 参数        : @p_frame 双系心跳数据帧指针
 作者        : 张彦升
 日期        : 2013年10月23日 9:27:08
*/
CIAPI_FUNC(uint16_t) CISeriesHeartbeat_CalcHash(SeriesHeartBeatFrame* p_frame);
/*
 功能描述    : 验证心跳数据帧是否正确
 返回值      : 验证成功返回CI_TRUE，否则返回CI_FALSE
 参数        : @p_frame 双系心跳数据帧指针
 作者        : 张彦升
 日期        : 2013年10月23日 9:27:08
*/
CIAPI_FUNC(CI_BOOL) CISeriesHeartbeat_Verify(SeriesHeartBeatFrame* p_frame);
/*
 功能描述    : 双系心跳数据帧初始化
 返回值      : 成功为0，失败为-1
 参数        : @p_frame 双系心跳数据帧指针
 作者        : 张彦升
 日期        : 2013年10月23日 9:27:08
*/
CIAPI_FUNC(int) CISeriesHeartbeat_Init(SeriesHeartBeatFrame* p_frame);

#endif /*!_series_heartbeat_h__*/
