/*********************************************************************
Copyright (C), 2014,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年10月24日 10:36:32
用途        : 心跳信号的所有函数必须使用安全函数，负责会出现未知问题
历史修改记录: v1.0    创建
**********************************************************************/
#include "util/ci_header.h"

#ifdef LINUX_ENVRIONMENT

#include "syn_type.h"
#include "cpu_heartbeat.h"

#include "util/utility.h"
#include "util/algorithms.h"
/*
 功能描述    : cpu同步数据打印
 返回值      : 无
 参数        : @p_frame 心跳数据帧指针
 作者        : 张彦升
 日期        : 2013年10月23日 9:27:08
*/
void CICpuHeartbeat_Print(const CpuHeartBeatFrame* p_frame)
{
    printf("type                     :%d\n",p_frame->type);
    printf("hash                     :%#x\n",p_frame->hash);
    printf("sn                       :%#x\n",p_frame->sn);
    printf("time_stamp               :%#X\n",p_frame->time_stamp);
    printf("series_state             :%d\n",p_frame->series_state);
    printf("cpu_state                :%d\n",p_frame->cpu_state);
    printf("elapse                   :%d\n",p_frame->elapse);
}
/*
 功能描述    : 心跳信号的hash函数需要特殊对待，不能调用状态机内部或联锁层调用的函数，虽然
              调用CIAlgorithm_Crc16也不会出现问题，但遵循这个原则能避免不少问题。
 返回值      : 旧的hash值
 参数        : @p_frame 心跳数据帧指针
 作者        : 张彦升
 日期        : 2014年4月17日 15:02:44
*/
uint16_t CICpuHeartbeat_CalcHash(CpuHeartBeatFrame* p_frame)
{
    uint16_t old_hash = p_frame->hash;
    uint16_t crc_code = 0;

    /*为了避免该域有值而导致计算结果不一致，而将此置为0*/
    p_frame->hash = 0;

    crc_code = CIAlgorithm_Crc16(p_frame,sizeof(CpuHeartBeatFrame));
    p_frame->hash = crc_code;

    return old_hash;
}
/*
 功能描述    : 校验帧数据的正确性
 返回值      : 成功为0，失败为-1
 参数        : @p_frame 心跳数据帧指针
 作者        : 张彦升
 日期        : 2014年5月8日 15:49:24
*/
CI_BOOL CICpuHeartbeat_Verify(CpuHeartBeatFrame* p_frame)
{
    uint16_t old_hash = 0;

    /*检查数据校验码是否正确*/
    old_hash = CICpuHeartbeat_CalcHash(p_frame);
    if (old_hash != p_frame->hash)
    {
#if 0
        CILog_SigSafeMsg("验证双CPU心跳信号校验码不一致:%#x",old_hash);
#endif
        return CI_FALSE;
    }
    /*检查帧数据类型*/
    if (ST_HEARTBEAT != p_frame->type)
    {
#if 0
        CILog_SigSafeMsg("验证双CPU心跳信号帧类型不一致:%d",p_frame->type);
#endif

        return CI_FALSE;
    }

    return CI_TRUE;
}
/*
 功能描述    : 初始化数据帧
 返回值      : 成功为0，失败为-1
 参数        : @p_frame 心跳数据帧指针
 作者        : 张彦升
 日期        : 2014年8月12日 10:23:34
*/
int32_t CICpuHeartbeat_Init(CpuHeartBeatFrame* p_frame)
{
    p_frame->type = ST_HEARTBEAT;
    p_frame->sn = 0;
    p_frame->hash = 0;
    p_frame->series_state = 0;
    p_frame->cpu_state = 0;
    p_frame->elapse = 0;
    p_frame->time_stamp = CI_GetTimeStamp();

    return 0;
}

#endif /*!LINUX_ENVRIONMENT*/