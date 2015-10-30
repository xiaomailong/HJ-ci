##################################################################
# Copyright (C), 2015,  Co.Hengjun, Ltd.
# 该脚本用以控制和管理CI工程。
# 这个脚本当中存储所有与平台有关的测试项目
# 
# 修订日志：
#   20150311 张彦升 创建
# add_gmock_test在gmock.cmake当中定义
# add_gmock_test(<target> <sources>...)

add_gmock_test(test_remote_log test_remote_log.cpp)

add_gmock_test(test_local_log test_local_log.cpp)

add_gmock_test(test_hmi_manage test_hmi_manage.cpp)
add_gmock_test(test_performance test_performance.cpp)
