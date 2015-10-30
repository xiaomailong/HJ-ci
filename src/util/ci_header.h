/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年12月10日 9:48:49
用途        : 联锁软件公用头文件
历史修改记录: v1.0    创建
**********************************************************************/
#ifndef _ci_header_h__
#define _ci_header_h__

#pragma pack(1)

#include <limits.h>
#ifndef UCHAR_MAX
#error "未能在limits.h当中找到UCHAR_MAX!"
#endif

#if UCHAR_MAX != 255
#error "请确认您的系统当中每个CHAR类型占用的是8bit!"
#endif

#include <stdio.h>
#ifndef NULL
#   error "未能在stdio.h当中找到对NULL的定义！"
#endif

#include <string.h>
#include <stdlib.h>

/* For size_t*/
#ifdef HAVE_STDDEF_H
#include <stddef.h>
#endif

/* CAUTION:
 * 请在编译过程当中使用-DNDEBUG选项，这样在最终版本当中assert()项会被移除
 */
#include <assert.h>

#ifdef WIN32
    typedef signed char int8_t;
    typedef signed short int int16_t;
    typedef signed int int32_t;
    typedef signed long long int64_t;
    typedef unsigned char uint8_t;
    typedef unsigned short uint16_t; 
    typedef unsigned int uint32_t; 
    typedef unsigned long long uint64_t;
#else
#   include <signal.h>         /* for SIG_ERR */
#   include <unistd.h>
#   include <time.h>   /*定时中断所需*/
#   include <sys/time.h>
#   include <sys/stat.h>   /*for S_IRUSR*/
#   include <stdint.h>
#   include <fcntl.h>

#   if	defined(SIG_IGN) && !defined(SIG_ERR)
#       define	SIG_ERR (void (*)(int))-1
#   endif

#endif /*WIN32*/
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#include <errno.h>
#include <ctype.h>

typedef uint8_t CI_BOOL;     /*联锁机用布尔类型*/ 

#ifndef CI_FALSE
#define CI_FALSE 0x0
#endif /*CI_FALSE*/

#ifndef CI_TRUE
#define CI_TRUE 0x5a
#endif /*CI_TRUE*/

#define IsTRUE(value)	 ((value)==CI_TRUE)     /*是否为真*/
#define IsFALSE(value)  ((value)!=CI_TRUE)      /*是否为假*/

/*不建议使用该类型，但是为了兼容以前的程序不得不使用该变量*/
typedef char char_t;
/*
 * 不建议使用string代表字符指针，使用char*更加直观，另外倘若有部分模块使用C++开发的话
 * 势必会造成冲突
 */
typedef char* string;

/*下面两个宏在原来的程序当中充当着打印调用栈信息的责任，不建议使用，其会打印过多信息*/
#define FUNCTION_OUT
#define FUNCTION_IN

/*
 * 对所有的函数及数据将使用这种方式进行封装，使用这种方式主要有三个目的：
 * 1.帮助编写其它模块代码的人迅速知晓一个函数或数据是可以使用的。
 * 2.方便向其它版本扩展，如添加__declspec(dllexport)。
 * 3.帮助阅读源代码的人很快的知道函数或数据是哪个模块的。
 * note!API函数及数据书写格式为：CIModule_ConcreteName，总体分为两部分，由_分割，前面部分
 * 由前缀和模块名组成，后面为具体的名称，两部分的命名方法都为驼峰命名。
 */
#ifndef CIAPI_FUNC
#   define CIAPI_FUNC(RTYPE) RTYPE
#endif
#ifndef CIAPI_DATA
#   define CIAPI_DATA(RTYPE) extern RTYPE
#endif

#ifndef WIN32
    typedef void (*sigactionhandler_t)(int,siginfo_t *,void *);
#endif /*WIN32*/

#ifdef WIN32
    /*在我们的程序当中所有对齐方式使用1字节对齐，应将该警告屏蔽掉*/
#   pragma warning(disable :4103)
    /*disable visual studio level4:未给出函数原型: 将“()”转换为“(void) */
#   pragma warning(disable :4255)
    /*printf not safe,use printf_s*/
#   pragma warning(disable :4996)
    /*warning C4127: 条件表达式是常量*/
#   pragma warning(disable :4127)
    /*未引用的形参*/
#   pragma warning(disable :4100)
#endif /*WIN32*/

/*support for unused parameter,variable,and function*/
#ifdef __GNUC__
#  define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
#else
#  define UNUSED(x) UNUSED_ ## x
#endif

#ifdef __GNUC__
#  define UNUSED_FUNCTION(x) __attribute__((__unused__)) UNUSED_ ## x
#else
#  define UNUSED_FUNCTION(x) UNUSED_ ## x
#endif

#endif /*_ci_header_h__*/
