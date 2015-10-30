/***************************************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.
文件名:  read_SNT.c
作者:    hjh
版本 :   1.0	
创建日期:2011/12/7
用途:    信号节点数据配置到结构体
历史修改记录:      
2013/01/22 V1.2.1 hjh
	node_input_addr和node_output_addr中没有配置地址的执行单元将其地址置为0
***************************************************************************************/
#include <string.h>
#include "global_data.h"
#include "util/app_config.h"
#include "utility_function.h"
#include "platform/platform_api.h"

/*宏定义*/
#define MAX_ROW 10					/*数据缓冲区行的最大值*/
#define MAX_LINE 50					/*数据缓冲区列的最大值*/

/*全局变量*/
FILE* fp_SNT;
static char_t strbuff[MAX_ROW][MAX_LINE];	/*数据缓冲区*/
static int8_t count1 = 0,count2 = 0;		/*数组strbuff的下标*/
static int16_t node_num = 0;				/*信号节点索引号*/
static int8_t btn_num = 0;					/*按钮计数*/

/*函数声明*/
/****************************************************
函数名:    read_SNT
功能描述:  读SNT配置文件
返回值:    
作者  :    hjh
日期  ：   2011/12/7
****************************************************/
CI_BOOL read_SNT(char_t* StationName);
/****************************************************
函数名:    signal_node_struct
功能描述:  信号节点数据读到结构体
返回值:    
参数:      char_t * SNT_name
参数:      char_t * SSI_name
作者  :    hejh
日期  ：   2012/9/19
****************************************************/
CI_BOOL signal_node_struct(char_t* SNT_name,char_t *SSI_name);
/****************************************************
函数名:    read_snt_data
功能描述:  读snt文件
返回值:    
参数:      char str
参数:      int8_t tab_count
作者  :    hejh
日期  ：   2012/9/19
****************************************************/
CI_BOOL read_snt_data(char str,int8_t tab_count);
/****************************************************
函数名:    open_ssi
功能描述:  打开SSI文件并将节点读到device_name
返回值:    
参数:      char_t * filename
作者  :    hejh
日期  ：   2012/9/19
****************************************************/
CI_BOOL open_ssi(char_t* filename);
/****************************************************
函数名:    open_snt
功能描述:  打开SNT文件
返回值:    
参数:      char_t * filename
作者  :    hejh
日期  ：   2012/9/19
****************************************************/
CI_BOOL open_snt(char_t* filename);
/****************************************************
函数名:    clear_snt_array
功能描述:  清空数组
返回值:    
参数:      void
作者  :    hejh
日期  ：   2012/9/14
****************************************************/
void clear_snt_array(void);
/****************************************************
函数名:    save_ilt_data
功能描述:  保存ILT数据
返回值:    
参数:      char_t ch
作者  :    hejh
日期  ：   2012/9/15
****************************************************/
void save_snt_data(char_t ch);
/****************************************************
函数名:    signal_node_type
功能描述:  信号节点类型
返回值:    
参数:      char_t ch
作者  :    hejh
日期  ：   2012/9/15
****************************************************/
CI_BOOL signal_node_type( char_t ch);
/****************************************************
函数名:    signal_node_direction
功能描述:  信号节点方向
返回值:    
参数:      char_t ch
作者  :    hejh
日期  ：   2012/9/15
****************************************************/
CI_BOOL signal_node_direction(char_t ch);
/****************************************************
函数名:    signal_node_button
功能描述:  信号节点对应按钮
返回值:    
参数:      char_t ch
作者  :    hejh
日期  ：   2012/9/15
****************************************************/
CI_BOOL signal_node_button(char_t ch);
/****************************************************
函数名:    signal_node_property
功能描述:  信号节点属性
返回值:    
参数:      char_t ch
作者  :    hejh
日期  ：   2012/9/15
****************************************************/
CI_BOOL signal_node_property(char_t ch);
/****************************************************
函数名:    siganl_node_addproperty
功能描述:  信号节点附加属性
返回值:    
参数:      char_t ch
作者  :    hejh
日期  ：   2012/9/19
****************************************************/
CI_BOOL siganl_node_addproperty(char_t ch);
/****************************************************
函数名:    node_throat
功能描述:  信号节点所属咽喉
返回值:    
参数:      char_t ch
作者  :    hejh
日期  ：   2012/9/19
****************************************************/
CI_BOOL node_throat( char_t ch);
/****************************************************
函数名:    pre_node_index
功能描述:  前节点索引号
返回值:    
参数:      char_t ch
作者  :    hejh
日期  ：   2012/9/19
****************************************************/
CI_BOOL pre_node_index(char_t ch);
/****************************************************
函数名:    next_node_index
功能描述:  后节点索引号
返回值:    
参数:      char_t ch
作者  :    hejh
日期  ：   2012/9/19
****************************************************/
CI_BOOL next_node_index(char_t ch);
/****************************************************
函数名:    rev_node_index
功能描述:  岔后第二节点索引号
返回值:    
参数:      char_t ch
作者  :    hejh
日期  ：   2012/9/19
****************************************************/
CI_BOOL rev_node_index(char_t ch);
/****************************************************
函数名:    node_input_addr
功能描述:  节点输入地址
返回值:    
参数:      char_t ch
作者  :    hejh
日期  ：   2012/9/19
****************************************************/
CI_BOOL node_input_addr(char_t ch);
/****************************************************
函数名:    node_output_addr
功能描述:  节点输出地址
返回值:    
参数:      char_t ch
作者  :    hejh
日期  ：   2012/9/19
****************************************************/
CI_BOOL node_output_addr(char_t ch);
/****************************************************
函数名:    button_struct
功能描述:  按钮结构体
返回值:    
参数:      char_t * SNT_name
作者  :    hejh
日期  ：   2012/9/19
****************************************************/
CI_BOOL button_struct(char_t *SNT_name);
/****************************************************
函数名:    button_byte
功能描述:  按钮类型
返回值:    
参数:      char_t ch
作者  :    hejh
日期  ：   2012/9/19
****************************************************/
CI_BOOL button_byte(char_t ch);
/****************************************************
函数名:    index_cmp
功能描述:  根据字符串查找对应的索引号  
返回值:    1.索引号
		   2.-1
参数:      str
作者  :    hjh
日期  ：   2011/11/29
****************************************************/
extern int16_t index_cmp(char_t* str);
/****************************************************
函数名:    is_addr_repeated
功能描述:  检查信号节点地址重复
返回值:    
参数:      uint16_t node_count
参数:      uint16_t addr
作者  :    hejh
日期  ：   2012/9/21
****************************************************/
CI_BOOL is_addr_repeated(uint16_t node_count,uint16_t addr);
/****************************************************
函数名:    is_addr_available
功能描述:  检查信号节点地址超限
返回值:    
参数:      uint16_t addr
参数:      int8_t addr_type
作者  :    hejh
日期  ：   2012/9/21
****************************************************/
CI_BOOL is_addr_available(uint16_t addr,int8_t addr_type);

/*函数定义*/
/****************************************************
函数名:    read_SNT
功能描述:  读SNT配置文件
返回值:    
作者  :    hjh
日期  ：   2011/12/7
****************************************************/
CI_BOOL read_SNT(char_t* StationName)
{
	char_t SNT_filename[FILE_NAME_LENGTH] =APP_PATH PATH_SPERATOR "settings" PATH_SPERATOR;
	char_t SSI_filename[FILE_NAME_LENGTH] =APP_PATH PATH_SPERATOR "settings" PATH_SPERATOR;
	CI_BOOL result = CI_TRUE;

	/*拼接SNT,SSI文件名称*/
	strcat_check(SNT_filename,StationName,sizeof(SNT_filename));
	strcat_check(SNT_filename,".jdb",sizeof(SNT_filename));
	strcat_check(SSI_filename,StationName,sizeof(SSI_filename));
	strcat_check(SSI_filename,".syb",sizeof(SSI_filename));
	
	/*将数据读到信号节点结构体*/
	if (IsTRUE(signal_node_struct(SNT_filename,SSI_filename)))
	{
		/*将数据读到按钮结构体*/
		button_struct(SNT_filename);
	}
	/*数据读取失败*/
	else
	{
		result = CI_FALSE;
	}

	/*关闭文件*/
	if (fp_SNT)
	{
		fclose(fp_SNT);
	}
	return result;
}

/****************************************************
函数名:    signal_node_struct
功能描述:  信号节点数据读到结构体
返回值:    
参数:      char_t * SNT_name
参数:      char_t * SSI_name
作者  :    hejh
日期  ：   2012/9/19
****************************************************/
CI_BOOL signal_node_struct(char_t* SNT_name,char_t* SSI_name)
{
	CI_BOOL result = CI_TRUE;
	uint8_t ch_read;		/*已读到的字符*/
	uint8_t ch_last;		/*下次要读的字符*/
	int8_t tab_cnt = 0;	/*Tab空格计数*/

	/*确认文件已打开*/
	if (IsTRUE(open_ssi(SSI_name)) && IsTRUE(open_snt(SNT_name)))
	{
		/*对缓冲区进行清空操作*/
		clear_snt_array();
		while(!feof(fp_SNT))
		{
			/*读.SNT文件*/
			if (fread(&ch_read,1,1,fp_SNT))
			{
				if (ch_read != '"')
				{
					/*判断回车*/
					if(ch_read == '\r')
					{
						/*节点输出地址*/
						result = node_output_addr(ch_read);
						if ((fread(&ch_last,1,1,fp_SNT)) &&(ch_last == '\n'))
						{
							/*一行结束后将局部变量清零*/
							node_num++;
							tab_cnt = 0;
							clear_snt_array();
						}						
					}
					else
					{	
						/*Tab空格计数*/
						if (ch_read == '\t')
						{
							tab_cnt++;
						}
						/*处理SNT数据*/
						result = read_snt_data(ch_read,tab_cnt);						
					}
				}
				
			}
			if (IsFALSE(result))
			{
				break;
			}
		}
		/*读取的信号节点数量和TOTAL_SIGNAL_NODE不相同*/
		if (node_num != TOTAL_SIGNAL_NODE)
		{
			result = CI_FALSE;
			CIHmi_SendDebugTips("信号节点数量不符：联锁程序中TOTAL_SIGNAL_NODE是%d，信号节点表中总数是%d!",TOTAL_SIGNAL_NODE,node_num);	
		}
	}
	else
	{
		result = CI_FALSE;
		CIHmi_SendDebugTips("%s或%s打开文件失败!",SNT_name,SSI_name);	
	}
	return result;
}

/****************************************************
函数名:    open_ssi
功能描述:  打开SSI文件并将节点读到device_name
返回值:    
参数:      char_t * filename
作者  :    hejh
日期  ：   2012/9/19
****************************************************/
CI_BOOL open_ssi(char_t* filename)
{
	FILE* fp_SSI;
	CI_BOOL result = CI_TRUE;
	int32_t m;

	/*打开文件错误*/
	if ((fp_SSI = fopen(filename,"rb")) == NULL)
	{
		CIHmi_SendDebugTips("%s文件名错误，查找不到该文件!",filename);	
		result = CI_FALSE;
	}
	else
	{
		/*将.SSI数据读到数组*/
		for (m = 0; ((!feof(fp_SSI)) && (m < TOTAL_NAMES)); m++)
		{
			fscanf(fp_SSI,"%d\t%s\n",&m,(char*)&device_name[m]);
		}
		/*读取到的设备数量和TOTAL_NAMES不相同*/
		if (m >= TOTAL_NAMES)
		{
			CIHmi_SendDebugTips("信号节点数量不符：联锁程序中TOTAL_NAMES是%d，节点索引表中信号节点总数是%d!",TOTAL_NAMES,m);	
		}
	}
	/*关闭文件*/
	if (fp_SSI)
	{
		fclose(fp_SSI);
	}
	return result;
}

/****************************************************
函数名:    open_snt
功能描述:  打开SNT文件
返回值:    
参数:      char_t * filename
作者  :    hejh
日期  ：   2012/9/19
****************************************************/
CI_BOOL open_snt(char_t* filename)
{
	CI_BOOL result = CI_TRUE;

	/*打开文件错误*/
	if ((fp_SNT = fopen(filename,"rb")) == NULL)
	{
		CIHmi_SendDebugTips("%s文件名错误，查找不到该文件!",filename);	
		result = CI_FALSE;
	}
	
	return result;
}

/****************************************************
函数名:    read_snt_data
功能描述:  读snt文件
返回值:    
参数:      char str
参数:      int8_t tab_count
作者  :    hejh
日期  ：   2012/9/19
****************************************************/
CI_BOOL read_snt_data(char str,int8_t tab_count)
{
	CI_BOOL result = CI_TRUE;

	switch(tab_count)
	{
		case 1:
			/*保存数据*/
			if (str != '\t')
			{
				save_snt_data(str);
			}
			else
			{
				total_signal_nodes++;
			}
			break;
		case 2:
			/*节点类型*/
			result = signal_node_type(str);
			break;
		case 3:
			/*方向*/
			result = signal_node_direction(str);
			break;
		case 4:
			/*对应按钮*/
			result = signal_node_button(str);
			break;
		case 5:
			/*属性*/
			result = signal_node_property(str);
			break;
		case 6:
			/*附加属性*/
			result = siganl_node_addproperty(str);
			break;
		case 7:
			/*所属咽喉区*/
			result = node_throat(str);
			break;
		case 8:
			/*前节点*/
			result = pre_node_index(str);
			break;
		case 9:
			/*后节点*/
			result = next_node_index(str);
			break;
		case 10:
			/*岔后第二节点*/
			result = rev_node_index(str);
			break;
		case 11:
			/*节点输入地址*/
			result = node_input_addr(str);
			break;
		default:
			break;
	}
	return result;
}

/****************************************************
函数名:    clear_snt_array
功能描述:  清空数组
返回值:    
参数:      void
作者  :    hejh
日期  ：   2012/9/14
****************************************************/
void clear_snt_array(void)
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
函数名:    save_ilt_data
功能描述:  保存ILT数据
返回值:    
参数:      char_t ch
作者  :    hejh
日期  ：   2012/9/15
****************************************************/
void save_snt_data(char_t ch)
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
函数名:    signal_node_type
功能描述:  信号节点类型
返回值:    
参数:      char_t ch
作者  :    hejh
日期  ：   2012/9/15
****************************************************/
CI_BOOL signal_node_type( char_t ch)
{
	CI_BOOL result = CI_TRUE;

	/*读到tab时对数据进行处理*/
	if (ch == '\t')
	{
		/*进站信号机*/
		if (strcmp_no_case(strbuff[0],"NT_ENTRY_SIGNAL") == 0)
		{
			signal_nodes[node_num].type = NT_ENTRY_SIGNAL;
		}
		/*出站信号机*/
		else if (strcmp_no_case(strbuff[0],"NT_OUT_SIGNAL") == 0)
		{
			signal_nodes[node_num].type = NT_OUT_SIGNAL;
		}
		/*出站兼调车信号机*/
		else if (strcmp_no_case(strbuff[0],"NT_OUT_SHUNTING_SIGNAL") == 0)
		{
			signal_nodes[node_num].type = NT_OUT_SHUNTING_SIGNAL;
		}
		/*进路信号机*/
		else if (strcmp_no_case(strbuff[0],"NT_ROUTE_SIGNAL") == 0)
		{
			signal_nodes[node_num].type = NT_ROUTE_SIGNAL;
		}
		/*单置调车信号机*/
		else if (strcmp_no_case(strbuff[0],"NT_SINGLE_SHUNTING_SIGNAL") == 0)
		{
			signal_nodes[node_num].type = NT_SINGLE_SHUNTING_SIGNAL;
		}
		/*差置调车信号机*/
		else if (strcmp_no_case(strbuff[0],"NT_DIFF_SHUNTING_SIGNAL") == 0)
		{
			signal_nodes[node_num].type = NT_DIFF_SHUNTING_SIGNAL;
		}
		/*并置调车信号机*/
		else if (strcmp_no_case(strbuff[0],"NT_JUXTAPOSE_SHUNGTING_SIGNAL") == 0)
		{
			signal_nodes[node_num].type = NT_JUXTAPOSE_SHUNGTING_SIGNAL;
		}
		/*尽头线调车信号机*/
		else if (strcmp_no_case(strbuff[0],"NT_STUB_END_SHUNTING_SIGNAL") == 0)
		{
			signal_nodes[node_num].type = NT_STUB_END_SHUNTING_SIGNAL;
		}
		/*机务段调车信号机*/
		else if (strcmp_no_case(strbuff[0],"NT_LOCODEPOT_SHUNTING_SIGNAL") == 0)
		{
			signal_nodes[node_num].type = NT_LOCODEPOT_SHUNTING_SIGNAL;
		}
		/*复示信号机*/
		else if (strcmp_no_case(strbuff[0],"NT_REVIEW_SIGNAL") == 0)
		{
			signal_nodes[node_num].type = NT_REVIEW_SIGNAL;
		}
		/*驼峰信号机*/
		else if (strcmp_no_case(strbuff[0],"NT_HUMPING_SIGNAL") == 0)
		{
			signal_nodes[node_num].type = NT_HUMPING_SIGNAL;
		}
		/*道岔*/
		else if (strcmp_no_case(strbuff[0],"NT_SWITCH") == 0)
		{
			signal_nodes[node_num].type = NT_SWITCH;
		}
		/*股道*/
		else if (strcmp_no_case(strbuff[0],"NT_TRACK") == 0)
		{
			signal_nodes[node_num].type = NT_TRACK;
		}
		/*无岔区段*/
		else if (strcmp_no_case(strbuff[0],"NT_NON_SWITCH_SECTION") == 0)
		{
			signal_nodes[node_num].type = NT_NON_SWITCH_SECTION;
		}
		/*道岔区段*/
		else if (strcmp_no_case(strbuff[0],"NT_SWITCH_SECTION") == 0)
		{
			signal_nodes[node_num].type = NT_SWITCH_SECTION;
		}
		/*尽头线*/
		else if (strcmp_no_case(strbuff[0],"NT_STUB_END_SECTION") == 0)
		{
			signal_nodes[node_num].type = NT_STUB_END_SECTION;
		}
		/*机待线*/
		else if (strcmp_no_case(strbuff[0],"NT_LOCODEPOT_SECTION") == 0)
		{
			signal_nodes[node_num].type = NT_LOCODEPOT_SECTION;
		}
		/*接近区段*/
		else if (strcmp_no_case(strbuff[0],"NT_APP_DEP_SECTION") == 0)
		{
			signal_nodes[node_num].type = NT_APP_DEP_SECTION;
		}
		/*照查点*/
		else if (strcmp_no_case(strbuff[0],"NT_CHECK_POINT") == 0)
		{
			signal_nodes[node_num].type = NT_CHECK_POINT;
		}
		/*请求同意点*/
		else if (strcmp_no_case(strbuff[0],"NT_REQUEST_AGREE_POINT") == 0)
		{
			signal_nodes[node_num].type = NT_REQUEST_AGREE_POINT;
		}
		/*变更按钮*/
		else if (strcmp_no_case(strbuff[0],"NT_CHANGE_BUTTON") == 0)
		{
			signal_nodes[node_num].type = NT_CHANGE_BUTTON;
		}
		/*列车终端按钮*/
		else if (strcmp_no_case(strbuff[0],"NT_TRAIN_END_BUTTON") == 0)
		{
			signal_nodes[node_num].type = NT_TRAIN_END_BUTTON;
		}
		/*延续进路终端按钮*/
		else if (strcmp_no_case(strbuff[0],"NT_SUCCESSIVE_END_BUTTON") == 0)
		{
			signal_nodes[node_num].type = NT_SUCCESSIVE_END_BUTTON;
		}
		/*调车终端按钮*/
		else if (strcmp_no_case(strbuff[0],"NT_SHUNTING_END_BUTTON") == 0)
		{
			signal_nodes[node_num].type = NT_SHUNTING_END_BUTTON;
		}
		/*侵限绝缘*/
		else if (strcmp_no_case(strbuff[0],"NT_EXCEED_LIMIT") == 0)
		{
			signal_nodes[node_num].type = NT_EXCEED_LIMIT;
		}
		/*半自动闭塞*/
		else if (strcmp_no_case(strbuff[0],"NT_SEMI_AUTOMATIC_BLOCK") == 0)
		{
			signal_nodes[node_num].type = NT_SEMI_AUTOMATIC_BLOCK;
		}
		/*表示器*/
		else if (strcmp_no_case(strbuff[0],"NT_INDICATOR") == 0)
		{
			signal_nodes[node_num].type = NT_INDICATOR;
		}
		/*引总锁闭*/
		else if (strcmp_no_case(strbuff[0],"NT_THROAT_GUIDE_LOCK") == 0)
		{
			signal_nodes[node_num].type = NT_THROAT_GUIDE_LOCK;
		}
		/*总启动*/
		else if (strcmp_no_case(strbuff[0],"NT_ZQD") == 0)
		{
			signal_nodes[node_num].type = NT_ZQD;
		}
		/*灯丝报警*/
		else if (strcmp_no_case(strbuff[0],"NT_DSBJ") == 0)
		{
			signal_nodes[node_num].type = NT_DSBJ;
		}
		/*挤岔报警*/
		else if (strcmp_no_case(strbuff[0],"NT_JCBJ") == 0)
		{
			signal_nodes[node_num].type = NT_JCBJ;
		}
		/*输入*/
		else if (strcmp_no_case(strbuff[0],"NT_INPUT") == 0)
		{
			signal_nodes[node_num].type = NT_INPUT;
		}
		/*输出*/
		else if (strcmp_no_case(strbuff[0],"NT_OUTPUT") == 0)
		{
			signal_nodes[node_num].type = NT_OUTPUT;
		}
		/*输入输出*/
		else if (strcmp_no_case(strbuff[0],"NT_INPUTOUTPUT") == 0)
		{
			signal_nodes[node_num].type = NT_INPUTOUTPUT;
		}
		/*设备类型错误*/
		else
		{
			result = CI_FALSE;
			CIHmi_SendDebugTips("信号节点表文件中第%d行设备类型错误，查找不到该类型!",node_num + 1);		
		}
		/*数组清空*/
		clear_snt_array();
	}
	/*保存数据*/
	else
	{
		save_snt_data(ch);
	}
	return result;
}

/****************************************************
函数名:    signal_node_direction
功能描述:  信号节点方向
返回值:    
参数:      char_t ch
作者  :    hejh
日期  ：   2012/9/15
****************************************************/
CI_BOOL signal_node_direction(char_t ch)
{
	CI_BOOL result = CI_TRUE;

	/*读到tab时对数据进行处理*/
	if (ch == '\t')
	{
		if (strcmp_no_case(strbuff[0],"") != 0)
		{
			/*上行*/
			if (strcmp_no_case(strbuff[0],"DIR_UP") == 0)
			{
				signal_nodes[node_num].direction = DIR_UP;
			}
			/*下行*/
			else if (strcmp_no_case(strbuff[0],"DIR_DOWN") == 0)
			{
				signal_nodes[node_num].direction = DIR_DOWN;
			}
			/*上下行*/
			else if (strcmp_no_case(strbuff[0],"DIR_UP_DOWN") == 0)
			{
				signal_nodes[node_num].direction = DIR_UP_DOWN;
			}
			/*右上方向*/
			else if (strcmp_no_case(strbuff[0],"DIR_RIGHT_UP") == 0)
			{
				signal_nodes[node_num].direction = DIR_RIGHT_UP;
			}
			/*左上方向*/
			else if (strcmp_no_case(strbuff[0],"DIR_LEFT_UP") == 0)
			{
				signal_nodes[node_num].direction = DIR_LEFT_UP;
			}
			/*左下方向*/
			else if (strcmp_no_case(strbuff[0],"DIR_LEFT_DOWN") == 0)
			{
				signal_nodes[node_num].direction = DIR_LEFT_DOWN;
			}
			/*右下方向*/
			else if (strcmp_no_case(strbuff[0],"DIR_RIGHT_DOWN") == 0)
			{
				signal_nodes[node_num].direction = DIR_RIGHT_DOWN;
			}
			/*节点方向错误*/
			else
			{
				result = CI_FALSE;
				CIHmi_SendDebugTips("信号节点表文件中第%d行设备方向错误，查找不到!",node_num + 1);	
			}
			/*清空数组*/
			clear_snt_array();
		}
	}
	/*保存数据*/
	else
	{
		save_snt_data(ch);
	}
	return result;
}

/****************************************************
函数名:    signal_node_button
功能描述:  信号节点对应按钮
返回值:    
参数:      char_t ch
作者  :    hejh
日期  ：   2012/9/15
****************************************************/
CI_BOOL signal_node_button(char_t ch)
{
	CI_BOOL result = CI_TRUE;

	/*读到tab时对数据进行处理*/
	if (ch == '\t')
	{
		/*一个按钮*/
		if (count1 == 0)
		{
			signal_nodes[node_num].buttons[0] = index_cmp(strbuff[count1]);
			signal_nodes[node_num].buttons[1] = NO_NODE;
			if (signal_nodes[node_num].buttons[0] != NO_INDEX)
			{
				total_buttons++;
			}
			
		}
		/*两个按钮*/
		else if (count1 == 1)
		{
			signal_nodes[node_num].buttons[0] = index_cmp(strbuff[count1-1]);
			signal_nodes[node_num].buttons[1] = index_cmp(strbuff[count1]);
			total_buttons += 2;
		}
		/*三个按钮*/
		else if (count1 == 2)
		{
			/*半自动闭塞，先不考虑*/
			total_buttons += 3;
		}
		/*数据错误*/
		else
		{
			result = CI_TRUE;
			CIHmi_SendDebugTips("信号节点表文件中第%d行按钮数据错误!",node_num + 1);	
		}
		/*清空数组*/
		clear_snt_array();
	}
	/*保存数据*/
	else
	{
		save_snt_data(ch);
	}
	return result;
}

/****************************************************
函数名:    signal_node_property
功能描述:  信号节点属性
返回值:    
参数:      char_t ch
作者  :    hejh
日期  ：   2012/9/15
****************************************************/
CI_BOOL signal_node_property(char_t ch)
{
	CI_BOOL result = CI_TRUE;
	int16_t temp = NO_INDEX;
	//int16_t temp1 = NO_INDEX,temp2 = NO_INDEX;
	
	/*读到tab时对数据进行处理*/
	if (ch == '\t')
	{
		/*将与进站信号机关联的闭塞方式进行分类*/
		if (signal_nodes[node_num].type != NT_ENTRY_SIGNAL)
		{
			/*一架设备*/
			if (count1 == 0)
			{
				if (strcmp_no_case(strbuff[0],"") != 0)
				{
					temp = index_cmp(strbuff[count1]);
					if (temp != NO_INDEX)
					{
						signal_nodes[node_num].property = temp;
					}
					else
					{
						result = CI_FALSE;
						CIHmi_SendDebugTips("信号节点表文件中第%d行信号节点属性错误!",node_num + 1);	
					}
				}				
			}
			/*两组道岔*/
			if (count1 == 1)
			{
				/*判断不为空*/
				//if ((strcmp_no_case(strbuff[count1 - 1],"") != 0) && (strcmp_no_case(strbuff[count1],"") != 0))
				//{
				//	/*能匹配到索引号*/
				//	temp1 = index_cmp(strbuff[count1 - 1]);
				//	temp2 = index_cmp(strbuff[count1]);
				//	if ((temp1 != NO_INDEX) && (temp2 != NO_INDEX))
				//	{
				//		/*左移16位，位与*/
				//		signal_nodes[node_num].property = temp1;
				//		signal_nodes[node_num].property = ((signal_nodes[node_num].property & 0xFFFF) | (temp2 << 16));
				//	}
				//	else
				//	{
				//		result = CI_FALSE;
				//		PRINTF1("信号节点表文件中第%d行信号节点属性错误!",node_num + 1);	
				//	}
				//}
			}
		}
		clear_snt_array();	
	}
	/*保存数据*/
	else
	{
		save_snt_data(ch);
	}
	return result;
}

/****************************************************
函数名:    siganl_node_addproperty
功能描述:  信号节点附加属性
返回值:    
参数:      char_t ch
作者  :    hejh
日期  ：   2012/9/19
****************************************************/
CI_BOOL siganl_node_addproperty(char_t ch)
{
	uint8_t i;
	CI_BOOL result = CI_TRUE;
	int16_t temp = NO_INDEX;
	
	/*读到tab时对数据进行处理*/
	if (ch == '\t')
	{
		if (strcmp_no_case(strbuff[0],"") != 0)
		{
			/*道岔区段高4位需要填充为FFFF，否则默认为0000，会查找到索引号为0的道岔，出现错误*/
			if (signal_nodes[node_num].type == NT_SWITCH_SECTION)
			{
				temp = index_cmp(strbuff[0]);
				if (temp == NO_INDEX)
				{
					result = CI_FALSE;
					CIHmi_SendDebugTips("信号节点表文件中第%d行信号节点附加属性错误!",node_num + 1);	
				}
				else
				{
					signal_nodes[node_num].additional_property = temp | 0xFFFF0000;
				}
			}
			/*侵限绝缘*/
			else if (signal_nodes[node_num].type == NT_EXCEED_LIMIT)
			{
				/*条件道岔在反位*/
				if ('(' == strbuff[0][0])
				{
					for (i = 1; i<strlen(strbuff[0]);i++)
					{
						/*移位操作*/
						if (('/' == strbuff[0][i]) || (')' == strbuff[0][i]))
						{
							strbuff[0][i-1] = 0;
							break;
						}
						else
						{
							strbuff[0][i-1] = strbuff[0][i];
						}
					}
					/*反位*/
					temp = index_cmp(strbuff[0]);
					if (temp == NO_INDEX)
					{
						result = CI_FALSE;
						CIHmi_SendDebugTips("信号节点表文件中第%d行信号节点附加属性错误!",node_num + 1);	
					}
					else
					{
						signal_nodes[node_num].additional_property = (temp | 0xffff0000) & 0xff00ffff;
					}
				}
				/*条件道岔在定位*/
				else
				{
					temp = index_cmp(strbuff[0]);
					if (temp == NO_INDEX)
					{
						result = CI_FALSE;
						CIHmi_SendDebugTips("信号节点表文件中第%d行信号节点附加属性错误!",node_num + 1);	
					}
					else
					{
						signal_nodes[node_num].additional_property = (temp | 0xffff0000) & 0xff55ffff;
					}
				}
			}
			/*其他节点直接匹配*/
			else
			{
				temp = index_cmp(strbuff[0]);
				if (temp == NO_INDEX)
				{
					result = CI_FALSE;
					CIHmi_SendDebugTips("信号节点表文件中第%d行信号节点附加属性错误!",node_num + 1);	
				}
				else
				{
					signal_nodes[node_num].additional_property = temp;
				}
			}
		}		
		clear_snt_array();
	}
	/*保存数据*/
	else
	{
		save_snt_data(ch);
	}
	return result;
}

/****************************************************
函数名:    node_throat
功能描述:  信号节点所属咽喉
返回值:    
参数:      char_t ch
作者  :    hejh
日期  ：   2012/9/19
****************************************************/
CI_BOOL node_throat( char_t ch)
{
	CI_BOOL result = CI_TRUE;

	/*读到tab时对数据进行处理*/
	if (ch == '\t')
	{
		/*数据不为空*/
		if (strcmp_no_case(strbuff[0],"") != 0)
		{
			/*咽喉0*/
			if (strcmp_no_case(strbuff[0],"0") == 0)
			{
				signal_nodes[node_num].throat = 0;
			}
			/*咽喉1*/
			else if (strcmp_no_case(strbuff[0],"1") == 0)
			{
				signal_nodes[node_num].throat = 1;
			}
			/*咽喉1*/
			else if (strcmp_no_case(strbuff[0],"2") == 0)
			{
				signal_nodes[node_num].throat = 2;
			}
			/*咽喉1*/
			else if (strcmp_no_case(strbuff[0],"3") == 0)
			{
				signal_nodes[node_num].throat = 3;
			}
			/*咽喉1*/
			else if (strcmp_no_case(strbuff[0],"4") == 0)
			{
				signal_nodes[node_num].throat = 4;
			}
			/*咽喉1*/
			else if (strcmp_no_case(strbuff[0],"5") == 0)
			{
				signal_nodes[node_num].throat = 5;
			}
			/*数据错误*/
			else
			{
				result = CI_FALSE;
				CIHmi_SendDebugTips("信号节点表文件中第%d行设备所属咽喉区错误，查找不到!",node_num + 1);	
			}
			/*数组清空*/
			clear_snt_array();
		}
	}
	/*保存数据*/
	else
	{
		save_snt_data(ch);
	}
	return result;
}

/****************************************************
函数名:    pre_node_index
功能描述:  前节点索引号
返回值:    
参数:      char_t ch
作者  :    hejh
日期  ：   2012/9/19
****************************************************/
CI_BOOL pre_node_index(char_t ch)
{
	node_t temp;
	CI_BOOL result = CI_TRUE;
	
	/*读到tab时对数据进行处理*/
	if (ch == '\t')
	{
		/*数据不为空*/
		if (strcmp_no_case(strbuff[0],"") != 0)
		{
			temp = index_cmp(strbuff[0]);
			/*匹配到前节点索引号*/
			if (temp != NO_INDEX)
			{
				signal_nodes[node_num].previous_node_index = temp;
			}
			else
			{
				result = CI_FALSE;
				CIHmi_SendDebugTips("信号节点表文件中第%d行设备前节点错误，查找不到!",node_num + 1);	
			}
			clear_snt_array();
		}
	}
	/*保存数据*/
	else
	{
		save_snt_data(ch);
	}
	return result;
}

/****************************************************
函数名:    next_node_index
功能描述:  后节点索引号
返回值:    
参数:      char_t ch
作者  :    hejh
日期  ：   2012/9/19
****************************************************/
CI_BOOL next_node_index(char_t ch)
{
	node_t temp;
	CI_BOOL result = CI_TRUE;

	/*读到tab时对数据进行处理*/
	if (ch == '\t')
	{
		/*数据不为空*/
		if (strcmp_no_case(strbuff[0],"") != 0)
		{
			temp = index_cmp(strbuff[0]);
			/*匹配到索引号*/
			if (temp != NO_INDEX)
			{
				signal_nodes[node_num].next_node_index = temp;
			}
			else
			{
				result = CI_FALSE;
				CIHmi_SendDebugTips("信号节点表文件中第%d行设备后节点错误，查找不到!",node_num + 1);	
			}
			clear_snt_array();
		}
	}
	/*保存数据*/
	else
	{
		save_snt_data(ch);
	}
	return result;
}

/****************************************************
函数名:    rev_node_index
功能描述:  岔后第二节点索引号
返回值:    
参数:      char_t ch
作者  :    hejh
日期  ：   2012/9/19
****************************************************/
CI_BOOL rev_node_index(char_t ch)
{
	node_t temp;
	CI_BOOL result = CI_TRUE;

	/*读到tab时对数据进行处理*/
	if (ch == '\t')
	{
		/*数据不为空*/
		if (strcmp_no_case(strbuff[0],"") != 0)
		{
			temp = index_cmp(strbuff[0]);
			/*匹配到索引号*/
			if (temp != NO_INDEX)
			{
				signal_nodes[node_num].reverse_node_index = temp;
			}
			else
			{
				result = CI_FALSE;
				CIHmi_SendDebugTips("信号节点表文件中第%d行设备岔后第二节点错误，查找不到!",node_num + 1);	
			}
			clear_snt_array();
		}
	}
	/*保存数据*/
	else
	{
		/*读到-时，数据保存在下一行*/
		if (ch == '-')
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
	return result;
}

/****************************************************
函数名:    node_input_addr
功能描述:  节点输入地址
返回值:    
参数:      char_t ch
作者  :    hejh
日期  ：   2012/9/19
****************************************************/
CI_BOOL node_input_addr(char_t ch)
{
	uint8_t i;
	uint16_t temp = 0;
	uint16_t ga_temp = 0;
	CI_BOOL result = CI_TRUE;
	
	/*读到tab时对数据进行处理*/
	if (ch == '\t')
	{
		if ((strcmp_no_case(strbuff[0],"") != 0) 
		&& (strcmp_no_case(strbuff[1],"") != 0) 
		&& (strcmp_no_case(strbuff[2],"") != 0))
		{
			if (count1 == 2)
			{
				/*网关地址*/
				temp = 0;
				for ( i = 0; i < strlen(strbuff[count1-2]); i++)
				{
					temp = (strbuff[count1-2][i] - 48) + temp*10;
				}
				if (IsTRUE(is_addr_available(temp,0)))
				{
					ga_temp = temp;
					signal_nodes[node_num].input_address = (temp << 8) & 0xff00;
					if (ga_temp > total_getways)
					{
						total_getways = ga_temp;
					}
				}
				else
				{
					result = CI_FALSE;
					CIHmi_SendDebugTips("信号节点表文件中第%d行设备输入地址超出范围!",node_num + 1);	
				}
				/*执行单元地址*/
				temp = 0;
				for ( i = 0; i < strlen(strbuff[count1-1]); i++)
				{
					temp = (strbuff[count1-1][i] - 48) + temp*10;
				}
				if (IsTRUE(is_addr_available(temp,1)))
				{
					CIEeu_SetDataMask(ga_temp,temp);
					signal_nodes[node_num].input_address |= (temp << 2) & 0xfffc;
				}
				else
				{
					result = CI_FALSE;
					CIHmi_SendDebugTips("信号节点表文件中第%d行设备输入地址超出范围!",node_num + 1);
				}
				/*室外设备地址*/
				temp = 0;
				for ( i = 0; i < strlen(strbuff[count1]); i++)
				{
					temp =(strbuff[count1][i] - 48) + temp*10;
				}
				if (IsTRUE(is_addr_available(temp,2)))
				{
					signal_nodes[node_num].input_address |= temp;
				}
				else
				{
					result = CI_FALSE;
					CIHmi_SendDebugTips("信号节点表文件中第%d行设备输入地址超出范围!",node_num + 1);	
				}
				/*检查地址重复*/
				if (IsFALSE(is_addr_repeated(node_num,signal_nodes[node_num].input_address)))
				{
					signal_nodes[node_num].input_address = 0;
					result = CI_FALSE;
					CIHmi_SendDebugTips("信号节点表文件中第%d行设备输入地址重复!",node_num + 1);	
				}
			}
			else
			{
				result = CI_FALSE;
				CIHmi_SendDebugTips("信号节点表文件中第%d行设备输入地址错误!",node_num + 1);	
				signal_nodes[node_num].input_address = 0;
			}
		}
		else
		{
			/*hjh 2013-1-22 没有配置地址的执行单元将其地址置为0*/
			if (((strcmp_no_case(strbuff[0],"") == 0) 
				&& (strcmp_no_case(strbuff[1],"") == 0) 
				&& (strcmp_no_case(strbuff[2],"") == 0)))
			{
				signal_nodes[node_num].input_address = 0;
			}
			else
			{
				result = CI_FALSE;
				CIHmi_SendDebugTips("信号节点表文件中第%d行设备输入地址错误!",node_num + 1);	
				signal_nodes[node_num].input_address = 0;
			}
		}
		clear_snt_array();
	}
	/*保存数据*/
	else
	{
		/*读到-时，数据保存在下一行*/
		if (ch == '-')
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
	return result;
}

/****************************************************
函数名:    node_output_addr
功能描述:  节点输出地址
返回值:    
参数:      char_t ch
作者  :    hejh
日期  ：   2012/9/19
****************************************************/
CI_BOOL node_output_addr(char_t ch)
{
	uint8_t i;
	uint16_t temp = 0;
	CI_BOOL result = CI_TRUE;

	/*读到tab时对数据进行处理*/
	if (ch == '\r')
	{
		if ((strcmp_no_case(strbuff[0],"") != 0) 
			&& (strcmp_no_case(strbuff[1],"") != 0) 
			&& (strcmp_no_case(strbuff[2],"") != 0))
		{
			if (count1 == 2)
			{
				/*网关地址*/
				temp = 0;
				for ( i = 0; i < strlen(strbuff[count1-2]); i++)
				{
					temp = (strbuff[count1-2][i] - 48) + temp*10;
				}
				if (IsTRUE(is_addr_available(temp,0)))
				{
					signal_nodes[node_num].output_address = (temp << 8) & 0xff00;
				}
				else
				{
					result = CI_FALSE;
					CIHmi_SendDebugTips("信号节点表文件中第%d行设备输出地址超出范围!",node_num + 1);	
				}
				/*执行单元地址*/
				temp = 0;
				for ( i = 0; i < strlen(strbuff[count1-1]); i++)
				{
					temp = (strbuff[count1-1][i] - 48) + temp*10;
				}
				if (IsTRUE(is_addr_available(temp,1)))
				{
					signal_nodes[node_num].output_address |= (temp << 2) & 0xfffc;
				}
				else
				{
					result = CI_FALSE;
					CIHmi_SendDebugTips("信号节点表文件中第%d行设备输出地址超出范围!",node_num + 1);	
				}
				/*室外设备地址*/
				temp = 0;
				for ( i = 0; i < strlen(strbuff[count1]); i++)
				{
					temp = (strbuff[count1][i] - 48) + temp*10;
				}
				if (IsTRUE(is_addr_available(temp,2)))
				{
					signal_nodes[node_num].output_address |= temp;
				}
				else
				{
					result = CI_FALSE;
					CIHmi_SendDebugTips("信号节点表文件中第%d行设备输出地址超出范围!",node_num + 1);	
				}
				/*检查地址重复*/
				if (IsFALSE(is_addr_repeated(node_num,signal_nodes[node_num].output_address)))
				{
					signal_nodes[node_num].output_address = 0;
					result = CI_FALSE;
					CIHmi_SendDebugTips("信号节点表文件中第%d行设备输出地址重复!",node_num + 1);	
				}
			}
			else
			{
				result = CI_FALSE;
				CIHmi_SendDebugTips("信号节点表文件中第%d行设备输出地址错误!",node_num + 1);	
				signal_nodes[node_num].output_address = 0;
			}
		}
		else
		{
			/*hjh 2013-1-22 没有配置地址的执行单元将其地址置为0*/
			if (((strcmp_no_case(strbuff[0],"") == 0) 
				&& (strcmp_no_case(strbuff[1],"") == 0) 
				&& (strcmp_no_case(strbuff[2],"") == 0)))
			{
				signal_nodes[node_num].output_address = 0;
			}
			else
			{
				result = CI_FALSE;
				CIHmi_SendDebugTips("信号节点表文件中第%d行设备输出地址错误!",node_num + 1);	
				signal_nodes[node_num].output_address = 0;
			}
		}
		clear_snt_array();
	}
	return result;
}

/****************************************************
函数名:    button_struct
功能描述:  按钮结构体
返回值:    
参数:      char_t * SNT_name
作者  :    hejh
日期  ：   2012/9/19
****************************************************/
CI_BOOL button_struct(char_t* SNT_name)
{
	CI_BOOL result = CI_TRUE;
	char_t ch_read,ch_last;
	int8_t tab_cnt = 0;

	/*打开文件*/
	if (IsTRUE(open_snt(SNT_name)))
	{
		node_num = 0;

		/*按钮与信号节点配对()*/
		clear_snt_array();
		while(!feof(fp_SNT))
		{
			/*读信号节点表文件*/
			if (fread(&ch_read,1,1,fp_SNT))
			{
				if (ch_read != '"')
				{
					/*判断回车*/
					if(ch_read == '\r')
					{
						if ((fread(&ch_last,1,1,fp_SNT)) && (ch_last == '\n'))
						{
							node_num++;
							tab_cnt = 0;
						}					
					}
					/*Tab空格计数*/
					else if (ch_read == '\t')
					{
						tab_cnt++;
					}

					switch(tab_cnt)
					{
						case 3:
							/*保存数据*/
							if (ch_read != '\t')
							{
								save_snt_data(ch_read);
							}
							break;
						case 4:
							/*按钮类型*/
							result = button_byte(ch_read);
							break;
						default:
							break;
					}
				}
			}
			if (IsFALSE(result))
			{
				break;
			}
		}
		/*按钮数量和TOTAL_BUTTONS不相同*/
		if (btn_num != TOTAL_BUTTONS)
		{
			result = CI_FALSE;
			CIHmi_SendDebugTips("按钮数量不符：联锁程序中TOTAL_BUTTONS是%d，信号节点表中按钮总数是%d!",TOTAL_BUTTONS,btn_num);	
		}
	}
	/*文件错误*/
	else
	{
		result = CI_FALSE;
	}
	return result;
}

/****************************************************
函数名:    button_byte
功能描述:  按钮类型
返回值:    
参数:      char_t ch
作者  :    hejh
日期  ：   2012/9/19
****************************************************/
CI_BOOL button_byte(char_t ch)
{
	CI_BOOL result = CI_TRUE;
	EN_node_type nodeType = NT_NO;
	char_t str[MAX_LINE] = "";

	/*读到tab时对数据进行处理*/
	if (ch == '\t')
	{
		/*数据不为空*/
		if (strcmp_no_case(strbuff[0],"") != 0)
		{
			nodeType = gn_type(node_num);
			/*信号按钮*/
			if ((nodeType >= NT_ENTRY_SIGNAL)
				&& (nodeType < NT_SIGNAL))
			{
				/*一个按钮*/
				if (count1 == 0)
				{
					buttons[btn_num].node_index = node_num;
					strcat_check(str,strbuff[count1],sizeof(strbuff[count1]));
					buttons[btn_num].button_index = index_cmp(str);
					if (strbuff[count1][strlen(strbuff[count1]) - 1] == 'A')
					{
						/*通过按钮*/
						if (strbuff[count1][strlen(strbuff[count1]) - 2] == 'T')
						{
							buttons[btn_num].button_type = BT_PASSING;
						}
						/*列车按钮*/
						else if (strbuff[count1][strlen(strbuff[count1]) - 2] == 'L')
						{
							buttons[btn_num].button_type = BT_TRAIN;
						}
						/*调车按钮*/
						else
						{		
							buttons[btn_num].button_type = BT_SHUNTING;
						}						
					}
					else
					{
						result = CI_FALSE;
						CIHmi_SendDebugTips("信号节点表文件中第%d行按钮错误!",node_num);	
					}
				}
				/*两个按钮*/
				else if (count1 == 1)
				{
					buttons[btn_num].node_index = node_num;					
					if ((strbuff[count1 - 1][strlen(strbuff[count1 - 1]) - 1] == 'A') 
						&& (strbuff[count1][strlen(strbuff[count1]) - 1] == 'A'))
					{
						strcat_check(str,strbuff[count1 - 1],sizeof(strbuff[count1 - 1]));
						buttons[btn_num].button_index = index_cmp(str);
						/*通过按钮*/
						if (strbuff[count1 - 1][strlen(strbuff[count1 - 1]) - 2] == 'T')
						{
							buttons[btn_num].button_type = BT_PASSING;
						}
						/*列车按钮*/
						else if (strbuff[count1 - 1][strlen(strbuff[count1 - 1]) - 2] == 'L')
						{
							buttons[btn_num].button_type = BT_TRAIN;
						}
						/*调车按钮*/
						else
						{
							buttons[btn_num].button_type = BT_SHUNTING;
						}
						btn_num++;
						buttons[btn_num].node_index = node_num;
						strcat_check(str,strbuff[count1],sizeof(strbuff[count1]));
						buttons[btn_num].button_index = index_cmp(str);
						/*通过按钮*/
						if (strbuff[count1][strlen(strbuff[count1]) - 2] == 'T')
						{
							buttons[btn_num].button_type = BT_PASSING;
						}
						/*列车按钮*/
						else if (strbuff[count1][strlen(strbuff[count1]) - 2] == 'L')
						{
							buttons[btn_num].button_type = BT_TRAIN;
						}
						/*调车按钮*/
						else
						{
							buttons[btn_num].button_type = BT_SHUNTING;
						}
					}
					else
					{
						result = CI_FALSE;
						CIHmi_SendDebugTips("信号节点表文件中第%d行按钮错误!",node_num);	
					}				
				}
				else
				{
					result = CI_FALSE;
				}
			}
			/*引总锁闭按钮*/
			else if (nodeType == NT_THROAT_GUIDE_LOCK)
			{
				/*一个按钮*/
				if (count1 == 0)
				{
					buttons[btn_num].node_index = node_num;
					strcat_check(str,strbuff[count1],sizeof(strbuff[count1]));
					buttons[btn_num].button_index = index_cmp(str);
					if (strbuff[count1][strlen(strbuff[count1]) - 1] == 'A')
					{
						buttons[btn_num].button_type = BT_THROAT_GUIDE_LOCK;
					}
					else
					{
						result = CI_FALSE;
						CIHmi_SendDebugTips("信号节点表文件中第%d行按钮错误!",node_num);	
					}
				}
				else
				{
					result = CI_FALSE;
				}
			}
			/*挤岔报警按钮*/
			else if (nodeType == NT_JCBJ)
			{
				/*一个按钮*/
				if (count1 == 0)
				{
					buttons[btn_num].node_index = node_num;
					strcat_check(str,strbuff[count1],sizeof(strbuff[count1]));
					buttons[btn_num].button_index = index_cmp(str);
					if (strbuff[count1][strlen(strbuff[count1]) - 1] == 'A')
					{
						buttons[btn_num].button_type = BT_JCBJ;
					}
					else
					{
						result = CI_FALSE;
						CIHmi_SendDebugTips("信号节点表文件中第%d行按钮错误!",node_num);	
					}
				}
				else
				{
					result = CI_FALSE;
				}
			}
			/*输入按钮*/
			else if (nodeType == NT_INPUT)
			{
				/*一个按钮*/
				if (count1 == 0)
				{
					buttons[btn_num].node_index = node_num;
					strcat_check(str,strbuff[count1],sizeof(strbuff[count1]));
					buttons[btn_num].button_index = index_cmp(str);
					if (strbuff[count1][strlen(strbuff[count1]) - 1] == 'A')
					{
						buttons[btn_num].button_type = BT_INPUT;
					}
					else
					{
						result = CI_FALSE;
						CIHmi_SendDebugTips("信号节点表文件中第%d行按钮错误!",node_num);	
					}
				}
				else
				{
					result = CI_FALSE;
				}
			}
			/*输出按钮*/
			else if (nodeType == NT_OUTPUT)
			{
				/*一个按钮*/
				if (count1 == 0)
				{
					buttons[btn_num].node_index = node_num;
					strcat_check(str,strbuff[count1],sizeof(strbuff[count1]));
					buttons[btn_num].button_index = index_cmp(str);
					if (strbuff[count1][strlen(strbuff[count1]) - 1] == 'A')
					{
						buttons[btn_num].button_type = BT_OUTPUT;
					}
					else
					{
						result = CI_FALSE;
						CIHmi_SendDebugTips("信号节点表文件中第%d行按钮错误!",node_num);	
					}
				}
				else
				{
					result = CI_FALSE;
				}
			}
			/*输入输出按钮*/
			else if (nodeType == NT_INPUTOUTPUT)
			{
				/*一个按钮*/
				if (count1 == 0)
				{
					buttons[btn_num].node_index = node_num;
					strcat_check(str,strbuff[count1],sizeof(strbuff[count1]));
					buttons[btn_num].button_index = index_cmp(str);
					if (strbuff[count1][strlen(strbuff[count1]) - 1] == 'A')
					{
						buttons[btn_num].button_type = BT_INPUTOUTPUT;
					}
					else
					{
						result = CI_FALSE;
						CIHmi_SendDebugTips("信号节点表文件中第%d行按钮错误!",node_num);	
					}
				}
				else
				{
					result = CI_FALSE;
				}
			}
			/*半自动按钮*/
			else if (nodeType == NT_SEMI_AUTOMATIC_BLOCK)
			{
				/*三个按钮*/
				if (count1 == 2)
				{
					/*闭塞按钮*/
					buttons[btn_num].node_index = node_num;
					strcat_check(str,strbuff[count1 - 2],sizeof(strbuff[count1 - 2]));
					buttons[btn_num].button_index = index_cmp(str);
					buttons[btn_num].button_type = BT_SEMIAUTO_BLOCK;
					btn_num++;
					/*复原按钮*/
					buttons[btn_num].node_index = node_num;
					strcat_check(str,strbuff[count1 - 1],sizeof(strbuff[count1 - 1]));
					buttons[btn_num].button_index = index_cmp(str);
					buttons[btn_num].button_type = BT_SEMIAUTO_CANCEL;
					btn_num++;
					/*事故按钮*/
					buttons[btn_num].node_index = node_num;
					strcat_check(str,strbuff[count1],sizeof(strbuff[count1]));
					buttons[btn_num].button_index = index_cmp(str);
					buttons[btn_num].button_type = BT_SEMIAUTO_FAILURE;
				}
				else
				{
					result = CI_FALSE;
				}
			}
			else
			{
				/*一个按钮*/
				if (count1 == 0)
				{
					buttons[btn_num].node_index = node_num;
					strcat_check(str,strbuff[count1],sizeof(strbuff[count1]));
					buttons[btn_num].button_index = index_cmp(str);
					if (strbuff[count1][strlen(strbuff[count1]) - 1] == 'A')
					{
						buttons[btn_num].button_type = BT_AGREEMENT;
					}
					else
					{
						result = CI_FALSE;
						CIHmi_SendDebugTips("信号节点表文件中第%d行按钮错误!",node_num);	
					}
				}
				else
				{
					result = CI_FALSE;
				}
			}
			btn_num++;
			clear_snt_array();
		}
	}
	return result;
}

/****************************************************
函数名:    index_cmp
功能描述:  字符串比较
返回值:    
参数:      char_t * str
作者  :    hejh
日期  ：   2012/9/15
****************************************************/
int16_t index_cmp(char_t* str)
{
	int16_t a,b,c = NO_INDEX;
	
	/*数据不为空*/
	if (strlen(str) != 0)
	{
		for (a=0;device_name[a][0]!='\0';a++)
		{
			/*匹配索引号*/
			if (!strcmp_no_case(device_name[a],str))
			{
				c = a;
				/*清空数组*/
				for (b=0;*str!='\0';b++)
				{
					*(str+b) = 0;
				}
				break;
			}
		}
	}
	return (c);
}

/****************************************************
函数名:    is_addr_repeated
功能描述:  检查信号节点地址重复
返回值:    
参数:      uint16_t node_count
参数:      uint16_t addr
作者  :    hejh
日期  ：   2012/9/21
****************************************************/
CI_BOOL is_addr_repeated(uint16_t node_count,uint16_t addr)
{
	CI_BOOL result = CI_TRUE;
	uint16_t i;

	/*地址重复检查*/
	for (i = 0; i < node_count; i++)
	{
		/*输入地址重复*/
		if (signal_nodes[i].input_address == addr)
		{
			/*道岔需判断不是双动道岔*/
			if ((!((IsTRUE(is_switch(node_count))) && (signal_nodes[i].property == node_count))
				&& (gn_type(node_count) != NT_INPUT)
				&& (gn_type(node_count) != NT_OUTPUT)
				&& (gn_type(node_count) != NT_INPUTOUTPUT)))
			{
				result = CI_FALSE;
				break;
			}			
		}
		/*输出地址重复*/
		if (signal_nodes[i].output_address == addr)
		{
			/*道岔需判断不是双动道岔*/
			if ((!((IsTRUE(is_switch(node_count))) && (signal_nodes[i].property == node_count))
				&& (gn_type(node_count) != NT_INPUT)
				&& (gn_type(node_count) != NT_OUTPUT)
				&& (gn_type(node_count) != NT_INPUTOUTPUT)))
			{
				result = CI_FALSE;
				break;
			}
		}	
	}
	return result;
}

/****************************************************
函数名:    is_addr_available
功能描述:  检查信号节点地址超限
返回值:    
参数:      uint16_t addr
参数:      int8_t addr_type
作者  :    hejh
日期  ：   2012/9/21
****************************************************/
CI_BOOL is_addr_available(uint16_t addr,int8_t addr_type)
{
	CI_BOOL result = CI_TRUE;

	/*网关地址超限*/
	if ((addr_type == 0) && (addr >= MAX_GETWAYS))
	{
		result = CI_FALSE;
	}
	/*执行单元地址超限*/
	else if ((addr_type == 1) && (addr >= MAX_EEU_PER_LAYER))
	{
		result = CI_FALSE;
	}
	/*室外设备地址超限*/
	else if ((addr_type == 2) && (addr >= MAX_NODE_PEEU))
	{
		result = CI_FALSE;
	}
	else
	{
		result = CI_TRUE;
	}
	return result;
}