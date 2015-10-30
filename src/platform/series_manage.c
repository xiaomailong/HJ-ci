/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年10月29日 15:40:45
用途        : 双系管理
历史修改记录: v1.0    创建
             v1.1    使用网络进行通信
             v1.2    2014.07.31 张彦升
               备系校核阶段使用网络进行通信，进入热备阶段使用光纤通信
**********************************************************************/
#include "util/ci_header.h"

#ifdef LINUX_ENVRIONMENT

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

#include "util/ci_header.h"
#include "util/log.h"

#include "series_manage.h"
#include "cpu_manage.h"
#include "failures_int.h"
#include "led_int.h"
#include "hmi_manage.h"
#include "cycle_int.h"
#include "performance.h"
#include "timer.h"
#include "cfg_manage.h"
#include "eeu_manage.h"
#include "switch_board_manage.h"
#include "remote_log.h"

#include "interlocking/inter_api.h"

#include "syn_data/series_heartbeat.h"
#include "syn_data/new_cycle_syn.h"
#include "syn_data/cfg_syn.h"
#include "syn_data/input_syn.h"
#include "syn_data/result_syn.h"
#include "syn_data/syn_type.h"

#include "util/config.h"
#include "util/utility.h"
#include "util/app_config.h"

#define FILE_NAME_MAX_LEN           128               /*文件名的最大长度*/

static SeriesState series_local_state = SERIES_PENDING;     /*本系工作状态*/
static SeriesState series_peer_state = SERIES_PENDING;      /*另外一系工作状态*/

/*是否切换双系标志*/
static const char* series_state_name[] = {
    [SERIES_PENDING]        = "series_pending",
    [SERIES_CHECK]          = "series_check",
    [SERIES_MASTER]         = "series_master",
    [SERIES_STANDBY]        = "series_standby",
};

static NewCycleSynFrame self_new_cycle_frame,peer_new_cycle_frame;
static CIInputSynFrame self_input_frame,peer_input_frame;   //输入
static CIResultSynFrame self_result_frame,peer_result_frame;   //输出
static SeriesHeartBeatFrame self_beat_frame,peer_beat_frame;
static CfgSynFrame self_cfg_frame, peer_cfg_frame;

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

static CI_BOOL b_print_cfg_syn = CI_FALSE;          /*是否打印配置同步信息*/
static CI_BOOL b_print_new_cycle_syn = CI_FALSE;    /*是否打印新周期同步信息*/
static CI_BOOL b_print_input_syn = CI_FALSE;        /*是否打印输入信息*/
static CI_BOOL b_print_result_syn = CI_FALSE;       /*是否打印结果信息*/
static CI_BOOL b_print_heartbeat = CI_FALSE;        /*是否打印心跳信号*/

/*
 * 为了维护方便添加该选项，当备系替换程序之后打开该选项不对配置做比较
 */
static CI_BOOL b_cfg_compare = CI_TRUE;

static uint32_t peer_new_cycle_last_time = -1;
static uint32_t peer_input_last_time = -1;
static uint32_t peer_result_last_time = -1;
static uint32_t peer_heartbeat_last_time = -1;
static uint32_t peer_cfg_last_time = -1;

static uint32_t peer_heartbeat_last_sn = -1;

/*用来标志自身是否延迟*/
static CI_BOOL b_self_delayed = CI_FALSE;

static CI_BOOL b_peer_series_eeu_broke = CI_FALSE;
static CI_BOOL b_peer_series_hmi_broke = CI_FALSE;

#define FIBER_IOC_MAGIC 'm'
#define FIBER_IOC_RESET _IO(FIBER_IOC_MAGIC, 0)
#define FIBER_IOC_ALLOC_MEMORY _IOWR(FIBER_IOC_MAGIC, 1, int)
#define FIBER_IOC_CLEAN_BUFFER _IOWR(FIBER_IOC_MAGIC, 2, int)  /*清除缓存*/

/*
功能描述    : 设置本系的状态，该函数并没有暴漏给外界，我们认为其它是没有权利更改双系状态
             的
返回值      : 成功为0，失败为-1
参数        : 
作者        : 张彦升
日期        : 2013年10月30日 8:41:10
*/
static int32_t series_set_local_state(SeriesState state)
{
    series_local_state = state;

    if (SERIES_MASTER == series_local_state)
    {
        /*先熄灭再点灯*/
        CILedInt_BlankingStandby();
        CILedInt_LightMaster();
    }
    else if (SERIES_STANDBY == series_local_state)
    {
        /*先熄灭再点灯*/
        CILedInt_BlankingMaster();
        CILedInt_LightStandby();
    }

    return 0;
}
/*
功能描述    : 得到本系的状态
返回值      : 成功为0，失败为-1
参数        : 
作者        : 张彦升
日期        : 2013年10月30日 8:41:30
*/
SeriesState CISeries_GetLocalState(void)
{
    return series_local_state;
}
/*
 功能描述    : 设置另外一系的状态
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年7月28日 11:09:00
*/
void CISeries_SetPeerState(SeriesState state)
{
    series_peer_state = state;
    return;
}
/*
 功能描述    : 得到另外一系的状态
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年7月28日 11:09:00
*/
SeriesState CISeries_GetPeerState(void)
{
    return series_peer_state;
}
/*
 功能描述    : 得到状态的名称
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年7月28日 13:25:29
*/
const char* CISeries_GetStateName(SeriesState state)
{
    return series_state_name[state];
}
/*
 功能描述    : 得到双系的id（I或者II）
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年3月14日 18:14:27
*/
const char* CISeries_GetId(void)
{
    return CIConfig_GetValue("Series");
}
/*
功能描述    : 切换至主系
返回值      : 成功为0，失败为-1
参数        : 
作者        : 张彦升
日期        : 2013年10月29日 14:14:54
*/
int32_t CISeries_SwitchToMaster(void)
{
    assert(SERIES_STANDBY == CISeries_GetLocalState());

    CIRemoteLog_Write("series_switch:standby_to_master");
    /*设置自己为主系*/
    series_set_local_state(SERIES_MASTER);

    return 0;
}
/*
功能描述    : 切换至备系
返回值      : 成功为0，失败为-1
参数        : 
作者        : 张彦升
日期        : 2013年10月29日 14:14:54
*/
int32_t CISeries_SwitchToCheck(void)
{
    /*从主机状态至校核状态*/
    assert(SERIES_MASTER == CISeries_GetLocalState());

    CIRemoteLog_Write("series_switch:master_to_check");
    /*设置自己为校核状态*/
    series_set_local_state(SERIES_CHECK);

    return 0;
}
/*
 功能描述    : 切换到热备状态
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年5月23日 13:31:04
*/
int32_t CISeries_SwitchToStandby(void)
{
    /*从校核状态至热备*/
    assert(SERIES_CHECK == CISeries_GetLocalState());

    CIRemoteLog_Write("series_switch:check_to_standby");
    /*设置自己为校核状态*/
    series_set_local_state(SERIES_STANDBY);

    return 0;
}
/*
 功能描述    : 从CPU双系状态调整
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年7月28日 12:56:29
*/
int32_t CISeries_SlaveCpuAdjustState(void)
{
    assert(CPU_STATE_SLAVE == CICpu_GetLocalState());

    /*从CPU跟随另外一个CPU的双系状态，即主CPU的双系状态*/
    if (CICpu_GetPeerSeriesState() != CISeries_GetLocalState())
    {
        series_set_local_state(CICpu_GetPeerSeriesState());
    }

    return 0;
}
/*
 功能描述    : 清除新周期缓存
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年7月18日 9:05:02
*/
static int32_t series_clean_recv_buffer(int32_t fd)
{
    int32_t ret = 0;

    ret = ioctl(fd,FIBER_IOC_CLEAN_BUFFER);

    return ret;
}
/*
 功能描述    : 双系配置比较数据装配函数
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年5月23日 13:20:41
*/
static int32_t series_assemble_cfg_syn(CfgSynFrame* p_frame,uint32_t sn)
{
    assert(NULL != p_frame);

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
    p_frame->series_state = CISeries_GetLocalState();

    /*计算crc码*/
    CICfgSyn_CalcHash(p_frame);

    return 0;
}
/*
功能描述    : 发送配置比较数据
返回值      : 成功为0，失败为-1
参数        : 无
作者        : 张彦升
日期        : 2013年10月30日 13:01:57
*/
int32_t CISeries_SendCfgSyn(void)
{
    static uint32_t sn = 1;
    int32_t ret = 0;

    CIPerformance_Send(PDT_SERIES_CFG_T_BEGIN);

    series_assemble_cfg_syn(&self_cfg_frame,sn);

    if (CI_TRUE == b_print_cfg_syn)
    {
        CILog_Msg("series_cfg_send_begin:%#x",sn);
    }

    ret = write(cfg_fd_a, &self_cfg_frame, sizeof(self_cfg_frame));
    if (0 > ret)
    {
        CIRemoteLog_Write("series_cfg_send_fail:%#x", self_cfg_frame.sn);
        return -1;
    }
    else
    {
        if (CI_TRUE == b_print_cfg_syn)
        {
            CILog_Msg("series_cfg_send_success:%#x", self_cfg_frame.sn);
        }
    }

    if (CI_TRUE == b_print_cfg_syn)
    {
        CILog_Msg("series_cfg_send_end:%#x",sn);
    }

    sn += 1;
    CIPerformance_Send(PDT_SERIES_CFG_T_END);

    return 0;
}
/*
 功能描述    : 接收配置比较数据
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年6月19日 12:36:36
*/
int32_t series_recv_cfg(int32_t fd,const char* fd_name)
{
    if (0 > read(fd, &peer_cfg_frame, sizeof(peer_cfg_frame)))
    {
        if (CI_TRUE == b_print_cfg_syn)
        {
            CILog_Errno("series_%s_recv_fail",fd_name);
        }
        return -1;
    }
    else
    {
        if (CI_FALSE == CICfgSyn_Verify(&peer_cfg_frame))
        {
            if (CI_TRUE == b_print_cfg_syn)
            {
                CIRemoteLog_Write("series_cfg_recv_fail:data_error");
            }
            return -1;
        }
        else if (peer_cfg_last_time == peer_cfg_frame.time_stamp)
        {
            if (CI_TRUE == b_print_cfg_syn)
            {
                CIRemoteLog_Write("series_cfg_recv_fail:timestamp_islast");
            }

            return -1;
        }

        return 0;
    }
}
/*
功能描述    : 接收配置比较数据
返回值      : 收到数据返回0，未收到数据返回-1
参数        : 
作者        : 张彦升
日期        : 2013年10月30日 13:02:17
*/
int32_t CISeries_RecvCfgSyn(void)
{
    CIPerformance_Send(PDT_SERIES_CFG_R_BEGIN);

    if (CI_TRUE == b_print_cfg_syn)
    {
        CILog_Msg("series_cfg_recv_begin");
    }

    if (0 > series_recv_cfg(cfg_fd_a, "cfg_a"))
    {
        if (0 > series_recv_cfg(cfg_fd_b, "cfg_b"))
        {
            return -1;
        }
    }
    else
    {
        series_clean_recv_buffer(cfg_fd_b);
    }

    if (CI_TRUE == b_print_cfg_syn)
    {
        CIRemoteLog_Write("series_cfg_recv_success:%#x", peer_cfg_frame.sn);
    }

    CIPerformance_Send(PDT_SERIES_CFG_R_END);

    return 0;
}
/*
 功能描述    : 检查配置比较数据
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年5月4日 14:16:39
*/
int32_t CISeries_CheckCfgSyn(void)
{
    int32_t ret = 0;

    if (CI_TRUE == b_print_cfg_syn)
    {
        CILog_Msg("series_cfg_compare_begin");
    }

    ret = series_assemble_cfg_syn(&self_cfg_frame,CICycleInt_GetCounter());
    assert(-1 != ret);

    if (self_cfg_frame.series_state == peer_cfg_frame.series_state)
    {
        assert(SERIES_PENDING != peer_cfg_frame.series_state);

        /*
         * 注意，在这里发现双主系故障的时候未做故障处理，因为在校核阶段，所以由
         * 请求校核的一方做故障处理。
         * 值得注意的是，在双系启动期间，由于备系启动请求校核，这导致在各自的双系
         * 通信道缓存当中可能存有上次请求的数据，这个数据也有可能导致报这个错误
         */
        if (SERIES_MASTER == peer_cfg_frame.series_state)
        {
            CIRemoteLog_Write("series_cfg_compare_fail:备系校核检测出双主系故障");
        }
        else if(SERIES_STANDBY == peer_cfg_frame.series_state)
        {
            CIRemoteLog_Write("series_cfg_compare_fail:备系校核检测出双备系故障");
        }

        return -1;
    }
    if (0 != memcmp(self_cfg_frame.station_name,peer_cfg_frame.station_name,STATION_NAME_SIZE))
    {
        CIRemoteLog_Write("series_cfg_compare_fail:双系配置比较站场名称不一致");
        return -1;
    }

    /*为了维护方便在启动的时候部分可以不比较*/
    if (CI_TRUE == b_cfg_compare)
    {
#define SERIES_CFG_STR_CMP(field,len,tip)                                  \
        if (0 != memcmp(self_cfg_frame.field,peer_cfg_frame.field,len)){   \
            CIRemoteLog_Write("series_cfg_compare_fail:"#tip);             \
            return -1;                                                     \
        }

        SERIES_CFG_STR_CMP(ci_md5, MD5_DIGEST_LENGTH, "双系CI主程序md5不一致");
        SERIES_CFG_STR_CMP(ilt_md5, MD5_DIGEST_LENGTH, "双系联锁表的md5值不一致");
        SERIES_CFG_STR_CMP(snt_md5, MD5_DIGEST_LENGTH, "双系信号节点表的md5值不一致");
        SERIES_CFG_STR_CMP(ssi_md5, MD5_DIGEST_LENGTH, "双系ssi文件的md5值不一致");
        SERIES_CFG_STR_CMP(cfg_md5, MD5_DIGEST_LENGTH, "双系ssi文件的md5值不一致");
        SERIES_CFG_STR_CMP(ko_dpram_md5, MD5_DIGEST_LENGTH, "双系双口ram模块的md5不一致");
        SERIES_CFG_STR_CMP(ko_fiber_md5, MD5_DIGEST_LENGTH, "双系光纤模块的md5不一致");
        SERIES_CFG_STR_CMP(ko_uart_md5, MD5_DIGEST_LENGTH, "双系串口模块的md5不一致");
        SERIES_CFG_STR_CMP(ko_can_md5, MD5_DIGEST_LENGTH, "双系can板模块的md5不一致");
        SERIES_CFG_STR_CMP(ko_net_md5, MD5_DIGEST_LENGTH, "双系网口模块的md5不一致");
        SERIES_CFG_STR_CMP(ko_wtd_md5, MD5_DIGEST_LENGTH, "双系看门狗模块的md5不一致");

#undef SERIES_CFG_STR_CMP
    }

    if (CI_TRUE == b_print_cfg_syn)
    {
        CILog_Msg("series_cfg_compare_success");
    }

    return 0;
}
/*
 功能描述    : 打开备系校核同步数据开关
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年5月14日 18:38:59
*/
int32_t CISeries_OpenPrintCfgSyn(void)
{
    b_print_cfg_syn = CI_TRUE;

    return 0;
}
/*
 功能描述    : 关闭启动时比较配置详细信息
 返回值      : 无
 参数        : 无
 作者        : 张彦升
 日期        : 2014年8月21日 14:53:50
*/
void CISeries_CloseCfgCompare(void)
{
    b_cfg_compare = CI_FALSE;

    return;
}
/*
 功能描述    : 打开启动时比较配置详细信息
 返回值      : 无
 参数        : 无
 作者        : 张彦升
 日期        : 2014年8月21日 14:53:50
*/
void CISeries_OpenCfgCompare(void)
{
    b_cfg_compare = CI_TRUE;

    return;
}
/*
功能描述    : 进行配置比较
返回值      : 成功为0，失败为-1
参数        : 
作者        : 张彦升
日期        : 2014年5月4日 13:57:50
*/
int32_t CISeries_CfgSyn(void)
{
    const int32_t try_times = 10;
    int32_t count = 0;
    int ret = 0;

    /*只有进入了校核状态才会进行配置比较*/
    if (SERIES_CHECK != CISeries_GetLocalState() || CPU_STATE_MASTER != CICpu_GetLocalState())
    {
        return 0;
    }

    for(count = 0;count < try_times;count++)
    {
        CIRemoteLog_Write("正在尝试第%d次双系配置比较",count + 1);

        ret = CISeries_SendCfgSyn();
        if (-1 == ret)
        {
            CIRemoteLog_Write("发送双系配置比较请求数据失败");
            return -1;
        }

        usleep(CI_CYCLE_MS * 1000);

        ret = CISeries_RecvCfgSyn();
        if(0 > ret)
        {
            continue;
        }

        ret = CISeries_CheckCfgSyn();
        if (-1 == ret)
        {
            CIRemoteLog_Write("双系配置比较数据不一致");
        }
        else
        {
            /*有一次比较成功即可*/
            break;
        }
    }
    if (try_times == count)
    {
        CIRemoteLog_Write("双系配置比较超时");
        return -1;
    }

    return 0;
}





/*
 功能描述    : 装配新周期同步数据
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年6月16日 10:20:54
*/
static int32_t series_assemble_new_cycle_syn_data(NewCycleSynFrame* p_frame,uint32_t sn)
{
    assert(NULL != p_frame);

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
参数        : 
作者        : 张彦升
日期        : 2013年10月24日 16:10:17
*/
int32_t CISeries_SendNewCycleSyn(void)
{
    CIPerformance_Send(PDT_SERIES_NEW_CYCLE_T_BEGIN);

    if (CI_TRUE == b_print_new_cycle_syn)
    {
        CILog_Msg("series_new_cycle_send_begin:%#x",CICycleInt_GetCounter());
    }

    series_assemble_new_cycle_syn_data(&self_new_cycle_frame,CICycleInt_GetCounter());

    if (0 >= write(new_cycle_fd_a,&self_new_cycle_frame,sizeof(self_new_cycle_frame)))
    {
        if (CI_TRUE == b_print_new_cycle_syn)
        {
            CILog_Errno("series_new_cycle_send_fail:%#x", self_new_cycle_frame.sn);
        }
        return -1;
    }
    else
    {
        if (CI_TRUE == b_print_new_cycle_syn)
        {
            CILog_Msg("series_new_cycle_send_success:%#x", self_new_cycle_frame.sn);
        }
    }

    if (CI_TRUE == b_print_new_cycle_syn)
    {
        CILog_Msg("series_new_cycle_send_end:%#x",CICycleInt_GetCounter());
    }

    CIPerformance_Send(PDT_SERIES_NEW_CYCLE_T_END);

    return 0;
}
/*
 功能描述    : 从fd中读取热备新周期数据
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年7月31日 9:54:21
*/
static int32_t series_recv_new_cycle(int32_t fd,const char* fd_name)
{
    if (0 >= read(fd,&peer_new_cycle_frame,sizeof(peer_new_cycle_frame)))
    {
        if (CI_TRUE == b_print_new_cycle_syn)
        {
            CILog_Errno("series_%s_recv_fail",fd_name);
        }
        return -1;
    }
    else
    {
        if (CI_FALSE == CINewCycleSyn_Verify(&peer_new_cycle_frame))
        {
            if (CI_TRUE == b_print_new_cycle_syn)
            {
                CIRemoteLog_Write("series_%s_recv_fail:data_error",fd_name);
            }
            return -1;
        }
        if (peer_new_cycle_last_time == peer_new_cycle_frame.time_stamp)
        {
            if (CI_TRUE == b_print_new_cycle_syn)
            {
                CIRemoteLog_Write("series_%s_recv_fail:timestamp_islast",fd_name);
            }

            return -1;
        }
        if (sn_before(peer_new_cycle_frame.sn, CICycleInt_GetCounter()))
        {
            CIRemoteLog_Write("series_%s_recv_fail:sn(%#x)<(%#x)", fd_name, peer_new_cycle_frame.sn, CICycleInt_GetCounter());

            return -1;
        }

        if (sn_after(peer_new_cycle_frame.sn, CICycleInt_GetCounter()))
        {
            CIRemoteLog_Write("series_reset_cycle_counter:recv_new_cycle:%#x->%#x", CICycleInt_GetCounter(), peer_new_cycle_frame.sn);
            CICycleInt_SetCounter(peer_new_cycle_frame.sn);
        }
    }

    if (CI_TRUE == b_print_new_cycle_syn)
    {
        CILog_Msg("series_%s_recv_success:%#x", fd_name, peer_new_cycle_frame.sn);
    }

    return 0;
}
/*
功能描述    : 接受SERIES新周期同步消息
返回值      : 成功为0，失败为-1
参数        : 
作者        : 张彦升
日期        : 2013年10月25日 10:19:57
*/
int32_t CISeries_RecvNewCycleSyn(void)
{
    CIPerformance_Send(PDT_SERIES_NEW_CYCLE_R_BEGIN);

    if (CI_TRUE == b_print_new_cycle_syn)
    {
        CILog_Msg("series_new_cycle_recv_begin:%#x",CICycleInt_GetCounter());
    }

    if (0 > series_recv_new_cycle(new_cycle_fd_a,"new_cycle_a"))
    {
        if (0 > series_recv_new_cycle(new_cycle_fd_b,"new_cycle_b"))
        {
            return -1;
        }
    }
    else
    {
        series_clean_recv_buffer(new_cycle_fd_b);
    }

    peer_new_cycle_last_time = peer_new_cycle_frame.time_stamp;

    if (CI_TRUE == b_print_new_cycle_syn)
    {
        CILog_Msg("series_new_cycle_recv_end:%#x",CICycleInt_GetCounter());
    }

    CIPerformance_Send(PDT_SERIES_NEW_CYCLE_R_END);

    return 0;
}
/*
功能描述    : 检查新周期同步的数据是否一致
返回值      : 成功为0，失败为-1
参数        : 
作者        : 张彦升
日期        : 2014年5月6日 8:03:18
*/
int32_t CISeries_CheckNewCycleSyn(void)
{
    if (SERIES_MASTER != CISeries_GetLocalState())
    {
        /*主系通信道断开时使用备系进行请求,所以不做请求帧同步*/
        if(SERIES_STANDBY == CISeries_GetLocalState() && CI_FALSE == CISeries_IsPeerEeuBroke())
        {
            CIEeu_AdjustRequstFsn(peer_new_cycle_frame.eeu_this_request_fsn,
                peer_new_cycle_frame.eeu_last_request_fsn);
        }
    }
    return 0;
}
/*
 功能描述    : 打开新周期同步信息输出
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年4月11日 15:29:15
*/
int32_t CISeries_OpenPrintNewCycleSyn(void)
{
    b_print_new_cycle_syn = CI_TRUE;

    return 0;
}
/*
功能描述    : 发送本系热备状态下输入同步数据
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2013年10月25日 13:40:37
*/
int32_t CISeries_SendInputSyn(void)
{
    CIPerformance_Send(PDT_SERIES_INPUT_T_BEGIN);

    if (CI_TRUE == b_print_input_syn)
    {
        CILog_Msg("series_input_send_begin:%#x",CICycleInt_GetCounter());
    }

    /*将assemble函数放在提示打印信息前面提高测量发送速度的准确性*/
    CIInputSyn_Assemble(&self_input_frame);

    if (0 >= write(input_fd_a,&self_input_frame,sizeof(self_input_frame)))
    {
        if (CI_TRUE == b_print_input_syn)
        {
            CILog_Errno("series_input_send_fail:%#x",CICycleInt_GetCounter());
        }
        return -1;
    }
    else
    {
        if (CI_TRUE == b_print_input_syn)
        {
            CILog_Msg("series_input_send_success:%#x",CICycleInt_GetCounter());
        }
    }

    if (CI_TRUE == b_print_input_syn)
    {
        CILog_Msg("series_input_send_end:%#x",CICycleInt_GetCounter());
    }

    CIPerformance_Send(PDT_SERIES_INPUT_T_END);

    return 0;
}
/*
 功能描述    : 接收热备状态下的输入数据
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年7月31日 10:12:15
*/
static int32_t series_recv_input(int32_t fd,const char* p_fd_name)
{
    if (0 >= read(fd,&peer_input_frame,sizeof(peer_input_frame)))
    {
        if (CI_TRUE == b_print_input_syn)
        {
            CILog_Errno("series_%s_recv_fail:%#x",p_fd_name,CICycleInt_GetCounter());
        }
        return -1;
    }
    else
    {
        if (CI_FALSE == CIInputSyn_Verify(&peer_input_frame))
        {
            if (CI_TRUE == b_print_input_syn)
            {
                CIRemoteLog_Write("series_%s_recv_fail:data_error",p_fd_name);
            }
            return -1;
        }
        if (peer_input_last_time == peer_input_frame.time_stamp)
        {
            if (CI_TRUE == b_print_input_syn)
            {
                CIRemoteLog_Write("series_%s_recv_fail:timestamp_islast",p_fd_name);
            }

            return -1;
        }
        if (sn_before(peer_input_frame.cycle_counter, CICycleInt_GetCounter()))
        {
            CIRemoteLog_Write("series_%s_recv_fail:sn(%#x)<(%#x)", p_fd_name, peer_input_frame.cycle_counter, CICycleInt_GetCounter());

            return -1;
        }

        if (sn_after(peer_input_frame.cycle_counter, CICycleInt_GetCounter()))
        {
            CIRemoteLog_Write("series_reset_cycle_counter:recv_input:%#x->%#x", CICycleInt_GetCounter(), peer_input_frame.cycle_counter);
            CICycleInt_SetCounter(peer_input_frame.cycle_counter);
        }
    }

    if (CI_TRUE == b_print_input_syn)
    {
        CILog_Msg("series_%s_recv_success:%#x", p_fd_name, peer_input_frame.cycle_counter);
    }

    return 0;
}
/*
功能描述    : 接收主系输入同步数据
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2013年10月29日 12:53:29
*/
int32_t CISeries_RecvInputSyn(void)
{
    CIPerformance_Send(PDT_SERIES_INPUT_R_BEGIN);

    if (CI_TRUE == b_print_input_syn)
    {
        CILog_Msg("series_input_recv_begin:%#x",CICycleInt_GetCounter());
    }

    if (0 > series_recv_input(input_fd_a,"input_a"))
    {
        if (0 > series_recv_input(input_fd_b,"input_b"))
        {
            return -1;
        }
    }
    else
    {
        series_clean_recv_buffer(input_fd_b);
    }

    peer_input_last_time = peer_input_frame.time_stamp;

    if (CI_TRUE == b_print_input_syn)
    {
        CILog_Msg("series_input_recv_end:%#x",CICycleInt_GetCounter());
    }

    CIPerformance_Send(PDT_SERIES_INPUT_R_END);

    return 0;
}

/*
功能描述    : 检查双系输入数据是否一致
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2013年10月29日 13:15:53
*/
int32_t CISeries_CheckInputSyn(void)
{
    if (CI_TRUE == b_print_input_syn)
    {
        CILog_Msg("series_input_compare_begin");
    }

    CIInputSyn_Assemble(&self_input_frame);

    if (CI_TRUE == peer_input_frame.b_switch && SERIES_STANDBY == CISeries_GetLocalState())
    {
        CISwitch_LetSwitch();
    }
    if(CI_FALSE == peer_input_frame.eeu_channel_a_ok && CI_FALSE == peer_input_frame.eeu_channel_b_ok)
    {
        b_peer_series_eeu_broke = CI_TRUE;
    }
    if(CI_FALSE == peer_input_frame.hmi_channel_a_ok && CI_FALSE == peer_input_frame.hmi_channel_b_ok)
    {
        b_peer_series_hmi_broke = CI_TRUE;
    }

    if (-1 == CIinputSyn_Compare(self_input_frame,peer_input_frame))
    {
        return -1;
    }

    if (CI_TRUE == b_print_input_syn)
    {
        CILog_Msg("series_input_compare_success");
    }

    return 0;
}

/*
功能描述    : 数据容错函数，该函数很少会执行到，所以需要仔细测试
返回值      : 成功为0，失败为-1
参数        : 
作者        : 何境泰
日期        : 2013年12月25日 8:50:19
*/
int32_t CISeries_FaultTolerantInputSyn(void)
{
    int ret = 0;

    if (CI_TRUE == b_print_input_syn)
    {
        CIRemoteLog_Write("series_input_tolerant_begin:%#x",CICycleInt_GetCounter());
    }

    ret = CIinputSyn_Replace(&peer_input_frame);

    if (CI_TRUE == b_print_input_syn)
    {
        CIRemoteLog_Write("series_input_tolerant_end:%#x",CICycleInt_GetCounter());
    }

    return ret;
}
/*
 功能描述    : 是否打印输入信息
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年4月11日 15:29:15
*/
int32_t CISeries_OpenPrintInputSyn(void)
{
    b_print_input_syn = CI_TRUE;

    return 0;
}


/*
功能描述    : 发送本系输出结果
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2013年10月29日 12:55:13
*/
int32_t CISeries_SendResultSyn(void)
{
    CIPerformance_Send(PDT_SERIES_RESULT_T_BEGIN);

    if (CI_TRUE == b_print_result_syn)
    {
        CILog_Msg("series_result_send_begin:%#x",CICycleInt_GetCounter());
    }

    /*对帧中的数据进行装配*/
    CIResultSyn_Assemble(&self_result_frame);

    if (0 >= write(result_fd_a,&self_result_frame,sizeof(self_result_frame)))
    {
        if (CI_TRUE == b_print_result_syn)
        {
            CILog_Errno("series_result_send_fail:%#x", self_result_frame.cycle_counter);
        }
        return -1;
    }
    else
    {
        if (CI_TRUE == b_print_result_syn)
        {
            CILog_Msg("series_result_send_success:%#x", self_result_frame.cycle_counter);
        }
    }

    if (CI_TRUE == b_print_result_syn)
    {
        CILog_Msg("series_result_send_end:%#x",CICycleInt_GetCounter());
    }

    CIPerformance_Send(PDT_SERIES_RESULT_T_END);

    return 0;
}
/*
 功能描述    : 接收热备状态下的结果数据
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年7月31日 10:18:05
*/
static int32_t series_recv_result(int32_t fd,const char* p_fd_name)
{
    if (0 >= read(fd,&peer_result_frame,sizeof(peer_result_frame)))
    {
        if (CI_TRUE == b_print_result_syn)
        {
            CILog_Errno("series_%s_recv_fail:%#x",p_fd_name,CICycleInt_GetCounter());
        }
        return -1;
    }
    else
    {
        if (CI_FALSE == CIResultSyn_Verify(&peer_result_frame))
        {
            if (CI_TRUE == b_print_result_syn)
            {
                CIRemoteLog_Write("series_%s_recv_fail:data_error",p_fd_name);
            }
            return -1;
        }
        if (peer_result_last_time == peer_result_frame.time_stamp)
        {
            if (CI_TRUE == b_print_result_syn)
            {
                CIRemoteLog_Write("series_%s_recv_fail:timestamp_islast",p_fd_name);
            }
            return -1;
        }
        if (sn_before(peer_result_frame.cycle_counter, CICycleInt_GetCounter()))
        {
            CIRemoteLog_Write("series_%s_recv_fail:sn(%#x) < (%#x)", p_fd_name, peer_result_frame.cycle_counter, CICycleInt_GetCounter());

            return -1;
        }

        if (sn_after(peer_result_frame.cycle_counter, CICycleInt_GetCounter()))
        {
            CIRemoteLog_Write("series_reset_cycle_counter:recv_result:%#x->%#x", CICycleInt_GetCounter(), peer_result_frame.cycle_counter);
            CICycleInt_SetCounter(peer_result_frame.cycle_counter);
            b_self_delayed = CI_TRUE;
        }
    }

    if (CI_TRUE == b_print_result_syn)
    {
        CILog_Msg("series_%s_recv_success:%#x", p_fd_name, peer_result_frame.cycle_counter);
    }

    return 0;
}
/*
功能描述    : 接收主系输出结果
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2013年10月29日 12:59:06
*/
int32_t CISeries_RecvResultSyn(void)
{
    CIPerformance_Send(PDT_SERIES_RESULT_R_BEGIN);

    if (CI_TRUE == b_print_result_syn)
    {
        CILog_Msg("series_result_recv_begin");
    }

    if (0 > series_recv_result(result_fd_a,"result_a"))
    {
        if (0 > series_recv_result(result_fd_b,"result_b"))
        {
            return -1;
        }
    }
    else
    {
        series_clean_recv_buffer(result_fd_b);
    }

    peer_result_last_time = peer_result_frame.time_stamp;

    if (CI_TRUE == b_print_result_syn)
    {
        CILog_Msg("series_result_recv_end");
    }

    CIPerformance_Send(PDT_SERIES_RESULT_R_END);

    return 0;
}

/*
功能描述    : 检查双系输出结果帧是否出错
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2013年10月29日 14:41:22
*/
int32_t CISeries_CheckResultSyn(void)
{
    if (CI_TRUE == b_print_result_syn)
    {
        CILog_Msg("series_result_compare_begin:%#x",CICycleInt_GetCounter());
    }

    /*如果自身延迟了，则从略本周期的结果比较，该工作一定是在备系完成*/
    if (CI_TRUE == b_self_delayed)
    {
        b_self_delayed = CI_FALSE;
        return 0;
    }

    /*由于双系当中该数据很可能没有装配所以需要装配一次*/
    CIResultSyn_Assemble(&self_result_frame);

    if (0 > CIResultSyn_Compare(self_result_frame,peer_result_frame))
    {
        if (CI_TRUE == b_print_result_syn)
        {
            CIRemoteLog_Write("series_result_compare_fail:%#x",CICycleInt_GetCounter());
        }
        CI_DumpToFile("./series_peer_input_frame",&peer_input_frame,sizeof(peer_input_frame));
        CI_DumpToFile("./series_self_input_frame", &self_input_frame, sizeof(self_input_frame));

        CI_DumpToFile("./series_peer_result_frame",&peer_result_frame,sizeof(peer_result_frame));
        CI_DumpToFile("./series_self_result_frame",&self_result_frame,sizeof(self_result_frame));

        return -1;
    }
    else
    {
        if (CI_TRUE == b_print_result_syn)
        {
            CILog_Msg("series_result_compare_end:%#x",CICycleInt_GetCounter());
        }
    }

    return 0;
}
/*
 功能描述    : 打开结果信息的输出
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年4月11日 15:29:15
*/
int32_t CISeries_OpenPrintResultSyn(void)
{
    b_print_result_syn = CI_TRUE;

    return 0;
}

/*
功能描述    : 心跳信号数据装配
返回值      : 成功为0，失败为-1
参数        : 
作者        : 张彦升
日期        : 2014年4月15日 13:49:54
*/
static int32_t series_assemble_heartbeat_data(SeriesHeartBeatFrame* p_frame,uint32_t sn)
{
    assert(NULL != p_frame);

    p_frame->type = ST_HEARTBEAT;
    p_frame->series_state = (uint8_t)CISeries_GetLocalState();
    p_frame->elapse = CITimer_GetElapsedCycles();
    p_frame->sn = sn;
    p_frame->time_stamp = CI_GetTimeStamp();

    p_frame->eeu_channel_a_ok = CIEeu_IsChannelAOk();
    p_frame->eeu_channel_b_ok = CIEeu_IsChannelBOk();
    p_frame->hmi_channel_a_ok = CIHmi_IsChannelAOk();
    p_frame->hmi_channel_b_ok = CIHmi_IsChannelBOk();

    /*计算crc码*/
    CISeriesHeartbeat_CalcHash(p_frame);

    return 0;
}
/*
功能描述    : 发送双系心跳信号，在这里需要注意的一点是我们装配序号的步骤在最后，也就是本次
心跳发送完成之后装配下次心跳信号序号
返回值      : 成功为0，失败为-1
参数        : 
作者        : 张彦升
日期        : 2013年10月28日 15:01:47
*/
int32_t CISeries_SendHeartbeat(void)
{
    static uint32_t sn = 1;

    CIPerformance_SigSend(PDT_SERIES_HEARTBEAT_T_BEGIN);

    if (CI_TRUE == b_print_heartbeat)
    {
        CILog_SigMsg("series_heartbeat_send_begin:%#x",sn);
    }
    /*对帧中的数据进行装配*/
    series_assemble_heartbeat_data(&self_beat_frame,sn);

    if (0 >= write(heartbeat_fd_a,&self_beat_frame,sizeof(self_beat_frame)))
    {
        if (CI_TRUE == b_print_heartbeat)
        {
            CILog_SigErrno("series_heartbeat_send_fail:%#x", self_beat_frame.sn);
        }
        return -1;
    }
    else
    {
        if (CI_TRUE == b_print_heartbeat)
        {
            CILog_SigMsg("series_heartbeat_send_success:%#x", self_beat_frame.sn);
        }
    }

    if (CI_TRUE == b_print_heartbeat)
    {
        CILog_SigMsg("series_heartbeat_send_end:%#x",sn);
    }

    sn += 1;

    CIPerformance_SigSend(PDT_SERIES_HEARTBEAT_T_END);

    return 0;
}
/*
 功能描述    : 发送心跳信号数据
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年7月31日 10:30:13
*/
static int32_t series_recv_heartbeat(int32_t fd,const char* p_fd_name)
{
    if (0 >= read(fd,&peer_beat_frame,sizeof(peer_beat_frame)))
    {
        if (CI_TRUE == b_print_heartbeat)
        {
            CILog_SigErrno("series_%s_recv_fail",p_fd_name);
        }
        return -1;
    }
    else
    {
        if (CI_FALSE == CISeriesHeartbeat_Verify(&peer_beat_frame))
        {
            if (CI_TRUE == b_print_heartbeat)
            {
                CILog_SigMsg("series_%s_recv_fail:data_error",p_fd_name);
            }
            return -1;
        }
        if (peer_heartbeat_last_sn == peer_beat_frame.sn)
        {
            if (CI_TRUE == b_print_heartbeat)
            {
                CILog_SigMsg("series_%s_recv_fail:sn_islast%#x", p_fd_name, peer_beat_frame.sn);
            }
            return -1;
        }
        if (peer_heartbeat_last_time == peer_beat_frame.time_stamp)
        {
            if (CI_TRUE == b_print_heartbeat)
            {
                CILog_SigMsg("series_%s_recv_fail:timestamp_islast",p_fd_name);
            }
            return -1;
        }
    }

    if (CI_TRUE == b_print_heartbeat)
    {
        CILog_SigMsg("series_%s_recv_success:%#x", p_fd_name, peer_beat_frame.sn);
    }

    return 0;
}
/*
功能描述    : 接受双系心跳信号
返回值      : 成功为0，失败为-1
参数        : 
作者        : 张彦升
日期        : 2013年10月28日 15:02:04
*/
int32_t CISeries_RecvHeartbeat(void)
{
    CIPerformance_SigSend(PDT_SERIES_HEARTBEAT_R_BEGIN);

    if (CI_TRUE == b_print_heartbeat)
    {
        CILog_SigMsg("series_heartbeat_recv_begin");
    }

    if (0 > series_recv_heartbeat(heartbeat_fd_a,"heartbeat_a"))
    {
        if (0 > series_recv_heartbeat(heartbeat_fd_b,"heartbeat_b"))
        {
            series_peer_state = SERIES_PENDING;
            return -1;
        }
    }
    else
    {
        series_clean_recv_buffer(heartbeat_fd_b);
    }

    series_clean_recv_buffer(heartbeat_fd_a);
    series_clean_recv_buffer(heartbeat_fd_b);

    peer_heartbeat_last_time = peer_beat_frame.time_stamp;
    peer_heartbeat_last_sn = peer_beat_frame.sn;

    if (CI_TRUE == b_print_heartbeat)
    {
        CILog_SigMsg("series_heartbeat_recv_end:%#x", peer_beat_frame.sn);
    }

    CIPerformance_SigSend(PDT_SERIES_HEARTBEAT_R_END);

    return 0;
}
extern uint32_t CICycle_GetEeuRequestRelativeCycle(void);

/*
功能描述    : 检查双系心跳信号
返回值      : 成功为0，失败为-1
参数        : 
作者        : 张彦升
日期        : 2014年4月30日 13:54:19
*/
int32_t CISeries_CheckHeartbeat(void)
{
    static int16_t failure_dual_master_count = 0;
    static int16_t eeu_force_switch_count = 0;
    static int16_t hmi_force_switch_count = 0;

    series_peer_state = peer_beat_frame.series_state;

    if (SERIES_PENDING == peer_beat_frame.series_state)
    {
        /*这里只是检查了另外一系的状态在PENDING状态，是允许状态，所以返回0*/
        return 0;
    }

    if (peer_beat_frame.series_state == CISeries_GetLocalState())
    {
        if (SERIES_MASTER == peer_beat_frame.series_state)
        {
            failure_dual_master_count ++;
            if (failure_dual_master_count > 8)
            {
                CIFailureInt_SetInfo(FAILURE_DUAL_MASTER_SERIES,"双系心跳信号发现双主系错误");
            }
        }
        else if (SERIES_STANDBY == peer_beat_frame.series_state)
        {
            CIFailureInt_SetInfo(FAILURE_DUAL_STANDBY_SERIES,"双系心跳信号发现双备系错误");
        }
        return -1;
    }

    /*
     * 注意侦序号同步bug,主系恰好与备系错了50ms的时间
     *       主系        备系
     *                   ---
     *       ---          | 
     *        |           | 
     *newcycle|           |   备系在某种情况下未收到主系的新周期,导致延迟
     *        |           | 
     *        |           | 
     *        |           | 
     *        |           | 
     *        |          ---
     *        |           |  由于主系此刻进入新周期并发送了新周期数据,备系接收后开始运行状态机剩余部分
     *       ---          |  由于备系50ms内可能未执行完所有的状态机导致此时报延迟错误
     *        |           |  备系此时又进入新周期一直延迟等待,导致这种状态循环往复
     *        |           |
     */

    failure_dual_master_count = 0;
    /*
     * 中断周期号跟随方式为: 
     * 1.主系从CPU跟随主系主CPU
     * 2.备系主CPU跟随主系主CPU
     * 3.备系从CPU跟随主系主CPU，再跟随备系主CPU
     */
    if ( SERIES_MASTER != CISeries_GetLocalState())
    {
        /*
         * 在进入联锁周期的定时周期内部做校正
         * 在连锁周期结束时和电子单元请求时，尝试调整帧序号可能会错过状态机或者电子单元请求，
         * 这里做简单的处理，让其在这两个定时器发生的时候不进行调整
         * BUG 
         *        。->
         * requst 。  | 当调整周期跨度大于1时仍然会错过请求机会
         *        。<-
         **/
        if (0 != CITimer_GetElapsedCycles() % CI_CYCLE_RELATIVE
            && CICycle_GetEeuRequestRelativeCycle() != CITimer_GetElapsedCycles() % EEU_REQUEST_CYCLE_RELATIVE
            && 1 <= CITimer_RoundSub(peer_beat_frame.elapse,CITimer_GetElapsedCycles()))
        {
            CITimer_SetElapsedCycles(peer_beat_frame.elapse);
        }
    }

    /*当为主系状态时，并且备系的健康状态优于主系时应该进行切换并提示*/
    if (CISeries_GetLocalState() == SERIES_MASTER && CISeries_GetPeerState() == SERIES_STANDBY)
    {
        /*主系电子单元的两路通信道都断开了，但是备系有一条或者两条通信道都正常则切换*/
        if (CI_FALSE == CIEeu_IsChannelAOk() && CI_FALSE == CIEeu_IsChannelBOk())
        {
            if (CI_TRUE == peer_beat_frame.eeu_channel_a_ok || CI_TRUE == peer_beat_frame.eeu_channel_b_ok)
            {
                eeu_force_switch_count ++;
                /*这里一定要有3个周期的冗余,因为两系的定时器可能有偏差*/
                if(3 < eeu_force_switch_count)
                {
                    CILog_SigMsg("备系电子单元通信状态健康于主系，正在设置自动切换标志，系统将在下个周期切换");
                    CISwitch_LetSwitch();
                    /*这里一定要清空，本次设置完切换标志可成功切换，若系统下次继续发生切换应该从头开始计数*/
                    eeu_force_switch_count = 0;
                }
            }
        }
        else
        {
            eeu_force_switch_count = 0;
        }
        /*主系控显机的两路通信道都断开了，但是备系有一条或者两条通信道都正常则切换*/
        if (CI_FALSE == CIHmi_IsChannelAOk() && CI_FALSE == CIHmi_IsChannelBOk() )
        {
            if (CI_TRUE == peer_beat_frame.hmi_channel_a_ok || CI_TRUE == peer_beat_frame.hmi_channel_b_ok)
            {
                hmi_force_switch_count ++;
                if(100 < hmi_force_switch_count && 100 < CITimer_GetElapsedCycles())    /*断开5S以上且开机5S以后*/
                {
                    if (CI_TRUE == peer_beat_frame.eeu_channel_a_ok && CI_TRUE == peer_beat_frame.eeu_channel_b_ok)  /*备系CAN正常*/
                    {
                        CILog_SigMsg("备系控显机通信状态健康于主系，正在设置自动切换标志，系统将在下个周期切换");
                        CISwitch_LetSwitch();
                        hmi_force_switch_count = 0;
                    }
                }
            }
        }
        else
        {
            hmi_force_switch_count = 0;
        }
    }
    if (CI_FALSE == peer_beat_frame.eeu_channel_a_ok && CI_FALSE == peer_beat_frame.eeu_channel_b_ok)
    {
        b_peer_series_eeu_broke = CI_TRUE;
    }
    else
    {
        b_peer_series_eeu_broke = CI_FALSE;
    }
    if (CI_FALSE == peer_beat_frame.hmi_channel_a_ok && CI_FALSE == peer_beat_frame.hmi_channel_b_ok)
    {
        b_peer_series_hmi_broke = CI_TRUE;
    }
    else
    {
        b_peer_series_hmi_broke = CI_FALSE;
    }

    return 0;
}
/*
 * 返回另外一系的电子单元通信状态 
 */
CI_BOOL CISeries_IsPeerEeuBroke(void)
{
    return b_peer_series_eeu_broke;
}
/*
 * 返回另外一系的控显机通信状态 
 */
CI_BOOL CISeries_IsPeerHmiBroke(void)
{
    return b_peer_series_hmi_broke;
}
/*
功能描述    : 打开双系打印心跳信号数据标志
返回值      : 成功为0，失败为-1
参数        : 
作者        : 张彦升
日期        : 2014年4月11日 15:28:00
*/
int32_t CISeries_OpenPrintHeartbeat(void)
{
    b_print_heartbeat = CI_TRUE;

    return 0;
}
/*
功能描述    : 确定主备系关系
            当在首次启动的时候需要通过抢占方式确定主备系，我们认为两个系统几乎在同一
            时间点上启动，同一时间点上进入该函数确定双系关系的概率是十分低的，所以
            在总能够通过这种方式得到主备系的关系
返回值      : 成功为0，失败为-1
参数        : 
作者        : 张彦升
日期        : 2013年10月30日 8:44:18
*/
static int32_t series_validate_relationship(void)
{
    int32_t ret = 0;
    int32_t count = 0;
    int32_t recv_count = 0;
    int32_t t_rand = 0;

    series_clean_recv_buffer(heartbeat_fd_a);
    series_clean_recv_buffer(heartbeat_fd_b);

    /*
     * 假若双系同时启动，它们都同时进入了该函数当中，势必都接收不到对方的心跳信号
     * 而各自选择进入主系状态，这时，极有可能发生双主系严重故障，一种可行的办法是
     * 让双系的主CPU在启动的时候各自发送一帧心跳信号帧，并且这一帧的内容双系状态
     * 为PENDING状态，每个CPU接收到状态为PENDING的心跳信号时进入使用随机等待接收
     * 从而避免这种故障
     */
    if (CPU_STATE_MASTER == CICpu_GetLocalState())
    {
        ret = CISeries_SendHeartbeat();
        assert(-1 != ret);
    }
    /*连续接受10次，避免有脏数据*/
    while (count < 10)
    {
        ret = CISeries_RecvHeartbeat();
        if (-1 != ret && -1 != CISeries_CheckHeartbeat())
        {
            recv_count ++;

            /*如果对方状态在pending状态则随机等待，避免死锁*/
            if (SERIES_PENDING == CISeries_GetPeerState())
            {
                /*生成1000ms至2000ms之间的随机值*/
                CIRemoteLog_Write("收到另外一系的心跳信号在SERIES_PENDING状态，正在随机等待");
                t_rand = rand() % 2000 + 1000;
                usleep(t_rand * 1000);
            }
            /*如果对方在热备状态，严重故障，报警*/
            else if (SERIES_STANDBY == CISeries_GetPeerState())
            {
                CIRemoteLog_Write("收到另外一系的心跳信号在SERIES_STANDBY状态，请检查故障");
                return -1;
            }
            /*如果对方在校核状态，严重故障，报警*/
            else if (SERIES_CHECK == CISeries_GetPeerState())
            {
                CIRemoteLog_Write("收到另外一系的心跳信号在SERIES_CHECK状态，请检查故障");
                return -1;
            }
        }
        /*停留200ms，确保下次能够接受到*/
        usleep(200000);
        count ++;
    }
    /*
    * 若没有收到数据，则认为另外一系还没有启动起来，将自己设为主系，若收到数据，则认为
    * 另外一系已经启动了，将自己设为备系校核状态
    */
    if (3 > recv_count)
    {
        /*
         * 这时候虽然判断另外一系没启动起来，为了避免双主系应确保切换装置返回的另外一系的
         * 状态在非主系情况下
         */
        if (CI_TRUE == CISwitch_TestMasterAlive())
        {
            CIRemoteLog_Write("切换装置返回另外一系：存活，双系心跳信号：丢失，备系无法启动");
            CILedInt_BlankingFailure();
            return -1;
        }
        else
        {
            series_set_local_state(SERIES_MASTER);
        }
    }
    else
    {
        CIRemoteLog_Write("收到另外一系心跳信号，正在进入备系校核状态");
        series_set_local_state(SERIES_CHECK);
    }

    return 0;
}
/*
 功能描述    : 清除光纤驱动中存留的所有缓存数据
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年6月16日 10:56:57
*/
int32_t CISeries_CleanRecvBuffer(void)
{
    CIRemoteLog_Write("now clean all series buffer");

    series_clean_recv_buffer(new_cycle_fd_a);
    series_clean_recv_buffer(new_cycle_fd_b);

    series_clean_recv_buffer(input_fd_a);
    series_clean_recv_buffer(input_fd_b);

    series_clean_recv_buffer(result_fd_a);
    series_clean_recv_buffer(result_fd_b);

    series_clean_recv_buffer(heartbeat_fd_a);
    series_clean_recv_buffer(heartbeat_fd_b);

    series_clean_recv_buffer(cfg_fd_a);
    series_clean_recv_buffer(cfg_fd_b);

    return 0;
}

/*
 功能描述    : 光纤fd初始化
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年7月31日 9:13:07
*/
static int32_t series_fd_init(void)
{
    /*heartbeat_fd_a*/
    heartbeat_fd_a = open("/dev/fibera1",O_RDWR | O_NONBLOCK);
    if (0 > heartbeat_fd_a)
    {
        CILog_Errno("open /dev/fibera1 failed");

        return -1;
    }
    new_cycle_fd_a = open("/dev/fibera2",O_RDWR);
    if (0 > new_cycle_fd_a)
    {
        CILog_Errno("open /dev/fibera2 failed");

        return -1;
    }
    input_fd_a = open("/dev/fibera3",O_RDWR);
    if (0 > input_fd_a)
    {
        CILog_Errno("open /dev/fibera3 failed");

        return -1;
    }
    result_fd_a = open("/dev/fibera4",O_RDWR);
    if (0 > result_fd_a)
    {
        CILog_Errno("open /dev/fibera4 failed");

        return -1;
    }
    cfg_fd_a = open("/dev/fibera5",O_RDWR);
    if (0 > cfg_fd_a)
    {
        CILog_Errno("open /dev/fibera5 failed");
        return -1;
    }

    /*下面打开b口设备*/
    /*heartbeat_fd_b*/
    heartbeat_fd_b = open("/dev/fiberb1", O_RDWR | O_NONBLOCK);
    if (0 > heartbeat_fd_b)
    {
        CILog_Errno("open /dev/fiberb1 failed");

        return -1;
    }
    new_cycle_fd_b = open("/dev/fiberb2", O_RDWR);
    if (0 > new_cycle_fd_b)
    {
        CILog_Errno("open /dev/fiberb2 failed");

        return -1;
    }
    input_fd_b = open("/dev/fiberb3", O_RDWR);
    if (0 > input_fd_b)
    {
        CILog_Errno("open /dev/fiberb3 failed");

        return -1;
    }
    result_fd_b = open("/dev/fiberb4", O_RDWR);
    if (0 > result_fd_b)
    {
        CILog_Errno("open /dev/fiberb4 failed");

        return -1;
    }
    cfg_fd_b = open("/dev/fiberb5",O_RDWR);
    if (0 > cfg_fd_b)
    {
        CILog_Errno("open /dev/fiberb5 failed");
        return -1;
    }

    return 0;
}
/*
 功能描述    : 为所有设备申请所使用的内存空间
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年5月16日 14:22:02
*/
static int32_t series_fd_alloc_memory(void)
{
    int32_t ret = 0;

    /*reset memory*/
    ret = ioctl(new_cycle_fd_a,FIBER_IOC_RESET);
    if (0 > ret)
    {
        CILog_Errno("ioctl new_cycle_fd_a FIBER_IOC_RESET failed");

        return -1;
    }
    CILog_Msg("FIBER_IOC_RESET:%#x",FIBER_IOC_RESET);
    CILog_Msg("FIBER_IOC_ALLOC_MEMORY:%#x",FIBER_IOC_ALLOC_MEMORY);
    CILog_Msg("双系通信内存分配:");
    CILog_Msg("心跳:%#x",sizeof(SeriesHeartBeatFrame));
    CILog_Msg("新周期:%#x",sizeof(NewCycleSynFrame));
    CILog_Msg("输入数据:%#x",sizeof(CIInputSynFrame));
    CILog_Msg("输出数据:%#x",sizeof(CIResultSynFrame));
    CILog_Msg("配置比较:%#x",sizeof(CfgSynFrame));

    /*heartbeat*/
    ret = ioctl(heartbeat_fd_a,FIBER_IOC_ALLOC_MEMORY,sizeof(SeriesHeartBeatFrame));
    if (0 > ret)
    {
        CILog_Errno("ioctl heartbeat_fd_a FIBER_IOC_ALLOC_MEMORY failed");

        return -1;
    }
    ret = ioctl(heartbeat_fd_b,FIBER_IOC_ALLOC_MEMORY,sizeof(SeriesHeartBeatFrame));
    if (0 > ret)
    {
        CILog_Errno("ioctl heartbeat_fd_b FIBER_IOC_ALLOC_MEMORY failed");

        return -1;
    }
    /*alloc memory for every device*/
    ret = ioctl(new_cycle_fd_a,FIBER_IOC_ALLOC_MEMORY,sizeof(NewCycleSynFrame));
    if (0 > ret)
    {
        CILog_Errno("ioctl new_cycle_fd_a FIBER_IOC_ALLOC_MEMORY failed");

        return -1;
    }
    ret = ioctl(new_cycle_fd_b,FIBER_IOC_ALLOC_MEMORY,sizeof(NewCycleSynFrame));
    if (0 > ret)
    {
        CILog_Errno("ioctl new_cycle_fd_b FIBER_IOC_ALLOC_MEMORY failed");

        return -1;
    }

    ret = ioctl(input_fd_a,FIBER_IOC_ALLOC_MEMORY,sizeof(CIInputSynFrame));
    if (0 > ret)
    {
        CILog_Errno("ioctl input_fd_a FIBER_IOC_ALLOC_MEMORY failed");

        return -1;
    }
    ret = ioctl(input_fd_b, FIBER_IOC_ALLOC_MEMORY, sizeof(CIInputSynFrame));
    if (0 > ret)
    {
        CILog_Errno("ioctl input_fd_b FIBER_IOC_ALLOC_MEMORY failed");

        return -1;
    }
    /*result*/
    ret = ioctl(result_fd_a,FIBER_IOC_ALLOC_MEMORY,sizeof(CIResultSynFrame));
    if (0 > ret)
    {
        CILog_Errno("ioctl result_fd_a FIBER_IOC_ALLOC_MEMORY failed");

        return -1;
    }
    ret = ioctl(result_fd_b,FIBER_IOC_ALLOC_MEMORY,sizeof(CIResultSynFrame));
    if (0 > ret)
    {
        CILog_Errno("ioctl result_fd_b FIBER_IOC_ALLOC_MEMORY failed");

        return -1;
    }
    /*cfg*/
    ret = ioctl(cfg_fd_a,FIBER_IOC_ALLOC_MEMORY,sizeof(CfgSynFrame));
    if (0 > ret)
    {
        CILog_Errno("ioctl cfg_fd_a FIBER_IOC_ALLOC_MEMORY failed");

        return -1;
    }
    ret = ioctl(cfg_fd_b,FIBER_IOC_ALLOC_MEMORY,sizeof(CfgSynFrame));
    if (0 > ret)
    {
        CILog_Errno("ioctl cfg_fd_b FIBER_IOC_ALLOC_MEMORY failed");

        return -1;
    }

    return 0;
}
/*
功能描述    : 初始化双系
返回值      : 成功为0，失败为-1
参数        : 
作者        : 张彦升
日期        : 2013年10月29日 15:43:55
*/
int32_t CISeries_Init(void)
{
    int ret = 0;

    static CI_BOOL b_initialized = CI_FALSE;

    if (CI_TRUE == b_initialized)
    {
        return -1;
    }

    ret = series_fd_init();
    if (-1 == ret)
    {
        return -1;
    }
    ret = series_fd_alloc_memory();
    if (-1 == ret)
    {
        return -1;
    }
    ret = CISeriesHeartbeat_Init(&self_beat_frame);
    assert(-1 != ret);

    ret = CINewCycleSyn_Init(&self_new_cycle_frame);
    assert(-1 != ret);

    ret = CIInputSyn_Init(&self_input_frame);
    assert(-1 != ret);

    ret = CIResultSyn_Init(&self_result_frame);
    assert(-1 != ret);

    ret = CICfgSyn_Init(&self_cfg_frame);
    assert(-1 != ret);

    /*建立通信之后开始抢占主备系*/
    ret = series_validate_relationship();
    if (-1 == ret)
    {
        return -1;
    }

    CIRemoteLog_Write("series_state:%s",CISeries_GetStateName(series_local_state));

    b_initialized = CI_TRUE;

    return 0;
}

#endif /*!LINUX_ENVRIONMENT*/
