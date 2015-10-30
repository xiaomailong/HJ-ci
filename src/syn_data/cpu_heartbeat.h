/*********************************************************************
Copyright (C), 2014,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年10月14日 9:06:14
用途        : 
历史修改记录: 
**********************************************************************/

#ifndef _cpu_heartbeat_h__
#define _cpu_heartbeat_h__

/*
 * CPU心跳数据帧，对于双CPU帧内不做发送设备和接收设备id的检查，因为对于双口ram是直接
 * 与电路板相连的，它是不可改变的，发送的入口和接收的出口都是固定不变的，所以我们认为
 * 无需做设备id的检查
 */
typedef struct _CpuHeartBeatFrame
{
    uint16_t type;                       /*数据类型*/
    uint16_t hash;                       /*hash值*/
    uint32_t sn;                         /*帧编号*/
    uint32_t time_stamp;                 /*时间戳*/
    uint32_t elapse;                     /*流逝的周期数*/

    uint8_t  series_state;               /*本系状态*/
    uint8_t  cpu_state;                  /*本CPU状态*/
}CpuHeartBeatFrame;
/*
 功能描述    : cpu同步数据打印
 返回值      : 无
 参数        : @p_frame 心跳数据帧指针
 作者        : 张彦升
 日期        : 2013年10月23日 9:27:08
*/
CIAPI_FUNC(void) CICpuHeartbeat_Print(const CpuHeartBeatFrame* frame);
/*
 功能描述    : 心跳信号的hash函数需要特殊对待，不能调用状态机内部或联锁层调用的函数，虽然
              调用CIAlgorithm_Crc16也不会出现问题，但遵循这个原则能避免不少问题。
 返回值      : 旧的hash值
 参数        : @p_frame 心跳数据帧指针
 作者        : 张彦升
 日期        : 2014年4月17日 15:02:44
*/
CIAPI_FUNC(uint16_t) CICpuHeartbeat_CalcHash(CpuHeartBeatFrame* p_frame);
/*
 功能描述    : 校验帧数据的正确性
 返回值      : 成功为0，失败为-1
 参数        : @p_frame 心跳数据帧指针
 作者        : 张彦升
 日期        : 2014年5月8日 15:49:24
*/
CIAPI_FUNC(CI_BOOL) CICpuHeartbeat_Verify(CpuHeartBeatFrame* p_frame);
/*
 功能描述    : 初始化数据帧
 返回值      : 成功为0，失败为-1
 参数        : @p_frame 心跳数据帧指针
 作者        : 张彦升
 日期        : 2014年8月12日 10:23:34
*/
CIAPI_FUNC(int) CICpuHeartbeat_Init(CpuHeartBeatFrame* p_frame);

#endif /*! _cpu_heartbeat_h__*/
