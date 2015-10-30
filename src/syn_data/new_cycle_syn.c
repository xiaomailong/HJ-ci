/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年10月14日 12:44:20
用途        : 新周期同步数据
历史修改记录: v1.0    创建
**********************************************************************/
#include "util/ci_header.h"

#ifdef LINUX_ENVRIONMENT

#include "syn_type.h"
#include "new_cycle_syn.h"

#include "util/algorithms.h"
#include "util/utility.h"
/*
 功能描述    : 同步数据打印
 返回值      : 无
 参数        : @p_frame 新周期同步数据帧指针
 作者        : 张彦升
 日期        : 2013年10月23日 9:27:08
*/
void CINewCycleSyn_Print(const NewCycleSynFrame* p_frame)
{
    printf("type                    :%d\n",p_frame->type);
    printf("hash                    :%#x\n",p_frame->hash);
    printf("sn                      :%#x\n",p_frame->sn);
    printf("time_stamp              :%#x\n",p_frame->time_stamp);
    printf("elapse_cycle            :%#x\n",p_frame->elapse_cycle);
    printf("eeu_this_request_fsn    :%#x\n",p_frame->eeu_this_request_fsn);
    printf("eeu_last_request_fsn    :%#x\n",p_frame->eeu_last_request_fsn);

    return;
}
/*
功能描述    : 计算hash值
返回值      : 旧的hash值
参数        : @p_frame 新周期同步数据帧指针
作者        : 张彦升
日期        : 2013年10月31日 15:53:30
*/
uint16_t CINewCycleSyn_CalcHash(NewCycleSynFrame* p_frame)
{
    uint16_t old_hash = p_frame->hash;
    uint16_t crc_code = 0;

    /*为了避免该域有值而导致计算结果不一致，而将此置为0*/
    p_frame->hash = 0;

    crc_code = CIAlgorithm_Crc16(p_frame,sizeof(NewCycleSynFrame));
    p_frame->hash = crc_code;

    return old_hash;
}
/*
功能描述    : 验证数据帧内容
返回值      : 验证成功返回CI_TRUE，否则返回CI_FALSE
参数        : @p_frame 新周期同步数据帧指针
作者        : 张彦升
日期        : 2013年10月31日 15:53:30
*/
CI_BOOL CINewCycleSyn_Verify(NewCycleSynFrame* p_frame)
{
    uint16_t old_hash = 0;

    /*检查数据校验码是否正确*/
    old_hash = CINewCycleSyn_CalcHash(p_frame);
    if (old_hash != p_frame->hash)
    {
#if 0
        CILog_Msg("验证新周期同步数据校验码不一致:old_hash(%#x) != new_hash(%#x)",
            old_hash,p_frame->hash);
#endif
        return CI_FALSE;
    }
    /*检查帧数据类型*/
    if (ST_NEW_CYCLE != p_frame->type)
    {
#if 0
        CILog_Msg("验证新周期帧类型不一致:%d",p_frame->type);
#endif

        return CI_FALSE;
    }

    return CI_TRUE;
}
/*
功能描述    : 初始化新周期同步帧
返回值      : 成功为0，失败为-1
参数        : @p_frame 新周期同步数据帧指针
作者        : 张彦升
日期        : 2013年10月31日 15:53:30
*/
int32_t CINewCycleSyn_Init(NewCycleSynFrame* p_frame)
{
    p_frame->type = ST_NEW_CYCLE;
    p_frame->sn = 0;
    p_frame->hash = 0;
    p_frame->elapse_cycle = 0;
    p_frame->time_stamp = CI_GetTimeStamp();
    p_frame->eeu_last_request_fsn = -1;

    return 0;
}
#endif /*!LINUX_ENVRIONMENT*/