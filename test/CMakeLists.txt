##################################################################
# Copyright (C), 2015,  Co.Hengjun, Ltd.
# 该脚本当中配置所有测试项目
# 如何将gmock与cmake融合起来请看：
# https://www.johnlamp.net/cmake-tutorial-1-getting-started.html
# 
# 修订日志：
#   20150310 张彦升 创建
# add_gmock_test在gmock.cmake当中定义
# add_gmock_test(<target> <sources>...)
# add_gmock_test(sample_gmock_test sample_gmock_test.cpp)
ADD_SUBDIRECTORY(platform)
ADD_SUBDIRECTORY(interlocking)
ADD_SUBDIRECTORY(util)
