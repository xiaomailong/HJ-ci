/***************************************************************************************
Copyright (C), 2015,  Co.Hengjun, Ltd.
文件名:special_interlocking.c
作者:  hejh
版本:  V1.0	
日期:  2015/07/27
用途:  特殊联锁模块
历史修改记录:  

***************************************************************************************/
#include "special_interlocking.h"

/****************************************************
函数名：   sepecial_interlocking
功能描述： 特殊联锁
返回值：   void
作者：	   hejh
日期：     2015/07/27
****************************************************/
void sepecial_interlocking()
{
	dk();

	dk_bgfylz65(NO_INDEX);

	dk_bgcpkz53();

}

/****************************************************
函数名：   dk
功能描述： 普通道口
返回值：   void
作者：	   hejh
日期：     2015/08/15
****************************************************/
void dk()
{
	int16_t i,j,k;
	route_t route_index = NO_INDEX;
	CI_BOOL result = CI_FALSE,add_flag = CI_TRUE;
	int16_t dk[MAX_HIGH_CROSS],dk_count = 0;

	for (i = 0; i < TOTAL_HIGH_CROSS; i++)
	{
		if (dk_config[i].AlarmIndex != NO_INDEX)
		{
			/*道口报警状态*/
			if (IsTRUE(dk_config[i].AlarmState))
			{
				/*停止报警*/
				if ((dk_config[i].StopSection != NO_INDEX)
					&& (gn_section_state(dk_config[i].StopSection) == SCS_CLEARED)
					&& (IsFALSE(is_node_locked(dk_config[i].StopSection,LT_LOCKED))))
				{
					dk_config[i].AlarmState = CI_FALSE;
				}
			}
			/*道口非报警状态*/
			else
			{
				/*检查信号机*/
				if (dk_config[i].SignalIndex != NO_INDEX)
				{
					route_index = gn_belong_route(dk_config[i].SignalIndex);
					if (route_index != NO_INDEX)
					{
						/*进路类型正确*/
						if (((dk_config[i].RouteType == RT_SHUNTING_ROUTE) && (gr_type(route_index) == RT_SHUNTING_ROUTE))
							|| ((dk_config[i].RouteType == RT_TRAIN_ROUTE) && (gr_type(route_index) == RT_TRAIN_ROUTE))
							|| (dk_config[i].RouteType == RT_ERROR))
						{
							/*报警条件*/
							if ((IsFALSE(is_signal_close(dk_config[i].SignalIndex)))
								&& (dk_config[i].StartSection != NO_INDEX)
								&& (gn_section_state(dk_config[i].StartSection) != SCS_CLEARED))
							{
								result = CI_TRUE;
								for (j = 0; j < dk_config[i].SwitchsCount; j++)
								{
									/*道岔位置符合本进路*/
									for (k = 0; k < gr_switches_count(route_index); k++)
									{
										if (dk_config[i].Switchs[j].SwitchIndex == gr_switch(route_index,k))
										{
											if (dk_config[i].Switchs[j].Location != gr_switch_location(route_index,k))
											{
												result = CI_FALSE;
												break;
											}
										}
									}
								}
								if (IsTRUE(result))
								{
									dk_config[i].AlarmState = CI_TRUE;
								}
							}
						}
					}
				}
				/*不检查信号机*/
				else
				{
					/*报警条件*/
					if ((dk_config[i].StartSection != NO_INDEX)
						&& (gn_section_state(dk_config[i].StartSection) != SCS_CLEARED)
						&& (dk_config[i].StopSection != NO_INDEX)
						&& (IsTRUE(is_node_locked(dk_config[i].StopSection,LT_LOCKED))))
					{
						result = CI_TRUE;
						route_index = gn_belong_route(dk_config[i].StopSection);
						if (route_index != NO_INDEX)
						{
							for (j = 0; j < TOTAL_HIGH_CROSS; j++)
							{
								if ((dk_config[j].SignalIndex != NO_INDEX)
									&& (dk_config[j].SignalIndex == gr_start_signal(route_index))
									&& (gn_belong_route(dk_config[j].SignalIndex) == route_index))
								{
									result = CI_FALSE;
									break;
								}
							}
						}

						if (IsTRUE(result))
						{
							for (j = 0; j < dk_config[i].SwitchsCount; j++)
							{
								/*道岔位置符合本进路*/
								for (k = 0; k < gr_switches_count(route_index); k++)
								{
									if (dk_config[i].Switchs[j].SwitchIndex == gr_switch(route_index,k))
									{
										if (dk_config[i].Switchs[j].Location != gr_switch_location(route_index,k))
										{
											result = CI_FALSE;
											break;
										}
									}
								}
							}
						}						
						if (IsTRUE(result))
						{
							dk_config[i].AlarmState = CI_TRUE;
						}
					}
				}
			}
		}		
	}

	/*统计道口输出数量*/
	for (i = 0; i < TOTAL_HIGH_CROSS; i++)
	{
		add_flag = CI_TRUE;
		for (j = 0; j < dk_count; j++)
		{
			if (dk[j] == dk_config[i].AlarmIndex)
			{
				add_flag = CI_FALSE;
				break;
			}
		}
		if (IsTRUE(add_flag))
		{
			dk[dk_count] = dk_config[i].AlarmIndex;
			dk_count++;
		}		
	}
	/*输出*/
	for (i = 0; i < dk_count; i++)
	{
		result = CI_FALSE;
		for (j = 0; j < TOTAL_HIGH_CROSS; j++)
		{
			if ((dk[i] == dk_config[j].AlarmIndex)
				&& (IsTRUE(dk_config[j].AlarmState)))
			{
				result = CI_TRUE;
				break;
			}
		}
		if (IsTRUE(result))
		{
			send_command(dk[i],SIO_NO_SIGNAL);
		}
		else
		{
			send_command(dk[i],SIO_HAS_SIGNAL);
		}
	}
}

/****************************************************
函数名：   dk_bgfylz65
功能描述： 宝钢付原料站65#道口
返回值：   void
参数：     int16_t button_index
作者：	   hejh
日期：     2015/07/27
****************************************************/
void dk_bgfylz65(int16_t button_index)
{
	if (button_index != NO_INDEX)
	{
		if (dk_bgfylz65_config.section1 != NO_INDEX)
		{
			/*按下同意按钮*/
			if (gb_node(button_index) == dk_bgfylz65_config.TYD)
			{
				send_command(dk_bgfylz65_config.TYD,SIO_HAS_SIGNAL);
				dk_bgfylz65_config.send_cycle_count = CICycleInt_GetCounter();
			}

			/*按下收权按钮*/
			if (gb_node(button_index) == dk_bgfylz65_config.SQD)
			{
				send_command(dk_bgfylz65_config.SQD,SIO_HAS_SIGNAL);
				dk_bgfylz65_config.send_cycle_count = CICycleInt_GetCounter();
			}
		}
	}
	else
	{
		if (dk_bgfylz65_config.section1 != NO_INDEX)
		{
			///*停止输出同意信息*/
			//if (gn_state(dk_bgfylz65_config.TYD) == SIO_HAS_SIGNAL)
			//{
			//	send_command(dk_bgfylz65_config.TYD,SIO_NO_SIGNAL);
			//}
			///*停止输出收权信息*/
			//if (gn_state(dk_bgfylz65_config.SQD) == SIO_HAS_SIGNAL)
			//{
			//	send_command(dk_bgfylz65_config.SQD,SIO_NO_SIGNAL);
			//}

			if (dk_bgfylz65_config.send_cycle_count != NO_TIMER)
			{
				if (CICycleInt_GetCounter() - dk_bgfylz65_config.send_cycle_count >= (4 * 1000 / CI_CYCLE_MS))
				{
					send_command(dk_bgfylz65_config.TYD,SIO_NO_SIGNAL);
					send_command(dk_bgfylz65_config.SQD,SIO_NO_SIGNAL);
					dk_bgfylz65_config.send_cycle_count = NO_TIMER;
				}
			}
		}
	}	
}

/****************************************************
函数名：   dk_bgcpkz53
功能描述： 宝钢成品库站53#道口
返回值：   void
作者：	   hejh
日期：     2015/08/13
****************************************************/
void dk_bgcpkz53()
{
	CI_BOOL result = CI_TRUE;

	if (dk_bgcpkz53_config.AlarmIndex != NO_INDEX)
	{
		/*获取信号机的状态*/
		if ((dk_bgcpkz53_config.SignalIndex != NO_INDEX)
			&& (gn_signal_state(dk_bgcpkz53_config.SignalIndex) == dk_bgcpkz53_config.SignalState))
		{
			/*获取区段1和区段2的状态*/
			if (dk_bgcpkz53_config.Section1Index != NO_INDEX)
			{
				if (dk_bgcpkz53_config.Section1State == SCS_OCCUPIED)
				{
					if (gn_section_state(dk_bgcpkz53_config.Section1Index) != SCS_CLEARED)
					{
						result = CI_FALSE;
					}
					else
					{
						if (dk_bgcpkz53_config.Section2Index != NO_INDEX)
						{
							if (dk_bgcpkz53_config.Section2State == SCS_OCCUPIED)
							{
								if (gn_section_state(dk_bgcpkz53_config.Section2Index) != SCS_CLEARED)
								{
									result = CI_FALSE;
								}
							}
							else
							{
								if (gn_section_state(dk_bgcpkz53_config.Section2Index) == dk_bgcpkz53_config.Section2State)
								{
									result = CI_FALSE;
								}
							}							
						}
					}
				}
				else
				{
					if (gn_section_state(dk_bgcpkz53_config.Section1Index) == dk_bgcpkz53_config.Section1State)
					{
						result = CI_FALSE;
					}
					else
					{
						if (dk_bgcpkz53_config.Section2Index != NO_INDEX)
						{
							if (dk_bgcpkz53_config.Section2State == SCS_OCCUPIED)
							{
								if (gn_section_state(dk_bgcpkz53_config.Section2Index) != SCS_CLEARED)
								{
									result = CI_FALSE;
								}
							}
							else
							{
								if (gn_section_state(dk_bgcpkz53_config.Section2Index) == dk_bgcpkz53_config.Section2State)
								{
									result = CI_FALSE;
								}
							}							
						}
					}
				}
				
			}
		}
		if (IsTRUE(result))
		{
			send_command(dk_bgcpkz53_config.AlarmIndex,dk_bgcpkz53_config.AlarmState);
		}
		else
		{			
			if (dk_bgcpkz53_config.AlarmState == SIO_HAS_SIGNAL)
			{
				send_command(dk_bgcpkz53_config.AlarmIndex,SIO_NO_SIGNAL);
			}
			else
			{
				send_command(dk_bgcpkz53_config.AlarmIndex,SIO_HAS_SIGNAL);
			}
		}
	}
}