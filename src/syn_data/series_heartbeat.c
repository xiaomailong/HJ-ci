/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年10月25日 14:23:34
用途        : 双系心跳数据管理
历史修改记录: v1.0    创建
**********************************************************************/
#include "util/ci_header.h"

#ifdef LINUX_ENVRIONMENT

#include "syn_type.h"
#include "series_heartbeat.h"

#include "util/algorithms.h"
#include "util/utility.h"
/*
 功能描述    : cpu同步数据打印
 返回值      : 无
 参数        : @p_frame 双系心跳数据帧指针
 作者        : 张彦升
 日期        : 2013年10月23日 9:27:08
*/
void CISeriesHeartbeat_Print(const SeriesHeartBeatFrame* p_frame)
{
    printf("type                    :%d\n",p_frame->type);
    printf("sn                      :%#x\n",p_frame->sn);
    printf("hash                    :%#X\n",p_frame->hash);
    printf("time_stamp              :%#x\n",p_frame->time_stamp);
    printf("series_state            :%d\n",p_frame->series_state);
}
/*
 功能描述    : 计算hash值
 返回值      : 旧的hash值
 参数        : @p_frame 双系心跳数据帧指针
 作者        : 张彦升
 日期        : 2013年10月23日 9:27:08
*/
uint16_t CISeriesHeartbeat_CalcHash(SeriesHeartBeatFrame* p_frame)
{
    uint16_t old_hash = p_frame->hash;
    uint16_t crc_code = 0;

    /*为了避免该域有值而导致计算结果不一致，而将此置为0*/
    p_frame->hash = 0;

    crc_code = CIAlgorithm_Crc16(p_frame,sizeof(SeriesHeartBeatFrame));
    p_frame->hash = crc_code;

    return old_hash;
}
/*
 功能描述    : 验证心跳数据帧是否正确
 返回值      : 验证成功返回CI_TRUE，否则返回CI_FALSE
 参数        : @p_frame 双系心跳数据帧指针
 作者        : 张彦升
 日期        : 2013年10月23日 9:27:08
*/
CI_BOOL CISeriesHeartbeat_Verify(SeriesHeartBeatFrame* p_frame)
{
    uint16_t old_hash = 0;

    /*检查数据校验码是否正确*/
    old_hash = CISeriesHeartbeat_CalcHash(p_frame);
    if (old_hash != p_frame->hash)
    {
#if 0
        CILog_SigSafeMsg("验证双系心跳信号校验码不一致:old_hash(%#x) != new_hash(%#x)",
                        old_hash,p_frame->hash);
#endif
        return CI_FALSE;
    }
    /*检查帧数据类型*/
    if (ST_HEARTBEAT != p_frame->type)
    {
#if 0
        CILog_SigSafeMsg("验证双系心跳信号帧类型不一致:%d",p_frame->type);
#endif

        return CI_FALSE;
    }
    
    return CI_TRUE;
}
/*
 功能描述    : 双系心跳数据帧初始化
 返回值      : 成功为0，失败为-1
 参数        : @p_frame 双系心跳数据帧指针
 作者        : 张彦升
 日期        : 2013年10月23日 9:27:08
*/
int32_t CISeriesHeartbeat_Init(SeriesHeartBeatFrame* p_frame)
{
    p_frame->type = ST_HEARTBEAT;
    p_frame->sn = 0;
    p_frame->hash = 0;
    p_frame->time_stamp = CI_GetTimeStamp();
    p_frame->series_state = 0;
    p_frame->elapse = 0;

    p_frame->eeu_channel_a_ok = CI_TRUE;
    p_frame->eeu_channel_b_ok = CI_TRUE; /*电子单元通信状态*/
    p_frame->hmi_channel_a_ok = CI_TRUE;
    p_frame->hmi_channel_b_ok = CI_TRUE; /*控显机通信状态*/

    return 0;
}

#endif /*!LINUX_ENVRIONMENT*/