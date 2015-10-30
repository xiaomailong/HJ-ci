/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年10月12日 13:11:32
用途        : 管理本程序实例
历史修改记录: 
**********************************************************************/
#include "util/ci_header.h"

#ifdef LINUX_ENVRIONMENT

#include <sched.h>
#include <getopt.h>
#include <sys/mman.h>
#include <sys/resource.h>

#include "app.h"
#include "sig.h"
#include "platform_api.h"

#include "cpu_manage.h"
#include "series_manage.h"
#include "hmi_manage.h"
#include "eeu_manage.h"
#include "timer.h"
#include "performance.h"
#include "cfg_manage.h"
#include "local_log.h"

#include "led_int.h"
#include "watchdog_int.h"
#include "communicate_state_int.h"
#include "cpu_heartbeat_int.h"
#include "series_heartbeat_int.h"
#include "cycle_int.h"
#include "failures_int.h"
#include "remote_log.h"
#include "patchlevel.h"
#include "switch_board_manage.h"
#include "monitor.h"
#include "tail_log.h"

#include "util/log.h"

#include "interlocking/inter_api.h"

#include "syn_data/input_syn.h"
#include "syn_data/result_syn.h"
#include "syn_data/cfg_syn.h"


static const struct option linux_long_options[] =
{
    {"print_timer", no_argument, NULL, 't'},                    /*输出定时器信息*/
    {"print_cpu_heartbeat", no_argument, NULL, 'c'},            /*输出CPU心跳信号信息*/
    {"print_series_heartbeat", no_argument, NULL, 's'},         /*输出双系心跳信号信息*/
    {"print_hmi", no_argument, NULL, 'H'},                      /*输出控显机信息*/
    {"print_eeu", no_argument, NULL, 'e'},                      /*输出电子单元信息*/
    {"print_fsm", no_argument, NULL, 'f'},                      /*输出状态机信息*/
    {"print_new_cycle", no_argument, NULL, 'n'},                /*输出新周期状态信息*/
    {"print_input", no_argument, NULL, 'i'},                    /*输出输入状态信息*/
    {"print_result", no_argument, NULL, 'r'},                   /*输出输出状态信息*/
    {"print_interlocking", no_argument, NULL, 'I'},             /*输出联锁运算信息*/
    {"print_config_compare", no_argument, NULL, 'C'},           /*输出配置比较信息*/
    {"print_all", no_argument, NULL, 'a'},                      /*输出所有信息*/
    {"stdout", no_argument, NULL, 'o'},                         /*向标准输出输出日志信息*/
    {"record_log", no_argument, NULL, 'L'},                     /*使用日志文件方式记录日志*/
    {"parse_cpu_input", required_argument, NULL, 'N'},          /*使用完整格式打印CPU输入数据*/
    {"parse_cpu_result", required_argument, NULL, 'R'},         /*使用完整格式打印CPU输出数据*/
    {"parse_cfg_syn", required_argument, NULL, 'F'},            /*使用完整格式打印配置比较数据*/

    {"parse_series_input", required_argument, NULL, 'D'},       /*使用完整格式打印双系输入数据*/
    {"parse_series_check_input", required_argument, NULL, 'K'}, /*使用完整格式打印双系校核输入数据*/
    {"parse_series_result", required_argument, NULL, 'E'},      /*使用完整格式打印双系输出数据*/
    {"close_series_cfg_compare", no_argument, NULL, 'M'},       /*关闭双系配置比较*/

    {"help", no_argument, NULL, 'h'},                           /*打印帮助信息*/
    {"version", no_argument, NULL, 'v'},                        /*打印版本信息*/
    {NULL, 0, NULL, 0},
};

/*
 功能描述    : 打印使用帮助
 返回值      : 无
 参数        : 无 
 作者        : 张彦升
 日期        : 2014年4月11日 12:33:12
*/
static void linux_usage(void)
{
    fprintf (stdout,"Usage: %s [OPTIONS] ...\n", CIApp_GetProgramName());

    fputs("Computer Interlocking.\n"
          "\tcomputer interlocking program for HJ. Railway Signaling.\n\n", stdout);

    fputs("Mandatory arguments to long options are mandatory for short options too.\n", stdout);
    fputs("OPTIONS:\n", stdout);
    fputs("       -t, --print_timer                     print all information about timer used\n"
          "                                             by system.\n", stdout);
    fputs("       -c, --print_cpu_heartbeat             print all information about cpu heartbeat.\n"
          "                                             notice that these information are\n"
          "                                             enormous and output very fast.\n", stdout);
    fputs("       -s, --print_series_heartbeat          print all information about series heartbeat\n"
          "                                             notice that these information are enormous\n"
          "                                             and output very fast.\n", stdout);
    fputs("       -H, --print_hmi                       print all information about operator(Human\n"
          "                                             Machine Interface)\n", stdout);
    fputs("       -e, --print_eeu                       print all information about electronic unit\n"
                                                        , stdout);
    fputs("       -f, --print_fsm                       print all information about fsm(Finite State\n"
          "                                             Machine)\n", stdout);
    fputs("       -n, --print_new_cycle                 print all information about new cycle state\n"
          "                                             of fsm(Finite State Machine)\n", stdout);
    fputs("       -i, --print_input                     print all information about input state\n"
          "                                             of fsm(Finite State Machine)\n", stdout);
    fputs("       -r, --print_result                    print all information about result state\n"
          "                                             of fsm(Finite State Machine)\n", stdout);
    fputs("       -I, --print_interlocking              print all information about interlocking\n"
          "                                             calculate state of fsm(Finite State Machine)\n", stdout);
    fputs("       -C, --print_config_compare            print all information about configure compare\n"
          "                                             state of fsm(Finite State Machine)\n", stdout);
    fputs("       -a, --print_all                       print all information equivalent to -tcsHefnirI\n", stdout);
    fputs("       -o, --stdout                          print all log information to stdout\n", stdout);
    fputs("       -N, --parse_cpu_input=dump_file       parse cpu input dump file add print\n", stdout);
    fputs("       -R, --parse_cpu_result=dump_file      parse cpu result dump file add print\n", stdout);
    fputs("       -F, --parse_cfg_syn=dump_file         parse cpu and series config dump file add print\n", stdout);

    fputs("       -D, --parse_series_input=dump_file    parse series input dump file add print\n", stdout);
    fputs("       -E, --parse_series_result=dump_file   parse series result dump file add print\n", stdout);
    fputs("       -M, --close_series_cfg_compare        close series cfg compare(use with standby bootup)\n", stdout);

    fputs("       -L, --record_log                      record system log into log file\n", stdout);
    fputs("       -h, --help                            display this help and exit\n", stdout);
    fputs("       -v, --version                         output version information and exit\n", stdout);

    return;
}
/*
 功能描述    : 使用完整格式打印输入数据的dump文件
 返回值      : 无
 参数        : @file_name 文件名称指针
 作者        : 张彦升
 日期        : 2014年4月14日 14:49:31
*/
static void linux_cpu_input_dump_print(const char* p_file_name)
{
    int fd = 0;
    int ret = 0;
    CIInputSynFrame frame;
    assert(NULL != p_file_name);

    fd = open(p_file_name,O_RDONLY);
    if (-1 == fd)
    {
        CILog_Errno("open %s failed",p_file_name);
        return;
    }
    ret = read(fd,&frame,sizeof(frame));
    if (-1 == ret)
    {
        CILog_Errno("read error");
        return;
    }
    CIInputSyn_Print(&frame);

    close(fd);

    return;
}
/*
 功能描述    : 使用完整格式打印输出数据的dump文件
 返回值      : 无
 参数        : @file_name 文件名称指针
 作者        : 张彦升
 日期        : 2014年4月14日 14:49:31
*/
static void linux_cpu_result_dump_print(const char* p_file_name)
{
    int fd = 0;
    int ret = 0;
    CIResultSynFrame frame;
    assert(NULL != p_file_name);

    fd = open(p_file_name,O_RDONLY);
    if (-1 == fd)
    {
        CILog_Errno("open %s failed",p_file_name);
        return;
    }
    ret = read(fd,&frame,sizeof(frame));
    if (-1 == ret)
    {
        CILog_Errno("read error");
        return;
    }
    CIResultSyn_Print(&frame);

    close(fd);

    return;
}
/*
 功能描述    : 使用完整格式打印配置比较帧
 返回值      : 无
 参数        : @file_name 文件名称指针
 作者        : 张彦升
 日期        : 2014年5月12日 13:38:59
*/
static void linux_cfg_syn_dump_print(const char* p_file_name)
{
    int fd = 0;
    int ret = 0;

    CfgSynFrame frame;
    assert(NULL != p_file_name);

    fd = open(p_file_name,O_RDONLY);
    if (-1 == fd)
    {
        CILog_Errno("open %s failed",p_file_name);
        return;
    }
    ret = read(fd,&frame,sizeof(frame));
    if (-1 == ret)
    {
        CILog_Errno("read error");
        return;
    }
    CICfgSyn_Print(&frame);

    close(fd);

    return ;
}
/*
 功能描述    : 解析双系输入dump文件并打印
 返回值      : 无
 参数        : @file_name文件名称指针
 作者        : 张彦升
 日期        : 2014年5月14日 18:52:48
*/
static void linux_series_input_dump_print(const char* p_file_name)
{
    int fd = 0;
    int ret = 0;

    CIInputSynFrame frame;
    assert(NULL != p_file_name);

    fd = open(p_file_name,O_RDONLY);
    if (-1 == fd)
    {
        CILog_Errno("open %s failed",p_file_name);
        return;
    }
    ret = read(fd,&frame,sizeof(frame));
    if (-1 == ret)
    {
        CILog_Errno("read error");
        return;
    }
    CIInputSyn_Print(&frame);

    close(fd);

    return ;
}
/*
 功能描述    : 解析双系输出dump文件并打印
 返回值      : 无
 参数        : @file_name 文件名称指针
 作者        : 张彦升
 日期        : 2014年5月14日 18:52:48
*/
static void linux_series_result_dump_print(const char* p_file_name)
{
    int fd = 0;
    int ret = 0;

    CIResultSynFrame frame;
    assert(NULL != p_file_name);

    fd = open(p_file_name,O_RDONLY);
    if (-1 == fd)
    {
        CILog_Errno("open %s failed",p_file_name);
        return;
    }
    ret = read(fd,&frame,sizeof(frame));
    if (-1 == ret)
    {
        CILog_Errno("read error");
        return;
    }
    CIResultSyn_Print(&frame);

    close(fd);

    return ;
}
/*
 功能描述    : 解析参数
 返回值      : 成功为0，失败为-1
 参数        : @argc 参数的个数
              @argv 指向参数二维数组字符串的指针
 作者        : 张彦升
 日期        : 2014年4月11日 13:05:30
*/
static int linux_decode_arguments (int argc, char **argv)
{
    int c = 0;
    int oi = 0;
    int32_t ret = 0;

    while (1)
    {
        c = getopt_long(argc, argv, "tcsHefnirIaoCLN:R:F:D:K:E:AmMS:hv", linux_long_options, &oi);
        if (-1 == c)
        {
            break;
        }
        switch (c)
        {
        case 't':
            CITimer_OpenPrintTimer();
            break;
        case 'c':
            CICpu_OpenPrintHeartbeat();
            break;
        case 's':
            CISeries_OpenPrintHeartbeat();
            break;
        case 'H':
            CIHmi_OpenPrintHmi();
            break;
        case 'e':
            CIEeu_OpenPrintEeu();
            break;
        case 'f':
            CICycleInt_OpenPrintFsm();
            break;
        case 'n':
            CICpu_OpenPrintNewCycleSyn();
            CISeries_OpenPrintNewCycleSyn();
            break;
        case 'i':
            CICpu_OpenPrintInputSyn();
            CISeries_OpenPrintInputSyn();
            break;
        case 'r':
            CICpu_OpenPrintResultSyn();
            CISeries_OpenPrintResultSyn();
            break;
        case 'I':
            break;
        case 'C':
            CICpu_OpenPrintCfgSyn();
            CISeries_OpenPrintCfgSyn();
            break;
        case 'a':
            CITimer_OpenPrintTimer();
            CICpu_OpenPrintHeartbeat();
            CISeries_OpenPrintHeartbeat();
            CIHmi_OpenPrintHmi();
            CIEeu_OpenPrintEeu();
            CICycleInt_OpenPrintFsm();
            CICpu_OpenPrintNewCycleSyn();
            CISeries_OpenPrintNewCycleSyn();

            CICpu_OpenPrintInputSyn();
            CISeries_OpenPrintInputSyn();

            CICpu_OpenPrintResultSyn();
            CISeries_OpenPrintResultSyn();

            CICpu_OpenPrintCfgSyn();
            CISeries_OpenPrintCfgSyn();
            break;
        case 'o':
            CILog_OpenStdoutWrite();
            break;
        case 'N':
            linux_cpu_input_dump_print(optarg);
            ret = -1;
            break;
        case 'R':
            linux_cpu_result_dump_print(optarg);
            ret = -1;
            break;
        case 'F':
            linux_cfg_syn_dump_print(optarg);
            ret = -1;
            break;
        case 'D':
            linux_series_input_dump_print(optarg);
            ret = -1;
            break;
        case 'E':
            linux_series_result_dump_print(optarg);
            ret = -1;
            break;
        case 'L':
            CILocalLog_Open();
            break;
        case 'M':
            /*关闭双系配置比较*/
            CISeries_CloseCfgCompare();
            break;
        case 'S':
            /*使用指定setting目录*/
            /*
            ret = CIConfig_SetSettingPath(optarg);
            */
            break;
        case 'h':
            linux_usage();
            ret = -1;
            break;
        case 'v':
            fprintf(stderr,"%s\n","CI version:" CI_VERSION);
            ret = -1;
            break;
        default:
            linux_usage();
            ret = -1;
            break;
        }
    }
    return ret;
}
/*
 功能描述    : 提升本进程优先级
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年7月16日 9:53:23
*/
static int32_t linux_enhance_priority(void)
{
    struct sched_param param; 

    /*我们发现提升优先级后效果不明显，暂时不提升优先级*/
    return 0;

    param.sched_priority = 40; 
    if (sched_setscheduler(getpid(), SCHED_RR, &param) == -1)
    { 
        CILog_Errno("sched_setscheduler() failed"); 
        return -1;
    } 
    return 0;
}
/*
 功能描述    : 程序退出时的清除函数
 返回值      : 无
 参数        : 无
 作者        : 张彦升
 日期        : 2014年4月28日 19:01:17
*/
static void linux_cleaner(void)
{
    CILog_Msg("system_exit:clean and exit...");
    /*将LED灯全部熄灭
     * 20150407注释灭灯代码，系统能运行到这里一定是出故障了，保留点亮的灯
     * 给工作人员额外的信息*/
    /*CILedInt_BlankingAll();*/

    return;
}
static void linux_sig_int(void)
{
    CIWatchdog_Stop();
    CISig_Block();
    CILog_SigMsg("recv SIGINT");

    CIFailureInt_SetInfo(FAILURE_INTERRUPT,"system interrupt by SIGINT");

    return;
}

static void linux_sig_usr1(void)
{
    CIWatchdog_Stop();
    CISig_Block();
    CILog_SigMsg("recv SIGUSR1");

    CIFailureInt_SetInfo(FAILURE_INTERRUPT,"system interrupt by SIGUSR1");

    return;
}
/*
功能描述    : 正常终止程序
返回值      : 无
参数        : 无
作者        : 张彦升
日期        : 2013年10月12日 13:50:59
*/
static void linux_terminate(void)
{
    /*系统终止时给本地记录一份日志*/
    CILocalLog_Open();

    CILog_Msg("system_exit:now terminate ci");
    /*阻塞信号，停止定时器*/
    CISig_UnBlock();

    CILocalLog_CachePush();
    CIRemoteLog_CachePush();

    /*这里加注释的原因为：当CPU结果比较不一致时会将当前运算的信息保存下来*/
    assert(0);

    return;
}
/*
功能描述    : 对系统进行初始化
Taking care of the following during the initial startup phase:
  1.Call mlockall() as soon as possible from main().
  2.Create all threads at startup time of the application, and touch each 
    page of the entire stack of each thread. Never start threads dynamically
    during RT show time, this will ruin RT behavior.
  3.Never use system calls that are known to generate page faults, such as 
    fopen(). (Opening of files does the mmap() system call, which generates
    a page-fault).
  If you use 'compile time global variables' and/or 'compile time global
  arrays', then use mlockall() to prevent page faults when accessing them.
  see:https://rt.wiki.kernel.org/index.php/Frequently_Asked_Questions
返回值      : 0为成功，-1为失败
参数        : 无
作者        : 张彦升
日期        : 2013年10月12日 15:28:17
*/
static int32_t linux_init(void)
{
    int32_t ret = 0;
    struct rlimit core_limit;

    core_limit.rlim_cur = RLIM_INFINITY;
    core_limit.rlim_max = RLIM_INFINITY;

    static CI_BOOL b_initialized = CI_FALSE;

    if (CI_TRUE == b_initialized)
    {
        return -1;
    }

    /*
     * NOTE!在现在的系统当中，为了增加CF卡的寿命，没有做swap分区，所以调用mlockall
     * 系统调用从某种程度上意义不大，因为本质上不会出现页错误，但是按照惯例我们还是
     * 将其调用一次，避免后续版本会出现页错误而导致的延迟
     */
    ret = mlockall(MCL_CURRENT | MCL_FUTURE);
    if (-1 == ret)
    {
        CILog_Errno("mlockall failed");
        return -1;
    }

    /* 设置系统限制，在程序停止时生成core文件 */
    if (0 > setrlimit(RLIMIT_CORE, &core_limit))
    {
        CILog_Errno("ci setrlimit -c unlimit failed");

        return -1;
    }
    /*
     * 初始化本地日志记录，对日志记录初始化返回值的检查使用assert，对其它的初始化因为
     * 日志模块已经可以使用，所以无需生成core，直接返回即可
     */
    ret = CILocalLog_Init();
    assert(-1 != ret);

    /*初始化远程日志记录*/
    ret = CIRemoteLog_Init();
    assert(-1 != ret);

    /*仅保存末尾日志信息*/
    ret = CITailLog_Init();
    assert(-1 != ret);

    CILog_Msg("system_startup:system initializing...");

    atexit(linux_cleaner);

    /*
     * 下面的信号可能导致程序异常退出，另外SIGKILL和SIGSTOP操作系统底层要求不能
     * 忽略，要想使系统正常退出，请使用SIGUSR1，或者SIGINT
     */
    CISsig_Set(SIGHUP, SIG_IGN);            /*链接断开信号屏蔽*/
    CISsig_Set(SIGPIPE, SIG_IGN);           /*管道信号屏蔽*/
    CISsig_Set(SIGPOLL, SIG_IGN);           /*轮询事件信号屏蔽*/
    CISsig_Set(SIGPROF, SIG_IGN);           /*梗概时间超时信号屏蔽*/
    CISsig_Set(SIGSTKFLT, SIG_IGN);         /*协处理器栈故障信号屏蔽*/
    CISsig_Set(SIGTSTP, SIG_IGN);           /*终端停止符信号屏蔽，会暂停进程*/
    CISsig_Set(SIGTTIN, SIG_IGN);           /*后台读控制tty信号屏蔽，会暂停进程*/
    CISsig_Set(SIGTTOU, SIG_IGN);           /*后台写控制tty信号屏蔽，会暂停进程*/
    CISsig_Set(SIGTERM, SIG_IGN);

    /*调试时可以将这两个信号打开，在实际当中为了安全，将其屏蔽掉*/
#ifdef _DEBUG
    CISsig_Set(SIGINT, (__sighandler_t)linux_sig_int);
#else
    CISsig_Set(SIGINT, SIG_IGN);             /*中断键被按下*/
    CISsig_Set(SIGQUIT,SIG_IGN);             /*停止键被按下*/
#endif /*_DEBUG*/

    CISsig_Set(SIGUSR1,(__sighandler_t)linux_sig_usr1); /*在正常工作情况下使用SIGUSR1进行中断*/

    if (0 > linux_enhance_priority()) { return -1; }
    /*配置比较全局数据初始化*/
    if (0 > CICfg_Init()) { return -1; }

    /*务必先初始化联锁运算层的数据，因为在初始平台层的时候有用到运算层的数据*/
    if (0 > CIInter_Init()) { return -1; }

    /*初始化信号*/
    if (0 > CISig_Init()) { return -1; }

    /*初始化信号时将ci_timer阻塞，以免其它不能同步运行*/
    if (0 > CISig_Block()) { return -1; }

    /*
     * 初始化配置信息，包括联锁运算层的配置信息及平台层的配置信息，配置信息读取的在前面
     * 的原因主要是因为要根据配置信息确定主从CPU，是否在模拟环境下进行等。
     */
    /*看门狗初始化*/
    if (0 > CIWatchdog_Init()) { return -1; }

    if (0 > CISwitchBoard_Init()) { return -1; }

    CISwitch_CleanCache();

    /*初始化CPU*/
    if (0 > CICpu_Init()) { return -1; }

    /*控显机初始化，控显机初始化必须在双系初始化之前完成，因为备系校核需要得到ip及端口*/
    if (0 > CIHmi_Init()) { return -1; }

    /*初始化检测机通信*/
    if (0 > CIPerformance_Init()) { return -1; }
    /*检测机通信*/
    if (0 > CIMonitor_Init()) { return -1; }
    /*
     * 初始化定时器相关，确保心跳信号在其它定时器之前被初始化，因此每次在其它定时器之前
     * 被执行，因为心跳信号内部保证了所有CPU的心跳计时器都在同一时间点上，
     * 双系心跳最好在双CPU之前完成，这样可以使周期流逝同步的时候先同步双系再同步双CPU
     */
    if (0 > CISeriesHeartbeatInt_Init()) { return -1; }
    if (0 > CICpuHeartbeatInt_init()) { return -1; }
    if (0 > CICommunicateStateInt_Init()) { return -1; }
    if (0 > CILedInt_Init()) { return -1; }
    if (0 > CIFailureInt_Init()) { return -1; }

    /*电子单元通信初始化*/
    if (0 > CIEeu_Init()) { return -1; }

    /*确定主备系*/
    if (0 > CISeries_Init()) { return -1; }

    /*确保CPU和双系初始化在状态机初始化之前，因为初始化时要CPU状态*/
    if (0 > CICycleInt_Init()) { return -1; }

    /*初始化定时器*/
    if (0 > CITimer_Init()) { return -1; }

    /*双系之间配置比较，双系比较一定在双CPU之前*/
    if (0 > CISeries_CfgSyn()) { return -1; }

    /*双CPU配置信息比较*/
    if (0 > CICpu_CfgSyn()) { return -1; }

    /*
     * 这是最后一步动作，在双CPU同步完成后即可进入状态机运行，在这里将光纤缓存里面的数据
     * 清一次，否则第一次读取上来的数据肯定hash校验码不一致
     */
    if (0 > CISeries_CleanRecvBuffer()) { return -1; }

    /*初始化成功，点亮初始化灯*/
    if (0 > CILedInt_LightInit()) { return -1; }

    /*在第一次启动的时候可能没有比较配置比较信息，在启动以后一定要打开该选项*/
    CISeries_OpenCfgCompare();

    b_initialized = CI_TRUE;

    return ret;
}
/*
功能描述    : 运行系统，对信号解除阻塞，开始接受信号，系统开始运转
返回值      : 无
参数        : 无
作者        : 张彦升
日期        : 2013年10月12日 15:28:30
*/
static void linux_run(void)
{
    CISig_UnBlock();
    CITimer_Start();        /*启动定时器*/

    CILog_Msg("now go into interlocking cycle");

    /*运行联锁周期*/
    while(CI_FALSE == CIFailureInt_BeTerminate())
    {
        CICycleInt_Once();
    }
    return;
}

CIAppOps linux_app_ops = {
    .init = linux_init,
    .run = linux_run,
    .terminate = linux_terminate,
    .decode_arguments = linux_decode_arguments,
};

#endif /*!LINUX_ENVRIONMENT*/
