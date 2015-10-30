/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

文件名      : config.h
作者        : 张彦升
版本        : 1.0
创建日期    : 2013年10月17日 8:45:41
用途        : 配置信息
历史修改记录:
    v1.0    创建
    v2.0    张彦升 改用xml作为配置文件
        我们应该接收一个现实：一个程序工作的是否正常取决于其初始状态。
        避免配置文件的配置错误能避免实际运行当中的大多数错误。将4个CPU的配置
        文件整合到一个文件当中能减少由配置错误而发生的故障，在实际当中每个CPU
        的配置文件只有选择的group id不同，而即使Group id配置相同了，由于
        该group的信息固定，所以在双CPU或双系配置比较同步的时候便能发现问题。
**********************************************************************/

#ifndef _config_h__
#define _config_h__

/*
 * 当查找配置文件内容的时候可以使用条件查找
 */
typedef struct _ConfigSelectCondition
{
    const char* name;
    const char* value;
}ConfigSelectCondition;
/*
 功能描述    : 从配置文件当中得到设备的id
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年3月27日 14:11:15
*/
CIAPI_FUNC(int32_t) CIConfig_GetDeviceId(void);
/*
功能描述    : 获取系统配置项值
参数        : @p_item 要得到的配置项的名称
返回值      : 若找不到则返回NULL
             若找到唯一的一项，则以字符串返回
日期        : 2013年11月18日 10:46:55
*/
CIAPI_FUNC(const char*) CIConfig_GetValue(const char* p_item);

/*
功能描述    : 获取系统配置项值
参数        : @p_item 要得到的配置项的名称
             @con 查询的条件，目前只支持一个条件查询
返回值      : 若找不到则返回NULL
             若查找到多项则将这些项全部打印出来，并返回NULL
             若找到唯一的一项，则以字符串返回
日期        : 2013年11月18日 10:46:55
*/
CIAPI_FUNC(const char*) CIConfig_SelectValue(const char* p_item,
                                             const ConfigSelectCondition* con);

/*
 功能描述    : 设置setting文件存放的目录
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年6月17日 13:47:17
*/
CIAPI_FUNC(int32_t) CIConfig_SetSettingPath(const char* path);
/*
 功能描述    : 初始化系统配置
 返回值      : 成功为0，失败为-1
 参数        : @file_path 配置文件的路径
 日期        : 2015年3月14日 17:27:52
*/
CIAPI_FUNC(int32_t) CIConfig_Init(void);

#endif /*!_config_h__*/
