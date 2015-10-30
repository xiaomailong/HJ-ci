##################################################################
# Copyright (C), 2015,  Co.Hengjun, Ltd.
# 该脚本配置所有在util相关的测试项目。
# 
# 修订日志：
#   20150311 张彦升 创建
# add_gmock_test在gmock.cmake当中定义
# add_gmock_test(<target> <sources>...)
add_gmock_test(test_log test_log.cpp)

add_gmock_test(test_config test_config.cpp)

add_gmock_test(test_algorithms test_algorithms.cpp)

add_gmock_test(test_utility test_utility.cpp)

add_gmock_test(test_sig_str test_sig_str.cpp)

