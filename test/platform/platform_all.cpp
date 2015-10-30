#include <gtest/gtest.h>

#include "platform/app.h"

#ifdef __cplusplus
extern "C"{
#endif /*__cplusplus*/

//#include "inter_api_for_test.c"

#ifdef __cplusplus
}
#endif

class TestPlatform : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
    }

    virtual void init()
    {
        int ret = 0;
        CIAppOps* app_ops = NULL;

    #ifdef WIN32
        extern CIAppOps win_app_ops;
        app_ops = &win_app_ops;
    #else
        extern CIAppOps linux_app_ops;
        app_ops = &linux_app_ops;
    #endif /*WIN32*/

        ret = app_ops->init();
        ASSERT_EQ(0,ret);
    }
};
