# CMakeList.txt : CMake project for steamcclliient, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Make sure to set correct paths before building...
set(OSW_INCLUDE_PATH "\\open-steamworks\\OpenSteamworks")
set(OSW_LIB_PATH "\\open-steamworks\\OpenSteamAPI\\lib\\Win32\\OpenSteamAPI.lib")

# Add source to this project's executable.
add_executable (steamcclliient 
    "steamcclliient.cpp" "steamcclliient.h"
    "clientcommands.cpp" "clientcommands.h"
    "clientcontext.cpp"  "clientcontext.h"
    "utils.cpp" "utils.h"
)

target_compile_definitions(steamcclliient PRIVATE STEAMWORKS_CLIENT_INTERFACES)
target_include_directories(steamcclliient PUBLIC ${OSW_INCLUDE_PATH})
target_link_libraries(steamcclliient ${OSW_LIB_PATH})

# TODO: Add tests and install targets if needed.
