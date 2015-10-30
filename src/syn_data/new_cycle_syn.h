/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年10月14日 12:43:59
用途        : 新周期同步
历史修改记录: v1.0    创建
**********************************************************************/

#ifndef _new_cycle_syn_h__
#define _new_cycle_syn_h__

/**
 * CPU新周期同步数据封装，暂时无其它数据
 */
typedef struct _NewCycleSynFrame
{
    uint16_t type;                  /*数据类型*/
    uint16_t hash;                  /*hash值*/
    uint32_t sn;                    /*帧编号*/
    uint32_t time_stamp;            /*时间戳*/
    uint32_t elapse_cycle;          /*流逝的周期号*/
    uint16_t eeu_this_request_fsn;  /*电子单元本次请求数据的帧号*/
    uint16_t eeu_last_request_fsn;  /*电子单元上次请求数据的帧号*/
}NewCycleSynFrame;

/*
功能描述    : 同步数据打印
返回值      : 无
参数        : @p_frame 新周期同步数据帧指针
作者        : 张彦升
日期        : 2013年10月23日 9:27:08
*/
CIAPI_FUNC(void) CINewCycleSyn_Print(const NewCycleSynFrame* p_frame);
/*
功能描述    : 计算hash值
返回值      : 旧的hash值
参数        : @p_frame 新周期同步数据帧指针
作者        : 张彦升
日期        : 2013年10月31日 15:53:30
*/
CIAPI_FUNC(uint16_t) CINewCycleSyn_CalcHash(NewCycleSynFrame* p_frame);
/*
功能描述    : 验证数据帧内容
返回值      : 验证成功返回CI_TRUE，否则返回CI_FALSE
参数        : @p_frame 新周期同步数据帧指针
作者        : 张彦升
日期        : 2013年10月31日 15:53:30
*/
CIAPI_FUNC(CI_BOOL) CINewCycleSyn_Verify(NewCycleSynFrame* p_frame);
/*
功能描述    : 初始化新周期同步帧
返回值      : 成功为0，失败为-1
参数        : @p_frame 新周期同步数据帧指针
作者        : 张彦升
日期        : 2013年10月31日 15:53:30
*/
CIAPI_FUNC(int32_t) CINewCycleSyn_Init(NewCycleSynFrame* p_frame);

#endif /*! _new_cycle_syn_h__*/
