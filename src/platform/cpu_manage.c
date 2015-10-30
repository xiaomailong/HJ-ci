/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年10月29日 15:19:44
用途        : cpu管理
历史修改记录: v1.0    创建
**********************************************************************/
#include "util/ci_header.h"
#include "cpu_manage.h"

#ifdef WIN32
CpuState CICpu_GetLocalState(void)
{
    return CPU_STATE_MASTER;
}
#else
#include <fcntl.h>
#include <sys/ioctl.h>

#include "util/ci_header.h"
#include "util/config.h"
#include "util/utility.h"
#include "util/log.h"

#include "failures_int.h"
#include "cycle_int.h"
#include "hmi_manage.h"
#include "timer.h"
#include "performance.h"
#include "cfg_manage.h"
#include "eeu_manage.h"
#include "remote_log.h"

#include "syn_data/new_cycle_syn.h"
#include "syn_data/input_syn.h"
#include "syn_data/cpu_heartbeat.h"
#include "syn_data/result_syn.h"
#include "syn_data/cfg_syn.h"
#include "syn_data/syn_type.h"

#include "interlocking/inter_api.h"

static CpuState local_cpu_state = CPU_STATE_NONE;           /*本CPU工作状态*/
static SeriesState peer_cpu_series_state = SERIES_PENDING;  /*另外一个CPU的双系状态*/

static const char* cpu_state_name[] = {
    [CPU_STATE_NONE]      = "cpu_none",
    [CPU_STATE_MASTER]    = "cpu_master",
    [CPU_STATE_SLAVE]     = "cpu_slave",
};

static CfgSynFrame self_cfg_frame,peer_cfg_frame;
static NewCycleSynFrame self_new_cycle_frame,peer_new_cycle_frame;
static CIInputSynFrame self_input_frame,peer_input_frame;
static CIResultSynFrame self_result_frame,peer_result_frame;
static CpuHeartBeatFrame self_beat_frame,peer_beat_frame;

/*CPU同步通信文件句柄*/
static int32_t cfg_fd_a;
static int32_t new_cycle_fd_a;
static int32_t input_fd_a;
static int32_t result_fd_a;
static int32_t heartbeat_fd_a;

static int32_t cfg_fd_b;
static int32_t new_cycle_fd_b;
static int32_t input_fd_b;
static int32_t result_fd_b;
static int32_t heartbeat_fd_b;

static CI_BOOL b_print_cfg_syn = CI_FALSE;          /*是否打印配置比较信息*/
static CI_BOOL b_print_new_cycle_syn = CI_FALSE;    /*是否打印新周期同步信息*/
static CI_BOOL b_print_input_syn = CI_FALSE;        /*是否打印输入信息*/
static CI_BOOL b_print_result_syn = CI_FALSE;       /*是否打印结果信息*/
static CI_BOOL b_print_heartbeat = CI_FALSE;        /*是否打印心跳信号*/

static uint32_t peer_new_cycle_last_time = -1;
static uint32_t peer_input_last_time = -1;
static uint32_t peer_result_last_time = -1;
static uint32_t peer_heartbeat_last_time = -1;
static uint32_t peer_cfg_last_time = -1;

static uint32_t peer_heartbeat_last_sn = -1;

#define DPRAM_IOC_MAGIC  'k'
#define DPRAM_IOCRESET    _IO(DPRAM_IOC_MAGIC, 0)
#define DPRAM_IOC_CLEAN_MEMORY _IOWR(DPRAM_IOC_MAGIC,  1, int)
#define DPRAM_IOC_ALLOC_MEMORY _IOWR(DPRAM_IOC_MAGIC,  2, int)
#define DPRAM_IOC_CHECK_MEMORY _IOWR(DPRAM_IOC_MAGIC,  3, int)

/*
功能描述    : 设置CPU状态，该函数不能暴漏给其它模块，意味着其它模块没有权利更改本CPU状态
返回值      : 成功为0，失败为-1
参数        : @state 枚举类型的CPU状态
作者        : 张彦升
日期        : 2013年10月28日 14:34:38
*/
static int32_t cpu_set_local_state(CpuState state)
{
    if (CPU_STATE_NONE == state)
    {
        CIRemoteLog_Write("CPU状态不能为不确定状态");

        return -1;
    }

    local_cpu_state = state;

    return 0;
}
/*
功能描述    : 得到CPU状态
返回值      : 本CPU的状态
参数        : 无
作者        : 张彦升
日期        : 2013年10月28日 14:34:52
*/
CpuState CICpu_GetLocalState(void)
{
    return local_cpu_state;
}
/*
 功能描述    : 得到同伴CPU的双系状态
 返回值      : 同伴的双系状态
 参数        : 无
 作者        : 张彦升
 日期        : 2014年5月9日 14:51:23
*/
SeriesState CICpu_GetPeerSeriesState(void)
{
    return peer_cpu_series_state;
}
/*
功能描述    : 确定主从CPU的关系
返回值      : 成功为0，失败为-1
参数        : 无
作者        : 张彦升
日期        : 2013年10月28日 9:16:58
*/
static int32_t cpu_validate_relationship(void)
{
    const char* str_cpu_state;
    str_cpu_state = CIConfig_GetValue("CpuState");

    assert(0 != strcmp(str_cpu_state,""));

    if (strcmp(str_cpu_state,"master") == 0)
    {
        cpu_set_local_state(CPU_STATE_MASTER);
    }
    else if (strcmp(str_cpu_state,"slave") == 0)
    {
        cpu_set_local_state(CPU_STATE_SLAVE);
    }
    else
    {
        CIRemoteLog_Write("未能得到CPU状态");
        return -1;
    }

    CIRemoteLog_Write("cpu_state:%s",CICpu_GetStateName(local_cpu_state));

    return 0;
}

/*
 功能描述    : 得到cpu状态的名称
 返回值      : 根据CPU状态类型得到的状态名称
 参数        : 误
 作者        : 张彦升
 日期        : 2013年12月20日 14:33:36
*/
const char* CICpu_GetStateName(CpuState state)
{
    return cpu_state_name[state];
}
/*
 功能描述    : 打开配置比较信息的输出
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月11日 15:29:15
*/
int32_t CICpu_OpenPrintCfgSyn(void)
{
    b_print_cfg_syn = CI_TRUE;

    return 0;
}
/*
 功能描述    : 配置数据装配
 返回值      : 成功为0，失败为-1
 参数        : @p_frame 要装配的帧
              @sn 帧序号
 作者        : 张彦升
 日期        : 2014年4月15日 13:45:18
*/
static int32_t cpu_assemble_cfg_syn_data(CfgSynFrame* p_frame,uint32_t sn)
{
    p_frame->type = ST_CFG;
    p_frame->sn = sn;
    p_frame->time_stamp = CI_GetTimeStamp();

    CICfg_GetAppMd5(p_frame->ci_md5);
    CICfg_GetIltMd5(p_frame->ilt_md5);
    CICfg_GetSntMd5(p_frame->snt_md5);
    CICfg_GetSsiMd5(p_frame->ssi_md5);
    CICfg_GetCfgMd5(p_frame->cfg_md5);
    CICfg_GetDpramKoMd5(p_frame->ko_dpram_md5);
    CICfg_GetFiberKoMd5(p_frame->ko_fiber_md5);
    CICfg_GetNetKoMd5(p_frame->ko_net_md5);
    CICfg_GetCanKoMd5(p_frame->ko_can_md5);
    CICfg_GetUartKoMd5(p_frame->ko_uart_md5);
    CICfg_GetWtdKoMd5(p_frame->ko_wtd_md5);

    strcpy(p_frame->station_name,CICfg_StationName());
    p_frame->cpu_state = CICpu_GetLocalState();
    p_frame->hmi_ip_a = CIHmi_GetIpA();
    p_frame->hmi_port_a = CIHmi_GetPortA();
    p_frame->hmi_ip_b = CIHmi_GetIpB();
    p_frame->hmi_port_b = CIHmi_GetPortB();
    p_frame->hmi_device_id = CIHmi_GetDeviceId();

    /*计算crc码*/
    CICfgSyn_CalcHash(p_frame);
    return 0;
}
/*
功能描述    : 发送配置数据
返回值      : 成功为0，失败为-1
参数        : 无
作者        : 张彦升
日期        : 2013年11月1日 9:02:57
*/
int32_t CICpu_SendCfgSyn(void)
{
    /*CPU配置比较只有启动的时候会比较一次，所以将这一帧没有做静态修饰，以节省内存*/
    CfgSynFrame complement_frame;
    static uint32_t sn = 1;
    int32_t ret = 0;

    CIPerformance_Send(PDT_CPU_CFG_T_BEGIN);

    if (CI_TRUE == b_print_cfg_syn)
    {
        CILog_Msg("cpu_cfg_send_begin:%d",sn);
    }

    /*对帧中的数据进行装配*/
    cpu_assemble_cfg_syn_data(&self_cfg_frame,sn);

    ret = write(cfg_fd_a,&self_cfg_frame,sizeof(self_cfg_frame));
    if (0 >= ret)
    {
        CILog_Errno("cpu_cfg_send_fail_a:%#x", self_cfg_frame.sn);

        return -1;
    }
    else
    {
        if (CI_TRUE == b_print_cfg_syn)
        {
            CILog_Msg("cpu_cfg_send_success_a:%#x", self_cfg_frame.sn);
        }
    }

    /*send_b*/
    memset(&complement_frame,0,sizeof(complement_frame));

    /*得到反码数据*/
    CI_BitwiseNot(&self_cfg_frame,&complement_frame,sizeof(complement_frame));

    ret = write(cfg_fd_b,&complement_frame,sizeof(complement_frame));
    if (0 >= ret)
    {
        CILog_Errno("cpu_cfg_send_fail_b:%#x", self_cfg_frame.sn);

        return -1;
    }
    else
    {
        if (CI_TRUE == b_print_cfg_syn)
        {
            CILog_Msg("cpu_cfg_send_success_b:%#x", self_cfg_frame.sn);
        }
    }

    if (CI_TRUE == b_print_cfg_syn)
    {
        CILog_Msg("cpu_cfg_send_end:%d",sn);
    }

    sn += 1;

    CIPerformance_Send(PDT_CPU_CFG_T_END);

    return 0;
}
/*
 功能描述    : 接收原码数据
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年8月2日 16:44:37
*/
static int32_t cpu_recv_original_cfg_syn(void)
{
    int32_t ret = 0;

    memset(&peer_cfg_frame,0,sizeof(peer_cfg_frame));

    ret = read(cfg_fd_a,&peer_cfg_frame,sizeof(peer_cfg_frame));
    if (0 >= ret)
    {
#if 0
        if (CI_TRUE == b_print_cfg_syn)
        {
            CILog_Errno("cpu_cfg_recv_fail_a");
        }
#endif

        return -1;
    }
    else
    {
        if (CI_FALSE == CICfgSyn_Verify(&peer_cfg_frame))
        {
            if (CI_TRUE == b_print_cfg_syn)
            {
                CILog_Msg("cpu_cfg_recv_fail_a:data_error");
            }
            return -1;
        }
        else if (peer_cfg_last_time == peer_cfg_frame.time_stamp)
        {
            if (CI_TRUE == b_print_cfg_syn)
            {
                CILog_Msg("cpu_cfg_recv_fail_a:timestamp_islast");
            }

            return -1;
        }
    }

    if (CI_TRUE == b_print_cfg_syn)
    {
        CILog_Msg("cpu_cfg_recv_success_a:%#x", peer_cfg_frame.sn);
    }

    return 0;
}
/*
 功能描述    : 接收反码数据
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年8月2日 16:44:37
*/
static int32_t cpu_recv_complement_cfg_syn(void)
{
    /*CPU配置比较只有启动的时候会比较一次，所以将这一帧没有做静态修饰，以节省内存*/
    CfgSynFrame complement_frame;
    int32_t ret = 0;

    memset(&complement_frame,0,sizeof(complement_frame));

    ret = read(cfg_fd_b,&complement_frame,sizeof(complement_frame));
    if (0 >= ret)
    {
#if 0
        if (CI_TRUE == b_print_cfg_syn)
        {
            CILog_Errno("cpu_cfg_recv_fail_b");
        }
#endif

        return -1;
    }
    else
    {
        /*如果获取到数据则根据反位转换回原数据*/
        CI_BitwiseNot(&complement_frame,&peer_cfg_frame,sizeof(peer_cfg_frame));

        if (CI_FALSE == CICfgSyn_Verify(&peer_cfg_frame))
        {
            if (CI_TRUE == b_print_cfg_syn)
            {
                CIRemoteLog_Write("cpu_cfg_recv_fail_b:data_error");
            }
            return -1;
        }
        else if (peer_cfg_last_time == peer_cfg_frame.time_stamp)
        {
            if (CI_TRUE == b_print_cfg_syn)
            {
                CIRemoteLog_Write("cpu_cfg_recv_fail_b:timestamp_islast");
            }

            return -1;
        }
    }

    if (CI_TRUE == b_print_cfg_syn)
    {
        CILog_Msg("cpu_cfg_recv_success_b:%#x", peer_cfg_frame.sn);
    }

    return 0;
}
/*
功能描述    : 接受CPU同步数据
返回值      : 成功为0，失败为-1
参数        : 无
作者        : 张彦升
日期        : 2013年11月1日 9:03:14
*/
int32_t CICpu_RecvCfgSyn(void)
{
    int32_t ret = 0;

    CIPerformance_Send(PDT_CPU_CFG_R_BEGIN);

#if 0
    if (CI_TRUE == b_print_cfg_syn)
    {
        CILog_Msg("cpu_cfg_recv_begin");
    }
#endif

    ret = cpu_recv_original_cfg_syn();
    if (-1 == ret)
    {
        ret = cpu_recv_complement_cfg_syn();
    }
    else
    {
        ioctl(cfg_fd_b,DPRAM_IOC_CLEAN_MEMORY);
    }

    /*每次检查是否读取正确，并更新上次时间戳*/
    if (-1 == ret)
    {
        return -1;
    }
    else
    {
        peer_cfg_last_time = peer_cfg_frame.time_stamp;
    }

    if (CI_TRUE == b_print_cfg_syn)
    {
        CILog_Msg("cpu_cfg_recv_end:%#x", peer_cfg_frame.sn);
    }

    CIPerformance_Send(PDT_CPU_CFG_R_END);

    return 0;
}
/*
功能描述    : 比较配置数据
返回值      : 成功为0，失败为-1
参数        : 无
作者        : 何境泰
日期        : 2013年11月29日 14:36:06
*/
int32_t CICpu_CheckCfgSyn(void)
{
    if (CI_TRUE == b_print_cfg_syn)
    {
        CIRemoteLog_Write("cpu_cfg_compare_begin");
    }

    if (0 != memcmp(self_cfg_frame.ci_md5,peer_cfg_frame.ci_md5,MD5_DIGEST_LENGTH))
    {
        CIRemoteLog_Write("cpu_cfg_compare_fail:CI主程序md5不一致");
        return -1;
    }
    if (0 != memcmp(self_cfg_frame.ilt_md5,peer_cfg_frame.ilt_md5,MD5_DIGEST_LENGTH))
    {
        CIRemoteLog_Write("cpu_cfg_compare_fail:联锁表的md5值不一致");
        return -1;
    }
    if (0 != memcmp(self_cfg_frame.snt_md5,peer_cfg_frame.snt_md5,MD5_DIGEST_LENGTH))
    {
        CIRemoteLog_Write("cpu_cfg_compare_fail:信号节点表的md5值不一致");
        return -1;
    }
    if (0 != memcmp(self_cfg_frame.ssi_md5,peer_cfg_frame.ssi_md5,MD5_DIGEST_LENGTH))
    {
        CIRemoteLog_Write("cpu_cfg_compare_fail:ssi文件的md5值不一致");
        return -1;
    }
    if (0 != memcmp(self_cfg_frame.cfg_md5,peer_cfg_frame.cfg_md5,MD5_DIGEST_LENGTH))
    {
        CIRemoteLog_Write("cpu_cfg_compare_fail:cfg文件的md5值不一致");
        return -1;
    }
    if (0 != memcmp(self_cfg_frame.ko_dpram_md5,peer_cfg_frame.ko_dpram_md5,MD5_DIGEST_LENGTH))
    {
        CIRemoteLog_Write("cpu_cfg_compare_fail:双口ram模块的md5不一致");
        return -1;
    }
    if (0 != memcmp(self_cfg_frame.ko_fiber_md5,peer_cfg_frame.ko_fiber_md5,MD5_DIGEST_LENGTH))
    {
        CIRemoteLog_Write("cpu_cfg_compare_fail:光纤模块的md5不一致");
        return -1;
    }
    if (0 != memcmp(self_cfg_frame.ko_uart_md5,peer_cfg_frame.ko_uart_md5,MD5_DIGEST_LENGTH))
    {
        CIRemoteLog_Write("cpu_cfg_compare_fail:串口模块的md5不一致");
        return -1;
    }
    if (0 != memcmp(self_cfg_frame.ko_can_md5,peer_cfg_frame.ko_can_md5,MD5_DIGEST_LENGTH))
    {
        CIRemoteLog_Write("cpu_cfg_compare_fail:can板模块的md5不一致");
        return -1;
    }
    if (0 != memcmp(self_cfg_frame.ko_net_md5,peer_cfg_frame.ko_net_md5,MD5_DIGEST_LENGTH))
    {
        CIRemoteLog_Write("cpu_cfg_compare_fail:网口模块的md5不一致");
        return -1;
    }
    if (0 != memcmp(self_cfg_frame.ko_wtd_md5,peer_cfg_frame.ko_wtd_md5,MD5_DIGEST_LENGTH))
    {
        CIRemoteLog_Write("cpu_cfg_compare_fail:看门狗模块的md5不一致");
        return -1;
    }

    if (self_cfg_frame.cpu_state == peer_cfg_frame.cpu_state)
    {
        assert(CPU_STATE_NONE != peer_cfg_frame.cpu_state);

        if (CPU_STATE_MASTER == peer_cfg_frame.cpu_state)
        {
            CIRemoteLog_Write("cpu_cfg_compare_fail:配置比较检测出双主CPU故障");
        }
        else if(CPU_STATE_SLAVE == peer_cfg_frame.cpu_state)
        {
            CIRemoteLog_Write("cpu_cfg_compare_fail:配置比较检测出双从CPU故障");
        }

        return -1;
    }
    if (0 != memcmp(self_cfg_frame.station_name,peer_cfg_frame.station_name,STATION_NAME_SIZE))
    {
        CIRemoteLog_Write("cpu_cfg_compare_fail:配置比较站场名称不一致");
        return -1;
    }
    if (self_cfg_frame.hmi_ip_a != peer_cfg_frame.hmi_ip_a)
    {
        CIRemoteLog_Write("cpu_cfg_compare_fail:配置比较控显机A口ip地址不一致");
        return -1;
    }
    if (self_cfg_frame.hmi_port_a != peer_cfg_frame.hmi_port_a)
    {
        CIRemoteLog_Write("cpu_cfg_compare_fail:配置比较控显机A口PORT地址不一致");
        return -1;
    }
    if (self_cfg_frame.hmi_ip_b != peer_cfg_frame.hmi_ip_b)
    {
        CIRemoteLog_Write("cpu_cfg_compare_fail:配置比较控显机B口ip地址不一致");
        return -1;
    }
    if (self_cfg_frame.hmi_port_b != peer_cfg_frame.hmi_port_b)
    {
        CIRemoteLog_Write("cpu_cfg_compare_fail:配置比较控显机B口PORT地址不一致");
        return -1;
    }
    if (self_cfg_frame.hmi_device_id != peer_cfg_frame.hmi_device_id)
    {
        CIRemoteLog_Write("cpu_cfg_compare_fail:配置比较控显机设备ID不一致");
        return -1;
    }

    if (CI_TRUE == b_print_cfg_syn)
    {
        CIRemoteLog_Write("cpu_cfg_compare_success");
    }

    return 0;
}
/*
 功能描述    : 双CPU之间的比较
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月30日 7:58:15
*/
int32_t CICpu_CfgSyn(void)
{
    int32_t ret = -1;

    CIRemoteLog_Write("dual cpu now syn cfg data");

    /*两个CPU都是先发送后接收，这样可以减小延迟*/
    while (-1 == CICpu_SendCfgSyn())
    {
        usleep(1000);
    }

    while(1)
    {
        ret = CICpu_RecvCfgSyn();
        if (-1 != ret)
        {
            break;
        }
    }
    /*比较配置数据,配置比较主要完成配置内容及配置版本的比较*/
    ret = CICpu_CheckCfgSyn();
    if (-1 == ret)
    {
        CIRemoteLog_Write("双CPU配置数据同步失败");
    }
    else
    {
        CIRemoteLog_Write("双CPU配置数据同步成功");
    }

    return ret;
}
/*
 功能描述    : 打开新周期同步信息输出
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月11日 15:29:15
*/
int32_t CICpu_OpenPrintNewCycleSyn(void)
{
    b_print_new_cycle_syn = CI_TRUE;

    return 0;
}
/*
 功能描述    : 数据装配
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月15日 13:43:23
*/
static int32_t cpu_assemble_new_cycle_syn_data(NewCycleSynFrame* p_frame,uint32_t sn)
{
    p_frame->type = ST_NEW_CYCLE;
    p_frame->sn = sn;
    p_frame->time_stamp = CI_GetTimeStamp();
    p_frame->elapse_cycle = CITimer_GetElapsedCycles();
    CIEeu_GetRequstFsn(&p_frame->eeu_this_request_fsn,&p_frame->eeu_last_request_fsn);

    CINewCycleSyn_CalcHash(p_frame);

    return 0;
}
/*
功能描述    : 发送数据的步骤为：对sn进行装配，crc进行装配，最后发送
返回值      : 成功为0，失败为-1
参数        : 无
作者        : 张彦升
日期        : 2013年10月24日 16:10:17
*/
int32_t CICpu_SendNewCycleSyn(void)
{
    static NewCycleSynFrame complement_frame;
    int32_t ret = 0;

    CIPerformance_Send(PDT_CPU_NEW_CYCLE_T_BEGIN);

    if (CI_TRUE == b_print_new_cycle_syn)
    {
        CILog_Msg("cpu_new_cycle_send_begin:%#x",CICycleInt_GetCounter());
    }

    /*对帧中的数据进行装配*/
    cpu_assemble_new_cycle_syn_data(&self_new_cycle_frame,CICycleInt_GetCounter());

    ret = write(new_cycle_fd_a, &self_new_cycle_frame,sizeof(self_new_cycle_frame));
    if (0 >= ret)
    {
        if (CI_TRUE == b_print_new_cycle_syn)
        {
            CILog_Errno("cpu_new_cycle_send_fail_a:%#x", self_new_cycle_frame.sn);
        }

        return -1;
    }
    else
    {
        if (CI_TRUE == b_print_new_cycle_syn)
        {
            CILog_Msg("cpu_new_cycle_send_success_a:%#x", self_new_cycle_frame.sn);
        }
    }

    /*send_b:*/
    memset(&complement_frame,0,sizeof(complement_frame));

    /*得到反码数据*/
    CI_BitwiseNot(&self_new_cycle_frame,&complement_frame,sizeof(complement_frame));

    ret = write(new_cycle_fd_b, &complement_frame,sizeof(complement_frame));
    if (0 >= ret)
    {
        if (CI_TRUE == b_print_new_cycle_syn)
        {
            CILog_Errno("cpu_new_cycle_send_fail_b:%#x", self_new_cycle_frame.sn);
        }

        return -1;
    }
    else
    {
        if (CI_TRUE == b_print_new_cycle_syn)
        {
            CILog_Msg("cpu_new_cycle_send_success_b:%#x", self_new_cycle_frame.sn);
        }
    }

    if (CI_TRUE == b_print_new_cycle_syn)
    {
        CILog_Msg("cpu_new_cycle_send_end:%#x", self_new_cycle_frame.sn);
    }

    CIPerformance_Send(PDT_CPU_NEW_CYCLE_T_END);

    return 0;
}
/*
 功能描述    : 接收新周期原码数据
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年8月2日 17:22:56
*/
static int32_t cpu_recv_original_new_cycle(void)
{
    int32_t ret = 0;

    memset(&peer_new_cycle_frame,0,sizeof(peer_new_cycle_frame));

    ret = read(new_cycle_fd_a,&peer_new_cycle_frame,sizeof(peer_new_cycle_frame));
    if (0 >= ret)
    {
#if 0
        if (CI_TRUE == b_print_new_cycle_syn)
        {
            CILog_Errno("cpu_new_cycle_recv_fail_a");
        }
#endif

        return -1;
    }
    else
    {
        if (CI_FALSE == CINewCycleSyn_Verify(&peer_new_cycle_frame))
        {
            if (CI_TRUE == b_print_new_cycle_syn)
            {
                CIRemoteLog_Write("cpu_new_cycle_recv_fail_a:data_error");
            }
            return -1;
        }
        else if (peer_new_cycle_last_time == peer_new_cycle_frame.time_stamp)
        {
            if (CI_TRUE == b_print_new_cycle_syn)
            {
                CIRemoteLog_Write("cpu_new_cycle_recv_fail_a:timestamp_islast");
            }
            return -1;
        }
        else if (sn_before(peer_new_cycle_frame.sn, CICycleInt_GetCounter()))
        {
            if (CI_TRUE == b_print_new_cycle_syn)
            {
                CIRemoteLog_Write("cpu_new_cycle_recv_fail_a:sn(%#x)<(%#x)", peer_new_cycle_frame.sn, CICycleInt_GetCounter());
            }
            if (SERIES_CHECK != CISeries_GetLocalState() || CI_TRUE != CICycleInt_IsFirstCycle())
            {
                return -1;
            }
        }

        if (sn_after(peer_new_cycle_frame.sn, CICycleInt_GetCounter()))
        {
            CIRemoteLog_Write("cpu_reset_cycle_counter:recv_new_cycle:%#x->%#x", CICycleInt_GetCounter(), peer_new_cycle_frame.sn);
            CICycleInt_SetCounter(peer_new_cycle_frame.sn);
            /*避免挂起*/
            do 
            {
                ret = CICpu_SendNewCycleSyn();
            } while (-1 == ret);
        }
    }
    if (CI_TRUE == b_print_new_cycle_syn)
    {
        CILog_Msg("cpu_new_cycle_recv_success_a:%#x", peer_new_cycle_frame.sn);
    }

    return 0;
}
/*
 功能描述    : 接收新周期反码数据
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年8月2日 17:22:56
*/
static int32_t cpu_recv_complement_new_cycle(void)
{
    static NewCycleSynFrame complement_frame;
    int32_t ret = 0;

    memset(&complement_frame,0,sizeof(complement_frame));

    ret = read(new_cycle_fd_b,&complement_frame,sizeof(complement_frame));
    if (0 >= ret)
    {
#if 0
        if (CI_TRUE == b_print_new_cycle_syn)
        {
            CILog_Errno("cpu_new_cycle_recv_fail_b");
        }
#endif
        return -1;
    }
    else
    {
        /*如果获取到数据则根据反位转换回原数据*/
        CI_BitwiseNot(&complement_frame,&peer_new_cycle_frame,sizeof(peer_new_cycle_frame));

        if (CI_FALSE == CINewCycleSyn_Verify(&peer_new_cycle_frame))
        {
            if (CI_TRUE == b_print_new_cycle_syn)
            {
                CIRemoteLog_Write("cpu_new_cycle_recv_fail_b:data_error");
            }
            return -1;
        }
        else if (peer_new_cycle_last_time == peer_new_cycle_frame.time_stamp)
        {
            if (CI_TRUE == b_print_new_cycle_syn)
            {
                CIRemoteLog_Write("cpu_new_cycle_recv_fail_b:timestamp_islast");
            }
            return -1;
        }
        else if (sn_before(peer_new_cycle_frame.sn, CICycleInt_GetCounter()))
        {
            if (CI_TRUE == b_print_new_cycle_syn)
            {
                CIRemoteLog_Write("cpu_new_cycle_recv_fail_b:sn(%#x)<(%#x)", peer_new_cycle_frame.sn, CICycleInt_GetCounter());
            }
            if (SERIES_CHECK != CISeries_GetLocalState() || CI_TRUE != CICycleInt_IsFirstCycle())
            {
                return -1;
            }
        }
        else if (sn_after(peer_new_cycle_frame.sn, CICycleInt_GetCounter()))
        {
            CIRemoteLog_Write("cpu_reset_cycle_counter:recv_new_cycle:%#x->%#x", CICycleInt_GetCounter(), peer_new_cycle_frame.sn);
            CICycleInt_SetCounter(peer_new_cycle_frame.sn);
            /*避免挂起*/
            do 
            {
                ret = CICpu_SendNewCycleSyn();
            } while (-1 == ret);
        }
    }

    if (CI_TRUE == b_print_new_cycle_syn)
    {
        CILog_Msg("cpu_new_cycle_recv_success_b:%#x", peer_new_cycle_frame.sn);
    }

    return 0;
}
/*
功能描述    : 接受CPU新周期同步消息
返回值      : 成功为0，失败为-1
参数        : 无
作者        : 张彦升
日期        : 2013年10月25日 10:19:57
*/
int32_t CICpu_RecvNewCycleSyn(void)
{
    int32_t ret = 0;

    CIPerformance_Send(PDT_CPU_NEW_CYCLE_R_BEGIN);

#if 0
    if (CI_TRUE == b_print_new_cycle_syn)
    {
        CILog_Msg("cpu_new_cycle_recv_begin");
    }
#endif

    ret = cpu_recv_original_new_cycle();
    if (-1 == ret)
    {
        ret = cpu_recv_complement_new_cycle();
    }
    else
    {
        ioctl(new_cycle_fd_b,DPRAM_IOC_CLEAN_MEMORY);
    }

    if (-1 == ret)
    {
        return -1;
    }
    else
    {
        peer_new_cycle_last_time = peer_new_cycle_frame.time_stamp;
    }

    if (CI_TRUE == b_print_new_cycle_syn)
    {
        CILog_Msg("cpu_new_cycle_recv_end");
    }

    CIPerformance_Send(PDT_CPU_NEW_CYCLE_R_END);

    return 0;
}
/*
 功能描述    : 检查新周期同步是否正确
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2013年12月2日 14:40:51
*/
int32_t CICpu_CheckNewCycleSyn(void)
{
    /*如果是从CPU的话则对电子单元周期号进行更新*/
    if (CPU_STATE_SLAVE == CICpu_GetLocalState())
    {
        CIEeu_AdjustRequstFsn(peer_new_cycle_frame.eeu_this_request_fsn,
                            peer_new_cycle_frame.eeu_last_request_fsn);
    }
    
    return 0;
}
/*
 功能描述    : 是否打印输入信息
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月11日 15:29:15
*/
int32_t CICpu_OpenPrintInputSyn(void)
{
    b_print_input_syn = CI_TRUE;

    return 0;
}
/*
功能描述    : 发送本CPU输入同步数据
返回值      : 成功为0，失败为-1
参数        : 无
作者        : 何境泰
日期        : 2013年10月25日 13:40:37
*/
int32_t CICpu_SendInputSyn(void)
{
    static CIInputSynFrame complement_frame;
    int32_t ret = 0;

    CIPerformance_Send(PDT_CPU_INPUT_T_BEGIN);

    if (CI_TRUE == b_print_input_syn)
    {
        CILog_Msg("cpu_input_send_begin");
    }

    /*填充帧内容*/
    CIInputSyn_Assemble(&self_input_frame);

    ret = write(input_fd_a,&self_input_frame,sizeof(self_input_frame));
    if (0 >= ret)
    {
        if (CI_TRUE == b_print_input_syn)
        {
            CILog_Errno("cpu_input_send_fail_a:%#x",self_input_frame.cycle_counter);
        }

        return -1;
    }
    else
    {
        if (CI_TRUE == b_print_input_syn)
        {
            CILog_Msg("cpu_input_send_success_a:%#x", self_input_frame.cycle_counter);
        }
    }

    /*send_b:*/
    memset(&complement_frame,0,sizeof(complement_frame));

    /*得到反码数据*/
    CI_BitwiseNot(&self_input_frame,&complement_frame,sizeof(complement_frame));

    ret = write(input_fd_b,&complement_frame,sizeof(complement_frame));
    if (0 >= ret)
    {
        if (CI_TRUE == b_print_input_syn)
        {
            CILog_Errno("cpu_input_send_fail_b:%#x", self_input_frame.cycle_counter);
        }

        return -1;
    }
    else
    {
        if (CI_TRUE == b_print_input_syn)
        {
            CILog_Msg("cpu_input_send_success_b:%#x", self_input_frame.cycle_counter);
        }
    }

    if (CI_TRUE == b_print_input_syn)
    {
        CILog_Msg("cpu_input_send_end:%#x", self_input_frame.cycle_counter);
    }

    CIPerformance_Send(PDT_CPU_INPUT_T_END);

    return 0;
}
/*
 功能描述    : 接收输入源码数据
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年8月2日 17:18:20
*/
static int32_t cpu_recv_original_input(void)
{
    int32_t ret = 0;

    memset(&peer_input_frame,0,sizeof(peer_input_frame));

    ret = read(input_fd_a,&peer_input_frame,sizeof(peer_input_frame));
    if (0 >= ret)
    {
#if 0
        if (CI_TRUE == b_print_input_syn)
        {
            CILog_Errno("cpu_input_recv_fail_a");
        }
#endif
        return -1;
    }
    else
    {
        if (CI_FALSE == CIInputSyn_Verify(&peer_input_frame))
        {
            if (CI_TRUE == b_print_input_syn)
            {
                CIRemoteLog_Write("cpu_input_recv_fail_a:data_error");
            }
            return -1;
        }
        else if (peer_input_last_time == peer_input_frame.time_stamp)
        {
            if (CI_TRUE == b_print_input_syn)
            {
                CIRemoteLog_Write("cpu_input_recv_fail_a:timestamp_islast");
            }
            return -1;
        }
        else if (sn_before(peer_input_frame.cycle_counter, CICycleInt_GetCounter()))
        {
            if (CI_TRUE == b_print_input_syn)
            {
                CIRemoteLog_Write("cpu_input_recv_fail_a:sn(%#x) < (%#x)", peer_input_frame.cycle_counter, CICycleInt_GetCounter());
            }

            if (SERIES_CHECK != CISeries_GetLocalState() || CI_TRUE != CICycleInt_IsFirstCycle())
            {
                return -1;
            }
        }

        if (sn_after(peer_input_frame.cycle_counter, CICycleInt_GetCounter()))
        {
            CIRemoteLog_Write("cpu_reset_cycle_counter:recv_input:%#x->%#x", CICycleInt_GetCounter(), peer_input_frame.cycle_counter);
            CICycleInt_SetCounter(peer_input_frame.cycle_counter);
            /*避免挂起*/
            do 
            {
                ret = CICpu_SendInputSyn();
            } while (-1 == ret);
        }
    }

    if (CI_TRUE == b_print_input_syn)
    {
        CILog_Msg("cpu_input_recv_success_a:%#x", peer_input_frame.cycle_counter);
    }

    return 0;
}
/*
 功能描述    : 接收反码数据
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年8月2日 17:19:09
*/
static int32_t cpu_recv_complement_input(void)
{
    static CIInputSynFrame complement_frame;
    int32_t ret = 0;

    memset(&complement_frame,0,sizeof(complement_frame));

    ret = read(input_fd_b,&complement_frame,sizeof(complement_frame));
    if (0 >= ret)
    {
#if 0
        if (CI_TRUE == b_print_input_syn)
        {
            CILog_Errno("cpu_input_recv_fail_b");
        }
#endif
        return -1;
    }
    else
    {
        /*如果获取到数据则根据反位转换回原数据*/
        CI_BitwiseNot(&complement_frame,&peer_input_frame,sizeof(peer_input_frame));

        if (CI_FALSE == CIInputSyn_Verify(&peer_input_frame))
        {
            if (CI_TRUE == b_print_input_syn)
            {
                CIRemoteLog_Write("cpu_input_recv_fail_b:data_error");
            }
            return -1;
        }
        else if (peer_input_last_time == peer_input_frame.time_stamp)
        {
            if (CI_TRUE == b_print_input_syn)
            {
                CIRemoteLog_Write("cpu_input_recv_fail_b:timestamp_islast");
            }
            return -1;
        }
        else if (sn_before(peer_input_frame.cycle_counter, CICycleInt_GetCounter()))
        {
            if (CI_TRUE == b_print_input_syn)
            {
                CIRemoteLog_Write("cpu_input_recv_fail_b:sn(%#x)<(%#x)", peer_input_frame.cycle_counter, CICycleInt_GetCounter());
            }
            if (SERIES_CHECK != CISeries_GetLocalState() || CI_TRUE != CICycleInt_IsFirstCycle())
            {
                return -1;
            }
        }
        else if (sn_after(peer_input_frame.cycle_counter, CICycleInt_GetCounter()))
        {
            CIRemoteLog_Write("cpu_reset_cycle_counter:recv_input:%#x->%#x", CICycleInt_GetCounter(), peer_input_frame.cycle_counter);
            CICycleInt_SetCounter(peer_input_frame.cycle_counter);
            /*避免挂起*/
            do 
            {
                ret = CICpu_SendInputSyn();
            } while (-1 == ret);
        }
    }

    if (CI_TRUE == b_print_input_syn)
    {
        CILog_Msg("cpu_input_recv_success_b:%#x", peer_input_frame.cycle_counter);
    }

    return 0;
}
/*
功能描述    : 接受它cpu输入同步数据
返回值      : 成功为0，失败为-1
参数        : 无
作者        : 何境泰
日期        : 2013年10月29日 9:24:04
*/
int32_t CICpu_RecvInputSyn(void)
{
    int32_t ret = 0;

    CIPerformance_Send(PDT_CPU_INPUT_R_BEGIN);

#if 0
    if (CI_TRUE == b_print_input_syn)
    {
        CILog_Msg("cpu_input_recv_begin:%#x",CICycleInt_GetCounter());
    }
#endif

    ret = cpu_recv_original_input();
    if (-1 == ret)
    {
        ret = cpu_recv_complement_input();
    }
    else
    {
        ioctl(input_fd_b,DPRAM_IOC_CLEAN_MEMORY);
    }

    if (-1 == ret)
    {
        return -1;
    }
    else
    {
        peer_input_last_time = peer_input_frame.time_stamp;

    }

    if (CI_TRUE == b_print_input_syn)
    {
        CILog_Msg("cpu_input_recv_end:%#x", peer_input_frame.cycle_counter);
    }

    CIPerformance_Send(PDT_CPU_INPUT_R_END);

    return 0;
}

/*
功能描述    : 检查它cpu的输入数据和自己是否一致
返回值      : 成功为0，失败为-1
参数        : 无
作者        : 何境泰
日期        : 2013年10月29日 9:30:51
*/
int32_t CICpu_CheckInputSyn(void)
{
    int32_t ret = 0;

    if (CI_TRUE == b_print_input_syn)
    {
        CILog_Msg("cpu_input_compare_begin:%#x",CICycleInt_GetCounter());
    }
    /*填充帧内容*/
    CIInputSyn_Assemble(&self_input_frame);

    ret = CIinputSyn_Compare(self_input_frame,peer_input_frame);

    if (-1 == ret)
    {
        return -1;
    }
    else
    {
        if (CI_TRUE == b_print_input_syn)
        {
            CILog_Msg("cpu_input_compare_end:%#x",CICycleInt_GetCounter());
        }
    }

    return ret;
}


/*
功能描述    : 数据容错函数，该函数很少会执行到，所以需要仔细测试
返回值      : 成功为0，失败为-1
参数        : 无
作者        : 何境泰
日期        : 2013年12月25日 8:50:19
*/
int32_t CICpu_FaultTolerantInputSyn(void)
{
    int ret = 0;

    if (CI_TRUE == b_print_input_syn)
    {
        CILog_Msg("cpu_input_tolerant_begin");
    }

    /*只替换本帧的数据，还是替换系统动态内存当中的所有数据？遗留*/
    ret = CIinputSyn_Replace(&peer_input_frame);

    if (CI_TRUE == b_print_input_syn)
    {
        CILog_Msg("cpu_input_tolerant_end");
    }

    return ret;
}
/*
 功能描述    : 打开结果信息的输出
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月11日 15:29:15
*/
int32_t CICpu_OpenPrintResultSyn(void)
{
    b_print_result_syn = CI_TRUE;

    return 0;
}


/*
功能描述    : 发送本cpu输出结果
返回值      : 成功为0，失败为-1
参数        : 无
作者        : 何境泰
日期        : 2013年10月29日 9:42:40
*/
int32_t CICpu_SendResultSyn(void)
{
    static CIResultSynFrame complement_frame;
    int32_t ret = 0;

    CIPerformance_Send(PDT_CPU_RESULT_T_BEGIN);

    if (CI_TRUE == b_print_result_syn)
    {
        CILog_Msg("cpu_result_send_begin:%#x",CICycleInt_GetCounter());
    }

    /*对帧中的数据进行装配*/
    CIResultSyn_Assemble(&self_result_frame);

    ret = write(result_fd_a, &self_result_frame, sizeof(self_result_frame));
    if (0 >= ret)
    {
        if (CI_TRUE == b_print_result_syn)
        {
            CILog_Errno("cpu_result_send_fail_a:%#x", self_result_frame.cycle_counter);
        }

        return -1;
    }
    else
    {
        if (CI_TRUE == b_print_result_syn)
        {
            CILog_Msg("cpu_result_send_success_a:%#x", self_result_frame.cycle_counter);
        }
    }

    /*send_b:*/
    memset(&complement_frame,0,sizeof(complement_frame));

    /*得到反码数据*/
    CI_BitwiseNot(&self_result_frame,&complement_frame,sizeof(complement_frame));

    ret = write(result_fd_b, &complement_frame, sizeof(complement_frame));
    if (0 >= ret)
    {
        if (CI_TRUE == b_print_result_syn)
        {
            CILog_Errno("cpu_result_send_fail_b:%#x", self_result_frame.cycle_counter);
        }

        return -1;
    }
    else
    {
        if (CI_TRUE == b_print_result_syn)
        {
            CILog_Msg("cpu_result_send_success_b:%#x", self_result_frame.cycle_counter);
        }
    }

    if (CI_TRUE == b_print_result_syn)
    {
        CILog_Msg("cpu_result_send_end:%#x",CICycleInt_GetCounter());
    }

    CIPerformance_Send(PDT_CPU_RESULT_T_END);

    return 0;
}

/*
 功能描述    : 接收结果原码数据
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年8月2日 17:26:23
*/
static int32_t cpu_recv_original_result(void)
{
    int32_t ret = 0;

    memset(&peer_result_frame,0,sizeof(peer_result_frame));

    ret = read(result_fd_a,&peer_result_frame,sizeof(peer_result_frame));
    if (0 >= ret)
    {
#if 0
        if (CI_TRUE == b_print_result_syn)
        {
            CILog_Errno("cpu_result_recv_fail_a:%#x",CICycleInt_GetCounter());
        }
#endif
        return -1;
    }
    else
    {
        if (CI_FALSE == CIResultSyn_Verify(&peer_result_frame))
        {
            if (CI_TRUE == b_print_result_syn)
            {
                CIRemoteLog_Write("cpu_result_recv_fail_a:data_error");
            }
            return -1;
        }
        else if (peer_result_last_time == peer_result_frame.time_stamp)
        {
            if (CI_TRUE == b_print_result_syn)
            {
                CIRemoteLog_Write("cpu_result_recv_fail_a:timestamp_islast");
            }

            return -1;
        }
        else if (sn_before(peer_result_frame.cycle_counter, CICycleInt_GetCounter()))
        {
            if (CI_TRUE == b_print_result_syn)
            {
                CIRemoteLog_Write("cpu_result_recv_fail_a:sn(%#x)<(%#x)", peer_result_frame.cycle_counter);
            }
            if (SERIES_CHECK != CISeries_GetLocalState() || CI_TRUE != CICycleInt_IsFirstCycle())
            {
                return -1;
            }
        }

        if (sn_after(peer_result_frame.cycle_counter, CICycleInt_GetCounter()))
        {
            CIRemoteLog_Write("cpu_reset_cycle_counter:recv_result:%#x->%#x", CICycleInt_GetCounter(), peer_result_frame.cycle_counter);
            CICycleInt_SetCounter(peer_result_frame.cycle_counter);
            /*避免挂起*/
            do 
            {
                ret = CICpu_SendResultSyn();
            } while (-1 == ret);
        }
    }
    if (CI_TRUE == b_print_result_syn)
    {
        CILog_Msg("cpu_result_recv_success_a:%#x", peer_result_frame.cycle_counter);
    }

    return 0;
}
/*
 功能描述    : 接收结果反码数据
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年8月2日 17:26:23
*/
static int32_t cpu_recv_complement_result(void)
{
    static CIResultSynFrame complement_frame;
    int32_t ret = 0;

    memset(&complement_frame,0,sizeof(complement_frame));

    ret = read(result_fd_b,&complement_frame,sizeof(complement_frame));
    if (0 >= ret)
    {
#if 0
        if (CI_TRUE == b_print_result_syn)
        {
            CILog_Errno("cpu_result_recv_fail_b:",CICycleInt_GetCounter());
        }
#endif
        return -1;
    }
    else
    {
        /*如果获取到数据则根据反位转换回原数据*/
        CI_BitwiseNot(&complement_frame,&peer_result_frame,sizeof(peer_result_frame));

        if (CI_FALSE == CIResultSyn_Verify(&peer_result_frame))
        {
            if (CI_TRUE == b_print_result_syn)
            {
                CIRemoteLog_Write("cpu_result_recv_fail_b:data_error");
            }
            return -1;
        }
        else if (peer_result_last_time == peer_result_frame.time_stamp)
        {
            if (CI_TRUE == b_print_result_syn)
            {
                CIRemoteLog_Write("cpu_result_recv_fail_b:timestamp_islast");
            }

            return -1;
        }
        if (sn_before(peer_result_frame.cycle_counter, CICycleInt_GetCounter()))
        {
            if (CI_TRUE == b_print_result_syn)
            {
                CIRemoteLog_Write("cpu_result_recv_fail_b:sn(%#x)<(%#x)", peer_result_frame.cycle_counter, CICycleInt_GetCounter());
            }
            if (SERIES_CHECK != CISeries_GetLocalState() || CI_TRUE != CICycleInt_IsFirstCycle())
            {
                return -1;
            }
        }
        else if (sn_after(peer_result_frame.cycle_counter, CICycleInt_GetCounter()))
        {
            CIRemoteLog_Write("cpu_reset_cycle_counter:recv_result:%#x->%#x", CICycleInt_GetCounter(), peer_result_frame.cycle_counter);
            CICycleInt_SetCounter(peer_result_frame.cycle_counter);
            /*避免挂起*/
            do 
            {
                ret = CICpu_SendResultSyn();
            } while (-1 == ret);
        }
    }

    if (CI_TRUE == b_print_result_syn)
    {
        CILog_Msg("cpu_result_recv_success_b:%#x", peer_result_frame.cycle_counter);
    }

    return 0;
}
/*
功能描述    : 接收它cpu输出结果
返回值      : 成功为0，失败为-1
参数        : 无
作者        : 何境泰
日期        : 2013年10月29日 9:45:45
*/
int32_t CICpu_RecvResultSyn(void)
{
    int32_t ret = 0;

    CIPerformance_Send(PDT_CPU_RESULT_R_BEGIN);

#if 0
    if (CI_TRUE == b_print_result_syn)
    {
        CILog_Msg("cpu_result_recv_begin:%#x",CICycleInt_GetCounter());
    }
#endif

    ret = cpu_recv_original_result();
    if (-1 == ret)
    {
        ret = cpu_recv_complement_result();
    }
    else
    {
        ioctl(result_fd_b,DPRAM_IOC_CLEAN_MEMORY);
    }

    if (-1 == ret)
    {
        return -1;
    }
    else
    {
        peer_result_last_time = peer_result_frame.time_stamp;
    }

    if (CI_TRUE == b_print_result_syn)
    {
        CILog_Msg("cpu_result_recv_end:%#x",CICycleInt_GetCounter());
    }

    CIPerformance_Send(PDT_CPU_RESULT_R_END);

    return 0;
}



/*
功能描述    : 检查它cpu的结果数据和自己是否一致
返回值      : 成功为0，失败为-1
参数        : 无
作者        : 何境泰
日期        : 2013年10月29日 9:48:26
*/
int32_t CICpu_CheckResultSyn(void)
{
    int ret = 0;

    if (CI_TRUE == b_print_result_syn)
    {
        CILog_Msg("cpu_result_compare_begin:%#x",CICycleInt_GetCounter());
    }

    CIResultSyn_Assemble(&self_result_frame);

    ret = CIResultSyn_Compare(self_result_frame,peer_result_frame);

    if (-1 == ret)
    {
        CI_DumpToFile("./cpu_peer_input_frame",&peer_input_frame,sizeof(peer_input_frame));
        CI_DumpToFile("./cpu_self_input_frame",&self_input_frame,sizeof(self_input_frame));

        CI_DumpToFile("./cpu_peer_result_frame",&peer_result_frame,sizeof(peer_result_frame));
        CI_DumpToFile("./cpu_self_result_frame",&self_result_frame,sizeof(self_result_frame));

        return -1;
    }

    if (CI_TRUE == b_print_result_syn)
    {
        CILog_Msg("cpu_result_compare_end:%#x",CICycleInt_GetCounter());
    }

    return 0;
}
/*
 功能描述    : 打开心跳信号记录标志
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月11日 15:28:00
*/
int32_t CICpu_OpenPrintHeartbeat(void)
{
    b_print_heartbeat = CI_TRUE;

    return 0;
}
/*
 功能描述    : 数据装配
 返回值      : 成功为0，失败为-1
 参数        : @p_frame 要装配的帧
              @sn 帧序号
 作者        : 张彦升
 日期        : 2014年4月15日 13:46:29
*/
static int32_t cpu_assemble_heartbeat_data(CpuHeartBeatFrame* p_frame,uint32_t sn)
{
    p_frame->type = ST_HEARTBEAT;
    p_frame->sn = sn;
    p_frame->time_stamp = CI_GetTimeStamp();
    p_frame->elapse = CITimer_GetElapsedCycles();
    p_frame->series_state = (uint8_t)CISeries_GetLocalState();
    p_frame->cpu_state = (uint8_t)CICpu_GetLocalState();

    /*计算crc码*/
    CICpuHeartbeat_CalcHash(p_frame);

    return 0;
}
/*
功能描述    : 发送CPU心跳信号
返回值      : 成功为0，失败为-1
参数        : 无
作者        : 张彦升
日期        : 2013年10月28日 9:09:52
*/
int32_t CICpu_SendHeartbeat(void)
{
    static CpuHeartBeatFrame complement_frame;
    static uint32_t sn = 1;
    int32_t ret = 0;

    CIPerformance_SigSend(PDT_CPU_HEARTBEAT_T_BEGIN);

    if (CI_TRUE == b_print_heartbeat)
    {
        CILog_SigMsg("cpu_heartbeat_send_begin:%#x",sn);
    }

    /*对帧中的数据进行装配*/
    ret = cpu_assemble_heartbeat_data(&self_beat_frame,sn);

    ret = write(heartbeat_fd_a,&self_beat_frame,sizeof(self_beat_frame));
    if (0 >= ret)
    {
        if (CI_TRUE == b_print_heartbeat)
        {
            CILog_SigErrno("cpu_heartbeat_send_fail_a:%#x", self_beat_frame.sn);
        }

        return -1;
    }
    else
    {
        if (CI_TRUE == b_print_heartbeat)
        {
            CILog_SigMsg("cpu_heartbeat_send_success_a:%#x", self_beat_frame.sn);
        }
    }

    /*send_b*/
    memset(&complement_frame,0,sizeof(complement_frame));

    /*得到反码数据*/
    CI_BitwiseNot(&self_beat_frame,&complement_frame,sizeof(complement_frame));

    ret = write(heartbeat_fd_b,&complement_frame,sizeof(complement_frame));
    if (0 >= ret)
    {
        if (CI_TRUE == b_print_heartbeat)
        {
            CILog_SigErrno("cpu_heartbeat_send_fail_b:%#x", self_beat_frame.sn);
        }

        return -1;
    }
    else
    {
        if (CI_TRUE == b_print_heartbeat)
        {
            CILog_SigMsg("cpu_heartbeat_send_success_b:%#x", self_beat_frame.sn);
        }
    }

    if (CI_TRUE == b_print_heartbeat)
    {
        CILog_SigMsg("cpu_heartbeat_send_end:%#x",sn);
    }
    sn += 1;
    CIPerformance_SigSend(PDT_CPU_HEARTBEAT_T_END);

    return 0;
}
/*
 功能描述    : 接收原码心跳数据
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年8月2日 17:13:04
*/
static int32_t cpu_recv_original_heartbeat_syn(void)
{
    int32_t ret = 0;

    memset(&peer_beat_frame,0,sizeof(peer_beat_frame));

    ret = read(heartbeat_fd_a,&peer_beat_frame,sizeof(peer_beat_frame));
    if (0 >= ret)
    {
        if (CI_TRUE == b_print_heartbeat)
        {
            CILog_SigErrno("cpu_heartbeat_recv_fail_a");
        }

        return -1;
    }
    else
    {
        if (CI_FALSE == CICpuHeartbeat_Verify(&peer_beat_frame))
        {
            if (CI_TRUE == b_print_heartbeat)
            {
                CILog_SigMsg("cpu_heartbeat_recv_fail_a:data_error");
            }
            return -1;
        }
        else if (peer_heartbeat_last_sn == peer_beat_frame.sn)
        {
            if (CI_TRUE == b_print_heartbeat)
            {
                CILog_SigMsg("cpu_heartbeat_recv_fail_a:sn_islast:%#x", peer_beat_frame.sn);
            }
            return -1;
        }
        else if (peer_heartbeat_last_time == peer_beat_frame.time_stamp)
        {
            if (CI_TRUE == b_print_heartbeat)
            {
                CILog_SigMsg("cpu_heartbeat_recv_fail_a:timestamp_islast");
            }
            return -1;
        }
    }
    if (CI_TRUE == b_print_heartbeat)
    {
        CILog_SigMsg("cpu_heartbeat_recv_success_a:%#x", peer_beat_frame.sn);
    }

    return 0;
}
/*
 功能描述    : 接收反码心跳数据
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年8月2日 17:13:04
*/
static int32_t cpu_recv_complement_heartbeat_syn(void)
{
    static CpuHeartBeatFrame complement_frame;
    int32_t ret = 0;

    memset(&complement_frame,0,sizeof(complement_frame));

    ret = read(heartbeat_fd_b,&complement_frame,sizeof(complement_frame));
    if (0 >= ret)
    {
        if (CI_TRUE == b_print_heartbeat)
        {
            CILog_SigErrno("cpu_heartbeat_recv_fail_b");
        }
        return -1;
    }
    else
    {
        /*如果获取到数据则根据反位转换回原数据*/
        CI_BitwiseNot(&complement_frame,&peer_beat_frame,sizeof(peer_beat_frame));

        if (CI_TRUE == b_print_heartbeat)
        {
            CILog_SigMsg("cpu_heartbeat_recv_success_b:%#x", peer_beat_frame.sn);
        }

        if (CI_FALSE == CICpuHeartbeat_Verify(&peer_beat_frame))
        {
            if (CI_TRUE == b_print_heartbeat)
            {
                CILog_SigMsg("cpu_heartbeat_recv_fail_b:data_error");
            }
            return -1;
        }
        else if (peer_heartbeat_last_sn == peer_beat_frame.sn)
        {
            if (CI_TRUE == b_print_heartbeat)
            {
                CILog_SigMsg("cpu_heartbeat_recv_fail_b:sn_islast:%#x", peer_beat_frame.sn);
            }

            return -1;
        }
        else if (peer_heartbeat_last_time == peer_beat_frame.time_stamp)
        {
            if (CI_TRUE == b_print_heartbeat)
            {
                CILog_SigMsg("cpu_heartbeat_recv_fail_b:timestamp_islast");
            }

            return -1;
        }
    }

    return 0;
}
/*
功能描述    : 接受CPU心跳信号
返回值      : 成功为0，失败为-1
参数        : 无
作者        : 张彦升
日期        : 2013年10月28日 9:10:05
*/
int32_t CICpu_RecvHeartbeat(void)
{
    int32_t ret = 0;

    CIPerformance_SigSend(PDT_CPU_HEARTBEAT_R_BEGIN);

    if (CI_TRUE == b_print_heartbeat)
    {
        CILog_SigMsg("cpu_heartbeat_recv_begin");
    }

    ret = cpu_recv_original_heartbeat_syn();
    if (-1 == ret)
    {
        ret = cpu_recv_complement_heartbeat_syn();
    }
    else
    {
        ioctl(heartbeat_fd_b,DPRAM_IOC_CLEAN_MEMORY);
    }

    if (-1 == ret)
    {
        return -1;
    }
    else
    {
        peer_heartbeat_last_sn = peer_beat_frame.sn;
        peer_heartbeat_last_time = peer_beat_frame.time_stamp;
    }

    if (CI_TRUE == b_print_heartbeat)
    {
        CILog_SigMsg("cpu_heartbeat_recv_end:%#x", peer_beat_frame.sn);
    }

    CIPerformance_SigSend(PDT_CPU_HEARTBEAT_R_END);

    return 0;
}
/*
功能描述    : 检查CPU心跳信号是否正确
返回值      : 成功为0，失败为-1
参数        : 无
作者        : 张彦升
日期        : 2013年12月2日 14:40:16
*/
int32_t CICpu_CheckHearbeat(void)
{
    peer_cpu_series_state = peer_beat_frame.series_state;
    /*从CPU跟随主CPU的双系状态*/
    if (CPU_STATE_SLAVE == CICpu_GetLocalState())
    {
        CISeries_SlaveCpuAdjustState();
    }
    /*检查双CPU状态*/
    if (peer_beat_frame.cpu_state == self_beat_frame.cpu_state)
    {
        assert(CPU_STATE_NONE != peer_beat_frame.cpu_state);

        if (CPU_STATE_MASTER == peer_beat_frame.cpu_state)
        {
            CIFailureInt_SetInfo(FAILURE_DUAL_MASTER_CPU,"双CPU心跳检测出双主CPU故障");
        }
        else if(CPU_STATE_SLAVE == peer_beat_frame.cpu_state)
        {
            CIFailureInt_SetInfo(FAILURE_DUAL_SLAVE_CPU,"双CPU心跳检测出双从CPU故障");
        }

        return -1;
    }

    if (CPU_STATE_SLAVE == CICpu_GetLocalState())
    {
        /* 如果周期相差两个周期的话则进行校正
         * 中断周期号跟随方式为: 
         * 1.主系从CPU跟随主系主CPU
         * 2.备系主CPU跟随主系主CPU
         * 3.备系从CPU跟随主系主CPU，再跟随备系主CPU
         */
        if (0 != CITimer_GetElapsedCycles() % CI_CYCLE_RELATIVE
            && 1 <= CITimer_RoundSub(peer_beat_frame.elapse,CITimer_GetElapsedCycles()))
        {
            CITimer_SetElapsedCycles(peer_beat_frame.elapse);
        }
    }

    return 0;
}
/*
 * 检验双CPU之间的双口ram是否工作正常
 */
static int32_t cpu_validate_hardware(int32_t fd)
{
    if(0 > ioctl(fd,DPRAM_IOC_CHECK_MEMORY))
    {
        CIRemoteLog_Write("cpu dpram hardware check wrong.please check circuit board plug ok.");

        return -1;
    }
    return 0;
}
/*
 * 重新设置双CPU之间双口ram的空间分配
 */
static int32_t cpu_reset_dpram_mem(int32_t fd)
{
    int32_t ret = 0;

    ret = ioctl(fd,DPRAM_IOCRESET);
    if (-1 == ret)
    {
        CIRemoteLog_Write("use ioctl reset cpu dpram memory failed\n");

        return -1;
    }

    return 0;
}
/*
 功能描述    : 初始化CPU同步期间使用的fd
             cfg_fd使用dpram1a
             new_cycle_fd使用dpram2a
             input_fd使用dpram3a
             output_fd使用dpram4a
             heartbeat_fd使用dpram5a
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2013年12月1日 10:33:22
*/
static int32_t cpu_fd_init(void)
{
    int32_t ret = 0;

    /*heartbeat_fd_a*/
    heartbeat_fd_a = open("/dev/dpram1a",O_RDWR);

    if (-1 == heartbeat_fd_a)
    {
        CILog_Errno("open /dev/dpram1a failed");

        return -1;
    }
    if(0 > cpu_validate_hardware(heartbeat_fd_a))
    {
        return -1;
    }
    if (0 > cpu_reset_dpram_mem(heartbeat_fd_a))
    {
        return -1;
    }

    if (-1 == ioctl(heartbeat_fd_a,DPRAM_IOC_ALLOC_MEMORY,sizeof(CpuHeartBeatFrame)))
    {
        CILog_Errno("alloc heartbeat_fd_a memory failed");

        return -1;
    }
    /*heartbeat_fd_b*/
    heartbeat_fd_b = open("/dev/dpram1b",O_RDWR);

    if (-1 == heartbeat_fd_b)
    {
        CILog_Errno("open /dev/dpram1b failed");

        return -1;
    }

    ret = ioctl(heartbeat_fd_b,DPRAM_IOC_ALLOC_MEMORY,sizeof(CpuHeartBeatFrame));
    if (-1 == ret)
    {
        CILog_Errno("alloc heartbeat_fd_b memory failed");

        return -1;
    }
    /*cfg_fd_a*/
    cfg_fd_a = open("/dev/dpram2a",O_RDWR);

    if (-1 == cfg_fd_a)
    {
        CILog_Errno("open /dev/dpram2a failed");

        return -1;
    }

    ret = ioctl(cfg_fd_a,DPRAM_IOC_ALLOC_MEMORY,sizeof(CfgSynFrame));

    if (-1 == ret)
    {
        CILog_Errno("alloc cfg_fd_a memory failed");

        return -1;
    }
    /*cfg_fd_b*/
    cfg_fd_b = open("/dev/dpram2b",O_RDWR);

    if (-1 == cfg_fd_b)
    {
        CILog_Errno("open /dev/dpram2b failed");

        return -1;
    }

    ret = ioctl(cfg_fd_b,DPRAM_IOC_ALLOC_MEMORY,sizeof(CfgSynFrame));

    if (-1 == ret)
    {
        CILog_Errno("alloc cfg_fd_b memory failed");

        return -1;
    }

    /*new_cycle_fd_a*/
    new_cycle_fd_a = open("/dev/dpram3a",O_RDWR);
    if (-1 == new_cycle_fd_a)
    {
        CILog_Errno("open /dev/dpram3a failed");

        return -1;
    }

    ret = ioctl(new_cycle_fd_a,DPRAM_IOC_ALLOC_MEMORY,sizeof(NewCycleSynFrame));
    if (-1 == ret)
    {
        CILog_Errno("alloc new_cycle_fd_a memory failed");
        return -1;
    }
    /*new_cycle_fd_b*/
    new_cycle_fd_b = open("/dev/dpram3b",O_RDWR);
    if (-1 == new_cycle_fd_b)
    {
        CILog_Errno("open /dev/dpram3b failed");

        return -1;
    }

    ret = ioctl(new_cycle_fd_b,DPRAM_IOC_ALLOC_MEMORY,sizeof(NewCycleSynFrame));
    if (-1 == ret)
    {
        CILog_Errno("alloc new_cycle_fd_b memory failed");
        return -1;
    }

    /*input_fd_a*/
    input_fd_a = open("/dev/dpram4a",O_RDWR);

    if (-1 == input_fd_a)
    {
        CILog_Errno("open /dev/dpram4a failed");

        return -1;
    }

    ret = ioctl(input_fd_a,DPRAM_IOC_ALLOC_MEMORY,sizeof(CIInputSynFrame));
    if (-1 == ret)
    {
        CILog_Errno("alloc input_fd_a memory failed");

        return -1;
    }
    /*input_fd_b*/
    input_fd_b = open("/dev/dpram4b",O_RDWR);

    if (-1 == input_fd_b)
    {
        CILog_Errno("open /dev/dpram4b failed");

        return -1;
    }

    ret = ioctl(input_fd_b,DPRAM_IOC_ALLOC_MEMORY,sizeof(CIInputSynFrame));
    if (-1 == ret)
    {
        CILog_Errno("alloc input_fd_b memory failed");

        return -1;
    }

    /*result_fd_a*/
    result_fd_a = open("/dev/dpram5a",O_RDWR);

    if (-1 == result_fd_a)
    {
        CILog_Errno("open /dev/dpram5a failed");

        return -1;
    }

    ret = ioctl(result_fd_a,DPRAM_IOC_ALLOC_MEMORY,sizeof(CIResultSynFrame));
    if (-1 == ret)
    {
        CILog_Errno("alloc result_fd_a memory failed");

        return -1;
    }
    /*result_fd_b*/
    result_fd_b = open("/dev/dpram5b",O_RDWR);

    if (-1 == result_fd_b)
    {
        CILog_Errno("open /dev/dpram5b failed");

        return -1;
    }

    ret = ioctl(result_fd_b,DPRAM_IOC_ALLOC_MEMORY,sizeof(CIResultSynFrame));
    if (-1 == ret)
    {
        CILog_Errno("alloc result_fd_b memory failed");

        return -1;
    }

    return 0;
}
/*
功能描述    : CPU初始化
返回值      : 成功为0，失败为-1
参数        : 无
作者        : 张彦升
日期        : 2013年10月29日 15:36:48
*/
int32_t CICpu_Init(void)
{
    static CI_BOOL b_initialized = CI_FALSE;
    int ret = 0;

    if (CI_TRUE == b_initialized)
    {
        return -1;
    }

    ret = cpu_validate_relationship();
    assert(-1 != ret);

    ret = cpu_fd_init();
    assert(-1 != ret);

    ret = CICpuHeartbeat_Init(&self_beat_frame);
    assert(-1 != ret);

    ret = CINewCycleSyn_Init(&self_new_cycle_frame);
    assert(-1 != ret);

    ret = CIInputSyn_Init(&self_input_frame);
    assert(-1 != ret);

    ret = CIResultSyn_Init(&self_result_frame);
    assert(-1 != ret);

    ret = CICfgSyn_Init(&self_cfg_frame);
    assert(-1 != ret);

    b_initialized = CI_TRUE;

    return 0;
}

#endif /*!LINUX_ENVRIONMENT*/
