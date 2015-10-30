/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年11月4日 9:56:42
用途        : 双系切换板通信模块
历史修改记录: v1.0    创建
**********************************************************************/

#ifndef _switch_board_manage_h__
#define _switch_board_manage_h__

#ifdef __cplusplus
extern "C" {
#endif

	
typedef struct _SwitchCiFrame
{
    uint16_t frame_head_tag;             /*帧头标志*/
    uint32_t sn;                         /*帧编号*/
    uint8_t  series_state;               /*联锁机到切换板的双系状态信息*/
    uint16_t hash;                       /*hash值*/
}SwitchCiFrame;

typedef struct _SwitchBoardFrame
{
    uint16_t frame_head_tag;             /*帧头标志*/
    uint32_t sn;                         /*帧编号*/
    uint8_t  series_state;               /*联锁机到切换板的双系状态信息*/
    uint8_t  switch_cmd;                 /*切换板到联锁机的切换命令*/
    uint16_t hash;                       /*hash值*/
}SwitchBoardFrame;

/*
 功能描述    : 强制进行切换
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年9月24日 15:23:24
*/
CIAPI_FUNC(int32_t) CISwitch_LetSwitch();
/*
 功能描述    : 清楚切换标志
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年9月24日 15:23:24
*/
CIAPI_FUNC(int32_t) CISwitch_SwitchOver();
/*
 功能描述    : 清除切换板接收缓存当中的数据
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年9月23日 13:15:57
*/
CIAPI_FUNC(int32_t) CISwitch_CleanCache(void);
/*
 功能描述    : 判断切换
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年9月22日 10:58:06
*/
CIAPI_FUNC(CI_BOOL) CISwitch_CouldSwitch(void);
/*
 功能描述    : 用以判断主系是否存活
 返回值      : 若主系存活则返回真，否则为假
 参数        : 无
 日期        : 2015年5月29日 20:01:06
*/
CIAPI_FUNC(CI_BOOL) CISwitch_PeerAlive(void);
/*
 功能描述    : 产生一个切换命令
 返回值      : 无
 参数        : 无
 日期        : 2015年7月17日 21:00:43
*/
CI_BOOL CISwitch_ProduceSwitchCmd(void);
/*
 功能描述    : 是否有切换命令
 返回值      : 有则返回CI_TRUE，否则返回CI_FALSE
 参数        : 无
 日期        : 2015年7月17日 21:00:43
*/
CIAPI_FUNC(CI_BOOL) CISwitch_ConsumeSwitchCmd(void);
/*
 功能描述    : 该函数预留给双系启动阶段判断另外一系是否已经启动。
              当双系通信道断开，备系启动时因为备系收不到心跳信号会进入主系，此时
              通过该函数确认一次
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年6月3日 15:40:32
*/
CIAPI_FUNC(CI_BOOL) CISwitch_TestMasterAlive(void);
/*
 功能描述    : 切换板初始化
 返回值      : 成功为0，失败为-1
 参数        : 无
 作者        : 张彦升
 日期        : 2014年9月22日 10:58:36
*/
CIAPI_FUNC(int32_t) CISwitchBoard_Init(void);

#ifdef __cplusplus
}
#endif

#endif /*_switch_board_manage_h__*/
