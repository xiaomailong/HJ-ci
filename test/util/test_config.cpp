#include <gtest/gtest.h>

#include "util/config.c"
#ifndef WIN32
#   include "util/sig_str.c"
#endif
#include "util/log.c"

class TestConfig : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }
};
/*
 * ���������ļ��ĸ�ʽ��
 * ����ʱ������������ע��junk after document element����
 */
TEST_F(TestConfig, Init){
    /*����������ļ����е�˵��*/
    /*
    EXPECT_EQ(0,CIConfig_Init(APP_PATH "/test/util/data/config/Normal.xml"));
    EXPECT_NE(0,CIConfig_Init(APP_PATH "/test/util/data/config/NotWellFormed.xml"));
    EXPECT_NE(0,CIConfig_Init(APP_PATH "/test/util/data/config/RootElementMulti.xml"));
    EXPECT_NE(0,CIConfig_Init(APP_PATH "/test/util/data/config/RootElementNotConfiguration.xml"));
    EXPECT_NE(0,CIConfig_Init(APP_PATH "/test/util/data/config/DeviceIdDefineWrong.xml"));
    EXPECT_NE(0,CIConfig_Init(APP_PATH "/test/util/data/config/DeviceIdUndefine.xml"));

    EXPECT_NE(0,CIConfig_Init(APP_PATH "/test/util/data/config/ConfigurationNodeExceed.xml"));
    EXPECT_NE(0,CIConfig_Init(APP_PATH "/test/util/data/config/PropertyNodeExcced.xml"));
    */
}

//TEST_F(TestConfig,GetDeviceId){
//    EXPECT_EQ(0,CIConfig_Init(APP_PATH "/test/util/data/config/GetDeviceId_Normal.xml"));
//    EXPECT_EQ(5,CIConfig_GetDeviceId());
//}

//TEST_F(TestConfig,GetValue){
//    CIConfig_Init(APP_PATH "/test/util/data/config/Normal.xml");
//    ASSERT_STREQ(NULL,CIConfig_GetValue("UseConfigurationId"));
//    ASSERT_STREQ("bzz",CIConfig_GetValue("StationName"));
//    /*�������ִ�Сд*/
//    ASSERT_STRNE("bzz",CIConfig_GetValue("stationName"));
//
//    /*δ�������Ϊ��*/
//    ASSERT_STREQ(NULL,CIConfig_GetValue("NONE"));
//
//#ifdef WIN32
//    //��windows��CpuStateΪ��CPU������PeerSeriesSlaveIpδ����
//    ASSERT_STREQ("master",CIConfig_GetValue("CpuState"));
//    ASSERT_STREQ(NULL,CIConfig_GetValue("PeerSeriesSlaveIp"));
//#endif /* WIN32*/
//
//}
//TEST_F(TestConfig,SelectValue){
//    CIConfig_Init(APP_PATH "/test/util/data/config/Normal.xml");
//
//    ConfigSelectCondition con;
//    con.name = "LINUX_AUX";
//    con.value = "I";
//    ASSERT_STREQ("192.168.1.202",CIConfig_SelectValue("PeerSeriesMasterIp",&con));
//
//    con.value = "II";
//    ASSERT_STREQ("192.168.1.200",CIConfig_SelectValue("PeerSeriesMasterIp",&con));
//}
