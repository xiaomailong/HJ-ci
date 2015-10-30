#include <gtest/gtest.h>

#include "util/algorithms.c"
#include "util/ci_header.h"
#include "util/utility.c"

/*
 * ���Բ��������Լ���Ƿ���ȷ
 */
TEST(TestAlgorithms, Gcd){
    /*trick case: a = b*/
    EXPECT_EQ(0,CIAlgorithm_Gcd(0,0));
    EXPECT_EQ(13,CIAlgorithm_Gcd(13,13));

    /*first argument is a prime*/
    EXPECT_EQ(1,CIAlgorithm_Gcd(37,600));

    /*one is multiplum of other*/
    EXPECT_EQ(20,CIAlgorithm_Gcd(20,100));

    /*straight case*/
    EXPECT_EQ(1,CIAlgorithm_Gcd(1,0));
    EXPECT_EQ(18913,CIAlgorithm_Gcd(624129, 2061517));
    
    /*max value test NOTE �������Լ���ڳ����в�û��ʹ�ã����Ըò���δͨ���Ȳ���*/
    //EXPECT_EQ(1,CIAlgorithm_Gcd((uint32_t)-1, 2061517));
}
/*
 * ���Բ�������С������
 */
TEST(TestAlgorithms, Lcm){
    /*trick case: a = b*/
    EXPECT_EQ(0,CIAlgorithm_Lcm(0,0));
    EXPECT_EQ(13,CIAlgorithm_Lcm(13,13));

    /*both prime case*/
    EXPECT_EQ(21,CIAlgorithm_Lcm(3,7));

    /*some straight case*/
    EXPECT_EQ(1,CIAlgorithm_Lcm(1,1));
    EXPECT_EQ(2,CIAlgorithm_Lcm(1,2));
    EXPECT_EQ(3,CIAlgorithm_Lcm(1,3));
    EXPECT_EQ(6,CIAlgorithm_Lcm(2,3));

    /*value too max ���ض�*/
    EXPECT_EQ(0xffffu,CIAlgorithm_Lcm(0xffffff,0xffffff));
    EXPECT_EQ(4294770690U,CIAlgorithm_Lcm(0xffff,0xfffe));
}

/*
 * ����crc16�㷨
 */
TEST(TestAlgorithms,Crc16)
{
    uint8_t data[8] = {0x13,0x64,0x4b,0x2e,0xaa,0x55,0xaa,0x55};
    EXPECT_EQ(0x9f24,CIAlgorithm_Crc16(&data,8));
}
/*
 * ����crc32�㷨
 */
TEST(TestAlgorithms,Crc32)
{
    /*TODO crc32û��ʹ�ã����Բ�������*/
}
/*
 * ����md5�㷨
 */
TEST(TestAlgorithms,Md5)
{
#ifndef WIN32
    unsigned char buf[MD5_DIGEST_LENGTH] = {0};
    unsigned char md5_str[2 * MD5_DIGEST_LENGTH + 1] = {0};
    /*�ú���Ŀǰֻ��linux��ʹ��*/
    EXPECT_EQ(0,CIAlgorithm_Md5(APP_PATH "/test/util/data/algorithms/1",buf));
    ASSERT_STRCASEEQ("6f5902ac237024bdd0c176cb93063dc4",(const char*)CI_Md5ToStr(buf,md5_str,2 * MD5_DIGEST_LENGTH + 1));

    EXPECT_EQ(0,CIAlgorithm_Md5(APP_PATH "/test/util/data/algorithms/2",buf));
    ASSERT_STRCASEEQ("ab7ba6f56508304eecc70e76025618a5",(const char*)CI_Md5ToStr(buf,md5_str,2 * MD5_DIGEST_LENGTH + 1));

    EXPECT_EQ(0,CIAlgorithm_Md5(APP_PATH "/test/util/data/algorithms/3",buf));
    ASSERT_STRCASEEQ("e7799b148cf42e23ac1c548696b48fd8",(const char*)CI_Md5ToStr(buf,md5_str,2 * MD5_DIGEST_LENGTH + 1));
#endif /* WIN32*/
}
