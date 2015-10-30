/*********************************************************************
 Copyright (C), 2014,  Co.Hengjun, Ltd.

 作者        : 张彦升
 版本        : 1.0
 创建日期    : 2014年6月17日 12:43:07
 用途        : windows下的项目管理
 历史修改记录: v1.0    创建
**********************************************************************/

#ifdef WIN32

#include "app.h"
#include "hmi_manage.h"
#include "eeu_manage_win.h"
#include "local_log.h"

#include "interlocking/inter_api.h"
#include "util/log.h"

static uint32_t cycle_counter = 0;
/*
功能描述    : 获取联锁周期数
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2013年11月4日 9:39:22
*/
extern uint32_t CICycleInt_GetCounter(void)
{
    /*返回周期计数值*/
    return cycle_counter;
}
/*
 功能描述    : 解析参数，由于Windows下不像win经常会在终端中执行程序，所以该函数暂不用
             实现
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年4月11日 13:05:30
*/
static int win_decode_arguments (int argc, char **argv)
{
    return 0;
}
/*
功能描述    : 对系统进行初始化
返回值      : 0为成功，-1为失败
参数        : 
作者        : 张彦升
日期        : 2013年10月12日 15:28:17
*/
static int32_t win_init(void)
{
    int32_t ret = 0;
    static CI_BOOL b_initialized = CI_FALSE;

    if (CI_TRUE == b_initialized)
    {
        return -1;
    }
    SetErrorMode(SEM_NOGPFAULTERRORBOX | SEM_FAILCRITICALERRORS);

    /*
     * 使用日志文件方式记录，在linux下经常使用一个参数控制是否使用日志方式进行记录，而在
     * Windows下使用vs具有很大的便利性，所以在确保要使用日志方式记录后请取消该函数的注释，
     * 相应的日志文件会生成在./src目录下
     */
    /*初始化日志记录方式*/
    ret = CILocalLog_Init();
    assert(-1 != ret);

    /*务必先初始化联锁运算层的数据，因为在初始平台层的时候有用到运算层的数据*/
    ret = CIInter_Init();
    assert(-1 != ret);

    /*控显机初始化，控显机初始化必须在双系初始化之前完成，因为备系校核需要得到ip及端口*/
    ret = CIHmi_Init();
    assert(-1 != ret);

    ret = CIEeu_Init();
    assert(-1 != ret);

    b_initialized = CI_TRUE;

    return ret;
}
/*
功能描述    : 运行系统，对信号解除阻塞，开始接受信号，系统开始运转
返回值      : 
参数        : 
作者        : 张彦升
日期        : 2013年10月12日 15:28:30
*/
static void win_run(void)
{
    int32_t ret = 0;

    CILog_Msg("CI is running!");

    /*运行联锁周期*/
    while(CI_TRUE)
    {
        /*从控显机接受数据*/
        ret = CIHmi_RecvData();
        ret = CIEeu_ReceiveStatusData();

        ret = CIInter_Calculate();

        CIInter_ClearSection();                 /*清理系统模块*/
        CIHmi_SendNodeData();                   /*向控显机发送节点信息*/
        CIHmi_WriteCachePush();

        Sleep(CI_CYCLE_MS); /*延迟CI_CYCLE_MS ms*/
        cycle_counter ++;
    }
    return;
}
/*
功能描述    : 终止程序
返回值      : 
参数        : 
作者        : 张彦升
日期        : 2013年10月12日 13:50:59
*/
static void win_terminate(void)
{
    return;
}

CIAppOps win_app_ops = {
    win_init,
    win_run,
    win_terminate,
    win_decode_arguments,
};

#endif /*WIN32*/
