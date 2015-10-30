/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

文件名      : algorithms.h
作者        : 张彦升
版本        : 1.0
创建日期    : 2013年10月11日 13:29:04
用途        : 该文件当中保存一些算法程序
历史修改记录: 
**********************************************************************/

#ifndef _algorithms_h__
#define _algorithms_h__

#include "ci_header.h"

#define MD5_DIGEST_LENGTH 16

/*
功能描述    : 得到两个数的最大公约数，暂时只支持无符号类型
返回值      : 
参数        : 
作者        : 
日期        : 2013年10月11日 13:32:26
*/
CIAPI_FUNC(uint32_t) CIAlgorithm_Gcd(uint32_t x, uint32_t y);
/*
功能描述    : 得到两个数的最小公倍数。
返回值      : 
参数        : 
作者        : 张彦升
日期        : 2013年10月24日 14:37:04
*/
CIAPI_FUNC(uint32_t) CIAlgorithm_Lcm(uint16_t x,uint16_t y);
/*
功能描述    : 对crc32函数进行一次包装，提供我们自己的crc版本
返回值      : 计算出来的crc32值
参数        : buf需要计算crc的数据头指针，size为该数据的长度。
作者        : 张彦升
日期        : 2013年10月24日 15:13:05
*/
CIAPI_FUNC(uint32_t) CIAlgorithm_Crc32(const void* p_buf,uint32_t size);
/*
 功能描述    : 由于原来版本中使用该函数，其计算的值与上面的crc16不相同，为保持与控显机的
              crc一致暂时保留了该函数，后面会考虑两个函数的正确性决定使用哪一个函数
 返回值      : 成功为0，失败为-1
 参数        : 
 作者        : 张彦升
 日期        : 2014年2月17日 11:01:14
*/
CIAPI_FUNC(uint16_t) CIAlgorithm_Crc16(const void* p_data, uint32_t size);

/*
 功能描述    : 传入文件名，计算该文件的md5值
 返回值      : 成功为0，失败为-1
 参数        : @file_name 文件名
              @md5_sum 将md5计算完成后通过该参数传出
 作者        : 张彦升
 日期        : 2014年2月17日 12:40:36
*/
CIAPI_FUNC(uint32_t) CIAlgorithm_Md5(const char* p_file_name,
                                     unsigned char md5_sum[MD5_DIGEST_LENGTH]);

#endif /*_algorithms_h__*/
