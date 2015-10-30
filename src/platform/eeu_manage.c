/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年12月3日 14:46:38
用途        : 电子单元通信
历史修改记录: v1.0    创建
    v1.1    张彦升   添加检查SJA1000和FPGA内存逻辑
**********************************************************************/
#include "util/ci_header.h"

#ifdef LINUX_ENVRIONMENT

#include <fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>

#include "timer.h"

#include "util/ci_header.h"
#include "util/algorithms.h"
#include "util/utility.h"
#include "util/log.h"

#include "failures_int.h"
#include "cpu_manage.h"
#include "eeu_manage.h"
#include "communicate_state_int.h"
#include "performance.h"
#include "remote_log.h"

#include "interlocking/inter_api.h"

#define EEU_FRAME_LENGTH    13
#define EEU_GA_NUM          31
#define EEU_EA_NUM          16
#define EEU_SEND_BUFF_SIZE (15 * EEU_FRAME_LENGTH)

#define EEU_LOG_BUFF_SIZE (4096 * 4)

#define CAN_IOC_MAGIC       'e'
#define CAN_IOCRESET        _IO(CAN_IOC_MAGIC, 0)
#define CAN_IOC_SET_DATAMASK _IOW(CAN_IOC_MAGIC, 1, int*)
#define CAN_IOC_SET_DELAY_COUNTER _IOW(CAN_IOC_MAGIC, 2, int)
#define CAN_IOC_SET_READ_ADDRESS _IOW(CAN_IOC_MAGIC, 3, int)
#define CAN_IOC_GET_SJA_STATUS_FLAG _IOW(CAN_IOC_MAGIC, 4, int)
#define CAN_IOC_MAXNR 4

/*电子单元地址是否使用标志，如果该地址被使用则会在初始化的时候将其标志为1*/
static uint16_t eeu_address_mask[EEU_GA_NUM + 1] = {0};
/*电子单元请求和接收所使用的帧序号*/
static uint16_t eeu_fsn = 0;
/*请求帧的fsn序号*/
static uint16_t last_request_fsn = 0;
static uint16_t this_request_fsn = -1;

/*与电子模块通信文件句柄*/
static int32_t eeu_fd_a= -1;
static int32_t eeu_fd_b= -1;

/*
 *注意这里的缓冲区溢出问题，每一个电子单元出错最大将占用18个字节，共4096字节，代表当有
 *227个模块同时出错的时候，缓冲区将会溢出
 */
static char eeu_recv_log_buf[EEU_LOG_BUFF_SIZE] = {0};
static int32_t eeu_recv_log_buf_len = 0;

static CI_BOOL b_print_eeu = CI_FALSE;  /*是否打印电子单元输出信息*/

/*由于索引号是从1开始的，所以分配的时候多一个，避免索引溢出*/
static int32_t eeu_recv_err_count[EEU_GA_NUM + 1][EEU_EA_NUM + 1] = {{0}};

static CI_BOOL is_channel_a_ok = CI_FALSE;  /*用以判断通信线路是否正常*/
static CI_BOOL is_channel_b_ok = CI_FALSE;  /*用以判断通信线路是否正常*/

/*
 功能描述    : 设置addr指向的地址的16位整数的第nr位为1
 返回值      : 无
 参数        : 
 作者        : 何境泰
 日期        : 2014年5月9日 13:45:55
*/
static void eeu_set_bit(uint16_t nr,unsigned short* addr)
{
    unsigned short mask = 0x0001;
    assert(16 > nr);

    mask <<= (nr);
    *addr |= mask;
    *addr &= 0xffff;
}
/*
 功能描述    : 判断缓冲区是否为空
 返回值      : 如果为空则返回CI_TRUE，否则返回CI_FALSE
 参数        : @buff 要判断的缓冲区
              @size 缓冲区的大小
 作者        : 张彦升
 日期        : 2014年8月19日 10:20:34
*/
static inline CI_BOOL eeu_test_buff_empty(const void* buff,int32_t size)
{
    unsigned char temp = 0;
    unsigned char* ptr = (unsigned char*)buff;
    int32_t i = 0;

    for (i = 0;i < size;i++)
    {
        temp |= ptr[i];
    }

    if (temp)
    {
        return CI_FALSE;
    }

    return CI_TRUE;
}
/*
 功能描述    : 电子模块通信协议当中定义帧序号只能为12位，为了回转需求使用这个函数
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年9月26日 8:47:29
*/
static inline uint16_t eeu_fsn_increase(uint16_t fsn)
{
    return (fsn + 1) % 4096;
}
/*
 功能描述    : 因为接收数据时一定要接收指定帧的数据，所以返回该数据并同步
 返回值      : 无
 参数        : 本次帧号和上次帧号
 作者        : 张彦升
 日期        : 2014年8月20日 15:44:19
*/
void CIEeu_GetRequstFsn(uint16_t* this_fsn,uint16_t* last_fsn)
{
    *this_fsn = this_request_fsn;
    *last_fsn = last_request_fsn;
}
/*
 功能描述    : 调整请求帧的序号
 返回值      : 无
 参数        : 要调整成的帧序号
 作者        : 张彦升
 日期        : 2014年8月20日 15:45:45
*/
void CIEeu_AdjustRequstFsn(uint16_t this_fsn,uint16_t last_fsn)
{
    this_request_fsn = this_fsn;
    last_request_fsn = last_fsn;
    eeu_fsn = eeu_fsn_increase(this_fsn);

    return;
}
/*
 功能描述    : 为电子单元添加数据有效标志
 返回值      : 
 参数        : @ga 网关地址，从1开始
              @ea 电子模块地址，从1开始
 作者        : 何境泰
 日期        : 2014年5月9日 13:47:15
*/
void CIEeu_SetDataMask(int16_t ga, int16_t ea)
{
    assert(0 < ga && 31 > ga);
    assert(0 < ea && 16 > ea);

    if ((ga >= TOTAL_GETWAY) || (ea >= MAX_EEU_PER_LAYER))
    {
        CILog_Msg("Error EEU ADDRESS");

        return;
    }

    eeu_set_bit(ea - 1,&eeu_address_mask[ga - 1]);

    return;
}
/*
 功能描述:  设备状态判断
 返回值    :    
 参数    :      
 作者    :  何境泰
 日期    :  2014年3月21日 9:32:45
*/
int32_t CIEeu_StatusJudge(void)
{
    int16_t ga = 0,ea = 0;

    /*系统刚系统时前5个周期可能收不上来数据，所以不进行故障判断*/
    if (50 > CITimer_GetSystemStartCounter())
    {
        return -1;
    }
    for (ga = 0;ga < EEU_GA_NUM;ga++)
    {
        for (ea = 0;ea < EEU_EA_NUM;ea++)
        {
            if (eeu_recv_err_count[ga][ea] >=3 )
            {
                /*CILog_Msg("status_judge:%d-%d fail",ga,ea);*/
                CIInter_DefaultSafeProcess(ga,ea);
            }
        }
    }

    return 0;
}
/*
 功能描述    : 向电子单元发送请求命令帧
 返回值      : 无
              @command             命令数据
              @et                  电子单元类型
              @ga                  网关地址
              @dt                  数据类型
              @ea                  电子单元地址
 作者       : 何境泰
 日期       : 2013年12月13日 13:22:38A
*/
static void eeu_assemble_send_frame(EeuFrame* frame,
                                    uint8_t et,
                                    int16_t ga,
                                    int16_t ea,
                                    uint8_t dt,
                                    uint32_t command)
{
    uint32_t temp_id = 0;

    frame->head = 0X86;

    /*将预留位清空，避免计算出错误的crc码*/
    frame->id.tag.preserve = 0;
    frame->id.tag.dir = 0;        /*由联锁机向电子单元的数据类型为0*/
    frame->id.tag.fsn = eeu_fsn;
    frame->id.tag.et = et;
    frame->id.tag.ga = ga;
    frame->id.tag.ea = ea;
    frame->id.tag.dt = dt;

    frame->state = command;

    temp_id = frame->id.frame_id;
    frame->id.frame_id = CI_Swap32(temp_id);

    /*计算crc暂时不包含头部，注意，这里计算crc时id域为未偏移之前大小端转换后的数据*/
    frame->crc = CIAlgorithm_Crc16(&frame->id.tag, 8);

    /*进行fpga的偏移和大小端变化，ID域向左偏移3位,以后如果修改fpga发送偏移问题,可以去掉*/
    temp_id=((temp_id << 3) & 0xfffffff8);
    frame->id.frame_id = CI_Swap32(temp_id);
    frame->preserve = 0;

    return;
}
/*
功能描述    : 发送多帧数据
返回值      : 
参数        : @send_buff    发送数据缓存区
             @size         发送数据大小(最大不超过15* 13)
作者        : 何境泰
日期        : 2014年5月4日 9:03:37
*/
static int32_t eeu_send_data(const void* send_data, int32_t size)
{
    int32_t ret = 0;

    if (EEU_SEND_BUFF_SIZE < size)
    {
        if (CI_TRUE == b_print_eeu)
        {
            CILog_Msg("发送数据大小超过限制");
        }
        return -1;
    }
    /*发送电子模块数据*/
    ret = write(eeu_fd_a, send_data, size);
    if (0 >= ret)
    {
        if (CI_TRUE == b_print_eeu)
        {
            CILog_Errno("eeu_send_fail_a");
        }
        ret = write(eeu_fd_b, send_data, size);
        if (0 >= ret)
        {
            if (CI_TRUE == b_print_eeu)
            {
                CILog_Errno("eeu_send_fail_b");
            }
            return -1;
        }
    }
    return 0;
}
/*
 功能描述    : 向电子单元发送请求命令帧
 返回值      : 无
 作者        : 何境泰
 日期        : 2014年3月3日 11:23:11
*/
void CIEeu_RequestStatus(void)
{
    /*
     * 请求命令状态模型,电子单元通信正常时:
     * 正常通信时始终使用上上个周期的帧序号进行验证,因为网关存在一个周期的延迟
     *       主系        备系
     *       ---         ---
     *        |           |
     *recieve |<- 0x0e  ->|
     *        |           |
     *        |           |
     *request |-> 0x10    |
     *        |           |
     *        |           |
     *       ---         ---
     *        |           |
     *recieve |<- 0x0f  ->|
     *        |           |
     *        |           |
     *request |-> 0x11    |
     *        |           |
     *        |           |
     *       ---         ---
     *
     * 主系电子单元通信断开，备系电子单元通信道正常时(1):
     *       主系        备系
     *       ---         ---
     *        |           |
     *recieve |<- 0x0e  ->|
     *        |           |
     *        |           |
     *request |-> 0x10    |
     *broke   X           |
     *        |           |
     *       ---         ---
     *        |           |
     *recieve X<- 0x0f  ->|     主系接受失败,备系接受成功
     *switch  |           |     设置切换标志，将在下个周期切换
     *        |           |
     *request |   0x11  <-|     备系进行请求
     *        |           |
     *        |           |
     *       ---         ---
     *        |           |
     *recieve X<- 0x10  ->|     主系接受失败,备系接受成功
     *switch  |<--------->|     切换标志为真，发生双系切换
     *        |           |
     *request |   0x12  <-|     已经变为主系，系统正常请求，没有时间延迟，一切正常
     *        |           |
     *        |           |
     *       ---         ---
     *
     * 主系电子单元通信断开，备系电子单元通信道正常时(2):
     *       主系        备系
     *       ---         ---
     *        |           |
     *recieve |<- 0x0e  ->|
     *        |           |
     *broke   X           |
     *request |-> 0x10    |     由于在主系请求之前通道断开，所以请求未发下去
     *        |           |
     *        |           |
     *       ---         ---
     *        |           |
     *recieve X<- 0x0f  ->X     由于上次请求数据未发下去，所以主备系都未收到数据
     *        |           |
     *        |           |
     *request |   0x10  <-|     在这种情况下，主系不进行请求，备系使用上个周期的帧序号进行请求
     *        |           |
     *        |           |
     *       ---         ---
     *        |           |
     *recieve X<- 0x0f  ->|     此时主系未收到数据，备系正常收到数据
     *        |           |     此时主系察觉备系健康于自身，设置切换标志，将会在下个周期进行切换
     *        |           |
     *request |   0x11  <-|     备系继续请求
     *        |           |
     *        |           |
     *       ---         ---
     *        |           |
     *recieve X<- 0x10  ->|
     *switch  |<--------->|     主系察觉备系健康于自身，发生切换
     *        |           |
     *request |   0x12  <-|     系统切换成功，系统正常运行，在此期间有两个周期数据丢失
     *        |           |
     *        |           |
     *       ---         ---
     * 主系电子单元通信断开，备系电子单元通信道也断开:
     *       主系        备系
     *       ---         ---
     *        |           |
     *recieve |<- 0x0e  ->|
     *        |           |
     *        |           |
     *request |-> 0x10    |     因为两系状态都断开，所以不存在健康系数的问题，也不存在切换
     *        |           |
     *        |           |
     *       ---         ---
     *
     * 电子单元请求模型，双系通信道（光纤）断开：
     * 双系通信道断开，将会发生切换，切换过程当中帧序号可能会存在错乱
     * 断开情况1，在上一个周期双系通信断开，并在下一个周期fsm_switch的时候检测到并提升为主系
     *       主系        备系
     *       ---         ---
     *        |           |
     *recieve |<- 0x0e  ->|
     *        |           |
     *        |           |
     *request |-> 0x10    |     备系只递增请求帧序号，做假请求last(0x0f)this(0x10)
     *        |           |
     *        X           |     光纤通信道断开
     *                   ---
     *                    |
     *            0x0f  ->|     备系接收last(0x0f)成功
     *                    |
     *               ---->|     备系检测到主系断开，提升为主系
     *            0x11  <-|     切换后请求last(0x10)this(0x11)
     *                    |
     *                    |
     *                   ---
     *
     * 断开情况2，在上一个周期双系通信断开，但在下一个周期fsm_switch的时候未
     *     检测到断开，在这种情况下，只有一个周期会丢失请求状态
     *       主系        备系
     *       ---         ---
     *        |           |
     *recieve |<- 0x0e  ->|
     *        |           |
     *        |           |
     *request |-> 0x10    |     备系只递增请求帧序号，做假请求last(0x0f)this(0x10)
     *        |           |
     *        X           |     光纤通信道断开
     *                   ---
     *                    |
     *            0x0f  ->|     备系接收last(0x0f)成功
     *                    |
     *                    |
     *switch              |     备系在此情况下未检测到，也未提升为主系
     *                  <-|     备系只递增帧序号，做假请求last(0x10)this(0x11)
     *                    |
     *                   ---
     *                    |
     *            0x10  ->|     备系接收last(0x10)成功(原理上应该成功，但是电子单元对真序号跳跃的处理导致这里**失败**)
     *                    |
     *           -------->|     检测到双系通信道断开，提升为主系
     *            0x11  <-|     请求last(0x11)this(0x12)
     *                    |
     *                    |
     *                   ---
     *                    |
     *            0x11  ->|     备系接收last(0x11) **失败**
     *                    |
     *                    |
     *            0x12  <-|     请求last(0x12)this(0x13)
     *                    |
     *                    |
     *                   ---
     *                    |
     *            0x12  ->|     备系接收last(0x12)成功
     *                    |
     *                    |
     *            0x13  <-|     请求last(0x13)this(0x14)
     *                    |
     *                    |
     *                   ---
     *
     * 断开情况3，在上个周期请求前断开，这种情况中间会有两个周期未收到模块数据
     *       主系        备系
     *       ---         ---
     *        |           |
     *recieve |<- 0x0e  ->|
     *        |           |
     *        X           |     光纤通信道断开
     *request             |     备系只递增请求帧序号，做假请求last(0x0f)this(0x10)
     *                    |
     *                    |
     *                   ---
     *                    |
     *            0x0f  ->|     备系接收last(0x0f) **失败**
     *                    |
     *                    |
     *switch        ----->|     备系在此情况下肯定能检测到主系停机，自己提升为主系
     *            0x11  <-|     正常请求last(0x10)this(0x11)
     *                    |
     *                   ---
     *                    |
     *            0x10  ->|     备系接收last(0x10) **失败**
     *                    |
     *                    |
     *                    |
     *            0x12  <-|     正常请求last(0x11)this(0x12)
     *                    |
     *                   ---
     *                    |
     *            0x11  ->|     备系接收last(0x11)成功
     *                    |
     *                    |
     *                    |
     *            0x13  <-|     正常请求last(0x12)this(0x13)
     *                    |
     *                   ---
     */
    static EeuFrame eeu_frame;
    static CI_BOOL b_first_detect_eeu_broke = CI_TRUE;

    /*当主系断开时,这是需要备系进行请求,并且请求的帧与主系上次请求的相同*/
    if(CI_TRUE == CISeries_IsPeerEeuBroke()
        && SERIES_STANDBY == CISeries_GetLocalState()
        && CI_FALSE == CIEeu_IsChannelAOk() && CI_FALSE == CIEeu_IsChannelBOk()
        && CI_TRUE == b_first_detect_eeu_broke
        )
    {
        eeu_fsn = this_request_fsn;
        /*只有在备系充当主系时的第一次才会重复的发送一次*/
        b_first_detect_eeu_broke = CI_FALSE;
    }
    else
    {
        /*请求的时候保证上次帧序号被正确设置*/
        last_request_fsn = this_request_fsn;
        this_request_fsn = eeu_fsn;
        b_first_detect_eeu_broke = CI_TRUE;
    }

    if (CI_TRUE == b_print_eeu)
    {
        CIRemoteLog_Write("eeu_request_status:last(%#x)this(%#x)",last_request_fsn,this_request_fsn);
    }

    /*请求帧的命令为0x11111111,请求帧数据类型为1*/
    eeu_assemble_send_frame(&eeu_frame,0,0,0,1,0x11111111);

    /*帧序号按照4096来回转的*/
    eeu_fsn = eeu_fsn_increase(eeu_fsn);

    eeu_send_data(&eeu_frame,sizeof(eeu_frame));

    return;
}
/*
 功能描述    : 只递增帧序号，而不请求
 返回值      : 无
 作者        : 何境泰
 日期        : 2014年3月3日 11:23:11
*/
void CIEeu_RequestIncFsn(void)
{
    last_request_fsn = this_request_fsn;
    this_request_fsn = eeu_fsn;
    eeu_fsn = eeu_fsn_increase(eeu_fsn);
}
/*
 功能描述    : 发送所有节点命令
              电子单元发送数据可以多帧进行发送，电子单元通信板中提供的发送缓存大小
              为0x7f，最多一次可以发送15帧数据，但是由于电子单元接收速率的原因，
              多次发送过程当中电子单元通信板中的FPGA发送速率很快，可能导致模块收
              不到某一帧的情况，因此在这里应选择速度较慢的逐帧发送，并在发送函数里
              给每次发送做延迟，这样从一定程度上能确保模块正确收到数据
 返回值      : 无
 参数        : 无
 作者        : 何境泰
 日期        : 2014年4月16日 13:37:53
*/
void CIEeu_SendAllCommand(void)
{
    int16_t ga = 0,ea = 0;
    static EeuFrame frame;  /*initialize to 0 relying on static behavior*/
    static uint32_t last_commands[MAX_GETWAYS][MAX_EEU_PER_LAYER];    /*上次发送的控制命令数据*/
    static char commands_log_buf[EEU_LOG_BUFF_SIZE] = {0};
    static int32_t commands_log_buf_len = 0;
    static struct timespec ts = {0,3000000}; /*3ms*/

    for (ga = 0; ga < TOTAL_GETWAY; ga++)
    {
        for (ea = 0; ea < EEU_EA_NUM; ea++)
        {
            if (0 != commands[ga][ea])
            {
                if (commands[ga][ea] != last_commands[ga][ea])
                {
                    commands_log_buf_len += snprintf(commands_log_buf + commands_log_buf_len,
                            EEU_LOG_BUFF_SIZE - commands_log_buf_len,
                            "%d-%d:%08x,",ga,ea,commands[ga][ea]);
                }
                eeu_assemble_send_frame(&frame,0, ga, ea, 0,commands[ga][ea]);
                /*帧序号按照4096来回转的*/
                eeu_fsn = eeu_fsn_increase(eeu_fsn);
        
                /*延迟3ms的时间,nanosleep是系统调用，其实现并非SIGALARM，较为安全*/
                nanosleep(&ts ,NULL);

                eeu_send_data(&frame,sizeof(frame));
            }
        }
    }

    if (CI_TRUE == b_print_eeu && commands_log_buf_len != 0)
    {
        CIRemoteLog_Write("eeu_send_commands:%s",commands_log_buf);
    }
    /*命令数据保存下，只记录改变的命令数据*/
    memcpy(last_commands,commands,TOTAL_GETWAY * EEU_EA_NUM * sizeof(uint32_t));
    memset(commands_log_buf,0,EEU_LOG_BUFF_SIZE);
    commands_log_buf_len = 0;
}
/*
 功能描述    : 检验电子单元数据帧是否传输错误
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年8月19日 13:25:28
*/
static CI_BOOL eeu_frame_verify(const EeuFrame* frame)
{
    uint16_t crc = CIAlgorithm_Crc16(&frame->id.tag, 8);
    if (frame->crc != crc)
    {
        eeu_recv_log_buf_len += snprintf(eeu_recv_log_buf + eeu_recv_log_buf_len,
                            EEU_LOG_BUFF_SIZE - eeu_recv_log_buf_len,
                            "C%x_%x_%x",frame->crc,crc,frame->id.frame_id);

        return CI_FALSE;
    }

    /*检查数据长度*/
    if (0x86 != frame->head)
    {
        eeu_recv_log_buf_len += snprintf(eeu_recv_log_buf + eeu_recv_log_buf_len,
                            EEU_LOG_BUFF_SIZE - eeu_recv_log_buf_len,
                            "H%x",frame->head);
        return CI_FALSE;
    }

    return CI_TRUE;
}
/*
 功能描述    : 电子单元从A口接收数据
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年8月19日 13:22:18
*/
static int32_t eeu_recv_data_ext(int32_t fd,EeuFrame* frame,const char* UNUSED(fd_name))
{
    int32_t ret = 0;

    ret = read(fd,frame,sizeof(*frame));
    if (0 >= ret)
    {
        if (CI_TRUE == b_print_eeu)
        {
            eeu_recv_log_buf_len += snprintf(eeu_recv_log_buf + eeu_recv_log_buf_len,
                            EEU_LOG_BUFF_SIZE - eeu_recv_log_buf_len,
                            "F");
        }
        return -1;
    }
    else
    {
        /*检查线路传输是否错误*/
        if (CI_FALSE == eeu_frame_verify(frame))
        {
            return -1;
        }
        /*校正id*/
        frame->id.frame_id = CI_Swap32(frame->id.frame_id);
        /*检查发送方向*/
        if (frame->id.tag.dir != 1)
        {
            eeu_recv_log_buf_len += snprintf(eeu_recv_log_buf + eeu_recv_log_buf_len,
                            EEU_LOG_BUFF_SIZE - eeu_recv_log_buf_len,
                            "D%x",frame->id.tag.dir);
            return -1;
        }

        /*检查电子单元类型*/
        if (frame->id.tag.et > 7)
        {
            eeu_recv_log_buf_len += snprintf(eeu_recv_log_buf + eeu_recv_log_buf_len,
                            EEU_LOG_BUFF_SIZE - eeu_recv_log_buf_len,
                            "E%x",frame->id.tag.et);
            return -1;
        }

        /*检查网关地址*/
        if (frame->id.tag.ga > EEU_GA_NUM)
        {
            eeu_recv_log_buf_len += snprintf(eeu_recv_log_buf + eeu_recv_log_buf_len,
                            EEU_LOG_BUFF_SIZE - eeu_recv_log_buf_len,
                            "G%x",frame->id.tag.ga);
            return -1;
        }
        
        /*检查是否丢帧*/
        if (frame->id.tag.fsn != last_request_fsn)
        {
            if (CI_TRUE == b_print_eeu)
            {
                eeu_recv_log_buf_len += snprintf(eeu_recv_log_buf + eeu_recv_log_buf_len,
                            EEU_LOG_BUFF_SIZE - eeu_recv_log_buf_len,
                            "S%x_%x",frame->id.tag.fsn,last_request_fsn);
            }

            return -1;
        }
    }

    return 0;
}
/*
 功能描述    : 检查sja1000状态
 返回值      : 如果SJA1000无错误，则返回CI_TRUE,否则返回CI_FALSE
 参数        : 无
 日期        : 2015年7月1日 10:13:59
*/
static CI_BOOL eeu_check_sja_status(void)
{
    uint8_t status = 0;

    if (0 == ioctl(eeu_fd_a,CAN_IOC_GET_SJA_STATUS_FLAG,&status))
    {
        /*
         * SJA1000英文文档6.3.5节,Table 5,Page 15指出该状态的含义，我们在这里
         * 只要发现第7位和第6位为1，即说明SJA1000已经处于故障状态
         */
        if (status & 0xc0)
        {
            return CI_FALSE;
        }
    }

    return CI_TRUE;
}
/*
 功能描述    : 获得电子单元当前状态 
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 何境泰
 日期        : 2014年4月2日 15:55:29
*/
int32_t CIEeu_ReceiveStatusData(void)
{
    register int32_t ret = 0;
    register int16_t ga = 0,ea = 0;
    register int32_t recv_count = 0;
    register int32_t recv_len_read_before = 0;
    static uint32_t last_state[EEU_GA_NUM][EEU_EA_NUM] = {{0}};

    /*由于该函数会频繁被调用，所以，将其内存设置为静态的*/
    static char log_commands_buf[EEU_LOG_BUFF_SIZE] = {0};
    register int32_t log_commands_buf_len = 0;
    static EeuFrame frame; /*initialize to 0 relying on compiler behavior */
    static EeuFrame frame_dummy;

    CIPerformance_Send(PDT_EEU_R_BEGIN);

    is_channel_a_ok = CI_FALSE;
    is_channel_b_ok = CI_FALSE;

    /*对每个网关的每个模块分别读取*/
    for (ga = 1;ga < EEU_GA_NUM;ga++)
    {
        for (ea = 1;ea < EEU_EA_NUM;ea++)
        {
            if(CI_TRUE == CIEeu_IsAddressUsing(ga,ea))
            {
                recv_len_read_before = eeu_recv_log_buf_len;

                if (CI_TRUE == b_print_eeu)
                {
                    eeu_recv_log_buf_len += snprintf(eeu_recv_log_buf + eeu_recv_log_buf_len,
                        EEU_LOG_BUFF_SIZE - eeu_recv_log_buf_len,
                        "%d-%d",ga,ea);
                }

                /*
                 * 设置从电子单元读取数据的地址，高16位为网关地址，低16位为电子单元地址，
                 * 注意驱动当中是从0开始的，而实际当中是从1开始的
                 */
                ret = ioctl(eeu_fd_a,CAN_IOC_SET_READ_ADDRESS,(((ga -1) << 16) & 0xffff0000) | ((ea -1) & 0x0000ffff));
                if (-1 == ret)
                {
                    if (CI_TRUE == b_print_eeu)
                    {
                        eeu_recv_log_buf_len += snprintf(eeu_recv_log_buf + eeu_recv_log_buf_len,
                            EEU_LOG_BUFF_SIZE - eeu_recv_log_buf_len,
                            "ioctl fail,");
                    }
                }
                else
                {
                    /*分别从A口和B口尝试读取数据*/
                    ret = eeu_recv_data_ext(eeu_fd_a,&frame,"a");
                    if (0 > ret)
                    {
                        ret = eeu_recv_data_ext(eeu_fd_b,&frame,"b");
                        if (0 == ret)
                        {
                            /*从B口能正确收取数据*/
                            is_channel_b_ok = CI_TRUE;
                        }
                    }
                    else
                    {
                        /*能收上来数据，则证明A路是正常的*/
                        is_channel_a_ok = CI_TRUE;
                        /*从B口接收一次数据确保缓存被清除掉，另外以检查通信道是否正常*/
                        if(sizeof(frame_dummy) == read(eeu_fd_b,&frame_dummy,sizeof(frame_dummy)))
                        {
                            is_channel_b_ok = CI_TRUE;
                        }
                    }
                    if (0 > ret)
                    {
                        eeu_recv_err_count[ga][ea] ++;
                        /*当出错的时候将其记录下来*/
                        eeu_recv_log_buf_len += snprintf(eeu_recv_log_buf + eeu_recv_log_buf_len,
                            EEU_LOG_BUFF_SIZE - eeu_recv_log_buf_len,
                            "%08x",frame.state);

                        frame.state = 0;
                    }
                    else
                    {
                        /*如果接收正确，并发现本次状态与上次不一样则记录*/
                        if (last_state[ga][ea] != frame.state)
                        {
                            log_commands_buf_len += snprintf(log_commands_buf + log_commands_buf_len,
                                EEU_LOG_BUFF_SIZE - log_commands_buf_len,
                                "%d-%d:%08x,",ga,ea,frame.state);
                        }
                        last_state[ga][ea] = frame.state;

                        CIInter_InputNodeState(ga, ea, frame.state);
                        /*重置读取日志长度*/
                        eeu_recv_log_buf_len = recv_len_read_before;
                        eeu_recv_log_buf[eeu_recv_log_buf_len] = 0;
                        eeu_recv_err_count[ga][ea] = 0;
                        recv_count++;
                    }
                }
            }
        }
    }
    if (CI_TRUE == b_print_eeu && 0 != eeu_recv_log_buf_len)
    {
        CIRemoteLog_Write("eeu_recv_count:%d,%s",recv_count,eeu_recv_log_buf);
    }
    if (CI_TRUE == b_print_eeu && 0 != log_commands_buf_len)
    {
        CIRemoteLog_Write("eeu_recv_commands:%s",log_commands_buf);
    }

    /*如果通信道道断开则点亮故障灯*/
    if (CI_FALSE == is_channel_a_ok)
    {
        CIFailureInt_SetInfo(FAILURE_EEU_CHANNEL_HALT,"电子单元A路通信道断开");
        /*当发现SJA1000处于故障状态时复位*/
        if (CI_FALSE == eeu_check_sja_status())
        {
            CILog_Msg("sja status fail a");
            //ioctl(eeu_fd_a, CAN_IOCRESET);
        }
    }
    if (CI_FALSE == is_channel_b_ok)
    {
        CIFailureInt_SetInfo(FAILURE_EEU_CHANNEL_HALT,"电子单元B路通信道断开");

        if (CI_FALSE == eeu_check_sja_status())
        {
            CILog_Msg("sja status fail b");
            //ioctl(eeu_fd_a, CAN_IOCRESET);
        }
    }

    /*REMAIN 当两路都断开时，此时应该拒绝切换？*/
    memset(eeu_recv_log_buf,0,EEU_LOG_BUFF_SIZE);
    eeu_recv_log_buf_len = 0;

    memset(log_commands_buf,0,EEU_LOG_BUFF_SIZE);
    log_commands_buf_len = 0;

    CIPerformance_Send(PDT_EEU_R_END);

    return 0;
}
/*
 功能描述    : 打开输出电子单元信息
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年4月11日 15:51:44
*/
int32_t CIEeu_OpenPrintEeu(void)
{
    b_print_eeu = CI_TRUE;
    return 0;
}

/*
 功能描述    : 初始化与电子模块交互的文件fd
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 何境泰
 日期        : 2014年3月3日 13:57:18
*/
static int32_t eeu_fd_init(void)
{
    int32_t ret = 0;

    eeu_fd_a = open("/dev/cana",O_RDWR);
    if (-1 == eeu_fd_a)
    {
        CILog_Errno("open /dev/cana failed");

        return -1;
    }

    eeu_fd_b = open("/dev/canb",O_RDONLY);

    if (-1 == eeu_fd_b)
    {
        CILog_Errno("open /dev/canb failed");

        return -1;
    }

    if (CI_TRUE == eeu_test_buff_empty(eeu_address_mask,sizeof(eeu_address_mask)))
    {
        CILog_Msg("eeu_address_mask is null");
        return -1;
    }

    /*设置数据有效标志*/
    ret = ioctl(eeu_fd_a,CAN_IOC_SET_DATAMASK, eeu_address_mask);
    if (-1 == ret)
    {
        CILog_Errno("ioctl eeu_fd CAN_IOC_SET_DATAMASK failed");
        return -1;
    }

    return 0;
}
/*
 功能描述    : 得到本站场使用的网关的个数
 返回值      : 根据配置文件而得到的站场网关的个数
 参数        : 无
 日期        : 2015年4月4日 9:07:49
*/
int32_t CIEeu_UsingGaNum(void)
{
    int16_t ga = 0;
    int32_t count = 0;

    for (ga = 0;ga < EEU_GA_NUM;ga++)
    {
        if (eeu_address_mask[ga])
        {
            count += 1;
        }
    }
    
    return count;
}
/*
 功能描述    : 得到本站场使用的模块的个数
 返回值      : 根据配置文件而得到的使用网关的个数
 参数        : 无
 日期        : 2015年4月4日 9:33:26
*/
int32_t CIEeu_UsingEaNum(void)
{
    int16_t ga = 0,ea = 0,count = 0;

    for (ga = 0;ga < EEU_GA_NUM;ga++)
    {
        for (ea = 0;ea < EEU_EA_NUM;ea++)
        {
            if (CI_TRUE == CIEeu_IsAddressUsing(ga,ea))
            {
                count ++;
            }
        }
    }
    
    return count;
}
/*
 功能描述    : 判断模块是否使用，
    在联锁程序启动的时候会读取配置文件，并调用CIEeu_SetDataMask设置模块地址是否
    使用，该函数用以判断电子单元模块是否被使用
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年6月24日 9:20:28
*/
CI_BOOL CIEeu_IsAddressUsing(int16_t ga, int16_t ea)
{
    assert(16 > ea);
    assert(31 > ga);

    /*这里对0号特殊处理,因为联锁程序是从1开始的,而驱动当中是从1开始的*/
    if(0 == ga || 0 == ea)
    {
        return CI_FALSE;
    }

    if (eeu_address_mask[ga - 1] & (1 << (ea - 1)))
    {
        return CI_TRUE;
    }
    return CI_FALSE;
}
/*
 功能描述    : 得到由网关向上传输数据的时间
 返回值      : 时间
 参数        : 无
 日期        : 2015年4月4日 9:48:14
*/
double CIEeu_TransTime(void)
{
    /*
     * 接收一个网关的数据需要5ms,如使用了20个网关则至少为电子单元预留
     * 100ms传输时间。
     */
    //return CIEeu_UsingGaNum() * 5.0;
    return 200;
}
/*
 功能描述    : 从fpga读取电子单元数据所使用的时间
 返回值      : 时间
 参数        : 无
 日期        : 2015年4月4日 9:51:16
*/
static double eeu_fpga_read_time(void)
{
    return CIEeu_UsingEaNum() * 0.2;
}
/*
 功能描述    : 电子单元向下发送命令的时间
 返回值      : 时间
 参数        : 无
 日期        : 2015年4月4日 9:58:46
*/
static double eeu_send_time(void)
{
    /*发送命令不好确定，根据与赵的讨论可能最大为70个命令帧，这里使用40计算*/
    return 40 * 3.0;
}
/*
 功能描述    : 得到电子单元一个周期需要的时间
 返回值      : 时间
 参数        : 无
 日期        : 2015年4月4日 9:44:29
*/
double CIEeu_NeedTime(void)
{
    /*
     * 电子单元花费的时间包括：
     * 1.网关回复数据向上传输的时间，每个网关耗时5ms。
     * 2.联锁从FPGA读取模块数据的时间，每个模块耗时0.2ms。
     * 3.联锁向下发送请求命令帧的时间，由站场大小而决定，每个命令帧耗时3ms。
     * 当下：
     * ga=20，ea=125
     * 计算时间为20 * 5 + 125 * 0.2 + 120 = 245ms
     */
    //return CIEeu_TransTime() + eeu_fpga_read_time() + eeu_send_time();
    return 200;
}

/*
 功能描述    : 得到A通信道的状态
 返回值      : 通信道正常则返回CI_TRUE,否则返回CI_FALSE
 参数        : 无
 日期        : 2015年5月29日 18:30:00
*/
CI_BOOL CIEeu_IsChannelAOk(void)
{
    return is_channel_a_ok;
}
/*
 功能描述    : 得到B通信道的状态
 返回值      : 通信道正常则返回CI_TRUE,否则返回CI_FALSE
 参数        : 无
 日期        : 2015年5月29日 18:30:00
*/
CI_BOOL CIEeu_IsChannelBOk(void)
{
    return is_channel_b_ok;
}
/*
 功能描述    : 测试电子单元通信道是否断开
 返回值      : 通信道正常则返回CI_TRUE,否则返回CI_FALSE
 参数        : 无
 日期        : 2015年5月29日 18:30:00
*/
CI_BOOL CIEeu_TestChannelBroke(void)
{
    int i = 0;

    for (i = 0;i < 10;i++)
    {
        CIEeu_RequestStatus();
        /*模拟一个连锁周期*/
        usleep(CI_CYCLE_MS * 1000);

        CIEeu_ReceiveStatusData();
    }

    if(CI_FALSE == CIEeu_IsChannelAOk() && CI_FALSE == CIEeu_IsChannelBOk())
    {
        return CI_TRUE;
    }
    return CI_FALSE;
}
/*
 * 清除计数标志 
 */
void CIEeu_ClearErrorCount(void)
{
    memset(eeu_recv_err_count,0,(EEU_GA_NUM + 1) * (EEU_EA_NUM + 1) * sizeof(int32_t));
}
/*
功能描述    : 电子单元模块初始化
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2014年3月3日 13:56:04
*/
int32_t CIEeu_Init(void)
{
    static CI_BOOL b_initialized = CI_FALSE;

    if (CI_TRUE == b_initialized)
    {
        return -1;
    }

    if (0 > eeu_fd_init())
    {
        return -1;
    }

    b_initialized = CI_TRUE;

    return 0;
}

#endif /*!LINUX_ENVRIONMENT*/
