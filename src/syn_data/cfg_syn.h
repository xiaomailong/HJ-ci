/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年11月1日 8:50:32
用途        : 配置信息同步管理
历史修改记录: v1.0    创建
**********************************************************************/

#ifndef _cfg_syn_h__
#define _cfg_syn_h__

#include "util/algorithms.h"
#include "util/app_config.h"

typedef struct _CfgSynFrame
{
    uint16_t type;                                  /*数据类型*/
    uint16_t hash;                                  /*hash值*/
    uint32_t sn;                                    /*帧编号*/
    uint32_t time_stamp;                            /*时间戳*/

    unsigned char ci_md5[MD5_DIGEST_LENGTH];        /*CI程序的md5值*/
    unsigned char ilt_md5[MD5_DIGEST_LENGTH];       /*联锁表的md5值*/
    unsigned char snt_md5[MD5_DIGEST_LENGTH];       /*信号节点表的md5值*/
    unsigned char ssi_md5[MD5_DIGEST_LENGTH];
    unsigned char cfg_md5[MD5_DIGEST_LENGTH];

    unsigned char ko_dpram_md5[MD5_DIGEST_LENGTH];  /*双口ram模块的md5*/
    unsigned char ko_fiber_md5[MD5_DIGEST_LENGTH];  /*光纤模块的md5*/
    unsigned char ko_uart_md5[MD5_DIGEST_LENGTH];   /*串口模块的md5*/
    unsigned char ko_can_md5[MD5_DIGEST_LENGTH];    /*can板模块的md5*/
    unsigned char ko_net_md5[MD5_DIGEST_LENGTH];    /*网口模块的md5*/
    unsigned char ko_wtd_md5[MD5_DIGEST_LENGTH];    /*看门狗模块的md5*/

    uint8_t cpu_state;                              /*cpu状态*/
    uint8_t series_state;                           /*series状态*/
    char station_name[STATION_NAME_SIZE];           /*站场名称*/
    uint32_t hmi_ip_a;                              /*控显机A口ip地址*/
    uint16_t hmi_port_a;                            /*控显机A口端口*/
    uint32_t hmi_ip_b;                              /*控显机B口ip地址*/
    uint16_t hmi_port_b;                            /*控显机B口端口*/
    uint8_t hmi_device_id;                          /*控显机设备id*/
}CfgSynFrame;
/*
功能描述    : 打印配置帧内容信息
返回值      : 无
参数        : @p_frame 配置数据帧指针
作者        : 张彦升
日期        : 2013年10月23日 9:27:08
*/
CIAPI_FUNC(void) CICfgSyn_Print(const CfgSynFrame* p_frame);
/*
 功能描述    : 计算hash值
 返回值      : 旧的hash值
 参数        : @p_frame 配置数据帧指针
 作者        : 张彦升
 日期        : 2014年2月17日 12:50:42
*/
CIAPI_FUNC(uint16_t) CICfgSyn_CalcHash(CfgSynFrame* p_frame);
/*
 功能描述    : 验证帧内容，保证没有出现线路传输错误。
 返回值      : 验证成功为CI_TRUE，否则为CI_FALSE
 参数        : @p_frame 配置数据帧指针
 作者        : 张彦升
 日期        : 2014年8月12日 10:18:39
*/
CIAPI_FUNC(CI_BOOL) CICfgSyn_Verify(CfgSynFrame* p_frame);
/*
 功能描述    : 初始化配置数据帧
 返回值      : 成功为0，失败为-1
 参数        : @p_frame 配置数据帧指针
 作者        : 张彦升
 日期        : 2014年8月12日 10:18:41
*/
CIAPI_FUNC(int32_t) CICfgSyn_Init(CfgSynFrame* p_frame);

#endif /*!_cfg_syn_h__*/

