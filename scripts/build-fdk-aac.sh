#!/bin/bash
# =============================================================================
# build-fdk-aac.sh
# Script automático para compilar libfdk-aac para Android NDK
# Suporta: Linux, macOS, Windows (via MSYS2/WSL)
# =============================================================================

set -e  # Exit on error

# Cores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configurações padrão
NDK_VERSION="${NDK_VERSION:-26.1.10909125}"
API_LEVEL="${API_LEVEL:-21}"
BUILD_DIR="${BUILD_DIR:-$(pwd)/build-fdk-aac}"
INSTALL_PREFIX="${INSTALL_PREFIX:-$(pwd)/fdk-aac-installed}"

# ABIs para compilar
ABIS=("arm64-v8a" "armeabi-v7a")

# =============================================================================
# Funções
# =============================================================================

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

detect_os() {
    case "$(uname -s)" in
        Linux*)     OS=Linux;;
        Darwin*)    OS=Mac;;
        CYGWIN*|MINGW*|MSYS*) OS=Windows;;
        *)          OS="UNKNOWN:$(uname -s)";;
    esac
    log_info "Sistema detectado: $OS"
}

find_ndk() {
    # Tenta encontrar NDK automaticamente
    if [ -n "$ANDROID_NDK_HOME" ]; then
        NDK="$ANDROID_NDK_HOME"
    elif [ -n "$ANDROID_SDK_ROOT" ]; then
        NDK="$ANDROID_SDK_ROOT/ndk/$NDK_VERSION"
    elif [ -d "$HOME/Android/Sdk/ndk/$NDK_VERSION" ]; then
        NDK="$HOME/Android/Sdk/ndk/$NDK_VERSION"
    elif [ -d "$HOME/Library/Android/sdk/ndk/$NDK_VERSION" ]; then
        NDK="$HOME/Library/Android/sdk/ndk/$NDK_VERSION"
    else
        log_error "NDK não encontrado. Defina ANDROID_NDK_HOME ou ANDROID_SDK_ROOT"
        exit 1
    fi

    if [ ! -d "$NDK" ]; then
        log_error "NDK não encontrado em: $NDK"
        log_info "Instale via: sdkmanager --install \"ndk;$NDK_VERSION\""
        exit 1
    fi

    log_info "NDK encontrado: $NDK"
}

get_toolchain() {
    case "$OS" in
        Linux)  TOOLCHAIN_PREBUILT="linux-x86_64";;
        Mac)    TOOLCHAIN_PREBUILT="darwin-x86_64";;
        Windows) TOOLCHAIN_PREBUILT="windows-x86_64";;
    esac

    TOOLCHAIN="$NDK/toolchains/llvm/prebuilt/$TOOLCHAIN_PREBUILT"

    if [ ! -d "$TOOLCHAIN" ]; then
        log_error "Toolchain não encontrada: $TOOLCHAIN"
        exit 1
    fi

    log_info "Toolchain: $TOOLCHAIN"
}

get_abi_config() {
    local abi=$1
    case "$abi" in
        arm64-v8a)
            TARGET="aarch64-linux-android"
            ARCH="arm64"
            ;;
        armeabi-v7a)
            TARGET="armv7a-linux-androideabi"
            ARCH="arm"
            ;;
        x86_64)
            TARGET="x86_64-linux-android"
            ARCH="x86_64"
            ;;
        x86)
            TARGET="i686-linux-android"
            ARCH="x86"
            ;;
        *)
            log_error "ABI não suportada: $abi"
            exit 1
            ;;
    esac

    export CC="$TOOLCHAIN/bin/${TARGET}${API_LEVEL}-clang"
    export CXX="$TOOLCHAIN/bin/${TARGET}${API_LEVEL}-clang++"
    export AR="$TOOLCHAIN/bin/llvm-ar"
    export AS="$TOOLCHAIN/bin/llvm-as"
    export LD="$TOOLCHAIN/bin/ld"
    export RANLIB="$TOOLCHAIN/bin/llvm-ranlib"
    export STRIP="$TOOLCHAIN/bin/llvm-strip"
    export NM="$TOOLCHAIN/bin/llvm-nm"

    export CFLAGS="-fPIC -O3 -ffunction-sections -fdata-sections"
    export LDFLAGS="-Wl,--gc-sections"

    log_info "Compilando para $abi (target: $TARGET)"
}

clone_fdk_aac() {
    if [ ! -d "$BUILD_DIR/fdk-aac" ]; then
        log_info "Clonando fdk-aac..."
        git clone --depth 1 https://github.com/mstorsjo/fdk-aac.git "$BUILD_DIR/fdk-aac"
    else
        log_info "fdk-aac já clonado, atualizando..."
        cd "$BUILD_DIR/fdk-aac"
        git pull
        cd -
    fi

    # Gera o script configure a partir do configure.ac
    # (o configure não está no repositório, precisa ser gerado)
    log_info "Gerando script configure com autoreconf..."
    cd "$BUILD_DIR/fdk-aac"
    if ! command -v autoreconf &> /dev/null; then
        log_error "autoreconf não encontrado. Instale com: sudo apt install autoconf automake libtool"
        exit 1
    fi
    autoreconf -fi 2>&1 | tee autoreconf.log
    if [ ${PIPESTATUS[0]} -ne 0 ]; then
        log_error "autoreconf falhou. Veja: $BUILD_DIR/fdk-aac/autoreconf.log"
        exit 1
    fi
    cd -
}

build_abi() {
    local abi=$1
    local build_abi_dir="$BUILD_DIR/build-$abi"
    local install_abi_dir="$INSTALL_PREFIX/$abi"

    log_info "========================================="
    log_info "Build iniciado: $abi"
    log_info "========================================="

    # Configura toolchain para este ABI
    get_abi_config "$abi"

    # Limpa e cria diretório de build
    rm -rf "$build_abi_dir"
    mkdir -p "$build_abi_dir"
    cd "$build_abi_dir"

    # Configure
    log_info "Configurando..."
    "$BUILD_DIR/fdk-aac/configure" \
        --host="$TARGET" \
        --with-sysroot="$TOOLCHAIN/sysroot" \
        --enable-static \
        --disable-shared \
        --prefix="$install_abi_dir" \
        CC="$CC" \
        CXX="$CXX" \
        AR="$AR" \
        RANLIB="$RANLIB" \
        STRIP="$STRIP" \
        CFLAGS="$CFLAGS" \
        LDFLAGS="$LDFLAGS" \
        2>&1 | tee configure.log

    if [ ${PIPESTATUS[0]} -ne 0 ]; then
        log_error "Configure falhou para $abi. Veja: $build_abi_dir/configure.log"
        exit 1
    fi

    # Build
    log_info "Compilando (parallel)..."
    make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4) 2>&1 | tee make.log

    if [ ${PIPESTATUS[0]} -ne 0 ]; then
        log_error "Build falhou para $abi. Veja: $build_abi_dir/make.log"
        exit 1
    fi

    # Install
    log_info "Instalando..."
    make install 2>&1 | tee install.log

    if [ ${PIPESTATUS[0]} -ne 0 ]; then
        log_error "Install falhou para $abi. Veja: $build_abi_dir/install.log"
        exit 1
    fi

    # Verifica se a biblioteca foi gerada
    if [ ! -f "$install_abi_dir/lib/libfdk-aac.a" ]; then
        log_error "Biblioteca não encontrada: $install_abi_dir/lib/libfdk-aac.a"
        exit 1
    fi

    log_info "✓ Build completo para $abi"
    log_info "  Biblioteca: $install_abi_dir/lib/libfdk-aac.a"
    log_info "  Headers: $install_abi_dir/include"

    cd -
}

copy_to_project() {
    local project_dir="${PROJECT_DIR:-$(dirname $(dirname $(realpath $0)))}"
    local cpp_dir="$project_dir/app/src/main/cpp/fdk-aac"

    log_info "Copiando para o projeto..."

    # Cria estrutura
    mkdir -p "$cpp_dir/include/fdk-aac"

    for abi in "${ABIS[@]}"; do
        mkdir -p "$cpp_dir/lib/$abi"

        if [ -f "$INSTALL_PREFIX/$abi/lib/libfdk-aac.a" ]; then
            cp "$INSTALL_PREFIX/$abi/lib/libfdk-aac.a" "$cpp_dir/lib/$abi/"
            log_info "  Copiado: $abi/libfdk-aac.a"
        else
            log_warn "  Biblioteca não encontrada para $abi"
        fi
    done

    # Copia headers (são iguais para todos ABIs)
    cp -r "$INSTALL_PREFIX/${ABIS[0]}/include/fdk-aac/"* "$cpp_dir/include/fdk-aac/"
    log_info "  Copiado: headers"

    # Cria .gitkeep para preservar estrutura
    touch "$cpp_dir/lib/.gitkeep"

    log_info "✓ Copiado para: $cpp_dir"
}

cleanup() {
    log_info "Limpando arquivos temporários..."
    rm -rf "$BUILD_DIR"
    log_info "✓ Limpo"
}

show_help() {
    cat << EOF
Uso: $0 [OPÇÕES]

Opções:
    -h, --help              Mostra esta ajuda
    -n, --ndk PATH          Caminho do NDK (ou use ANDROID_NDK_HOME)
    -v, --ndk-version VER   Versão do NDK (padrão: $NDK_VERSION)
    -a, --api-level N       API level Android (padrão: $API_LEVEL)
    -b, --build-dir DIR     Diretório de build (padrão: $BUILD_DIR)
    -i, --install DIR       Diretório de instalação (padrão: $INSTALL_PREFIX)
    -p, --project-dir DIR   Diretório do projeto para copiar
    --abi ABI               Compilar apenas um ABI (arm64-v8a, armeabi-v7a, x86_64, x86)
    --no-copy               Não copiar para o projeto
    --no-cleanup            Não limpar após build

Exemplos:
    $0                      # Build padrão
    $0 --abi arm64-v8a      # Apenas ARM64
    $0 -n /opt/android-ndk  # NDK específico

EOF
}

# =============================================================================
# Main
# =============================================================================

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -n|--ndk)
            NDK="$2"
            shift 2
            ;;
        -v|--ndk-version)
            NDK_VERSION="$2"
            shift 2
            ;;
        -a|--api-level)
            API_LEVEL="$2"
            shift 2
            ;;
        -b|--build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        -i|--install)
            INSTALL_PREFIX="$2"
            shift 2
            ;;
        -p|--project-dir)
            PROJECT_DIR="$2"
            shift 2
            ;;
        --abi)
            ABIS=("$2")
            shift 2
            ;;
        --no-copy)
            NO_COPY=1
            shift
            ;;
        --no-cleanup)
            NO_CLEANUP=1
            shift
            ;;
        *)
            log_error "Opção desconhecida: $1"
            show_help
            exit 1
            ;;
    esac
done

log_info "========================================="
log_info "Build fdk-aac para Android"
log_info "========================================="
log_info "NDK Version: $NDK_VERSION"
log_info "API Level: $API_LEVEL"
log_info "ABIs: ${ABIS[*]}"
log_info "Build Dir: $BUILD_DIR"
log_info "Install Dir: $INSTALL_PREFIX"

# Detecta OS
detect_os

# Encontra NDK
find_ndk

# Toolchain
get_toolchain

# Clone
clone_fdk_aac

# Build para cada ABI
for abi in "${ABIS[@]}"; do
    build_abi "$abi"
done

# Copia para o projeto
if [ -z "$NO_COPY" ]; then
    copy_to_project
fi

# Cleanup
if [ -z "$NO_CLEANUP" ]; then
    cleanup
fi

log_info "========================================="
log_info "✓ BUILD COMPLETO"
log_info "========================================="
log_info "Bibliotecas compiladas para:"
for abi in "${ABIS[@]}"; do
    if [ -f "$INSTALL_PREFIX/$abi/lib/libfdk-aac.a" ]; then
        size=$(du -h "$INSTALL_PREFIX/$abi/lib/libfdk-aac.a" | cut -f1)
        log_info "  $abi: $size"
    fi
done

if [ -z "$NO_COPY" ]; then
    log_info ""
    log_info "Próximo passo:"
    log_info "  cd $PROJECT_DIR"
    log_info "  ./gradlew assembleDebug"
fi
