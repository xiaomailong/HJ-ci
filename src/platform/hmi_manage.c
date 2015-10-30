/*********************************************************************
 Copyright (C), 2011,  Co.Hengjun, Ltd.
 
 版本        : 1.2
 创建日期    : 2013年11月20日 11:04:51
 用途        : 联锁机和控显机的通信
 历史修改记录: 
    v1.0    创建
    v1.2    2014年6月26日  张彦升
            为了能够在windows下运行，添加在windows下使用socket通信的逻辑。
    v1.3    删除station_index。
            将向控显机发送的提示（调试信息）同时使用CILog_Msg输出，方便调试。
    v1.4    2015年5月7日  张彦升
            修正空闲机双系切换逻辑
**********************************************************************/
#ifdef WIN32
/*WIN32_LEAN_AND_MEAN能够避免编译时的重定义错误问题*/
#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN
#   endif
#   include <winsock2.h>
#   include <windows.h>

#   pragma comment(lib,"WS2_32.lib")

static SOCKET ci_hmi_sock; /*在windows下创建socket使用网络*/
static struct sockaddr_in hmi_addr;

#else
#   include <stdarg.h>
#   include <sys/ioctl.h>
#   include <fcntl.h>

#   include "series_manage.h"
#   include "switch_board_manage.h"

#   define NET_IOC_MAGIC  'k'
#   define NET_IOCRESET    _IO(NET_IOC_MAGIC, 0)
#   define NET_IOC_DEST_IP_A _IOWR(NET_IOC_MAGIC,  50, int)
#   define NET_IOC_DEST_PORT_A _IOWR(NET_IOC_MAGIC,  51, int)
#   define NET_IOC_DEST_IP_B _IOWR(NET_IOC_MAGIC,  52, int)
#   define NET_IOC_DEST_PORT_B _IOWR(NET_IOC_MAGIC,  53, int)
#   define NET_IOC_RESET _IOWR(NET_IOC_MAGIC,  54, int)

/*与控显机通信文件句柄*/
static int32_t hmi_fd_a;
static int32_t hmi_fd_b;

#endif /*WIN32*/
#   include "cpu_manage.h"

#include <time.h>
#include "hmi_manage.h"
#include "eeu_manage.h"

#include "util/algorithms.h"
#include "util/config.h"
#include "util/app_config.h"
#include "util/utility.h"
#include "util/log.h"

#include "interlocking/inter_api.h"
#include "device_type.h"
#include "remote_log.h"

#define HMI_FRAME_RECIEVE_MAX_LEN   0xFF    /*控显机发送过来的数据的最大长度*/
#define HMI_FRAME_SEND_MAX_LEN      0x7FF   /*向控显机发送的数据的最大长度*/

#define HMI_FRAME_BASE_LEN          19      /*除去帧内容的长度*/
#define HMI_FRAME_RECIEVE_DATA_LEN  (HMI_FRAME_RECIEVE_MAX_LEN - HMI_FRAME_BASE_LEN)
#define HMI_FRAME_SEND_DATA_LEN     (HMI_FRAME_SEND_MAX_LEN - HMI_FRAME_BASE_LEN)

#define HMI_TIPS_MAXLEN 512
#define HMI_SEND_BUF_LEN 0x7f0

static uint32_t hmi_ip_a = 0;            /*控显机A口的ip地址*/
static uint32_t hmi_ip_b = 0;            /*控显机B口的ip地址*/
static uint16_t hmi_port_a = 0;           /*控显机A口的端口地址*/
static uint16_t hmi_port_b = 0;           /*控显机A口的端口地址*/

static uint32_t last_time_stamp = 0xffffffff;
static uint16_t last_frame_sn = 0xffff;

static char hmi_send_buf[HMI_SEND_BUF_LEN] = {0};   /*发送缓存*/
static int32_t hmi_send_cache_len = 0;

static CI_BOOL b_print_hmi = CI_FALSE;  /*是否打印控显机信息*/
static CI_BOOL b_hmi_linked = CI_TRUE; /*用来标志控显机是否链接*/

static uint8_t hmi_device_id = 0;

static CI_BOOL is_channel_a_ok = CI_FALSE;  /*用以判断通信线路是否正常*/
static CI_BOOL is_channel_b_ok = CI_FALSE;  /*用以判断通信线路是否正常*/

/*
 * 帧数据类型
 */
typedef enum _HmiDataType
{
    DT_TIME_STRING                      = 1,        /*对时帧*/
    DT_NETWORK_RESPONSE                 = 2,        /*应答帧*/
    DT_NETWORK_DETECT                   = 3,        /*网络检测帧*/
    DT_HMI_OPERATION_COMMAND            = 4,        /*控显机到联锁机的操作命令*/
    DT_HMI_LINK_REQUEST                 = 5,        /*控显机到联锁机的连接请求*/
    DT_HMI_SWITCH_COMMAND               = 6,        /*控显机到联锁机的双系切换命令*/
    DT_INTERLOCKING_DEVICE_STATUS       = 7,        /*联锁机到控显机的室外设备状态数据*/
    DT_INTERLOCKING_DEBUG_TIPS          = 8,        /*联锁机到控显机的调试提示信息数据*/
    DT_INTERLOCKING_ALLOW_LINK          = 9,        /*联锁机到控显机的允许连接命令*/
    DT_INTERLOCKING_ERR_TIPS            = 10,       /*联锁机到控显机的错误提示信息数据*/
    DT_INTERLOCKING_TIMER_TIPS          = 11,       /*联锁机到控显机的倒计时提示信息数据*/
    DT_INTERLOCKING_NORMAL_TIPS         = 12,       /*联锁机到控显机的正常提示信息数据*/
    DT_INTERLOCKING_STATUS              = 16,       /*联锁机到控显机的主备系状态*/
    DT_HMI_PING                         = 17,       /*控显机到联锁机的ping命令*/
}HmiDataType;
/*
 * 帧头
 */
typedef struct _HmiFrameHead
{
    uint16_t frame_head_tag;                /*帧头标志*/
    uint16_t frame_sn;                      /*帧编号*/
    uint32_t time_stamp;                    /*时间戳*/
    uint8_t send_device_id;                 /*发送设备ID*/
    uint8_t send_device_type;               /*发送设备类型*/
    uint8_t send_station_id;                /*发送站场号*/
    uint8_t recieve_device_id;              /*接收设备ID*/
    uint8_t recieve_device_type;            /*接收设备类型*/
    uint8_t recieve_station_id;             /*接收站场号*/
    uint8_t data_type;                      /*数据类型*/
    uint16_t data_length;                   /*数据域数据长度*/	
}HmiFrameHead;
/*
 * 从控显机接受的帧
 */
typedef struct _HmiRecieveFrame
{
    HmiFrameHead head;                       /*头*/
    int16_t button1;
    int16_t button2;
    int16_t button3;
    int16_t button4;
    int16_t button5;
    uint16_t crc;                            /*crc校验码*/
}HmiRecieveFrame;

extern int32_t CIMonitor_Write(const void* buf,int32_t data_len);

/*
 功能描述    : 打开控显机信息输出
 返回值      : 成功为0，失败为-1
 参数        : 
 日期        : 2014年4月11日 15:45:54
*/
int32_t CIHmi_OpenPrintHmi(void)
{
    b_print_hmi = CI_TRUE;

    return 0;
}
/*
 功能描述    : 返回已整数值保存的ip地址
 返回值      : 控显机的IP地址
 参数        : 
 日期        : 2014年5月12日 10:32:43
*/
uint32_t CIHmi_GetIpA(void)
{
    /*该函数下应该只在linux下使用，在Windows下无意义*/
    return hmi_ip_a;
}
/*
 功能描述    : 返回控显机的端口
 返回值      : 控显机的端口
 参数        : 
 日期        : 2014年5月12日 10:33:08
*/
uint16_t CIHmi_GetPortA(void)
{
    return hmi_port_a;
}
/*
 功能描述    : 返回已整数值保存的B口ip地址
 返回值      : 控显机的IP地址
 参数        : 
 作者        : 张彦升
 日期        : 2014年5月12日 10:32:43
*/
uint32_t CIHmi_GetIpB(void)
{
    return hmi_ip_b;
}
/*
 功能描述    : 返回控显机B口的端口
 返回值      : 控显机的端口
 参数        : 
 作者        : 张彦升
 日期        : 2014年5月12日 10:33:08
*/
uint16_t CIHmi_GetPortB(void)
{
    return hmi_port_b;
}
/*
 功能描述    : 返回控显机设备id
 返回值      : 控显机的id
 参数        : 
 作者        : 张彦升
 日期        : 2014年5月12日 10:33:08
*/
uint8_t CIHmi_GetDeviceId(void)
{
    return hmi_device_id;
}
/*
 功能描述    : 向联锁机发送数据，该发送函数不使用缓存
 返回值      : 成功为0，失败为-1
 参数        : 
 日期        : 2014年7月24日 13:16:56
*/
static int32_t hmi_direct_write(const void* buf,int32_t nbytes)
{
    int32_t write_num = 0;

    assert(NULL != buf);
#ifdef WIN32
    write_num = sendto(ci_hmi_sock,
                        (const char*)buf,
                        nbytes,
                        0,
                        (struct sockaddr *)&hmi_addr,
                        sizeof(hmi_addr));
#else
    /*两系的主CPU分别下发数据，从CPU从不下发数据*/
    if (CPU_STATE_MASTER == CICpu_GetLocalState())
    {
        write_num = write(hmi_fd_a,buf,nbytes);
        CIMonitor_Write(buf,nbytes);
    }
    else
    {
        return 0;
    }
#endif /*WIN32*/

    if (0 >= write_num)
    {
        if (CI_TRUE == b_print_hmi)
        {
            CILog_Errno("hmi_send_fail");
        }
        return -1;
    }
    else
    {
        if (CI_TRUE == b_print_hmi)
        {
            CILog_Msg("hmi_send_success");
        }
    }
    return write_num;
}
/*
 功能描述    : 向联锁机下发数据，该发送函数使用缓存
 返回值      : 成功下发的字节个数
 参数        : @buf 要下发的数据指针
              @nbytes 数据的长度
 日期        : 2014年6月26日 10:34:42
*/
static int32_t hmi_cache_write(const void* buf,int32_t nbytes)
{
    assert(nbytes < HMI_SEND_BUF_LEN);
    assert(NULL != buf);

    /*范围超过*/
    if (HMI_SEND_BUF_LEN < hmi_send_cache_len + nbytes)
    {
        CIHmi_WriteCachePush();
    }

    memcpy(hmi_send_buf + hmi_send_cache_len,buf,nbytes);
    hmi_send_cache_len += nbytes;

    return 0;
}
/*
 功能描述    : 将缓存当中的数据全部发送
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2014年9月17日 13:26:35
*/
int32_t CIHmi_WriteCachePush(void)
{
    if (0 != hmi_send_cache_len)
    {
        hmi_direct_write(hmi_send_buf,hmi_send_cache_len);
        /*发送完成之后清除缓存并将本次数据放在缓存当中*/
        memset(hmi_send_buf,0,HMI_SEND_BUF_LEN);
        hmi_send_cache_len = 0;
    }

    return 0;
}
/*
功能描述    : 根据帧内容解析出按下的按钮
返回值      : 成功为0，失败为-1
参数        : 
日期        : 2014年3月5日 9:15:01
*/
static int16_t hmi_parser_pressbutton(const HmiRecieveFrame* frame)
{
    assert(NULL != frame);

    CIHmi_SendDebugTips("hmi recieve:A1:%d,A2:%d,A3:%d,A4:%d,A5:%d",
        frame->button1, frame->button2, frame->button3, frame->button4, frame->button5);

    /*将该数据传递给联锁运算层*/
    CICommand_SetButton(frame->button1,
        frame->button2,
        frame->button3,
        frame->button4,
        frame->button5);

    return 0;
}
/*
 功能描述    : 装配数据
 返回值      : 装备的数据大小
 参数        : @data 帧数据域
              @data_len 帧数据域长度
              @data_type 数据类型
              @dist_buff 装配后的buffer
              @dist_size dist_buff的最大长度
              @assembled_size 装配数据的大小
 日期        : 2014年7月24日 13:03:01
*/
static int32_t hmi_assemble_data( void* dist,
                                 int32_t dist_size,
                                 const void* src,
                                 int32_t src_size,
                                 uint8_t data_type
                                 )
{
    HmiFrameHead head;
    static uint16_t frame_sn = 0;
    int8_t series_status;
    int8_t eeu_channel_status_a;
    int8_t eeu_channel_status_b;
    uint16_t crc = 0;
    int32_t len = 0;

    if (NULL == dist)
    {
        return -1;
    }

    head.frame_head_tag = 0xAA55;
    head.frame_sn = frame_sn ++;
    head.time_stamp = CI_GetTimeStamp();
    head.send_device_id = (uint8_t)CIConfig_GetDeviceId();
    head.send_device_type = DT_CI;
    head.send_station_id = 0; /*现有联锁逻辑当中不再区分站场号，默认为0能保证控显机正常通信*/
    /*注意，当send_station_id不为0的时候控显机通信板硬件程序可能存在bug*/
    head.recieve_device_id = hmi_device_id;
    head.recieve_device_type = DT_HMI;
    head.recieve_station_id = 0; /*不再使用station_id，在多站场当中避免数据错乱使用device_id控制*/
    head.data_type = data_type;

    assert(0xFFFF > src_size);

    head.data_length = (uint16_t)src_size;

    /*检查允许的帧数据长度*/
    if (HMI_FRAME_SEND_DATA_LEN < src_size)
    {
        CILog_Msg("hmi send HMI_FRAME_SEND_DATA_LEN(%d) < data_len(%d)",
                    HMI_FRAME_SEND_DATA_LEN,
                    src_size);
        return -1;
    }

    if (HMI_FRAME_SEND_MAX_LEN > dist_size)
    {
        CILog_Msg("hmi send HMI_FRAME_SEND_MAX_LEN > buff_max_size(%d)", dist_size);
        return -1;
    }

#define HMI_SEND_MEMCPY(data)                       \
    memcpy((char*)dist + len,&data,sizeof(data));   \
    len += sizeof(data);

    /*copy head*/
    HMI_SEND_MEMCPY(head)

#ifdef WIN32
    series_status = SERIES_MASTER;
    eeu_channel_status_a = 0;
    eeu_channel_status_b = 0;
#else
    series_status = CISeries_GetLocalState();
    eeu_channel_status_a = CIEeu_IsChannelAOk();
    eeu_channel_status_b = CIEeu_IsChannelBOk();
#endif /* WIN32*/

    HMI_SEND_MEMCPY(series_status)
    HMI_SEND_MEMCPY(eeu_channel_status_a)
    HMI_SEND_MEMCPY(eeu_channel_status_b)

    /*copy data*/
    if (NULL != src)
    {
        memcpy((char*)dist + len,src,src_size);
        len += src_size;
    }

    /*calculate crc and copy*/
    crc = CIAlgorithm_Crc16(dist,len);
    HMI_SEND_MEMCPY(crc);

#undef HMI_SEND_MEMCPY
    return len;
}
/*
功能描述    : 发送提示信息信息
返回值      : 无
参数        : @fmt 标准C格式
日期        : 2013年12月6日 18:45:27
修订        :
    20150312 张彦升 向该函数当中添加CILog_Msg。原来版本当中在PRINTF宏当中使用
       两个CILog_Msg进行记录，这种方式并不好，现在希望联锁部分CIHmi_SendNormalTips
       和CIHmi_SendDebugTips这两个函数作为接口即可，不要使用PRINTF宏，详见日志
       记录模型
*/
static int32_t hmi_send_tips(HmiDataType data_type,const char *fmt, va_list args)
{
    int32_t buff_len = 0;
    int32_t data_len = 0;
    int ret = 0;

    static char data_buff[HMI_TIPS_MAXLEN] = {0};
    static uint8_t send_buff[HMI_FRAME_SEND_MAX_LEN] = {0};

    memset(data_buff,0,HMI_TIPS_MAXLEN);
    memset(send_buff,0,HMI_FRAME_SEND_MAX_LEN);

    if (NULL != fmt)
    {
        data_len = vsnprintf(data_buff, HMI_TIPS_MAXLEN, fmt, args);

        /*truncated*/
#ifdef WIN32
        if (0 >= data_len) 
        {
            data_buff[HMI_TIPS_MAXLEN - 1] = 0;
            data_len = HMI_TIPS_MAXLEN;
        } 
#else
        if (data_len >= HMI_TIPS_MAXLEN)
        {
            data_buff[HMI_TIPS_MAXLEN - 1] = 0;
            data_len = HMI_TIPS_MAXLEN;
        }
#endif /* WIN32*/
    }

#ifdef WIN32
    CILog_Msg("%s",data_buff);
#else
#   ifndef CI_UNIT_TEST
    CIRemoteLog_Write("%s",data_buff);
#   endif /*CI_UNIT_TEST*/
#endif /* WIN32*/

    /*只有主CPU发送数据*/
    if(CPU_STATE_MASTER != CICpu_GetLocalState())
    {
        return 0;
    }

    buff_len = hmi_assemble_data(send_buff,HMI_FRAME_SEND_MAX_LEN,
                                 data_buff,data_len,
                                 (uint8_t)data_type);

#ifdef CI_UNIT_TEST
    return buff_len;
#else
    ret = hmi_cache_write(send_buff,buff_len);
    return ret;
#endif /* CI_UNIT_TEST*/
}
/*
功能描述    : 联锁机向控显机发送的需要向提示框显示的信息
返回值      : 无
参数        : @fmt 以printf标准格式传入参数并格式化打印输出
作者        : 何境泰
日期        : 2013年12月6日 18:45:27
*/
int32_t CIHmi_SendNormalTips(char* fmt, ...)
{
    int32_t ret = 0;
    va_list ap;

    va_start(ap, fmt);
    ret = hmi_send_tips(DT_INTERLOCKING_NORMAL_TIPS,fmt, ap);
    va_end(ap);

    return ret;
}
/*
功能描述    : 发送调试数据信息
返回值      : 
参数        : 
日期        : 2013年12月6日 18:45:27
*/
int32_t CIHmi_SendDebugTips(char* fmt,...)
{
    int32_t ret = 0;
    va_list ap;

    va_start(ap, fmt);
    ret = hmi_send_tips(DT_INTERLOCKING_DEBUG_TIPS,fmt, ap);
    va_end(ap);

    return ret;
}
/*
功能描述    : 发送timer数据信息
返回值      : 无
参数        : @fmt 标准C格式
日期        : 2013年12月6日 18:45:27
*/
int32_t CIHmi_SendTimerTips(char* fmt,...)
{
    int32_t ret = 0;
    va_list ap;

    va_start(ap, fmt);
    ret = hmi_send_tips(DT_INTERLOCKING_TIMER_TIPS,fmt, ap);
    va_end(ap);

    return ret;
}
/*
 功能描述    : 发送节点数据
 返回值      : 
 参数        : 
 日期        : 2013年12月13日 13:45:04
*/
int32_t CIHmi_SendNodeData(void)
{
    node_t i = 0;
    int32_t ret = 0;
    int32_t buff_len = 0;
    static uint16_t device_status[MAX_SIGNAL_NODES] = {0};
    static uint8_t send_buff[HMI_FRAME_SEND_MAX_LEN] = {0};

    if(CICpu_GetLocalState() != CPU_STATE_MASTER)
    {
        return 0;
    }
#ifdef CI_UNIT_TEST
# ifdef TOTAL_SIGNAL_NODE
#   undef TOTAL_SIGNAL_NODE
#   define TOTAL_SIGNAL_NODE 0
# endif /*TOTAL_SIGNAL_NODE*/
#endif /*CI_UNIT_TEST*/
    /*获取状态数据*/
    for (i = 0; i < TOTAL_SIGNAL_NODE; i++)
    {
        device_status[i] = CINode_GetData(i);
    }

    buff_len = hmi_assemble_data(send_buff,HMI_FRAME_SEND_MAX_LEN,
        device_status, TOTAL_SIGNAL_NODE * sizeof(uint16_t),
                                 DT_INTERLOCKING_DEVICE_STATUS);

    ret = hmi_direct_write(send_buff,buff_len);

#ifdef CI_UNIT_TEST
# undef TOTAL_SIGNAL_NODE
#endif /*CI_UNIT_TEST*/

    return ret;
}
/*
 功能描述    : 发送允许连接帧
 返回值      : 成功为0，失败为-1
 参数        : 
 日期        : 2014年7月21日 13:17:38
*/
static int32_t hmi_allow_link(void)
{
    uint8_t send_buff[HMI_FRAME_SEND_MAX_LEN] = {0};
    int32_t data_size = 0;

    b_hmi_linked = CI_TRUE;

#ifndef WIN32
    if (CPU_STATE_MASTER == CICpu_GetLocalState())
    {
#endif /* WIN32*/
        CIHmi_SendNormalTips("收到控显机请求链接信息，正在发送允许请求帧");
        data_size = hmi_assemble_data(send_buff,HMI_FRAME_SEND_MAX_LEN,
                                      NULL,0,
                                      DT_INTERLOCKING_ALLOW_LINK);

        hmi_direct_write(send_buff,data_size);
#ifndef WIN32
    }
#endif /* WIN32*/

    return 0;
}
/*
功能描述    : 从联锁接收控显机数据
返回值      : 成功为0，失败为-1
参数        :
日期        : 2014年6月26日 10:34:42
*/
static int32_t hmi_read(int32_t fd, HmiRecieveFrame* frame)
{
    int32_t read_num;
    HmiFrameHead* head = NULL;
    uint16_t crc = 0;

    memset(frame, 0, sizeof(*frame));

    assert(NULL != frame);

#ifdef WIN32
    /*Windows下不使用fd参数，直接使用ci_hmi_sock*/
    read_num = recvfrom(ci_hmi_sock, (char*)frame, sizeof(*frame), 0, NULL, NULL);
#else
    read_num = read(fd, frame, sizeof(*frame));
#endif /*WIN32*/
    if (0 >= read_num)
    {
#ifdef WIN32
        /*CILog_Msg("hmi_recv_fail:%d",WSAGetLastError());*/
#endif /*WIN32*/

        if (CI_TRUE == b_print_hmi)
        {
            CILog_Errno("hmi_recv_fail");
        }

        return -1;
    }
    else if (sizeof(*frame) != read_num)
    {
        CILog_Msg("控显机发送帧长度不正确:%d!=%d",sizeof(*frame),read_num);

        return -1;
    }

    head = &frame->head;
    /*将帧头直接拷贝过去，因为在x86体系上都是用的是小字节序所以不会出现问题*/
    if (0xAA55 != head->frame_head_tag)
    {
        CIHmi_SendNormalTips("hmi frame head is not 0xAA55(%#x)", head->frame_head_tag);

        return -1;
    }
    /*数据区加帧头及其它的数据不可能大于0xff*/
    if (HMI_FRAME_RECIEVE_DATA_LEN < head->data_length)
    {
        CIHmi_SendNormalTips("hmi data parse failed：HMI_FRAME_RECIEVE_DATA_LEN(%d) < head->data_length(%d)",
            HMI_FRAME_RECIEVE_DATA_LEN, head->data_length);

        return -1;
    }

    crc = CIAlgorithm_Crc16(frame, sizeof(*frame) - sizeof(uint16_t));
    if (crc != frame->crc)
    {
        CIHmi_SendNormalTips("hmi frame crc wrong %#X != %#x", crc, frame->crc);

        return -1;
    }

    /*检查数据重复性*/
    if (frame->head.time_stamp == last_time_stamp)
    {
        CIHmi_SendNormalTips("hmi frame timestamp(%#x) same as last", last_time_stamp);

        return -1;
    }
    /*当为请求链接帧时不检测帧序号，这样可以使帧序号从头开始*/
    if(DT_HMI_LINK_REQUEST != frame->head.data_type)
    {
        /*
        if (frame->head.frame_sn == last_frame_sn)
        {
            CIHmi_SendNormalTips("hmi frame fsn(%#x) same as last", last_frame_sn);

            return -1;
        }
        */
    }

    /*TODO 检查数据丢失，检查控显机链接失效*/

    if (DT_HMI != frame->head.send_device_type)
    {
        CIHmi_SendNormalTips("控显机发送帧类型号应是:%d,而非:%d", DT_HMI, frame->head.send_device_type);

        return -1;
    }
    /*
    if (hmi_device_id != frame->head.send_device_id)
    {
        CIHmi_SendNormalTips("控显机发送设备编号(%d!=%d)", hmi_device_id, frame->head.send_device_id);

        return -1;
    }
    */

    last_time_stamp = frame->head.time_stamp;
    last_frame_sn = frame->head.frame_sn;

    return 0;
}
/*
功能描述    : 从控显机接收数据
返回值      : 
参数        : 
日期        : 2013年12月13日 14:10:33
*/
int32_t CIHmi_RecvData(void)
{
    static HmiRecieveFrame frame;
    int32_t ret = 0;
    static uint8_t buf[HMI_FRAME_RECIEVE_MAX_LEN];

    /*控显机每一个周期可发送一个空的数据作为心跳数据，用以判断控显机的存活*/
    is_channel_a_ok = CI_FALSE;
    is_channel_b_ok = CI_FALSE;

#ifdef WIN32
    ret = hmi_read(0,&frame);
    if (0 > ret)
    {
        return -1;
    }
#else
    ret = hmi_read(hmi_fd_a,&frame);
    if (0 > ret)
    {
        ret = hmi_read(hmi_fd_b,&frame);
        if (0 > ret)
        {
            return -1;
        }
    }
    else
    {
        is_channel_a_ok = CI_TRUE;
        /*如果从A口收到数据了则清空缓存*/
        if (read(hmi_fd_b, buf, HMI_FRAME_RECIEVE_MAX_LEN) == 0)
        {
            is_channel_b_ok = CI_TRUE;
        }
    }

#endif /* WIN32*/

#if 0
    if (CI_TRUE == b_print_hmi)
    {
        CILog_Msg("hmi_recv_success");
    }
#endif

    /*根据不同的数据类型做不同的动作*/
    switch(frame.head.data_type)
    {
    case DT_TIME_STRING:
        break;
    case DT_NETWORK_RESPONSE:
        break;
    case DT_NETWORK_DETECT:
        break;
    case DT_HMI_OPERATION_COMMAND:
        if (CI_TRUE == b_hmi_linked)
        {
            hmi_parser_pressbutton(&frame);
        }
        else
        {
            CIHmi_SendNormalTips("控显机链接过期，拒绝操作");
        }
        break;
    case DT_HMI_LINK_REQUEST:
        hmi_allow_link();
        break;
    case DT_HMI_SWITCH_COMMAND:
#ifndef WIN32
#ifndef CI_UNIT_TEST
        if (SERIES_MASTER == CISeries_GetLocalState())
        {
            if(CI_TRUE != b_hmi_linked)
            {
                CIHmi_SendNormalTips("控显机链接过期，拒绝切换");
            }
            else
            {
                CIHmi_SendNormalTips("收到控显机切换命令，正在切换");
                /*强制进行切换*/
                CISwitch_ProduceSwitchCmd();
            }
        }
        else
        {
            CIHmi_SendNormalTips("联锁机状态为非主系状态，拒绝切换");
        }
#endif /*CI_UNIT_TEST*/
#endif /* WIN32*/
        break;
    case DT_HMI_PING:
        break;
    default:
        CIHmi_SendNormalTips("控显机帧数据类型未知:%d", frame.head.data_type);
        return -1;
    }

    return 0;
}
/*
 功能描述    : 控显机A通信道是否断开
 返回值      : 若断开则返回CI_TRUE，否则返回CI_FALSE
 参数        : 无
 日期        : 2015年5月29日 18:34:31
*/
CI_BOOL CIHmi_IsChannelAOk(void)
{
    return is_channel_a_ok;
}
/*
 功能描述    : 控显机B通信道是否断开
 返回值      : 若断开则返回CI_TRUE，否则返回CI_FALSE
 参数        : 无
 日期        : 2015年5月29日 18:34:31
*/
CI_BOOL CIHmi_IsChannelBOk(void)
{
    return is_channel_b_ok;
}
/*
 功能描述    : 从配置文件当中找到配置名为config_name的一项，并将其转换为32位的ip地址
 返回值      : 成功为0，失败为-1
 参数        : @config_name配置文件的名称
               @ip返回的ip地址
 日期        : 2015年5月19日 10:06:27
*/
static int32_t hmi_get_ip(const char* config_name,uint32_t* ip)
{
    const char* hmi_ip_str = NULL;

    *ip = 0;

    /*try get ip address from configuration file*/
    hmi_ip_str = CIConfig_GetValue(config_name);
    if (NULL == hmi_ip_str)
    {
        CILog_Msg("配置文件当中找不到%s项",config_name);
        return -1;
    }
    if (CI_FALSE == CI_ValidateIpAddress(hmi_ip_str))
    {
        CILog_Msg("配置文件当中%s项值不符合ip规范",config_name);
        return -1;
    }
    return CI_StrIpToInt32(hmi_ip_str,ip);
}
#ifdef WIN32
/*
 功能描述    : 从配置文件当中得到配置项，并验证其是否是ip格式，最终返回配置项字符串指针
 返回值      : 成功为字符串的配置项指针，否则为返回NULL
 参数        : @config_name配置项名称
 日期        : 2015年5月19日 10:51:49
*/
static const char* hmi_get_ip_str(const char* config_name)
{
    const char* hmi_ip_str = NULL;

    /*try get ip address from configuration file*/
    hmi_ip_str = CIConfig_GetValue(config_name);
    if (NULL == hmi_ip_str)
    {
        CILog_Msg("配置文件当中找不到%s项",config_name);
        return NULL;
    }
    if (CI_FALSE == CI_ValidateIpAddress(hmi_ip_str))
    {
        CILog_Msg("配置文件当中%s项值不符合ip规范",config_name);
        return NULL;
    }
    return hmi_ip_str;
}
#endif
/*
 功能描述    : 从配置文件当中读取config_name配置项，并将其转换为端口地址
 返回值      : 成功为0，失败为-1
 参数        : @config_name配置项的名称
              @port要写入的端口地址，若函数返回错误，则其值被改为0
 日期        : 2015年5月19日 10:12:30
*/
static int32_t hmi_get_port(const char* config_name,uint16_t* port)
{
    const char* hmi_port_str = NULL;
    
    *port = 0;

    /*try get hmi port from configuration file*/
    hmi_port_str = CIConfig_GetValue(config_name);
    if (NULL == hmi_port_str)
    {
        CILog_Msg("配置文件当中找不到%s项",config_name);
        return -1;
    }
    /*在未强制转换前验证一次控显机端口，如果其大于65535则报错*/
    if (CI_FALSE == CI_ValidatePort(atoi(hmi_port_str)))
    {
        CILog_Msg("在配置文件当中%s值不符合规范",config_name);
        return -1;
    }
    *port = (uint16_t)atoi(hmi_port_str);

    return 0;
}
#ifndef WIN32 /*this function used only under linux envrionment*/
/*
 功能描述    : 控显机ip地址和端口初始化
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年3月19日 11:04:34
*/
static int32_t hmi_ip_port_init(void)
{
    int32_t ret = 0;

    ret = hmi_get_ip("HMIIpA",&hmi_ip_a);
    if (0 > ret)
    {
        return -1;
    }
    ret = hmi_get_ip("HMIIpB",&hmi_ip_b);
    if (0 > ret)
    {
        return -1;
    }

    ret = hmi_get_port("HMIListenPortA", &hmi_port_a);
    if (0 > ret)
    {
        return -1;
    }
    ret = hmi_get_port("HMIListenPortB", &hmi_port_b);
    if (0 > ret)
    {
        return -1;
    }

    return 0;
}
#endif

#ifdef WIN32

/*
 功能描述    : 在windows下使用socket直接与控显机进行通信
 返回值      : 成功为0，失败为-1
 参数        : 
 日期        : 2014年6月17日 20:12:52
*/
static int32_t hmi_fd_init(void)
{
    WSADATA wsa_data;
    WORD sock_version = MAKEWORD(2,2);
    unsigned long ul = 1;                       /*1 block,0 nonblock*/
    int32_t ret = 0;
    const char* ci_ip = NULL;                   /*本机地址*/
    uint16_t ci_listen_port = 0;                /*本机与控显机通信的端口*/

    const char* hmi_ip = NULL;              /*Windows下控显机的地址*/
    uint16_t hmi_port = 0;            /*Windows下控显机的端口*/

    struct sockaddr_in ci_serv_addr;

    hmi_ip = hmi_get_ip_str("HMIIp");
    assert(NULL != hmi_ip);
    ret = hmi_get_port("HMIListenPort", &hmi_port);
    assert(-1 != ret);

    ci_ip = hmi_get_ip_str("CiLocalhostIp");
    assert(NULL != ci_ip);
    ret = hmi_get_port("CiListenPort", &ci_listen_port);
    assert(-1 != ret);

    if(0 != WSAStartup(sock_version, &wsa_data))
    {
        return 0;
    }

    ci_hmi_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(INVALID_SOCKET == ci_hmi_sock)
    {
        CILog_Msg("socket() failed:%d\n", WSAGetLastError());
        return -1;
    }

    ci_serv_addr.sin_addr.S_un.S_addr = inet_addr(ci_ip);;
    ci_serv_addr.sin_family = AF_INET;
    ci_serv_addr.sin_port = htons(ci_listen_port);

    hmi_addr.sin_addr.S_un.S_addr = inet_addr(hmi_ip);;
    hmi_addr.sin_family = AF_INET;
    hmi_addr.sin_port = htons(hmi_port);

    if(SOCKET_ERROR == bind(ci_hmi_sock, (struct sockaddr *)&ci_serv_addr, sizeof(ci_serv_addr)))
    {
        CILog_Errno("绑定本地通信地址失败，请检查CiLocalhostIp配置，Windows平台下应为本机地址");
        closesocket(ci_hmi_sock);
        return -1;
    }

    /*设置成非阻塞模式*/
    ret=ioctlsocket(ci_hmi_sock,FIONBIO,(unsigned long *)&ul);
    if(SOCKET_ERROR == ret)
    {
        CILog_Errno("设置本地SOCKET为广播失败");
        closesocket(ci_hmi_sock);
        return -1;
    }
    return 0;
}
#else
/*
功能描述    : 初始化与控显机交互的文件fd
返回值      : 
参数        : 
日期        : 2014年2月19日 6:05:23
*/
static int32_t hmi_fd_init(void)
{
    hmi_fd_a = open("/dev/neta",O_RDWR);

    if (-1 == hmi_fd_a)
    {
        CILog_Errno("open /dev/neta failed");

        return -1;
    }

    hmi_fd_b = open("/dev/netb",O_RDONLY);

    if (-1 == hmi_fd_b)
    {
        CILog_Errno("open /dev/netb failed");

        return -1;
    }

    if (0 == hmi_ip_a || 0 == hmi_port_a || 0 == hmi_ip_b || 0 == hmi_port_b)
    {
        CILog_Msg("please init hmi ip or port first");
        return -1;
    }
    /*只需向A口写入即可，B口与A口任意一个都可更改ip和端口信息*/
    ioctl(hmi_fd_a,NET_IOC_DEST_IP_A,hmi_ip_a);
    ioctl(hmi_fd_a,NET_IOC_DEST_PORT_A,hmi_port_a);

    ioctl(hmi_fd_b,NET_IOC_DEST_IP_B,hmi_ip_b);
    ioctl(hmi_fd_b,NET_IOC_DEST_PORT_B,hmi_port_b);

    /*PC104可复位A口，PC104可复位B口*/
    ioctl(hmi_fd_a,NET_IOC_RESET);
    ioctl(hmi_fd_b,NET_IOC_RESET);

    return 0;
}
#endif /*WIN32*/
/*
功能描述    : 初始化控显机相关数据
返回值      : 成功为0，失败为-1
参数        : 
日期        : 2013年12月4日 8:39:07
*/
int32_t CIHmi_Init(void)
{
    static CI_BOOL b_initialized = CI_FALSE;
    int ret = 0;
    const char* s = NULL;

#ifndef CI_UNIT_TEST
    if (CI_TRUE == b_initialized)
    {
        return -1;
    }
#endif /* CI_UNIT_TEST*/

#ifndef WIN32
    ret = hmi_ip_port_init();
    if (0 > ret)
    {
        return -1;
    }
#endif /* WIN32*/

    ret = hmi_fd_init();
    if (0 > ret)
    {
        return -1;
    }

    s = CIConfig_GetValue("HMIDeviceID");
    if (NULL == s)
    {
        CILog_Msg("配置文件当中找不到HMIDeviceID项");
        return -1;
    }

    hmi_device_id = (uint8_t)atoi(s);

    b_initialized = CI_TRUE;

    return 0;
}
