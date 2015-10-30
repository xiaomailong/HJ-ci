/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年12月3日 14:46:38
用途        : 电子单元通信
历史修改记录: v1.0   创建
             v1.1   添加windows下模拟(模拟机)逻辑
**********************************************************************/
#ifdef WIN32
/*WIN32_LEAN_AND_MEAN能够避免编译时的重定义错误问题*/
#ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib,"WS2_32.lib")

#include "interlocking/inter_api.h"
#include "util/algorithms.h"
#include "util/config.h"
#include "util/utility.h"
#include "util/log.h"

#include "eeu_manage_win.h"

static int32_t simulator_port = 0;          /*模拟机的端口地址*/

static SOCKET ci_simulator_sock;
static struct sockaddr_in simulator_addr;
/*
 功能描述    : 为了与linux下保持统一，在windows下该函数为空
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年6月30日 11:07:20
*/
void CIEeu_SetDataMask(uint32_t ga, uint32_t ea)
{
    return;
}
/*
功能描述    : 发送节点命令
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2013年12月13日 13:21:34
*/
void CIEeu_SendNodeIndexCmd(int16_t node_index,uint32_t cmd)
{
    static uint16_t sn = 0;
    static EeuFrame frame = {0};
    int32_t write_num = 0;

    frame.head = 0xa155;
    frame.frame_sn = sn++;
    frame.node_index = node_index;
    frame.cmd = cmd;
    frame.hash = CIAlgorithm_Crc16(&frame,sizeof(frame) - sizeof(frame.hash));

    write_num = sendto(ci_simulator_sock,
                        (const char*)&frame,
                        sizeof(frame),
                        0,
                        (struct sockaddr *)&simulator_addr,
                        sizeof(simulator_addr));
    if (0 >= write_num)
    {
        CILog_Errno("向模拟机发送数据失败");
        return;
    }
    return;
}
/*
功能描述    : 从数据缓冲区中读取一帧数据
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2013年12月13日 13:27:05
*/
int32_t CIEeu_ReceiveStatusData()
{
    EeuFrame frame = {0};
    int32_t read_num;
    uint16_t hash = 0;

    while(1)
    {
        read_num = recvfrom(ci_simulator_sock, (char*)&frame,sizeof(frame),0,NULL,NULL);

        if (0 >= read_num)
        {
            /*CILog_Msg("从模拟机读取数据失败:%d",WSAGetLastError());*/

            return -1;
        }
        /*收到数据之后检查帧头*/
        if (frame.head != 0xa155)
        {
            CILog_Msg("模拟机帧头不正确%#X != 0xA155",frame.head);

            continue;
        }
        hash = CIAlgorithm_Crc16(&frame,sizeof(frame) - sizeof(frame.hash));
        if (hash != frame.hash)
        {
            CILog_Msg("模拟机校验码不正确New(%#X) != old(%#X)",hash,frame.head);

            continue;
        }
        /*本应该在这里校验一次crc的，但是控显机没有计算crc，所以先不判断*/
        CINode_SetState(frame.node_index,frame.cmd);
    }

    return 0;
}
/*
 功能描述    : 得到A通信道的状态
 返回值      : 通信道正常则返回CI_TRUE,否则返回CI_FALSE
 参数        : 无
 日期        : 2015年5月29日 18:30:00
*/
CI_BOOL CIEeu_IsChannelAOk(void)
{
    return CI_TRUE;
}
/*
 功能描述    : 得到B通信道的状态
 返回值      : 通信道正常则返回CI_TRUE,否则返回CI_FALSE
 参数        : 无
 日期        : 2015年5月29日 18:30:00
*/
CI_BOOL CIEeu_IsChannelBOk(void)
{
    return CI_TRUE;
}
/*
功能描述     : 设置是否要记录电子单元数据
返回值       : 
参数         : 
作者         : 何境泰
日期         : 2014年7月14日 10:53:08
*/
void CIEeu_SetRecord (CI_BOOL is_record)
{
	return;
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
    int ret = 0;
    WSADATA wsa_data;
    WORD sock_version = MAKEWORD(2,2);
    unsigned long ul = 1;                       /*1 block,0 nonblock*/
    const char* ci_ip = NULL;                   /*本机地址*/
    const char* ci_port_str = NULL;
    int32_t ci_simulator_listen_port = 0;             /*本机与模拟机通信的端口*/

    const char* simulator_ip_str = NULL;        /*模拟机的地址*/
    const char* simulator_port_str = NULL;

    struct sockaddr_in ci_serv_addr;

    if (CI_TRUE == b_initialized)
    {
        return -1;
    }

    if(0 != WSAStartup(sock_version, &wsa_data))
    {
        return 0;
    }

    /***********************Simulator server config***********************/
    /*get simulator server ip*/
    simulator_ip_str = CIConfig_GetValue("SimulatorServerIp");
    if (NULL == simulator_ip_str)
    {
        CILog_Msg("配置文件当中找不到SimulatorServerIp项");
        return -1;
    }
    if (CI_FALSE == CI_ValidateIpAddress(simulator_ip_str))
    {
        CILog_Msg("配置文件当中SimulatorServerIp项值不符合ip规范");
        return -1;
    }
    /*get simulator port*/
    /*
    simulator_port_str = CIConfig_GetValue("SimulatorServerListenPort");
    if (NULL == simulator_port_str)
    {
        CILog_Msg("配置文件当中找不到SimulatorServerListenPort项");
        return -1;
    }
    simulator_port = atoi(simulator_port_str);
    if (CI_FALSE == CI_ValidatePort(simulator_port))
    {
        CILog_Msg("在配置文件当中SimulatorServerListenPort值不符合规范");
        return -1;
    }
    */

    /*模拟机端口地址在代码中写死，因：配置文件太过杂乱*/
    simulator_port = 3003;
    /**********************CI config read*********************************/
    /*ci ip address*/
    ci_ip = CIConfig_GetValue("CiLocalhostIp");
    if (NULL == ci_ip)
    {
        CILog_Msg("无法得到本地ip地址，请检查CiLocalhostIp配置项");
        return -1;
    }
    if (CI_FALSE == CI_ValidateIpAddress(ci_ip))
    {
        CILog_Msg("配置文件当中CiLocalhostIp项值不符合ip规范");
        return -1;
    }
    /*si simulator port*/
    /*
    ci_port_str = CIConfig_GetValue("CiSimulatorListenPort");
    if (NULL == ci_port_str)
    {
        CILog_Msg("配置文件当中找不到CiSimulatorListenPort项");
        return -1;
    }
    ci_simulator_listen_port = atoi(ci_port_str);
    if (CI_FALSE == CI_ValidatePort(ci_simulator_listen_port))
    {
        CILog_Msg("在配置文件当中CiSimulatorListenPort值不符合规范");
        return -1;
    }
    */
    ci_simulator_listen_port = 3002;

    /*init socket*/

    ci_simulator_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(INVALID_SOCKET == ci_simulator_sock)
    {
        CILog_Msg("socket() failed:%d\n", WSAGetLastError());
        return -1;
    }

    ci_serv_addr.sin_addr.S_un.S_addr = inet_addr(ci_ip);;
    ci_serv_addr.sin_family=AF_INET;
    ci_serv_addr.sin_port=htons((uint16_t)ci_simulator_listen_port);

    simulator_addr.sin_addr.S_un.S_addr = inet_addr(simulator_ip_str);;
    simulator_addr.sin_family = AF_INET;
    simulator_addr.sin_port = htons((uint16_t)simulator_port);

    if(SOCKET_ERROR == bind(ci_simulator_sock, (struct sockaddr *)&ci_serv_addr, sizeof(ci_serv_addr)))
    {
        CILog_Errno("bind failed");
        closesocket(ci_simulator_sock);
        return -1;
    }

    /*设置成非阻塞模式*/
    ret=ioctlsocket(ci_simulator_sock,FIONBIO,(unsigned long *)&ul);
    if(SOCKET_ERROR == ret)
    {
        CILog_Errno("set non block failed");
        closesocket(ci_simulator_sock);
        return -1;
    }

    b_initialized = CI_TRUE;

    return 0;
}

#endif /*WIN32*/
