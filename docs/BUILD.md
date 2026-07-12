# Build Guide / Guia de Build

🇧🇷 Guia completo para compilar o EETGW em diferentes plataformas.
🇺🇸 Complete guide to build EETGW on different platforms.

---

## ⚠️ NOTA IMPORTANTE / IMPORTANT NOTE

🇧🇷 **Antes de compilar, execute os passos de preparação abaixo:**
🇺🇸 **Before building, follow the preparation steps below:**

1. Clone o repositório / Clone the repo: `git clone https://github.com/Colombiano/EETGW.git`
2. Instale o Oboe / Install Oboe: `git clone https://github.com/google/oboe.git app/src/main/cpp/oboe`
3. Compile o fdk-aac / Build fdk-aac: `./scripts/build-fdk-aac.sh`
4. Gere o Gradle Wrapper / Generate Gradle Wrapper: `gradle wrapper --gradle-version 8.2`
5. Compile / Build: `./gradlew assembleDebug`

---

## 📋 Requisitos / Requirements

### 🇧🇷 Todos os Sistemas / 🇺🇸 All Systems
- Android Studio Hedgehog (2023.1.1) ou superior / or higher
- Android SDK 34 (compileSdk)
- Android NDK r25b ou r26b / or r26b
- CMake 3.22.1+
- Git

### 🇧🇷 Versões Testadas / 🇺🇸 Tested Versions
| Componente / Component | Versão / Version |
|------------------------|------------------|
| Android Studio | 2023.1.1 |
| Gradle | 8.2 |
| Kotlin | 1.9.0 |
| NDK | r26b |
| CMake | 3.22.1 |
| Wear OS | 3.5+ |

---

## 🐧 Linux (Ubuntu/Debian) — Recomendado / Recommended

### 1. 🇧🇷 Instalar dependências / 🇺🇸 Install dependencies

```bash
sudo apt update
sudo apt install -y \
    openjdk-17-jdk \
    git \
    cmake \
    ninja-build \
    libncurses5 \
    libncursesw5 \
    autoconf \
    automake \
    libtool \
    curl
```

### 2. 🇧🇷 Android Studio + NDK / 🇺🇸 Android Studio + NDK

```bash
# Download do Android Studio / Download Android Studio
wget https://redirector.gvt1.com/edgedl/android/studio/ide-zips/2023.1.1.26/android-studio-2023.1.1.26-linux.tar.gz
tar -xzf android-studio-*.tar.gz
sudo mv android-studio /opt/

# Launcher
sudo ln -s /opt/android-studio/bin/studio.sh /usr/local/bin/android-studio

# Inicie o Android Studio e instale o NDK pelo SDK Manager:
# Tools > SDK Manager > SDK Tools > NDK (Side by side) > 26.1.10909125
```

### 3. 🇧🇷 Variáveis de ambiente / 🇺🇸 Environment variables

```bash
# ~/.bashrc
export ANDROID_SDK_ROOT=$HOME/Android/Sdk
export ANDROID_NDK_HOME=$ANDROID_SDK_ROOT/ndk/26.1.10909125
export PATH=$PATH:$ANDROID_SDK_ROOT/platform-tools:$ANDROID_SDK_ROOT/cmdline-tools/latest/bin
```

```bash
source ~/.bashrc
```

### 4. 🇧🇷 Instalar NDK via command line (alternativa) / 🇺🇸 Install NDK via command line (alternative)

```bash
sdkmanager --install "ndk;26.1.10909125"
```

### 5. 🇧🇷 Preparar o projeto / 🇺🇸 Prepare the project

```bash
# Clone o repositório
git clone https://github.com/Colombiano/EETGW.git
cd EETGW

# Clone o Oboe (obrigatório)
git clone https://github.com/google/oboe.git app/src/main/cpp/oboe

# Baixe minimp3 e minimp4 (single-header libraries)
mkdir -p app/src/main/cpp/minimp3 app/src/main/cpp/minimp4
curl -L https://raw.githubusercontent.com/lieff/minimp3/master/minimp3.h > app/src/main/cpp/minimp3/minimp3.h
curl -L https://raw.githubusercontent.com/lieff/minimp3/master/minimp3_ex.h > app/src/main/cpp/minimp3/minimp3_ex.h
curl -L https://raw.githubusercontent.com/lieff/minimp4/master/minimp4.h > app/src/main/cpp/minimp4/minimp4.h

# Compile o fdk-aac
chmod +x scripts/build-fdk-aac.sh
./scripts/build-fdk-aac.sh
```

### 6. 🇧🇷 Instalar Gradle e gerar Wrapper / 🇺🇸 Install Gradle and generate Wrapper

```bash
# Instale o Gradle 8.2 (se ainda não tiver)
cd /tmp
wget https://services.gradle.org/distributions/gradle-8.2-bin.zip
sudo mkdir -p /opt/gradle
sudo unzip -d /opt/gradle gradle-8.2-bin.zip
export PATH=$PATH:/opt/gradle/gradle-8.2/bin

# Volte ao projeto e gere o wrapper
cd /caminho/para/EETGW
gradle wrapper --gradle-version 8.2

# Isso cria: gradlew, gradlew.bat, e gradle/wrapper/gradle-wrapper.jar
```

### 7. 🇧🇷 Build / 🇺🇸 Build

```bash
./gradlew assembleDebug

# APK gerado em:
# app/build/outputs/apk/debug/app-debug.apk
```

---

## 🪟 Windows

### 1. 🇧🇷 Instalar Android Studio / 🇺🇸 Install Android Studio

```powershell
# Baixe em: https://developer.android.com/studio
# Download at: https://developer.android.com/studio
# Instale com NDK e CMake / Install with NDK and CMake
```

### 2. 🇧🇷 Configurar variáveis de ambiente / 🇺🇸 Configure environment variables

```powershell
# PowerShell como Administrador / as Administrator
[Environment]::SetEnvironmentVariable("ANDROID_SDK_ROOT", "C:\Users\$env:USERNAME\AppData\Local\Android\Sdk", "User")
[Environment]::SetEnvironmentVariable("ANDROID_NDK_HOME", "$env:ANDROID_SDK_ROOT\ndk\26.1.10909125", "User")

# Recarregue o PowerShell / Reload PowerShell
```

### 3. 🇧🇷 Preparar o projeto / 🇺🇸 Prepare the project

```powershell
# Clone
git clone https://github.com/Colombiano/EETGW.git
cd EETGW

# Clone o Oboe
git clone https://github.com/google/oboe.git app\src\main\cpp\oboe

# Baixe minimp3 e minimp4
# (Use curl ou navegador para baixar os arquivos .h para as pastas minimp3/ e minimp4/)

# Compile fdk-aac (requer MSYS2 ou WSL)
# Veja a seção Linux acima para o script build-fdk-aac.sh
```

### 4. 🇧🇷 Build / 🇺🇸 Build

```powershell
cd EETGW
gradle wrapper --gradle-version 8.2
.\gradlew.bat assembleDebug
```

---

## 🍎 macOS

### 1. 🇧🇷 Instalar dependências / 🇺🇸 Install dependencies

```bash
# Homebrew
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

brew install android-sdk cmake ninja
```

### 2. 🇧🇷 Variáveis de ambiente / 🇺🇸 Environment variables

```bash
# ~/.zshrc ou ~/.bash_profile
export ANDROID_SDK_ROOT=$HOME/Library/Android/sdk
export ANDROID_NDK_HOME=$ANDROID_SDK_ROOT/ndk/26.1.10909125
export PATH=$PATH:$ANDROID_SDK_ROOT/platform-tools
```

```bash
source ~/.zshrc
```

### 3. 🇧🇷 Build / 🇺🇸 Build

```bash
cd EETGW
git clone https://github.com/google/oboe.git app/src/main/cpp/oboe
chmod +x scripts/build-fdk-aac.sh
./scripts/build-fdk-aac.sh
gradle wrapper --gradle-version 8.2
./gradlew assembleDebug
```

---

## 🐧 Linux (Arch/Manjaro)

```bash
# Pacotes / Packages
sudo pacman -S android-studio android-sdk android-ndk cmake ninja jdk17-openjdk \
    autoconf automake libtool curl

# O restante é igual ao Ubuntu / The rest is the same as Ubuntu
```

---

## 🎯 Build Variants / Variantes de Build

### 🇧🇷 Debug (desenvolvimento) / 🇺🇸 Debug (development)
```bash
./gradlew assembleDebug
# APK: app/build/outputs/apk/debug/app-debug.apk
```

### 🇧🇷 Release (distribuição) / 🇺🇸 Release (distribution)
```bash
# Crie keystore primeiro / Create keystore first
keytool -genkey -v -keystore eetgw.keystore -alias eetgw -keyalg RSA -keysize 2048 -validity 10000

# Build release / Build release
./gradlew assembleRelease
# APK: app/build/outputs/apk/release/app-release-unsigned.apk

# Assine / Sign
jarsigner -verbose -sigalg SHA1withRSA -digestalg SHA1 \
    -keystore eetgw.keystore \
    app/build/outputs/apk/release/app-release-unsigned.apk \
    eetgw

# Alinhe / Align
zipalign -v 4 \
    app/build/outputs/apk/release/app-release-unsigned.apk \
    app/build/outputs/apk/release/app-release.apk
```

---

## 🐛 Troubleshooting / Solução de Problemas

### "./gradlew: Arquivo ou diretório inexistente"
```bash
# O gradle-wrapper.jar não foi gerado. Execute:
gradle wrapper --gradle-version 8.2
# ou / or:
/opt/gradle/gradle-8.2/bin/gradle wrapper --gradle-version 8.2
```

### "gradle: comando não encontrado"
```bash
# Instale o Gradle manualmente:
cd /tmp
wget https://services.gradle.org/distributions/gradle-8.2-bin.zip
sudo mkdir -p /opt/gradle
sudo unzip -d /opt/gradle gradle-8.2-bin.zip
export PATH=$PATH:/opt/gradle/gradle-8.2/bin
echo 'export PATH=$PATH:/opt/gradle/gradle-8.2/bin' >> ~/.bashrc
```

### "CMake not found"
```bash
# Android Studio > SDK Manager > SDK Tools > CMake > Install
# Ou / Or:
sdkmanager --install "cmake;3.22.1"
```

### "NDK not found"
```bash
# Verifique / Check:
ls $ANDROID_NDK_HOME

# Se vazio / If empty:
sdkmanager --install "ndk;26.1.10909125"

# Ou defina a variável correta / Or set the correct variable:
export ANDROID_NDK_HOME=$ANDROID_SDK_ROOT/ndk/26.1.10909125
```

### "Oboe não encontrado" / "Oboe not found"
```bash
# Clone o Oboe na pasta correta:
git clone https://github.com/google/oboe.git app/src/main/cpp/oboe
```

### "fdk-aac not found"
```bash
# Verifique se a biblioteca está no lugar correto
# Check if library is in correct place:
ls app/src/main/cpp/fdk-aac/lib/arm64-v8a/libfdk-aac.a

# Se não existir, compile primeiro (veja seção 5 acima)
# If not found, build first (see section 5 above):
./scripts/build-fdk-aac.sh
```

### "autoreconf: not found" (ao compilar fdk-aac)
```bash
sudo apt install -y autoconf automake libtool
```

### "typedef redefinition" ou "bs_t conflito"
```bash
# ✅ CORRIGIDO: minimp3 e minimp4 estão separados em arquivos .cpp diferentes
# ✅ FIXED: minimp3 and minimp4 are in separate .cpp files
# Se ainda ocorrer, atualize o repositório:
git pull origin main
```

### "unknown type name 'JniClass'"
```bash
# ✅ CORRIGIDO: O include de JniHelpers.h está correto no native_bridge.cpp
# ✅ FIXED: JniHelpers.h include is correct in native_bridge.cpp
# Atualize o repositório:
git pull origin main
```

### "mipmap/ic_launcher not found"
```bash
# ✅ CORRIGIDO: Ícone fallback adicionado em res/mipmap/ic_launcher.xml
# ✅ FIXED: Fallback icon added at res/mipmap/ic_launcher.xml
# Atualize o repositório:
git pull origin main
```

### "-Werror" no Oboe
```bash
# ✅ CORRIGIDO: O CMakeLists.txt agora remove -Werror do target Oboe
# ✅ FIXED: CMakeLists.txt now removes -Werror from Oboe target
# Atualize o repositório:
git pull origin main
# Limpe o cache do CMake e recompile:
./gradlew clean
./gradlew assembleDebug
```

### "ABI not supported"
```bash
# Verifique se o NDK suporta seu ABI
# Check if NDK supports your ABI:
ls $ANDROID_NDK_HOME/toolchains/llvm/prebuilt/*/bin/ | grep aarch64

# Deve mostrar: aarch64-linux-android21-clang
# Should show: aarch64-linux-android21-clang
```

### 🇧🇷 Build lento / 🇺🇸 Slow build
```bash
# Use mais threads / Use more threads:
./gradlew assembleDebug --parallel --build-cache --configure-on-demand

# Ou no gradle.properties / Or in gradle.properties:
org.gradle.parallel=true
org.gradle.caching=true
org.gradle.configureondemand=true
org.gradle.jvmargs=-Xmx4096m
```

### "MINIMP3_IMPLEMENTATION" ou "MINIMP4_IMPLEMENTATION" redefinido
```bash
# ✅ CORRIGIDO: As implementações são definidas apenas nos .cpp respectivos
# ✅ FIXED: Implementations are defined only in their respective .cpp files
```

---

## 📱 🇧🇷 Instalação no Relógio / 🇺🇸 Installation on Watch

### 🇧🇷 Via ADB (desenvolvimento) / 🇺🇸 Via ADB (development)
```bash
# Conecte o relógio via WiFi ou USB
# Connect watch via WiFi or USB:
adb connect 192.168.1.100:5555  # IP do relógio / watch IP

# Instale / Install:
adb install app/build/outputs/apk/debug/app-debug.apk

# Logs / Logs:
adb logcat | grep EETGW
```

### 🇧🇷 Via Android Studio / 🇺🇸 Via Android Studio
```
Run > Run 'app'
# Selecione o emulador ou dispositivo Wear OS
# Select Wear OS emulator or device
```

---

## 🔄 Build Automatizado (CI/CD) / Automated Build (CI/CD)

🇧🇷 Veja `.github/workflows/android.yml` para configuração GitHub Actions.
🇺🇸 See `.github/workflows/android.yml` for GitHub Actions configuration.

---

🇧🇷 **Resumo dos erros corrigidos nesta versão:**
🇺🇸 **Summary of fixes in this version:**

1. ✅ **AudioPlayer.h** — Adicionado `#include <functional>`
2. ✅ **CMakeLists.txt** — Removido `-Werror` do Oboe com proteção robusta
3. ✅ **DataTypes.h** — Corrigido memory leak no destrutor de Mp4Context
4. ✅ **mipmap/ic_launcher.xml** — Adicionado ícone fallback
5. ✅ **native_bridge.cpp** — Include de JniHelpers.h verificado e correto
6. ✅ **Mp3Engine.cpp/Mp4Engine.cpp** — Separados para evitar conflito de `bs_t`

🇧🇷 Dúvidas? Abra uma issue ou consulte `CONTRIBUTING.md`.
🇺🇸 Questions? Open an issue or see `CONTRIBUTING.md`.