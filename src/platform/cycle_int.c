/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年10月15日 9:39:26
用途        : 联锁运算周期管理，这里三个状态自动机对应如下：
             主系状态自动机： 主系主CPU
             备系状态自动机： 备系主CPU
             从CPU状态自动机：主系从CPU和备系从CPU
历史修改记录: v1.0    创建
**********************************************************************/
#include "util/ci_header.h"

#ifdef LINUX_ENVRIONMENT

#include <sys/time.h>
#include <semaphore.h>
#include <math.h>

#include "cycle_int.h"
#include "cpu_manage.h"
#include "series_manage.h"
#include "switch_board_manage.h"
#include "hmi_manage.h"
#include "eeu_manage.h"
#include "timer.h"
#include "failures_int.h"
#include "performance.h"
#include "series_heartbeat_int.h"
#include "remote_log.h"

#include "util/app_config.h"
#include "util/log.h"

#include "interlocking/inter_api.h"

static void cycle_action(void);

static CI_BOOL cycle_condition_check(uint32_t elapsed_cycle);

static const CITimer ci_cycle_timer = 
{
    CI_CYCLE_RELATIVE,
    cycle_action,
    cycle_condition_check,
    "ci_cycle_timer",
    CI_TRUE,                            /*默认打开该定时器*/
};

static void eeu_request_action(void);

static CI_BOOL eeu_request_condition_check(uint32_t elapsed_eeu_request);

static const CITimer eeu_request_timer = 
{
    EEU_REQUEST_CYCLE_RELATIVE,
    eeu_request_action,
    eeu_request_condition_check,
    "eeu_request_timer",
    CI_TRUE,                            /*默认打开该定时器*/
};

typedef struct _Fsm Fsm;
static uint32_t cycle_counter = 0;      /*联锁周期序号*/
static Fsm master_fsm;
static Fsm standby_fsm;
static Fsm slave_fsm;

static Fsm* ci_fsm = NULL;

/*延迟计数，当在一个联锁周期没有运算完成，定时中断到来后进行计数*/
static uint32_t delay_count;
static int32_t standby_count = 0;

static CI_BOOL b_print_fsm = CI_FALSE;  /*打开状态机信息的输出*/

#define cache_align __attribute__((aligned(64)))

static cache_align sem_t cycle_sem;               /*使用该信号量将联锁周期后面的时间片让出来*/
static cache_align sem_t eeu_request_sem;         /*该信号量为请求电子单元数据所用*/

static uint32_t eeu_request_relative_cycle = 0; /*电子单元请求的周期号*/

static CI_BOOL b_first_cycle = CI_TRUE;
static CI_BOOL b_switch_to_master = CI_FALSE;

typedef int32_t (*state_handler) (Fsm* fsm);            /* 状态——消息响应函数 */

struct _Fsm
{
    state_handler fsm_cur_state;                        /*当前状态 */
    state_handler fsm_start_state;                      /*开始状态 */
    state_handler fsm_terminal_state;                   /*终止状态*/

    state_handler fsm_begin_new_cycle;                  /*开始新周期 */
    state_handler fsm_series_new_cycle_syn;             /*双薪新周期同步*/
    state_handler fsm_cpu_new_cycle_syn_step1;          /*双CPU新周期同步步骤1*/
    state_handler fsm_cpu_new_cycle_syn_step2;          /*双CPU新周期同步步骤2*/
    state_handler fsm_communicate;                      /*数据通信 */
    state_handler fsm_device_status_judge;              /*设备状态判断 */
    state_handler fsm_series_input_syn;                 /*双系输入数据同步*/
    state_handler fsm_cpu_input_syn_step1;              /*双CPU输入数据同步步骤1*/
    state_handler fsm_cpu_input_syn_step2;              /*双CPU输入数据同步步骤2*/
    state_handler fsm_authority_manage;                 /*操作权限管理*/
    state_handler fsm_series_switch;                    /*双系切换*/
    state_handler fsm_interlocking_calculate;           /*联锁逻辑运算*/
    state_handler fsm_series_result_syn;                /*双系同步比较运算结果*/
    state_handler fsm_cpu_result_syn_step1;             /*双CPU同步比较运算结果*/
    state_handler fsm_cpu_result_syn_step2;             /*双CPU同步比较运算结果*/
    state_handler fsm_output_result;                    /*运算结果输出*/
    state_handler fsm_eeu_request;                      /*电子单元请求数据状态*/
};

static void fsm_transform(Fsm* fsm,state_handler state);

/*
功能描述 : 打印当前状态的名称
返回值   : 根据状态机指针得到的状态机名称
参数     : @fsm 状态机指针
作者     : 张彦升
日期     : 2013年11月11日 14:47:24
*/
static const char* fsm_state_name(const Fsm* fsm)
{
    if (fsm->fsm_cur_state == fsm->fsm_start_state)
    {
        return "fsm_start_state";
    }
    else if (fsm->fsm_cur_state == fsm->fsm_terminal_state)
    {
        return "fsm_terminal_state";
    }
    else if (fsm->fsm_cur_state == fsm->fsm_begin_new_cycle)
    {
        return "fsm_begin_new_cycle";
    }
    else if (fsm->fsm_cur_state == fsm->fsm_series_new_cycle_syn)
    {
        return "fsm_series_new_cycle_syn";
    }
    else if (fsm->fsm_cur_state == fsm->fsm_cpu_new_cycle_syn_step1)
    {
        return "fsm_cpu_new_cycle_syn_step1";
    }
    else if (fsm->fsm_cur_state == fsm->fsm_cpu_new_cycle_syn_step2)
    {
        return "fsm_cpu_new_cycle_syn_step2";
    }
    else if (fsm->fsm_cur_state == fsm->fsm_communicate)
    {
        return "fsm_communicate";
    }
    else if (fsm->fsm_cur_state == fsm->fsm_device_status_judge)
    {
        return "fsm_device_status_judge";
    }
    else if (fsm->fsm_cur_state == fsm->fsm_series_input_syn)
    {
        return "fsm_series_input_syn";
    }
    else if (fsm->fsm_cur_state == fsm->fsm_cpu_input_syn_step1)
    {
        return "fsm_cpu_input_syn_step1";
    }
    else if (fsm->fsm_cur_state == fsm->fsm_cpu_input_syn_step2)
    {
        return "fsm_cpu_input_syn_step2";
    }
    else if (fsm->fsm_cur_state == fsm->fsm_authority_manage)
    {
        return "fsm_authority_manage";
    }
    else if (fsm->fsm_cur_state == fsm->fsm_series_switch)
    {
        return "fsm_series_switch";
    }
    else if (fsm->fsm_cur_state == fsm->fsm_interlocking_calculate)
    {
        return "fsm_interlocking_calculate";
    }
    else if (fsm->fsm_cur_state == fsm->fsm_series_result_syn)
    {
        return "fsm_series_result_syn";
    }
    else if (fsm->fsm_cur_state == fsm->fsm_cpu_result_syn_step1)
    {
        return "fsm_cpu_result_syn_step1";
    }
    else if (fsm->fsm_cur_state == fsm->fsm_cpu_result_syn_step2)
    {
        return "fsm_cpu_result_syn_step2";
    }
    else if (fsm->fsm_cur_state == fsm->fsm_output_result)
    {
        return "fsm_output_result";
    }
    else if (fsm->fsm_cur_state == fsm->fsm_eeu_request)
    {
        return "fsm_eeu_request";
    }
    else
    {
        return "";
    }
}
/*
功能描述    : 等待新周期开始，该状态不做任何工作
返回值      : 始终为0
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:05:24
*/
static int32_t wait_new_cycle(Fsm* UNUSED(fsm))
{
    int32_t ret = 0;

    /*释放资源以便降低CPU使用率*/
    ret = sem_wait(&cycle_sem);

    return ret;
}
/*
功能描述    : 终止状态，该状态只为保证状态自动机的有效运行，不做任何工作
返回值      : 始终为0
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:35:03
*/
static int32_t terminal_state(Fsm* UNUSED(fsm))
{
    while (0 == sem_trywait(&cycle_sem));

    return 0;
}
/*
功能描述    : 开始新周期，下一状态为双CPU联锁新周期同步
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:05:38
*/
static int32_t begin_new_cycle(Fsm* fsm)
{
    CIPerformance_Send(PDT_CI_CYCLE_BEGIN);

    if (CI_TRUE == b_first_cycle)
    {
        /*启动运行的第一个周期清空一次缓存*/
        CISeries_CleanRecvBuffer();
    }
    /*清除eeu_request_mutex，确保下发命令与发送请求之间存在延迟*/
    while (0 == sem_trywait(&eeu_request_sem));
    /*下一状态为双CPU新周期数据同步*/
    fsm_transform(fsm,fsm->fsm_series_new_cycle_syn);

    return 0;
}
/*
功能描述    : 主系状态自动机双系联锁新周期同步
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:49:02
*/
static int32_t pr_series_new_cycle_syn(Fsm* fsm)
{
    int32_t count = 0;
    int32_t ret = 0;

    /*若发送失败则尝试10次，假若双系通信板坏了，主系不能挂在这一步骤，系统还能运行*/
    do 
    {
        ret = CISeries_SendNewCycleSyn();
    } while (10 > count++ && -1 == ret);

    fsm_transform(fsm,fsm->fsm_cpu_new_cycle_syn_step1);

    return 0;
}
/*
功能描述    : 备系状态自动机双CPU新周期数据同步
             发送双系新周期消息
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:50:17
*/
static int32_t sp_series_new_cycle_syn(Fsm* fsm)
{
    /*在故障可切换状态下说明另外一系已经出现故障，则应为发送数据，而非接收数据*/
    if (CI_TRUE == b_switch_to_master || CI_TRUE == CISeriesHeartbeatInt_IsLost())
    {
        if (-1 == CISeries_SendNewCycleSyn())
        {
            return -1;
        }
    }
    else
    {
        if (-1 == CISeries_RecvNewCycleSyn())
        {
            return -1;
        }
        else 
        {
            if (-1 == CISeries_CheckNewCycleSyn())
            {
                return -1;
            }
        }
    }

    fsm_transform(fsm,fsm->fsm_cpu_new_cycle_syn_step1);

    return 0;
}
/*
功能描述    : 从CPU双CPU新周期同步，在从CPU的状态机当中该步骤为空
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:52:09
*/
static int32_t sl_series_new_cycle_syn(Fsm* fsm)
{
    /*在故障可切换状态下说明另外一系已经出现故障，则不应该接收数据*/
    if (CI_TRUE != b_switch_to_master
        && SERIES_MASTER != CISeries_GetLocalState()
        && CI_FALSE == CISeriesHeartbeatInt_IsLost()
        )
    {
        if (-1 == CISeries_RecvNewCycleSyn())
        {
            return -1;
        }
        else 
        {
            if (-1 == CISeries_CheckNewCycleSyn())
            {
                return -1;
            }
        }
    }

    fsm_transform(fsm,fsm->fsm_cpu_new_cycle_syn_step1);

    return 0;
}
/*
功能描述    : 主系状态自动机双CPU新周期数据同步步骤一
返回值      : 成功返回0，失败返回-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:49:02
*/
static int32_t pr_cpu_new_cycle_syn_step1(Fsm* fsm)
{
    if (-1 == CICpu_SendNewCycleSyn())
    {
        /*
         * 发送不成功，我们应该让其一直停留在这一步持续发送，等待心跳管理对其处理
         * 一直阻塞是有问题的，心跳信号存在，但就是收不到同步信息似乎不可能，但是需要
         * 我们进行考虑，请在后面更正这里的逻辑。
         */
        return -1;
    }
    else
    {
        /*下一周期为步骤二*/
        fsm_transform(fsm,fsm->fsm_cpu_new_cycle_syn_step2);
        return 0;
    }
}
/*
功能描述    : 备系状态自动机双CPU新周期数据同步步骤一
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:50:17
*/
static int32_t sp_cpu_new_cycle_syn_step1(Fsm* fsm)
{
    if (-1 == CICpu_SendNewCycleSyn())
    {
        /*
         * 发送不成功，我们应该让其一直停留在这一步持续发送，等待心跳管理对其处理
         * 一直阻塞是有问题的，心跳信号存在，但就是收不到同步信息似乎不可能，但是需要
         * 我们进行考虑，请在后面更正这里的逻辑。
         */
        return -1;
    }
    else
    {
        /*下一周期步骤二*/
        fsm_transform(fsm,fsm->fsm_cpu_new_cycle_syn_step2);

        return 0;
    }
}
/*
功能描述    : 从CPU状态机双CPU新周期同步步骤一
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:52:09
*/
static int32_t sl_cpu_new_cycle_syn_step1(Fsm* fsm)
{
    if (-1 == CICpu_SendNewCycleSyn())
    {
        /*
         * 发送不成功，我们应该让其一直停留在这一步持续发送，等待心跳管理对其处理
         * 一直阻塞是有问题的，心跳信号存在，但就是收不到同步信息似乎不可能，但是需要
         * 我们进行考虑，请在后面更正这里的逻辑。
         */
        return -1;
    }
    else
    {
        /*下一周期步骤二*/
        fsm_transform(fsm,fsm->fsm_cpu_new_cycle_syn_step2);

        return 0;
    }
}
/*
功能描述    : 主系状态自动机双CPU新周期数据同步步骤2
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:49:02
*/
static int32_t pr_cpu_new_cycle_syn_step2(Fsm* fsm)
{
    if (-1 == CICpu_RecvNewCycleSyn())
    {
        /*接受不到数据，则停留在这一步一直接受数据，直到心跳管理程序对其进行处理*/
        return -1;
    }
    else
    {
        if (-1 == CICpu_CheckNewCycleSyn())
        {
            /*如果检查数据不一致*/
            CILog_Msg("新周期同步数据检查不一致");
            /*
             * 当数据检查不一致的时候我们的执行步骤是怎样的，待留，记住，如果在这里直接返回
             * 将导致状态机重新接受新周期数据
             */
        }
        /*无论cpu的应答消息是否到来，主cpu以我为主，执行下一周期为获取通信数据*/
        fsm_transform(fsm,fsm->fsm_communicate);
        return 0;
    }
}
/*
功能描述    : 备系状态自动机双CPU新周期数据同步步骤2
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:50:17
*/
static int32_t sp_cpu_new_cycle_syn_step2(Fsm* fsm)
{
    if (-1 == CICpu_RecvNewCycleSyn())
    {
        /*接受不到数据，则停留在这一步一直接受数据，直到心跳管理程序对其进行处理*/
        return -1;
    }
    else
    {
        if (-1 == CICpu_CheckNewCycleSyn())
        {
            /*如果检查数据不一致*/
            CILog_Msg("新周期同步数据检查不一致");
        }

        /*无论cpu的应答消息是否到来，主cpu以我为主，执行下一周期为获取通信数据*/
        fsm_transform(fsm,fsm->fsm_communicate);

        return 0;
    }
}
/*
功能描述    : 从CPU状态机双CPU新周期同步步骤二
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:52:09
*/
static int32_t sl_cpu_new_cycle_syn_step2(Fsm* fsm)
{
    if (-1 == CICpu_RecvNewCycleSyn())
    {
        return -1;
    }
    else
    {
        if (-1 == CICpu_CheckNewCycleSyn())
        {
            /*如果检查数据不一致*/
            CILog_Msg("新周期同步数据检查不一致");
        }
        fsm_transform(fsm,fsm->fsm_communicate);
        return 0;
    }
}
/*
功能描述    : 接受数据。包括控显机的数据，电子单元的数据，双系切换板的数据
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年12月4日 10:35:35
*/
static int32_t communicate_recieve_data(void)
{
    int32_t ret = 0;

    /*从控显机接受数据*/
    ret = CIHmi_RecvData();

    ret = CIEeu_ReceiveStatusData();

    return ret;
}

/*
功能描述    : 从控显机，电子单元等收取数据
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:49:02
*/
static int32_t pr_communicate(Fsm* fsm)
{
    int32_t ret = 0;

    ret = communicate_recieve_data();

    /*
     * 很可能没有接收到数据，这在系统当中是允许的，不一定每一个周期都能收到数据，假若系统
     * 要求每次都要得到电子单元的状态则又是另一种情况，说明需要每个周期都得到数据，那么这
     * 里的逻辑就需要重新更改
     */
    /*下一周期为设备状态判断*/
    fsm_transform(fsm,fsm->fsm_device_status_judge);

    return ret;
}
/*
功能描述    : 备系状态自动机双CPU新周期数据同步
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:50:17
*/
static int32_t sp_communicate(Fsm* fsm)
{
    int32_t ret = 0;

    ret = communicate_recieve_data();

    /*下一周期为设备状态判断*/
    fsm_transform(fsm,fsm->fsm_device_status_judge);

    return ret;
}
/*
功能描述    : 从CPU双CPU新周期同步
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:52:09
*/
static int32_t sl_communicate(Fsm* fsm)
{
    int32_t ret = 0;

    ret = communicate_recieve_data();

    /*下一周期为设备状态判断*/
    fsm_transform(fsm,fsm->fsm_device_status_judge);

    return ret;
}
/*
功能描述    : 主系状态机设备状态判断
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:49:02
*/
static int32_t pr_device_status_judge(Fsm* fsm)
{
    CIEeu_StatusJudge();

    fsm_transform(fsm,fsm->fsm_series_input_syn);

    return 0;
}
/*
功能描述    : 备系状态自动机设备状态判断
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:50:17
*/
static int32_t sp_device_status_judge(Fsm* fsm)
{
    CIEeu_StatusJudge();

    fsm_transform(fsm,fsm->fsm_series_input_syn);

    return 0;
}
/*
功能描述    : 从CPU状态机状态设备判断
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:52:09
*/
static int32_t sl_device_status_judge(Fsm* fsm)
{
    CIEeu_StatusJudge();

    fsm_transform(fsm,fsm->fsm_series_input_syn);

    return 0;
}
/*
功能描述    : 主系状态自动机双系输入数据同步
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:49:02
*/
static int32_t pr_series_input_syn(Fsm* fsm)
{
    int32_t count = 0;
    int32_t ret = 0;

    do
    {
        ret = CISeries_SendInputSyn();
    } while (10 > count++ && -1 == ret);

    fsm_transform(fsm,fsm->fsm_cpu_input_syn_step1);

    return 0;
}
/*
功能描述    : 备系状态自动机双系新周期数据同步
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:50:17
*/
static int32_t sp_series_input_syn(Fsm* fsm)
{
    /*在故障可切换状态下说明另外一系已经出现故障，则应为发送数据，而非接收数据*/
    if (CI_TRUE == b_switch_to_master || CI_TRUE == CISeriesHeartbeatInt_IsLost())
    {
        if (-1 == CISeries_SendInputSyn())
        {
            return -1;
        }
    }
    else
    {
        if (-1 == CISeries_RecvInputSyn())
        {
            return -1;
        }
        else
        {
            if (-1 == CISeries_CheckInputSyn())
            {
                CIFailureInt_SetInfo(FAILURE_SERIES_INPUT_CMP_FAIL,"备系主CPU双系输入数据不一致");

                /*不一致则进行数据容错*/
                if(SERIES_STANDBY == CISeries_GetLocalState() 
                    && CI_TRUE == CISeries_IsPeerEeuBroke()
                    && (CI_TRUE == CIEeu_IsChannelAOk() || CI_TRUE == CIEeu_IsChannelBOk())
                  )
                {
                    /*
                     * 请注意同步问题 
                     *        主系      备系
                     *        ---       ---
                     * A\/B\/  |         |  A\/B\/     两系电子单元Ａ路和Ｂ路都正常
                     *         |         |
                     *         |         |
                     * request |->93     |
                     *        ---       ---
                     * AX BX   |         |  AX BX      两系电子单元Ａ路和Ｂ路都断开，说明上一周期主系请求未下发下去
                     *         |   (1)   |
                     *         |         |
                     * request |     93<-|             备系使用主系上一周期请求号进行请求
                     *        ---       ---
                     * AX BX   |         |  A\/B\/     备系电子单元通信正常，主系电子单元断开
                     *       <-|-> (2)   |             主系设置切换标志，系统将在下一个周期进行切换
                     *         |         |
                     * request |     94<-|             备系尝试请求
                     *        ---       ---
                     * AX BX   |   (3)   |  A\/B\/     备系电子单元通信正常，主系电子单元断开
                     *         |<------->|             双系发送切换
                     *         |         |
                     * request |     96<-|             系统正常，主系进行请求
                     *        ---       ---
                     *
                     * 上述模型存在的问题：
                     * 主系共有３个周期探测到电子单元通信断开，恰好在第３个周期设置了故障导向安全
                     * 而在此它的输入数据同步至了备系，导致备系使用了故障导向安全的数据进行切换，
                     * 由此，可能出现信号灯灭灯的情况。
                     * 解决上述问题的一种思路是，当发生备系健康于主系的情况时不进行同步输入数据
                     */
                    CILog_Msg("电子单元状态备系健康于主系，本周期不进行容错双系输入数据");
                }
                else
                {
                    CISeries_FaultTolerantInputSyn();

                    /*容错完成之后再检查一次，确保代码正确*/
                    assert(-1 != CISeries_CheckInputSyn());
                }
            }
        }
    }

    fsm_transform(fsm,fsm->fsm_cpu_input_syn_step1);

    return 0;
}
/*
功能描述    : 从CPU状态自动机双系输入数据，该步骤无操作
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:52:09
*/
static int32_t sl_series_input_syn(Fsm* fsm)
{
    /*在故障可切换状态下说明另外一系已经出现故障，则不应该接收数据*/
    if (CI_TRUE != b_switch_to_master
        && SERIES_MASTER != CISeries_GetLocalState()
        && CI_FALSE == CISeriesHeartbeatInt_IsLost()
        )
    {
        if (-1 == CISeries_RecvInputSyn())
        {
            return -1;
        }
        else
        {
            if (-1 == CISeries_CheckInputSyn())
            {
                CIFailureInt_SetInfo(FAILURE_SERIES_INPUT_CMP_FAIL,"从CPU双系输入数据不一致");
                /*不一致则进行数据容错，不进行重启*/
                CISeries_FaultTolerantInputSyn();
                /*容错完成之后再检查一次，确保代码正确*/
                assert(-1 != CISeries_CheckInputSyn());
            }
        }
    }

    fsm_transform(fsm,fsm->fsm_cpu_input_syn_step1);

    return 0;
}
/*
功能描述    : 主系状态机输入数据同步
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:49:02
*/
static int32_t pr_cpu_input_syn_step1(Fsm* fsm)
{
    /*发送不成功则一直尝试发送，直至发送成功*/
    if (-1 == CICpu_SendInputSyn())
    {
        return -1;
    }
    else
    {
        fsm_transform(fsm,fsm->fsm_cpu_input_syn_step2);
        return 0;
    }
}
/*
功能描述    : 备系状态自动机双CPU新周期数据同步
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:50:17
*/
static int32_t sp_cpu_input_syn_step1(Fsm* fsm)
{
    /*发送不成功则一直尝试发送，直至发送成功*/
    if (-1 == CICpu_SendInputSyn())
    {
        return -1;
    }
    else
    {
        fsm_transform(fsm,fsm->fsm_cpu_input_syn_step2);
        return 0;
    }
}
/*
功能描述    : 从CPU双CPU新周期同步，接受来自主CPU的同步数据
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:52:09
*/
static int32_t sl_cpu_input_syn_step1(Fsm* fsm)
{
    /*发送不成功则一直尝试发送，直至发送成功*/
    if (-1 == CICpu_SendInputSyn())
    {
        return -1;
    }
    else
    {
        fsm_transform(fsm,fsm->fsm_cpu_input_syn_step2);

        return 0;
    }
}
/*
功能描述    : 主系状态机输入数据同步步骤2
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:49:02
*/
static int32_t pr_cpu_input_syn_step2(Fsm* fsm)
{
    if (-1 == CICpu_RecvInputSyn())
    {
        return -1;
    }
    else
    {
        /*检查以及校正数据*/
        if(0 > CICpu_CheckInputSyn())
        {
            CIFailureInt_SetInfo(FAILURE_CPU_INPUT_CMP_FAIL,"主系主CPU输入数据比较不一致");
        }

        fsm_transform(fsm,fsm->fsm_authority_manage);

        return 0;
    }
}
/*
功能描述    : 备系状态自动机双CPU新周期数据同步
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:50:17
*/
static int32_t sp_cpu_input_syn_step2(Fsm* fsm)
{
    if (-1 == CICpu_RecvInputSyn())
    {
        return -1;
    }
    else
    {
        if( 0 > CICpu_CheckInputSyn())
        {
            CIFailureInt_SetInfo(FAILURE_CPU_INPUT_CMP_FAIL,"备系主CPU输入数据比较不一致");
        }
        fsm_transform(fsm,fsm->fsm_authority_manage);

        return 0;
    }
}
/*
功能描述    : 从CPU双CPU输入数据同步
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:52:09
*/
static int32_t sl_cpu_input_syn_step2(Fsm* fsm)
{
    if (-1 == CICpu_RecvInputSyn())
    {
        return -1;
    }
    else
    {
        if (-1 == CICpu_CheckInputSyn())
        {
            CIFailureInt_SetInfo(FAILURE_CPU_INPUT_CMP_FAIL,"从CPU双CPU输入数据不一致");
            /*从cpu输入数据与主cpu不一致，以主cpu为准进行容错处理*/
            CICpu_FaultTolerantInputSyn();

            /*容错完成之后再比较一次，确保代码正确*/
            assert(-1 != CICpu_CheckInputSyn());
        }
        fsm_transform(fsm,fsm->fsm_authority_manage);

        return 0;
    }
}

/*
功能描述    : 权限管理
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:49:02
*/
static int32_t pr_authority_manage(Fsm* fsm)
{
    /*下一周期为双系切换*/
    fsm_transform(fsm,fsm->fsm_series_switch);

    return 0;
}
/*
功能描述    : 权限管理
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:50:17
*/
static int32_t sp_authority_manage(Fsm* fsm)
{
    /*下一周期为双系切换*/
    fsm_transform(fsm,fsm->fsm_series_switch);

    return 0;
}
/*
功能描述    : 权限管理
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:52:09
*/
static int32_t sl_authority_manage(Fsm* fsm)
{
    /*下一周期为双系切换*/
    fsm_transform(fsm,fsm->fsm_series_switch);

    return 0;
}
/*
 功能描述    : 切换到校核
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年5月29日 20:09:05
*/
static Fsm* switch_to_check(void)
{
    CISeries_SwitchToCheck();
    CITimer_ResetSystemStartCounter();
    ci_fsm = &standby_fsm;
    standby_count = 0;

    return ci_fsm;
}
/*
 功能描述    : 切换到主系
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年5月29日 20:09:05
*/
static Fsm* switch_to_master(void)
{
    CISeries_SwitchToMaster();
    CITimer_ResetSystemStartCounter();
    CISeries_CleanRecvBuffer();

    ci_fsm = &master_fsm;
    /*清除eeu_request_mutex，确保下发命令与发送请求之间存在延迟*/
    while (0 == sem_trywait(&eeu_request_sem));

    return ci_fsm;
}
/*
功能描述    : 双系切换
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:49:02
*/
static int32_t pr_series_switch(Fsm* fsm)
{
    int ret = 0;

    /* 
     *
     * 对于主系只在此状态机内做一次接收动作，其它状态机内都不会出现接受数据的情况
     * */
    ret = CISeries_RecvCfgSyn();
    if (0 == ret)
    {
        /*
         * 在这里收到配置比较的请求时无论检查对方的数据是否正确都给个回复，否则对方会
         * 进入等待状态
         */
        CILog_Msg("收到备系配置比较请求");
        ret = CISeries_CheckCfgSyn();
        if (0 == ret)
        {
            CILog_Msg("检查备系配置比较数据成功，正在尝试发送配置比较数据...");
        }
        else
        {
            CILog_Msg("检查双系配置比较数据失败，正在尝试发送配置比较数据...");
        }
        ret = CISeries_SendCfgSyn();
        if (-1 == ret)
        {
            CILog_Msg("发送双系配置比较数据失败");
        }
        else
        {
            CILog_Msg("发送双系配置比较数据成功");
        }
    }
    /*
     * 人工切换存在风险，假若另外一系未在热备状态下就进行切换将会出现严重问题，另外
     * 在系统启动超过3个联锁周期才可以切换，否则有可能出现不明确问题
     **/
    if (SERIES_STANDBY == CISeries_GetPeerState() && 40 < CITimer_GetSystemStartCounter())
    {
        if (CI_TRUE == CISwitch_CouldSwitch())
        {
            fsm = switch_to_check();
        }
    }
    CISwitch_SwitchOver();

    if (CI_TRUE == CISwitch_ConsumeSwitchCmd() && CI_FALSE == CISeries_IsPeerEeuBroke())
    {
        /*设置切换标志，系统将在下个周期进行切换*/
        CISwitch_LetSwitch();
    }

    /*下一周期为联锁运算*/
    fsm_transform(fsm,fsm->fsm_interlocking_calculate);

    return 0;
}
/*
功能描述    : 双系切换
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:50:17
*/
static int32_t sp_series_switch(Fsm* fsm)
{
    /*当时校核状态时，系统运行了3个周期便会切换为热备状态*/
    if (SERIES_CHECK == CISeries_GetLocalState() && 4 == standby_count)
    {
        CISeries_SwitchToStandby();
    }

    b_switch_to_master = CI_FALSE;

    standby_count ++;
    /*
     * 当是热备状态，并且需要切换时，因为我们将备系校核和备系热备合并在了一个状态机当中，
     * 所以在这里进行了判断，如果将这个两个状态机分开的话则无需这么做，但是代码会显得很
     * 冗余
     */
    if (SERIES_STANDBY == CISeries_GetLocalState() && 40 < CITimer_GetSystemStartCounter())
    {
        /*
         * 当心跳信号丢失（双系光纤通信道断开），但是从切换装置得到另外一系仍然在主系状态
         * 这是备系提升为主系肯定会发生双主系故障，为此，将备系停机。
         * NOTE：
         * 一个很重要的问题是心跳信号丢失的周期控制。
         *
         *    |--fsm_communicate
         *    |  fsm_device_status_judge
         *    |  fsm_series_input_syn
         *time|  fsm_cpu_input_syn_step1
         *    |  fsm_cpu_input_syn_step2
         *    |  fsm_authority_manage
         *    |--fsm_series_switch
         * 我们认为从fsm_communicate到fsm_series_switch状态转换所耗的时间小于50ms。
         * 若从fsm_communicate到fsm_series_switch状态转换所耗的时间大于心跳信号检测丢失
         * 的时间则会导致从切换装置收到的状态与当前检测的不一致。
         * 通过这种控制，总是可以使双系安全的完成切换
         */
        if (CI_TRUE == CISeriesHeartbeatInt_IsLost())
        {
            if (CISwitch_PeerAlive() == CI_TRUE)
            {
                CIFailureInt_SetInfo(FAILURE_SERIES_HEARTBEAT_LOST,"双系通信道故障，切换器返回主系状态正常,备系自动停机");
            }
            else
            {
                fsm = switch_to_master();
                b_switch_to_master = CI_TRUE;
            }
        }
        if (CI_TRUE == CISwitch_CouldSwitch())
        {
            fsm = switch_to_master();
            b_switch_to_master = CI_TRUE;
            CIEeu_ClearErrorCount();
        }
    }
    CISwitch_SwitchOver();
    /*下一周期为联锁运算*/
    fsm_transform(fsm,fsm->fsm_interlocking_calculate);

    return 0;
}
/*
功能描述    : 双系切换
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:52:09
*/
static int32_t sl_series_switch(Fsm* fsm)
{
    CISwitch_SwitchOver();
    /*下一周期为联锁运算*/
    fsm_transform(fsm,fsm->fsm_interlocking_calculate);

    return 0;
}
/*
功能描述    : 联锁计算
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:49:02
*/
static int32_t pr_interlocking_calculation(Fsm* fsm)
{
    int32_t ret = 0;

    ret = CIInter_Calculate();
    assert(-1 != ret);
    /*确保当前周期的向控显机发送的所有提示信息都发送*/
    CIHmi_WriteCachePush();
    /*下一周期为比较双CPU结果数据*/
    fsm_transform(fsm,fsm->fsm_series_result_syn);

    return 0;
}
/*
功能描述    : 备系状态自动机双CPU新周期数据同步
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:50:17
*/
static int32_t sp_interlocking_calculation(Fsm* fsm)
{
    int32_t ret = 0;

    ret = CIInter_Calculate();
    /*确保当前周期的向控显机发送的所有提示信息都发送*/
    CIHmi_WriteCachePush();
    assert(-1 != ret);
    /*下一周期为比较双CPU结果数据*/
    fsm_transform(fsm,fsm->fsm_series_result_syn);

    return 0;
}
/*
功能描述    : 从CPU双CPU新周期同步
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:52:09
*/
static int32_t sl_interlocking_calculation(Fsm* fsm)
{
    int32_t ret = 0;
    ret = CIInter_Calculate();
    assert(-1 != ret);
    /*下一周期为比较双CPU结果数据*/
    fsm_transform(fsm,fsm->fsm_series_result_syn);

    return 0;
}
/*
功能描述    : 双系结果数据同步
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:49:02
*/
static int32_t pr_series_result_syn(Fsm* fsm)
{
    int32_t count = 0;
    int32_t ret = 0;

    do 
    {
        ret = CISeries_SendResultSyn();
    } while (10 > count++ && -1 == ret);

    fsm_transform(fsm,fsm->fsm_cpu_result_syn_step1);

    return 0;
}
/*
功能描述    : 备系状态自动机双CPU新周期数据同步
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:50:17
*/
static int32_t sp_series_result_syn(Fsm* fsm)
{
    /*在故障可切换状态下说明另外一系已经出现故障，则应为发送数据，而非接收数据*/
    if (CI_TRUE == b_switch_to_master || CI_TRUE == CISeriesHeartbeatInt_IsLost() )
    {
        if (-1 == CISeries_SendResultSyn())
        {
            return -1;
        }
    }
    else
    {
        if (-1 == CISeries_RecvResultSyn())
        {
            return -1;
        }
        else
        {
            if(CI_TRUE == CISeries_IsPeerEeuBroke()
                && (CI_TRUE == CIEeu_IsChannelAOk() || CI_TRUE == CIEeu_IsChannelBOk())
                && SERIES_STANDBY == CISeries_GetLocalState())
            {
                CILog_Msg("备系电子单元通信将康于主系，本周期不做双系结果比较");
            }
            else if(SERIES_CHECK == CISeries_GetLocalState())
            {
                CILog_Msg("校核阶段不进行双系结果处理");
            }
            else
            {
                if (-1 == CISeries_CheckResultSyn())
                {
                    /*
                     * 在校核状态下的第一个周期不进行故障设置
                     * SERIES_CHECK == CISeries_GetState() && CI_TRUE == CICycleInt_IsFirstCycle()
                     * 德摩根律得下式
                     */
                    if (SERIES_CHECK != CISeries_GetLocalState() || CI_TRUE != CICycleInt_IsFirstCycle())
                    {
                        /*如果检查不一致则认为严重故障，并应该重启*/
                        CIFailureInt_SetInfo(FAILURE_SERIOUS,"双系结果数据不一致");
                        return -1;
                    }
                }
            }
        }
    }

    fsm_transform(fsm,fsm->fsm_cpu_result_syn_step1);

    return 0;
}
/*
功能描述    : 从CPU双CPU新周期同步
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:52:09
*/
static int32_t sl_series_result_syn(Fsm* fsm)
{
    /*在故障可切换状态下说明另外一系已经出现故障，则不应该继续接收数据*/
    if (CI_TRUE != b_switch_to_master
        && SERIES_MASTER != CISeries_GetLocalState()
        && CI_FALSE == CISeriesHeartbeatInt_IsLost()
        )
    {
        if (-1 == CISeries_RecvResultSyn())
        {
            return -1;
        }
        else
        {
            if(SERIES_STANDBY == CISeries_GetLocalState()
                && CI_TRUE == CISeries_IsPeerEeuBroke()
                && (CI_TRUE == CIEeu_IsChannelAOk() || CI_TRUE == CIEeu_IsChannelBOk())
              )
            {
                CILog_Msg("备系电子单元通信将康于主系，本周期不做双系结果比较");
            }
            else if(SERIES_CHECK == CISeries_GetLocalState())
            {
                CILog_Msg("双系结果不一致，校核阶段不进行结果不一致故障处理");
            }
            else
            {
                if (-1 == CISeries_CheckResultSyn())
                {
                    /*
                     * 在校核状态下的第一个周期不进行故障设置
                     * SERIES_CHECK == CISeries_GetState() && CI_TRUE == CICycleInt_IsFirstCycle()
                     * 德摩根律得下式
                     */
                    if (SERIES_CHECK != CISeries_GetLocalState() || CI_TRUE != CICycleInt_IsFirstCycle())
                    {
                        /*如果检查不一致则认为严重故障，并应该重启*/
                        CIFailureInt_SetInfo(FAILURE_SERIOUS,"双系结果数据不一致");
                        return -1;
                    }
                }
            }
        }
    }

    fsm_transform(fsm,fsm->fsm_cpu_result_syn_step1);

    return 0;
}
/*
功能描述    : 双CPU结果同步
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:49:02
*/
static int32_t pr_cpu_result_syn_step1(Fsm* fsm)
{
    if (-1 == CICpu_SendResultSyn())
    {
        return -1;
    }
    else
    {
        /*下一周期为比较双CPU结果数据步骤2*/
        fsm_transform(fsm,fsm->fsm_cpu_result_syn_step2);

        return 0;
    }
}
/*
功能描述    : 备系状态自动机双CPU新周期数据同步
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:50:17
*/
static int32_t sp_cpu_result_syn_step1(Fsm* fsm)
{
    if (-1 == CICpu_SendResultSyn())
    {
        return -1;
    }
    else
    {
        /*下一周期为比较双CPU结果数据步骤2*/
        fsm_transform(fsm,fsm->fsm_cpu_result_syn_step2);

        return 0;
    }
}
/*
功能描述    : 从CPU双CPU新周期同步
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:52:09
*/
static int32_t sl_cpu_result_syn_step1(Fsm* fsm)
{
    if (-1 == CICpu_SendResultSyn())
    {
        return -1;
    }
    else
    {
        /*下一周期为比较双CPU结果数据步骤2*/
        fsm_transform(fsm,fsm->fsm_cpu_result_syn_step2);
    }

    return 0;
}
/*
功能描述    : 主CPU双CPU结果同步步骤2
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:49:02
*/
static int32_t pr_cpu_result_syn_step2(Fsm* fsm)
{
    if (-1 == CICpu_RecvResultSyn())
    {
        return -1;
    }
    else
    {
        if (-1 == CICpu_CheckResultSyn())
        {
            /*
             * 在校核状态下的第一个周期不进行故障设置
             * SERIES_CHECK == CISeries_GetState() && CI_TRUE == CICycleInt_IsFirstCycle()
             * 德摩根律得下式
             */
            if (SERIES_CHECK != CISeries_GetLocalState() || CI_TRUE != CICycleInt_IsFirstCycle())
            {
                /*对于输出数据比较不进行容错，直接设置故障*/
                CIFailureInt_SetInfo(FAILURE_SYNRESULT,"输出同步比较不一致");
                return -1;
            }
        }
        fsm_transform(fsm,fsm->fsm_output_result);

        return 0;
    }
}
/*
功能描述    : 备系状态自动机双CPU新周期数据同步
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:50:17
*/
static int32_t sp_cpu_result_syn_step2(Fsm* fsm)
{
    if (-1 == CICpu_RecvResultSyn())
    {
        return -1;
    }
    else
    {
        if (-1 == CICpu_CheckResultSyn())
        {
            /*
             * 在校核状态下的第一个周期不进行故障设置
             * SERIES_CHECK == CISeries_GetState() && CI_TRUE == CICycleInt_IsFirstCycle()
             * 德摩根律得下式
             */
            if (SERIES_CHECK != CISeries_GetLocalState() || CI_TRUE != CICycleInt_IsFirstCycle())
            {
                /*对于输出数据比较不进行容错，直接设置故障*/
                CIFailureInt_SetInfo(FAILURE_SYNRESULT,"输出同步比较不一致");
                return -1;
            }
        }
        fsm_transform(fsm,fsm->fsm_output_result);

        return 0;
    }
}
/*
功能描述    : 从CPU双CPU新周期同步
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:52:09
*/
static int32_t sl_cpu_result_syn_step2(Fsm* fsm)
{
    if (-1 == CICpu_RecvResultSyn())
    {
        return -1;
    }
    else
    {
        if (-1 == CICpu_CheckResultSyn())
        {
            /*
             * 在校核状态下的第一个周期不进行故障设置
             * SERIES_CHECK == CISeries_GetState() && CI_TRUE == CICycleInt_IsFirstCycle()
             * 德摩根律得下式
             */
            if (SERIES_CHECK != CISeries_GetLocalState() || CI_TRUE != CICycleInt_IsFirstCycle())
            {
                /*对于输出数据比较不进行容错，直接设置故障*/
                CIFailureInt_SetInfo(FAILURE_SYNRESULT,"输出同步比较不一致");
                return -1;
            }
        }
        fsm_transform(fsm,fsm->fsm_output_result);

        return 0;
    }
}
/*
功能描述    : 结果数据输出
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:49:02
*/
static int32_t pr_output_result(Fsm* fsm)
{
    CIHmi_SendNodeData();

    CIPerformance_Send(PDT_EEU_T_BEGIN);
    CIEeu_SendAllCommand();

    CIPerformance_Send(PDT_EEU_T_END);

    CIInter_ClearCommands();                /*清理禁止信号*/
    CIInter_ClearSection();                 /*清理系统模块*/

    fsm_transform(fsm,fsm->fsm_eeu_request);

    CIPerformance_Send(PDT_CI_CYCLE_END);

    return 0;
}
/*
功能描述    : 备系状态自动机双CPU新周期数据同步
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:50:17
*/
static int32_t sp_output_result(Fsm* fsm)
{
    CIHmi_SendNodeData();

    /*
     *       主系　　　　备系
     *        ---         ---
     * AX B\/  |           |  A\/B\/
     *         |           |
     *         |           |
     *         |           |
     * 2f7[2f3]|->         |          本次请求2f7上次请求2f3
     *        ---         ---
     * AX BX   |           |  AX BX
     *         |           |
     *         |           |
     *         |           |
     *         |         <-| 2f7[2f3]
     *        ---         ---
     * AX BX   |           |  A\/B\/
     *         |           |
     *         |           |
     *         |     <-----|           备系将康系数高于主系，开始下发控制命令
     *         |         <-| 2f8[2f7]
     *        ---         ---
     * AX BX   |           |  A\/B\/
     *         |           |
     *         |<--------->|  switch
     *         |           |
     *         |         <-| 2fc[2f8]
     *        ---         ---
     * 上述模型当中有一个周期下发了控制命令，若不发送控制命令可能存在3
     * 个周期不下发控制命令，在电子模块实现逻辑当中要求1.2s内无控制命令
     * 就会故障导向安全，通过这种方式避免了模块过早的故障导向安全。
     */

    if(CI_TRUE == CISeries_IsPeerEeuBroke() && SERIES_STANDBY == CISeries_GetLocalState())
    {
        CILog_Msg("主系电子单元断开，备系正在发送命令");
        CIEeu_SendAllCommand();
    }

    CIInter_ClearCommands();                /*清理禁止信号*/
    CIInter_ClearSection();                 /*清理系统模块*/

    fsm_transform(fsm,fsm->fsm_eeu_request);

    CIPerformance_Send(PDT_CI_CYCLE_END);

    return 0;
}
/*
功能描述    : 从CPU双CPU新周期同步
返回值      : 成功为0，失败为-1
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:52:09
*/
static int32_t sl_output_result(Fsm* fsm)
{
    CIInter_ClearCommands();                 /*清理静止信号*/
    CIInter_ClearSection();                  /*清理系统模块*/

    fsm_transform(fsm,fsm->fsm_eeu_request);

    CIPerformance_Send(PDT_CI_CYCLE_END);

    return 0;
}

/*
 功能描述    : 主系主CPU电子单元请求数据状态
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年8月22日 14:10:27
*/
static int32_t pr_eeu_request(Fsm* fsm)
{
    /*
     * 等待请求定时器将其唤醒并下发数据
     * 由备系向主系切换时可能会导致请求一直停留在下一个周期,所以这里不等待
     * 请求信号量
     **/
    if (0 == sem_wait(&eeu_request_sem) || CI_TRUE == b_switch_to_master)
    {
        CIEeu_RequestStatus();
        fsm_transform(fsm,fsm->fsm_terminal_state);

        return 0;
    }
    else
    {
        return -1;
    }
}
/*
 功能描述    : 备系主CPU电子单元请求数据状态
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年8月22日 14:10:27
*/
static int32_t sp_eeu_request(Fsm* fsm)
{
    /*
     * 主系停机，备系向主系切换过程当中会产生延迟，因此这里不等待信号量
     *       主系        备系
     *       ---         ---
     *        |           |
     *        |           |
     *        |           |
     *        |           |
     *STOP    X           |   主系停机
     *                   ---
     *NewCycle            |   等待接受双系新周期数据
     *                    |   等待超时，判断主系停机，自己做为主系运行
     *                    |
     *request             |   由于本次周期延迟原因，状态机还未进入fsm_eeu_request，所以恰好错过请求定时器
     *                    |   进入fsm_eeu_request，等待请求定时器
     *                   ---  提示连锁周期被延迟在fsm_eeu_request状态
     *                    |
     *
     *  根据上面流程图，最好在进入fsm_eeu_request的时候本周期不进行等待定时器，
     *  因为在这个周期停留在新周期已经超过3个定时周期，这段时间保证了电子单元
     *  请求能够被正确的处理
     */

    static int eeu_requestCount = 0 ;
    if (0 == sem_wait(&eeu_request_sem) ||
        (CI_TRUE == b_switch_to_master && CI_TRUE == CISeriesHeartbeatInt_IsLost()))
    {

        /*只有在热备状态下才进行请求，在校核状态下不应该请求*/
        if(CI_TRUE == CISeries_IsPeerEeuBroke() && SERIES_STANDBY == CISeries_GetLocalState() )
        {
              eeu_requestCount = eeu_requestCount%100 +1;
              if (eeu_requestCount <4)
              {
                  CILog_Msg("备系正在尝试电子单元请求");
                  CIEeu_RequestStatus();
              }
              else
              {
                  CIEeu_RequestIncFsn();            
              } 
                fsm_transform(fsm,fsm->fsm_terminal_state);
                return 0;
        }
        else
        {
            CIEeu_RequestIncFsn();
            eeu_requestCount = 0 ;
            fsm_transform(fsm,fsm->fsm_terminal_state);
            return 0;
        }
    }
    else
    {
        return -1;
    }
}
/*
 功能描述    : 从CPU电子单元请求数据状态
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年8月22日 14:10:27
*/
static int32_t sl_eeu_request(Fsm* fsm)
{
    fsm_transform(fsm,fsm->fsm_terminal_state);
    return 0;
}

/*主系主CPU状态机*/
static Fsm master_fsm =
{
    wait_new_cycle,                     /*fsm_cur_state*/
    wait_new_cycle,                     /*fsm_start_state*/
    terminal_state,                     /*fsm_terminal_state*/
    
    begin_new_cycle,                    /*fsm_begin_new_cycle*/
    pr_series_new_cycle_syn,            /*fsm_series_new_cycle_syn*/
    pr_cpu_new_cycle_syn_step1,         /*fsm_cpu_new_cycle_syn_step1*/
    pr_cpu_new_cycle_syn_step2,         /*fsm_cpu_new_cycle_syn_step2*/
    pr_communicate,                     /*fsm_communicate*/
    pr_device_status_judge,             /*fsm_device_status_judge*/
    pr_series_input_syn,                /*fsm_series_input_syn*/
    pr_cpu_input_syn_step1,             /*fsm_cpu_input_syn_step1*/
    pr_cpu_input_syn_step2,             /*fsm_cpu_input_syn_step2*/
    pr_authority_manage,                /*fsm_authority_manage*/
    pr_series_switch,                   /*fsm_series_switch*/
    pr_interlocking_calculation,        /*fsm_interlocking_calculate*/
    pr_series_result_syn,               /*fsm_series_result_syn*/
    pr_cpu_result_syn_step1,            /*fsm_cpu_result_syn_step1*/
    pr_cpu_result_syn_step2,            /*fsm_cpu_result_syn_step2*/
    pr_output_result,                   /*fsm_output_result*/
    pr_eeu_request,                     /*fsm_eeu_request*/
};
/*备系主CPU状态自动机*/
static Fsm standby_fsm =
{
    wait_new_cycle,                     /*fsm_cur_state*/
    wait_new_cycle,                     /*fsm_start_state*/
    terminal_state,                     /*fsm_terminal_state*/

    begin_new_cycle,                    /*fsm_begin_new_cycle*/
    sp_series_new_cycle_syn,            /*fsm_series_new_cycle_syn*/
    sp_cpu_new_cycle_syn_step1,         /*fsm_cpu_new_cycle_syn_step1*/
    sp_cpu_new_cycle_syn_step2,         /*fsm_cpu_new_cycle_syn_step2*/
    sp_communicate,                     /*fsm_communicate*/
    sp_device_status_judge,             /*fsm_device_status_judge*/
    sp_series_input_syn,                /*fsm_series_input_syn*/
    sp_cpu_input_syn_step1,             /*fsm_cpu_input_syn_step1*/
    sp_cpu_input_syn_step2,             /*fsm_cpu_input_syn_step2*/
    sp_authority_manage,                /*fsm_authority_manage*/
    sp_series_switch,                   /*fsm_series_switch*/
    sp_interlocking_calculation,        /*fsm_interlocking_calculate*/
    sp_series_result_syn,               /*fsm_series_result_syn*/
    sp_cpu_result_syn_step1,            /*fsm_cpu_result_syn_step1*/
    sp_cpu_result_syn_step2,            /*fsm_cpu_result_syn_step2*/
    sp_output_result,                   /*fsm_output_result*/
    sp_eeu_request,                     /*fsm_eeu_request*/
};
/*主系和备系的从CPU状态自动机*/
static Fsm slave_fsm =
{
    wait_new_cycle,                     /*fsm_cur_state*/
    wait_new_cycle,                     /*fsm_start_state*/
    terminal_state,                     /*fsm_terminal_state*/

    begin_new_cycle,                    /*fsm_begin_new_cycle*/
    sl_series_new_cycle_syn,            /*fsm_series_new_cycle_syn*/
    sl_cpu_new_cycle_syn_step1,         /*fsm_cpu_new_cycle_syn_step1*/
    sl_cpu_new_cycle_syn_step2,         /*fsm_cpu_new_cycle_syn_step2*/
    sl_communicate,                     /*fsm_communicate*/
    sl_device_status_judge,             /*fsm_device_status_judge*/
    sl_series_input_syn,                /*fsm_series_input_syn*/
    sl_cpu_input_syn_step1,             /*fsm_cpu_input_syn_step1*/
    sl_cpu_input_syn_step2,             /*fsm_cpu_input_syn_step2*/
    sl_authority_manage,                /*fsm_authority_manage*/
    sl_series_switch,                   /*fsm_series_switch*/
    sl_interlocking_calculation,        /*fsm_interlocking_calculate*/
    sl_series_result_syn,               /*fsm_series_result_syn*/
    sl_cpu_result_syn_step1,            /*fsm_cpu_result_syn_step1*/
    sl_cpu_result_syn_step2,            /*fsm_cpu_result_syn_step2*/
    sl_output_result,                   /*fsm_output_result*/
    sl_eeu_request,                     /*fsm_eeu_request*/
};
/*
功能描述    : 状态机状态转化函数
返回值      : 无
参数        : @fsm 状态机指针
             @state 转移向的状态
作者        : 何境泰
日期        : 2013年10月21日 14:37:27
*/
static void fsm_transform(Fsm* fsm,state_handler state)
{
    fsm->fsm_cur_state = state;
    if (CI_TRUE == b_print_fsm)
    {
        CIRemoteLog_Write("%s:%#x",fsm_state_name(fsm),cycle_counter);
    }
}
/*
功能描述    : 判断有限状态自动机是否已经运行到终止状态
返回值      : 是最后一个状态则返回CI_TRUE，否则返回CI_FALSE
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:17:04
*/
static CI_BOOL fsm_be_end_state(const Fsm* fsm)
{
    if (fsm->fsm_cur_state == fsm->fsm_terminal_state)
    {
        return CI_TRUE;
    }
    else
    {
        return CI_FALSE;
    }
}
/*
功能描述    : 状态机是否实在开始状态
返回值      : 如果是开始状态则返回CI_TRUE，如果不是则返回CI_FALSE
参数        : @fsm状态机指针
参数        : @fsm 状态机指针
作者        : 张彦升
日期        : 2013年10月22日 13:23:01
*/
static CI_BOOL fsm_be_start_state(Fsm* fsm)
{
    if (fsm->fsm_cur_state == fsm->fsm_start_state)
    {
        return CI_TRUE;
    }
    else
    {
        return CI_FALSE;
    }
}
/*
 功能描述    : 当联锁运算周期到来之后便可以进行新周期
 返回值      : 无
 参数        : 无
 作者        : 张彦升
 日期        : 2013年10月21日 14:43:53
*/
static void cycle_action(void)
{
    if (CI_TRUE == fsm_be_start_state(ci_fsm))
    {
        /*周期数增1*/
        cycle_counter ++;

        ci_fsm->fsm_cur_state = ci_fsm->fsm_begin_new_cycle;

        if (CI_TRUE == b_print_fsm)
        {
            CILog_SigMsg("%s:%#x",fsm_state_name(ci_fsm),cycle_counter);
        }

        sem_post(&cycle_sem);
        delay_count = 0;
    }
    else
    {
        /*本周期被延迟了*/
        delay_count ++;
        if (6 < delay_count)
        {
            CIFailureInt_SetInfo(FAILURE_SERIOUS,"连续6个周期联锁周期被延迟");
        }
        CILog_SigMsg("本联锁周期被延迟，在%s状态",fsm_state_name(ci_fsm));
    }

    return ;
}

static CI_BOOL cycle_condition_check(uint32_t elapsed_cycle)
{
    /*使用该变量放置定时周期号由大变小周期被再次触发*/
    static uint32_t last_elapsed_cycle = -1;

    if (last_elapsed_cycle == elapsed_cycle)
    {
        return CI_FALSE;
    }
    if (CI_FALSE == ci_cycle_timer.b_open)
    {
        return CI_FALSE;
    }
    if (0 != elapsed_cycle % ci_cycle_timer.relative_cycle)
    {
        return CI_FALSE;
    }

    last_elapsed_cycle = elapsed_cycle;

    return CI_TRUE;
}

static void eeu_request_action(void)
{
    /*发生故障时不向下发送命令*/
    if (CI_FALSE == CIFailureInt_BeTerminate())
    {
        sem_post(&eeu_request_sem);
    }
    return;
}
static CI_BOOL eeu_request_condition_check(uint32_t elapsed_cycle)
{
    static uint32_t trans_cycle = 0;

    if (CI_FALSE == eeu_request_timer.b_open)
    {
        return CI_FALSE;
    }
    if (0 == eeu_request_relative_cycle)
    {
        /*
         * 为每个联锁周期留50ms的波动时间，为电子单元传输数据预留50ms的波动时间，
         * 如果这个条件不满足则会出现数据收不到的现象
         */
        /*assert(CIEeu_NeedTime() + 50 < CI_CYCLE_MS - 50);*/
        /*根据计算得到时间为理论时间，因此需要为其再加50ms的波动时间*/
        /*trans_cycle = ceil((CIEeu_TransTime() + 50) / (TIMER_CYCLE_NANO / NANO_ONE_MILSECOND));*/
        /*eeu_request_relative_cycle = EEU_REQUEST_CYCLE_RELATIVE - trans_cycle;*/
        eeu_request_relative_cycle = 4;
        /*printf("eeu_request_relative_cycle:%d\n",eeu_request_relative_cycle);*/
    }

    assert(0 < eeu_request_relative_cycle &&
        EEU_REQUEST_CYCLE_RELATIVE > eeu_request_relative_cycle); 

    if (eeu_request_relative_cycle != elapsed_cycle % eeu_request_timer.relative_cycle
        || (CICpu_GetLocalState() != CPU_STATE_MASTER))
    {
        return CI_FALSE;
    }

    return CI_TRUE;
}

uint32_t CICycle_GetEeuRequestRelativeCycle(void)
{
    return eeu_request_relative_cycle;
}
/*
功能描述    : 一个联锁运算周期
返回值      : 无
参数        : 无
作者        : 张彦升
日期        : 2013年10月16日 14:40:20
*/
void CICycleInt_Once(void)
{
    while (CI_FALSE == fsm_be_end_state(ci_fsm)
        && CI_FALSE == CIFailureInt_BeTerminate())
    {
        ci_fsm->fsm_cur_state(ci_fsm);
    }

    if (CI_FALSE == CIFailureInt_BeTerminate())
    {
        fsm_transform(ci_fsm,ci_fsm->fsm_start_state);

        b_first_cycle = CI_FALSE;

        /*
         * 如果延迟之后让状态转移至新周期，重新开始这一周期
         * 当下列情况下时，状态机无意义的延迟一个周期
         * | fsm_cpu_result_syn_step2         
         * |                               <---ci_cycle_timer
         * | fsm_output_result                
         */
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
        /* 由于发生切换时备系可能会进入循环延迟状态,所以将其屏蔽掉,见上面bug说明
        if (0 < delay_count)
        {
            cycle_counter++;

            fsm_transform(ci_fsm,ci_fsm->fsm_begin_new_cycle);
        }
        */
    }

    return;
}

int32_t CICycleInt_Init(void)
{
    static CI_BOOL b_initialized = CI_FALSE;
    int32_t ret = 0;

    if (CI_TRUE == b_initialized)
    {
        return -1;
    }

    /*主系使用一个状态机，热备和校核状态使用一个状态机，从CPU使用一个状态机*/
    if (CPU_STATE_MASTER == CICpu_GetLocalState())
    {
        assert(SERIES_STANDBY != CISeries_GetLocalState());

        if (SERIES_MASTER == CISeries_GetLocalState())
        {
            ci_fsm = &master_fsm;
        }
        else if (SERIES_CHECK == CISeries_GetLocalState())
        {
            ci_fsm = &standby_fsm;
        }
        else
        {
            CILog_Msg("fsm初始化时双系状态错误");
            return -1;
        }
    }
    else if (CPU_STATE_SLAVE == CICpu_GetLocalState())
    {
        ci_fsm = &slave_fsm;
    }
    else
    {
        CILog_Msg("fsm初始化时CPU状态错误");

        return -1;
    }

    assert(NULL != ci_fsm);

    /*注册定时器*/
    ret = CITimer_Regist(&ci_cycle_timer);              /*联锁运算*/
    ret = CITimer_Regist(&eeu_request_timer);           /*请求电子单元数据*/

    /*信号量初始化的时候都应该保持为0*/
    ret = sem_init(&cycle_sem,0,0);
    if (-1 == ret)
    {
        CILog_Errno("初始化cycle_sem失败");
        return -1;
    }
    ret = sem_init(&eeu_request_sem,0,0);
    if (-1 == ret)
    {
        CILog_Errno("初始化eeu_request_sem失败");
        return -1;
    }

    cycle_counter = 0;
    b_initialized = CI_TRUE;

    return ret;
}

/*
功能描述    : 获取联锁周期数
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2013年11月4日 9:39:22
*/
uint32_t CICycleInt_GetCounter(void)
{
    /*返回周期计数值*/
    return cycle_counter;
}

/*
功能描述    : 修改联锁周期数
返回值      : 旧的联锁周期号
参数        : @counter 要设置的计数值
作者        : 何境泰
日期        : 2013年12月25日 10:05:52
*/
uint32_t CICycleInt_SetCounter(uint32_t counter)
{
    uint32_t old_cycle_counter = 0;

    old_cycle_counter = cycle_counter;
    cycle_counter = counter;

    return old_cycle_counter;
}

/*
功能描述    : 重置周期计数器
返回值      : 无
参数        : 无
作者        : 何境泰
日期        : 2013年12月11日 13:57:33
*/
void CICycleInt_ResetCounter(void)
{
    cycle_counter = 0;
}
/*
 功能描述    : 打开状态机输出
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年4月11日 15:53:40
*/
int32_t CICycleInt_OpenPrintFsm(void)
{
    b_print_fsm = CI_TRUE;
    return 0;
}
/*
 功能描述    : 是否是第一个周期
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年7月18日 14:16:45
*/
CI_BOOL CICycleInt_IsFirstCycle(void)
{
    return b_first_cycle;
}

#endif /*!LINUX_ENVRIONMENT*/
