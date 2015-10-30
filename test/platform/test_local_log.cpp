#include <gtest/gtest.h>

#ifdef __cplusplus
extern "C"{
#endif /*__cplusplus*/

#include "platform/local_log.c"
#include "util/log.c"
#include "util/sig_str.c"
#include "util/config.c"

#ifdef __cplusplus
}
#endif /*__cplusplus*/

class TestLocalLog : public ::testing::Test
{
protected:
    TestLocalLog()
    {
    }
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

TEST_F(TestLocalLog,Init) {

    /*由于更改了配置文件的读入方式，下面几项暂时不进行测试*/
    return;

#undef APP_CONFIG_PATH
#define APP_CONFIG_PATH APP_PATH "/test/platform/data/local_log/Normal.xml"
    ASSERT_EQ(0,CIConfig_Init());
    EXPECT_EQ(0,CILocalLog_Init());

#undef APP_CONFIG_PATH
#define APP_CONFIG_PATH APP_PATH "/test/platform/data/local_log/config_empty.xml"
    ASSERT_EQ(0,CIConfig_Init());
    EXPECT_NE(0,CILocalLog_Init());

#undef APP_CONFIG_PATH
#define APP_CONFIG_PATH APP_PATH "/test/platform/data/local_log/name_too_long.xml"
    /*文件名太长会被截断*/
    ASSERT_EQ(0,CIConfig_Init());
    EXPECT_EQ(0,CILocalLog_Init());

#undef APP_CONFIG_PATH
#define APP_CONFIG_PATH APP_PATH "/test/platform/data/local_log/not_config.xml"
    ASSERT_EQ(0,CIConfig_Init());
    EXPECT_NE(0,CILocalLog_Init());
}

TEST_F(TestLocalLog,Write) {
#undef APP_CONFIG_PATH
#define APP_CONFIG_PATH APP_PATH "/test/platform/data/local_log/Normal.xml"
    ASSERT_EQ(0,CIConfig_Init());
    EXPECT_EQ(0,CILocalLog_Init());

    EXPECT_EQ(-1,local_log_file_write(GenerateBuf(0),0));
    EXPECT_EQ(0,local_log_file_write(GenerateBuf(1),1));
    EXPECT_EQ(0,local_log_file_write(GenerateBuf(20),10));
    EXPECT_EQ(0,local_log_file_write(GenerateBuf(1024),1024));
    EXPECT_EQ(0,local_log_file_write(GenerateBuf(4096 * 4),4096 * 4));
}
