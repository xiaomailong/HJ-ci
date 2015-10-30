#include <gtest/gtest.h>

#include "util/utility.c"
#include "util/algorithms.c"

TEST(TestUtility, ValidateIpAddress){
    EXPECT_EQ(CI_FALSE,CI_ValidateIpAddress(NULL));
    EXPECT_EQ(CI_FALSE,CI_ValidateIpAddress("192.168"));
    EXPECT_EQ(CI_FALSE,CI_ValidateIpAddress("a.a.a.a"));
    EXPECT_EQ(CI_FALSE,CI_ValidateIpAddress("256.1.1.1"));
    EXPECT_EQ(CI_FALSE,CI_ValidateIpAddress("1.259.1.1"));
    EXPECT_EQ(CI_FALSE,CI_ValidateIpAddress("1.259.1.1"));

    EXPECT_EQ(CI_FALSE,CI_ValidateIpAddress("0.0.005.0"));

    EXPECT_EQ(CI_TRUE,CI_ValidateIpAddress("0.2.1.1"));

    EXPECT_EQ(CI_TRUE,CI_ValidateIpAddress("255.255.255.255"));
    EXPECT_EQ(CI_TRUE,CI_ValidateIpAddress("1.2.3.4"));
    EXPECT_EQ(CI_TRUE,CI_ValidateIpAddress("0.0.0.0"));
}

TEST(TestUtility, StrIpToInt32){
    uint32_t ip = 0;
    EXPECT_EQ(-1,CI_StrIpToInt32(NULL,&ip));
    EXPECT_EQ(-1,CI_StrIpToInt32("",&ip));
    EXPECT_EQ(-1,CI_StrIpToInt32("192.168",&ip));
    EXPECT_EQ(-1,CI_StrIpToInt32(".168",&ip));
    EXPECT_EQ(-1,CI_StrIpToInt32("....",&ip));
    EXPECT_EQ(-1,CI_StrIpToInt32("256.1.1.1",&ip));

    /*not digit*/
    EXPECT_EQ(-1,CI_StrIpToInt32("a.1.1.201",&ip));

    EXPECT_EQ(0,CI_StrIpToInt32("255.255.255.255",&ip));
    EXPECT_EQ(0xffffffff,ip);

    EXPECT_EQ(0,CI_StrIpToInt32("255.255.255.254",&ip));
    EXPECT_EQ(0xfeffffff,ip);

    EXPECT_EQ(0,CI_StrIpToInt32("1.255.255.254",&ip));
    EXPECT_EQ(0xfeffff01,ip);

}