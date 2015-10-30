/***************************************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.
文件名:  process_command.c
作者:   LYC
版本 :   1.2	
创建日期:2011/11/29
用途:    命令处理模块
历史修改记录:  
2012/7/20,V1.1，LYC:
	1.增加了按下初始化系统功能按钮1和2后，初始化系统的功能；
	2.修改了提示信息。根据实际情况将错误信息，警告信息，提示信息正确分类处理；修改了部分Bug
	3.解决了进路搜索算法中的Bug，搜索进路时把无岔区段和股道包含进去增加了发送当前按下的按钮的功能
	4.修改命令处理模块中列车按钮与列车终端按钮之间办理不了进路的BUG
	5.修改命令处理模块列车按钮和列车终端按钮之间办理变更进路的BUG
2012/8/14,V1.2，LYC:
	1.增加了命令处理函数建立进路的判断条件
	2.修改check_ci_condition中的错误提示信息，keep_signal中的BUG
	3.修改了#48、#52中记录的BUG
	4.修改了和引导进路相关的BUG
	5.修改改方运行时离去区段占用时信号不关闭的问题，列车进路始端信号断丝时的信号降级显示问题，
	  通过进路的接车进路和发车进路在显示上要有联系
2012/12/7 V1.2.1 hjh
	1.is_build_route_command中增加以调车信号机处的列车按钮为终端的进路办理及始端的基本进路办理
	2.is_three_button_build_route中增加以调车信号机处的列车按钮为始端的变更进路办理
2013/1/6 V1.2.1 hjh
	增加输入错误按钮时的提示错误信息功能
2013/2/28 V1.2.1 LYC
	增加了配置是否允许跨咽喉调车的代码
2013/4/12 V1.2.1 LYC
	增加了功能按钮错误防护功能
2013/7/18 V1.2.1 LYC
	修改了is_three_button_build_route函数：增加了延续进路的终端按钮是调车终端按钮和延续进路终端按钮时可以办理进路的情况
2013/8/2 V1.2.1 LYC
	修改了is_four_button_build_route函数：增加了变更延续进路的终端按钮是调车终端按钮和延续进路终端按钮时可以办理进路的情况
2013/9/3 V1.2.1 LYC 
	修改了function_button_choose函数：增加了调用引总锁闭和引总解锁模块时,检查第二个按钮所在信号点是否是道岔的条件  
2014/5/23 V1.2.3 LYC 
	function_button_choose增加了取消机务段同意功能
***************************************************************************************/

#include "process_command.h"
#include "global_data.h"
#include "semi_auto_block.h"
#include "throat_lock.h"
#include "switch_control.h"
#include "cancel_route.h"
#include "relay_cancel_route.h"
#include "section_unlock.h"
#include "auto_unlock.h"
#include "guide_route.h"
#include "open_signal.h"
#include "init_manage.h"
#include "hold_route_shunting.h"
#include "special_interlocking.h"

string function_button_name[]=
{	
	"FB_NO_BUTTON",			   
	"FB_STOP_OPERATION",		   
	"FB_CANCEL_ROUTE",		   
	"FB_HUMAN_UNLOCK",		   
	"FB_REOPEN_SIGNAL",		   
	"FB_SECTION_UNLOCK",        
	"FB_GUIDE_ROUTE",		   
	"FB_THROAT_LOCK",		   
	"FB_THROAT_UNLOCK",		   
	"FB_SWITCH_LOCK",		   
	"FB_SWITCH_UNLOCK",		   
	"FB_SWITCH_CLOSE",		   
	"FB_SWITCH_UNCLOSE",       
	"FB_SWITCH_NORMAL",		   
	"FB_SWITCH_REVERSE",	   
	"FB_CLOSE_SIGNAL",		   
	"FB_SUCCESSIVE_UNLOCK",	 
	"FB_BUTTON_CLOSE",
	"FB_HOLD_ROUTE",
	"FB_HOLD_ROUTE_FAULT",
	"FB_SWITCH_LOCAL_CONTROL", 
	"FB_SEMI_AUTO_BLOCK",      
	"FB_SEMI_AUTO_CANCEL",     
	"FB_SEMI_AUTO_FAIL",  
	"FB_SYSTEM_MENU",
	"FB_INIT_DATA1",
	"FB_INIT_DATA2",
    NULL,
};
CI_TIMER press_button_time = 0;


/****************************************************
函数名：   process_command
功能描述： 控显机下发整条进路命令后执行过程
返回值：   void
作者：	   hejh
日期：     2015/06/09
****************************************************/
void process_command()
{
	/*判断接收到按钮按下事件*/
	if (NO_INDEX != first_button)
	{
		/*功能按钮*/
		if ((first_button < 0) &&( first_button > FB_INIT_DATA1))
		{
			CIHmi_SendDebugTips("first_button :%s",function_button_name[-1-first_button]);
			/*终止操作*/
			if (FB_STOP_OPERATION == first_button)
			{			
				//CIHmi_SendNormalTips("终止操作");
				clean_button_log();
			}
			/*其他功能操作*/
			else
			{
				if ((second_button != NO_INDEX) && (second_button >= 0) && (second_button < TOTAL_SIGNAL_NODE + TOTAL_BUTTONS))
				{
					CIHmi_SendDebugTips("second_button:%s",device_name[second_button]);
					function_button_choose();
					clean_button_log();
				}
				else
				{
					CIHmi_SendNormalTips("错误办理：%s",gn_name(first_button));
					/*清除按钮记录*/
					clean_button_log();	
				}
			}
		}
		/*进路按钮*/
		if ((first_button >= TOTAL_SIGNAL_NODE) && (first_button < TOTAL_SIGNAL_NODE + TOTAL_BUTTONS))
		{
			/*一个按钮*/
			if (second_button == NO_INDEX)
			{
				CIHmi_SendDebugTips("first_button :%s",device_name[first_button]);
				single_button_command();
				if (first_button != NO_INDEX)
				{
					CIHmi_SendNormalTips("错误办理：%s",gn_name(first_button));
					/*清除按钮记录*/
					clean_button_log();	
				}
			}
			/*两个按钮*/
			if ((second_button != NO_INDEX) && (third_button == NO_INDEX))
			{
				CIHmi_SendDebugTips("按钮:%s,%s",device_name[first_button],device_name[second_button]);
				if (first_button != second_button)
				{
					if (IsTRUE(is_build_route_command()))
					{
						/*设置进路建立命令*/
						can_build_route = CI_TRUE;
					} 
					else
					{
						/*清除按钮记录*/
						clean_button_log();	
						process_warning(ERR_NO_ROUTE,"");
					}
				}
				else
				{
					CIHmi_SendNormalTips("错误办理：%s,%s",device_name[first_button],device_name[second_button]);
					/*清除按钮记录*/
					clean_button_log();	
				}				
			}
			/*三个按钮*/
			if ((second_button != NO_INDEX) && (third_button != NO_INDEX) && (fourth_button == NO_INDEX))
			{
				CIHmi_SendDebugTips("按钮:%s,%s,%s",device_name[first_button],
													device_name[second_button],
													device_name[third_button]);
				if ((first_button != second_button)
					&& (third_button != second_button)
					&& (first_button != third_button))
				{
					if (IsTRUE(is_three_button_build_route()))
					{
						/*设置进路建立命令*/
						can_build_route = CI_TRUE;
					} 
					else
					{
						/*清除按钮记录*/
						clean_button_log();	
						process_warning(ERR_NO_ROUTE,"");
					}
				}
				else
				{
					CIHmi_SendNormalTips("错误办理：%s,%s,%s",device_name[first_button],
						device_name[second_button],
						device_name[third_button]);
				}
			}
			/*四个按钮*/
			if ((second_button != NO_INDEX) && (third_button != NO_INDEX) && (fourth_button != NO_INDEX) && (fifth_button == NO_INDEX))
			{
				CIHmi_SendDebugTips("按钮:%s,%s,%s,%s",device_name[first_button],
													   device_name[second_button],
													   device_name[third_button],
													   device_name[fourth_button]);
				if (IsTRUE(is_four_button_build_route()))
				{
					/*设置进路建立命令*/
					can_build_route = CI_TRUE;
				} 
				else
				{
					/*清除按钮记录*/
					clean_button_log();	
					process_warning(ERR_NO_ROUTE,"");
				}				
			}
			/*五个按钮*/
			if ((second_button != NO_INDEX) && (third_button != NO_INDEX) && (fourth_button != NO_INDEX) && (fifth_button != NO_INDEX))
			{
				CIHmi_SendDebugTips("按钮:%s,%s,%s,%s,%s",device_name[first_button],
														  device_name[second_button],
														  device_name[third_button],
														  device_name[fourth_button],
														  device_name[fifth_button]);
				if (IsTRUE(is_five_button_build_route()))
				{
					/*设置进路建立命令*/
					can_build_route = CI_TRUE;
				} 
				else
				{
					/*清除按钮记录*/
					clean_button_log();	
					process_warning(ERR_NO_ROUTE,"");
				}
			}
		}
	}
}

/****************************************************
函数名:    process_command
功能描述:  根据按钮命令执行相应的操作
返回值:    
参数:    
作者  :    LYC
日期  ：   2011/11/29
****************************************************/
void process_command1 (void)
{
	CI_BOOL effective_flag = CI_FALSE;

	FUNCTION_IN;

	/*判断按下按钮超过10s后则清除按钮*/
	if ((CICycleInt_GetCounter()- press_button_time) >= (SECONDS_10 / CI_CYCLE_MS))
	{
		clean_button_log();
	}

	/*判断接收到按钮按下事件*/
	if (FB_NO_BUTTON != pressed_button)
	{
		/*引用全局变量参数检查*/
		if ((pressed_button < 0) &&( pressed_button > FB_INIT_DATA1))
		{
			CIHmi_SendDebugTips("pressed_button:%s",function_button_name[-1-pressed_button]);
			effective_flag = CI_TRUE;
		} 
		else
		{			
			if ((pressed_button >= 0) && (pressed_button < TOTAL_SIGNAL_NODE))
			{
				/*其他按钮*/
				if ((NT_CHANGE_BUTTON == gn_type(pressed_button)) || (NT_TRAIN_END_BUTTON == gn_type(pressed_button)) 
					|| (NT_SUCCESSIVE_END_BUTTON == gn_type(pressed_button)) || (NT_SHUNTING_END_BUTTON == gn_type(pressed_button)))
				{
					effective_flag = CI_TRUE;
				}
				/*道岔和区段,引总锁闭按钮*/
				if ((NT_SWITCH == gn_type(pressed_button)) || (NT_TRACK == gn_type(pressed_button)) || (NT_NON_SWITCH_SECTION == gn_type(pressed_button)) 
					|| (NT_SWITCH_SECTION == gn_type(pressed_button)) || (NT_STUB_END_SECTION == gn_type(pressed_button))
					|| (NT_LOCODEPOT_SECTION == gn_type(pressed_button)) || (NT_THROAT_GUIDE_LOCK == gn_type(pressed_button)))
				{
					if (FB_NO_BUTTON > first_button)
					{
						effective_flag = CI_TRUE;
					}
				}
			}
			/*按钮*/
			if ((pressed_button >= TOTAL_SIGNAL_NODE) && (pressed_button < TOTAL_NAMES))
			{
				effective_flag = CI_TRUE;
			}
			if (IsTRUE(effective_flag))
			{
				/*引用全局变量范围检查*/
				if ((pressed_button >= 0) && (pressed_button < TOTAL_NAMES))
				{
					CIHmi_SendDebugTips("pressed_button:%s",device_name[pressed_button]);
				}
			}			
		}

		/*判断接收到的按钮是否终止操作按钮*/
		if (FB_STOP_OPERATION == pressed_button)
		{			
			clean_button_log();
			CIHmi_SendNormalTips("终止操作");
		} 
		else
		{
			if (IsTRUE(effective_flag))
			{
				/*记录当前按钮被按下的周期数*/
				press_button_time = CICycleInt_GetCounter();
				/*判断是否未记录第一个按钮*/
				if (FB_NO_BUTTON == first_button || pressed_button < 0)						
				{	
					clean_button_log();
					first_button = pressed_button;
					/*判断接收的第一个按钮可以形成命令*/
					single_button_command();			
				} 
				else
				{
					/*判断未记录第二个按钮*/
					if (FB_NO_BUTTON == second_button)
					{				
						second_button = pressed_button;
						/*判断当前命令是否需要第三个按钮*/
						if (IsFALSE(need_third_button()))
						{
							/*判断命令按钮是否功能按钮 */
							if (first_button <= FB_CANCEL_ROUTE)
							{						
								function_button_choose();						
								clean_button_log();
							} 						
							/*判断命令按钮是否建立进路按钮*/
							else if (IsTRUE(is_build_route_command()))
							{							
								can_build_route = CI_TRUE;								
							}
							else
							{
								clean_button_log();
								process_warning(ERR_NO_ROUTE,"");
							}
						}					
					} 
					else
					{
						/*第三个按钮处理*/
						process_third_button();
					}
				}
			}										
		}
		/*发送数据*/
		send_pressed_buttons();
	}
	/*清除当前按钮记录*/
	pressed_button = FB_NO_BUTTON;		
	FUNCTION_OUT;
}
/****************************************************
函数名:    single_button_command
功能描述:  判断已接收到的第一个按钮是否直接形成命令，
		如果形成命令，直接调用相应的命令模块。
返回值:		
作者  :    LYC
日期  ：   2011/11/29
****************************************************/
void single_button_command(void)
{
	FUNCTION_IN;
	/*选择单按钮操作命令列表 */	
	switch(gb_type(first_button) )
	{
		case BT_REQUEST : /*请求按钮*/

			clean_button_log();
			break;
		case BT_INPUTOUTPUT : /*输入输出对象处的按钮*/
			dk_bgfylz65(first_button);




			clean_button_log();
			break;	
		case BT_JCBJ:/*挤岔报警*/
			CIHmi_SendNormalTips("挤岔报警确认：%s",gn_name(first_button));
			SwitchNoindicateAlarm(first_button);
			clean_button_log();
			break;
		case BT_SEMIAUTO_BLOCK : /*半自动闭塞按钮*/
			CIHmi_SendNormalTips("半自动闭塞：%s",gn_name(first_button));
			semi_auto_block_block(first_button);
			clean_button_log();
			break;					   	
		case BT_SEMIAUTO_CANCEL : /*半自动复原按钮*/
			CIHmi_SendNormalTips("半自动复原：%s",gn_name(first_button));
			semi_auto_block_recovery(first_button);
			clean_button_log();
			break;
		case BT_SEMIAUTO_FAILURE : /*半自动事故按钮*/
			CIHmi_SendNormalTips("半自动事故：%s",gn_name(first_button));
			semi_auto_block_failure(first_button);
			clean_button_log();
			break;
		default :/*没有满足类型则直接退出函数*/
			; 
	}
	FUNCTION_OUT;
}

/****************************************************
函数名:    is_build_route_command
功能描述:  判断已接收到的两个按钮是否可以构成建立
		 进路的命令。
返回值:  如果可以建立进路返回真，无法建立进路返回假
作者  :    LYC
日期  ：   2011/11/29
****************************************************/
CI_BOOL is_build_route_command(void)
{	
	int16_t a = -1,b = -1;
	CI_BOOL result = CI_FALSE;	/*定义返回值**/	

	FUNCTION_IN;
	a = gb_node(first_button);
	b = gb_node(second_button);
	/*可以建立进路情况列表**/
	/*第一个按钮是通过按钮，第二个按钮是进站信号机或列车终端按钮或进路信号机的列车按钮或通过按钮**/
	if ((BT_PASSING == gb_type(first_button))		
		&& ((NT_ENTRY_SIGNAL == gn_type(b) 
		|| BT_TRAIN_END == gb_type(second_button)) 
		|| (NT_ROUTE_SIGNAL == gn_type(b) && BT_TRAIN == gb_type(second_button))
		|| (BT_PASSING == gb_type(second_button)))
		)
	{
		if (gb_type(first_button) == BT_PASSING)
		{
			if (gb_node(first_button) != NO_INDEX)
			{
				first_button = signal_nodes[gb_node(first_button)].buttons[TRAIN_BUTTON_INDEX];
			}
		}
		if (gb_type(second_button) == BT_PASSING)
		{
			if (gb_node(second_button) != NO_INDEX)
			{
				second_button = signal_nodes[gb_node(second_button)].buttons[TRAIN_BUTTON_INDEX];
			}			
		}
		result = CI_TRUE;
		passing_route = CI_TRUE;
		CIHmi_SendNormalTips("通过进路：%s --> %s",
			gn_name(gb_node(first_button)),
			gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
	}
	/*hjh 第一个按钮是进路信号机的列车按钮，
	第二个按钮是进站信号机的列车按钮,办理通过进路*/
	if ((IsFALSE(result))
		&&(NT_ROUTE_SIGNAL == gn_type(a)) 
		&& (BT_TRAIN == gb_type(first_button))		
		&& (NT_ENTRY_SIGNAL == gn_type(b))
		&& (BT_TRAIN == gb_type(second_button)))
	{
		result = CI_TRUE;
		passing_route = CI_TRUE;
		CIHmi_SendNormalTips("通过进路：%s --> %s",
			gn_name(gb_node(first_button)),
			gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
	}
	/*以下进路，需配置是否允许跨咽喉办理*/
	if (gn_throat(a) == gn_throat(b))
	{
		/*第一个按钮是进站信号机或进路信号机的列车按钮，
		第二个按钮是出站信号机或出站兼调车信号机的列车按钮**/
		if((IsFALSE(result))
			&&((NT_ENTRY_SIGNAL == gn_type(a)) || (NT_ROUTE_SIGNAL == gn_type(a)))
			&& (BT_TRAIN == gb_type(first_button))		
			&& (NT_OUT_SIGNAL == gn_type(b) || NT_OUT_SHUNTING_SIGNAL == gn_type(b))
			&& (BT_TRAIN == gb_type(second_button))
			)
		{
			result = CI_TRUE;
			CIHmi_SendNormalTips("列车进路：%s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
		}
		/*第一个按钮是出站信号机或出站兼调车信号机的列车按钮，
		第二个按钮是进站信号机或进路信号机的列车按钮**/
		if((IsFALSE(result))
			&&(NT_OUT_SIGNAL == gn_type(a) || NT_OUT_SHUNTING_SIGNAL == gn_type(a))
			&& (BT_TRAIN == gb_type(first_button))		
			&& (NT_ENTRY_SIGNAL == gn_type(b) || (NT_ROUTE_SIGNAL == gn_type(b)))
			&& (BT_TRAIN == gb_type(second_button)))
		{
			result = CI_TRUE;
			CIHmi_SendNormalTips("列车进路：%s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
		}
		/*两个按钮是进路信号机的列车按钮*/
		if ((IsFALSE(result))
			&&(NT_ROUTE_SIGNAL == gn_type(a)) 
			&& (BT_TRAIN == gb_type(first_button))		
			&& (NT_ROUTE_SIGNAL == gn_type(b))
			&& (BT_TRAIN == gb_type(second_button)))
		{
			result = CI_TRUE;
			CIHmi_SendNormalTips("列车进路：%s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
		}

		/*第一个是列车按钮，第二个是列车终端按钮**/
		if ((IsFALSE(result))
			&&(BT_TRAIN == gb_type(first_button))
			&& (second_button > TOTAL_SIGNAL_NODE) && (BT_TRAIN == gb_type(second_button)))
		{
			result = CI_TRUE;
			CIHmi_SendNormalTips("列车进路：%s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
		}
		/*hjh 第一个是调车信号机的列车按钮，第二个是出站信号机或出站兼调车的列车按钮*/
		if ((IsFALSE(result))
			&&(NT_DIFF_SHUNTING_SIGNAL == gn_type(a)) && (BT_TRAIN == gb_type(first_button))
			&& (NT_ROUTE_SIGNAL == gn_type(b) || NT_OUT_SIGNAL == gn_type(b) || NT_OUT_SHUNTING_SIGNAL == gn_type(b))
			&& (BT_TRAIN == gb_type(second_button)))
		{
			result = CI_TRUE;
			CIHmi_SendNormalTips("列车进路：%s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
		}
		/*第一个是进路信号机或出站信号机或出站兼调车的列车按钮，第二个是调车信号机的列车按钮*/
		if ((IsFALSE(result))
			&&(NT_ROUTE_SIGNAL == gn_type(a) || NT_OUT_SIGNAL == gn_type(a) || NT_OUT_SHUNTING_SIGNAL == gn_type(a))
			&& (BT_TRAIN == gb_type(first_button))
			&& (NT_DIFF_SHUNTING_SIGNAL == gn_type(b)) && (BT_TRAIN == gb_type(second_button)))
		{
			result = CI_TRUE;
			CIHmi_SendNormalTips("列车进路：%s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
		}
		/*第一个是驼峰信号机，第二个是调车按钮*/
		if ((IsFALSE(result))
			&&(NT_HUMPING_SIGNAL == gn_type(a)) 
			&& (BT_SHUNTING == gb_type(first_button))		
			&& (NT_SINGLE_SHUNTING_SIGNAL == gn_type(b) || NT_DIFF_SHUNTING_SIGNAL == gn_type(b)
			|| NT_JUXTAPOSE_SHUNGTING_SIGNAL == gn_type(b) || NT_STUB_END_SHUNTING_SIGNAL == gn_type(b))
			&& (BT_SHUNTING == gb_type(second_button)))
		{
			result = CI_TRUE;
			CIHmi_SendNormalTips("调车进路：%s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
		}
		/*第一个是调车按钮，第二个是驼峰信号机*/
		if ((IsFALSE(result))
			&&(NT_SINGLE_SHUNTING_SIGNAL == gn_type(a) || NT_DIFF_SHUNTING_SIGNAL == gn_type(a)
			|| NT_JUXTAPOSE_SHUNGTING_SIGNAL == gn_type(a) || NT_STUB_END_SHUNTING_SIGNAL == gn_type(a)) 
			&& (BT_SHUNTING == gb_type(first_button))		
			&& (NT_HUMPING_SIGNAL == gn_type(b))
			&& (BT_SHUNTING == gb_type(second_button)))
		{
			result = CI_TRUE;
			CIHmi_SendNormalTips("调车进路：%s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
		}
		/*第一个按钮是调车按钮，第二个按钮是反方向的差置信号机 **/		
		if((IsFALSE(result))
			&&(BT_SHUNTING == gb_type(first_button)) 
			&& (gn_direction(a) != gn_direction(b))
			&& (NT_DIFF_SHUNTING_SIGNAL == gn_type(b)))
		{
			result = CI_TRUE;
			CIHmi_SendNormalTips("调车进路：%s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
		}
			/*第一个按钮是调车按钮，第二个按钮是反方向的并置调车信号机**/
		if((IsFALSE(result))
			&&(BT_SHUNTING == gb_type(first_button)) 
			&& (gn_direction(a) != gn_direction(b))
			&& (NT_JUXTAPOSE_SHUNGTING_SIGNAL == gn_type(b)))
		{
			result = CI_TRUE;
			CIHmi_SendNormalTips("调车进路：%s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
		}
		/*第一个按钮是调车按钮，第二个按钮是尽头式调车信号机或机务段调车信号机**/
		if((IsFALSE(result))
			&&(BT_SHUNTING == gb_type(first_button)) 			
			&& ((NT_STUB_END_SHUNTING_SIGNAL == gn_type(b)) || (NT_LOCODEPOT_SHUNTING_SIGNAL == gn_type(b))))
		{
			result = CI_TRUE;
			CIHmi_SendNormalTips("调车进路：%s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
		}
		/*第一个按钮是调车按钮，第二个按钮是同方向的单置调车信号机 **/
		if((IsFALSE(result))
			&&(BT_SHUNTING == gb_type(first_button)) 	
			&& (gn_direction(a) == gn_direction(b))
			&& (NT_SINGLE_SHUNTING_SIGNAL == gn_type(b)))
		{
			result = CI_TRUE;
			CIHmi_SendNormalTips("调车进路：%s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
		}
		/*第一个按钮是调车按钮，第二个按钮是出站兼调车信号机或进路信号机的调车按钮 **/
		if((IsFALSE(result))
			&&(BT_SHUNTING == gb_type(first_button)) 	
			&& (NT_OUT_SHUNTING_SIGNAL == gn_type(b) || NT_ROUTE_SIGNAL == gn_type(b))
			&& (BT_SHUNTING == gb_type(second_button)))
		{
			result = CI_TRUE;
			CIHmi_SendNormalTips("调车进路：%s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
		}	
		/*第一个按钮是调车按钮，第二个按钮是调车终端按钮**/
		if ((IsFALSE(result))
			&&(BT_SHUNTING == gb_type(first_button))
			&& (NT_SHUNTING_END_BUTTON == gn_type(b)))
		{
			result = CI_TRUE;
			CIHmi_SendNormalTips("调车进路：%s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
		}
		/*第一个按钮是调车按钮，第二个按钮是调车按钮**/
		if ((IsFALSE(result))
			&&(BT_SHUNTING == gb_type(first_button))
			&& (BT_SHUNTING == gb_type(second_button)))
		{
			result = CI_TRUE;
			CIHmi_SendNormalTips("调车进路：%s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
		}
	} 
	/*错误操作1*/
	if ((((BT_TRAIN == gb_type(first_button)) || (BT_PASSING == gb_type(first_button))) && ((NT_SHUNTING_END_BUTTON == gn_type(b)) || (NT_STUB_END_SHUNTING_SIGNAL == gn_type(b)) || (NT_LOCODEPOT_SHUNTING_SIGNAL == gn_type(b)) || (NT_SUCCESSIVE_END_BUTTON == gn_type(b))))
		||((BT_SHUNTING == gb_type(first_button)) && (gn_direction(a) == gn_direction(b))&& (NT_DIFF_SHUNTING_SIGNAL == gn_type(b)))
		||((BT_SHUNTING == gb_type(first_button)) && (gn_direction(a) == gn_direction(b))&& (NT_JUXTAPOSE_SHUNGTING_SIGNAL == gn_type(b)))
		||((BT_SHUNTING == gb_type(first_button)) && (BT_PASSING == gb_type(second_button))))
	{		
		//process_warning(ERR_NO_ROUTE,"");
		//CIHmi_SendNormalTips("错误办理：%s --> %s",
		//	gn_name(gb_node(first_button)),
		//	gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
		///*清除按钮*/
		//clean_button_log();
	}
	FUNCTION_OUT;
	return result;
}

/****************************************************
函数名:    is_three_button_build_route
功能描述:  判断三按钮变更进路建立条件
返回值:		如果可以建立进路返回真，无法建立进路返回假
作者  :    LYC
日期  ：   2011/12/1
****************************************************/
CI_BOOL  is_three_button_build_route(void)
{
	CI_BOOL result = CI_FALSE,result_dt = CI_FALSE;
	int16_t a = -1,b = -1,c = -1;

	FUNCTION_IN;	
	a = gb_node(first_button);
	b = gb_node(second_button);
	c = gb_node(third_button);
	/*可以建立进路情况列表**/
	/*判断可能为延续进路**/
	/*第一个按钮是进站或进路信号机的列车按钮，
	  第二个按钮是同一咽喉的出站信号机或出站兼调车信号机的列车按钮或列车终端按钮
	  第三个按钮是不在同一咽喉的进站或进路信号机或调车按钮或调车终端按钮或延续进路终端按钮*/
	if((NT_ENTRY_SIGNAL == gn_type(a) || NT_ROUTE_SIGNAL == gn_type(a))
		&& (BT_TRAIN == gb_type(first_button))
		&& ((NT_OUT_SIGNAL == gn_type(b)) || (NT_OUT_SHUNTING_SIGNAL == gn_type(b)) || (NT_TRAIN_END_BUTTON == gn_type(b)))
		&& (BT_TRAIN == gb_type(second_button))
		&& ((NT_ENTRY_SIGNAL == gn_type(c)) || (NT_ROUTE_SIGNAL == gn_type(c))|| (NT_STUB_END_SHUNTING_SIGNAL == gn_type(c)) 
			|| (NT_LOCODEPOT_SHUNTING_SIGNAL == gn_type(c)) || (NT_SHUNTING_END_BUTTON == gn_type(c)) || (NT_SUCCESSIVE_END_BUTTON == gn_type(c)))
		&& (gn_throat(a) == gn_throat(b))
		&& (gn_throat(b) != gn_throat(c)))
	{
		result = CI_TRUE;
		CIHmi_SendNormalTips("延续进路：%s --> %s --> %s",
			gn_name(gb_node(first_button)),
			gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)),
			gb_node(third_button) == NO_INDEX ? gn_name(third_button) : gn_name(gb_node(third_button)));
	}
	/*以下进路，需配置是否允许跨咽喉办理，目前为不允许跨咽喉办理*/
	if (gn_throat(a) == gn_throat(c))
	{
		/*第一个按钮是进站信号机或进路信号机的列车按钮，
		第二个按钮是变通按钮或调车按钮，第三个按钮是出站信号机**/
		if((IsFALSE(result))
			&&(NT_ENTRY_SIGNAL == gn_type(a) || NT_ROUTE_SIGNAL == gn_type(a))
			&& (BT_TRAIN == gb_type(first_button))
			&& ((NT_CHANGE_BUTTON == gn_type(b)) || (BT_SHUNTING == gb_type(second_button)))
			&& (NT_OUT_SIGNAL == gn_type(c)))
		{
			result = CI_TRUE;
			CIHmi_SendNormalTips("列车进路：%s --> %s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)),
				gb_node(third_button) == NO_INDEX ? gn_name(third_button) : gn_name(gb_node(third_button)));
		}
		/*第三个按钮是进站信号机或进路信号机的列车按钮，
		第二个按钮是变通按钮或调车按钮,第一个按钮是出站信号机**/
		if((IsFALSE(result))
			&&(NT_ENTRY_SIGNAL == gn_type(c) || NT_ROUTE_SIGNAL == gn_type(c))
			&& (BT_TRAIN == gb_type(third_button))
			&& ((NT_CHANGE_BUTTON == gn_type(b)) || (BT_SHUNTING == gb_type(second_button)))		
			&& (NT_OUT_SIGNAL == gn_type(a)))
		{
			result = CI_TRUE;			
			CIHmi_SendNormalTips("列车进路：%s --> %s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)),
				gb_node(third_button) == NO_INDEX ? gn_name(third_button) : gn_name(gb_node(third_button)));
		}
		/*第一个按钮是进站信号机或进路信号机的列车按钮，
		第二个按钮是变通按钮或调车按钮,第三按钮是出站兼调车信号机的列车按钮**/
		if((IsFALSE(result))
			&&(NT_ENTRY_SIGNAL == gn_type(a) || NT_ROUTE_SIGNAL == gn_type(a))
			&& (BT_TRAIN == gb_type(first_button))
			&& ((NT_CHANGE_BUTTON == gn_type(b)) || (BT_SHUNTING == gb_type(second_button)))		
			&& (NT_OUT_SHUNTING_SIGNAL == gn_type(c))
			&& (BT_TRAIN == gb_type(third_button)))
		{
			result = CI_TRUE;
			CIHmi_SendNormalTips("列车进路：%s --> %s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)),
				gb_node(third_button) == NO_INDEX ? gn_name(third_button) : gn_name(gb_node(third_button)));
		}
		/*第三个按钮是进站信号机或进路信号机的列车按钮，
		第二个按钮是变通按钮或调车按钮,第一按钮是出站兼调车信号机的列车按钮**/
		if((IsFALSE(result))
			&&(NT_ENTRY_SIGNAL == gn_type(c) || NT_ROUTE_SIGNAL == gn_type(c))
			&& (BT_TRAIN == gb_type(third_button))
			&& ((NT_CHANGE_BUTTON == gn_type(b)) || (BT_SHUNTING == gb_type(second_button)))		
			&& (NT_OUT_SHUNTING_SIGNAL == gn_type(a))
			&& (BT_TRAIN == gb_type(first_button)))
		{
			result = CI_TRUE;
			CIHmi_SendNormalTips("列车进路：%s --> %s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)),
				gb_node(third_button) == NO_INDEX ? gn_name(third_button) : gn_name(gb_node(third_button)));
		}
		/*第一个按钮是进路信号机的列车按钮，
		第二个按钮是变通按钮或调车按钮，第三个按钮是进路信号机的列车按钮*/
		if ((IsFALSE(result))
			&&(NT_ROUTE_SIGNAL == gn_type(a)) 
			&& (BT_TRAIN == gb_type(first_button))		
			&&((NT_CHANGE_BUTTON == gn_type(b)) || (BT_SHUNTING == gb_type(second_button)))		
			&& (NT_ROUTE_SIGNAL == gn_type(c))
			&& (BT_TRAIN == gb_type(third_button)))
		{
			result = CI_TRUE;
			CIHmi_SendNormalTips("列车进路：%s --> %s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)),
				gb_node(third_button) == NO_INDEX ? gn_name(third_button) : gn_name(gb_node(third_button)));
		}
		/*第一个是列车按钮，第二个是变通按钮或调车按钮，第三个按钮是列车终端按钮**/
		if ((IsFALSE(result))
			&&(BT_TRAIN == gb_type(first_button))
			&&((NT_CHANGE_BUTTON == gn_type(b)) || (BT_SHUNTING == gb_type(second_button)))
			&& (third_button >= 0) && (third_button < (TOTAL_SIGNAL_NODE + TOTAL_BUTTONS))
			&& (BT_TRAIN == gb_type(third_button)))
		{
			result = CI_TRUE;
			CIHmi_SendNormalTips("列车进路：%s --> %s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)),
				gb_node(third_button) == NO_INDEX ? gn_name(third_button) : gn_name(gb_node(third_button)));
		}
		/*第一个是调车信号机的列车按钮，第二个是出站信号机或出站兼调车的列车按钮*/
		if ((IsFALSE(result))
			&&(NT_DIFF_SHUNTING_SIGNAL == gn_type(a)) && (BT_TRAIN == gb_type(first_button))
			&&((NT_CHANGE_BUTTON == gn_type(b)) || (BT_SHUNTING == gb_type(second_button)))
			&& (NT_ROUTE_SIGNAL == gn_type(c) || NT_OUT_SIGNAL == gn_type(c) || NT_OUT_SHUNTING_SIGNAL == gn_type(c))
			&& (BT_TRAIN == gb_type(third_button)))
		{
			result = CI_TRUE;
			CIHmi_SendNormalTips("列车进路：%s --> %s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)),
				gb_node(third_button) == NO_INDEX ? gn_name(third_button) : gn_name(gb_node(third_button)));
		}
		/*判断第二个按钮所在的信号点是否第一个按钮所在信号机的反方向单置信号机*/
		if ((IsFALSE(result))
			&&(gn_direction(a) != gn_direction(b)) && (NT_SINGLE_SHUNTING_SIGNAL == gn_type(b)))
		{
			/*返回真则是反方向的单置信号机，假则不是*/
			result_dt = CI_TRUE;
		}
		/*第一个按钮是调车按钮，第二个按钮是变通按钮或反方向单置信号机,第三个按钮是反方向的差置信号机 **/
		if((IsFALSE(result))
			&&(BT_SHUNTING == gb_type(first_button)) 
			&&(NT_CHANGE_BUTTON == gn_type(b) || IsTRUE(result_dt))
			&& (gn_direction(a) != gn_direction(c))
			&& (NT_DIFF_SHUNTING_SIGNAL == gn_type(c)))
		{
			result = CI_TRUE;
			CIHmi_SendNormalTips("调车进路：%s --> %s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)),
				gb_node(third_button) == NO_INDEX ? gn_name(third_button) : gn_name(gb_node(third_button)));
		}
		/*第一个按钮是调车按钮，第二个按钮是变通按钮或反方向单置信号机,第三个按钮是反方向的并置调车信号机**/
		if((IsFALSE(result))
			&&(BT_SHUNTING == gb_type(first_button))
			&&(NT_CHANGE_BUTTON == gn_type(b) || IsTRUE(result_dt))
			&& (gn_direction(a) != gn_direction(c))
			&& (NT_JUXTAPOSE_SHUNGTING_SIGNAL == gn_type(c)))
		{
			result = CI_TRUE;
			CIHmi_SendNormalTips("调车进路：%s --> %s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)),
				gb_node(third_button) == NO_INDEX ? gn_name(third_button) : gn_name(gb_node(third_button)));
		}
		/*第一个按钮是调车按钮，第二个按钮是变通按钮或反方向单置信号机,第三个按钮是尽头式调车信号机或机务段调车信号机**/
		if((IsFALSE(result))
			&&(BT_SHUNTING == gb_type(first_button))
			&&(NT_CHANGE_BUTTON == gn_type(b) || IsTRUE(result_dt))
			&& ((NT_STUB_END_SHUNTING_SIGNAL == gn_type(c)) || (NT_LOCODEPOT_SHUNTING_SIGNAL == gn_type(c))))
		{
			result = CI_TRUE;
			CIHmi_SendNormalTips("调车进路：%s --> %s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)),
				gb_node(third_button) == NO_INDEX ? gn_name(third_button) : gn_name(gb_node(third_button)));
		}
		/*第一个按钮是调车按钮，第二个按钮是变通按钮或反方向单置信号机,第三个按钮是同方向的单置调车信号机 **/
		if((IsFALSE(result))
			&&(BT_SHUNTING == gb_type(first_button)) 
			&&(NT_CHANGE_BUTTON == gn_type(b) || IsTRUE(result_dt))
			&& (gn_direction(a) == gn_direction(c))
			&& (NT_SINGLE_SHUNTING_SIGNAL == gn_type(c)))
		{
			result = CI_TRUE;
			CIHmi_SendNormalTips("调车进路：%s --> %s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)),
				gb_node(third_button) == NO_INDEX ? gn_name(third_button) : gn_name(gb_node(third_button)));
		}
		/*第一个按钮是调车按钮，第二个按钮是变通按钮或反方向单置信号机,第三个按钮是出站兼调车信号机或进路信号机的调车按钮 **/
		if((IsFALSE(result))
			&&(BT_SHUNTING == gb_type(first_button))
			&&(NT_CHANGE_BUTTON == gn_type(b) || IsTRUE(result_dt))
			&& (NT_OUT_SHUNTING_SIGNAL == gn_type(c) || NT_ROUTE_SIGNAL == gn_type(c))
			&& (BT_SHUNTING == gb_type(third_button)))
		{
			result = CI_TRUE;
			CIHmi_SendNormalTips("调车进路：%s --> %s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)),
				gb_node(third_button) == NO_INDEX ? gn_name(third_button) : gn_name(gb_node(third_button)));
		}	
		/*第一个按钮是调车按钮，第二个按钮是变通按钮或反方向单置信号机，第三个按钮是调车终端按钮**/
		if ((IsFALSE(result))
			&&(BT_SHUNTING == gb_type(first_button))
			&&(NT_CHANGE_BUTTON == gn_type(b) || IsTRUE(result_dt))
			&& (NT_SHUNTING_END_BUTTON == gn_type(c)))
		{
			result = CI_TRUE;
			CIHmi_SendNormalTips("调车进路：%s --> %s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)),
				gb_node(third_button) == NO_INDEX ? gn_name(third_button) : gn_name(gb_node(third_button)));
		}	
	}
	/*错误操作1*/
	if (((BT_TRAIN == gb_type(first_button)) || (BT_PASSING == gb_type(first_button)))
		&& (((BT_SHUNTING == gb_type(second_button)) && (BT_SHUNTING == gb_type(third_button)))
		|| ((NT_CHANGE_BUTTON == gn_type(b)) && (NT_CHANGE_BUTTON == gn_type(c)))
		|| ((BT_SHUNTING == gb_type(second_button)) && (NT_SHUNTING_END_BUTTON == gn_type(c))))) 
	{
		process_warning(ERR_NO_ROUTE,"");
		CIHmi_SendNormalTips("错误办理：%s --> %s --> %s",
			gn_name(gb_node(first_button)),
			gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)),
			gb_node(third_button) == NO_INDEX ? gn_name(third_button) : gn_name(gb_node(third_button)));
		/*清除按钮*/
		clean_button_log();	
	}	
	/*错误操作2，第一个按钮是调车，第三个按钮是反方向单置信号机*/
	if (BT_SHUNTING == gb_type(first_button) 
		&& (gn_direction(a) != gn_direction(c)) 
		&& (NT_SINGLE_SHUNTING_SIGNAL == gn_type(c)))
	{
		process_warning(ERR_NO_ROUTE,"");
		CIHmi_SendNormalTips("错误办理：%s --> %s --> %s",
			gn_name(gb_node(first_button)),
			gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)),
			gb_node(third_button) == NO_INDEX ? gn_name(third_button) : gn_name(gb_node(third_button)));
		/*清除按钮*/
		clean_button_log();
	}
	/*错误操作3，延续进路不满足*/
	if((NT_ENTRY_SIGNAL == gn_type(a) || NT_ROUTE_SIGNAL == gn_type(a))
		&& (BT_TRAIN == gb_type(first_button))
		&& ((NT_OUT_SIGNAL == gn_type(b)) || (NT_OUT_SHUNTING_SIGNAL == gn_type(b)) || (NT_TRAIN_END_BUTTON == gn_type(b)))
		&& (BT_TRAIN == gb_type(second_button))
		&& ((NT_OUT_SIGNAL == gn_type(c)) || (NT_OUT_SHUNTING_SIGNAL == gn_type(c))|| (NT_SINGLE_SHUNTING_SIGNAL == gn_type(c))
		|| (NT_DIFF_SHUNTING_SIGNAL == gn_type(c))|| (NT_JUXTAPOSE_SHUNGTING_SIGNAL == gn_type(c)))
		&& (gn_throat(a) == gn_throat(b))
		&& (gn_throat(b) != gn_throat(c)))
	{		
		process_warning(ERR_NO_ROUTE,"");
		CIHmi_SendNormalTips("错误办理：%s --> %s --> %s",
			gn_name(gb_node(first_button)),
			gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)),
			gb_node(third_button) == NO_INDEX ? gn_name(third_button) : gn_name(gb_node(third_button)));

		/*清除按钮*/
		clean_button_log();
	}
	FUNCTION_OUT;
	return result;		
}

/****************************************************
函数名:     is_four_button_build_route
功能描述:   判断四按钮变更进路建立条件
返回值:		如果可以建立进路返回真，无法建立进路返回假
作者  :    LYC
日期  ：   2011/12/1
****************************************************/
CI_BOOL  is_four_button_build_route(void)
{
	CI_BOOL result = CI_FALSE,result_dt3 = CI_FALSE,result_dt4 = CI_FALSE;
	int16_t a = -1,b = -1,c = -1,d = -1;

	FUNCTION_IN;
	a = gb_node(first_button);
	b = gb_node(second_button);
	c = gb_node(third_button);
	d = gb_node(fourth_button);
	/*可以建立进路情况列表**/
	/*判断可能为变更延续进路
	 第一个按钮是进站或进路信号机的列车按钮，
	 第二个按钮为同一咽喉区的变更按钮或调车按钮
	 第三个按钮是同一咽喉的出站信号机或出站兼调车信号机的列车按钮或列车终端按钮
	 第四个按钮是不在同一咽喉的进站或进路信号机或调车按钮或调车终端按钮或延续进路终端按钮*/
	if((NT_ENTRY_SIGNAL == gn_type(a) || NT_ROUTE_SIGNAL == gn_type(a))
		&& (BT_TRAIN == gb_type(first_button))
		&& ((NT_CHANGE_BUTTON == gn_type(b)) || (BT_SHUNTING == gb_type(second_button)))
		&& ((NT_OUT_SIGNAL == gn_type(c)) || (NT_OUT_SHUNTING_SIGNAL == gn_type(c)) || (NT_TRAIN_END_BUTTON == gn_type(c)))
		&& (BT_TRAIN == gb_type(third_button))
		&& ((NT_ENTRY_SIGNAL == gn_type(d)) || (NT_ROUTE_SIGNAL == gn_type(d))|| (NT_STUB_END_SHUNTING_SIGNAL == gn_type(d)) 
			|| (NT_LOCODEPOT_SHUNTING_SIGNAL == gn_type(d)) || (NT_SHUNTING_END_BUTTON == gn_type(d)) || (NT_SUCCESSIVE_END_BUTTON == gn_type(d)))
		&& (gn_throat(a) == gn_throat(b))
		&& (gn_throat(b) == gn_throat(c))
		&& (gn_throat(c) != gn_throat(d)))
	{
		result = CI_TRUE;
		CIHmi_SendNormalTips("延续进路：%s --> %s --> %s --> %s",
			gn_name(gb_node(first_button)),
			gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)),
			gb_node(third_button) == NO_INDEX ? gn_name(third_button) : gn_name(gb_node(third_button)),
			gb_node(fourth_button) == NO_INDEX ? gn_name(fourth_button) : gn_name(gb_node(fourth_button)));
	}
	/*以下进路需配置是否允许跨咽喉办理，目前不允许*/
	if (gn_throat(a) == gn_throat(d))
	{
		/*第一个按钮是进站信号机或进路信号机的列车按钮，
		第二、三个按钮是变通按钮或调车按钮，第四个按钮是出站信号机**/
		if((IsFALSE(result))
			&&(NT_ENTRY_SIGNAL == gn_type(a) || NT_ROUTE_SIGNAL == gn_type(a))
			&& (BT_TRAIN == gb_type(first_button))
			&& ((NT_CHANGE_BUTTON == gn_type(b)) || (BT_SHUNTING == gb_type(second_button)))
			&& ((NT_CHANGE_BUTTON == gn_type(c)) || (BT_SHUNTING == gb_type(third_button)))	
			&& (NT_OUT_SIGNAL == gn_type(d)))
		{
			result = CI_TRUE;
			CIHmi_SendNormalTips("列车进路：%s --> %s --> %s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)),
				gb_node(third_button) == NO_INDEX ? gn_name(third_button) : gn_name(gb_node(third_button)),
				gb_node(fourth_button) == NO_INDEX ? gn_name(fourth_button) : gn_name(gb_node(fourth_button)));
		}

		/*第一个按钮是出站信号机，第二、三个按钮是变通按钮或调车按钮,
		第四个按钮是进站信号机或进路信号机的列车按钮**/
		if((IsFALSE(result))
			&&(NT_OUT_SIGNAL == gn_type(a))			
			&& ((NT_CHANGE_BUTTON == gn_type(b)) || (BT_SHUNTING == gb_type(second_button)))
			&& ((NT_CHANGE_BUTTON == gn_type(c)) || (BT_SHUNTING == gb_type(third_button)))		
			&& (NT_ENTRY_SIGNAL == gn_type(d)|| NT_ROUTE_SIGNAL == gn_type(d))
			&& (BT_TRAIN == gb_type(fourth_button)))

		{
			result = CI_TRUE;		
			CIHmi_SendNormalTips("列车进路：%s --> %s --> %s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)),
				gb_node(third_button) == NO_INDEX ? gn_name(third_button) : gn_name(gb_node(third_button)),
				gb_node(fourth_button) == NO_INDEX ? gn_name(fourth_button) : gn_name(gb_node(fourth_button)));
		}
		/*第一个按钮是进站信号机或进路信号机的列车按钮，
		第二、三个按钮是变通按钮或调车按钮,第四按钮是出站兼调车信号机的列车按钮**/
		if((IsFALSE(result))
			&&(NT_ENTRY_SIGNAL == gn_type(a) || NT_ROUTE_SIGNAL == gn_type(a))
			&& (BT_TRAIN == gb_type(first_button))
			&& ((NT_CHANGE_BUTTON == gn_type(b)) || (BT_SHUNTING == gb_type(second_button)))
			&& ((NT_CHANGE_BUTTON == gn_type(c)) || (BT_SHUNTING == gb_type(third_button)))		
			&& (NT_OUT_SHUNTING_SIGNAL == gn_type(d))
			&& (BT_TRAIN == gb_type(fourth_button)))
		{
			result = CI_TRUE;
			CIHmi_SendNormalTips("列车进路：%s --> %s --> %s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)),
				gb_node(third_button) == NO_INDEX ? gn_name(third_button) : gn_name(gb_node(third_button)),
				gb_node(fourth_button) == NO_INDEX ? gn_name(fourth_button) : gn_name(gb_node(fourth_button)));
		}
		/*第一按钮是出站兼调车信号机的列车按钮，第二、三个按钮是变通按钮或调车按钮,
		第四个按钮是进站信号机或进路信号机的列车按钮，**/
		if((IsFALSE(result))
			&&(BT_TRAIN == gb_type(first_button))
			&& (NT_OUT_SHUNTING_SIGNAL == gn_type(a))			
			&& ((NT_CHANGE_BUTTON == gn_type(b)) || (BT_SHUNTING == gb_type(second_button)))
			&& ((NT_CHANGE_BUTTON == gn_type(c)) || (BT_SHUNTING == gb_type(third_button)))				
			&& (NT_ENTRY_SIGNAL == gn_type(d) || NT_ROUTE_SIGNAL == gn_type(d))
			&& (BT_TRAIN == gb_type(fourth_button))
			)
		{
			result = CI_TRUE;
			CIHmi_SendNormalTips("列车进路：%s --> %s --> %s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)),
				gb_node(third_button) == NO_INDEX ? gn_name(third_button) : gn_name(gb_node(third_button)),
				gb_node(fourth_button) == NO_INDEX ? gn_name(fourth_button) : gn_name(gb_node(fourth_button)));
		}
		/*第一个按钮是进路信号机的列车按钮，第二、三个按钮是变通按钮或调车按钮，
		第四个按钮是进路信号机的列车按钮*/
		if ((IsFALSE(result))
			&&(NT_ROUTE_SIGNAL == gn_type(a)) 
			&& (BT_TRAIN == gb_type(first_button))	
			&&((NT_CHANGE_BUTTON == gn_type(b)) || (BT_SHUNTING == gb_type(second_button)))
			&& ((NT_CHANGE_BUTTON == gn_type(c)) || (BT_SHUNTING == gb_type(third_button)))		
			&& (NT_ROUTE_SIGNAL == gn_type(d))
			&& (BT_TRAIN == gb_type(fourth_button)))
		{
			result = CI_TRUE;
			CIHmi_SendNormalTips("列车进路：%s --> %s --> %s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)),
				gb_node(third_button) == NO_INDEX ? gn_name(third_button) : gn_name(gb_node(third_button)),
				gb_node(fourth_button) == NO_INDEX ? gn_name(fourth_button) : gn_name(gb_node(fourth_button)));
		}
		/*第一个是列车按钮，第二、三个是变通按钮或调车按钮，第四个按钮是列车终端按钮**/
		if ((IsFALSE(result))
			&&(BT_TRAIN == gb_type(first_button))
			&&((NT_CHANGE_BUTTON == gn_type(b)) || (BT_SHUNTING == gb_type(second_button)))
			&& ((NT_CHANGE_BUTTON == gn_type(c)) || (BT_SHUNTING == gb_type(third_button)))
			&& (BT_TRAIN_END == gb_type(fourth_button)))
		{
			result = CI_TRUE;
			CIHmi_SendNormalTips("列车进路：%s --> %s --> %s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)),
				gb_node(third_button) == NO_INDEX ? gn_name(third_button) : gn_name(gb_node(third_button)),
				gb_node(fourth_button) == NO_INDEX ? gn_name(fourth_button) : gn_name(gb_node(fourth_button)));
		}
		/*判断第三个按钮所在的信号点是否第一个按钮所在信号机的反方向单置信号机*/
		if (gn_direction(a) != gn_direction(b) && NT_SINGLE_SHUNTING_SIGNAL == gn_type(b))
		{
			result_dt3 = CI_TRUE;
		}
		/*判断第四个按钮所在的信号点是否第一个按钮所在信号机的反方向单置信号机*/
		if (gn_direction(a) != gn_direction(c) && NT_SINGLE_SHUNTING_SIGNAL == gn_type(c))
		{
			result_dt4 = CI_TRUE;
		}

		/*hjh 2015-7-17 第一个按钮是调车按钮，第四个按钮是调车按钮*/		
		if((IsFALSE(result))
			&&(BT_SHUNTING == gb_type(first_button)) && (BT_SHUNTING == gb_type(fourth_button)))
		{
			result = CI_TRUE;
			CIHmi_SendNormalTips("调车进路：%s --> %s --> %s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)),
				gb_node(third_button) == NO_INDEX ? gn_name(third_button) : gn_name(gb_node(third_button)),
				gb_node(fourth_button) == NO_INDEX ? gn_name(fourth_button) : gn_name(gb_node(fourth_button)));
		}

		/*第一个按钮是调车按钮，第二、三个按钮是变通按钮或反方向的单置信号机,第四个按钮是反方向的差置信号机 **/		
		if((IsFALSE(result))
			&&(BT_SHUNTING == gb_type(first_button)) 
			&& (NT_CHANGE_BUTTON == gn_type(b) || IsTRUE(result_dt3))
			&& (NT_CHANGE_BUTTON == gn_type(c) || IsTRUE(result_dt4))
			&& (gn_direction(a) != gn_direction(d))
			&& (NT_DIFF_SHUNTING_SIGNAL == gn_type(d)))
		{
			result = CI_TRUE;
			CIHmi_SendNormalTips("调车进路：%s --> %s --> %s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)),
				gb_node(third_button) == NO_INDEX ? gn_name(third_button) : gn_name(gb_node(third_button)),
				gb_node(fourth_button) == NO_INDEX ? gn_name(fourth_button) : gn_name(gb_node(fourth_button)));
		}
		/*第一个按钮是调车按钮，第二、三个按钮是变通按钮或反方向的单置信号机,第四个按钮是反方向的并置调车信号机**/
		if((IsFALSE(result))
			&&(BT_SHUNTING == gb_type(first_button)) 
			&& (NT_CHANGE_BUTTON == gn_type(b) || IsTRUE(result_dt3))
			&& (NT_CHANGE_BUTTON == gn_type(c) || IsTRUE(result_dt4))
			&& (gn_direction(a) != gn_direction(d))
			&& (NT_JUXTAPOSE_SHUNGTING_SIGNAL == gn_type(d)))
		{
			result = CI_TRUE;
			CIHmi_SendNormalTips("调车进路：%s --> %s --> %s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)),
				gb_node(third_button) == NO_INDEX ? gn_name(third_button) : gn_name(gb_node(third_button)),
				gb_node(fourth_button) == NO_INDEX ? gn_name(fourth_button) : gn_name(gb_node(fourth_button)));
		}
		/*第一个按钮是调车按钮，第二、三个按钮是变通按钮或反方向的单置信号机,第四个按钮是尽头式调车信号机**/
		if((IsFALSE(result))
			&&(BT_SHUNTING == gb_type(first_button)) 
			&& (NT_CHANGE_BUTTON == gn_type(b) || IsTRUE(result_dt3))
			&& (NT_CHANGE_BUTTON == gn_type(c) || IsTRUE(result_dt4))
			&& ((NT_STUB_END_SHUNTING_SIGNAL == gn_type(d)) || (NT_LOCODEPOT_SHUNTING_SIGNAL == gn_type(b))))
		{
			result = CI_TRUE;
			CIHmi_SendNormalTips("调车进路：%s --> %s --> %s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)),
				gb_node(third_button) == NO_INDEX ? gn_name(third_button) : gn_name(gb_node(third_button)),
				gb_node(fourth_button) == NO_INDEX ? gn_name(fourth_button) : gn_name(gb_node(fourth_button)));
		}
		/*第一个按钮是调车按钮，第二、三个按钮是变通按钮或反方向的单置信号机,第四个按钮是同方向的单置调车信号机 **/
		if((IsFALSE(result))
			&&(BT_SHUNTING == gb_type(first_button)) 
			&& (NT_CHANGE_BUTTON == gn_type(b) || IsTRUE(result_dt3))
			&& (NT_CHANGE_BUTTON == gn_type(c) || IsTRUE(result_dt4))
			&& (gn_direction(a) == gn_direction(d))
			&& (NT_SINGLE_SHUNTING_SIGNAL == gn_type(d)))
		{
			result = CI_TRUE;
			CIHmi_SendNormalTips("调车进路：%s --> %s --> %s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)),
				gb_node(third_button) == NO_INDEX ? gn_name(third_button) : gn_name(gb_node(third_button)),
				gb_node(fourth_button) == NO_INDEX ? gn_name(fourth_button) : gn_name(gb_node(fourth_button)));
		}
		/*第一个按钮是调车按钮，第二、三个按钮是变通按钮或反方向的单置信号机,第四个按钮是出站兼调车信号机的调车按钮 **/
		if((IsFALSE(result))
			&&(BT_SHUNTING == gb_type(first_button))
			&& (NT_CHANGE_BUTTON == gn_type(b) || IsTRUE(result_dt3))
			&& (NT_CHANGE_BUTTON == gn_type(c) || IsTRUE(result_dt4))
			&& (NT_OUT_SHUNTING_SIGNAL == gn_type(d))
			&& (BT_SHUNTING == gb_type(fourth_button)))
		{
			result = CI_TRUE;
			CIHmi_SendNormalTips("调车进路：%s --> %s --> %s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)),
				gb_node(third_button) == NO_INDEX ? gn_name(third_button) : gn_name(gb_node(third_button)),
				gb_node(fourth_button) == NO_INDEX ? gn_name(fourth_button) : gn_name(gb_node(fourth_button)));
		}	
		/*第一个按钮是调车按钮，第二、三个按钮是变通按钮或反方向的单置信号机，第四个按钮是调车终端按钮**/
		if ((IsFALSE(result))
			&&(BT_SHUNTING == gb_type(first_button))
			&&(NT_CHANGE_BUTTON == gn_type(b) || IsTRUE(result_dt3))
			&&(NT_CHANGE_BUTTON == gn_type(c) || IsTRUE(result_dt4))
			&& (NT_SHUNTING_END_BUTTON == gn_type(d)))
		{
			result = CI_TRUE;
			CIHmi_SendNormalTips("调车进路：%s --> %s --> %s --> %s",
				gn_name(gb_node(first_button)),
				gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)),
				gb_node(third_button) == NO_INDEX ? gn_name(third_button) : gn_name(gb_node(third_button)),
				gb_node(fourth_button) == NO_INDEX ? gn_name(fourth_button) : gn_name(gb_node(fourth_button)));
		}	
	}
	FUNCTION_OUT;
	return result;		
}

/****************************************************
函数名：   is_five_button_build_route
功能描述： 判断5个按钮的进路建立条件
返回值：   CI_BOOL
参数：     void
作者：	   hejh
日期：     2015/06/02
****************************************************/
CI_BOOL  is_five_button_build_route(void)
{
	CI_BOOL result = CI_FALSE;
	int16_t a = -1,b = -1,c = -1,d = -1,e = -1;

	FUNCTION_IN;
	a = gb_node(first_button);
	b = gb_node(second_button);
	c = gb_node(third_button);
	d = gb_node(fourth_button);
	d = gb_node(fifth_button);
	/*可以建立进路情况列表**/
	/*判断可能为变更延续进路
	 第一个按钮是进站或进路信号机的列车按钮，
	 第二个按钮为同一咽喉区的变更按钮或调车按钮
	 第三个按钮为同一咽喉区的变更按钮或调车按钮
	 第四个按钮是同一咽喉的出站信号机或出站兼调车信号机的列车按钮或列车终端按钮
	 第五个按钮是不在同一咽喉的进站或进路信号机或调车按钮或调车终端按钮或延续进路终端按钮*/
	if((NT_ENTRY_SIGNAL == gn_type(a) || NT_ROUTE_SIGNAL == gn_type(a)) && (BT_TRAIN == gb_type(first_button))
		&& ((NT_CHANGE_BUTTON == gn_type(b)) || (BT_SHUNTING == gb_type(second_button)))
		&& ((NT_CHANGE_BUTTON == gn_type(c)) || (BT_SHUNTING == gb_type(third_button)))
		&& ((NT_OUT_SIGNAL == gn_type(d)) || (NT_OUT_SHUNTING_SIGNAL == gn_type(d)) || (NT_TRAIN_END_BUTTON == gn_type(d))) && (BT_TRAIN == gb_type(fourth_button))		
		&& ((NT_ENTRY_SIGNAL == gn_type(e)) || (NT_ROUTE_SIGNAL == gn_type(e))|| (NT_STUB_END_SHUNTING_SIGNAL == gn_type(e)) 
			|| (NT_LOCODEPOT_SHUNTING_SIGNAL == gn_type(e)) || (NT_SHUNTING_END_BUTTON == gn_type(e)) || (NT_SUCCESSIVE_END_BUTTON == gn_type(e)))
		&& (gn_throat(a) == gn_throat(b))
		&& (gn_throat(b) == gn_throat(c))
		&& (gn_throat(c) == gn_throat(d))
		&& (gn_throat(d) != gn_throat(e)))
	{
		result = CI_TRUE;
		CIHmi_SendNormalTips("延续进路：%s --> %s --> %s --> %s --> %s",
			gn_name(gb_node(first_button)),
			gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)),
			gb_node(third_button) == NO_INDEX ? gn_name(third_button) : gn_name(gb_node(third_button)),
			gb_node(fourth_button) == NO_INDEX ? gn_name(fourth_button) : gn_name(gb_node(fourth_button)),
			gb_node(fifth_button) == NO_INDEX ? gn_name(fifth_button) : gn_name(gb_node(fifth_button)));
	}
	FUNCTION_OUT;
	return result;		
}
/****************************************************
函数名:    clean_button_log
功能描述:  清除四个按钮的记录
返回值:	   无
作者  :    LYC
日期  ：   2011/11/29
****************************************************/
void clean_button_log(void)
{
	first_button = FB_NO_BUTTON;							
	second_button = FB_NO_BUTTON;
	third_button = FB_NO_BUTTON;
	fourth_button = FB_NO_BUTTON;
	fifth_button = FB_NO_BUTTON;
	passing_route = CI_FALSE;   
}

/****************************************************
函数名:    function_button_choose
功能描述:  根据第一个按下的功能按钮选择调用相应的模块
返回值:    无
作者  :    LYC
日期  ：   2011/11/29
****************************************************/
void function_button_choose(void)
{
	route_t route_index = NO_INDEX;
	node_t section = NO_NODE;
	
	FUNCTION_IN;
	/*参数检查*/
	if (second_button >= 0 && second_button < TOTAL_NAMES)
	{
		route_index = gn_belong_route(gb_node(second_button));
	}
	
	switch(first_button)
	{
		case FB_CANCEL_ROUTE : /*1.判断第一个按钮是否取消进路按钮*/	
			/*第二个按钮是进路始端按钮,列车进路的列车按钮，调车进路的调车按钮*/
			if ((NO_INDEX == route_index) 
				|| (gr_start_signal(route_index) != gb_node(second_button)) 				
				|| ((RT_SHUNTING_ROUTE == gr_type(route_index)) && (BT_SHUNTING != gb_type(second_button)))				
				|| ((RT_TRAIN_ROUTE == gr_type(route_index)) && (BT_TRAIN != gb_type(second_button)))
				|| ((RT_SUCCESSIVE_ROUTE == gr_type(route_index)) && (BT_TRAIN != gb_type(second_button))))
			{
				process_warning(ERR_OPERATION,gn_name(gb_node(second_button)));
				CIHmi_SendNormalTips("错误办理：%s",gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
			}
			else
			{
				//CIHmi_SendNormalTips("取消进路：%s",gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
				command_cancel_route(route_index);
			}
			clean_button_log();
			break;
	
		case FB_HUMAN_UNLOCK :/*2.判断第一个按钮是否人工延时解锁按钮*/	
			/*第二个按钮是进路始端按钮,列车进路的列车按钮，调车进路的调车按钮*/
			if ((NO_INDEX == route_index) || (gr_start_signal(route_index) != gb_node(second_button)) 				
				|| ((RT_SHUNTING_ROUTE == gr_type(route_index)) && (BT_SHUNTING != gb_type(second_button)))				
				|| ((RT_TRAIN_ROUTE == gr_type(route_index)) && (BT_TRAIN != gb_type(second_button)))
				|| ((RT_SUCCESSIVE_ROUTE == gr_type(route_index)) && (BT_TRAIN != gb_type(second_button))))
			{
				process_warning(ERR_OPERATION,gn_name(gb_node(second_button)));
				CIHmi_SendNormalTips("错误办理：%s",gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
			}
			else
			{
				//CIHmi_SendNormalTips("人工解锁：%s",gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
				command_relay_cancel_route(route_index);
			}
			clean_button_log();
			break;
	
		case FB_REOPEN_SIGNAL : /*3.判断第一个按钮是否重复开放按钮*/	
			/*判断第二个按钮是否进路始端按钮*/
			/*第二个按钮是进路始端按钮,列车进路的列车按钮，调车进路的调车按钮*/
			if ((NO_INDEX == route_index) || (gr_start_signal(route_index) != gb_node(second_button)) 				
				|| ((RT_SHUNTING_ROUTE == gr_type(route_index)) && (BT_SHUNTING != gb_type(second_button)))				
				|| ((RT_TRAIN_ROUTE == gr_type(route_index)) && (BT_TRAIN != gb_type(second_button)))
				|| ((RT_SUCCESSIVE_ROUTE == gr_type(route_index)) && (BT_TRAIN != gb_type(second_button))))
			{
				process_warning(ERR_OPERATION,gn_name(gb_node(second_button)));
				CIHmi_SendNormalTips("错误办理：%s",gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
			}
			else
			{
				//CIHmi_SendNormalTips("重复开放信号：%s",gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
				command_repeated_open_signal(route_index);
			}
			clean_button_log();
			break;
	
		case FB_SECTION_UNLOCK : /*4.判断第一个按钮是否故障解锁按钮*/
			/*判断第二个按钮是否轨道*/
			if ((second_button < 0) || (second_button >= TOTAL_SIGNAL_NODE))
			{
				process_warning(ERR_OPERATION,gn_name(gb_node(second_button)));
				CIHmi_SendNormalTips("错误办理：%s",gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));			
			}
			else
			{	
				if (gn_type(second_button) == NT_SWITCH)
				{
					section = gn_switch_section(second_button);
				}
				else if (IsTRUE(is_section(second_button)))
				{
					section = second_button;
				}
				else
				{
					CIHmi_SendNormalTips("错误办理：%s",gn_name(gb_node(second_button)));
					process_warning(ERR_OPERATION,gn_name(gb_node(second_button)));
				}
				if (gn_belong_route(section) != NO_INDEX)
				{
					//CIHmi_SendNormalTips("区段故障解锁：%s",gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
					command_section_unlock(section);
				} 
			}
			clean_button_log();
			break;

		case FB_GUIDE_ROUTE : /*5.判断第一个按钮是否引导进路按钮*/	
			/*判断第二个按钮是进站或进路信号机的列车按钮*/			
			if ((NT_ENTRY_SIGNAL == gn_type(gb_node(second_button)) && BT_TRAIN == gb_type(second_button))
				|| (NT_ROUTE_SIGNAL == gn_type(gb_node(second_button)) && BT_TRAIN == gb_type(second_button)))
			{
				//CIHmi_SendNormalTips("引导进路：%s",gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
				command_guide_route(gb_node(second_button));
			}
			else
			{
				process_warning(ERR_OPERATION,gn_name(gb_node(second_button)));
				CIHmi_SendNormalTips("错误办理：%s",gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
			}
			clean_button_log();
			break;

		case FB_THROAT_LOCK : /*6.判断第一个按钮是否引总锁闭按钮*/
			/*2013/9/3 LYC 判断第二个按钮是引总锁闭咽喉按钮*/
			if (BT_THROAT_GUIDE_LOCK == gb_type(second_button))
			{
				//CIHmi_SendNormalTips("引总锁闭：%s",gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
				throat_lock(gn_throat(gb_node(second_button)));				
			} 
			else
			{
				process_warning(ERR_OPERATION,gn_name(gb_node(second_button)));
				CIHmi_SendNormalTips("错误办理：%s",gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
			}		
			clean_button_log();
			break;

		case FB_THROAT_UNLOCK : /*7.判断第一个按钮是否引总解锁按钮*/	
			/*2013/9/3 LYC 判断第二个按钮是引总锁闭咽喉按钮*/
			if (BT_THROAT_GUIDE_LOCK == gb_type(second_button))
			{
				//CIHmi_SendNormalTips("引总解锁：%s",gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
				throat_unlock(gn_throat(gb_node(second_button)));				
			}
			else
			{
				process_warning(ERR_OPERATION,gn_name(gb_node(second_button)));
				CIHmi_SendNormalTips("错误办理：%s",gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
			}			
			clean_button_log();
			break;

		case FB_SWITCH_LOCK : /*8.判断第一个按钮是否道岔单锁按钮*/
			/*判断第二个按钮是道岔*/
			if (NT_SWITCH != gn_type(gb_node(second_button)))
			{
				process_warning(ERR_OPERATION,gn_name(gb_node(second_button)));
				CIHmi_SendNormalTips("错误办理：%s",gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
			}
			else
			{
				//CIHmi_SendNormalTips("单锁道岔：%s",gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
				switch_single_lock(gb_node(second_button));
			}			
			clean_button_log();
			break;

		case FB_SWITCH_UNLOCK : /*9.判断第一个按钮是否道岔单解按钮*/
			/*判断第二个按钮是道岔*/
			if (NT_SWITCH != gn_type(gb_node(second_button)))
			{
				process_warning(ERR_OPERATION,gn_name(gb_node(second_button)));
				CIHmi_SendNormalTips("错误办理：%s",gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
			}
			else
			{
				//CIHmi_SendNormalTips("单解道岔：%s",gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
				switch_single_unlock(gb_node(second_button));
			}				
			clean_button_log();
			break;
	
		case FB_SWITCH_CLOSE : /*10.判断第一个按钮是否道岔封锁按钮*/
			/*判断第二个按钮是道岔*/
			if (NT_SWITCH != gn_type(gb_node(second_button)))
			{
				process_warning(ERR_OPERATION,gn_name(gb_node(second_button)));
				CIHmi_SendNormalTips("错误办理：%s",gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
			}
			else
			{
				//CIHmi_SendNormalTips("封锁道岔：%s",gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
				switch_close_up(gb_node(second_button));
			}				
			clean_button_log();
			break;
			
		case FB_SWITCH_UNCLOSE : /*11.判断第一个按钮是否道岔解封按钮*/
			/*判断第二个按钮是道岔*/
			if (NT_SWITCH != gn_type(gb_node(second_button)))
			{
				process_warning(ERR_OPERATION,gn_name(gb_node(second_button)));
				CIHmi_SendNormalTips("错误办理：%s",gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
			}
			else
			{
				//CIHmi_SendNormalTips("解封道岔：%s",gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
				switch_unclose_up(gb_node(second_button));
			}			
			clean_button_log();
			break;
		
		case FB_SWITCH_NORMAL : /*12.判断第一个按钮是否道岔定操按钮*/	
			/*判断第二个按钮是道岔*/
			if ( (NT_SWITCH != gn_type(second_button)))
			{
				process_warning(ERR_OPERATION,gn_name(second_button));
				CIHmi_SendNormalTips("错误办理：%s",gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
			}
			else
			{
				//CIHmi_SendNormalTips("定操道岔：%s",gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
				switch_single_operate(gb_node(second_button), SWS_NORMAL);
			}				
			clean_button_log();
			break;
		
		case FB_SWITCH_REVERSE : /*13.判断第一个按钮是否道岔反操按钮*/
			/*判断第二个按钮是道岔*/
			if ( (NT_SWITCH != gn_type(second_button)))
			{
				process_warning(ERR_OPERATION,gn_name(second_button));
				CIHmi_SendNormalTips("错误办理：%s",gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
			}
			else
			{
				//CIHmi_SendNormalTips("反操道岔：%s",gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
				switch_single_operate(gb_node(second_button), SWS_REVERSE);
			}				
			clean_button_log();
			break;
			
		case FB_CLOSE_SIGNAL : /*14.判断第一个按钮是否关闭信号按钮*/
			/*第二个按钮是进路始端按钮,列车进路的列车按钮，调车进路的调车按钮*/
			if ((NO_INDEX == route_index) || (gr_start_signal(route_index) != gb_node(second_button)) 				
				|| ((RT_SHUNTING_ROUTE == gr_type(route_index)) && (BT_SHUNTING != gb_type(second_button)))				
				|| ((RT_TRAIN_ROUTE == gr_type(route_index)) && (BT_TRAIN != gb_type(second_button)))
				|| ((RT_SUCCESSIVE_ROUTE == gr_type(route_index)) && (BT_TRAIN != gb_type(second_button))))
			{
				/*引总锁闭后的引导信号关闭*/
				if ((NO_INDEX == route_index) && (SGS_YB == gn_signal_state(gb_node(second_button))))
				{
					//CIHmi_SendNormalTips("关闭信号：%s",gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
					command_close_signal(gb_node(second_button));
				}
				else if ((NO_INDEX != route_index) && (SGS_YB == gn_signal_state(gb_node(second_button)))
					&& (gr_start_signal(route_index) != gb_node(second_button)))
				{
					command_close_signal(gb_node(second_button));
				}
				else
				{
					process_warning(ERR_OPERATION,gn_name(gb_node(second_button)));
					CIHmi_SendNormalTips("错误办理：%s",gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
				}
			}
			else
			{
				//CIHmi_SendNormalTips("关闭信号：%s",gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
				command_close_signal(gb_node(second_button));
			}	
			clean_button_log();
			break;
	
		case FB_SUCCESSIVE_UNLOCK : /*15.判断第一个按钮是否坡道解锁按钮*/ 
			if ((NO_INDEX == route_index) || (gr_start_signal(route_index) != gb_node(second_button)))
			{
				process_warning(ERR_OPERATION,gn_name(gb_node(second_button)));
				CIHmi_SendNormalTips("错误办理：%s",gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
			}
			else
			{
				//CIHmi_SendNormalTips("坡道解锁：%s",gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
				command_succesive_route_unlock(route_index);
			}
			clean_button_log();
			break;
			
		case FB_HOLD_ROUTE : /*16.判断第一个按钮是否非进路控制按钮*/
			if (gs_hold_route_shunting_index(gb_node(second_button)) != NO_INDEX)
			{
				//CIHmi_SendNormalTips("非进路调车：%s",gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
				command_hold_route_shunting(gs_hold_route_shunting_index(gb_node(second_button)));
			}
			else
			{
				process_warning(ERR_OPERATION,gn_name(gb_node(second_button)));
				CIHmi_SendNormalTips("错误办理：%s",gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
			}			
			clean_button_log();
			break;
		case FB_HOLD_ROUTE_FAULT : /*16.判断第一个按钮是否非进路故障按钮*/
			if (gs_hold_route_shunting_index(gb_node(second_button)) != NO_INDEX)
			{
				//CIHmi_SendNormalTips("非进路调车故障恢复：%s",gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
				hold_route_shunting_fault(gs_hold_route_shunting_index(gb_node(second_button)));
			}
			else
			{
				process_warning(ERR_OPERATION,gn_name(gb_node(second_button)));
				CIHmi_SendNormalTips("错误办理：%s",gb_node(second_button) == NO_INDEX ? gn_name(second_button) : gn_name(gb_node(second_button)));
			}			
			clean_button_log();
			break;
			
		case FB_SWITCH_LOCAL_CONTROL : /*17.判断第一个按钮是否非道岔局部控制按钮*/
			clean_button_log();
			break;

		case FB_INIT_DATA1 :		  /*18.判断第一个按钮是初始化系统数据按钮1*/
			if (FB_INIT_DATA2 == second_button)		/*判断第二个按钮是初始化系统数据按钮2*/
			{
				reset_signal_nodes();
				reset_routes();
			}
			break;

		default :  /*没有对应的功能按钮直接退出*/
			break;			
	}
	FUNCTION_OUT;
}


/****************************************************
函数名:    process_third_button
功能描述:  第三个按钮处理
返回类值:  无
作者  :    LYC
日期  ：   2011/11/29
****************************************************/
void process_third_button(void) 
{		
	FUNCTION_IN;
	/*判断未记录第三个按钮*/
	if (FB_NO_BUTTON == third_button)
	{
		/*记录第三个按钮*/
		third_button = pressed_button;
		/*判断满足三按钮变更进路建立条件*/
		if (IsTRUE( is_three_button_build_route()))
		{
			/*设置进路建立命令*/
			can_build_route = CI_TRUE;
		}							
	} 
	else
	{
		/**判断未记录第四个按钮*/
		if (FB_NO_BUTTON == fourth_button)
		{
			/**记录第四个按钮*/
			fourth_button = pressed_button;
			/*判断满足四按钮变更进路建立条件*/			
			if (IsTRUE(is_four_button_build_route()))
			{
				/*设置进路建立命令*/
				can_build_route = CI_TRUE;
			} 
			else
			{
				/*清除四个按钮的记录*/
				clean_button_log();	
				process_warning(ERR_NO_ROUTE,"");
			}
		} 
	}
	FUNCTION_OUT;
}

/****************************************************
函数名:    need_third_button
功能描述:  根据当前按下的两个按钮判断是否还需按下
第三个按钮才能形成命令
返回值:		需要按下第三个按钮返回真，不需要返回假
作者  :    LYC
日期  ：   2011/12/2
****************************************************/
CI_BOOL need_third_button(void)
{
	int16_t a = 0,b = 0;
	CI_BOOL result = CI_FALSE;

	FUNCTION_IN;
	/*参数检查*/
	if (first_button > -1)
	{
		a = gb_node(first_button);
		b = gb_node(second_button);
		/*判断需要按下第三个按钮的条件*/
		/*第二个按钮是变更按钮*/
		if (NT_CHANGE_BUTTON == gn_type(b))
		{
			result = CI_TRUE;
		}	
		/*第一个按钮是列车按钮第二个按钮是调车按钮*/
		if ((BT_TRAIN == gb_type(first_button)) && (BT_SHUNTING == gb_type(second_button)) 
			&& (NT_STUB_END_SHUNTING_SIGNAL != gn_type(b)) && (NT_LOCODEPOT_SHUNTING_SIGNAL != gn_type(b))
			&& (NT_OUT_SHUNTING_SIGNAL !=gn_type(b)))
		{	
			/*错误操作:第一个按钮是进路信号机的列车按钮，第二个按钮是调车信号机*/
			if ((NT_ROUTE_SIGNAL == gn_type(a)) 
				&& (BT_TRAIN == gb_type(first_button))
				&& (BT_SHUNTING == gb_type(second_button)))
			{
				result = CI_FALSE;
			}
			else
			{
				result = CI_TRUE;
			}

		}	
		/*第一个按钮是调车按钮，第二个按钮是反方向的单置信号机*/
		if (BT_SHUNTING == gb_type(first_button) 
			&& gn_direction(a) != gn_direction(b) 
			&& NT_SINGLE_SHUNTING_SIGNAL == gn_type(b))
		{
			result = CI_TRUE;
		}
		
	}
	FUNCTION_OUT;
	return result;
}
/*
 功能描述    : 平台层要使用运算层当中使用的button信息，通过此函数获得
 返回值      : 记录的当前按下的按钮pressed_button
 参数        : 
 作者        : 何境泰
 日期        : 2014年6月20日 12:40:09
 */
int16_t CICommand_GetButton( int16_t *button1,
                            int16_t *button2,
                            int16_t *button3,
                            int16_t *button4,
							int16_t *button5)
{
    *button1 = first_button;
    *button2 = second_button;
    *button3 = third_button;
    *button4 = fourth_button;
	*button5 = fifth_button;

    return pressed_button;
}
/*
 功能描述    : 对按钮信息进行更改
 返回值      : 记录的当前按下的按钮pressed_button
 参数        : 
 作者        : 何境泰
 日期        : 2014年6月20日 12:40:09
*/
int16_t CICommand_SetButton(int16_t button1, 
                            int16_t button2,
                            int16_t button3,
                            int16_t button4,
							int16_t button5)
{
    first_button = button1;
    second_button = button2;
    third_button = button3;
    fourth_button = button4;
    fifth_button = button5;

    return 0;
}
