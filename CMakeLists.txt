cmake_minimum_required(VERSION 3.12)
project(DIN_Protocol C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(include)

set(SOURCES
    src/din_crypto.c
    src/din_model.c
    src/din_pds.c
    src/din_ipfs.c
    src/din_blockchain.c
    src/din_secagg.c
    src/din_auditing.c
    src/din_tokenomics.c
    src/din_coordinator.c
    src/din_wallet.c
)

add_library(din_core STATIC ${SOURCES})

if(UNIX OR MINGW)
    target_link_libraries(din_core m)
endif()

# CLI simulation executable
add_executable(din_simulation examples/main_simulation.c)
target_link_libraries(din_simulation din_core)

if(UNIX OR MINGW)
    target_link_libraries(din_simulation m)
endif()

# WinApi GUI executable
if(WIN32)
    add_executable(din_gui WIN32 examples/main_gui.c src/din_gui.c src/din_wallet_gui.c)
    target_link_libraries(din_gui din_core gdi32 comctl32 user32)
    if(UNIX OR MINGW)
        target_link_libraries(din_gui m)
    endif()
endif()
