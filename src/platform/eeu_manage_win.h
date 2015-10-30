/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年12月3日 14:46:07
用途        : windows下电子单元通信
历史修改记录: v1.0    创建
**********************************************************************/
#ifndef _eeu_manage_win_h__
#define _eeu_manage_win_h__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _EeuFrame
{
    uint16_t head;          /*帧头*/
    uint16_t frame_sn;      /*帧编号*/
    int16_t node_index;     /*节点索引号*/
    uint32_t cmd;           /*命令*/
    uint16_t hash;          /*校验码*/
}EeuFrame;

CIAPI_FUNC(void) CIEeu_SetDataMask(uint32_t GA, uint32_t EA);
/*
功能描述    : 发送节点命令，在windows下即往模拟机发送信息，在linux下往can板发送
返回值      : 无
参数        : @node_index 节点索引号
             @cmd 控制命令
作者        : 何境泰
日期        : 2013年12月13日 13:21:34
*/
CIAPI_FUNC(void) CIEeu_SendNodeIndexCmd(int16_t node_index,uint32_t cmd);

/*
功能描述    : 从数据缓冲区中读取一帧数据
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2013年12月13日 13:27:05
*/
CIAPI_FUNC(int32_t) CIEeu_ReceiveStatusData();
/*
功能描述    : 电子单元模块初始化
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2014年3月3日 13:56:04
*/
CIAPI_FUNC(int32_t) CIEeu_Init(void);

/*
功能描述     : 设置是否要记录电子单元数据
返回值       : 
参数         : 
作者         : 何境泰
日期         : 2014年7月14日 10:53:08
*/
CIAPI_FUNC(void) CIEeu_SetRecord (CI_BOOL is_record);
/*
 功能描述    : 得到A通信道的状态
 返回值      : 通信道正常则返回CI_TRUE,否则返回CI_FALSE
 参数        : 无
 日期        : 2015年5月29日 18:30:00
*/
CIAPI_FUNC(CI_BOOL) CIEeu_IsChannelAOk(void);
/*
 功能描述    : 得到B通信道的状态
 返回值      : 通信道正常则返回CI_TRUE,否则返回CI_FALSE
 参数        : 无
 日期        : 2015年5月29日 18:30:00
*/
CIAPI_FUNC(CI_BOOL) CIEeu_IsChannelBOk(void);

#ifdef __cplusplus
}
#endif

#endif /*!_eeu_manage_win_h__*/