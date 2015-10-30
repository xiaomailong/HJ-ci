#include <gtest/gtest.h>

#include "util/log.c"
#ifndef WIN32
#   include "util/sig_str.c"
#endif /*WIN32*/

class TestLog : public ::testing::Test
{
protected:
    virtual void SetUp()
    {

    }
    virtual void TearDown()
    {
    }

    char* GenerateBuf(int size)
    {
        if (size <= 0)
        {
            return NULL;
        }
        buf = (char*)malloc(size);
        for(int i = 0;i < size - 1;i ++)
        {
            buf[i] = 'a';
        }
        buf[size - 1] = 0;
        return buf;
    }

    char* buf;

};

/*
 * 测试打开标准输出，该项测试需要查看标准输出是否有信息
 */
TEST_F(TestLog, OpenStdoutWrite){
    EXPECT_EQ(0,CILog_OpenStdoutWrite());
    EXPECT_EQ(CI_TRUE,stdout_handle.b_use);
}
/*
 * 测试关闭标准输出，该项测试需要查看标准输出是否有信息
 */
TEST_F(TestLog, CloseStdoutWrite){
    EXPECT_EQ(0,CILog_CloseStdoutWrite());
    EXPECT_EQ(CI_FALSE,stdout_handle.b_use);
}
/*
 * 注册新的Handler
 */
TEST_F(TestLog,RegistHandler) {
    CILogHandler handles[MAX_HANDLER] ;

    //因为标准输出占一个，所以只能添加MAX_HANDLER - 1个
    //添加的个数从1开始计数
    for (int i = 0;i < MAX_HANDLER - 1;i++)
    {
        EXPECT_EQ(0,CILog_RegistHandler(&handles[i]))
            << " Add Log Handler " << i
            << " handler pointer " << &handles[i]
            ;
        EXPECT_EQ(i + 2,log_handler_count);
    }
    //添加第MAX_HANDLER时不允许添加
    EXPECT_EQ(-1,CILog_RegistHandler(&handles[MAX_HANDLER - 1]));
    EXPECT_EQ(MAX_HANDLER,log_handler_count);

    //测试注册两次不会返回错误
    EXPECT_EQ(0,CILog_RegistHandler(&handles[0]));
    EXPECT_EQ(MAX_HANDLER,log_handler_count);

    //测试传入空指针
    EXPECT_EQ(-1,CILog_RegistHandler(NULL));
}
/*
 * 打印信息,这个测试暂时不能测试到底打印了多少信息，只能测试输出时函数是否工作
 * 正常
 */
TEST_F(TestLog,Msg) {
    EXPECT_GE(0,CILog_Msg(NULL));
    //GenerateBuf当中调用malloc，malloc可以在0作为参数的时候返回响应的内存空间
    EXPECT_EQ(-1,CILog_Msg(GenerateBuf(0)));
    EXPECT_LE(1,CILog_Msg(GenerateBuf(1)));
    EXPECT_LE(2,CILog_Msg(GenerateBuf(2)));
    EXPECT_LE(3,CILog_Msg(GenerateBuf(3)));

    EXPECT_LE(1023,CILog_Msg(GenerateBuf(1023)));
    EXPECT_LE(1024,CILog_Msg(GenerateBuf(1024)));
    EXPECT_LE(1025,CILog_Msg(GenerateBuf(1025)));
    EXPECT_LE(1026,CILog_Msg(GenerateBuf(1026)));

    EXPECT_LE(1024 * 4,CILog_Msg(GenerateBuf(1024 * 4 - 1)));
    EXPECT_LE(1024 * 4,CILog_Msg(GenerateBuf(1024 * 4)));
    EXPECT_LE(1024 * 4,CILog_Msg(GenerateBuf(1024 * 4 + 1)));

    EXPECT_LE(1024 * 16,CILog_Msg(GenerateBuf(1024 * 16 - 1)));
    EXPECT_LE(1024 * 16,CILog_Msg(GenerateBuf(1024 * 16)));
    EXPECT_LE(1024 * 16,CILog_Msg(GenerateBuf(1024 * 16 + 1)));

    EXPECT_LE(-1,CILog_Msg(GenerateBuf(1000000)));
}
/*
 * 打印信息
 */
TEST_F(TestLog,Errno) {
    EXPECT_GE(0,CILog_Errno(NULL));
    //GenerateBuf当中调用malloc，malloc可以在0作为参数的时候返回响应的内存空间
    EXPECT_EQ(-1,CILog_Errno(GenerateBuf(0)));
    EXPECT_LE(1,CILog_Errno(GenerateBuf(1)));
    EXPECT_LE(2,CILog_Errno(GenerateBuf(2)));
    EXPECT_LE(3,CILog_Errno(GenerateBuf(3)));

    EXPECT_LE(1023,CILog_Errno(GenerateBuf(1023)));
    EXPECT_LE(1024,CILog_Errno(GenerateBuf(1024)));
    EXPECT_LE(1025,CILog_Errno(GenerateBuf(1025)));
    EXPECT_LE(1026,CILog_Errno(GenerateBuf(1026)));

    EXPECT_LE(1024 * 4,CILog_Errno(GenerateBuf(1024 * 4 - 1)));
    EXPECT_LE(1024 * 4,CILog_Errno(GenerateBuf(1024 * 4)));
    EXPECT_LE(1024 * 4,CILog_Errno(GenerateBuf(1024 * 4 + 1)));

    EXPECT_LE(1024 * 16,CILog_Errno(GenerateBuf(1024 * 16 - 1)));
    EXPECT_LE(1024 * 16,CILog_Errno(GenerateBuf(1024 * 16)));
    EXPECT_LE(1024 * 16,CILog_Errno(GenerateBuf(1024 * 16 + 1)));

    EXPECT_LE(-1,CILog_Errno(GenerateBuf(1000000)));
}
TEST_F(TestLog,SigMsg)
{
#ifndef WIN32
    return;
    EXPECT_GE(0,CILog_SigMsg(NULL));

    //GenerateBuf当中调用malloc，malloc可以在0作为参数的时候返回响应的内存空间
    EXPECT_LE(0,CILog_SigMsg(GenerateBuf(0)));
    EXPECT_LE(1,CILog_SigMsg(GenerateBuf(1)));
    EXPECT_LE(2,CILog_SigMsg(GenerateBuf(2)));
    EXPECT_LE(3,CILog_SigMsg(GenerateBuf(3)));

    EXPECT_LE(1023,CILog_SigMsg(GenerateBuf(1023)));
    EXPECT_LE(1024,CILog_SigMsg(GenerateBuf(1024)));
    EXPECT_LE(1025,CILog_SigMsg(GenerateBuf(1025)));
    EXPECT_LE(1026,CILog_SigMsg(GenerateBuf(1026)));

    EXPECT_LE(1024 * 4,CILog_SigMsg(GenerateBuf(1024 * 4 - 1)));
    EXPECT_LE(1024 * 4,CILog_SigMsg(GenerateBuf(1024 * 4)));
    EXPECT_LE(1024 * 4,CILog_SigMsg(GenerateBuf(1024 * 4 + 1)));

    EXPECT_LE(1024 * 16,CILog_SigMsg(GenerateBuf(1024 * 16 - 1)));
    EXPECT_LE(1024 * 16,CILog_SigMsg(GenerateBuf(1024 * 16)));
    EXPECT_LE(1024 * 16,CILog_SigMsg(GenerateBuf(1024 * 16 + 1)));

    EXPECT_LE(-1,CILog_SigMsg(GenerateBuf(1000000)));
#endif /* WIN32*/
}
TEST_F(TestLog,SigErrno)
{
#ifndef WIN32
    return;
    EXPECT_GE(0,CILog_SigErrno(NULL));
    //GenerateBuf当中调用malloc，malloc可以在0作为参数的时候返回响应的内存空间
    EXPECT_EQ(-1,CILog_SigErrno(GenerateBuf(0)));
    EXPECT_LE(1,CILog_SigErrno(GenerateBuf(1)));
    EXPECT_LE(2,CILog_SigErrno(GenerateBuf(2)));
    EXPECT_LE(3,CILog_SigErrno(GenerateBuf(3)));

    EXPECT_LE(1023,CILog_SigErrno(GenerateBuf(1023)));
    EXPECT_LE(1024,CILog_SigErrno(GenerateBuf(1024)));
    EXPECT_LE(1025,CILog_SigErrno(GenerateBuf(1025)));
    EXPECT_LE(1026,CILog_SigErrno(GenerateBuf(1026)));

    EXPECT_LE(1024 * 4,CILog_SigErrno(GenerateBuf(1024 * 4 - 1)));
    EXPECT_LE(1024 * 4,CILog_SigErrno(GenerateBuf(1024 * 4)));
    EXPECT_LE(1024 * 4,CILog_SigErrno(GenerateBuf(1024 * 4 + 1)));

    EXPECT_LE(1024 * 16,CILog_SigErrno(GenerateBuf(1024 * 16 - 1)));
    EXPECT_LE(1024 * 16,CILog_SigErrno(GenerateBuf(1024 * 16)));
    EXPECT_LE(1024 * 16,CILog_SigErrno(GenerateBuf(1024 * 16 + 1)));

    EXPECT_LE(-1,CILog_SigErrno(GenerateBuf(1000000)));
#endif /* WIN32*/
}
