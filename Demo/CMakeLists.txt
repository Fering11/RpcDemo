# CMakeList.txt: Demo 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#

# 将源代码添加到此项目的可执行文件。
add_executable (Demo "Demo.cpp" "Demo.h" )
#add_executable (Client "client.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Demo PROPERTY CXX_STANDARD 20)
endif()

# 为ASIO添加必要的定义
target_compile_definitions(Demo PRIVATE 
    BOOST_ASIO_NO_DEPRECATED
    _WIN32_WINNT=0x0A00
)

# 链接Boost静态库
# ASIO本身是header-only，但如果使用协程或某些高级特性，可能需要以下库：
# Release版本使用 -mt-s (多线程静态链接)
# Debug版本使用 -mt-sgd (多线程静态链接+调试符号)
target_link_libraries(Demo PRIVATE 
    $<$<CONFIG:Debug>:libboost_context-vc1444-mt-gd-x64-1_90.lib>
    $<$<CONFIG:Release>:libboost_context-vc1444-mt-x64-1_90.lib>
)
