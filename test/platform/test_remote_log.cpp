#include <gtest/gtest.h>

#ifdef __cplusplus
extern "C"{
#endif /*__cplusplus*/

#include "platform/remote_log.c"
#include "util/log.c"
#include "util/sig_str.c"
#include "util/config.c"
#include "util/utility.c"

#include "inter_api_for_test.c"

#ifdef __cplusplus
}
#endif /*__cplusplus*/

class TestRemoteLog : public ::testing::Test
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

TEST_F(TestRemoteLog,Init) {
    return;
    /*检查关于remotelog的配置项全正确的情况下*/
#undef APP_CONFIG_PATH
#define APP_CONFIG_PATH APP_PATH "/test/platform/data/remote_log/Normal.xml"
    ASSERT_EQ(0,CIConfig_Init());
    EXPECT_EQ(0,CIRemoteLog_Init());

#undef APP_CONFIG_PATH
#define APP_CONFIG_PATH APP_PATH "/test/platform/data/remote_log/ip_not_config.xml"
    EXPECT_EQ(0,CIConfig_Init());
    EXPECT_NE(0,CIRemoteLog_Init());

#undef APP_CONFIG_PATH
#define APP_CONFIG_PATH APP_PATH "/test/platform/data/remote_log/ip_wrong1.xml"
    EXPECT_EQ(0,CIConfig_Init());
    EXPECT_NE(0,CIRemoteLog_Init());

#undef APP_CONFIG_PATH
#define APP_CONFIG_PATH APP_PATH "/test/platform/data/remote_log/ip_wrong2.xml"
    EXPECT_EQ(0,CIConfig_Init());
    EXPECT_NE(0,CIRemoteLog_Init());

#undef APP_CONFIG_PATH
#define APP_CONFIG_PATH APP_PATH "/test/platform/data/remote_log/ip_wrong3.xml"
    EXPECT_EQ(0,CIConfig_Init());
    EXPECT_NE(0,CIRemoteLog_Init());

#undef APP_CONFIG_PATH
#define APP_CONFIG_PATH APP_PATH "/test/platform/data/remote_log/ip_wrong4.xml"
    EXPECT_EQ(0,CIConfig_Init());
    EXPECT_NE(0,CIRemoteLog_Init());

#undef APP_CONFIG_PATH
#define APP_CONFIG_PATH APP_PATH "/test/platform/data/remote_log/port_not_config.xml"
    EXPECT_EQ(0,CIConfig_Init());
    EXPECT_NE(0,CIRemoteLog_Init());

#undef APP_CONFIG_PATH
#define APP_CONFIG_PATH APP_PATH "/test/platform/data/remote_log/port_wrong1.xml"
    EXPECT_EQ(0,CIConfig_Init());
    EXPECT_NE(0,CIRemoteLog_Init());

#undef APP_CONFIG_PATH
#define APP_CONFIG_PATH APP_PATH "/test/platform/data/remote_log/port_wrong2.xml"
    EXPECT_EQ(0,CIConfig_Init());
    EXPECT_NE(0,CIRemoteLog_Init());

#undef APP_CONFIG_PATH
#define APP_CONFIG_PATH APP_PATH "/test/platform/data/remote_log/port_wrong3.xml"
    EXPECT_EQ(0,CIConfig_Init());
    EXPECT_NE(0,CIRemoteLog_Init());
}

TEST_F(TestRemoteLog,Write) {
#undef APP_CONFIG_PATH
#define APP_CONFIG_PATH APP_PATH "/test/platform/data/remote_log/Normal.xml"
    ASSERT_EQ(0,CIConfig_Init());
    EXPECT_EQ(0,CIRemoteLog_Init());

    EXPECT_EQ(-1,remote_log_write(GenerateBuf(0),0));
    EXPECT_EQ(0,remote_log_write(GenerateBuf(1),1));
    EXPECT_EQ(0,remote_log_write(GenerateBuf(SOCKET_DEFAULT_BUF_LEN - 1),SOCKET_DEFAULT_BUF_LEN - 1));
    EXPECT_EQ(0,remote_log_write(GenerateBuf(SOCKET_DEFAULT_BUF_LEN),SOCKET_DEFAULT_BUF_LEN));
    EXPECT_EQ(0,remote_log_write(GenerateBuf(SOCKET_DEFAULT_BUF_LEN + 1),SOCKET_DEFAULT_BUF_LEN + 1));

    EXPECT_EQ(0,remote_log_write(GenerateBuf(10000),10000));
}

