/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 何境泰
版本        : 1.0
创建日期    : 2013年10月29日 11:14:19
用途        : 双系输出结果同步
历史修改记录: v1.0    创建
**********************************************************************/

#include "util/ci_header.h"

#ifdef LINUX_ENVRIONMENT

#include "syn_type.h"
#include "result_syn.h"

#include "util/algorithms.h"
#include "util/utility.h"
#include "interlocking/inter_api.h"
#include "../platform/eeu_manage.h"

/*
 功能描述    : 双系结果数据进路表打印
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年8月14日 9:13:55
*/
static inline void series_result_route_print(const SeriesResultRouteTable* route)
{
    printf("\tILT_index                     :%d\n",route->ILT_index);
    printf("\tstate                         :%#x\n",route->state);
    printf("\tstate_machine                 :%d\n",route->state_machine);
    printf("\tbackward_route                :%d\n",route->backward_route);
    printf("\tforward_route                 :%d\n",route->forward_route);
    printf("\tcurrent_cycle                 :%d\n",route->current_cycle);
    printf("\terror_count                   :%d\n",route->error_count);
    printf("\tother_flag                    :%d\n",route->other_flag);
    printf("\tapproach_unlock               :%s\n",CI_BoolPrint(route->approach_unlock));

    return;
}
/*
 功能描述    : 双系结果数据半自动数据打印
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年8月14日 9:13:55
*/
static inline void series_result_semi_auto_block_print(const SeriesResultSemiAutoBlock* block)
{
    printf("\tZJ                   :%d\n",block->ZJ);
    printf("\tFJ                   :%d\n",block->FJ);
    printf("\tstate                :%#x\n",block->state);
    printf("\tblock_state          :%d\n",block->block_state);

    return;
}
/*
 功能描述    : 打印双系输出结果，便于调试
 返回值      : 
 参数        : 
 作者        : 何境泰
 日期        : 2013年10月29日 11:17:39
*/
void CIResultSyn_Print(const CIResultSynFrame* p_frame)
{
    int i = 0,j = 0;

    printf("type                        :%d\n",p_frame->type);
    printf("hash                        :%#x\n",p_frame->hash);
    printf("sn                          :%#x\n",p_frame->sn);
    printf("time_stamp                  :%#x\n",p_frame->time_stamp);

    printf("commands:\n");
    for (i = 0;i < MAX_GETWAYS;i++)
    {
        for (j = 0;j < USING_EEU_PER_LAYER;j++)
        {
            printf("%08x ",p_frame->commands[i][j]);
        }
        printf("\n");
    }

    for (i = 0;i < MAX_ROUTE;i++)
    {
        printf("route:%i\n",i);
        series_result_route_print(&p_frame->route[i]);
    }

    for (i = 0;i < MAX_SEMI_AUTO_BLOCK;i++)
    {
        printf("semi auto block:%i\n",i);
        series_result_semi_auto_block_print(&p_frame->block[i]);
    }

    return;
}

/*
功能描述    : 为输出结果生成哈希值
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2013年10月29日 11:18:17
*/
uint16_t CIResultSyn_CalcHash(CIResultSynFrame* p_frame)
{
    uint16_t old_hash = p_frame->hash;
    uint16_t crc_code = 0;

    /*为了避免该域有值而导致计算结果不一致，而将此置为0*/
    p_frame->hash = 0;

    crc_code = CIAlgorithm_Crc16(p_frame,sizeof(CIResultSynFrame));
    p_frame->hash = crc_code;

    return old_hash;
}

/*
功能描述    : 检测帧类型和数据校验码错误
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2014年5月12日 16:22:38
*/
CI_BOOL CIResultSyn_Verify(CIResultSynFrame* p_frame)
{
    uint16_t old_hash = 0;

    /*检查数据校验码是否正确*/
    old_hash = CIResultSyn_CalcHash(p_frame);
    if (old_hash != p_frame->hash)
    {
#if 0
        CILog_Msg("验证双系结果帧校验码不一致:old_hash(%#x) != new_hash(%#x)",
            old_hash,p_frame->hash);
#endif
        return CI_FALSE;
    }
    /*检查帧数据类型*/
    if (ST_RESULT != p_frame->type)
    {
#if 0
        CILog_Msg("验证双系结果帧类型不一致:type(%d)",p_frame->type);
#endif

        return CI_FALSE;
    }

    return CI_TRUE;
}

/*
功能描述    : 双系输出结果初始化
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2013年10月29日 11:17:06
*/
int32_t CIResultSyn_Init(CIResultSynFrame* p_frame)
{
    p_frame->type = ST_RESULT;
    p_frame->hash = 0;
    p_frame->sn = 0;
    p_frame->time_stamp = CI_GetTimeStamp();

    return 0;
}


/*
功能描述    : 结果数据装配
返回值      : 成功为0，失败为-1
参数        : 
作者        : 张彦升
日期        : 2014年4月15日 13:54:44
*/
int32_t CIResultSyn_Assemble(CIResultSynFrame* p_frame)
{
    int32_t i = 0, j = 0;

    assert(NULL != p_frame);

    p_frame->type = ST_RESULT;

    p_frame->time_stamp = CI_GetTimeStamp();
    p_frame->cycle_counter = CICycleInt_GetCounter();

    /*拷贝commands信息*/
    for (i = 0; i < MAX_GETWAYS; i++)
    {
        for (j = 0; j < USING_EEU_PER_LAYER; j++)
        {
            if (CI_TRUE == CIEeu_IsAddressUsing(i, j))
            {
                p_frame->commands[i][j] = CI_CompressCommandState(commands[i][j]);
            }
            else
            {
                p_frame->commands[i][j] = 0;
            }
        }
    }

    /*拷贝进路表*/
    for (i = 0; i < MAX_ROUTE; i++)
    {
        p_frame->route[i].ILT_index = routes[i].ILT_index;
        p_frame->route[i].state = routes[i].state;
        p_frame->route[i].backward_route = routes[i].backward_route;
        p_frame->route[i].forward_route = routes[i].forward_route;
        p_frame->route[i].current_cycle = routes[i].current_cycle;
        p_frame->route[i].other_flag = routes[i].other_flag;
        p_frame->route[i].approach_unlock = routes[i].approach_unlock;
    }

    /*拷贝半自动闭塞数据*/
    for (i = 0; i < TOTAL_SEMI_AUTO_BLOCK; i++)
    {
        p_frame->block[i].ZJ = semi_auto_block_config[i].ZJ;
        p_frame->block[i].FJ = semi_auto_block_config[i].FJ;
        p_frame->block[i].state = semi_auto_block_config[i].state;
        p_frame->block[i].block_state = semi_auto_block_config[i].block_state;
    }

    /*计算crc码*/
    CIResultSyn_CalcHash(p_frame);

    return 0;
}


/*
功能描述    : 比较双系输出结果是否一致
返回值      :
参数        :
作者        : 何境泰
日期        : 2014年5月13日 9:31:26
*/
int32_t CIResultSyn_Compare(CIResultSynFrame self_result_frame, CIResultSynFrame peer_result_frame)
{
    int i = 0, j = 0;
    
    /*比较命令信息*/
    for (i = 0; i < MAX_GETWAYS; i++)
    {
        for (j = 0; j < USING_EEU_PER_LAYER; j++)
        {
            if (self_result_frame.commands[i][j] != peer_result_frame.commands[i][j])
            {
                CIRemoteLog_Write("result_cmp_commands_fail:%d_%d:%#x!=%#x",
                    i, j, self_result_frame.commands[i][j], peer_result_frame.commands[i][j]
                    );

                return -1;
            }
        }
    }

#define SERIES_RESULT_ARR_CMP(arr,index,field)                                          \
    if (self_result_frame.arr[index].field != peer_result_frame.arr[index].field) {     \
        CIRemoteLog_Write("series_result_cmp_"#arr"_fail:%d:"#field":%d!=%d",           \
            index,self_result_frame.arr[index].field,peer_result_frame.arr[index].field);\
        return -1;                                                                      \
            }

    /*比较进路表*/
    for (i = 0; i < MAX_ROUTE; i++)
    {
        SERIES_RESULT_ARR_CMP(route, i, ILT_index);
        SERIES_RESULT_ARR_CMP(route, i, backward_route);
        SERIES_RESULT_ARR_CMP(route, i, forward_route);
        SERIES_RESULT_ARR_CMP(route, i, other_flag);
        SERIES_RESULT_ARR_CMP(route, i, approach_unlock);
    }

    /*比较半自动信息*/
    for (i = 0; i < TOTAL_SEMI_AUTO_BLOCK; i++)
    {
        SERIES_RESULT_ARR_CMP(block, i, ZJ);
        SERIES_RESULT_ARR_CMP(block, i, FJ);
        SERIES_RESULT_ARR_CMP(block, i, state);
        SERIES_RESULT_ARR_CMP(block, i, block_state);
    }

#undef SERIES_RESULT_ARR_CMP

    return 0;
}
#endif /*!LINUX_ENVRIONMENT*/
