/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年11月4日 9:57:13
用途        : 双系切换板通信模块
历史修改记录: v1.0    创建
**********************************************************************/
#include "util/ci_header.h"

#ifdef LINUX_ENVRIONMENT

#include <fcntl.h>
#include <sys/ioctl.h>

#include "util/ci_header.h"
#include "util/algorithms.h"
#include "util/utility.h"
#include "util/log.h"

#include "cycle_int.h"
#include "timer.h"
#include "failures_int.h"

#include "switch_board_manage.h"
#include "series_manage.h"
#include "cpu_manage.h"

#define UART_IOC_MAGIC  'k'
#define UART_IOCRESET    _IO(UART_IOC_MAGIC, 0)
#define UART_IOC_CLEAN_MEMORY _IOWR(UART_IOC_MAGIC,  60, int)
#define UART_IOC_MAXNR 1

#define UART_BUFFER_SIZE 0x7f

#define SWITCH_CYCLE_NANO (50000000 * 3)      /*SWITCH控制的周期，使用纳秒级，默认为50ms*/
#define SWITCH_CYCLE_RELATIVE ((int)(SWITCH_CYCLE_NANO / TIMER_CYCLE_NANO))  /*相对于主定时器的周期*/

/*手动切换标志，当从切换板当中得到的数据显示为需要手动切换时设置该项为真*/
static int32_t switch_fd_a = 0;
static int32_t switch_fd_b = 0;

static CI_BOOL b_switch = CI_FALSE;
static CI_BOOL b_peer_alive = CI_FALSE;
static CI_BOOL b_recved_switch_cmd = CI_FALSE;

static uint32_t last_recv_sn = 0;

static void switch_action(void);

static CI_BOOL switch_condition_check(uint32_t elapsed_cycle);

static const CITimer switch_timer = 
{
    SWITCH_CYCLE_RELATIVE,
    switch_action,
    switch_condition_check,
    "switch_timer",
    CI_TRUE,                        /*默认打开该定时器*/
};
/*
 功能描述    : 向切换板发送双系状态信息
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年9月22日 10:58:21
*/
static int32_t switch_send_series_state(void)
{
    static SwitchCiFrame frame;   /*init relying on static initialize*/
    int32_t ret = 0;
    static int sn = 0;

    frame.frame_head_tag = 0x6a6a;
    frame.series_state = (uint8_t)CISeries_GetLocalState();
    frame.sn = sn;

    frame.hash = CIAlgorithm_Crc16(&frame,sizeof(frame) - sizeof(uint16_t));

    /*CI_HexDump(&frame,sizeof(frame));*/

    ret = write(switch_fd_a,&frame,sizeof(frame));
    if (-1 == ret)
    {
        CILog_SigMsg("switch board send data failed");
        return -1;
    }

    sn ++;

    return 0;
}
/*
 功能描述    : 验证数据传输是否存在问题
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年9月22日 10:59:40
*/
static CI_BOOL switch_frame_verify(SwitchBoardFrame* frame)
{
    uint16_t crc_code = 0;

    assert(NULL != frame);

    /*CI_HexDump(frame,sizeof(*frame));*/
    crc_code = CIAlgorithm_Crc16(frame,sizeof(*frame) - sizeof(uint16_t));

    if (crc_code != frame->hash)
    {
        CILog_SigMsg("switch_hash_fail:%#x!=%#x",crc_code,frame->hash);
        return CI_FALSE;
    }

    if (frame->frame_head_tag != 0x6a6a)
    {
        CILog_SigMsg("switch_head_error: %#x",frame->frame_head_tag);
        return CI_FALSE;
    }

    return CI_TRUE;
}
/*
 功能描述    : 从切换板接收数据
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年9月22日 10:59:26
*/
static int32_t switch_recv_frame(int32_t fd,SwitchBoardFrame* frame,const char* fd_name)
{
    int32_t ret = 0;
    uint32_t i = 0;
    static uint8_t recv_buf[UART_BUFFER_SIZE] = {0};

    memset(recv_buf, 0, UART_BUFFER_SIZE);

    ret = read(fd,recv_buf,UART_BUFFER_SIZE);
    if (0 >= ret)
    {
        return -1;
    }
    if ((unsigned)ret < sizeof(*frame))
    {
        if(NULL != fd_name)
        {
            CILog_SigMsg("%s_recv_len_error:%d",fd_name,ret);
        }
        /*CI_HexDump(frame,sizeof(*frame));*/
        return -1;
    }
    /*
     * 由于串口发送数据和接收数据的特性，我们需要找到帧头再拷贝数据，否则可能会出现拷贝
     * 数据错乱的问题
     */
    for (i = 0; i < UART_BUFFER_SIZE - sizeof(*frame);i++)
    {
        if (recv_buf[i] == 0x6a && recv_buf[i + 1] == 0x6a)
        {
            memcpy(frame,&recv_buf[i],sizeof(*frame));
            break;
        }
    }

    if (CI_FALSE == switch_frame_verify(frame))
    {
        /*CI_HexDump(frame,sizeof(*frame));*/
        return -1;
    }
    if (frame->sn == last_recv_sn)
    {
        if(NULL != fd_name)
        {
            CILog_SigMsg("%s_sn is last:%#x",fd_name,frame->sn);
        }
        /*CI_HexDump(frame,sizeof(*frame));*/
        return -1;
    }

    return 0;
}
/*
 功能描述    : 清除切换板接收缓存当中的数据
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年9月23日 13:15:57
*/
int32_t CISwitch_CleanCache(void)
{
    ioctl(switch_fd_a,UART_IOC_CLEAN_MEMORY);
    ioctl(switch_fd_b,UART_IOC_CLEAN_MEMORY);

    return 0;
}
/*
 功能描述    : 从切换板接收数据
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年9月24日 15:14:25
*/
static int32_t switch_recv(void)
{
    int32_t ret = 0;
    static SwitchBoardFrame frame,dummy_frame;
    static int failure_dual_master_count = 0;

    memset(&frame,0,sizeof(frame));
    b_peer_alive = CI_FALSE;

    ret = switch_recv_frame(switch_fd_a,&frame,"switch_a");
    if (-1 == ret)
    {
        /*CILog_SigMsg("切换板从A口接收数据失败，正在从B口接收");*/
        ret = switch_recv_frame(switch_fd_b,&frame,"switch_b");
        if (-1 == ret)
        {
            /*CILog_SigMsg("切换板从B口接收数据失败");*/
            return -1;
        }
    }
    else
    {
        /*清除B口数据*/
        switch_recv_frame(switch_fd_b,&dummy_frame,NULL);
    }

    if (SERIES_MASTER == frame.series_state
        || SERIES_STANDBY == frame.series_state
        || SERIES_CHECK == frame.series_state)
    {
        b_peer_alive = CI_TRUE;
    }

    /*如果另外一系的状态与本系的状态相同,则因该做故障处理*/
    if(CISeries_GetLocalState() == frame.series_state)
    {
        if(SERIES_MASTER == CISeries_GetLocalState())
        {
            failure_dual_master_count ++;
            if (failure_dual_master_count > 8)
            {
                if(CI_TRUE == CISeriesHeartbeatInt_IsLost())
                {
                    CIFailureInt_SetInfo(FAILURE_DUAL_MASTER_SERIES,"切换装置发现发现双主系错误");
                    /*当发现有可能出现双主系的情况时停止看门狗，避免再次重起后出现双主系*/
                    CIWatchdog_Stop();
                }
                else
                {
                    CIFailureInt_SetInfo(FAILURE_SWITCH_BOARD_COMMUNICATE,"切换装置线路通道错误");
                }
            }
        }
        else if (SERIES_STANDBY == CISeries_GetLocalState())
        {
            if(CI_TRUE == CISeriesHeartbeatInt_IsLost())
            {
                CIFailureInt_SetInfo(FAILURE_DUAL_STANDBY_SERIES,"切换装置发现双备系错误");
            }
            else
            {
                CIFailureInt_SetInfo(FAILURE_SWITCH_BOARD_COMMUNICATE,"切换装置线路通道错误");
            }
        }
    }
    else
    {
        failure_dual_master_count = 0;
    }

    /*
     *主系设置自己是否切换的状态，备系跟随主系
     * 注意：
     *       主系        备系
     *       ---         ---
     *        |           |
     *        |<--------->|  双系输入数据同步
     *        |           |  此时发生系统中断，并收到双系切换数据
     * switch |           |  此时主系切换至校核，但备系未收到主系的切换命令，停留在备系
     *        |           |  系统出现无主机状态，几个周期后由于两者都收不到主系的同步数据而停机
     *        |           |
     *        |           |
     *       ---         ---
     */
    if (0x4f == frame.switch_cmd && SERIES_MASTER == CISeries_GetLocalState())
    {
        CILog_SigMsg("收到切换命令,正在生成切换命令,联锁机将在下个周期自动切换");
        CISwitch_ProduceSwitchCmd();
    }
    /*
    CILog_SigMsg("switch_recv_success:%#x",frame.sn);
    */

    CISwitch_CleanCache();

    last_recv_sn = frame.sn;

    return 0;
}

/*
 功能描述    : 强制进行切换
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年9月24日 15:23:24
*/
int32_t CISwitch_LetSwitch(void)
{
    b_switch = CI_TRUE;

    return 0;
}
/*
 功能描述    : 清楚切换标志
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年9月24日 15:23:24
*/
int32_t CISwitch_SwitchOver()
{
    b_switch = CI_FALSE;

    return 0;
}
/*
 功能描述    : 判断切换
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年9月22日 10:58:06
*/
CI_BOOL CISwitch_CouldSwitch(void)
{
    if (40 > CITimer_GetSystemStartCounter())
    {
        return CI_FALSE;
    }
    else
    {
        return b_switch;
    }
}
/*
 功能描述    : 产生一个切换命令
 返回值      : 无
 参数        : 无
 日期        : 2015年7月17日 21:00:43
*/
CI_BOOL CISwitch_ProduceSwitchCmd(void)
{
    b_recved_switch_cmd = CI_TRUE;

    return b_recved_switch_cmd;
}
/*
 功能描述    : 返回切换状态，这是一个原子函数，确保切换命令不会被污染
 返回值      : 有则返回CI_TRUE，否则返回CI_FALSE
 参数        : 无
 日期        : 2015年7月17日 21:00:43
*/
CI_BOOL CISwitch_ConsumeSwitchCmd(void)
{
    CI_BOOL t = b_recved_switch_cmd;

    b_recved_switch_cmd = CI_FALSE;

    return t;
}
/*
 功能描述    : 用以判断主系是否存活
 返回值      : 若主系存活则返回真，否则为假
 参数        : 无
 日期        : 2015年5月29日 20:01:06
*/
CI_BOOL CISwitch_PeerAlive(void)
{
    return b_peer_alive;
}
/*
 功能描述    : 该函数预留给双系启动阶段判断另外一系是否已经启动。
              当双系通信道断开，备系启动时因为备系收不到心跳信号会进入主系，此时
              通过该函数确认一次
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年6月3日 15:40:32
*/
CI_BOOL CISwitch_TestMasterAlive(void)
{
    int i = 0;

    /*尝试接收10次，若根据此判断出对方存活*/
    for (i = 0;i < 10;i++)
    {
        switch_recv();
        CISwitch_CleanCache();
        /*与切换板的通信是每隔100ms进行一次*/
        usleep(100000);
    }

    b_switch = CI_FALSE;

    return CISwitch_PeerAlive();
}
/*
 功能描述    : 切换板定时操作
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年6月1日 13:52:37
*/
static void switch_action(void)
{
    /*主CPU发送双系状态*/
    if (CPU_STATE_MASTER == CICpu_GetLocalState())
    {
        switch_recv();
        switch_send_series_state();
    }
    return ;
}
/*
功能描述    : 定时中断条件检查函数
返回值      : 成功为CI_TRUE，失败为CI_FALSE
参数        : @elapsed_cycle 流逝的定时中断号
作者        : 张彦升
日期        : 2014年8月8日 14:18:01
*/
static CI_BOOL switch_condition_check(uint32_t elapsed_cycle)
{
    if (CI_FALSE == switch_timer.b_open)
    {
        return CI_FALSE;
    }
    if (0 != elapsed_cycle % switch_timer.relative_cycle)
    {
        return CI_FALSE;
    }

    return CI_TRUE;
}
/*
 功能描述    : 切换板初始化
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年9月22日 10:58:36
*/
int32_t CISwitchBoard_Init(void)
{
    static CI_BOOL b_initialized = CI_FALSE;
    int32_t ret = 0;

    if (CI_TRUE == b_initialized)
    {
        return -1;
    }

    switch_fd_a = open("/dev/uart1a",O_RDWR);
    if (-1 == switch_fd_a)
    {
        CILog_Errno("open /dev/uart1a failed");

        return -1;
    }
    switch_fd_b = open("/dev/uart1b",O_RDWR);
    if (-1 == switch_fd_b)
    {
        CILog_Errno("open /dev/uart1b failed");

        return -1;
    }

    ret = CITimer_Regist(&switch_timer);                   /*LED点灯*/

    CISwitch_CleanCache();
    b_initialized = CI_TRUE;

    return 0;
}

#endif /*!LINUX_ENVRIONMENT*/
