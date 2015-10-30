/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年12月10日 9:47:57
用途        : 该模块当中放置通用的函数
历史修改记录: v1.0    创建
**********************************************************************/
#ifndef _utility_h__
#define _utility_h__

#include "ci_header.h"

/*
功能描述    : 打印CI_BOOL类型数据
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2013年10月25日 8:49:36
*/
CIAPI_FUNC(const char*) CI_BoolPrint(CI_BOOL par);
/*
 功能描述    : 使用16进制方式打印数据，该函数不允许使用strcat等长度未知的函数
 返回值      : 
 参数        : @buf以char数组保存的数据
              @len数据的长度
 作者        : 张彦升
 日期        : 2014年3月5日 12:45:05
*/
CIAPI_FUNC(void) CI_HexDump(const void* p_data, int len);
/*
 功能描述    : 将一段内存写入一个文件当中，方便调试
 返回值      : 成功为0，失败为-1
 参数        : @file_name 要存的文件名
              @data 要dump的数据指针
              @len 要dump的数据长度
 作者        : 张彦升
 日期        : 2014年4月9日 15:36:25
*/
CIAPI_FUNC(int) CI_DumpToFile(const char* p_file_name,const void* p_data,int len);

/*
 功能描述    : 信号定时器安全dump函数
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年4月17日 14:56:31
*/
CIAPI_FUNC(int32_t) CI_SafeDumpToFile(const char* p_file_name,const void* p_data,int32_t len);
/*
 功能描述    : 将无符号的md5串转换成可打印的字符串
 返回值      : 转换成功后的buf指针，方便打印
 参数        : @md5要转换md5串
              @buf转换成功后要写入的串
              @len md5串的长度
 作者        : 张彦升
 日期        : 2014年5月13日 9:53:41
*/
CIAPI_FUNC(const unsigned char*) CI_Md5ToStr(const unsigned char* p_md5,
                                             unsigned char* p_buf,int32_t len);
/*
 功能描述    : 将ip地址转换为32位的整数
 返回值      : 成功为0，失败为-1
 参数        : @p_ip_buf 存有ip地址的字符串
 作者        : 张彦升
 日期        : 2014年3月27日 10:33:21
*/
CIAPI_FUNC(int32_t) CI_StrIpToInt32(const char* p_ip_buf,uint32_t* ip);
/*
功能描述    : 获取时间戳
            组成格式为：
            高位22位表示发送时间部分在本月中秒数累计值.
            低10位表示当前时间的毫秒部分。
            例如：发送时间为本月5日14时32分15秒595毫秒，则：
            TimeStamp = （5*24*3600 + 14*3600 + 32*60 + 15）* 1024 + 595
返回值      : 生成的32位时间戳
参数        : 无
作者        : 张彦升
日期        : 2013年12月10日 8:33:12
*/
CIAPI_FUNC(uint32_t) CI_GetTimeStamp(void);

/*
 功能描述    : 该函数实现按位取反功能
 返回值      : 成功为0，失败为-1
 参数        : @in 要取反的内存地址
              @out 取反后输出的内存地址
              @len 数据长度
 作者        : 张彦升
 日期        : 2014年7月11日 10:03:23
*/
CIAPI_FUNC(int32_t) CI_BitwiseNot(const void* p_in,void* p_out,int32_t len);
/*
 * Check at compile time that something is of a particular type.
 * Always evaluates to 1 so you may use it easily in comparisons.
 */
#define typecheck(type,x)               \
({      type __dummy;                   \
        typeof(x) __dummy2;             \
        (void)(&__dummy == &__dummy2);  \
        1;                              \
})
/*
 * 下面代码改编自 linux-3.12.4\include\linux\jiffies.h
 * 这段代码之所以能工作是因为以 UINT_MAX / 2为分割点来判断两个数的相对大小，判断两个
 * 无符号数相减的后得到的有符号数的符号位可达到我们的目的，见《深入理解计算机系统》
 * These inlines deal with sn wrapping correctly. You are 
 * strongly encouraged to use them
 * 1. Because people otherwise forget
 * 2. Because if the sn wrap changes in future you won't have to
 *    alter your driver code.
 *
 * sn_after(a,b) returns true if the sn a is after time b.
 *
 * Do this with "<0" and ">=0" to only test the sign of the result. A
 * good compiler would generate better code (and a really good compiler
 * wouldn't care). Gcc is currently neither.
 */
/*test if a after b*/
#define sn_after(a,b)           \
    (typecheck(uint32_t, a) &&  \
     typecheck(uint32_t, b) &&  \
     ((int32_t)((b) - (a)) < 0))

#define sn_before(a,b) sn_after(b,a)

#define sn_after_eq(a,b)            \
    (typecheck(uint32_t, a) && \
     typecheck(uint32_t, b) && \
     ((int32_t)((a) - (b)) >= 0))
#define sn_before_eq(a,b) sn_after_eq(b,a)

/*
功能描述    : 32位数字大小端字节互转
返回值      : 转换成功后的新数值
参数        : @val 要转换的值
作者        : 何境泰
日期        : 2014年5月23日 1:04:47
*/
CIAPI_FUNC(uint32_t) CI_Swap32(uint32_t val);
/*
 功能描述    : 压缩信号节点表状态信息
 返回值      : 压缩成功后的数据
 参数        : @state 需要压缩的数据
 作者        : 何境泰
 日期        : 2014年8月13日 16:15:46
*/
CIAPI_FUNC(uint8_t) CI_CompressSntState(uint16_t state);
/*
 功能描述    : 解压信号节点表的状态信息
 返回值      : 解压后的状态信息
 参数        : @需要解压的数据
 作者        : 何境泰
 日期        : 2014年8月13日 16:16:38
*/
CIAPI_FUNC(uint16_t) CI_DecompressSntState(uint8_t state);
/*
 功能描述    : 压缩command信息
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年6月23日 16:32:15
*/
CIAPI_FUNC(uint16_t) CI_CompressCommandState(uint32_t cmd);
/*
 功能描述    : 解压缩command信息
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年6月23日 16:32:15
*/
CIAPI_FUNC(uint32_t) CI_DecompressCommandState(uint16_t cmd);
/*
 功能描述    : 验证ip地址是否正确
 返回值      : 是正确的ip地址返回CI_TRUE，不是正确的ip地址返回CI_FALSE
 参数        : 无
 日期        : 2015年3月18日 13:31:48
*/
CIAPI_FUNC(CI_BOOL) CI_ValidateIpAddress(const char* ip);

/*
 功能描述    : 得到本机的ip地址信息，
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年3月14日 20:22:08
*/
CIAPI_FUNC(const char*) CI_LocalhostIp(void);

/*
 功能描述    : 验证网络端口地址是否在正确范围内
 返回值      : 在范围内则为CI_TRUE，否则为CI_FALSE
 参数        : 无
 日期        : 2015年3月14日 20:22:08
*/
CIAPI_FUNC(CI_BOOL) CI_ValidatePort(int32_t port);

#endif /*!_utility_h__*/
