####################################################################
# Copyright (C), 2015,  Co.Hengjun, Ltd.
# 该脚本用以控制和管理CI工程。
# 假若你的代码目录为D:/HJ/ci，使用方式如下：
# 1.windows平台
#  a)直接打开cmake-gui.exe，
#  b)在where is the source code项中输入源码目录。如D:/HJ/ci。
#  c)在where to build the binaries项中输入生成项目目录。如D:/HJ/ci/vs2013。
#  d)点击configure。
#  e)点击generate。
#  f)在D:/HJ/ci/vs2013目录下打开工程。
# 2.linux平台，执行下面命令
#  $pwd
#   /home/root
#  $mkdir bin
#  $cd bin
#  $cmake ..
#  $make
#  #生成完毕，尝试运行。
# 
# 修订日志：
#   20131010 张彦升 创建
#   20150310 张彦升 添加单元测试工程
#

#版本规则：
#MAJOR和MINOR在大的范围上控制版本
#REVISION 当头文件或源文件被修订的时候，在此值上进行增加
#SET(ci_VERSION_MAJOR "0")
#SET(ci_VERSION_MINOR "2")
#SET(ci_VERSION_REVISION "1")

#MATH(EXPR ci_INT_VERSION "(${ci_VERSION_MAJOR} * 10000) + (${ci_VERSION_MINOR} * 100) + (${ci_VERSION_REVISION} * 1)" )
#SET(ci_VERSION "${ci_VERSION_MAJOR}.${ci_VERSION_MINOR}.${ci_VERSION_REVISION}")

#MESSAGE(STATUS "current version:" ${ci_VERSION} "(" ${ci_INT_VERSION} ")")

set(CMAKE_LEGACY_CYGWIN_WIN32 0)

#打开这一项将会生成单元测试工程，以便进行测试
OPTION(ci_build_unit_tests "Build All Unit Test of CI." ON)


if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

#####################################################
# 针对项目进行的设置
# 
# 项目名称为ci
# 在该cmake脚本当中引用源码根目录可以使用${ci_SOURCE_DIR}变量。
# 引用生成目录可以使用${ci_BINARY_DIR}目录。
PROJECT(ci C CXX)

#cmake版本应在2.8以上
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.0 FATAL_ERROR)

# 添加cmake模块目录
LIST(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

INCLUDE(compiler)

ci_compiler_and_linker() # Defined in compiler.cmake.

# 将工程src暴露在外面以便能够引用
INCLUDE_DIRECTORIES("${ci_SOURCE_DIR}/src")

SET(CMAKE_BUILD_TYPE Debug) #生成debug版本

#添加ci工程
ADD_SUBDIRECTORY(src)

# 添加测试工程
if(ci_build_unit_tests)
    # 引用gmock模块
    INCLUDE(gmock)
    # 允许使用ctest
    ENABLE_TESTING()
    # 添加test目录下所有工程
    ADD_SUBDIRECTORY(test)
    
endif()


