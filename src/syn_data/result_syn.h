 /*********************************************************************
 Copyright (C), 2011,  Co.Hengjun, Ltd.
 
 作者        : 何境泰
 版本        : 1.0
 创建日期    : 2013年10月29日 10:56:47
 用途        : 双系输出数据
 历史修改记录: v1.0    创建
 **********************************************************************/
#ifndef _series_result_syn_h__
#define _series_result_syn_h__

#include "util/ci_header.h"
#include "util/app_config.h"
#include "interlocking/base_type_definition.h"
#include "interlocking/data_struct_definition.h"

/*双系结果同步进路表数据*/
typedef ST_route SeriesResultRouteTable;

/*
 * 双系结果半自动数据同步
 */
typedef struct _SeriesResultSemiAutoBlock
{
    int16_t ZJ;
    int16_t FJ;
    uint8_t state;
    uint8_t block_state;
}SeriesResultSemiAutoBlock;
/**
 * 双系输出结果数据封装
 */
typedef struct
{
    uint16_t type;                      /*数据类型*/
    uint16_t hash;                      /*hash值*/
    uint32_t sn;                        /*帧编号*/
    uint32_t time_stamp;                /*时间戳*/
    uint32_t cycle_counter;             /*联锁周期号*/


    uint16_t commands[MAX_GETWAYS][USING_EEU_PER_LAYER];         /*控制命令数据*/
    SeriesResultRouteTable route[MAX_ROUTE];                   /*进路表*/
    SeriesResultSemiAutoBlock block[MAX_SEMI_AUTO_BLOCK];    /*半自动闭塞动态数据*/
}CIResultSynFrame;

/*
功能描述    : 打印双系输出结果，便于调试
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2013年10月29日 11:10:53
*/
CIAPI_FUNC(void) CIResultSyn_Print(const CIResultSynFrame* p_frame);

/*
功能描述    : 为输出结果生成哈希值
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2013年10月29日 11:11:38
*/
CIAPI_FUNC(uint16_t) CIResultSyn_CalcHash(CIResultSynFrame* p_frame);

/*
功能描述    : 检测帧类型和数据校验码错误
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2014年5月12日 16:22:38
*/
CIAPI_FUNC(CI_BOOL) CIResultSyn_Verify(CIResultSynFrame* p_frame);

/*
功能描述    : 双系输出结果初始化
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2013年10月29日 11:10:15
*/
CIAPI_FUNC(int32_t) CIResultSyn_Init(CIResultSynFrame* p_frame);




/*
功能描述    : 输出数据装配
返回值      :
参数        :
作者        : 何境泰
日期        : 2013年10月29日 11:17:06
*/
CIAPI_FUNC(int32_t) CIResultSyn_Assemble(CIResultSynFrame* p_frame);



/*
功能描述    : 输出同步数据比较
返回值      :
参数        :
作者        : 何境泰
日期        : 2013年10月29日 10:27:52
*/
CIAPI_FUNC(int32_t)  CIResultSyn_Compare(CIResultSynFrame self_result_frame, CIResultSynFrame peer_result_frame);


#endif /*!_series_result_syn_h__*/
