###############################################################
# Copyright (C), 2015,  Co.Hengjun, Ltd.
# 该脚本用以控制和管理CI工程。
# 该脚本用以控制编译器的行为。
#
# 修订日志：
#   20150310 张彦升 创建

# 定义编译器的行为
macro(ci_compiler_and_linker)
    ADD_DEFINITIONS(-DAPP_PATH="${ci_SOURCE_DIR}")
    #ADD_DEFINITIONS(-fno-common)                       #对多重定义的全局符号产生警告
    ADD_DEFINITIONS(-D_DEBUG)                           #调试运行

    if (MSVC)
        SET(warnings "/W4 /EHsc")
        #WIN32宏在源代码当中用以判断是否是windows环境
        ADD_DEFINITIONS(-DWIN32)
        LINK_DIRECTORIES("${ci_SOURCE_DIR}/library")
        INCLUDE_DIRECTORIES("${ci_SOURCE_DIR}/library/include")
    else (MSVC)
        SET(warnings "-Wall -Wextra")
        #LINUX_ENVRIONMENT宏在源代码中用以判断是否是linux环境
        ADD_DEFINITIONS(-DLINUX_ENVRIONMENT)
    endif(MSVC)
    
    if (NOT CONFIGURED_ONCE)
        SET(CMAKE_CXX_FLAGS "${warnings}"
            CACHE STRING "Flags used by the compiler during all build types." FORCE)
        SET(CMAKE_C_FLAGS   "${warnings}"
            CACHE STRING "Flags used by the compiler during all build types." FORCE)
    endif()

endmacro()
