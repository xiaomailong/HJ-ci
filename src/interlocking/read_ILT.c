/***************************************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.
文件名:  read_ILT.c
作者:    hjh
版本 :   1.0	
创建日期:2011/12/7
用途:    联锁表数据配置到结构体
历史修改记录:         
***************************************************************************************/
#include <string.h>
#include "global_data.h"
#include "utility_function.h"

#include "platform/platform_api.h"

/*宏定义*/
#define MAX_ROW 20					/*数据缓冲区行的最大值*/
#define MAX_LINE 20					/*数据缓冲区列的最大值*/

/*全局变量*/
FILE* fp_ILT;						/*ILT文件指针*/
static char_t strbuff[MAX_ROW][MAX_LINE];	/*数据缓冲区*/
static int8_t count1 = 0,count2 = 0;		/*数组strbuff的下标*/
static int32_t path_num = 0;				/*联锁表进路索引号*/

/*函数声明*/
/****************************************************
函数名:    read_ILT
功能描述:  读ILT配置文件
返回值:    
参数:      ILT_filename
参数:      SSI_filename
作者  :    hjh
日期  ：   2011/12/7
****************************************************/
CI_BOOL read_ILT(char_t StationName[]);
/****************************************************
函数名:    ILT_Struct
功能描述:  联锁表结构体
返回值:    
作者  :    hjh
日期  ：   2011/12/2
****************************************************/
CI_BOOL ILT_struct();
/****************************************************
函数名:    read_ilt_data
功能描述:  读取联锁表数据
返回值:    
参数:      char str
参数:      int16_t tab_count
作者  :    hejh
日期  ：   2012/9/14
****************************************************/
CI_BOOL read_ilt_data(char str,int8_t tab_count);
/****************************************************
函数名:    save_ilt_data
功能描述:  保存ILT数据
返回值:    
参数:      char_t ch
作者  :    hejh
日期  ：   2012/9/15
****************************************************/
void save_ilt_data(char_t ch);
/****************************************************
函数名:    clear_array
功能描述:  清空数组
返回值:    
参数:      void
作者  :    hejh
日期  ：   2012/9/14
****************************************************/
void clear_array(void);
/****************************************************
函数名:    button_list
功能描述:  按钮列表
返回值:    
参数:      char_t ch
作者  :    hejh
日期  ：   2012/9/14
****************************************************/
CI_BOOL button_list(char_t ch);
/****************************************************
函数名:    routes_type
功能描述:  进路类型
返回值:    
参数:      char_t ch
作者  :    hejh
日期  ：   2012/9/14
****************************************************/
CI_BOOL routes_type(char_t ch);
/****************************************************
函数名:    route_direction
功能描述:  进路方向
返回值:    
参数:      char_t ch
作者  :    hejh
日期  ：   2012/9/14
****************************************************/
CI_BOOL route_direction(char_t ch);
/****************************************************
函数名:    start_show
功能描述:  始端信号显示
返回值:    
参数:      char_t ch
作者  :    hejh
日期  ：   2012/9/14
****************************************************/
CI_BOOL start_show(char_t ch);
/****************************************************
函数名:    switchs_list
功能描述:  道岔列表
返回值:    
参数:      char_t ch
作者  :    hejh
日期  ：   2012/9/14
****************************************************/
CI_BOOL switchs_list(char_t ch);
/****************************************************
函数名:    signal_track_list
功能描述:  敌对信号和轨道列表
返回值:    
参数:      char_t ch
参数:      int16_t count
作者  :    hejh
日期  ：   2012/9/14
****************************************************/
CI_BOOL signal_track_list(char_t ch,int8_t count);
/****************************************************
函数名:    face_list
功能描述:  迎面敌对
返回值:    
参数:      char_t ch
作者  :    hejh
日期  ：   2012/9/14
****************************************************/
CI_BOOL face_list(char_t ch);
/****************************************************
函数名:    index_cmp
功能描述:  根据字符串查找对应索引号
返回值:    1.索引号														   
		   2.-1
参数:      str
作者  :    hjh
日期  ：   2011/12/2
****************************************************/
extern int16_t index_cmp(char_t *str);
/****************************************************
函数名:    conflic_train_shunting
功能描述:  判断列车或调车敌对
返回值:    
参数:      str1
参数:      num1
参数:      num2
作者  :    hjh
日期  ：   2011/12/8
****************************************************/
CI_BOOL conflic_train_shunting(char_t *str1,int32_t num1,int8_t num2);


/*函数定义*/
/****************************************************
函数名:    read_ILT
功能描述:  读ILT配置文件
返回值:    
参数:      ILT_filename
参数:      SSI_filename
作者  :    hjh
日期  ：   2011/12/7
****************************************************/
CI_BOOL read_ILT(char_t StationName[])
{
	CI_BOOL result = CI_TRUE;
	char_t ILT_filename[FILE_NAME_LENGTH] = APP_PATH PATH_SPERATOR "settings" PATH_SPERATOR;

	/*拼接ILT文件名称*/
	strcat_check(ILT_filename,StationName,sizeof(ILT_filename));
	strcat_check(ILT_filename,".lsb",sizeof(ILT_filename));
	/*将数据读到联锁表结构体失败*/
	if (IsFALSE(ILT_struct(ILT_filename)))
	{
		result = CI_FALSE;
	}
	/*ILT文件结束*/
	if (fp_ILT)
	{
		fclose(fp_ILT);
	}
	return result;
}

/****************************************************
函数名:    ILT_struct
功能描述:  联锁表结构体
返回值:    
作者  :    hjh
日期  ：   2011/12/2
****************************************************/
CI_BOOL ILT_struct(char_t *ILT_name)
{
	CI_BOOL result = CI_TRUE;
	uint8_t ch_read = 0;	/*已读到的字符*/
	uint8_t ch_last = 0;	/*下次要读的字符*/
	int8_t ILT_tab_cnt=0;	/*Tab空格计数*/
	int16_t temp = NO_INDEX;

	/*打开文件失败*/
	if ((fp_ILT = fopen(ILT_name,"rb")) == NULL)
	{
		CIHmi_SendDebugTips("%s打开文件失败!",ILT_name);	
		result = CI_FALSE;
	}
	/*打开文件成功*/
	else
	{
		/*对缓冲区进行清空操作*/
		clear_array();
		while(!feof(fp_ILT))
		{
			/*读ILT文件*/
			if (fread(&ch_read,1,1,fp_ILT))
			{
				if (ch_read != '"')
				{
					/*判断回车*/
					if(ch_read == '\r')
					{				
						if ((fread(&ch_last,1,1,fp_ILT)) &&(ch_last == '\n'))
						{
							/*进路的接近区段*/
							if (strcmp_no_case(strbuff[0],"") != 0)
							{
								temp = index_cmp(strbuff[0]);
								if (temp == NO_INDEX)
								{
									result = CI_FALSE;
									CIHmi_SendDebugTips("联锁表文件中第%d行进路接近区段数据错误，查找不到!",path_num);	
								}
								else
								{
									ILT[path_num].approach_section = temp;
								}
							}
							/*一行结束后将局部变量清零*/
							path_num++;
							ILT_tab_cnt = 0;
							clear_array();
						}
					}
					else
					{
						/*Tab空格计数*/
						if (ch_read == '\t')
						{
							ILT_tab_cnt++;
						}
						/*处理ILT数据*/
						result = read_ilt_data(ch_read,ILT_tab_cnt);
					}
				}
			}
			if (IsFALSE(result))
			{
				break;
			}
		}
		/*读取的进路数量和TOTAL_ILT不一致*/
		if (path_num != TOTAL_ILT)
		{
			result = CI_FALSE;
			CIHmi_SendDebugTips("联锁表进路数量不符：联锁程序中TOTAL_ILT是%d，联锁表中进路总数是%d!",TOTAL_ILT,path_num);	
		}
	}
	return result;
}

/****************************************************
函数名:    read_ilt_data
功能描述:  读取联锁表数据
返回值:    
参数:      char str
参数:      int16_t tab_count
作者  :    hejh
日期  ：   2012/9/14
****************************************************/
CI_BOOL read_ilt_data(char str,int8_t tab_count)
{
	CI_BOOL result = CI_TRUE;

	switch (tab_count)
	{
		case 1:
			/*保存数据*/
			save_ilt_data(str);
			if (str == '\t')
			{
				total_ILTs++;
			}
			break;
		case 2:
			/*按钮列表数据处理*/
			result = button_list(str);
			break;
		case 3:
			/*进路类型数据处理*/
			result = routes_type(str);
			break;
		case 4:
			/*道岔数据处理*/
			result = switchs_list(str);
			break;
		case 5:
			/*敌对信号数据处理*/
			result = signal_track_list(str,tab_count);
			break;
		case 6:
			/*轨道区段数据处理*/
			result = signal_track_list(str,tab_count);
			break;
		case 7:/*迎面敌对数据处理*/
			result = face_list(str);
			break;
		default:
			/*没有数据处理*/
			break;
	}
	return result;
}

/****************************************************
函数名:    save_ilt_data
功能描述:  保存ILT数据
返回值:    
参数:      char_t ch
作者  :    hejh
日期  ：   2012/9/15
****************************************************/
void save_ilt_data(char_t ch)
{
	/*读取到的字符不是Tab*/
	if (ch != '\t')
	{
		/*读到,时，数据保存在下一行*/
		if (ch == ',')
		{
			count1++;
			count2 = 0;
		}
		/*为本行继续保存数据*/
		else
		{
			strbuff[count1][count2] = ch;
			count2++;
		}
	}
}

/****************************************************
函数名:    clear_array
功能描述:  清空数组
返回值:    
参数:      void
作者  :    hejh
日期  ：   2012/9/14
****************************************************/
void clear_array(void)
{
	int16_t m,n;

	/*对数组及下标进行清空操作*/
	for (m = 0; m < MAX_ROW; m++)
	{
		for (n = 0; n < MAX_LINE; n++)
		{
			strbuff[m][n] = 0;
		}
	}
	count1 = 0;
	count2 = 0;
}

/****************************************************
函数名:    button_list
功能描述:  按钮列表
返回值:    
参数:      char_t ch
作者  :    hejh
日期  ：   2012/9/14
****************************************************/
CI_BOOL button_list(char_t ch)
{
	CI_BOOL result = CI_TRUE;
	int16_t temp1 = NO_INDEX,temp2 = NO_INDEX,temp3 = NO_INDEX,temp4 = NO_INDEX;

	/*读到tab时对数据进行处理*/
	if (ch == '\t')
	{
		/*两个按钮*/
		if (count1 == 1)
		{
			temp1 = index_cmp(strbuff[count1-1]);
			temp2 = index_cmp(strbuff[count1]);
			if ((temp1 == NO_INDEX) || (temp2 == NO_INDEX))
			{
				result = CI_FALSE;
				CIHmi_SendDebugTips("联锁表文件中第%d行按钮数据错误!",path_num);	
			}
			else
			{
				ILT[path_num].start_button = temp1;
				ILT[path_num].end_button = temp2;
			}
		}
		/*三个按钮*/
		else if (count1 == 2)
		{
			temp1 = index_cmp(strbuff[count1-2]);
			temp2 = index_cmp(strbuff[count1-1]);
			temp3 = index_cmp(strbuff[count1]);
			if ((temp1 == NO_INDEX) || (temp2 == NO_INDEX) || (temp3 == NO_INDEX))
			{
				result = CI_FALSE;
				CIHmi_SendDebugTips("联锁表文件中第%d行按钮数据错误!",path_num);	
			}
			else
			{
				ILT[path_num].start_button = temp1;
				ILT[path_num].change_button = temp2;
				ILT[path_num].end_button = temp3;
			}	
		}
		/*四个按钮*/
		else if (count1 == 3)
		{
			temp1 = index_cmp(strbuff[count1-3]);
			temp2 = index_cmp(strbuff[count1-2]);
			temp3 = index_cmp(strbuff[count1-1]);
			temp4 = index_cmp(strbuff[count1]);
			if ((temp1 == NO_INDEX) || (temp2 == NO_INDEX) || (temp3 == NO_INDEX) || (temp4 == NO_INDEX))
			{
				result = CI_FALSE;
				CIHmi_SendDebugTips("联锁表文件中第%d行按钮数据错误!",path_num);	
			}
			else
			{
				ILT[path_num].start_button = temp1;
				ILT[path_num].change_button = temp2;
				ILT[path_num].change_button1 = temp3;
				ILT[path_num].end_button = temp4;
			}
		}
		/*数据错误*/
		else
		{
			CIHmi_SendDebugTips("联锁表文件中第%d行按钮数据错误!",path_num);	
		}
		/*处理完数据对数组清空*/
		clear_array();
	}
	/*保存数据*/
	else
	{
		save_ilt_data(ch);	
	}
	return result;
}

/****************************************************
函数名:    routes_type
功能描述:  进路类型
返回值:    
参数:      char_t ch
作者  :    hejh
日期  ：   2012/9/14
****************************************************/
CI_BOOL routes_type(char_t ch)
{
	CI_BOOL result = CI_TRUE;

	/*读到tab时对数据进行处理*/
	if (ch == '\t')
	{
		/*列车进路*/
		if (strcmp_no_case(strbuff[0],"RT_TRAIN_ROUTE") == 0)
		{
			ILT[path_num].route_kind = RT_TRAIN_ROUTE;
		}
		/*调车进路*/
		else if (strcmp_no_case(strbuff[0],"RT_SHUNTING_ROUTE") == 0)
		{
			ILT[path_num].route_kind = RT_SHUNTING_ROUTE;
		}
		/*延续进路*/
		else if (strcmp_no_case(strbuff[0],"RT_SUCCESSIVE_ROUTE") == 0)
		{
			ILT[path_num].route_kind = RT_SUCCESSIVE_ROUTE;
		}
		/*数据错误*/
		else
		{
			result = CI_FALSE;
			CIHmi_SendDebugTips("联锁表文件中第%d行进路类型错误，查找不到该类型!",path_num);	
		}
		/*清空数组*/
		clear_array();
	}
	/*保存数据*/
	else
	{
		save_ilt_data(ch);		
	}
	return result;
}

/****************************************************
函数名:    switchs_list
功能描述:  道岔列表
返回值:    
参数:      char_t ch
作者  :    hejh
日期  ：   2012/9/14
****************************************************/
CI_BOOL switchs_list(char_t ch)
{
	int16_t m,n,k,temp = NO_INDEX;
	CI_BOOL result = CI_TRUE,noSwitch = CI_FALSE;
	
	/*读到tab时对数据进行处理*/
	if (ch == '\t')
	{
		for (m = 0; m <= count1; m++)
		{
			/*防护道岔[]*/
			if (strbuff[m][0] == '[')
			{
				/*道岔在反位()*/
				if (strbuff[m][1] == '(')
				{
					for (n = 2;strbuff[m][n] != ')'; n++)
					{
						/*优化数据，只保留设备名称*/
						if (strbuff[m][n] == '/')
						{
							break;
						}
						else
						{
							strbuff[m][n-2] = strbuff[m][n];
						}
					}
					/*对无效的数组空间清空*/
					for (k = n - 2; k < MAX_ROW; k++)
					{
						strbuff[m][k] = 0;
					}
					temp = index_cmp(strbuff[m]);
					if (temp == NO_INDEX)
					{
						result = CI_FALSE;
						CIHmi_SendDebugTips("联锁表文件中第%d行道岔数据错误，查找不到!",path_num);	
						break;
					}
					else
					{
						ILT[path_num].switches[m].index = temp;
						ILT[path_num].switches[m].state = 0xAA00;	
					}
				}
				/*道岔在定位*/
				else
				{
					for (n = 1;strbuff[m][n] != ']'; n++)
					{
						/*优化数据，只保留设备名称*/
						if (strbuff[m][n] == '/')
						{
							break;
						}
						else
						{
							strbuff[m][n-1] = strbuff[m][n];
						}
					}
					/*对无效的数组空间清空*/
					for (k = n - 1; k < MAX_ROW; k++)
					{
						strbuff[m][k] = 0;
					}
					temp = index_cmp(strbuff[m]);
					if (temp == NO_INDEX)
					{
						result = CI_FALSE;
						CIHmi_SendDebugTips("联锁表文件中第%d行道岔数据错误，查找不到!",path_num);	
						break;
					}
					else
					{
						ILT[path_num].switches[m].index = temp;
						ILT[path_num].switches[m].state = 0xAA55;	
					}
				}
			}
			/*带动道岔{}*/
			else if (strbuff[m][0] == '{')
			{
				/*道岔在反位()*/
				if (strbuff[m][1] == '(')
				{
					for (n = 2;strbuff[m][n] != ')'; n++)
					{
						/*优化数据，只保留设备名称*/
						if (strbuff[m][n] == '/')
						{
							break;
						}
						else
						{
							strbuff[m][n-2] = strbuff[m][n];
						}
					}
					/*对无效的数组空间清空*/
					for (k = n - 2; k < MAX_ROW; k++)
					{
						strbuff[m][k] = 0;
					}
					temp = index_cmp(strbuff[m]);
					if (temp == NO_INDEX)
					{
						result = CI_FALSE;
						CIHmi_SendDebugTips("联锁表文件中第%d行道岔数据错误，查找不到!",path_num);	
						break;
					}
					else
					{
						ILT[path_num].switches[m].index = temp;
						ILT[path_num].switches[m].state = 0x5500;	
					}
				}
				/*道岔在定位*/
				else
				{
					for (n = 1;strbuff[m][n] != '}'; n++)
					{
						/*优化数据，只保留设备名称*/
						if (strbuff[m][n] == '/')
						{
							break;
						}
						else
						{
							strbuff[m][n-1] = strbuff[m][n];
						}
					}
					/*对无效的数组空间清空*/
					for (k = n - 1; k < MAX_ROW; k++)
					{
						strbuff[m][k] = 0;
					}
					temp = index_cmp(strbuff[m]);
					if (temp == NO_INDEX)
					{
						result = CI_FALSE;
						CIHmi_SendDebugTips("联锁表文件中第%d行道岔数据错误，查找不到!",path_num);	
						break;
					}
					else
					{
						ILT[path_num].switches[m].index = temp;
						ILT[path_num].switches[m].state = 0x5555;	
					}
				}
			}
			/*进路中正常道岔*/
			else
			{
				/*道岔在反位()*/
				if (strbuff[m][0] == '(')
				{
					for (n = 1;strbuff[m][n] != ')'; n++)
					{
						/*优化数据，只保留设备名称*/
						if (strbuff[m][n] == '/')
						{
							break;
						}
						else
						{
							strbuff[m][n-1] = strbuff[m][n];
						}
					}
					/*对无效的数组空间清空*/
					for (k = n - 1; k < MAX_ROW; k++)
					{
						strbuff[m][k] = 0;
					}
					temp = index_cmp(strbuff[m]);
					if (temp == NO_INDEX)
					{
						result = CI_FALSE;
						CIHmi_SendDebugTips("联锁表文件中第%d行道岔数据错误，查找不到!",path_num);	
						break;
					}
					else
					{
						ILT[path_num].switches[m].index = temp;
						ILT[path_num].switches[m].state = 0x0000;	
					}
				}
				/*道岔在定位*/
				else
				{
					for (n = 0;strbuff[m][n] != 0; n++)
					{
						/*优化数据，只保留设备名称*/
						if (strbuff[m][n] == '/')
						{
							break;
						}
					}
					/*对无效的数组空间清空*/
					for (k = n; k < MAX_ROW; k++)
					{
						strbuff[m][k] = 0;
					}
					/*结构体赋值*/
					temp = index_cmp(strbuff[m]);
					if (temp == NO_INDEX)
					{
						//result = CI_FALSE;
						noSwitch= CI_TRUE;
						CIHmi_SendDebugTips("联锁表文件中第%d行没有道岔，请注意!",path_num);	
						break;
					}
					else
					{
						ILT[path_num].switches[m].index = temp;
						ILT[path_num].switches[m].state = 0x0055;	
					}
				}
			}
		}
		if (IsFALSE(noSwitch))
		{
			ILT[path_num].switch_count = count1 + 1;
		}		
		clear_array();
	}
	/*保存数据*/
	else
	{
		save_ilt_data(ch);		
	}
	return result;
}

/****************************************************
函数名:    signal_track_list
功能描述:  敌对信号和轨道列表
返回值:    
参数:      char_t ch
参数:      int16_t count
作者  :    hejh
日期  ：   2012/9/14
****************************************************/
CI_BOOL signal_track_list(char_t ch,int8_t count)
{
	int8_t m,n,i,k;
	int16_t temp = NO_INDEX;
	char_t switch_name[MAX_ROW];
	CI_BOOL result = CI_TRUE;

	/*读到tab时对数据进行处理*/
	if (ch == '\t')
	{
		for (m = 0;m <= count1; m++)
		{
			/*条件敌对<>*/
			if (strbuff[m][0] == '<')
			{
				/*条件道岔在反位()*/
				if (strbuff[m][1] == '(')
				{
					/*查找道岔的索引号*/
					for (n = 2;strbuff[m][n] != ')'; n++)
					{
						/*优化数据，只保留设备名称*/
						if (strbuff[m][n] != '/')
						{
							switch_name[n-2] = strbuff[m][n];
							switch_name[n-1] = 0;
						}
					}
					/*结构体赋值*/
					temp = index_cmp(switch_name);
					if (temp == NO_INDEX)
					{
						result = CI_FALSE;
						CIHmi_SendDebugTips("联锁表文件中第%d行信号或轨道条件敌对道岔数据错误，查找不到!",path_num);	
						break;
					}
					else
					{
						if (count == 5)
						{
							ILT[path_num].signals[m].conditon_switch.index = temp;
							ILT[path_num].signals[m].conditon_switch.state = 0x0000;
						}
						if (count == 6)
						{
							ILT[path_num].tracks[m].conditon_switch.index = temp;
							ILT[path_num].tracks[m].conditon_switch.state = 0x0000;
						}
					}

					/*查找信号机或轨道的索引号*/
					for (k = n + 2; strbuff[m][k] != 0; k++)
					{
						strbuff[m][k-n-2] = strbuff[m][k];
					}
					/*对无效的数组空间清空*/
					for (i = (k - n -2); i < MAX_ROW; i++)
					{
						strbuff[m][i] = 0;
					}
					if (count == 5)
					{
						/*判断列车或调车敌对*/
						result = conflic_train_shunting(strbuff[m],path_num,m);
						if (result == CI_FALSE)
						{
							break;
						}
					}
					if (count == 6)
					{
						/*轨道区段结构体赋值*/
						temp = index_cmp(strbuff[m]);
						if (temp == NO_INDEX)
						{
							result = CI_FALSE;
							CIHmi_SendDebugTips("联锁表文件中第%d行轨道数据错误，查找不到!",path_num);	
							break;
						}
						else
						{
							ILT[path_num].tracks[m].index =temp;
						}
					}
				}
				/*条件道岔在定位*/
				else
				{
					/*查找道岔的索引号*/
					for (n = 1;strbuff[m][n] != '>'; n++)
					{
						if (strbuff[m][n] != '/')
						{
							switch_name[n-1] = strbuff[m][n];
							switch_name[n] = 0;
						}
					}
					/*结构体赋值*/
					temp = index_cmp(switch_name);
					if (temp == NO_INDEX)
					{
						result = CI_FALSE;
						CIHmi_SendDebugTips("联锁表文件中第%d行信号或轨道条件敌对道岔数据错误，查找不到!",path_num);	
						break;
					}
					else
					{
						if (count == 5)
						{
							ILT[path_num].signals[m].conditon_switch.index = temp;
							ILT[path_num].signals[m].conditon_switch.state = 0x0055;
						}
						if (count == 6)
						{
							ILT[path_num].tracks[m].conditon_switch.index = temp;
							ILT[path_num].tracks[m].conditon_switch.state = 0x0055;
						}
					}

					/*查找信号机或轨道的索引号*/
					for (k = n + 1; strbuff[m][k] != 0; k++)
					{
						strbuff[m][k-n-1] = strbuff[m][k];
					}
					/*对无效的数组空间清空*/
					for (i = (k - n - 1); i < MAX_ROW; i++)
					{
						strbuff[m][i] = 0;
					}
					if (count == 5)
					{
						/*判断列车或调车敌对*/
						result = conflic_train_shunting(strbuff[m],path_num,m);
						if (result == CI_FALSE)
						{
							break;
						}
					}
					if (count == 6)
					{
						/*轨道区段结构体赋值*/
						temp = index_cmp(strbuff[m]);
						if (temp == NO_INDEX)
						{
							result = CI_FALSE;
							CIHmi_SendDebugTips("联锁表文件中第%d行轨道数据错误，查找不到!",path_num);	
							break;
						}
						else
						{
							ILT[path_num].tracks[m].index =temp;
						}
					}
				}
			}
			/*无条件敌对*/
			else
			{
				if (count == 5)
				{
					/*判断列车或调车敌对*/
					result = conflic_train_shunting(strbuff[m],path_num,m);
					if (result == CI_FALSE)
					{
						break;
					}
				}
				if (count == 6)
				{
					/*轨道区段结构体赋值*/
					temp = index_cmp(strbuff[m]);
					if (temp == NO_INDEX)
					{
						result = CI_FALSE;
						CIHmi_SendDebugTips("联锁表文件中第%d行轨道数据错误，查找不到!",path_num);	
						break;
					}
					else
					{
						ILT[path_num].tracks[m].index =temp;
					}
				}
			}
		}
		/*信号机和轨道节点数赋值*/
		if (count == 5)
		{
			ILT[path_num].signal_count = count1 + 1;
		}
		if (count == 6)
		{
			ILT[path_num].track_count = count1 + 1;
		}
		clear_array();
	}
	/*保存数据*/
	else
	{
		save_ilt_data(ch);		
	}
	return result;
}

/****************************************************
函数名:    face_list
功能描述:  迎面敌对
返回值:    
参数:      char_t ch
作者  :    hejh
日期  ：   2012/9/14
****************************************************/
CI_BOOL face_list(char_t ch)
{
	CI_BOOL result = CI_TRUE;
	int16_t temp = NO_INDEX;
	
	/*读到tab时对数据进行处理*/
	if (ch == '\t')
	{
		/*有数据时*/
		if (count1 != 0)
		{
			/*列车迎面敌对*/
			if (strbuff[0][0] != 0)
			{
				temp = index_cmp(strbuff[0]);
				if (temp == NO_INDEX)
				{
					result = CI_FALSE;
					CIHmi_SendDebugTips("联锁表文件中第%d行列车迎面敌对数据错误，查找不到!",path_num);	
				}
				else
				{
					ILT[path_num].face_train_signal = temp;
				}
			}

			/*调车迎面敌对*/
			if (strbuff[1][0] != 0)
			{
				temp = index_cmp(strbuff[1]);
				if (temp == NO_INDEX)
				{
					result = CI_FALSE;
					CIHmi_SendDebugTips("联锁表文件中第%d行调车迎面敌对数据错误，查找不到!",path_num);	
				}
				else
				{
					ILT[path_num].face_shunting_signal = temp;
				}
			}
		}
		clear_array();
	}		
	/*保存数据*/
	else
	{
		save_ilt_data(ch);		
	}	
	return result;
}

/****************************************************
函数名:    conflic_train_shunting
功能描述:  判断列车或调车敌对
返回值:    
参数:      char_t * str
参数:      int16_t num1
参数:      int16_t num2
作者  :    hejh
日期  ：   2012/9/15
****************************************************/
CI_BOOL conflic_train_shunting(char_t *str,int32_t num1,int8_t num2)
{
	int16_t result;
	EN_node_type node_type;
	CI_BOOL judge_result = CI_TRUE;

	/*匹配信号节点*/
	result = index_cmp(str);
	/*无匹配值*/
	if (result == NO_INDEX)
	{
		/*列车敌对*/
		if ((str[strlen(str)-1] == 'L') || (str[strlen(str)-1] == '+'))
		{
			ILT[num1].signals[num2].conflict_signal_type = SST_TRAIN;
		}
		/*调车敌对*/
		else if ((str[strlen(str)-1] == 'D') || (str[strlen(str)-1] == '-'))
		{
			ILT[num1].signals[num2].conflict_signal_type = SST_SHUNTING;
		}
		/*数据错误*/
		else
		{
			judge_result = CI_FALSE;
			CIHmi_SendDebugTips("联锁表文件中第%d行敌对信号错误，查找不到!",path_num);	
		}
		str[strlen(str)-1] = 0;
		/*结构体赋值*/
		ILT[num1].signals[num2].index = index_cmp(str);
	}
	/*匹配出索引号*/
	else
	{
		/*获取节点类型*/
		node_type = gn_type(result);
		/*进站信号机或出站信号机*/
		if ((node_type == NT_ENTRY_SIGNAL) || (node_type == NT_OUT_SIGNAL))
		{
			ILT[num1].signals[num2].conflict_signal_type = SST_TRAIN;
		}
		/*调车信号机*/
		else if ((node_type == NT_SINGLE_SHUNTING_SIGNAL) || (node_type == NT_DIFF_SHUNTING_SIGNAL)
			 || (node_type == NT_JUXTAPOSE_SHUNGTING_SIGNAL) || (node_type == NT_STUB_END_SHUNTING_SIGNAL)
			 || (node_type == NT_LOCODEPOT_SHUNTING_SIGNAL) || (node_type == NT_HUMPING_SIGNAL))
		{
			ILT[num1].signals[num2].conflict_signal_type = SST_SHUNTING;
		}
		/*出站或进路信号机*/
		else if ((node_type == NT_OUT_SHUNTING_SIGNAL) || (node_type == NT_ROUTE_SIGNAL))
		{
			ILT[num1].signals[num2].conflict_signal_type = SST_TRAIN_SHUNTING;
		}
		/*数据错误*/
		else
		{
			judge_result = CI_FALSE;
			CIHmi_SendDebugTips("联锁表文件中第%d行敌对信号错误，查找不到!",path_num);	
		}
		/*结构体赋值*/
		ILT[num1].signals[num2].index = result;
	}
	return judge_result;
}

	