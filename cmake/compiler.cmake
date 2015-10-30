###############################################################
# Copyright (C), 2015,  Co.Hengjun, Ltd.
# �ýű����Կ��ƺ͹���CI���̡�
# �ýű����Կ��Ʊ���������Ϊ��
#
# �޶���־��
#   20150310 ������ ����

# �������������Ϊ
macro(ci_compiler_and_linker)
    ADD_DEFINITIONS(-DAPP_PATH="${ci_SOURCE_DIR}")
    #ADD_DEFINITIONS(-fno-common)                       #�Զ��ض����ȫ�ַ��Ų�������
    ADD_DEFINITIONS(-D_DEBUG)                           #��������

    if (MSVC)
        SET(warnings "/W4 /EHsc")
        #WIN32����Դ���뵱�������ж��Ƿ���windows����
        ADD_DEFINITIONS(-DWIN32)
        LINK_DIRECTORIES("${ci_SOURCE_DIR}/library")
        INCLUDE_DIRECTORIES("${ci_SOURCE_DIR}/library/include")
    else (MSVC)
        SET(warnings "-Wall -Wextra")
        #LINUX_ENVRIONMENT����Դ�����������ж��Ƿ���linux����
        ADD_DEFINITIONS(-DLINUX_ENVRIONMENT)
    endif(MSVC)
    
    if (NOT CONFIGURED_ONCE)
        SET(CMAKE_CXX_FLAGS "${warnings}"
            CACHE STRING "Flags used by the compiler during all build types." FORCE)
        SET(CMAKE_C_FLAGS   "${warnings}"
            CACHE STRING "Flags used by the compiler during all build types." FORCE)
    endif()

endmacro()
