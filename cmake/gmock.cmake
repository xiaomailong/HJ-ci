SET(GMOCK_DIR "./gmock-1.7.0"
    CACHE PATH "The path to the GoogleMock test framework.")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    # force this option to ON so that Google Test will use /MD instead of /MT
    # /MD is now the default for Visual Studio, so it should be our default, too
    OPTION(gtest_force_shared_crt
           "Use shared (DLL) run-time lib even when Google Test is built as static lib."
           ON)
elseif (APPLE)
    ADD_DEFINITIONS(-DGTEST_USE_OWN_TR1_TUPLE=1)
endif()

# 将gmock生成的中间文件生成到${CMAKE_BINARY_DIR}/gmock，防止，在./gmock-1.7.0当中生成
# 多余文件
ADD_SUBDIRECTORY(${GMOCK_DIR} ${CMAKE_BINARY_DIR}/gmock)
# 给gtest添加-w标志
# SET_PROPERTY(TARGET gtest APPEND_STRING PROPERTY COMPILE_FLAGS " -w")

# 将gmock头文件作为系统头文件引入
INCLUDE_DIRECTORIES(SYSTEM ${GMOCK_DIR}/gtest/include
                           ${GMOCK_DIR}/include)

#
# add_gmock_test(<target> <sources>...)
#
#  Adds a Google Mock based test executable, <target>, built from <sources> and
#  adds the test so that CTest will run it. Both the executable and the test
#  will be named <target>.
#
function(add_gmock_test target)
    # ${ARGN}表示传递进来的其它参数，因为该函数只指定了一个参数，所以${ARGN}从第2个参数
    # 开始
    ADD_EXECUTABLE(${target} ${ARGN})
    # 使用unit test
    ADD_DEFINITIONS(-DCI_UNIT_TEST=1)

    target_link_libraries(${target} gmock_main) # gmock_main在gmock项目的CMakeLists.txt当中定义

    # 添加测试方案，以便ctest能够执行
    add_test(NAME ${target} COMMAND ${target})

    # 在${target}被编译后触发该模块的测试
    add_custom_command(TARGET ${target}
                       POST_BUILD
                       COMMAND ${target}
                       WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                       COMMENT "Running ${target}" VERBATIM)
    # 为每个gmock的程序添加libexpat链接
    if(WIN32)
        TARGET_LINK_LIBRARIES(${target} libexpat)
    else(WIN32)
        TARGET_LINK_LIBRARIES(${target} expat)
        TARGET_LINK_LIBRARIES(${target} rt)
        TARGET_LINK_LIBRARIES(${target} ssl)
        TARGET_LINK_LIBRARIES(${target} crypto)
        # 将所有的测试程序生成到${CMAKE_BINARY_DIR}目录
        SET_TARGET_PROPERTIES(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR} )
    endif()
    ADD_CUSTOM_COMMAND(TARGET ${target} POST_BUILD   # Adds a post-build event to MyTest
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
            "${ci_SOURCE_DIR}/library/libexpat.dll"    # <--this is in-file
            ./Debug)                                  # <--this is out-file path

endfunction()
