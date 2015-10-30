/***************************************************************************************
Copyright (C), 2015,  Co.Hengjun, Ltd.
文件名:special_interlocking.c
作者:  hejh
版本:  V1.0	
日期:  2015/07/27
用途:  特殊联锁模块
历史修改记录:  

***************************************************************************************/
#ifndef SPECIAL_INTERLOCKING
#define SPECIAL_INTERLOCKING

#include "global_data.h"
#include "utility_function.h"
#include "error_process.h"
#include "util/ci_header.h"
#include "base_type_definition.h"
#include "keep_signal.h"

/****************************************************
函数名：   sepecial_interlocking
功能描述： 特殊联锁
返回值：   void
作者：	   hejh
日期：     2015/07/27
****************************************************/
void sepecial_interlocking();

/****************************************************
函数名：   dk
功能描述： 普通道口
返回值：   void
作者：	   hejh
日期：     2015/08/15
****************************************************/
void dk();

/****************************************************
函数名：   dk_bgfylz65
功能描述： 宝钢付原料站65#道口
返回值：   void
参数：     int16_t button_index
作者：	   hejh
日期：     2015/07/27
****************************************************/
void dk_bgfylz65(int16_t button_index);











#endif