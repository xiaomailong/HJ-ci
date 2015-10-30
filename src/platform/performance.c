/*********************************************************************
 Copyright (C), 2014,  Co.Hengjun, Ltd.

 作者        : 张彦升
 版本        : 1.0
 创建日期    : 2014年7月3日 10:47:41
 用途        : 用于侦听系统性能。
              该模块会通过网络将系统性能数据发送到远端，远端服务器可通过可视化程序
              将其实时显示出来
 历史修改记录: v1.0    创建
    v2.0    张彦升  将文件名从monitor转移为performance，monitor留给检测机使用
**********************************************************************/
#include "util/ci_header.h"

#ifdef LINUX_ENVRIONMENT

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "util/ci_header.h"
#include "util/config.h"
#include "util/utility.h"
#include "util/log.h"
#include "util/algorithms.h"

#include "performance.h"
#include "cpu_manage.h"
#include "series_manage.h"

/*
 * 按照IEEE 802.3帧大小为1492计算，除去udp、ip帧头共有1492 - 28 = 1464字节
 * 每个检测机帧为16字节，1464 / 16 = 91，则按照每个次发送91 * 16 = 1456个字节可
 * 使网络最优化
 */
#define PERFORMANCE_BUF_LEN 1456

static int32_t performance_port = 0;            /*性能检测机的端口地址*/
static int32_t performance_sock;            /*文件句柄*/

static struct sockaddr_in performance_addr;

/*
 功能描述    : socket初始化
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年7月3日 13:41:05
*/
static int32_t performance_sock_init(void)
{
    const char* monitor_ip_str = NULL;
    const char* monitor_port_str = NULL;

    monitor_ip_str = CIConfig_GetValue("PerformanceServerIP");
    if (NULL == monitor_ip_str)
    {
        CILog_Msg("配置文件当中找不到PerformanceServerIP项");
        return -1;
    }
    if (CI_FALSE == CI_ValidateIpAddress(monitor_ip_str))
    {
        CILog_Msg("配置文件当中PerformanceServerIP项值不符合ip规范");
        return -1;
    }

    monitor_port_str = CIConfig_GetValue("PerformanceServerPort");
    if (NULL == monitor_port_str)
    {
        CILog_Msg("配置文件当中找不到PerformanceServerPort项");
        return -1;
    }
    performance_port = atoi(monitor_port_str);
    if (CI_FALSE == CI_ValidatePort(performance_port))
    {
        CILog_Msg("在配置文件当中PerformanceServerPort值不符合规范");
        return -1;
    }

    performance_sock = socket(AF_INET,SOCK_DGRAM,0);
    if (-1 == performance_sock)
    {
        CILog_Errno("socket monitor_fd failed");

        return -1;
    }
    /*
    if (-1 == setsockopt(performance_sock, SOL_SOCKET, SO_BROADCAST,
        &broadcast, sizeof(broadcast)))
    {
        CILog_Errno("设置检测机为广播失败");
        return -1;
    }
    */

    performance_addr.sin_family = AF_INET;
    performance_addr.sin_addr.s_addr = inet_addr(monitor_ip_str);
    performance_addr.sin_port = htons(performance_port);

    return 0;
}

static uint16_t performance_get_sn(PERFORMANCE_DATA_TYPE mdt)
{
    static uint16_t sn_cpu_cfg_t = 0;
    static uint16_t sn_cpu_cfg_r = 0;

    static uint16_t sn_cpu_new_cycle_t = 0;
    static uint16_t sn_cpu_new_cycle_r = 0;

    static uint16_t sn_cpu_input_t = 0;
    static uint16_t sn_cpu_input_r = 0;

    static uint16_t sn_cpu_result_t = 0;
    static uint16_t sn_cpu_result_r = 0;

    static uint16_t sn_cpu_heartbeat_t = 0;
    static uint16_t sn_cpu_heartbeat_r = 0;

    static uint16_t sn_series_cfg_t = 0;
    static uint16_t sn_series_cfg_r = 0;

    static uint16_t sn_series_new_cycle_t = 0;
    static uint16_t sn_series_new_cycle_r = 0;

    static uint16_t sn_series_input_t = 0;
    static uint16_t sn_series_input_r = 0;

    static uint16_t sn_series_result_t = 0;
    static uint16_t sn_series_result_r = 0;

    static uint16_t sn_series_heartbeat_t = 0;
    static uint16_t sn_series_heartbeat_r = 0;

    static uint16_t sn_intrrupt = 0;
    static uint16_t sn_ci_cycle = 0;

    static uint16_t sn_eeu_t = 0;
    static uint16_t sn_eeu_r = 0;

    switch(mdt)
    {
    case PDT_CPU_CFG_T_BEGIN:
    case PDT_CPU_CFG_T_END:
        return sn_cpu_cfg_t += 1;
    case PDT_CPU_CFG_R_BEGIN:
    case PDT_CPU_CFG_R_END:
        return sn_cpu_cfg_r += 1;

    case PDT_CPU_NEW_CYCLE_T_BEGIN:
    case PDT_CPU_NEW_CYCLE_T_END:
        return sn_cpu_new_cycle_t += 1;
    case PDT_CPU_NEW_CYCLE_R_BEGIN:
    case PDT_CPU_NEW_CYCLE_R_END:
        return sn_cpu_new_cycle_r += 1;

    case PDT_CPU_INPUT_T_BEGIN:
    case PDT_CPU_INPUT_T_END:
        return sn_cpu_input_t += 1;
    case PDT_CPU_INPUT_R_BEGIN:
    case PDT_CPU_INPUT_R_END:
        return sn_cpu_input_r += 1;

    case PDT_CPU_RESULT_T_BEGIN:
    case PDT_CPU_RESULT_T_END:
        return sn_cpu_result_t += 1;
    case PDT_CPU_RESULT_R_BEGIN:
    case PDT_CPU_RESULT_R_END:
        return sn_cpu_result_r += 1;

    case PDT_CPU_HEARTBEAT_T_BEGIN:
    case PDT_CPU_HEARTBEAT_T_END:
        return sn_cpu_heartbeat_t += 1;
    case PDT_CPU_HEARTBEAT_R_BEGIN:
    case PDT_CPU_HEARTBEAT_R_END:
        return sn_cpu_heartbeat_r += 1;

    case PDT_SERIES_CFG_T_BEGIN:
    case PDT_SERIES_CFG_T_END:
        return sn_series_cfg_t += 1;
    case PDT_SERIES_CFG_R_BEGIN:
    case PDT_SERIES_CFG_R_END:
        return sn_series_cfg_r += 1;

    case PDT_SERIES_NEW_CYCLE_T_BEGIN:
    case PDT_SERIES_NEW_CYCLE_T_END:
        return sn_series_new_cycle_t += 1;
    case PDT_SERIES_NEW_CYCLE_R_BEGIN:
    case PDT_SERIES_NEW_CYCLE_R_END:
        return sn_series_new_cycle_r += 1;

    case PDT_SERIES_INPUT_T_BEGIN:
    case PDT_SERIES_INPUT_T_END:
        return sn_series_input_t += 1;
    case PDT_SERIES_INPUT_R_BEGIN:
    case PDT_SERIES_INPUT_R_END:
        return sn_series_input_r += 1;

    case PDT_SERIES_RESULT_T_BEGIN:
    case PDT_SERIES_RESULT_T_END:
        return sn_series_result_t += 1;
    case PDT_SERIES_RESULT_R_BEGIN:
    case PDT_SERIES_RESULT_R_END:
        return sn_series_result_r += 1;

    case PDT_SERIES_HEARTBEAT_T_BEGIN:
    case PDT_SERIES_HEARTBEAT_T_END:
        return sn_series_heartbeat_t += 1;
    case PDT_SERIES_HEARTBEAT_R_BEGIN:
    case PDT_SERIES_HEARTBEAT_R_END:
        return sn_series_heartbeat_r += 1;

    case PDT_INTRRUPT_BEGIN:
        return sn_intrrupt += 1;
    case PDT_CI_CYCLE_BEGIN:
    case PDT_CI_CYCLE_END:
        return sn_ci_cycle += 1;
    case PDT_EEU_T_BEGIN:
    case PDT_EEU_T_END:
        return sn_eeu_t += 1;
    case PDT_EEU_R_BEGIN:
    case PDT_EEU_R_END:
        return sn_eeu_r += 1;
    default:
        return 0;
    }

    return 0;
}
/*
 功能描述    : 将网络的sendto函数封装一次，从而节省带宽
              注意，该函数不是安全函数，心跳信号的中断如果在执行此函数时到来则会出问题
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年7月5日 22:11:39
*/
static int32_t performance_send(PerformanceFrame* p_frame)
{
    static char buf[PERFORMANCE_BUF_LEN] = {0};
    static int32_t offset = 0;

    assert(NULL != p_frame);

    memcpy(buf + offset,p_frame,sizeof(PerformanceFrame));
    offset += sizeof(PerformanceFrame);

    /*当这一帧数据满时则将其发出*/
    if (PERFORMANCE_BUF_LEN == offset)
    {
        sendto(performance_sock,buf,PERFORMANCE_BUF_LEN,
               0,(struct sockaddr *)&performance_addr,sizeof(performance_addr));

        memset(buf,0,PERFORMANCE_BUF_LEN);
        offset = 0;
    }
    return 0;
}
/*
 功能描述    : 将网络的sendto函数封装一次，从而节省带宽，该函数为安全函数可在中断处理函数
             内部调用
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年7月5日 22:11:39
*/
static int32_t performance_sig_send(PerformanceFrame* p_frame)
{
    static char buf[PERFORMANCE_BUF_LEN] = {0};
    static int32_t offset = 0;

    memcpy(buf + offset,p_frame,sizeof(PerformanceFrame));
    offset += sizeof(PerformanceFrame);

    /*当这一帧数据满时则将其发出*/
    if (PERFORMANCE_BUF_LEN == offset)
    {
        sendto(performance_sock,buf,PERFORMANCE_BUF_LEN,
               0,(struct sockaddr *)&performance_addr,sizeof(performance_addr));

        memset(buf,0,PERFORMANCE_BUF_LEN);
        offset = 0;
    }
    return 0;
}
/*
 功能描述    : 向检测机发送数据
 返回值      : 成功为0，失败为-1
 参数        : @mdt，发送数据类型
              @data，数据指针，为null，则只发送一帧数据内容为空的帧
 作者        : 张彦升
 日期        : 2014年7月3日 19:19:19
*/
int32_t CIPerformance_Send(PERFORMANCE_DATA_TYPE mdt)
{
    static PerformanceFrame frame;
    PerformanceFrameHead* head = &frame.head;

    head->frame_head_tag = 0xAA50;
    head->frame_sn = performance_get_sn(mdt);
    head->time_stamp = CI_GetTimeStamp();
    head->series_state = (uint8_t)CISeries_GetLocalState();
    head->cpu_state = (uint8_t)CICpu_GetLocalState();
    head->data_type = (uint16_t)mdt;

    frame.crc = CIAlgorithm_Crc16(&frame,sizeof(frame) - sizeof(frame.crc));

    performance_send(&frame);

    return 0;
}
/*
 功能描述    : 向检测机发送数据，该函数为相对应的安全函数，只可在中断响应函数内部调用
 返回值      : 成功为0，失败为-1
 参数        : @mdt，发送数据类型
              @data，数据指针，为null，则只发送一帧数据内容为空的帧
 作者        : 张彦升
 日期        : 2014年7月3日 19:19:19
*/
int32_t CIPerformance_SigSend(PERFORMANCE_DATA_TYPE mdt)
{
    static PerformanceFrame frame;
    PerformanceFrameHead* head = &frame.head;

    head->frame_head_tag = 0xAA50;
    head->frame_sn = performance_get_sn(mdt);
    head->time_stamp = CI_GetTimeStamp();
    head->series_state = (uint8_t)CISeries_GetLocalState();
    head->cpu_state = (uint8_t)CICpu_GetLocalState();
    head->data_type = (uint16_t)mdt;

    frame.crc = CIAlgorithm_Crc16(&frame,sizeof(frame) - sizeof(frame.crc));

    performance_sig_send(&frame);

    return 0;
}
/*
 功能描述    : 检测管理初始化
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年7月3日 10:46:38
*/
int32_t CIPerformance_Init(void)
{
    int32_t ret = 0;
    static CI_BOOL b_initialized = CI_FALSE;

#ifndef CI_UNIT_TEST
    if (CI_TRUE == b_initialized)
    {
        return -1;
    }
#endif /* CI_UNIT_TEST*/
    ret = performance_sock_init();
    if (0 > ret)
    {
        return -1;
    }

    b_initialized = CI_TRUE;

    return 0;
}

#endif /*LINUX_ENVRIONMENT*/
