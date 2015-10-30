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
 * ���Դ򿪱�׼��������������Ҫ�鿴��׼����Ƿ�����Ϣ
 */
TEST_F(TestLog, OpenStdoutWrite){
    EXPECT_EQ(0,CILog_OpenStdoutWrite());
    EXPECT_EQ(CI_TRUE,stdout_handle.b_use);
}
/*
 * ���Թرձ�׼��������������Ҫ�鿴��׼����Ƿ�����Ϣ
 */
TEST_F(TestLog, CloseStdoutWrite){
    EXPECT_EQ(0,CILog_CloseStdoutWrite());
    EXPECT_EQ(CI_FALSE,stdout_handle.b_use);
}
/*
 * ע���µ�Handler
 */
TEST_F(TestLog,RegistHandler) {
    CILogHandler handles[MAX_HANDLER] ;

    //��Ϊ��׼���ռһ��������ֻ�����MAX_HANDLER - 1��
    //��ӵĸ�����1��ʼ����
    for (int i = 0;i < MAX_HANDLER - 1;i++)
    {
        EXPECT_EQ(0,CILog_RegistHandler(&handles[i]))
            << " Add Log Handler " << i
            << " handler pointer " << &handles[i]
            ;
        EXPECT_EQ(i + 2,log_handler_count);
    }
    //��ӵ�MAX_HANDLERʱ���������
    EXPECT_EQ(-1,CILog_RegistHandler(&handles[MAX_HANDLER - 1]));
    EXPECT_EQ(MAX_HANDLER,log_handler_count);

    //����ע�����β��᷵�ش���
    EXPECT_EQ(0,CILog_RegistHandler(&handles[0]));
    EXPECT_EQ(MAX_HANDLER,log_handler_count);

    //���Դ����ָ��
    EXPECT_EQ(-1,CILog_RegistHandler(NULL));
}
/*
 * ��ӡ��Ϣ,���������ʱ���ܲ��Ե��״�ӡ�˶�����Ϣ��ֻ�ܲ������ʱ�����Ƿ���
 * ����
 */
TEST_F(TestLog,Msg) {
    EXPECT_GE(0,CILog_Msg(NULL));
    //GenerateBuf���е���malloc��malloc������0��Ϊ������ʱ�򷵻���Ӧ���ڴ�ռ�
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
 * ��ӡ��Ϣ
 */
TEST_F(TestLog,Errno) {
    EXPECT_GE(0,CILog_Errno(NULL));
    //GenerateBuf���е���malloc��malloc������0��Ϊ������ʱ�򷵻���Ӧ���ڴ�ռ�
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

    //GenerateBuf���е���malloc��malloc������0��Ϊ������ʱ�򷵻���Ӧ���ڴ�ռ�
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
    //GenerateBuf���е���malloc��malloc������0��Ϊ������ʱ�򷵻���Ӧ���ڴ�ռ�
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
