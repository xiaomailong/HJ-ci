/*********************************************************************
 Copyright (C), 2015,  Co.Hengjun, Ltd.

 版本        : 1.0
 创建日期    : 2015年3月27日
 历史修改记录: v1.0    创建
**********************************************************************/

#include "monitor.h"

#include "util/app_config.h"
#include "util/log.h"

#include "cpu_manage.h"

#define MONITOR_BUFFER_SIZE 0x1000      /*检测机串口使用4k的空间*/
/*
 * 帧头
 */
typedef struct _MonitorFrameHead
{
    uint16_t frame_head_tag;                /*帧头标志*/
    uint16_t frame_sn;                      /*帧编号*/
    uint32_t time_stamp;                    /*时间戳*/
    uint8_t send_device_id;                 /*发送设备ID*/
    uint8_t send_device_type;               /*发送设备类型*/
    uint8_t send_station_id;                /*发送站场号*/
    uint8_t recieve_device_id;              /*接收设备ID*/
    uint8_t recieve_device_type;            /*接收设备类型*/
    uint8_t series_state;                   /*双系状态*/
    uint8_t cpu_sate;                       /*cpu状态*/
    uint16_t data_length;                   /*数据域数据长度*/	
    uint16_t crc;                           /*crc校验码*/
}MonitorFrameHead;
/*
 * 从控显机接受的帧
 */
typedef struct _MonitorFrame
{
    MonitorFrameHead head;                      /*头*/
    uint32_t data[MAX_SIGNAL_NODES];           /*数据*/
}MonitorFrame;

static int32_t monitor_fd_a = 0;
static int32_t monitor_fd_b = 0;

/*
 功能描述    : 检测机发送数据。
            注意，这个函数本来应该定义为私有的，20150330王伟龙拒绝使用新定检测协议
            而改用套用控显机逻辑处理，所以暂时暴漏出来
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年3月30日 11:57:28
*/
int32_t CIMonitor_Write(const void* buf,int32_t data_len)
{
#ifdef WIN32
    return 0;
#else
    if (NULL == buf || 0 > data_len || CPU_STATE_MASTER != CICpu_GetLocalState())
    {
        return -1;
    }
    /*若数据长度太大则改为截断发送*/
    if (MONITOR_BUFFER_SIZE < data_len)
    {
        write(monitor_fd_a,buf,MONITOR_BUFFER_SIZE);
    }
    else
    {
        write(monitor_fd_a,buf,data_len);
    }

    return 0;

#endif /* WIN32*/
}
/*
 功能描述    : 初始化检测机通信文件fd
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年3月30日 12:01:05
*/
static int32_t monitor_fd_init(void)
{
#ifdef WIN32
    return 0;
#else
    monitor_fd_a = open("/dev/uart2a",O_RDWR);

    if (-1 == monitor_fd_a)
    {
        CILog_Errno("open /dev/uart2a failed");

        return -1;
    }

    monitor_fd_b = open("/dev/uart2b",O_RDWR);

    if (-1 == monitor_fd_b)
    {
        CILog_Errno("open /dev/uart2b failed");

        return -1;
    }

    return 0;
#endif /* WIN32*/
}
/*
 功能描述    : 初始化检测机通信
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年3月27日 12:34:53
*/
int32_t CIMonitor_Init(void)
{
    static CI_BOOL b_initialized = CI_FALSE;

#ifdef CI_UNIT_TEST
    if (CI_TRUE == b_initialized)
    {
        return -1;
    }
#endif /* CI_UNIT_TEST*/

    if (0 > monitor_fd_init())
    {
        return -1;
    }

    b_initialized = CI_TRUE;

    return 0;
}
