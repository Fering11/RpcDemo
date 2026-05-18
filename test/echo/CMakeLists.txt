
add_executable(test_echo_client "client.cpp" ${RPC_LIB_SOURCE})
add_executable(test_echo_server "server.cpp" ${RPC_LIB_SOURCE})

add_compile_definitions(
    BOOST_ASIO_NO_DEPRECATED
    _WIN32_WINNT=0x0A00
)

target_link_libraries(test_echo_server PRIVATE
     $<$<CONFIG:Debug>:libboost_context-vc1444-mt-gd-x64-1_90.lib>
    $<$<CONFIG:Release>:libboost_context-vc1444-mt-x64-1_90.lib>
)

target_link_libraries(test_echo_client PRIVATE
     $<$<CONFIG:Debug>:libboost_context-vc1444-mt-gd-x64-1_90.lib>
    $<$<CONFIG:Release>:libboost_context-vc1444-mt-x64-1_90.lib>
)