/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年10月25日 14:23:34
用途        : 双系心跳数据管理
历史修改记录: v1.0    创建
             v2.0    将配置文件全部计算md5值，并不传输所有内容
**********************************************************************/
#include "util/ci_header.h"

#ifdef LINUX_ENVRIONMENT

#include "syn_type.h"
#include "cfg_syn.h"

#include "util/algorithms.h"
#include "util/utility.h"

/*
功能描述    : 打印配置比较帧内容
返回值      : 无
参数        : @p_frame 配置帧指针
作者        : 张彦升
日期        : 2013年10月23日 9:27:08
*/
void CICfgSyn_Print(const CfgSynFrame* p_frame)
{
    unsigned char buf[2 * MD5_DIGEST_LENGTH + 1] = {0};

    printf("type          :%d\n",p_frame->type);
    printf("sn            :%d\n",p_frame->sn);
    printf("hash          :%#X\n",p_frame->hash);
    printf("time_stamp    :%#X\n",p_frame->time_stamp);
    printf("ci_md5        :%s\n",CI_Md5ToStr(p_frame->ci_md5,buf,2 * MD5_DIGEST_LENGTH + 1));
    printf("itl_md5       :%s\n",CI_Md5ToStr(p_frame->ilt_md5,buf,2 * MD5_DIGEST_LENGTH + 1));
    printf("snt_md5       :%s\n",CI_Md5ToStr(p_frame->snt_md5,buf,2 * MD5_DIGEST_LENGTH + 1));
    printf("ssi_md5       :%s\n",CI_Md5ToStr(p_frame->ssi_md5,buf,2 * MD5_DIGEST_LENGTH + 1));
    printf("ko_dpram_md5  :%s\n",CI_Md5ToStr(p_frame->ko_dpram_md5,buf,2 * MD5_DIGEST_LENGTH + 1));
    printf("ko_fiber_md5  :%s\n",CI_Md5ToStr(p_frame->ko_fiber_md5,buf,2 * MD5_DIGEST_LENGTH + 1));
    printf("ko_uart_md5   :%s\n",CI_Md5ToStr(p_frame->ko_uart_md5,buf,2 * MD5_DIGEST_LENGTH + 1));
    printf("ko_can_md5    :%s\n",CI_Md5ToStr(p_frame->ko_can_md5,buf,2 * MD5_DIGEST_LENGTH + 1));
    printf("ko_net_md5    :%s\n",CI_Md5ToStr(p_frame->ko_net_md5,buf,2 * MD5_DIGEST_LENGTH + 1));
    printf("ko_wtd_md5    :%s\n",CI_Md5ToStr(p_frame->ko_wtd_md5,buf,2 * MD5_DIGEST_LENGTH + 1));

    printf("cpu_state     :%d\n",p_frame->cpu_state);
    printf("station_name  :%s\n",p_frame->station_name);
    printf("hmi_ip_a      :%#x\n",p_frame->hmi_ip_a);
    printf("hmi_port_a    :%#x\n",p_frame->hmi_port_a);
    printf("hmi_ip_b      :%#x\n",p_frame->hmi_ip_b);
    printf("hmi_port_b    :%#x\n",p_frame->hmi_port_b);

    printf("hmi_device_id :%#x\n",p_frame->hmi_device_id);
}
/*
功能描述    : 计算hash值，然后将其填充到结构当中并返回该hash值
返回值      : 旧的hash值
参数        : @p_frame 配置数据帧指针
作者        : 张彦升
日期        : 2013年10月24日 14:34:14
*/
uint16_t CICfgSyn_CalcHash(CfgSynFrame* p_frame)
{
    uint16_t old_hash = p_frame->hash;
    uint16_t crc_code = 0;

    /*为了避免该域有值而导致计算结果不一致，而将此置为0*/
    p_frame->hash = 0;

    crc_code = CIAlgorithm_Crc16(p_frame,sizeof(CfgSynFrame));
    p_frame->hash = crc_code;

    return old_hash;
}
/*
 功能描述    : 验证帧内容，保证没有出现线路传输错误。
 返回值      : 验证成功为CI_TRUE，否则为CI_FALSE
 参数        : @p_frame 配置数据帧指针
 作者        : 张彦升
 日期        : 2014年8月12日 10:18:39
*/
CI_BOOL CICfgSyn_Verify(CfgSynFrame* p_frame)
{
    uint16_t old_hash = 0;

    /*检查数据校验码是否正确*/
    old_hash = CICfgSyn_CalcHash(p_frame);
    if (old_hash != p_frame->hash)
    {
#if 0
        CILog_Msg("验证配置比较校验码不一致:old_hash(%#x) != new_hash(%#x)",
                        old_hash, p_frame->hash);
#endif
        return CI_FALSE;
    }
    /*检查帧数据类型*/
    if (ST_CFG != p_frame->type)
    {
#if 0
        CILog_Msg("验证配置比较帧类型不一致:%d",p_frame->type);
#endif

        return CI_FALSE;
    }

    return CI_TRUE;
}
/*
 功能描述    : 初始化配置数据帧
 返回值      : 成功为0，失败为-1
 参数        : @p_frame 配置数据帧指针
 作者        : 张彦升
 日期        : 2014年8月12日 10:18:41
*/
int32_t CICfgSyn_Init(CfgSynFrame* p_frame)
{
    p_frame->type = ST_CFG;
    p_frame->hash = 0;
    p_frame->sn = 0;
    p_frame->time_stamp = CI_GetTimeStamp();

    memset(p_frame->ci_md5,0,MD5_DIGEST_LENGTH);
    memset(p_frame->ilt_md5,0,MD5_DIGEST_LENGTH);
    memset(p_frame->snt_md5,0,MD5_DIGEST_LENGTH);
    memset(p_frame->ssi_md5,0,MD5_DIGEST_LENGTH);
    memset(p_frame->cfg_md5,0,MD5_DIGEST_LENGTH);
    memset(p_frame->ko_dpram_md5,0,MD5_DIGEST_LENGTH);
    memset(p_frame->ko_fiber_md5,0,MD5_DIGEST_LENGTH);
    memset(p_frame->ko_uart_md5,0,MD5_DIGEST_LENGTH);
    memset(p_frame->ko_can_md5,0,MD5_DIGEST_LENGTH);
    memset(p_frame->ko_net_md5,0,MD5_DIGEST_LENGTH);
    memset(p_frame->ko_wtd_md5,0,MD5_DIGEST_LENGTH);
    
    p_frame->cpu_state = 0;
    memset(p_frame->station_name,0,STATION_NAME_SIZE);
    p_frame->hmi_ip_a = 0;
    p_frame->hmi_port_a = 0;
    p_frame->hmi_ip_b = 0;
    p_frame->hmi_port_b = 0;

    p_frame->hmi_device_id = -1;

    return 0;
}
#endif /*!LINUX_ENVRIONMENT*/