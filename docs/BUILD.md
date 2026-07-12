# Build Guide / Guia de Build

🇧🇷 Guia completo para compilar o EETGW em diferentes plataformas.
🇺🇸 Complete guide to build EETGW on different platforms.

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

## 🪟 Windows

### 1. 🇧🇷 Instalar Android Studio / 🇺🇸 Install Android Studio

```powershell
# 🇧🇷 Baixe em: https://developer.android.com/studio
# 🇺🇸 Download at: https://developer.android.com/studio
# 🇧🇷 Instale com NDK e CMake / 🇺🇸 Install with NDK and CMake
```

### 2. 🇧🇷 Configurar variáveis de ambiente / 🇺🇸 Configure environment variables

```powershell
# PowerShell como Administrador / as Administrator
[Environment]::SetEnvironmentVariable("ANDROID_SDK_ROOT", "C:\Users\$env:USERNAME\AppData\Local\Android\Sdk", "User")
[Environment]::SetEnvironmentVariable("ANDROID_NDK_HOME", "$env:ANDROID_SDK_ROOT\ndk\26.1.10909125", "User")

# Recarregue o PowerShell / Reload PowerShell
```

### 3. 🇧🇷 Compilar fdk-aac / 🇺🇸 Build fdk-aac

```powershell
# Abra "x64 Native Tools Command Prompt for VS 2022"
# Ou instale MSYS2: https://www.msys2.org/

# Clone
 git clone https://github.com/mstorsjo/fdk-aac.git
 cd fdk-aac

# Configure (usando MSYS2)
 pacman -S mingw-w64-x86_64-toolchain

 ./configure `
     --host=x86_64-w64-mingw32 `
     --enable-static `
     --disable-shared `
     CC=gcc `
     CXX=g++ `
     --prefix=$(pwd)/windows-build

 make -j$(nproc)
 make install

# Copie para o projeto / Copy to project
 Copy-Item windows-build/lib/libfdk-aac.a `
     ..\eetgw\app\src\main\cpp\fdk-aac\lib\arm64-v8a\
```

### 4. 🇧🇷 Build do projeto / 🇺🇸 Build project

```powershell
 cd ..\eetgw
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

### 2. 🇧🇷 Configurar Android Studio / 🇺🇸 Configure Android Studio

```bash
# 🇧🇷 Baixe Android Studio em: https://developer.android.com/studio
# 🇺🇸 Download Android Studio at: https://developer.android.com/studio
# 🇧🇷 Instale NDK via SDK Manager: Tools > SDK Manager > SDK Tools > NDK
# 🇺🇸 Install NDK via SDK Manager: Tools > SDK Manager > SDK Tools > NDK
```

### 3. 🇧🇷 Variáveis de ambiente / 🇺🇸 Environment variables

```bash
# ~/.zshrc ou ~/.bash_profile
export ANDROID_SDK_ROOT=$HOME/Library/Android/sdk
export ANDROID_NDK_HOME=$ANDROID_SDK_ROOT/ndk/26.1.10909125
export PATH=$PATH:$ANDROID_SDK_ROOT/platform-tools
```

```bash
source ~/.zshrc
```

### 4. 🇧🇷 Compilar fdk-aac / 🇺🇸 Build fdk-aac

```bash
# Clone
git clone https://github.com/mstorsjo/fdk-aac.git
cd fdk-aac

export NDK=$ANDROID_NDK_HOME
export TOOLCHAIN=$NDK/toolchains/llvm/prebuilt/darwin-x86_64
export TARGET=aarch64-linux-android21

./configure     --host=$TARGET     --with-sysroot=$TOOLCHAIN/sysroot     CC="$TOOLCHAIN/bin/$TARGET-clang"     CXX="$TOOLCHAIN/bin/$TARGET-clang++"     AR="$TOOLCHAIN/bin/llvm-ar"     STRIP="$TOOLCHAIN/bin/llvm-strip"     RANLIB="$TOOLCHAIN/bin/llvm-ranlib"     --enable-static     --disable-shared     --prefix=$(pwd)/android-build

make -j$(sysctl -n hw.ncpu)
make install

# Copie para o projeto / Copy to project
cp android-build/lib/libfdk-aac.a     ../eetgw/app/src/main/cpp/fdk-aac/lib/arm64-v8a/
```

### 5. 🇧🇷 Build / 🇺🇸 Build

```bash
cd ../eetgw
./gradlew assembleDebug
```

---

## 🐧 Linux (Ubuntu/Debian)

### 1. 🇧🇷 Instalar dependências / 🇺🇸 Install dependencies

```bash
sudo apt update
sudo apt install -y     openjdk-17-jdk     git     cmake     ninja-build     libncurses5     libncursesw5
```

### 2. 🇧🇷 Android Studio / 🇺🇸 Android Studio

```bash
# Download
wget https://redirector.gvt1.com/edgedl/android/studio/ide-zips/2023.1.1.26/android-studio-2023.1.1.26-linux.tar.gz
tar -xzf android-studio-*.tar.gz
sudo mv android-studio /opt/

# Launcher
sudo ln -s /opt/android-studio/bin/studio.sh /usr/local/bin/android-studio
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

### 4. 🇧🇷 Instalar NDK via command line / 🇺🇸 Install NDK via command line

```bash
sdkmanager --install "ndk;26.1.10909125"
```

### 5. 🇧🇷 Compilar fdk-aac (script completo) / 🇺🇸 Build fdk-aac (complete script)

```bash
cd eetgw
chmod +x scripts/build-fdk-aac.sh
./scripts/build-fdk-aac.sh
```

🇧🇷 Ou manualmente / 🇺🇸 Or manually:

```bash
git clone https://github.com/mstorsjo/fdk-aac.git
cd fdk-aac

export NDK=$ANDROID_NDK_HOME
export TOOLCHAIN=$NDK/toolchains/llvm/prebuilt/linux-x86_64
export TARGET=aarch64-linux-android21

./configure     --host=$TARGET     --with-sysroot=$TOOLCHAIN/sysroot     CC="$TOOLCHAIN/bin/$TARGET-clang"     CXX="$TOOLCHAIN/bin/$TARGET-clang++"     AR="$TOOLCHAIN/bin/llvm-ar"     STRIP="$TOOLCHAIN/bin/llvm-strip"     RANLIB="$TOOLCHAIN/bin/llvm-ranlib"     --enable-static     --disable-shared     --prefix=$(pwd)/android-build

make -j$(nproc)
make install

cp android-build/lib/libfdk-aac.a     ../eetgw/app/src/main/cpp/fdk-aac/lib/arm64-v8a/
```

### 6. 🇧🇷 Build / 🇺🇸 Build

```bash
cd ../eetgw
./gradlew assembleDebug
```

---

## 🐧 Linux (Arch/Manjaro)

```bash
# Pacotes / Packages
sudo pacman -S android-studio android-sdk android-ndk cmake ninja jdk17-openjdk

# 🇧🇷 O restante é igual ao Ubuntu / 🇺🇸 The rest is the same as Ubuntu
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
# 🇧🇷 Crie keystore primeiro / 🇺🇸 Create keystore first
keytool -genkey -v -keystore eetgw.keystore -alias eetgw -keyalg RSA -keysize 2048 -validity 10000

# 🇧🇷 Build release / 🇺🇸 Build release
./gradlew assembleRelease
# APK: app/build/outputs/apk/release/app-release-unsigned.apk

# 🇧🇷 Assine / 🇺🇸 Sign
jarsigner -verbose -sigalg SHA1withRSA -digestalg SHA1     -keystore eetgw.keystore     app/build/outputs/apk/release/app-release-unsigned.apk     eetgw

# 🇧🇷 Alinhe / 🇺🇸 Align
zipalign -v 4     app/build/outputs/apk/release/app-release-unsigned.apk     app/build/outputs/apk/release/app-release.apk
```

---

## 🐛 Troubleshooting / Solução de Problemas

### "CMake not found"
```bash
# Android Studio > SDK Manager > SDK Tools > CMake > Install
# Ou / Or:
sdkmanager --install "cmake;3.22.1"
```

### "NDK not found"
```bash
# 🇧🇷 Verifique / 🇺🇸 Check
ls $ANDROID_NDK_HOME

# 🇧🇷 Se vazio / 🇺🇸 If empty:
sdkmanager --install "ndk;26.1.10909125"
```

### "fdk-aac not found"
```bash
# 🇧🇷 Verifique se a biblioteca está no lugar correto
# 🇺🇸 Check if library is in correct place
ls app/src/main/cpp/fdk-aac/lib/arm64-v8a/libfdk-aac.a

# 🇧🇷 Se não existir, compile primeiro (veja seção 4 acima)
# 🇺🇸 If not found, build first (see section 4 above)
```

### "ABI not supported"
```bash
# 🇧🇷 Verifique se o NDK suporta seu ABI
# 🇺🇸 Check if NDK supports your ABI
ls $ANDROID_NDK_HOME/toolchains/llvm/prebuilt/*/bin/ | grep aarch64

# 🇧🇷 Deve mostrar: aarch64-linux-android21-clang
# 🇺🇸 Should show: aarch64-linux-android21-clang
```

### 🇧🇷 Build lento / 🇺🇸 Slow build
```bash
# 🇧🇷 Use mais threads / 🇺🇸 Use more threads
./gradlew assembleDebug --parallel --build-cache --configure-on-demand

# 🇧🇷 Ou no gradle.properties / 🇺🇸 Or in gradle.properties:
org.gradle.parallel=true
org.gradle.caching=true
org.gradle.configureondemand=true
org.gradle.jvmargs=-Xmx4096m
```

---

## 📱 🇧🇷 Instalação no Relógio / 🇺🇸 Installation on Watch

### 🇧🇷 Via ADB (desenvolvimento) / 🇺🇸 Via ADB (development)
```bash
# 🇧🇷 Conecte o relógio via WiFi ou USB
# 🇺🇸 Connect watch via WiFi or USB
adb connect 192.168.1.100:5555  # IP do relógio / watch IP

# 🇧🇷 Instale / 🇺🇸 Install
adb install app/build/outputs/apk/debug/app-debug.apk

# 🇧🇷 Logs / 🇺🇸 Logs
adb logcat | grep EETGW
```

### 🇧🇷 Via Android Studio / 🇺🇸 Via Android Studio
```
Run > Run 'app'
# 🇧🇷 Selecione o emulador ou dispositivo Wear OS
# 🇺🇸 Select Wear OS emulator or device
```

---

## 🔄 Build Automatizado (CI/CD) / Automated Build (CI/CD)

🇧🇷 Veja `.github/workflows/android.yml` para configuração GitHub Actions.
🇺🇸 See `.github/workflows/android.yml` for GitHub Actions configuration.

---

🇧🇷 Dúvidas? Abra uma issue ou consulte `CONTRIBUTING.md`.
🇺🇸 Questions? Open an issue or see `CONTRIBUTING.md`.
