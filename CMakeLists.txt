# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
#
cmake_minimum_required (VERSION 3.10)
#MT/d 静态
#set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(CMAKE_CXX_STANDARD 20)
project ("Demo")

if(MSVC)
    message(STATUS "Using MSVC")

    # 关闭警告 C4819: 无效的字符。避免中文注释被警告
    add_compile_options(
        "/wd4819"
    )

    # 关闭 /showIncludes
    string(REPLACE "/showIncludes" "" CMAKE_DEPFILE_FLAGS_C "${CMAKE_DEPFILE_FLAGS_C}")
    string(REPLACE "/showIncludes" "" CMAKE_DEPFILE_FLAGS_CXX "${CMAKE_DEPFILE_FLAGS_CXX}")
endif()

# 配置Boost库路径
set(BOOST_ROOT "D:/Library/boost")
set(BOOST_INCLUDEDIR "D:/Library/boost/include/boost-1_90")
set(BOOST_LIBRARYDIR "D:/Library/boost_static/lib")

# 添加Boost头文件路径
include_directories(${BOOST_INCLUDEDIR})

# 添加Boost库文件路径
link_directories(${BOOST_LIBRARYDIR})

message(STATUS "Boost include directory: ${BOOST_INCLUDEDIR}")
message(STATUS "Boost library directory: ${BOOST_LIBRARYDIR}")

# 包含子项目。
add_subdirectory ("Demo")
add_subdirectory ("rpc")
add_subdirectory ("test")