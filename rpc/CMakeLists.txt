# CMakeList.txt: Demo 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_policy(SET CMP0091 NEW)


# 添加protobuf的配置
set(CMAKE_PREFIX_PATH "D:/Library/protobuf/install")

# 查找Protobuf及Protobuf所依赖的包
find_package(Protobuf REQUIRED)
# 以下两个都是Protobuf所依赖的库
find_package(Absl REQUIRED)
find_package(utf8_range REQUIRED)

file(GLOB RPC_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/*.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

add_executable (rpc ${RPC_SOURCE})

target_include_directories(rpc PRIVATE  "${CMAKE_CURRENT_SOURCE_DIR}/proto/build")

target_link_libraries(rpc PRIVATE 
    protobuf::libprotobuf
    utf8_range::utf8_range
    absl::check 
    absl::log_flags 
    absl::statusor
)
# 为ASIO添加必要的定义
target_compile_definitions(rpc PRIVATE 
    BOOST_ASIO_NO_DEPRECATED
    _WIN32_WINNT=0x0A00
)

target_link_libraries(rpc PRIVATE 
     $<$<CONFIG:Debug>:libboost_context-vc1444-mt-gd-x64-1_90.lib>
    $<$<CONFIG:Release>:libboost_context-vc1444-mt-x64-1_90.lib>
)
