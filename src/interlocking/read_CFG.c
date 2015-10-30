/***************************************************************************************
Copyright (C), 2014,  Co.Hengjun, Ltd.
文件名:read_config.cpp
作者:  hejh
版本:  V1.0	
日期:  2014/07/04
用途:  读取配置文件
历史修改记录:  

***************************************************************************************/
#include <string.h>
#include "global_data.h"
#include "utility_function.h"
#include "platform/platform_api.h"

#define TAB '\t'
#define EOS '\0'
#define ENTER '\r'

FILE* FilePoint;						/*文件指针*/

/****************************************************
函数名：   read_CFG
功能描述： 读取配置文件
返回值：   CI_BOOL
作者：	   hejh
日期：     2014/08/26
****************************************************/
CI_BOOL read_CFG(char_t StationName[]);

/****************************************************
函数名：   ReadCFGFile
功能描述： 读取cfg文件
返回值：   CI_BOOL
参数：     char_t * FileNameCFG
作者：	   hejh
日期：     2014/09/02
****************************************************/
CI_BOOL ReadCFGFile(char_t *FileNameCFG);

/****************************************************
函数名：   AnalyzeCFGFile
功能描述： 解析cfg文件
返回值：   void
参数：     char_t * pbuff
作者：	   hejh
日期：     2014/09/02
****************************************************/
void AnalyzeCFGFile(char_t * pbuff);

/****************************************************
函数名：   WriteToSpecialConfig
功能描述： 记录特殊配置
返回值：   void
参数：     char_t * SpecialName
参数：     uint8_t TabCount
参数：     char_t * TempBuff
作者：	   hejh
日期：     2014/09/02
****************************************************/
void WriteToSpecialConfig(char_t * SpecialName,uint8_t TabCount,char_t * TempBuff,uint8_t CommaCount);

/****************************************************
函数名:    index_cmp
功能描述:  字符串比较
返回值:    
参数:      char_t * str
作者  :    hejh
日期  ：   2012/9/15
****************************************************/
int16_t index_cmp(char_t* str);

/****************************************************
函数名：   read_CFG
功能描述： 读取配置文件
返回值：   CI_BOOL
作者：	   hejh
日期：     2014/08/26
****************************************************/
CI_BOOL read_CFG(char_t StationName[])
{
	CI_BOOL result = CI_FALSE;
    char_t fileNameCFG[FILE_NAME_LENGTH] = SETTINGS_PATH;

	/*拼接配置文件名称*/	
	strcat_check(fileNameCFG,StationName,sizeof(fileNameCFG));
	strcat_check(fileNameCFG,".cfg",sizeof(fileNameCFG));

	if (IsTRUE(ReadCFGFile(fileNameCFG)))
	{
		result = CI_TRUE;
	}
	return result;
}

/****************************************************
函数名：   ReadCFGFile
功能描述： 读取cfg文件
返回值：   CI_BOOL
参数：     char_t * FileNameCFG
作者：	   hejh
日期：     2014/09/02
****************************************************/
CI_BOOL ReadCFGFile(char_t *FileNameCFG)
{
	CI_BOOL result = CI_FALSE;
	char_t buff[MAX_STRING_LENGTH];
	uint16_t RouteCount = 0;

	if ((FilePoint = fopen(FileNameCFG,"rb")) == NULL)
	{
		CIHmi_SendDebugTips("打开文件失败!【%s】",FileNameCFG);	
		result = CI_FALSE;
	}
	else
	{
		/*按行读入文件*/
		while(fgets(buff,MAX_STRING_LENGTH,FilePoint) != NULL)
		{
			AnalyzeCFGFile(buff);			
			RouteCount++;
		}
		result = CI_TRUE;
	}
	fclose(FilePoint);
	return result;
}

/****************************************************
函数名：   AnalyzeCFGFile
功能描述： 解析cfg文件
返回值：   void
参数：     char_t * pbuff
作者：	   hejh
日期：     2014/09/02
****************************************************/
void AnalyzeCFGFile(char_t * pbuff)
{
	uint8_t num = 0,tab_count = 0,comma_count = 0,temp_buff_count = 0,i;
	char_t temp_buff[ITEM_NAME_LENGTH] = {EOS};
	char_t buff[ITEM_NAME_LENGTH] = {EOS};

	/*判断整行结束*/
	while(pbuff[num] != EOS)
	{
		/*无效信息直接退出*/
		if (pbuff[num] == ENTER)
		{
			break;
		}
		if (pbuff[num] == '#')
		{
			break;
		}

		if (pbuff[num] == '=')
		{
			if (strcmp_no_case(buff,temp_buff) != 0)
			{
				strcpy_check(buff,temp_buff,sizeof(buff));				
			}
			temp_buff_count = 0;		
			/*临时缓存区清空*/
			for (i = 0; i < ITEM_NAME_LENGTH; i++)
			{
				temp_buff[i] = EOS;
			}
		}
		else if (pbuff[num] == ';')
		{
			temp_buff_count = 0;	
			comma_count = 0;			
			if (strlen_check(temp_buff) != 0)
			{
				WriteToSpecialConfig(buff,tab_count,temp_buff,comma_count);
				/*临时缓存区清空*/
				for (i = 0; i < ITEM_NAME_LENGTH; i++)
				{
					//buff[i] = EOS;
					temp_buff[i] = EOS;
				}
			}
			tab_count++;
		}
		else
		{
			/*临时缓存区复制数据*/
			if (pbuff[num] != ',')
			{
				temp_buff[temp_buff_count] = pbuff[num];
				temp_buff_count++;
			}
			else
			{
				comma_count++;
				temp_buff_count = 0;					
				WriteToSpecialConfig(buff,tab_count,temp_buff,comma_count);
				/*临时缓存区清空*/
				for (i = 0; i < ITEM_NAME_LENGTH; i++)
				{
					//buff[i] = EOS;
					temp_buff[i] = EOS;
				}
			}
		}		
		num++;
	}
}

/****************************************************
函数名：   WriteToSpecialConfig
功能描述： 记录特殊配置
返回值：   void
参数：     char_t * SpecialName
参数：     uint8_t TabCount
参数：     char_t * TempBuff
作者：	   hejh
日期：     2014/09/02
****************************************************/
void WriteToSpecialConfig(char_t * SpecialName,uint8_t TabCount,char_t * TempBuff,uint8_t CommaCount)
{
	int16_t i;
	uint8_t count = 0;

	/*列车进路延时30s*/
	if (strcmp_no_case(SpecialName,"LCYSJS30s") == 0)
	{
		for (i = 0; i < MAX_DELAY_30SECONDS; i++)
		{
			if (delay_30seconds_config[i].SignalIndex == NO_INDEX)
			{
				delay_30seconds_config[i].SignalIndex = index_cmp(TempBuff);
				total_delay_30seconds++;
				if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
				{
					CIHmi_SendDebugTips("Error!列车进路延时30s配置查找不到该对象【%s】",TempBuff);
				}
				if (total_delay_30seconds > MAX_DELAY_30SECONDS)
				{
					CIHmi_SendDebugTips("Warning!列车进路延时30s配置数量: %d > MAX_DELAY_30SECONDS(%d)，请联系开发人员！",total_delay_30seconds,MAX_DELAY_30SECONDS);
				}
				break;
			}
		}
	}
	/*开放允许信号前检查红灯断丝*/
	else if (strcmp_no_case(SpecialName,"JCHDDS") == 0)
	{		
		for (i = 0; i < MAX_RED_FILAMENT; i++)
		{
			if (red_filament_config[i].SignalIndex == NO_INDEX)
			{
				red_filament_config[i].SignalIndex = index_cmp(TempBuff);
				total_red_filament++;
				if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
				{
					CIHmi_SendDebugTips("Error!红灯断丝配置查找不到该对象【%s】",TempBuff);
				}
				if (total_red_filament > MAX_RED_FILAMENT)
				{
					CIHmi_SendDebugTips("Warning!检查红灯断丝配置数量: %d > MAX_RED_FILAMENT(%d)，请联系开发人员！",total_red_filament,MAX_RED_FILAMENT);
				}
				break;
			}
		}
	}
	/*挤岔报警时间*/
	else if (strcmp_no_case(SpecialName,"JCBJ") == 0)
	{
		time_jcbj = (int16_t)atoi(TempBuff);
	}
	/*半自动闭塞*/
	else if (strcmp_no_case(SpecialName,"BZD") == 0)
	{
		if (TabCount == 0)
		{
			semi_auto_block_config[total_semi_auto_block].entry_signal = index_cmp(TempBuff);
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!半自动闭塞配置查找不到该对象【%s】",TempBuff);
			}
		}
		else if (TabCount == 1)
		{
			semi_auto_block_config[total_semi_auto_block].mem_node = index_cmp(TempBuff);
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!半自动闭塞配置查找不到该对象【%s】",TempBuff);
			}
		}
		else if (TabCount == 2)
		{
			semi_auto_block_config[total_semi_auto_block].block_button = index_cmp(TempBuff);
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!半自动闭塞配置查找不到该对象【%s】",TempBuff);
			}
		}
		else if (TabCount == 3)
		{
			semi_auto_block_config[total_semi_auto_block].cancel_button = index_cmp(TempBuff);
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!半自动闭塞配置查找不到该对象【%s】",TempBuff);
			}
		}
		else if (TabCount == 4)
		{
			semi_auto_block_config[total_semi_auto_block].failuer_button = index_cmp(TempBuff);
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!半自动闭塞配置查找不到该对象【%s】",TempBuff);
			}
		}
		else if (TabCount == 5)
		{
			semi_auto_block_config[total_semi_auto_block].ZJ = index_cmp(TempBuff);
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!半自动闭塞配置查找不到该对象【%s】",TempBuff);
			}
		}			
		else if (TabCount == 6)
		{
			semi_auto_block_config[total_semi_auto_block].FJ = index_cmp(TempBuff);
			total_semi_auto_block++;
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!半自动闭塞配置查找不到该对象【%s】",TempBuff);
			}
			if (total_semi_auto_block > MAX_SEMI_AUTO_BLOCK)
			{
				CIHmi_SendDebugTips("Warning!半自动闭塞配置数量: %d > MAX_SEMI_AUTO_BLOCK(%d)，请联系开发人员！",total_semi_auto_block,MAX_SEMI_AUTO_BLOCK);
			}
		}
		else
		{
			CIHmi_SendDebugTips("Warning!半自动闭塞配置文件错误！");
		}
	}
	/*股道出岔*/
	else if (strcmp_no_case(SpecialName,"GDCC") == 0)
	{
		if (TabCount == 0)
		{
			middle_switch_config[total_middle_switch].SignalIndex = index_cmp(TempBuff);		
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!股道出岔配置查找不到该对象【%s】",TempBuff);
			}
		}
		else if (TabCount == 1)
		{
			middle_switch_config[total_middle_switch].SwitchIndex[middle_switch_config[total_middle_switch].SwitchCount] = index_cmp(TempBuff);
			middle_switch_config[total_middle_switch].SwitchCount++;
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!股道出岔配置查找不到该对象【%s】",TempBuff);
			}
		}
		else if (TabCount == 2)
		{
			middle_switch_config[total_middle_switch].SwitchIndex[middle_switch_config[total_middle_switch].SwitchCount] = index_cmp(TempBuff);
			middle_switch_config[total_middle_switch].SwitchCount++;
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!股道出岔配置查找不到该对象【%s】",TempBuff);
			}
		}
		else if (TabCount == 3)
		{
			middle_switch_config[total_middle_switch].SectionIndex[middle_switch_config[total_middle_switch].SectionCount] = index_cmp(TempBuff);
			middle_switch_config[total_middle_switch].SectionCount++;
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!股道出岔配置查找不到该对象【%s】",TempBuff);
			}
		}
		else if (TabCount == 4)
		{
			middle_switch_config[total_middle_switch].SectionIndex[middle_switch_config[total_middle_switch].SectionCount] = index_cmp(TempBuff);
			middle_switch_config[total_middle_switch].SectionCount++;
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!股道出岔配置查找不到该对象【%s】",TempBuff);
			}
		}
		else if (TabCount == 5)
		{
			middle_switch_config[total_middle_switch].SectionIndex[middle_switch_config[total_middle_switch].SectionCount] = index_cmp(TempBuff);
			middle_switch_config[total_middle_switch].SectionCount++;
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!股道出岔配置查找不到该对象【%s】",TempBuff);
			}
		}
		else if (TabCount == 6)
		{
			middle_switch_config[total_middle_switch].SectionIndex[middle_switch_config[total_middle_switch].SectionCount] = index_cmp(TempBuff);			
			middle_switch_config[total_middle_switch].SectionCount++;
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!股道出岔配置查找不到该对象【%s】",TempBuff);
			}
		}
		else if (TabCount == 7)
		{
			if (strcmp_no_case(TempBuff,"0"))
			{
				middle_switch_config[total_middle_switch].AllowReverseDepature = CI_TRUE;
			}
			else
			{
				middle_switch_config[total_middle_switch].AllowReverseDepature = CI_FALSE;
			}
			total_middle_switch++;
			if (total_middle_switch > MAX_MIDLLE_SWITCH)
			{
				CIHmi_SendDebugTips("Warning!股道出岔配置数量: %d > MAX_MIDLLE_SWITCH(%d)，请联系开发人员！",total_middle_switch,MAX_MIDLLE_SWITCH);
			}
		}
		else
		{
			CIHmi_SendDebugTips("Warning!【股道出岔配置文件错误】");
		}
	}
	/*特殊输入*/
	else if (strcmp_no_case(SpecialName,"TSSR") == 0)
	{
		if (TabCount == 0)
		{
			special_input_config[total_special_input].InputNodeIndex = index_cmp(TempBuff);		
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!特殊输入配置查找不到该对象【%s】",TempBuff);
			}
		}
		else if (TabCount == 1)
		{
			if (strcmp_no_case(TempBuff,"Input") == 0)
			{
				special_input_config[total_special_input].InputNodeState = SIO_HAS_SIGNAL;
			}
			else
			{
				CIHmi_SendDebugTips("Warning!特殊输入配置配置错误！");
			}			
		}
		else if (TabCount == 2)
		{
			special_input_config[total_special_input].OutputNodeIndex = index_cmp(TempBuff);		
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!特殊输入配置查找不到该对象【%s】",TempBuff);
			}
		}
		else if (TabCount == 3)
		{
			if (strcmp_no_case(TempBuff,"Clear") == 0)
			{
				special_input_config[total_special_input].OutputNodeState = SCS_CLEARED;
			}
			else if (strcmp_no_case(TempBuff,"Normal") == 0)
			{
				special_input_config[total_special_input].OutputNodeState = SWS_NORMAL;
			}
			else if (strcmp_no_case(TempBuff,"Reverse") == 0)
			{
				special_input_config[total_special_input].OutputNodeState = SWS_REVERSE;
			}
			else if (strcmp_no_case(TempBuff,"U") == 0)
			{
				special_input_config[total_special_input].OutputNodeState = SGS_U;
			}
			else if (strcmp_no_case(TempBuff,"UU") == 0)
			{
				special_input_config[total_special_input].OutputNodeState = SGS_UU;
			}
			else if (strcmp_no_case(TempBuff,"LU") == 0)
			{
				special_input_config[total_special_input].OutputNodeState = SGS_LU;
			}
			else if (strcmp_no_case(TempBuff,"L") == 0)
			{
				special_input_config[total_special_input].OutputNodeState = SGS_L;
			}
			else if (strcmp_no_case(TempBuff,"LL") == 0)
			{
				special_input_config[total_special_input].OutputNodeState = SGS_LL;
			}
			else if (strcmp_no_case(TempBuff,"B") == 0)
			{
				special_input_config[total_special_input].OutputNodeState = SGS_B;
			}
			else if (strcmp_no_case(TempBuff,"YB") == 0)
			{
				special_input_config[total_special_input].OutputNodeState = SGS_YB;
			}
			else
			{
				CIHmi_SendDebugTips("Warning!特殊输入配置错误！");
			}
			total_special_input++;
			if (total_special_input > MAX_SPECIAL_INPUT)
			{
				CIHmi_SendDebugTips("Warning!特殊输入配置数量: %d > MAX_SPECIAL_INPUT(%d)，请联系开发人员！",total_special_input,MAX_SPECIAL_INPUT);
			}
		}
		else
		{
			CIHmi_SendDebugTips("Warning!【特殊输入配置文件错误】");
		}
	}
	/*特殊输出*/
	else if (strcmp_no_case(SpecialName,"TSSC") == 0)
	{
		if (TabCount == 0)
		{
			special_output_config[total_special_output].InputNodeIndex = index_cmp(TempBuff);		
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!特殊输出配置查找不到该对象【%s】",TempBuff);
			}
		}
		else if (TabCount == 1)
		{
			if (strcmp_no_case(TempBuff,"Input") == 0)
			{
				special_output_config[total_special_output].InputNodeState = SIO_HAS_SIGNAL;
			}
			else if (strcmp_no_case(TempBuff,"Clear") == 0)
			{
				special_output_config[total_special_output].InputNodeState = SCS_CLEARED;
			}
			else if (strcmp_no_case(TempBuff,"Normal") == 0)
			{
				special_output_config[total_special_output].InputNodeState = SWS_NORMAL;
			}
			else if (strcmp_no_case(TempBuff,"Reverse") == 0)
			{
				special_output_config[total_special_output].InputNodeState = SWS_REVERSE;
			}
			else if (strcmp_no_case(TempBuff,"U") == 0)
			{
				special_output_config[total_special_output].InputNodeState = SGS_U;
			}
			else if (strcmp_no_case(TempBuff,"UU") == 0)
			{
				special_output_config[total_special_output].InputNodeState = SGS_UU;
			}
			else if (strcmp_no_case(TempBuff,"LU") == 0)
			{
				special_output_config[total_special_output].InputNodeState = SGS_LU;
			}
			else if (strcmp_no_case(TempBuff,"L") == 0)
			{
				special_output_config[total_special_output].InputNodeState = SGS_L;
			}
			else if (strcmp_no_case(TempBuff,"LL") == 0)
			{
				special_output_config[total_special_output].InputNodeState = SGS_LL;
			}
			else if (strcmp_no_case(TempBuff,"B") == 0)
			{
				special_output_config[total_special_output].InputNodeState = SGS_B;
			}
			else if (strcmp_no_case(TempBuff,"YB") == 0)
			{
				special_output_config[total_special_output].InputNodeState = SGS_YB;
			}
			else
			{
				CIHmi_SendDebugTips("Warning!特殊输出配置错误！");
			}
		}
		else if (TabCount == 2)
		{
			special_output_config[total_special_output].OutputNodeIndex = index_cmp(TempBuff);		
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!特殊输出配置查找不到该对象【%s】",TempBuff);
			}
		}
		else if (TabCount == 3)
		{
			if (strcmp_no_case(TempBuff,"Output") == 0)
			{
				special_output_config[total_special_output].OutputNodeState = SIO_HAS_SIGNAL;
			}
			else
			{
				CIHmi_SendDebugTips("Warning!特殊输出配置错误！");
			}
			total_special_output++;
			if (total_special_output > MAX_SPECIAL_INPUT)
			{
				CIHmi_SendDebugTips("Warning!特殊输出配置数量: %d > MAX_SPECIAL_INPUT(%d)，请联系开发人员！",total_special_output,MAX_SPECIAL_INPUT);
			}
		}
		else
		{
			CIHmi_SendDebugTips("Warning!【特殊输出配置文件错误】");
		}
	}
	/*信号显示变更*/
	else if (strcmp_no_case(SpecialName,"XHXSBG") == 0)
	{
		if (TabCount == 0)
		{
			signal_show_change_config[total_signal_show_change].StartSignal = index_cmp(TempBuff);		
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!信号显示变更配置查找不到该对象【%s】",TempBuff);
			}
		}
		else if (TabCount == 1)
		{
			signal_show_change_config[total_signal_show_change].EndSignal = index_cmp(TempBuff);		
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!信号显示变更配置查找不到该对象【%s】",TempBuff);
			}			
		}
		else if (TabCount == 2)
		{
			if (strcmp_no_case(TempBuff,"L") == 0)
			{
				signal_show_change_config[total_signal_show_change].OldShow = SGS_L;
			}
			else if (strcmp_no_case(TempBuff,"U") == 0)
			{
				signal_show_change_config[total_signal_show_change].OldShow = SGS_U;
			}
			else if (strcmp_no_case(TempBuff,"UU") == 0)
			{
				signal_show_change_config[total_signal_show_change].OldShow = SGS_UU;
			}
			else if (strcmp_no_case(TempBuff,"LU") == 0)
			{
				signal_show_change_config[total_signal_show_change].OldShow = SGS_LU;
			}
			else if (strcmp_no_case(TempBuff,"LL") == 0)
			{
				signal_show_change_config[total_signal_show_change].OldShow = SGS_LL;
			}
			else
			{
				CIHmi_SendDebugTips("Warning!信号显示变更配置错误！");
			}
		}
		else if (TabCount == 3)
		{
			if (strcmp_no_case(TempBuff,"L") == 0)
			{
				signal_show_change_config[total_signal_show_change].NewShow = SGS_L;
			}
			else if (strcmp_no_case(TempBuff,"U") == 0)
			{
				signal_show_change_config[total_signal_show_change].NewShow = SGS_U;
			}
			else if (strcmp_no_case(TempBuff,"UU") == 0)
			{
				signal_show_change_config[total_signal_show_change].NewShow = SGS_UU;
			}
			else if (strcmp_no_case(TempBuff,"LU") == 0)
			{
				signal_show_change_config[total_signal_show_change].NewShow = SGS_LU;
			}
			else if (strcmp_no_case(TempBuff,"LL") == 0)
			{
				signal_show_change_config[total_signal_show_change].NewShow = SGS_LL;
			}
			else
			{
				CIHmi_SendDebugTips("Warning!信号显示变更配置错误！");
			}
			total_signal_show_change++;
			if (total_signal_show_change > MAX_SIGNAL_SHOW_CHANGE)
			{
				CIHmi_SendDebugTips("Warning!信号显示变更配置数量: %d > MAX_SIGNAL_SHOW_CHANGE(%d)，请联系开发人员！",total_signal_show_change,MAX_SIGNAL_SHOW_CHANGE);
			}
		}
		else
		{
			CIHmi_SendDebugTips("Warning!【信号显示变更配置文件错误】");
		}
	}
	/*信号延时开放*/
	else if (strcmp_no_case(SpecialName,"XHYSKF") == 0)
	{
		if (TabCount == 0)
		{
			signal_delay_open_config[total_signal_delay_open].SignalIndex = index_cmp(TempBuff);		
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!信号延时开放配置查找不到该对象【%s】",TempBuff);
			}
		}
		else if (TabCount == 1)
		{
			if (strcmp_no_case(TempBuff,"10") == 0)
			{
				signal_delay_open_config[total_signal_delay_open].TimerCounter = SECONDS_10;
			}
			else if (strcmp_no_case(TempBuff,"15") == 0)
			{
				signal_delay_open_config[total_signal_delay_open].TimerCounter = SECONDS_15;
			}
			else if (strcmp_no_case(TempBuff,"30") == 0)
			{
				signal_delay_open_config[total_signal_delay_open].TimerCounter = SECONDS_30;
			}
			else
			{
				CIHmi_SendDebugTips("Warning!信号延时开放配置错误！");
			}
			total_signal_delay_open++;
			if (total_signal_delay_open > MAX_SIGNAL_DELAY_OPEN)
			{
				CIHmi_SendDebugTips("Warning!信号延时开放配置数量: %d > MAX_SIGNAL_DELAY_OPEN(%d)，请联系开发人员！",total_signal_delay_open,MAX_SIGNAL_DELAY_OPEN);
			}
		}
		else
		{
			CIHmi_SendDebugTips("Warning!【信号延时开放配置文件错误】");
		}
	}
	/*轨道合并*/
	else if (strcmp_no_case(SpecialName,"GDHB") == 0)
	{
		if (TabCount == 0)
		{
			section_compose_config[total_section_compose].Index0 = index_cmp(TempBuff);		
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!轨道合并配置查找不到该对象【%s】",TempBuff);
			}
		}
		else if (TabCount == 1)
		{
			if (strcmp_no_case(TempBuff,"Input") == 0)
			{
				section_compose_config[total_section_compose].State0 = SIO_HAS_SIGNAL;
			}
			else if (strcmp_no_case(TempBuff,"Clear") == 0)
			{
				section_compose_config[total_section_compose].State0 = SCS_CLEARED;
			}
			else
			{
				CIHmi_SendDebugTips("Warning!轨道合并配置错误！");
			}
		}
		else if (TabCount == 2)
		{
			section_compose_config[total_section_compose].Index1 = index_cmp(TempBuff);		
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!轨道合并配置查找不到该对象【%s】",TempBuff);
			}
		}
		else if (TabCount == 3)
		{
			if (strcmp_no_case(TempBuff,"Input") == 0)
			{
				section_compose_config[total_section_compose].State1 = SIO_HAS_SIGNAL;
			}
			else if (strcmp_no_case(TempBuff,"Clear") == 0)
			{
				section_compose_config[total_section_compose].State1 = SCS_CLEARED;
			}
			else
			{
				CIHmi_SendDebugTips("Warning!轨道合并配置错误！");
			}
		}
		else if (TabCount == 4)
		{
			section_compose_config[total_section_compose].Index2 = index_cmp(TempBuff);		
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!轨道合并配置查找不到该对象【%s】",TempBuff);
			}
		}
		else if (TabCount == 5)
		{
			if (strcmp_no_case(TempBuff,"Input") == 0)
			{
				section_compose_config[total_section_compose].State2 = SIO_HAS_SIGNAL;
			}
			else if (strcmp_no_case(TempBuff,"Clear") == 0)
			{
				section_compose_config[total_section_compose].State2 = SCS_CLEARED;
			}
			else
			{
				CIHmi_SendDebugTips("Warning!轨道合并配置错误！");
			}
		}
		else if (TabCount == 6)
		{
			section_compose_config[total_section_compose].Index3 = index_cmp(TempBuff);		
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!轨道合并配置查找不到该对象【%s】",TempBuff);
			}
		}
		else if (TabCount == 7)
		{
			if (strcmp_no_case(TempBuff,"Input") == 0)
			{
				section_compose_config[total_section_compose].State3 = SIO_HAS_SIGNAL;
			}
			else if (strcmp_no_case(TempBuff,"Clear") == 0)
			{
				section_compose_config[total_section_compose].State3 = SCS_CLEARED;
			}
			else
			{
				CIHmi_SendDebugTips("Warning!轨道合并配置错误！");
			}
			total_section_compose++;
			if (total_section_compose > MAX_SECTION_COMPOSE)
			{
				CIHmi_SendDebugTips("Warning!轨道合并配置数量: %d > MAX_SECTION_COMPOSE(%d)，请联系开发人员！",total_section_compose,MAX_SECTION_COMPOSE);
			}
		}
		else
		{
			CIHmi_SendDebugTips("Warning!【轨道合并配置文件错误】");
		}
	}
	/*延续进路*/
	else if (strcmp_no_case(SpecialName,"YXJL") == 0)
	{
		//if (TabCount == 0)
		//{
		//	/*始端信号机*/
		//	YXJLCount++;
		//	if (YXJLCount < MAX_SUCCESSIVE_ROUTE)
		//	{
		//		successive_route_config[YXJLCount].StartSignalIndex = index_cmp(TempBuff);
		//	}
		//	else
		//	{
		//		CIHmi_SendDebugTips("警告！【延续进路数量%d超过上限MAX_SUCCESSIVE_ROUTE】",YXJLCount);
		//	}
		//}
		//else if (TabCount == 1)
		//{
		//	/*终端信号机*/
		//	successive_route_config[YXJLCount].EndSignalIndex = index_cmp(TempBuff);
		//}
		//else if (TabCount == 2)
		//{
		//	/*延续终端*/
		//	if (successive_route_config[YXJLCount].SuccessiveEndCount < MAX_SUCCESSIVE_END)
		//	{				
		//		successive_route_config[YXJLCount].SuccessiveEnd[successive_route_config[YXJLCount].SuccessiveEndCount] = index_cmp(TempBuff);
		//		successive_route_config[YXJLCount].SuccessiveEndCount++;
		//	}
		//	else
		//	{
		//		CIHmi_SendDebugTips("警告！【延续进路中终端数量%d超过上限MAX_SUCCESSIVE_END】",successive_route_config[YXJLCount].SuccessiveEndCount);
		//	}
		//}
		//else
		//{
		//	CIHmi_SendDebugTips("警告！【延续进路配置文件第%d行有错误】",YXJLCount++);
		//	YXJLCount--;
		//}
	}
	/*自动闭塞*/
	else if (strcmp_no_case(SpecialName,"ZDBS") == 0)
	{

	}
	/*三显示自动闭塞*/
	else if (strcmp_no_case(SpecialName,"ZDBS3") == 0)
	{

	}
	/*半自动闭塞*/
	else if (strcmp_no_case(SpecialName,"BZDBS") == 0)
	{

	}
	/*接近区段*/
	else if (strcmp_no_case(SpecialName,"JJQD") == 0)
	{

	}
	/*改方运行*/
	else if (strcmp_no_case(SpecialName,"GFYX") == 0)
	{

	}
	/*机务段同意*/
	else if (strcmp_no_case(SpecialName,"JWTY") == 0)
	{

	}
	/*场间联系*/
	else if (strcmp_no_case(SpecialName,"CJLX") == 0)
	{

	}
	/*临时限速*/
	else if (strcmp_no_case(SpecialName,"LSXS") == 0)
	{

	}
	/*特殊防护道岔*/
	else if (strcmp_no_case(SpecialName,"TSFHDC") == 0)
	{

	}
	/*普通道口*/
	else if (strcmp_no_case(SpecialName,"DK") == 0)
	{
		if (TabCount == 0)
		{
			total_high_cross++;
			if (total_high_cross > MAX_HIGH_CROSS)
			{
				CIHmi_SendDebugTips("Warning!道口配置数量: %d > MAX_HIGH_CROSS(%d)，请联系开发人员！",total_high_cross,MAX_HIGH_CROSS);
			}
			dk_config[total_high_cross - 1].AlarmIndex = index_cmp(TempBuff);
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!道口配置查找不到该对象【%s】",TempBuff);
			}
		}
		else if (TabCount == 1)
		{
			dk_config[total_high_cross - 1].SignalIndex = index_cmp(TempBuff);
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!道口配置查找不到该对象【%s】",TempBuff);
			}
		}
		else if (TabCount == 2)
		{
			if (strcmp_no_case(TempBuff,"ShuntingRoute") == 0)
			{
				dk_config[total_high_cross - 1].RouteType = RT_SHUNTING_ROUTE;
			}
			else if (strcmp_no_case(TempBuff,"TrainRoute") == 0)
			{
				dk_config[total_high_cross - 1].RouteType = RT_TRAIN_ROUTE;
			}
			else if (strcmp_no_case(TempBuff,"AllRoute") == 0)
			{
				dk_config[total_high_cross - 1].RouteType = RT_ERROR;
			}
			else
			{
				CIHmi_SendDebugTips("Warning!道口配置错误！");
			}
		}
		else if (TabCount == 3)
		{
			dk_config[total_high_cross - 1].StartSection = index_cmp(TempBuff);
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!道口配置查找不到该对象【%s】",TempBuff);
			}
		}
		else if (TabCount == 4)
		{
			dk_config[total_high_cross - 1].StopSection = index_cmp(TempBuff);
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!道口配置查找不到该对象【%s】",TempBuff);
			}
		}
		else if ((TabCount >= 5) && (TabCount < 15))
		{
			count = dk_config[total_high_cross - 1].SwitchsCount;
			if (CommaCount == 1)
			{
				dk_config[total_high_cross - 1].Switchs[count].SwitchIndex = index_cmp(TempBuff);
				if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
				{
					CIHmi_SendDebugTips("Error!道口配置查找不到该对象【%s】",TempBuff);
				}
			}
			if (CommaCount == 0)
			{
				if (strcmp_no_case(TempBuff,"Normal") == 0)
				{
					dk_config[total_high_cross - 1].Switchs[count].Location = SWS_NORMAL;
				}
				else if (strcmp_no_case(TempBuff,"Reverse") == 0)
				{
					dk_config[total_high_cross - 1].Switchs[count].Location = SWS_REVERSE;
				}
				else
				{
					CIHmi_SendDebugTips("Warning!道口配置错误！");
				}
				dk_config[total_high_cross - 1].SwitchsCount++;
			}
		}
		else
		{
			CIHmi_SendDebugTips("Warning!道口配置文件错误！");
		}
	}
	/*宝钢付原料站65#道口*/
	else if (strcmp_no_case(SpecialName,"DK_BGFYLZ65") == 0)
	{
		if (TabCount == 0)
		{
			dk_bgfylz65_config.section1 = index_cmp(TempBuff);
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!宝钢付原料站65#道口配置查找不到该对象【%s】",TempBuff);
			}
		}
		else if (TabCount == 1)
		{
			dk_bgfylz65_config.section2 = index_cmp(TempBuff);
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!宝钢付原料站65#道口配置查找不到该对象【%s】",TempBuff);
			}
		}
		else if (TabCount == 2)
		{
			dk_bgfylz65_config.QQD = index_cmp(TempBuff);
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!宝钢付原料站65#道口配置查找不到该对象【%s】",TempBuff);
			}
		}
		else if (TabCount == 3)
		{
			dk_bgfylz65_config.TYD = index_cmp(TempBuff);
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!宝钢付原料站65#道口配置查找不到该对象【%s】",TempBuff);
			}
		}
		else if (TabCount == 4)
		{
			dk_bgfylz65_config.XD = index_cmp(TempBuff);
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!宝钢付原料站65#道口配置查找不到该对象【%s】",TempBuff);
			}
		}
		else if (TabCount == 5)
		{
			dk_bgfylz65_config.SQD = index_cmp(TempBuff);
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!宝钢付原料站65#道口配置查找不到该对象【%s】",TempBuff);
			}
		}			
		else if (TabCount == 6)
		{
			dk_bgfylz65_config.TYA = index_cmp(TempBuff);
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!宝钢付原料站65#道口配置查找不到该对象【%s】",TempBuff);
			}
		}
		else if (TabCount == 7)
		{
			dk_bgfylz65_config.SQA = index_cmp(TempBuff);
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!宝钢付原料站65#道口配置查找不到该对象【%s】",TempBuff);
			}
		}
		else
		{
			CIHmi_SendDebugTips("Warning!宝钢付原料站65#道口配置文件错误！");
		}
	}
	/*宝钢成品库站53#道口*/
	else if (strcmp_no_case(SpecialName,"DK_BGCPKZ53") == 0)
	{
		if (TabCount == 0)
		{
			dk_bgcpkz53_config.AlarmIndex = index_cmp(TempBuff);
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!宝钢成品库站53#道口配置查找不到该对象【%s】",TempBuff);
			}
		}
		else if (TabCount == 1)
		{
			if (strcmp_no_case(TempBuff,"Alarm") == 0)
			{
				dk_bgcpkz53_config.AlarmState = SIO_NO_SIGNAL;
			}
			else if (strcmp_no_case(TempBuff,"NoAlarm") == 0)
			{
				dk_bgcpkz53_config.AlarmState = SIO_HAS_SIGNAL;
			}
			else
			{
				CIHmi_SendDebugTips("Warning!宝钢成品库站53#道口配置错误！");
			}
		}
		else if (TabCount == 2)
		{
			dk_bgcpkz53_config.SignalIndex = index_cmp(TempBuff);
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!宝钢成品库站53#道口配置查找不到该对象【%s】",TempBuff);
			}
		}
		else if (TabCount == 3)
		{
			if (strcmp_no_case(TempBuff,"B") == 0)
			{
				dk_bgcpkz53_config.SignalState = SGS_B;
			}
			else if (strcmp_no_case(TempBuff,"L") == 0)
			{
				dk_bgcpkz53_config.SignalState = SGS_L;
			}
			else if (strcmp_no_case(TempBuff,"U") == 0)
			{
				dk_bgcpkz53_config.SignalState = SGS_U;
			}
			else if (strcmp_no_case(TempBuff,"UU") == 0)
			{
				dk_bgcpkz53_config.SignalState = SGS_UU;
			}
			else
			{
				CIHmi_SendDebugTips("Warning!宝钢成品库站53#道口配置错误！");
			}
		}
		else if (TabCount == 4)
		{
			dk_bgcpkz53_config.Section1Index = index_cmp(TempBuff);
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!宝钢成品库站53#道口配置查找不到该对象【%s】",TempBuff);
			}
		}
		else if (TabCount == 5)
		{
			if (strcmp_no_case(TempBuff,"Occupied") == 0)
			{
				dk_bgcpkz53_config.Section1State = SCS_OCCUPIED;
			}
			else if (strcmp_no_case(TempBuff,"Clear") == 0)
			{
				dk_bgcpkz53_config.Section1State = SCS_CLEARED;
			}
			else
			{
				CIHmi_SendDebugTips("Warning!宝钢成品库站53#道口配置错误！");
			}
		}			
		else if (TabCount == 6)
		{
			dk_bgcpkz53_config.Section2Index = index_cmp(TempBuff);
			if ((strlen(TempBuff) != 0) && (index_cmp(TempBuff) == NO_INDEX))
			{
				CIHmi_SendDebugTips("Error!宝钢成品库站53#道口配置查找不到该对象【%s】",TempBuff);
			}
		}
		else if (TabCount == 7)
		{
			if (strcmp_no_case(TempBuff,"Occupied") == 0)
			{
				dk_bgcpkz53_config.Section2State = SCS_OCCUPIED;
			}
			else if (strcmp_no_case(TempBuff,"Clear") == 0)
			{
				dk_bgcpkz53_config.Section2State = SCS_CLEARED;
			}
			else
			{
				CIHmi_SendDebugTips("Warning!宝钢成品库站53#道口配置错误！");
			}
		}
		else
		{
			CIHmi_SendDebugTips("Warning!宝钢成品库站53#道口配置文件错误！");
		}
	}





}
