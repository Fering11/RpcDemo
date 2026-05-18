include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../rpc/")

file(GLOB RPC_LIB_SOURCE
    ${CMAKE_CURRENT_SOURCE_DIR}/../rpc/detail/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../rpc/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../rpc/detail/*.h
)

add_subdirectory(echo)
