#include <gtest/gtest.h>

#ifdef __cplusplus
extern "C"{
#endif

#include "util/algorithms.c"
#include "util/config.c"
#include "util/log.c"
#include "util/sig_str.c"
#include "util/utility.c"

#include "platform/series_manage.h"
#include "platform/cpu_manage.h"
#include "inter_api_for_test.c"

#include "platform/hmi_manage.c"

#ifdef __cplusplus
}
#endif

extern int32_t CIMonitor_Write(const void* buf,int32_t data_len)
{
    return 0;
}
typedef uint8_t CI_BOOL;

extern CI_BOOL CIEeu_IsChannelAOk(void)
{
    return 0;
}
extern CI_BOOL CIEeu_IsChannelBOk(void)
{
    return 0;
}

class TestHmiManage : public ::testing::Test
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

TEST_F(TestHmiManage,Init) {

#undef APP_CONFIG_PATH
#define APP_CONFIG_PATH APP_PATH "/test/platform/data/hmi_manage/Normal.xml"
    ASSERT_EQ(0,CIConfig_Init());
    EXPECT_EQ(0,CIHmi_Init());

#undef APP_CONFIG_PATH
#define APP_CONFIG_PATH APP_PATH "/test/platform/data/hmi_manage/ip_empty.xml"
    ASSERT_EQ(0,CIConfig_Init());
    EXPECT_NE(0,CIHmi_Init());

#undef APP_CONFIG_PATH
#define APP_CONFIG_PATH APP_PATH "/test/platform/data/hmi_manage/ip_wrong1.xml"
    ASSERT_EQ(0,CIConfig_Init());
    EXPECT_NE(0,CIHmi_Init());

#undef APP_CONFIG_PATH
#define APP_CONFIG_PATH APP_PATH "/test/platform/data/hmi_manage/ip_wrong2.xml"
    ASSERT_EQ(0,CIConfig_Init());
    EXPECT_NE(0,CIHmi_Init());

#undef APP_CONFIG_PATH
#define APP_CONFIG_PATH APP_PATH "/test/platform/data/hmi_manage/ip_wrong3.xml"
    ASSERT_EQ(0,CIConfig_Init());
    EXPECT_NE(0,CIHmi_Init());

#undef APP_CONFIG_PATH
#define APP_CONFIG_PATH APP_PATH "/test/platform/data/hmi_manage/ip_wrong4.xml"
    ASSERT_EQ(0,CIConfig_Init());
    EXPECT_NE(0,CIHmi_Init());

#undef APP_CONFIG_PATH
#define APP_CONFIG_PATH APP_PATH "/test/platform/data/hmi_manage/no_ip.xml"
    ASSERT_EQ(0,CIConfig_Init());
    EXPECT_NE(0,CIHmi_Init());

#undef APP_CONFIG_PATH
#define APP_CONFIG_PATH APP_PATH "/test/platform/data/hmi_manage/no_port.xml"
    ASSERT_EQ(0,CIConfig_Init());
    EXPECT_NE(0,CIHmi_Init());

#undef APP_CONFIG_PATH
#define APP_CONFIG_PATH APP_PATH "/test/platform/data/hmi_manage/port_empty.xml"
    ASSERT_EQ(0,CIConfig_Init());
    EXPECT_NE(0,CIHmi_Init());

#undef APP_CONFIG_PATH
#define APP_CONFIG_PATH APP_PATH "/test/platform/data/hmi_manage/port_wrong1.xml"
    ASSERT_EQ(0,CIConfig_Init());
    EXPECT_NE(0,CIHmi_Init());

#undef APP_CONFIG_PATH
#define APP_CONFIG_PATH APP_PATH "/test/platform/data/hmi_manage/port_wrong2.xml"
    ASSERT_EQ(0,CIConfig_Init());
    EXPECT_NE(0,CIHmi_Init());

#undef APP_CONFIG_PATH
#define APP_CONFIG_PATH APP_PATH "/test/platform/data/hmi_manage/port_wrong3.xml"
    ASSERT_EQ(0,CIConfig_Init());
    EXPECT_NE(0,CIHmi_Init());
}

TEST_F(TestHmiManage,SendNormalTips) {
    /*这几个测试暂时对返回值没有具体要求，测试只保证程序不会产生异常*/
    CIHmi_SendNormalTips(GenerateBuf(0));
    CIHmi_SendNormalTips(GenerateBuf(200));
    CIHmi_SendNormalTips(GenerateBuf(1024));
    CIHmi_SendNormalTips(GenerateBuf(2048));
    CIHmi_SendNormalTips(GenerateBuf(4096));
    CIHmi_SendNormalTips(GenerateBuf(1000000));
}
TEST_F(TestHmiManage,SendDebugTips) {
    /*这几个测试暂时对返回值没有具体要求，测试只保证程序不会产生异常*/
    CIHmi_SendDebugTips(GenerateBuf(0));
    CIHmi_SendDebugTips(GenerateBuf(200));
    CIHmi_SendDebugTips(GenerateBuf(1024));
    CIHmi_SendDebugTips(GenerateBuf(2048));
    CIHmi_SendDebugTips(GenerateBuf(4096));
    CIHmi_SendDebugTips(GenerateBuf(1000000));
}
TEST_F(TestHmiManage,SendTimerTips) {
    /*这几个测试暂时对返回值没有具体要求，测试只保证程序不会产生异常*/
    CIHmi_SendTimerTips(GenerateBuf(0));
    CIHmi_SendTimerTips(GenerateBuf(200));
    CIHmi_SendTimerTips(GenerateBuf(1024));
    CIHmi_SendTimerTips(GenerateBuf(2048));
    CIHmi_SendTimerTips(GenerateBuf(4096));
    CIHmi_SendTimerTips(GenerateBuf(1000000));
}
