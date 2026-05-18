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

include_directories(${Protobuf_INCLUDE_DIRS})

file(GLOB RPC_LIB_SOURCE
    ${CMAKE_CURRENT_SOURCE_DIR}/detail/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/detail/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

file(GLOB THREAD_LIB_SOURCE
     ${CMAKE_CURRENT_SOURCE_DIR}/threadpool/*.hpp
     ${CMAKE_CURRENT_SOURCE_DIR}/threadpool/*.h
)

file(GLOB PROTOBUF_SOURCE 
    "${CMAKE_CURRENT_SOURCE_DIR}/proto/build/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/proto/build/*.h")
    
add_executable (client ${RPC_LIB_SOURCE} ${PROTOBUF_SOURCE}
    "client.hpp" "rpc_client.cpp" "method/operation.hpp")
add_executable (server ${RPC_LIB_SOURCE} ${PROTOBUF_SOURCE} 
    "server.hpp" "rpc_server.cpp" "method/operation.hpp" "method/operation.cpp")

target_include_directories(client PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/proto/build")
target_include_directories(server PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/proto/build")

target_link_libraries(client PRIVATE
    protobuf::libprotobuf
    utf8_range::utf8_range
    absl::check 
    absl::log_flags 
    absl::statusor
)
target_link_libraries(server PRIVATE
    protobuf::libprotobuf
    utf8_range::utf8_range
    absl::check 
    absl::log_flags 
    absl::statusor
)

# 为ASIO添加必要的定义
add_compile_definitions(
    BOOST_ASIO_NO_DEPRECATED
    _WIN32_WINNT=0x0A00
)

target_link_libraries(client PRIVATE
     $<$<CONFIG:Debug>:libboost_context-vc1444-mt-gd-x64-1_90.lib>
    $<$<CONFIG:Release>:libboost_context-vc1444-mt-x64-1_90.lib>
)

target_link_libraries(server PRIVATE
     $<$<CONFIG:Debug>:libboost_context-vc1444-mt-gd-x64-1_90.lib>
    $<$<CONFIG:Release>:libboost_context-vc1444-mt-x64-1_90.lib>
)
