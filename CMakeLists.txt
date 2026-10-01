cmake_minimum_required(VERSION 3.15)
project(DMC5_DiscordRPC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(REF_SDK_DIR     "${CMAKE_CURRENT_SOURCE_DIR}/reframework")
set(DISCORD_RPC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/discord-rpc/win64-static")

add_library(DMC5_DiscordRPC SHARED
    plugin/rpc.cpp
)

target_include_directories(DMC5_DiscordRPC PRIVATE
    ${REF_SDK_DIR}/include
    ${DISCORD_RPC_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/plugin   # для json.hpp
)

target_link_libraries(DMC5_DiscordRPC PRIVATE
    ${DISCORD_RPC_DIR}/lib/discord-rpc.lib
    ws2_32
    winhttp
    crypt32
)

add_custom_command(TARGET DMC5_DiscordRPC POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/plugin/locrpc.json"
        "$<TARGET_FILE_DIR:DMC5_DiscordRPC>/locrpc.json"
    COMMENT "Copying locrpc.json next to the plugin DLL"
)