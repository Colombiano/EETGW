cmake_minimum_required(VERSION 3.22.1)
project("eetgw")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =============================================================================
# Configuracoes / Configurations
# =============================================================================
set(FFMPEG OFF CACHE BOOL "Usar FFmpeg (maior, mais formatos) / Use FFmpeg (larger, more formats)")
set(OBOE_DIR "${CMAKE_CURRENT_SOURCE_SOURCE_DIR}/oboe" CACHE STRING "Diretorio do Oboe / Oboe directory")

# =============================================================================
# Flags do Compilador / Compiler Flags
# =============================================================================
# Nosso codigo: warnings sao warnings, nao erros
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-error")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -funroll-loops -fomit-frame-pointer")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3 -fno-omit-frame-pointer")

# =============================================================================
# Oboe (Audio de Baixa Latencia / Low Latency Audio)
# =============================================================================
if (EXISTS "${OBOE_DIR}/CMakeLists.txt")
    # Salvar flags originais / Save original flags
    set(SAVED_CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    set(SAVED_CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    
    # Para a compilacao do Oboe, garantir que warnings nao falhem o build
    # Usar regex com word boundary para nao corromper flags como -Wformat-security
    string(REGEX REPLACE "(^| )-Werror($| )" "\\1\\2" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string(REGEX REPLACE "(^| )-Werror($| )" "\\1\\2" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error -Wno-unused-parameter -Wno-sign-compare")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-error -Wno-unused-parameter -Wno-sign-compare")
    
    add_subdirectory(${OBOE_DIR} ./oboe)
    
    # Restaurar flags originais / Restore original flags
    set(CMAKE_CXX_FLAGS "${SAVED_CMAKE_CXX_FLAGS}")
    set(CMAKE_C_FLAGS "${SAVED_CMAKE_C_FLAGS}")

    # Garantir que o target oboe nao use -Werror
    if(TARGET oboe)
        get_target_property(OBOE_COMPILE_OPTS oboe COMPILE_OPTIONS)
        if(OBOE_COMPILE_OPTS)
            list(REMOVE_ITEM OBOE_COMPILE_OPTS "-Werror")
            set_target_properties(oboe PROPERTIES COMPILE_OPTIONS "${OBOE_COMPILE_OPTS}")
        endif()
        target_compile_options(oboe PRIVATE -Wno-error -Wno-unused-parameter)
    endif()
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
    Mp4Engine.cpp
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