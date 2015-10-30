#include <gtest/gtest.h>

#ifdef __cplusplus
extern "C"{
#endif /*__cplusplus*/

#include "platform/series_manage.h"
#include "platform/cpu_manage.h"
#include "platform/performance.c"

#include "util/algorithms.c"
#include "util/config.c"
#include "util/log.c"
#include "util/sig_str.c"
#include "util/utility.c"

#include "inter_api_for_test.c"

#ifdef __cplusplus
}
#endif /*__cplusplus*/

class TestPerformance : public ::testing::Test
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

TEST_F(TestPerformance,Init) {
#ifndef WIN32
    return;
#undef APP_CONFIG_PATH
#define APP_CONFIG_PATH APP_PATH "/test/platform/data/performance/Normal.xml"
    ASSERT_EQ(0,CIConfig_Init());
    EXPECT_EQ(0,CIPerformance_Init());

#undef APP_CONFIG_PATH
#define APP_CONFIG_PATH APP_PATH "/test/platform/data/performance/ip_empty.xml"
    ASSERT_EQ(0,CIConfig_Init());
    EXPECT_NE(0,CIPerformance_Init());

#undef APP_CONFIG_PATH
#define APP_CONFIG_PATH APP_PATH "/test/platform/data/performance/ip_wrong1.xml"
    ASSERT_EQ(0,CIConfig_Init());
    EXPECT_NE(0,CIPerformance_Init());

#undef APP_CONFIG_PATH
#define APP_CONFIG_PATH APP_PATH "/test/platform/data/performance/ip_wrong2.xml"
    ASSERT_EQ(0,CIConfig_Init());
    EXPECT_NE(0,CIPerformance_Init());

#undef APP_CONFIG_PATH
#define APP_CONFIG_PATH APP_PATH "/test/platform/data/performance/ip_wrong3.xml"
    ASSERT_EQ(0,CIConfig_Init());
    EXPECT_NE(0,CIPerformance_Init());

#undef APP_CONFIG_PATH
#define APP_CONFIG_PATH APP_PATH "/test/platform/data/performance/ip_wrong4.xml"
    ASSERT_EQ(0,CIConfig_Init());
    EXPECT_NE(0,CIPerformance_Init());

#undef APP_CONFIG_PATH
#define APP_CONFIG_PATH APP_PATH "/test/platform/data/performance/port_empty.xml"
    ASSERT_EQ(0,CIConfig_Init());
    EXPECT_NE(0,CIPerformance_Init());

#undef APP_CONFIG_PATH
#define APP_CONFIG_PATH APP_PATH "/test/platform/data/performance/port_wrong1.xml"
    ASSERT_EQ(0,CIConfig_Init());
    EXPECT_NE(0,CIPerformance_Init());

#undef APP_CONFIG_PATH
#define APP_CONFIG_PATH APP_PATH "/test/platform/data/performance/port_wrong2.xml"
    ASSERT_EQ(0,CIConfig_Init());
    EXPECT_NE(0,CIPerformance_Init());
#endif /* WIN32*/

}
TEST_F(TestPerformance,Send) {
#ifndef WIN32

    EXPECT_EQ(0,CIPerformance_Send((PERFORMANCE_DATA_TYPE)0));
    EXPECT_EQ(0,CIPerformance_Send((PERFORMANCE_DATA_TYPE)1));
    EXPECT_EQ(0,CIPerformance_Send((PERFORMANCE_DATA_TYPE)2));
    EXPECT_EQ(0,CIPerformance_Send((PERFORMANCE_DATA_TYPE)100));
    EXPECT_EQ(0,CIPerformance_Send((PERFORMANCE_DATA_TYPE)10000));
#endif /* WIN32*/

}
TEST_F(TestPerformance,SigSend) {
#ifndef WIN32

    EXPECT_EQ(0,CIPerformance_SigSend((PERFORMANCE_DATA_TYPE)0));
    EXPECT_EQ(0,CIPerformance_SigSend((PERFORMANCE_DATA_TYPE)1));
    EXPECT_EQ(0,CIPerformance_SigSend((PERFORMANCE_DATA_TYPE)2));
    EXPECT_EQ(0,CIPerformance_SigSend((PERFORMANCE_DATA_TYPE)100));
    EXPECT_EQ(0,CIPerformance_SigSend((PERFORMANCE_DATA_TYPE)10000));
#endif /* WIN32*/

}
