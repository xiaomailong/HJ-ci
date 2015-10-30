
#include "data_struct_definition.h"
#include "utility_function.h"
#include "global_data.h"
#include "platform/platform_api.h"

/***************************************************************************************
Copyright (C), 2012,  Co.Hengjun, Ltd.
文件名:  semi_auto_block.c
作者:    CY
版本 :   1.0	
创建日期:2012/3/27
用途:    半自动闭塞模块
历史修改记录:         
***************************************************************************************/

extern semi_auto_block_t semi_auto_block_config[MAX_SEMI_AUTO_BLOCK];
/****************************************************
函数名:    gsab_state
功能描述:  获取半自动闭塞状态
返回值:    
参数:      int16_t index
作者  :    CY
日期  ：   2012/3/28
****************************************************/
EN_semi_auto_block_state gsab_state(int16_t index);

/****************************************************
函数名:    ssab_state
功能描述:  设置半自动闭塞状态
返回值:    
参数:      int16_t index
参数:      EN_semi_auto_block_state state
作者  :    CY
日期  ：   2012/3/28
****************************************************/
void ssab_state(int16_t index, EN_semi_auto_block_state state);

/****************************************************
函数名:    sab_send_positive_signal
功能描述:  发送正信号
返回值:    
参数:      int16_t index
作者  :    CY
日期  ：   2012/3/28
****************************************************/
CI_BOOL sab_send_positive_signal(int16_t index);

/****************************************************
函数名:    sab_send_negative_signal
功能描述:  发送负信号
返回值:    
参数:      int16_t index
作者  :    CY
日期  ：   2012/3/28
****************************************************/
CI_BOOL sab_send_negative_signal(int16_t index);

/****************************************************
函数名:    sab_recv_positive_info
功能描述:  接收正信号
返回值:    
参数:      int16_t index
作者  :    CY
日期  ：   2012/3/28
****************************************************/
CI_BOOL sab_recv_positive_signal(int16_t index);

/****************************************************
函数名:    sab_recv_negative_info
功能描述:  接收负信号
返回值:    
参数:      int16_t index
作者  :    CY
日期  ：   2012/3/28
****************************************************/
CI_BOOL sab_recv_negative_signal(int16_t index);

/****************************************************
函数名:    sab_out_signal_has_opened
功能描述:  判断发车站出站信号是否已开放
返回值:    
参数:      int16_t index
作者  :    CY
日期  ：   2012/3/28
****************************************************/
CI_BOOL sab_out_signal_has_opened(int16_t index);

/****************************************************
函数名:    sab_out_signal_has_closed
功能描述:  判断发车站出站信号是否已正常关闭
返回值:    
参数:      int16_t index
作者  :    CY
日期  ：   2012/3/28
****************************************************/
CI_BOOL sab_out_signal_has_closed(int16_t index);

/****************************************************
函数名:    gsab_block
功能描述:  获取半自动闭塞接发车状态
返回值:    
参数:      int16_t index
作者  :    CY
日期  ：   2012/3/28
****************************************************/
EN_semi_auto_state gsab_recieve_departure_state(int16_t index);

/****************************************************
函数名:    ssab_block
功能描述:  设置半自动闭塞接发车状态
返回值:    
参数:      int16_t index
参数:      EN_semi_auto_state state
作者  :    CY
日期  ：   2012/3/28
****************************************************/
void ssab_recieve_departure_state(int16_t index, EN_semi_auto_state state);

/****************************************************
函数名:    sab_train_arrive_approach
功能描述:  判断列车是否已到达接近区段
返回值:    
参数:      int16_t index
作者  :    CY
日期  ：   2012/3/28
****************************************************/
CI_BOOL sab_train_arrive_approach(int16_t index);

/****************************************************
函数名:    sab_train_arrive_entry_signal
功能描述:  判断列车是否已到达进站信号机内方
返回值:    
参数:      int16_t index
作者  :    CY
日期  ：   2012/3/28
****************************************************/
CI_BOOL sab_train_arrive_entry_signal(int16_t index);

/****************************************************
函数名:    sab_train_signal_opened
功能描述:  判断接车信号是否已开放
返回值:    
参数:      int16_t index
作者  :    CY
日期  ：   2012/3/28
****************************************************/
CI_BOOL sab_train_signal_opened(int16_t index);

/****************************************************
函数名:    sab_recieving_route_unlock
功能描述:  判断接车进路是否已解锁
返回值:    
参数:      int16_t index
作者  :    CY
日期  ：   2012/3/28
****************************************************/
void sab_recieving_route_unlock(int16_t index);

/****************************************************
函数名:    departure_semi_auto
功能描述:  发车站状态处理
返回值:    
参数:      int16_t i
作者  :    CY
日期  ：   2012/3/28
****************************************************/
void departure_semi_auto(int16_t i);

/****************************************************
函数名:    recieving_semi_auto
功能描述:  接车站状态处理
返回值:    
参数:      int16_t i
作者  :    CY
日期  ：   2012/3/28
****************************************************/
void recieving_semi_auto(int16_t i);

/****************************************************
函数名：   is_sab_departure_route_locked
功能描述： 发车进路是否解锁
返回值：   CI_BOOL
参数：     int16_t index
作者：	   hejh
日期：     2015/06/26
****************************************************/
CI_BOOL is_sab_departure_route_locked(int16_t index);

/****************************************************
函数名：   is_sab_train_in_entry_signal
功能描述： 判断列车完全进入信号机内方（进路内第一个道岔区段解锁）
返回值：   CI_BOOL
参数：     int16_t index
作者：	   hejh
日期：     2015/06/26
****************************************************/
CI_BOOL is_sab_train_in_entry_signal(int16_t index);

/****************************************************
函数名:    semi_auto_block_failure
功能描述:  按压半自动故障按钮后的处理过程
返回值:    
参数:      int16_t button_index
作者  :    CY
日期  ：   2012/3/28
****************************************************/
void semi_auto_block_failure(int16_t button_index)
{
	int16_t i;
	for (i = 0; i < TOTAL_SEMI_AUTO_BLOCK; i++)
	{
		if(semi_auto_block_config[i].failuer_button == button_index)
		{
			CIHmi_SendDebugTips("【%s】本站事故复原",gn_name(semi_auto_block_config[i].mem_node));
			ssab_state(i,SAB_RECOVER);
		}
	}
}

/****************************************************
函数名:    semi_auto_block_recovery
功能描述:  按压半自动复原按钮后的处理过程
返回值:    
参数:      int16_t button_index
作者  :    CY
日期  ：   2012/3/28
****************************************************/
void semi_auto_block_recovery(int16_t button_index)
{
	int16_t i;
	EN_semi_auto_block_state sab_state;
	for (i=0 ;i < TOTAL_SEMI_AUTO_BLOCK;i++)
	{
		/*复原按钮要匹配上*/
		if(semi_auto_block_config[i].cancel_button == button_index)
		{
			sab_state = gsab_state(i);
			/*发车站复原*/
			if (gsab_recieve_departure_state(i)== SAS_DEPARTURE )
			{
				if ((sab_state == SAB_REQUEST_BLOCK) ||
					( sab_state == SAB_SENT_REQUEST) || 
					(sab_state == SAB_RECIEVED_AUTO_REPLY) ||
					(sab_state == SAB_RECIEVED_AGREEMENT)
					//|| (sab_state == SAB_OUT_SIGNAL_OPENED)
					)
				{
					CIHmi_SendDebugTips("【%s】本站发车复原",gn_name(semi_auto_block_config[i].mem_node));
					ssab_state(i,SAB_RECOVER);
				}
				if (sab_state == SAB_OUT_SIGNAL_OPENED)
				{
					if (IsTRUE(is_sab_departure_route_locked(i)))
					{
						CIHmi_SendNormalTips("发车进路未解锁：%s",gn_name(semi_auto_block_config[i].mem_node));
					}
					else
					{
						CIHmi_SendDebugTips("【%s】本站发车复原",gn_name(semi_auto_block_config[i].mem_node));
						ssab_state(i,SAB_RECOVER);
					}
				}
				if ((sab_state == SAB_DEPARTED) || (sab_state == SAB_SENT_DEPARTED))
				{
					CIHmi_SendNormalTips("列车已出发：%s",gn_name(semi_auto_block_config[i].mem_node));
				}
			}
			/*接车站复原*/
			if (gsab_recieve_departure_state(i)== SAS_RECIEVING)
			{
				if ( (sab_state == SAB_RECIEVED_REQUEST) || 
					(sab_state == SAB_AGREED_RECIEVE_TRAIN)||
					(sab_state == SAB_SENDING_AGREEMENT )||
					( sab_state == SAB_SENT_AGREEMENT))
				{
					CIHmi_SendDebugTips("【%s】本站接车复原",gn_name(semi_auto_block_config[i].mem_node));
					ssab_state(i,SAB_RECOVER);
				}
				if (sab_state == SAB_RECIEVED_DEPARTED)
				{
					CIHmi_SendNormalTips("对方站列车已出发：%s",gn_name(semi_auto_block_config[i].mem_node));
				}
				if ((sab_state == SAB_TRAIN_APPROACH) || (sab_state == SAB_TRAIN_ARRIVING) || (sab_state == SAB_TRAIN_IN_ENTRY_SIGNAL))
				{
					CIHmi_SendNormalTips("列车未到达股道：%s",gn_name(semi_auto_block_config[i].mem_node));
				}
				if (sab_state == SAB_TRAIN_ARRIVED_TRACK)
				{
					CIHmi_SendDebugTips("【%s】本站解除闭塞",gn_name(semi_auto_block_config[i].mem_node));
					ssab_state(i,SAB_RECOVER);
				}
			}
			break;
		}
	}
}

/****************************************************
函数名:    semi_auto_block_block
功能描述:  按压半自动闭塞按钮后的处理过程
返回值:    
参数:      int16_t button_index
作者  :    CY
日期  ：   2012/3/28
****************************************************/
void semi_auto_block_block(int16_t button_index)
{
	int16_t i;
	EN_semi_auto_block_state sab_state;
	for (i=0 ;i < TOTAL_SEMI_AUTO_BLOCK;i++)
	{
		if(semi_auto_block_config[i].block_button == button_index)
		{
			sab_state = gsab_state(i);
			/*如果半自动时正常状态，按下闭塞按钮后表示要办理发车请求，所以转为发车站状态*/
			if ((sab_state == SAB_NORMAL) && 
				(gsab_recieve_departure_state(i)== SAS_NORMAL) )
			{
				CIHmi_SendDebugTips("【%s】本站请求闭塞",gn_name(semi_auto_block_config[i].mem_node));
				ssab_state(i,SAB_REQUEST_BLOCK);
				ssab_recieve_departure_state(i,SAS_DEPARTURE);
			}
			/*如果是收到接车请求后按下了闭塞按钮，则表示同意接车所以转为接车站状态*/
			if ((sab_state == SAB_AGREED_RECIEVE_TRAIN) &&
				(gsab_recieve_departure_state(i)== SAS_RECIEVING))
			{
				CIHmi_SendDebugTips("【%s】本站闭塞",gn_name(semi_auto_block_config[i].mem_node));
				ssab_state(i,SAB_SENDING_AGREEMENT);
			}
			break;
		}
	}
}

/****************************************************
函数名:    semi_auto_block_process
功能描述:  半自动状态管理过程
返回值:    
作者  :    CY
日期  ：   2012/3/28
****************************************************/
void semi_auto_block_process(void)
{
	int16_t i;
	for (i = 0; i < TOTAL_SEMI_AUTO_BLOCK; i++)
	{
		/*等待接收请求接车信息*/
		if ((gsab_recieve_departure_state(i)== SAS_NORMAL) && 
			IsTRUE(sab_recv_positive_signal(i)))
		{
			CIHmi_SendDebugTips("【%s】本站收到闭塞请求",gn_name(semi_auto_block_config[i].mem_node));
			ssab_state(i,SAB_RECIEVED_REQUEST);
			ssab_recieve_departure_state(i,SAS_RECIEVING);
		} 
		/*发车站处理，防止接发车站状态发生混乱*/
		if (gsab_recieve_departure_state(i)== SAS_DEPARTURE)
		{
			departure_semi_auto(i);
		}
		/*接车站处理*/
		if (gsab_recieve_departure_state(i)== SAS_RECIEVING)
		{
			recieving_semi_auto(i);
		}
		switch(gsab_state(i))
		{
			/*按压复原或事故按钮/办理事故复原*/
			case SAB_RECOVER:              	
				if (IsTRUE(sab_send_negative_signal(i)))
				{
					CIHmi_SendDebugTips("【%s】闭塞已解除",gn_name(semi_auto_block_config[i].mem_node));
					ssab_state(i,SAB_NORMAL);  /*已发送复原信息*/
				}
				break;
			case SAB_NORMAL:
				ssab_recieve_departure_state(i,SAS_NORMAL);/*半自动恢复*/				
				break;
			default:
				break;
		}
	}
}

/****************************************************
函数名:    recieving_semi_auto
功能描述:  接受半自动复原信息
返回值:  
参数:      i

作者  :   CY
日期  ：   2012/6/15
****************************************************/
void recieving_semi_auto(int16_t i)
{
	switch(gsab_state(i))
	{
		case SAB_RECIEVED_REQUEST:     /*收到(+)/对方站请求闭塞*/
			if (IsTRUE(sab_send_negative_signal(i)))/*发送(—)/自动回执信息正在发送*/
			{
				CIHmi_SendDebugTips("【%s】本站自动回执信息已发送",gn_name(semi_auto_block_config[i].mem_node));
				ssab_state(i,SAB_AGREED_RECIEVE_TRAIN);/*自动回执信息已发送*/
			}
			break;
		case SAB_AGREED_RECIEVE_TRAIN: /*等待按压闭塞按钮/本站同意接车*/
			if(IsTRUE(sab_recv_negative_signal(i))) /*收到（-）/发车站复原*/
			{
				CIHmi_SendDebugTips("【%s】对方站复原",gn_name(semi_auto_block_config[i].mem_node));
				ssab_state(i,SAB_NORMAL);
			}
			break;
		case SAB_SENDING_AGREEMENT:    
			if (IsTRUE(sab_send_positive_signal(i))) /*发送（+）/同意接车信息正在发送*/
			{
				CIHmi_SendDebugTips("【%s】本站同意接车",gn_name(semi_auto_block_config[i].mem_node));
				ssab_state(i,SAB_SENT_AGREEMENT);
			}
			break;
		case SAB_SENT_AGREEMENT:       /*同意接车信息已发送*/ 
			if(IsTRUE(sab_recv_positive_signal(i))) /*收到（+）/已收到对方站通知出发信息*/
			{
				CIHmi_SendDebugTips("【%s】对方站通知出发",gn_name(semi_auto_block_config[i].mem_node));
				ssab_state(i,SAB_TRAIN_APPROACH);
			}
			break;
		case SAB_TRAIN_APPROACH:       /*列车到达接近区段/列车接近*/  /*进站信号机开放/接车信号已开放*/
			if(IsTRUE(sab_train_arrive_approach(i)) && IsTRUE(sab_train_signal_opened(i))) 
			{
				CIHmi_SendDebugTips("【%s】列车接近，本站接车信号已开放",gn_name(semi_auto_block_config[i].mem_node));
				ssab_state(i,SAB_TRAIN_ARRIVING);
			}
			break;
		case SAB_TRAIN_ARRIVING:       /*列车进入进站信号机内方/列车已到达*/
			if(IsTRUE(sab_train_arrive_entry_signal(i))) 
			{
				CIHmi_SendDebugTips("【%s】列车进入本站进站信号机内方",gn_name(semi_auto_block_config[i].mem_node));
				ssab_state(i,SAB_TRAIN_IN_ENTRY_SIGNAL);
			}
			break;
		case SAB_TRAIN_IN_ENTRY_SIGNAL:       /*列车完全进入进站信号机内方/接车进路内方第一个道岔区段解锁*/
			if(IsTRUE(is_sab_train_in_entry_signal(i))) 
			{
				CIHmi_SendDebugTips("【%s】列车完全进入本站进站信号机内方",gn_name(semi_auto_block_config[i].mem_node));
				ssab_state(i,SAB_TRAIN_ARRIVED_TRACK);
			}
			break;
		case SAB_TRAIN_ARRIVED_TRACK:  /*接车进路已解锁/列车到达股道*/
			break;
		//case SAB_CANCEL_BLOCK:         /*拔出闭塞按钮/取消闭塞手续*/
		//	break;
		default:
			break;
	}
}

/****************************************************
函数名:    departure_semi_auto
功能描述:  发送半自动复原信息
返回值:  
参数:      i

作者  :    CY
日期  ：   2012/6/15
****************************************************/
void departure_semi_auto(int16_t i)
{
	switch(gsab_state(i))
	{
			//发车站
		case SAB_REQUEST_BLOCK:        /*按压闭塞按钮/本站请求闭塞*/      
			if (IsTRUE(sab_send_positive_signal(i)))/*发送（+）/请求闭塞信息正在发送	*/
			{
				CIHmi_SendDebugTips("【%s】本站已发送请求闭塞信息",gn_name(semi_auto_block_config[i].mem_node));
				ssab_state(i,SAB_SENT_REQUEST);/*请求闭塞信息已发送	*/
			}
			break;
		case SAB_SENT_REQUEST: /*请求闭塞信息已发送	*/        
			if (IsTRUE(sab_recv_negative_signal(i))) /*收到（—）/对方站已收到请求信息*/
			{
				CIHmi_SendDebugTips("【%s】收到对方站自动回执信息",gn_name(semi_auto_block_config[i].mem_node));
				ssab_state(i,SAB_RECIEVED_AUTO_REPLY);
			} 
			break;
		case SAB_RECIEVED_AUTO_REPLY:  /*等待接收正信号*/
			if (IsTRUE(sab_recv_positive_signal(i)))
			{
				CIHmi_SendDebugTips("【%s】对方站同意接车",gn_name(semi_auto_block_config[i].mem_node));
				ssab_state(i,SAB_RECIEVED_AGREEMENT);
			} 
			break;
		case SAB_RECIEVED_AGREEMENT:   	/*收到（+）/已收到对方站同意接车信息*/
			if (IsTRUE(sab_out_signal_has_opened(i)))
			{
				CIHmi_SendDebugTips("【%s】本站出站信号机已开放",gn_name(semi_auto_block_config[i].mem_node));
				ssab_state(i,SAB_OUT_SIGNAL_OPENED);
			} 
			break;
		case SAB_OUT_SIGNAL_OPENED:    /*出站信号机已开放/出站信号已开放*/
			if (IsTRUE(sab_out_signal_has_closed(i)))
			{
				CIHmi_SendDebugTips("【%s】本站出站信号机已关闭",gn_name(semi_auto_block_config[i].mem_node));
				ssab_state(i,SAB_DEPARTED);
			} 
			break;
		case SAB_DEPARTED:             /*出站信号机正常自动关闭/本站已发车*/   	
			if (IsTRUE(sab_send_positive_signal(i)))/*发送（+）/正在发送本站通知出发信息*/
			{
				CIHmi_SendDebugTips("【%s】本站已发送通知出发信息",gn_name(semi_auto_block_config[i].mem_node));
				ssab_state(i,SAB_SENT_DEPARTED);
			}
			break;
		case SAB_SENT_DEPARTED:        /*已发送本站通知出发信息*/
			if (IsTRUE(sab_recv_negative_signal(i)))
			{
				CIHmi_SendDebugTips("【%s】对方站解除闭塞",gn_name(semi_auto_block_config[i].mem_node));
				ssab_state(i,SAB_RECIEVED_CANCEL);
			} 
			break;
		case SAB_RECIEVED_CANCEL:      /*接收到解除闭塞信息/已收到解除闭塞信息*/
			ssab_state(i,SAB_NORMAL);
			break;
		default:
			break;
	}
}

/****************************************************
 函数名:    gsab_state
 功能描述:  获取半自动闭塞状态
 返回值:    
 参数:      int16_t index
 作者  :    CY
 日期  ：   2012/3/28
 ****************************************************/
 EN_semi_auto_block_state gsab_state(int16_t index)
{
	return semi_auto_block_config[index].state;
}

/****************************************************
函数名:    ssab_state
功能描述:  设置半自动闭塞状态
返回值:    
参数:      int16_t index
参数:      EN_semi_auto_block_state state
作者  :    CY
日期  ：   2012/3/28
****************************************************/
void ssab_state(int16_t index, EN_semi_auto_block_state state)
{
	int16_t mem_node = semi_auto_block_config[index].mem_node;
	semi_auto_block_config[index].state = state;
	sn_state(mem_node,state);
}

/****************************************************
函数名:    sab_send_positive_signal
功能描述:  发送正信号
返回值:    
参数:      int16_t index
作者  :    CY
日期  ：   2012/3/28
****************************************************/
CI_BOOL sab_send_positive_signal(int16_t index)
{
	CI_BOOL result = CI_FALSE;
	if (semi_auto_block_config[index].send_cycle_count == NO_TIMER )
	{
		/*还没开始发送正电*/
		semi_auto_block_config[index].send_cycle_count = CICycleInt_GetCounter();		
		sn_state(semi_auto_block_config[index].ZJ,SIO_HAS_SIGNAL);
		sn_state(semi_auto_block_config[index].FJ,SIO_NO_SIGNAL);
		send_command(semi_auto_block_config[index].ZJ,SIO_HAS_SIGNAL);
		CIHmi_SendDebugTips("【%s】开始发送正电信息",gn_name(semi_auto_block_config[index].mem_node));
	}
	else
	{
		/*正电发送过程计时*/
		if (CICycleInt_GetCounter() - semi_auto_block_config[index].send_cycle_count >= SEMI_AUTO_SEND_TIME )
		{
			semi_auto_block_config[index].send_cycle_count = NO_TIMER;
			sn_state(semi_auto_block_config[index].ZJ,SIO_NO_SIGNAL);
			send_command(semi_auto_block_config[index].ZJ,SIO_NO_SIGNAL);			
			result = CI_TRUE;
			CIHmi_SendDebugTips("【%s】正电信息发送完毕",gn_name(semi_auto_block_config[index].mem_node));
		}
		else
		{
			sn_state(semi_auto_block_config[index].ZJ,SIO_HAS_SIGNAL);
			send_command(semi_auto_block_config[index].ZJ,SIO_HAS_SIGNAL);	
			//CIHmi_SendDebugTips("正在发送正电信息",gn_name(semi_auto_block_config[index].mem_node));
		}
	}
	return result;
}



/****************************************************
函数名:    sab_send_negative_signal
功能描述:  发送负信号
返回值:    
参数:      int16_t index
作者  :    CY
日期  ：   2012/3/28
****************************************************/
CI_BOOL sab_send_negative_signal(int16_t index)
{
	CI_BOOL result = CI_FALSE;
	if (semi_auto_block_config[index].send_cycle_count == NO_TIMER )
	{
		/*开始发送负电*/
		semi_auto_block_config[index].send_cycle_count = CICycleInt_GetCounter();
		sn_state(semi_auto_block_config[index].ZJ,SIO_NO_SIGNAL);
		sn_state(semi_auto_block_config[index].FJ,SIO_HAS_SIGNAL);
		send_command(semi_auto_block_config[index].FJ,SIO_HAS_SIGNAL);	
		CIHmi_SendDebugTips("【%s】开始发送负电信息",gn_name(semi_auto_block_config[index].mem_node));
	}
	else
	{
		/*负电发送过程计时*/
		if (CICycleInt_GetCounter() - semi_auto_block_config[index].send_cycle_count >= SEMI_AUTO_SEND_TIME )
		{
			semi_auto_block_config[index].send_cycle_count = NO_TIMER;
			sn_state(semi_auto_block_config[index].FJ,SIO_NO_SIGNAL);
			send_command(semi_auto_block_config[index].FJ,SIO_NO_SIGNAL);			
			result = CI_TRUE;
			CIHmi_SendDebugTips("【%s】负电信息发送完毕",gn_name(semi_auto_block_config[index].mem_node));
		}
		else
		{
			sn_state(semi_auto_block_config[index].FJ,SIO_HAS_SIGNAL);
			send_command(semi_auto_block_config[index].FJ,SIO_HAS_SIGNAL);		
			//CIHmi_SendDebugTips("正在发送负电信息",gn_name(semi_auto_block_config[index].mem_node));
		}
	}
	return result;
}

/****************************************************
函数名:    sab_recv_positive_info
功能描述:  接收正信号
返回值:    
参数:      int16_t index
作者  :    CY
日期  ：   2012/3/28
****************************************************/
CI_BOOL sab_recv_positive_signal(int16_t index)
{
	CI_BOOL result = CI_FALSE;
	if (semi_auto_block_config[index].ZD_recv_cycle_count == NO_TIMER )
	{		
		if (gn_state(semi_auto_block_config[index].ZJ) == SIO_HAS_SIGNAL)
		{
			/*收到正电*/
			semi_auto_block_config[index].ZD_recv_cycle_count = CICycleInt_GetCounter();
			CIHmi_SendDebugTips("【%s】收到正电信息",gn_name(semi_auto_block_config[index].mem_node));
		}
	}
	else
	{
		//if (CICycleInt_GetCounter() - semi_auto_block_config[index].recv_cycle_count >= SEMI_AUTO_RECV_TIME )
		//{
		//	/*正电接受的时间达到要求*/
		//	//semi_auto_block_config[index].recv_cycle_count = NO_TIMER;
		//	if (gn_state(semi_auto_block_config[index].ZJ) == SIO_HAS_SIGNAL)
		//	{				
		//		//result = CI_TRUE;
		//	}
		//}
		//else
		//{
		//	/*直到接收不到信息时认为接收结束*/
		//	if (gn_state(semi_auto_block_config[index].ZJ) == SIO_NO_SIGNAL)
		//	{				
		//		semi_auto_block_config[index].recv_cycle_count = NO_TIMER;
		//		result = CI_TRUE;
		//		PRINTF("正电信息接收完毕");
		//	}
		//}
		/*直到接收不到信息时认为接收结束*/
		if (gn_state(semi_auto_block_config[index].ZJ) != SIO_HAS_SIGNAL)
		{				
			semi_auto_block_config[index].ZD_recv_cycle_count = NO_TIMER;
			result = CI_TRUE;
			CIHmi_SendDebugTips("【%s】正电信息接收完毕",gn_name(semi_auto_block_config[index].mem_node));
		}
	}
	return result;
}

/****************************************************
函数名:    sab_recv_negative_info
功能描述:  接收负信号
返回值:    
参数:      int16_t index
作者  :    CY
日期  ：   2012/3/28
****************************************************/
CI_BOOL sab_recv_negative_signal(int16_t index)
{
	CI_BOOL result = CI_FALSE;
	if (semi_auto_block_config[index].FD_recv_cycle_count == NO_TIMER )
	{		
		if (gn_state(semi_auto_block_config[index].FJ) == SIO_HAS_SIGNAL)
		{
			/*开始接收负电*/
			semi_auto_block_config[index].FD_recv_cycle_count = CICycleInt_GetCounter();
			CIHmi_SendDebugTips("【%s】收到负电信息",gn_name(semi_auto_block_config[index].mem_node));
		}
	}
	else
	{
		/*负电接收过程计时*/
		//if (CICycleInt_GetCounter() - semi_auto_block_config[index].recv_cycle_count >= SEMI_AUTO_RECV_TIME )
		//{
		//	//semi_auto_block_config[index].recv_cycle_count = NO_TIMER;
		//	if (gn_state(semi_auto_block_config[index].FJ) == SIO_HAS_SIGNAL)
		//	{			
		//		/*接收负电结束*/
		//		//result = CI_TRUE;
		//	}
		//}
		//else
		//{
		//	/*直到接收不到信息时认为接收结束*/
		//	if (gn_state(semi_auto_block_config[index].FJ) == SIO_NO_SIGNAL)
		//	{				
		//		semi_auto_block_config[index].recv_cycle_count = NO_TIMER;
		//		result = CI_TRUE;
		//		PRINTF("负电信息接收完毕");
		//	}
		//}
		/*直到接收不到信息时认为接收结束*/
		if (gn_state(semi_auto_block_config[index].FJ) != SIO_HAS_SIGNAL)
		{				
			semi_auto_block_config[index].FD_recv_cycle_count = NO_TIMER;
			result = CI_TRUE;
			CIHmi_SendDebugTips("【%s】负电信息接收完毕",gn_name(semi_auto_block_config[index].mem_node));
		}
	}
	return result;
}

/****************************************************
函数名:    sab_out_signal_has_opened
功能描述:  判断发车站出站信号是否已开放
返回值:    
参数:      int16_t index
作者  :    CY
日期  ：   2012/3/28
****************************************************/
CI_BOOL sab_out_signal_has_opened(int16_t index)
{
	CI_BOOL result = CI_FALSE;

	if (semi_auto_block_config[index].belong_route != NO_INDEX )
	{
		if (gr_state(semi_auto_block_config[index].belong_route) == RS_SIGNAL_OPENED)
		{
			result = CI_TRUE;
		}		
	}
	return result;
}
 /****************************************************
 函数名:    set_semi_relate_route
 功能描述:  设置半自动关联的进路
 返回值:    
 参数:      int16_t semi_auto_index
 参数:      int16_t route_index
 作者  :    CY
 日期  ：   2012/7/31
 ****************************************************/
 void set_semi_relate_route(int16_t semi_auto_index, route_t route_index)
 {
	 semi_auto_block_config[semi_auto_index].belong_route = route_index;
 }
/****************************************************
函数名:    sab_out_signal_has_closed
功能描述:  判断发车站出站信号是否已正常关闭
返回值:    
参数:      int16_t index
作者  :    CY
日期  ：   2012/3/28
****************************************************/
CI_BOOL sab_out_signal_has_closed(int16_t index)
{
	CI_BOOL result = CI_FALSE;

	if (semi_auto_block_config[index].belong_route != NO_INDEX )
	{
		/*if (gr_state(semi_auto_block_config[index].belong_route) == RS_SK_N_SIGNAL_CLOSED)
		{
			result = CI_TRUE;
		}*/

		/*压入发车进路的最后一个区段*/
		if ((gr_state(semi_auto_block_config[index].belong_route) == RS_SK_N_SIGNAL_CLOSED)
			|| (gr_state(semi_auto_block_config[index].belong_route) == RS_AUTOMATIC_UNLOCKING)
			|| (gr_state(semi_auto_block_config[index].belong_route) == RS_AUTO_UNLOCK_FAILURE))
		{
			if (gn_section_state(gr_last_section(semi_auto_block_config[index].belong_route)) != SCS_CLEARED)
			{
				result = CI_TRUE;
			}
		}
		if (gr_state(semi_auto_block_config[index].belong_route) != RS_A_SIGNAL_CLOSED)
		{
			CIHmi_SendDebugTips("半自动进路状态：%d",gr_state(semi_auto_block_config[index].belong_route));
			if (gn_section_state(gr_last_section(semi_auto_block_config[index].belong_route)) != SCS_CLEARED)
			{
				result = CI_TRUE;
			}
		}
	}
	return result;
}

/****************************************************
函数名:    gsab_block
功能描述:  获取半自动闭塞接发车状态
返回值:    
参数:      int16_t index
作者  :    CY
日期  ：   2012/3/28
****************************************************/
EN_semi_auto_state gsab_recieve_departure_state(int16_t index)
{
	return semi_auto_block_config[index].block_state;
}

/****************************************************
函数名:    ssab_block
功能描述:  设置半自动闭塞接发车状态
返回值:    
参数:      int16_t index
参数:      EN_semi_auto_state state
作者  :    CY
日期  ：   2012/3/28
****************************************************/
void ssab_recieve_departure_state(int16_t index, EN_semi_auto_state state)
{
//	int16_t mem_node = semi_auto_block_config[index].mem_node;

	semi_auto_block_config[index].block_state = state;	
	//sn_state(mem_node, state);
}

/****************************************************
函数名:    sab_train_arrive_approach
功能描述:  判断列车是否已到达接近区段
返回值:    
参数:      int16_t index
作者  :    CY
日期  ：   2012/3/28
****************************************************/
CI_BOOL sab_train_arrive_approach(int16_t index)
{
	CI_BOOL result = CI_FALSE;
	if (semi_auto_block_config[index].belong_route != NO_INDEX )
	{
		if (IsFALSE(is_approach_cleared(semi_auto_block_config[index].belong_route,CI_TRUE)))
		{
			result = CI_TRUE;
		}		
	}
	return result;
}

/****************************************************
函数名:    sab_train_arrive_entry_signal
功能描述:  判断列车是否已到达进站信号机内方
返回值:    
参数:      int16_t index
作者  :    CY
日期  ：   2012/3/28
****************************************************/
CI_BOOL sab_train_arrive_entry_signal(int16_t index)
{
	CI_BOOL result = CI_FALSE;
	if (semi_auto_block_config[index].belong_route != NO_INDEX )
	{
		if (gr_state(semi_auto_block_config[index].belong_route) == RS_SK_N_SIGNAL_CLOSED)
		{
			result = CI_TRUE;
		}		
	}
	return result;
}

/****************************************************
函数名:    sab_train_signal_opened
功能描述:  判断接车信号是否已开放
返回值:    
参数:      int16_t index
作者  :    CY
日期  ：   2012/3/28
****************************************************/
CI_BOOL sab_train_signal_opened(int16_t index)
{
	CI_BOOL result = CI_FALSE;
	if (semi_auto_block_config[index].belong_route != NO_INDEX )
	{
		if (gr_state(semi_auto_block_config[index].belong_route) == RS_SIGNAL_OPENED)
		{
			result = CI_TRUE;
		}		
	}
	return result;
}

/****************************************************
函数名:    sab_recieving_route_unlock
功能描述:  判断接车进路是否已解锁
返回值:    
参数:      int16_t index
作者  :    CY
日期  ：   2012/3/28
****************************************************/
void sab_recieving_route_unlock(int16_t index)
{
	if (gsab_recieve_departure_state(index)== SAS_RECIEVING && gsab_state(index)==SAB_TRAIN_ARRIVED_TRACK)
	{
		/*hjh 2015-6-26 屏蔽，否则会半自动接车自动复原*/
		//ssab_state(index,SAB_RECOVER);
	}
}

/****************************************************
函数名：   is_sab_departure_route_locked
功能描述： 发车进路是否解锁
返回值：   CI_BOOL
参数：     int16_t index
作者：	   hejh
日期：     2015/06/26
****************************************************/
CI_BOOL is_sab_departure_route_locked(int16_t index)
{
	CI_BOOL result = CI_FALSE;

	if (semi_auto_block_config[index].belong_route != NO_INDEX )
	{
		result = CI_TRUE;
	}
	return result;
}

/****************************************************
函数名：   is_sab_train_in_entry_signal
功能描述： 判断列车完全进入信号机内方（进路内第一个道岔区段解锁）
返回值：   CI_BOOL
参数：     int16_t index
作者：	   hejh
日期：     2015/06/26
****************************************************/
CI_BOOL is_sab_train_in_entry_signal(int16_t index)
{
	CI_BOOL result = CI_FALSE;
	node_t first_switch_section = NO_INDEX;
	int16_t i;
	route_t route_index = semi_auto_block_config[index].belong_route;

	if (route_index != NO_INDEX )
	{
		for (i = 0; i < gr_nodes_count(route_index); i++)
		{
			first_switch_section = gr_node(route_index,i);
			if ((gn_type(first_switch_section) == NT_SWITCH_SECTION)
				&& (IsFALSE(is_node_locked(first_switch_section,LT_LOCKED))))
			{
				result = CI_TRUE;
				break;
			}
		}
	}
	return result;
}
