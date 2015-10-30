/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 何境泰
版本        : 1.0
创建日期    : 2013年12月3日 14:46:07
用途        : 电子单元通信
历史修改记录: v1.0    创建
**********************************************************************/

#ifndef _eeu_manage_h__
#define _eeu_manage_h__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _EeuTag                      /*联锁机与电子模块帧头协议*/    
{
    uint32_t dt : 2;                     /*数据类型*/
    uint32_t ga : 6;                     /*网关地址*/             
    uint32_t fsn : 12;                   /*帧序号*/
    uint32_t ea : 4;                     /*电子单元地址*/
    uint32_t et : 4;                     /*电子单元类型*/
    uint32_t dir : 1;                    /*方向标志*/
    uint32_t preserve : 3;               /*预留位*/
}EeuTag;

/*电子单元通信帧共1 + 4 + 4 + 2 + 2 = 13位*/
typedef struct _EeuFrame                 /*联锁机与电子模块协议帧*/
{
    uint8_t head;                        /*帧头*/
    union 
    {
        EeuTag tag;                      /*CAN数据ID解析*/
        uint32_t frame_id;
    }id;                                 /*帧ID*/
    uint32_t state;                      /*状态数据*/
    uint16_t crc;                        /*crc校验码*/
    uint16_t preserve;                   /*最后两位为预留位*/
}EeuFrame;

/*
 功能描述    : 因为接收数据时一定要接收指定帧的数据，所以返回该数据并同步
 返回值      : 无
 参数        : 本次帧号和上次帧号
 作者        : 张彦升
 日期        : 2014年8月20日 15:44:19
*/
CIAPI_FUNC(void) CIEeu_GetRequstFsn(uint16_t* this_fsn,uint16_t* last_fsn);
/*
 功能描述    : 调整请求帧的序号
 返回值      : 无
 参数        : 要调整成的帧序号
 作者        : 张彦升
 日期        : 2014年8月20日 15:45:45
*/
CIAPI_FUNC(void) CIEeu_AdjustRequstFsn(uint16_t this_fsn,uint16_t last_fsn);
/*
 功能描述    : 为电子单元添加数据有效标志
 返回值      : 
 参数        : 
 作者        : 何境泰
 日期        : 2014年5月9日 13:47:15
*/
CIAPI_FUNC(void) CIEeu_SetDataMask(int16_t ga, int16_t ea);
/*
 功能描述    : 判断模块是否使用，
    在联锁程序启动的时候会读取配置文件，并调用CIEeu_SetDataMask设置模块地址是否
    使用，该函数用以判断电子单元模块是否被使用
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年6月24日 9:20:28
*/
CIAPI_FUNC(CI_BOOL) CIEeu_IsAddressUsing(int16_t ga, int16_t ea);
/*
 功能描述    : 得到本站场使用的网关的个数
 返回值      : 根据配置文件而得到的使用网关的个数
 参数        : 无
 日期        : 2015年4月4日 9:33:26
*/
CIAPI_FUNC(int32_t) CIEeu_UsingGaNum(void);
/*
 功能描述    : 得到本站场使用的模块的个数
 返回值      : 根据配置文件而得到的使用网关的个数
 参数        : 无
 日期        : 2015年4月4日 9:33:26
*/
CIAPI_FUNC(int32_t) CIEeu_UsingEaNum(void);
/*
功能描述    :获得电子单元当前状态 
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2014年4月2日 15:55:29
*/
CIAPI_FUNC(int32_t) CIEeu_ReceiveStatusData(void);
/*
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2014年3月3日 11:23:11
*/
CIAPI_FUNC(void) CIEeu_RequestStatus(void);
/*
 功能描述    : 只递增帧序号，而不请求
 返回值      : 无
 作者        : 何境泰
 日期        : 2014年3月3日 11:23:11
*/
CIAPI_FUNC(void) CIEeu_RequestIncFsn(void);
/*
 功能描述    : 打开输出电子单元信息
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年4月11日 15:51:44
*/
CIAPI_FUNC(int32_t) CIEeu_OpenPrintEeu(void);
/*
功能描述    : 发送所有节点命令
返回值      : 无
参数        : 无
作者        : 何境泰
日期        : 2014年4月16日 13:37:53
*/
CIAPI_FUNC(void) CIEeu_SendAllCommand(void);
/*
功能描述:  设备状态判断
返回值    :    
参数    :      
作者    :  何境泰
日期    :  2014年3月21日 9:32:45
*/
CIAPI_FUNC(int32_t) CIEeu_StatusJudge(void);
/*
 功能描述    : 得到由网关向上传输数据的时间
 返回值      : 时间
 参数        : 无
 日期        : 2015年4月4日 9:48:14
*/
CIAPI_FUNC(double) CIEeu_TransTime(void);
/*
 功能描述    : 得到电子单元一个周期需要的时间
 返回值      : 时间
 参数        : 无
 日期        : 2015年4月4日 9:44:29
*/
CIAPI_FUNC(double) CIEeu_NeedTime(void);
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
/*
 功能描述    : 测试电子单元通信道是否断开
 返回值      : 通信道正常则返回CI_TRUE,否则返回CI_FALSE
 参数        : 无
 日期        : 2015年5月29日 18:30:00
 */
CIAPI_FUNC(CI_BOOL) CIEeu_TestChannelBroke(void);
/*
 * 清除计数标志 
 */
CIAPI_FUNC(void) CIEeu_ClearErrorCount(void);
/*
功能描述    : 电子单元模块初始化
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2014年3月3日 13:56:04
*/
CIAPI_FUNC(int32_t) CIEeu_Init(void);

#ifdef __cplusplus
}
#endif

#endif /*!_eeu_manage_h__*/
