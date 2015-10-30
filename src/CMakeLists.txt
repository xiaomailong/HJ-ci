#设置工程源文件包含起始目录
INCLUDE_DIRECTORIES(${ci_SOURCE_DIR})

AUX_SOURCE_DIRECTORY(platform platform)
AUX_SOURCE_DIRECTORY(interlocking interlocking)
AUX_SOURCE_DIRECTORY(util util)
AUX_SOURCE_DIRECTORY(syn_data syn_data)
AUX_SOURCE_DIRECTORY(inter_data inter_data)

# MESSAGE(STATUS "util" ${util})

# 由于SOURCE_GROUP默认不添加头文件，所有使用间接方案添加其头文件
# 参考http://stackoverflow.com/questions/1167154/listing-header-files-in-visual-studio-c-project-generated-by-cmake
FILE(GLOB_RECURSE header_platform platform/*.h)
FILE(GLOB_RECURSE header_interlocking interlocking/*.h)
FILE(GLOB_RECURSE header_util util/*.h)
FILE(GLOB_RECURSE header_syn_data syn_data/*.h)
FILE(GLOB_RECURSE header_inter_data inter_data/*.h)

SET(HEADERS
    ${header_platform}
    ${header_interlocking}
    ${header_util}
    ${header_syn_data}
    ${header_inter_data}
    ${header_core}
    )

SOURCE_GROUP("platform" ./platform/*)
SOURCE_GROUP("interlocking" ./interlocking/*)
SOURCE_GROUP("util" ./util/*)
SOURCE_GROUP("syn_data" ./syn_data/*)
SOURCE_GROUP("inter_data" ./inter_data/*)
#SOURCE_GROUP("include" ${headers})

SET(SRC_LIST 
    ${platform}
    ${interlocking}
    ${util}
    ${syn_data}
    ${inter_data}
    )

#SOURCE_GROUP("none_main_platform" REGULAR_EXPRESSION "./platform/.*[^m][^a][^i][^n].c" )
#SOURCE_GROUP("none_main_platform" REGULAR_EXPRESSION ./platform/* )
#string(REGEX REPLACE "platform/main.c"
#"" none_main_platform
#${platform}
#)

#MESSAGE(STATUS ${platform})

#MESSAGE(STATUS "SRC_LIST" ${SRC_LIST})

if(MSVC)
    ADD_EXECUTABLE(ci ${SRC_LIST} ${HEADERS})
    TARGET_LINK_LIBRARIES(ci libexpat)
    ADD_CUSTOM_COMMAND(TARGET ci POST_BUILD            # Adds a post-build event to MyTest
        COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
            "${ci_SOURCE_DIR}/library/libexpat.dll"    # <--this is in-file
             ./Debug)                                  # <--this is out-file path
else(MSVC)
    ADD_EXECUTABLE(ci ${SRC_LIST})
    SET_TARGET_PROPERTIES(ci PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR} )
    TARGET_LINK_LIBRARIES(ci rt)
    TARGET_LINK_LIBRARIES(ci ssl)
    TARGET_LINK_LIBRARIES(ci crypto)
    TARGET_LINK_LIBRARIES(ci expat)
    TARGET_LINK_LIBRARIES(ci m)
    TARGET_LINK_LIBRARIES(ci pthread)
endif(MSVC)



