/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年12月10日 9:48:16
用途        : 该模块当中放置通用的函数，这些函数不依赖于其它任何模块，它们通常
             更适合当中静态函数，以便编译器能够做更多的优化
历史修改记录: v1.0    创建
**********************************************************************/
#ifdef WIN32
#   include <Ws2tcpip.h>
#   include <windows.h>
#   pragma comment(lib,"WS2_32.lib")
#else
#   include <sys/time.h>
#   include <unistd.h>
#   include <string.h> /* for strncpy */
#   include <sys/types.h>
#   include <sys/socket.h>
#   include <sys/ioctl.h>
#   include <netinet/in.h>
#   include <net/if.h>
#   include <arpa/inet.h>
#endif /*WIN32*/

#   include <time.h>

#include "utility.h"
#include "algorithms.h"
#include "util/config.h"

/*
功能描述    : 打印CI_BOOL类型数据
返回值      : 
参数        : 
作者        : 何境泰
日期        : 2013年10月25日 8:49:36
*/
const char* CI_BoolPrint(CI_BOOL par)
{
    if (CI_FALSE == par)
    {
        return "CI_FALSE";
    }
    else if (CI_TRUE == par)
    {
        return "CI_TRUE";
    }
    else
    {
        return "UNKNOWN BOOL";
    }
}
/*
 功能描述    : 使用16进制方式打印数据，该函数不允许使用strcat等长度未知的函数
 返回值      : 
 参数        : @buf以char数组保存的数据
              @len数据的长度
 作者        : 张彦升
 日期        : 2014年3月5日 12:45:05
*/
void CI_HexDump(const void* p_data, int len)
{
    char str[80] = {0}, octet[10] = {0};
    const char* buf = (const char*)p_data;
    int ofs = 0, i = 0, l = 0;

    for (ofs = 0; ofs < len; ofs += 16)
    {
        memset(str,0,80);

        sprintf( str, "%07x: ", ofs );

        for (i = 0; i < 16; i++)
        {
            if ((i + ofs) < len)
            {
                sprintf( octet, "%02x ", buf[ofs + i] & 0xff );
            }
            else
            {
                strncpy( octet, "   ",3);
            }

            strncat(str,octet,3);
        }
        strncat(str,"  ",2);
        l = strlen(str);

        for (i = 0; (i < 16) && ((i + ofs) < len); i++)
        {
            str[l++] = isprint( buf[ofs + i] ) ? buf[ofs + i] : '.';
        }

        str[l] = '\0';
        printf( "%s\n", str );
    }
}
/*
 功能描述    : 将一段内存写入一个文件当中，方便调试
 返回值      : 成功为0，失败为-1
 参数        : @file_name 要存的文件名
              @data 要dump的数据指针
              @len 要dump的数据长度
 作者        : 张彦升
 日期        : 2014年4月9日 15:36:25
*/
int CI_DumpToFile(const char* p_file_name,const void* p_data,int len)
{
    int ret = 0;
    FILE *fp = NULL;

    fp = fopen(p_file_name,"wb");

    if (NULL == fp)
    {
        /*CILog_Errno("打开文件%s失败",p_file_name);*/
        return -1;
    }
    ret = fwrite(p_data,sizeof(char),len,fp);
    if (len != ret)
    {
        /*CILog_Errno("写入文件%s失败",p_file_name);*/
        return -1;
    }
    fclose(fp);
    
    return 0;
}
/*
功能描述    : 信号定时器安全dump函数
返回值      : 成功为0，失败为-1
参数        : 
作者        : 张彦升
日期        : 2014年4月17日 14:56:31
*/
int32_t CI_SafeDumpToFile(const char* p_file_name,const void* p_data,int32_t len)
{
#ifdef WIN32
    /*windows下不会使用信号触发，所以不用管是否安全*/
    CI_DumpToFile(p_file_name,p_data,len);

    return -1;
#else

    int fd = 0;
    int ret = 0;

    fd = open(p_file_name,O_WRONLY | O_CREAT,S_IRUSR);

    if (-1 == fd)
    {
        /*CILog_SigSafeErrno("打开文件失败:%s",p_file_name);*/
        return -1;
    }
    ret = write(fd,p_data,len);
    if (-1 == ret)
    {
        /*CILog_SigSafeErrno("写入文件失败:%s",p_file_name);*/
        return -1;
    }
    close(fd);

    return 0;
#endif /*WIN32*/

}
/*
 功能描述    : 将无符号的md5串转换成可打印的字符串
 返回值      : 转换成功后的buf指针，方便打印
 参数        : @md5要转换md5串
              @buf转换成功后要写入的串
              @len md5串的长度
 作者        : 张彦升
 日期        : 2014年5月13日 9:53:41
*/
const unsigned char* CI_Md5ToStr(const unsigned char* p_md5,unsigned char* p_buf,int32_t len)
{
    static const char digits[17] = "0123456789abcdef";
    int i = 0;
    int tmp = 0;

    if(2 * MD5_DIGEST_LENGTH > len)
    {
        return NULL;
    }

    for (i = 0;i < MD5_DIGEST_LENGTH;i++)
    {
        tmp = p_md5[i];
        p_buf[2 * i + 1] = digits[tmp % 16];
        tmp = tmp / 16;
        p_buf[2 * i] = digits[tmp % 16];
    }

    p_buf[2 * i] = 0;

    return p_buf;
}
/*
 功能描述    : 验证ip地址是否正确
 返回值      : 是正确的ip地址返回CI_TRUE，不是正确的ip地址返回CI_FALSE
 参数        : 无
 日期        : 2015年3月18日 13:31:48
*/
CI_BOOL CI_ValidateIpAddress(const char* ip)
{
    struct sockaddr_in sa;
    int result = 0;

    if (NULL == ip)
    {
        return CI_FALSE;
    }
    result = inet_pton(AF_INET, ip, &(sa.sin_addr));
    if (1 != result)
    {
        return CI_FALSE;
    }
    else
    {
        return CI_TRUE;
    }
}
/*
 功能描述    : 将ip地址转换为32位的整数
 返回值      : 成功为0，失败为-1
 参数        : @p_ip_buf 存有ip地址的字符串
 作者        : 张彦升
 日期        : 2014年3月27日 10:33:21
*/
int32_t CI_StrIpToInt32(const char* p_ip_buf,uint32_t* ip)
{
    int32_t i = 0;
    int32_t off = 0;
    int32_t value = 0;
    int32_t start_pos = 0;
    unsigned char* ptr = (unsigned char*)ip;

    if (CI_FALSE == CI_ValidateIpAddress(p_ip_buf) || NULL == ip)
    {
        return -1;
    }
    for (i = 0;i < 4;i++)
    {
        value = 0;
        start_pos = off;
        /*寻找.号*/
        while ('.' != p_ip_buf[off] && isdigit(p_ip_buf[off]))
        {
            value = value * 10 + p_ip_buf[off] - '0';
            off ++;
        }

        if (start_pos == off || 0 > value || 255 < value)
        {
            /*CILog_Msg("ip地址不正确:%s",p_ip_buf);*/
            return -1;
        }
        /*略过点号*/
        off ++;

        ptr[i] = (unsigned char)value;
    }

    return 0;
}
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
uint32_t CI_GetTimeStamp(void)
{
    uint32_t timestamp = 0;
#ifdef WIN32
    SYSTEMTIME cur_timeval;
    GetLocalTime(&cur_timeval);
    timestamp += cur_timeval.wDay * 24 * 3600;
    timestamp += cur_timeval.wHour * 3600;
    timestamp += cur_timeval.wMinute * 60;
    timestamp += cur_timeval.wSecond;
    timestamp <<= 10;
    timestamp += cur_timeval.wMilliseconds;
#else
    /*由于该函数在中断函数及非中断函数内部都调用了，所以务必使用系统调用直接实现*/
    struct timeval cur_timeval;
    gettimeofday(&cur_timeval,NULL);
    timestamp = cur_timeval.tv_sec;
    /*linux下简单的将前面的时间部分不要，我们需要微妙级别*/
    timestamp *= 1000000;
    timestamp += cur_timeval.tv_usec;
#endif /*WIN32*/

    return timestamp;
}
/*
 功能描述    : 该函数实现按位取反功能
 返回值      : 成功为0，失败为-1
 参数        : @in 要取反的内存地址
              @out 取反后输出的内存地址
              @len 数据长度
 作者        : 张彦升
 日期        : 2014年7月11日 10:03:23
*/
int32_t CI_BitwiseNot(const void* in,void* out,int32_t len)
{
    uint8_t *p_in = (uint8_t*)in;
    uint8_t *p_out = (uint8_t*)out;

    int32_t i = 0;

    for (i = 0;i < len;i++)
    {
        *p_out = ~(*p_in);
        p_in ++;
        p_out ++;
    }

    return 0;
}
/*
功能描述    : 32位数字大小端字节互转
返回值      : 转换成功后的新数值
参数        : @val 要转换的值
作者        : 何境泰
日期        : 2014年5月23日 1:04:47
*/
uint32_t CI_Swap32(uint32_t val)
{
    return ((((val) & 0xff000000) >> 24) |
        (((val) & 0x00ff0000) >>  8) |
        (((val) & 0x0000ff00) <<  8) |
        (((val) & 0x000000ff) << 24));
}
/*
 功能描述    : 压缩信号节点表状态信息
 返回值      : 压缩成功后的数据
 参数        : @state 需要压缩的数据
 作者        : 何境泰
 日期        : 2014年8月13日 16:15:46
*/
uint8_t CI_CompressSntState(uint16_t state)
{
    switch (state)
    {
		case 0x0000: return 0x00;
		case 0x3333: return 0x11;
		case 0x33CC: return 0x12;
		case 0xCC33: return 0x13;
		case 0x55AA: return 0x31;
		case 0x6666: return 0x32;
		case 0x5AA5: return 0x33;
		case 0x6699: return 0x34;
		case 0x6969: return 0x35;
		case 0x6996: return 0x36;
		case 0x9669: return 0x37;
		case 0x5A5A: return 0x38;
		case 0x9696: return 0x39;
		case 0x9966: return 0x3A;
		case 0x9999: return 0x3B;		
		case 0xAAAA: return 0x3C;
		case 0xCCCC: return 0x3D;
		case 0xA55A: return 0x3E;
		case 0xAA55: return 0x3F;		
		case 0x3CC3: return 0x51;
		case 0x5555: return 0x52;
		case 0xC33C: return 0x71;
		case 0xC3C3: return 0x72;
		case 0x3C3C: return 0x91;
		case 0xA5A5: return 0x92;

		/*挤岔报警状态*/
		case 0xAC01: return 0x95;
		case 0xAC02: return 0x96;
		case 0xAC03: return 0x97;
		case 0xAC04: return 0x98;

		/*半自动闭塞状态*/
		case 0xAB01: return 0xB1;
		case 0xAB02: return 0xB2;
		case 0xAB03: return 0xB3;
		case 0xAB04: return 0xB4;
		case 0xAB05: return 0xB5;
		case 0xAB06: return 0xB6;
		case 0xAB07: return 0xB7;
		case 0xAB08: return 0xB8;
		case 0xAB09: return 0xB9;
		case 0xAB0A: return 0xBA;
		case 0xAB0B: return 0xBB;
		case 0xAB0C: return 0xBC;
		case 0xAB0D: return 0xBD;
		case 0xAB0E: return 0xBE;
		case 0xAB0F: return 0xBF;
		case 0xAB10: return 0xC0;
		case 0xAB11: return 0xC1;
		case 0xAB12: return 0xC2;
		case 0xAB13: return 0xC3;
		case 0xAB14: return 0xC4;
		case 0xAB15: return 0xC5;
		case 0xAB16: return 0xC6;
		
		case (uint16_t)-1: return 0xFF;
		default:
			assert(0);
			return 0;
    }
}
/*
 功能描述    : 解压信号节点表的状态信息
 返回值      : 解压后的状态信息
 参数        : @需要解压的数据
 作者        : 何境泰
 日期        : 2014年8月13日 16:16:38
*/
uint16_t CI_DecompressSntState(uint8_t state)
{
    switch(state)
    {
		case 0x00: return 0x0000;
		case 0x11: return 0x3333;
		case 0x12: return 0x33CC;
		case 0x13: return 0xCC33;
		case 0x31: return 0x55AA;
		case 0x32: return 0x6666;
		case 0x33: return 0x5AA5;
		case 0x34: return 0x6699;
		case 0x35: return 0x6969;
		case 0x36: return 0x6996;
		case 0x37: return 0x9669;
		case 0x38: return 0x5A5A;
		case 0x39: return 0x9696;
		case 0x3A: return 0x9966;
		case 0x3B: return 0x9999;		
		case 0x3C: return 0xAAAA;
		case 0x3D: return 0xCCCC;
		case 0x3E: return 0xA55A;
		case 0x3F: return 0xAA55;		
		case 0x51: return 0x3CC3;
		case 0x52: return 0x5555;
		case 0x71: return 0xC33C;
		case 0x72: return 0xC3C3;
		case 0x91: return 0x3C3C;
		case 0x92: return 0xA5A5;


			/*挤岔报警状态*/

		case 0x95: return 0xAC01;
		case 0x96: return 0xAC02;
		case 0x97: return 0xAC03;
		case 0x98: return 0xAC04;

			/*半自动闭塞状态*/
		case 0xB1: return 0xAB01;
		case 0xB2: return 0xAB02;
		case 0xB3: return 0xAB03;
		case 0xB4: return 0xAB04;
		case 0xB5: return 0xAB05;
		case 0xB6: return 0xAB06;
		case 0xB7: return 0xAB07;
		case 0xB8: return 0xAB08;
		case 0xB9: return 0xAB09;
		case 0xBA: return 0xAB0A;
		case 0xBB: return 0xAB0B;
		case 0xBC: return 0xAB0C;
		case 0xBD: return 0xAB0D;
		case 0xBE: return 0xAB0E;
		case 0xBF: return 0xAB0F;
		case 0xC0: return 0xAB10;
		case 0xC1: return 0xAB11;
		case 0xC2: return 0xAB12;
		case 0xC3: return 0xAB13;
		case 0xC4: return 0xAB14;
		case 0xC5: return 0xAB15;
		case 0xC6: return 0xAB16;

		case 0xFF: return (uint16_t)-1;
		default:
			assert(0);
			return 0;
    }
}
/*
 功能描述    : 压缩command信息
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年6月23日 16:32:15
*/
uint16_t CI_CompressCommandState(uint32_t cmd)
{
    uint16_t high = (cmd >> 16) & 0xffff, low = cmd & 0xffff;

    union {
        struct {
            uint8_t h;
            uint8_t l;
        }d;

        uint16_t data;
    } dummy;

    dummy.d.h = CI_CompressSntState(high);
    dummy.d.l = CI_CompressSntState(low);

    return dummy.data;
}
/*
 功能描述    : 解压缩command信息
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年6月23日 16:32:15
*/
uint32_t CI_DecompressCommandState(uint16_t cmd)
{
    uint8_t high = (cmd >> 8) & 0xff, low = cmd & 0xff;
    union{
        struct {
            uint16_t h;
            uint16_t l;
        }d;

        uint32_t data;
    } dummy;

    dummy.d.h = CI_DecompressSntState(high);
    dummy.d.l = CI_DecompressSntState(low);

    return dummy.data;
}
/*
 功能描述    : 得到本机的ip地址信息，
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年3月14日 20:22:08
*/
const char* CI_LocalhostIp(void)
{
#ifdef WIN32
    /*TODO 请完善该函数并将其移动到正确的位置*/
    /*
     * Windows下也可以根据各种方法得到自身的ip地址，但是对于多网卡不好控制，
     * 还是留给用户自己配置较好
     */
    return "127.0.0.1";
#else
    int fd;
    struct ifreq ifr;
    const char* ethernet_name = NULL;

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    /* I want to get an IPv4 IP address */
    ifr.ifr_addr.sa_family = AF_INET;

#ifdef CI_UNIT_TEST
    ethernet_name = "eth0";
#else
    ethernet_name = CIConfig_GetValue("EthernetName");
    if (NULL == ethernet_name)
    {
        /*
         * REMAIN
         * 设计原则，由于对于utility模块的设计要求不能直接打印错误信息，所以这里
         * 对返回错误信息不明确。
         */
        return NULL;
    }
#endif /*CI_UNIT_TEST*/
    /*NOTICE!当网卡的名称不符合eth[0-9]的标准的时候，会返回149.186.101.*类的ip地址，
     * 所以本质上该函数的功能并不完善，应该谨慎使用*/
    strncpy(ifr.ifr_name, ethernet_name, IFNAMSIZ - 1);

    ioctl(fd, SIOCGIFADDR, &ifr);

    close(fd);

    return inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
#endif /*WIN32*/
}
/*
 功能描述    : 验证网络端口地址是否在正确范围内
 返回值      : 在范围内则为CI_TRUE，否则为CI_FALSE
 参数        : 无
 日期        : 2015年3月14日 20:22:08
*/
CI_BOOL CI_ValidatePort(int32_t port)
{
    if(1024 > port || 65535 < port)
    {
        return CI_FALSE;
    }

    return CI_TRUE;
}
