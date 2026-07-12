cmake_minimum_required(VERSION 3.22.1)
project("eetgw")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =============================================================================
# Configuracoes / Configurations
# =============================================================================
set(FFMPEG OFF CACHE BOOL "Usar FFmpeg (maior, mais formatos) / Use FFmpeg (larger, more formats)")
set(OBOE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/oboe" CACHE STRING "Diretorio do Oboe / Oboe directory")

# =============================================================================
# Flags do Compilador / Compiler Flags
# =============================================================================
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -funroll-loops -fomit-frame-pointer")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3 -fno-omit-frame-pointer")

# =============================================================================
# Oboe (Audio de Baixa Latencia / Low Latency Audio)
# =============================================================================
if (EXISTS "${OBOE_DIR}/CMakeLists.txt")
    add_subdirectory(${OBOE_DIR} ./oboe)
else()
    find_package(oboe QUIET)
    if (NOT oboe_FOUND)
        message(FATAL_ERROR "Oboe nao encontrado. Clone: git clone https://github.com/google/oboe.git ${OBOE_DIR}")
    endif()
endif()

# =============================================================================
# fdk-aac (AAC Decoder)
# =============================================================================
set(FDK_AAC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/fdk-aac")
set(FDK_AAC_LIB "${FDK_AAC_DIR}/lib/${ANDROID_ABI}/libfdk-aac.a")

if (EXISTS ${FDK_AAC_LIB})
    add_library(fdk-aac STATIC IMPORTED)
    set_target_properties(fdk-aac PROPERTIES
        IMPORTED_LOCATION ${FDK_AAC_LIB}
        INTERFACE_INCLUDE_DIRECTORIES "${FDK_AAC_DIR}/include"
    )
    message(STATUS "fdk-aac encontrado: ${FDK_AAC_LIB}")
else()
    message(WARNING "fdk-aac nao encontrado. Execute: ./scripts/build-fdk-aac.sh")
    add_library(fdk-aac INTERFACE)
    target_include_directories(fdk-aac INTERFACE "${FDK_AAC_DIR}/include")
endif()

# =============================================================================
# FFmpeg (Opcional / Optional)
# =============================================================================
if (FFMPEG)
    set(FFMPEG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg/${ANDROID_ABI}")
    if (EXISTS "${FFMPEG_DIR}")
        include_directories("${FFMPEG_DIR}/include")
        link_directories("${FFMPEG_DIR}/lib")
        set(FFMPEG_LIBS avformat avcodec avutil swresample)
        add_definitions(-DUSE_FFMPEG)
        message(STATUS "FFmpeg habilitado / FFmpeg enabled: ${FFMPEG_DIR}")
    else()
        message(WARNING "FFmpeg nao encontrado em ${FFMPEG_DIR}")
        set(FFMPEG OFF)
    endif()
endif()

# =============================================================================
# Includes
# =============================================================================
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/minimp3
    ${CMAKE_CURRENT_SOURCE_DIR}/minimp4
    ${CMAKE_CURRENT_SOURCE_DIR}/fdk-aac/include
)

# =============================================================================
# Source Files
# =============================================================================
set(SOURCES
    native_bridge.cpp
    Mp3Engine.cpp
    AudioPlayer.cpp
    MetadataCache.cpp
)

set(HEADERS
    JniHelpers.h
    Mp3Engine.h
    AudioPlayer.h
    MetadataCache.h
    DataTypes.h
)

# =============================================================================
# Biblioteca Nativa / Native Library
# =============================================================================
add_library(eetgw SHARED ${SOURCES} ${HEADERS})

# =============================================================================
# Link Libraries
# =============================================================================
target_link_libraries(eetgw
    oboe
    fdk-aac
    android
    log
    atomic
)

if (FFMPEG)
    target_link_libraries(eetgw ${FFMPEG_LIBS})
endif()

# =============================================================================
# Otimizacoes por ABI / ABI Optimizations
# =============================================================================
if (${ANDROID_ABI} STREQUAL "arm64-v8a")
    target_compile_options(eetgw PRIVATE
        -march=armv8-a+fp+simd+crc+crypto
        -mtune=cortex-a53
    )
elseif (${ANDROID_ABI} STREQUAL "armeabi-v7a")
    target_compile_options(eetgw PRIVATE
        -march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=softfp
        -mtune=cortex-a7
    )
endif()

# =============================================================================
# Grupo de Sources no IDE / IDE Source Groups
# =============================================================================
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} ${HEADERS})
