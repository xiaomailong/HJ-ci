/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年10月17日 20:26:56
用途        : 联锁运算程序运行入口
历史修改记录: v1.0    创建
**********************************************************************/

#include "interlocking/inter_api.h"

void CINode_SetState(node_t UNUSED(node_index), uint32_t UNUSED(status))
{
}
uint16_t CINode_GetData(node_t UNUSED(node_index))
{
    return 0;
}
int16_t CICommand_GetButton(int16_t *UNUSED(t_pressed_button),
                                        int16_t *UNUSED(t_first_button), 
                                        int16_t *UNUSED(t_second_button),
                                        int16_t *UNUSED(t_third_button),
                                        int16_t *UNUSED(t_fourth_button))
{
    return 0;
}
int16_t CICommand_SetButton(int16_t UNUSED(t_pressed_button),
                                        int16_t UNUSED(t_first_button), 
                                        int16_t UNUSED(t_second_button),
                                        int16_t UNUSED(t_third_button),
                                        int16_t UNUSED(t_fourth_button))
{
    return 0;
}
int16_t CICommand_UpdatePressedButton(int16_t UNUSED(t_pressed_button))
{
    return 0;
}
int32_t CIInter_ClearSection(void)
{
    return 0;
}
int32_t CIInter_ClearCommands(void)
{
    return 0;
}
CI_BOOL CIInter_IsSwitch(int16_t UNUSED(node_index))
{
    return CI_TRUE;
}
CI_BOOL CIInter_IsSignal(int16_t UNUSED(node_index))
{
    return CI_TRUE;
}
CI_BOOL CIInter_IsSection(int16_t UNUSED(node_index))
{
    return CI_TRUE;
}
void CIInter_InputNodeState(uint8_t UNUSED(ga),uint8_t UNUSED(ea),uint32_t UNUSED(state))
{
}
int32_t CIInter_Calculate(void)
{
    return 0;
}
int32_t CIInter_OpenAutoTest(void)
{
    return 0;
}
int32_t CIInter_CloseAutoTest(void)
{
    return 0;
}
void CIInter_AutoTest(void)
{
}
int32_t CIInter_Init(void)
{
    return 0;
}
void CIInter_DefaultSafeProcess(uint16_t UNUSED(ga),uint16_t UNUSED(ea))
{
}
extern CpuState CICpu_GetLocalState(void)
{
    return CPU_STATE_MASTER;
}
extern SeriesState CISeries_GetLocalState(void)
{
    return SERIES_MASTER;
}
