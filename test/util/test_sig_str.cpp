#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <iostream>

using namespace std;

#define MAX_BUF 0xffff

#include "util/sig_str.c"

class TestSigStr : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }
    void SnprintfString(int32_t max_size, const char* fmt, const char* arg)
    {
        memset(buf1,0,MAX_BUF);
        memset(buf2,0,MAX_BUF);

        ret1 = CISigStr_Snprintf(buf1,max_size,fmt,arg);

#ifdef WIN32
        ret2 = _snprintf(buf2,max_size,fmt,arg);
        if(ret1 > max_size)
        {
            ASSERT_EQ(-1,ret2);
            /*修正_snprintf被截断时最后一个字符未置为0的bug*/
            buf2[max_size - 1] = 0;
        }
        /*当max_size正好为格式化后的长度时，CISigStr_Snprintf后面会使用0进行填充*/
        else if (ret1 == max_size)
        {
            buf2[max_size - 1] = 0;
        }
        else
        {
            EXPECT_EQ(ret1,ret2);
        }
#else
        ret2 = snprintf(buf2,max_size,fmt,arg);
        EXPECT_EQ(ret1,ret2);
#endif /* WIN32*/

        EXPECT_STREQ(buf1,buf2);
    }
    /*
     功能描述    : 使用%C，传入char
     返回值      : 成功为0，失败为-1
     参数        : 无
     日期        : 2015年3月24日 13:54:37
    */
    void SnprintfStringChar(int32_t max_size, const char* fmt, const char ch)
    {
        memset(buf1,0,MAX_BUF);
        memset(buf2,0,MAX_BUF);

        ret1 = CISigStr_Snprintf(buf1,max_size,fmt,ch);

#ifdef WIN32
        ret2 = _snprintf(buf2,max_size,fmt,ch);
        if(ret1 > max_size)
        {
            ASSERT_EQ(-1,ret2);
            /*修正_snprintf被截断时最后一个字符未置为0的bug*/
            buf2[max_size - 1] = 0;
        }
        /*当max_size正好为格式化后的长度时，CISigStr_Snprintf后面会使用0进行填充*/
        else if (ret1 == max_size)
        {
            buf2[max_size - 1] = 0;
        }
        else
        {
            EXPECT_EQ(ret1,ret2);
        }
#else
        ret2 = snprintf(buf2,max_size,fmt,ch);
        EXPECT_EQ(ret1,ret2);
#endif /* WIN32*/

        EXPECT_STREQ(buf1,buf2);
    }
    /*
     功能描述    : 测试只传入fmt，未传入其它参数
     返回值      : 成功为0，失败为-1
     参数        : 无
     日期        : 2015年3月24日 13:56:08
    */
    void SnprintfStringNoSub(int32_t max_size, const char* fmt)
    {
        memset(buf1,0,MAX_BUF);
        memset(buf2,0,MAX_BUF);

        ret1 = CISigStr_Snprintf(buf1,max_size,fmt);

#ifdef WIN32
        ret2 = _snprintf(buf2,max_size,fmt);
        if(ret1 > max_size)
        {
            ASSERT_EQ(-1,ret2);
            /*修正_snprintf被截断时最后一个字符未置为0的bug*/
            buf2[max_size - 1] = 0;
        }
        /*当max_size正好为格式化后的长度时，CISigStr_Snprintf后面会使用0进行填充*/
        else if (ret1 == max_size)
        {
            buf2[max_size - 1] = 0;
        }
        else
        {
            EXPECT_EQ(ret1,ret2);
        }
#else
        ret2 = snprintf(buf2,max_size,fmt);
        EXPECT_EQ(ret1,ret2);
#endif /* WIN32*/

        EXPECT_STREQ(buf1,buf2);
    }
    void SnprintfStringVariable(int32_t max_size,
                                    const char* fmt,
                                    int32_t precesion,
                                    const char* arg)
    {
        memset(buf1,0,MAX_BUF);
        memset(buf2,0,MAX_BUF);

        ret1 = CISigStr_Snprintf(buf1,max_size,fmt,precesion,arg);

#ifdef WIN32
        ret2 = _snprintf(buf2,max_size,fmt,precesion,arg);
        if(ret1 > max_size)
        {
            ASSERT_EQ(-1,ret2);
            /*修正_snprintf被截断时最后一个字符未置为0的bug*/
            buf2[max_size - 1] = 0;
        }
        /*当max_size正好为格式化后的长度时，CISigStr_Snprintf后面会使用0进行填充*/
        else if (ret1 == max_size)
        {
            buf2[max_size - 1] = 0;
        }
        else
        {
            EXPECT_EQ(ret1,ret2);
        }
#else
        ret2 = snprintf(buf2,max_size,fmt,precesion,arg);
        EXPECT_EQ(ret1,ret2);
#endif /* WIN32*/

        EXPECT_STREQ(buf1,buf2);
    }
    /*
     功能描述    : 
     返回值      : 成功为0，失败为-1
     参数        : 无
     日期        : 2015年3月24日 14:23:02
    */
    void SnprintfStringVariableVriable(int32_t max_size,
        const char* fmt,
        int32_t precesion,
        int32_t precesion2,
        const char* arg)
    {
        memset(buf1,0,MAX_BUF);
        memset(buf2,0,MAX_BUF);

        ret1 = CISigStr_Snprintf(buf1,max_size,fmt,precesion,precesion2,arg);

#ifdef WIN32
        ret2 = _snprintf(buf2,max_size,fmt,precesion,precesion2,arg);
        if(ret1 > max_size)
        {
            ASSERT_EQ(-1,ret2);
            /*修正_snprintf被截断时最后一个字符未置为0的bug*/
            buf2[max_size - 1] = 0;
        }
        /*当max_size正好为格式化后的长度时，CISigStr_Snprintf后面会使用0进行填充*/
        else if (ret1 == max_size)
        {
            buf2[max_size - 1] = 0;
        }
        else
        {
            EXPECT_EQ(ret1,ret2);
        }
#else
        ret2 = snprintf(buf2,max_size,fmt,precesion,precesion2,arg);
        EXPECT_EQ(ret1,ret2);
#endif /* WIN32*/

        EXPECT_STREQ(buf1,buf2);
    }

    void SnprintfIntergers(int32_t max_size, const char* fmt, int32_t arg)
    {
        memset(buf1,0,MAX_BUF);
        memset(buf2,0,MAX_BUF);

        ret1 = CISigStr_Snprintf(buf1,max_size,fmt,arg);

#ifdef WIN32
        ret2 = _snprintf(buf2,max_size,fmt,arg);
        if(ret1 > max_size)
        {
            ASSERT_EQ(-1,ret2);
            /*修正_snprintf被截断时最后一个字符未置为0的bug*/
            buf2[max_size - 1] = 0;
        }
        /*当max_size正好为格式化后的长度时，CISigStr_Snprintf后面会使用0进行填充*/
        else if (ret1 == max_size)
        {
            buf2[max_size - 1] = 0;
        }
        else
        {
            EXPECT_EQ(ret1,ret2);
        }
#else
        ret2 = snprintf(buf2,max_size,fmt,arg);
        EXPECT_EQ(ret1,ret2);
#endif /* WIN32*/

        EXPECT_STREQ(buf1,buf2);
    }

    void SnprintfDouble(int32_t max_size, const char* fmt, double arg)
    {
        memset(buf1,0,MAX_BUF);
        memset(buf2,0,MAX_BUF);

        ret1 = CISigStr_Snprintf(buf1,max_size,fmt,arg);

#ifdef WIN32
        ret2 = _snprintf(buf2,max_size,fmt,arg);
        if(ret1 > max_size)
        {
            ASSERT_EQ(-1,ret2);
            /*修正_snprintf被截断时最后一个字符未置为0的bug*/
            buf2[max_size - 1] = 0;
        }
        /*当max_size正好为格式化后的长度时，CISigStr_Snprintf后面会使用0进行填充*/
        else if (ret1 == max_size)
        {
            buf2[max_size - 1] = 0;
        }
        else
        {
            EXPECT_EQ(ret1,ret2);
        }
#else
        ret2 = snprintf(buf2,max_size,fmt,arg);
        EXPECT_EQ(ret1,ret2);
#endif /* WIN32*/

        EXPECT_STREQ(buf1,buf2);
    }

    char buf1[MAX_BUF];
    char buf2[MAX_BUF];
    int32_t ret1;
    int32_t ret2;
};

TEST_F(TestSigStr, Strnlen){
    std::vector<std::string> str_arr;
    str_arr.push_back("");
    str_arr.push_back("1");
    str_arr.push_back("11");
    str_arr.push_back("1111111111111111111111110");

    for(std::vector<std::string>::iterator iter = str_arr.begin();
            iter != str_arr.end();++iter)
    {
        EXPECT_EQ(strlen(iter->c_str()),CISigStr_Strnlen(iter->c_str(),iter->size()));
    }
}
TEST_F(TestSigStr, StrnlenOverFlow){
    EXPECT_EQ(0,CISigStr_Strnlen("0123456789012345",0));
    EXPECT_EQ(10,CISigStr_Strnlen("0123456789012345",10));
    EXPECT_EQ(16,CISigStr_Strnlen("0123456789012345",16));
    EXPECT_EQ(16,CISigStr_Strnlen("0123456789012345",20));
}

TEST_F(TestSigStr,Snprintf){
    SnprintfString(0,"","");
    SnprintfString(1,"","");
    SnprintfString(1," ","");
    SnprintfString(2," ","");
    SnprintfString(4,"abcd","");

    SnprintfString(3,"%s","ss");

    SnprintfString(20,"%%","ss");
    SnprintfString(20,"%%abcd","ss");
    SnprintfString(20,"abcd%%","ss");
    SnprintfString(20,"ab%%cd","ss");
    SnprintfString(20,"1234567890123456789012345678901234567890","ss");
}
/*
 功能描述    : 测试只打印一个字符
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年3月24日 11:19:35
*/
TEST_F(TestSigStr,SnprintfChar){
    SnprintfStringChar(20,"%c",'A');
    SnprintfStringChar(20,"1234%c56789",'A');
}
/*
 功能描述    : 测试没有变参的情况
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年3月24日 11:23:35
*/
TEST_F(TestSigStr,SnprintfNoSub){
    SnprintfStringNoSub(20,"");
    SnprintfStringNoSub(20," ");
    SnprintfStringNoSub(20,"abcd ");
    SnprintfStringNoSub(20," abcd ");
    SnprintfStringNoSub(20," 1234567890123456789012345678901234567890");
    SnprintfStringNoSub(20,"%%abcd");

    /*下面这两项因为没有参数而无法测试过去*/
    //SnprintfStringNoSub(20,"abcd%s");
    //SnprintfStringNoSub(20,"abcd%%s");
}
/*
 功能描述    : 测试格式化字符串
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年3月24日 12:58:01
*/
TEST_F(TestSigStr,SnprintfString){
    SnprintfString(20,"%s","");
    SnprintfString(20,"%s","ABCD");
    SnprintfString(20,"%s","a ");
    SnprintfString(20,"%s","ABCD");
}
/*
 功能描述    : 
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年3月24日 13:00:23
*/
TEST_F(TestSigStr,SnprintfStringPrecesion){
    SnprintfString(20,"%.0s","");
    SnprintfString(20,"%.0s","ABCD");
    SnprintfString(20,"%.5s"," ");
    SnprintfString(20,"%.5s"," ABD");
    SnprintfString(20,"%.5s","ABCDiiiiiii");
    SnprintfString(20,"%.15s","ABCD");
    SnprintfString(20,"%.15s","ABCDafafdaffff");
}
/*
 功能描述    : string的宽度可以指定
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年3月24日 13:15:05
*/
TEST_F(TestSigStr,SnprintfVariableStringPrecesion){
    SnprintfStringVariable(20,"%.*s",0,"");
    SnprintfStringVariable(20,"%.*s",0,"ABCD");
    SnprintfStringVariable(20,"%.*s",5," ");
    SnprintfStringVariable(20,"%.*s",5," ABD");
    SnprintfStringVariable(20,"%.*s",5,"ABCDiiiiiii");
    SnprintfStringVariable(20,"%.*s",15,"ABCD");
    SnprintfStringVariable(20,"%.*s",15,"ABCDafafdaffff");
}
TEST_F(TestSigStr,SnprintfStringWidth){
    SnprintfString(100,"%5s","");
#ifdef WIN32
    /*linux 下填充不符合规范，可能不会填充0*/
    SnprintfString(100,"%05s","");
#endif /*WIN32*/
    SnprintfString(100,"%5s","ABC");
#ifdef WIN32
    SnprintfString(100,"%05s","ABC");
#endif /*WIN32*/
    SnprintfString(100,"%5s","ABaaaaC");
#ifdef WIN32
    SnprintfString(100,"%05s","ABaaaaC");
#endif /*WIN32*/
    SnprintfString(100,"%-5s","ABc");
    SnprintfString(100,"%-5s","ABcedfg");
    SnprintfString(100,"%20s","ABcedfg");
    SnprintfString(100,"%-20s","ABcedfg");
#ifdef WIN32
    SnprintfString(100,"%020s","ABcedfg");
    SnprintfString(100,"%-020s","ABcedfg");
#endif /*WIN32*/
}
TEST_F(TestSigStr,SnprintfStringVriableWidthPrecesion){
    SnprintfStringVariable(100,"%*s",0,"");
    SnprintfStringVariable(100,"%*s",0,"ABC");
    SnprintfStringVariable(100,"%*s",5,"");
    SnprintfStringVariable(100,"%*s",5,"ABC");
    SnprintfStringVariable(100,"%*s",5,"ABCEFGH");
    SnprintfStringVariable(100,"%*s",-5,"ABC");
    SnprintfStringVariable(100,"%-*s",5,"ABC");
#ifdef WIN32
    SnprintfStringVariable(100,"%0*s",5,"ABC");
    SnprintfStringVariable(100,"%-0*s",5,"ABC");
#endif /*WIN32*/
}
TEST_F(TestSigStr,SnprintfStringWidthPrecesion){
    SnprintfString(100,"%1.5s","");
#ifdef WIN32
    SnprintfString(100,"%05.5s","");
#endif /*WIN32*/
    SnprintfString(100,"%5.5s","ABC");
    SnprintfString(100,"%5.5s","ABCDEFGH");
#ifdef WIN32
    SnprintfString(100,"%05.5s","ABC");
    SnprintfString(100,"%05.5s","ABCEFGH");
#endif /*WIN32*/
    SnprintfString(100,"%-5.5s","ABC");
    SnprintfString(100,"%-5.5s","ABCEFGH");
    SnprintfString(100,"%20.5s","ABC");

}
TEST_F(TestSigStr,SnprintfStringVriableWidthVirablePrecesion){
    SnprintfStringVariableVriable(100,"%*.*s",0,0,"");
    SnprintfStringVariableVriable(100,"%*.*s",0,0,"ABC");
    SnprintfStringVariableVriable(100,"%*.*s",5,5,"");
    SnprintfStringVariableVriable(100,"%*.*s",5,5,"ABC");
    SnprintfStringVariableVriable(100,"%*.*s",5,5,"ABCEFGH");
    /*左对齐bug*/
    SnprintfStringVariableVriable(100,"%*.*s",-5,5,"ABC");
    SnprintfStringVariableVriable(100,"%*.*s",-5,5,"ABCDEFG");
    SnprintfStringVariableVriable(100,"%*.*s",5,-5,"ABC");
    SnprintfStringVariableVriable(100,"%*.*s",5,-5,"ABCDEFG");
    SnprintfStringVariableVriable(100,"%-*.*s",5,5,"ABC");
#ifdef WIN32
    SnprintfStringVariableVriable(100,"%0*.*s",5,5,"ABC");
    SnprintfStringVariableVriable(100,"%-0*.*s",5,5,"ABC");
#endif /*WIN32*/
}
TEST_F(TestSigStr,SnprintfInt){
    const char* int_formats[] = { 
        "%d",
        "%i", 
        "%u",
        "%o",
        "%x",
        "%X",
        "%3d",
        "%.3d",
        "%4.3d",
        "%3.4d", 
        "%#x", 
        "%#X",
        "%hd",
        "%ld",
        "%06d",
        "% 6d",
        "%+6d",
        "%+d",
        "% d",
        "%-5d",
        "%-2d",
        "%06u",
        "% 6u",
        "%+6u",
        "%+u",
        "% u",
        "%-5u",
        "%-2u",
        "%06x",
        "% 6x",
        "%+6x",
        "%+x",
        "% x",
        "%-5x",
        "%-2x",
        "%06o",
        "% 6o",
        "%+6o",
        "%+o",
        "% o",
        "%-5o",
        "%-2o",
        NULL};
    int int_integers[] = {
            0, 
            1,
            -1,
            36,
            -36,
            1000,
            1024,
            -1000,
            -1024,
            32767,
            -32767
    };
    const char **str = int_formats;

    while (*str != NULL)
    {
        for (unsigned int i = 0; i < sizeof(int_integers) / sizeof(int); i++)
        {
            SnprintfIntergers(100,*str,int_integers[i]);
        }
        str++;
    }
}
TEST_F(TestSigStr,SnprintfFloat){
    const char *f_formats[] = { 
        "%f",
        " %f",
        "%f ",
        "%10f",
        "% 10f",
        "%010f",
        "%+10f",
        "%-10f",
        "%.f", 
        "%.0f",
        "%.1f",
        "%.2f",
        "%.5f",
        "%.20f",
        "%.40f",
        "%5.5f",
        "%5.2f",
        "%#f",
        "%#.5f",
        0
    };
#define INFINITY 1.0/(1.0e-23)

    static double f_doubles[] = {
        0.0,
        -0.0,
        1.0,
        -1.0,
        0.0001,
        -0.0001,
        0.000000000000000005, 
        -0.000000000000000005, 
        50000000000000000000.0,
        -50000000000000000000.0,
        3.1415926,
        46278643.7489374839274832,
        789257894257846574.4732847932743,
        131467328384.37482374832,
        INFINITY,
        INFINITY * 0.0 /* nan */
    }; 
    const char **str = f_formats;
    while (*str != 0)
    {
        /* printf("\n%s: ", *str); */
        for (unsigned int i = 0; i < sizeof(f_doubles) / sizeof(double) ; i++)
        {
#ifdef WIN32
            /*BUG TODO在格式化浮点数的时候存在bug*/
            /*SnprintfDouble(512,*str, f_doubles[i]);*/
#endif /*WIN32*/
        }
        str++;
    }

}
