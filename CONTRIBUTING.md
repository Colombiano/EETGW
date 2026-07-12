# Contributing to EETGW / Contribuindo para EETGW

🇧🇷 Obrigado pelo interesse em contribuir! Este projeto é open-source e qualquer ajuda é bem-vinda.
🇺🇸 Thank you for your interest in contributing! This project is open-source and any help is welcome.

---

## 🚀 Como Contribuir / How to Contribute

### 1. Fork e Clone

```bash
# Fork no GitHub (https://github.com/Colombiano/eetgw), depois:
git clone https://github.com/SEU_USUARIO/eetgw.git
cd eetgw
```

### 2. Branch / Crie uma branch

```bash
git checkout -b feature/minha-feature
# ou / or
git checkout -b fix/bug-corrigido
```

### 3. Commit

```bash
git add .
git commit -m "feat: adiciona suporte a FLAC / add FLAC support"
```

**🇧🇷 Convenção de commits / 🇺🇸 Commit convention:**
- `feat:` — 🇧🇷 nova funcionalidade / 🇺🇸 new feature
- `fix:` — 🇧🇷 correção de bug / 🇺🇸 bug fix
- `docs:` — 🇧🇷 documentação / 🇺🇸 documentation
- `refactor:` — 🇧🇷 refatoração de código / 🇺🇸 code refactoring
- `perf:` — 🇧🇷 melhoria de performance / 🇺🇸 performance improvement
- `test:` — 🇧🇷 testes / 🇺🇸 tests
- `i18n:` — 🇧🇷 internacionalização / 🇺🇸 internationalization

### 4. Push e PR / Push and PR

```bash
git push origin feature/minha-feature
```

🇧🇷 Abra um Pull Request no GitHub com descrição clara do que foi feito.
🇺🇸 Open a Pull Request on GitHub with a clear description of what was done.

---

## 📋 Padrões de Código / Code Standards

### C++ (NDK)
- C++17 ou superior / C++17 or higher
- RAII para gerenciamento de recursos / RAII for resource management
- 🇧🇷 Nomenclatura: `snake_case` para funções, `PascalCase` para classes
- 🇺🇸 Naming: `snake_case` for functions, `PascalCase` for classes
- Headers com `#pragma once`
- 🇧🇷 Comentários bilíngues: Português primeiro, Inglês entre parênteses
- 🇺🇸 Bilingual comments: Portuguese first, English in parentheses

### Kotlin (UI)
- Jetpack Compose para Wear OS
- Funções composable com `@Composable` / Composable functions with `@Composable`
- State hoisting para gerenciamento de estado / State hoisting for state management
- MaterialTheme para cores e tipografia / MaterialTheme for colors and typography

---

## 🧪 Testes / Testing

### C++
```bash
# 🇧🇷 Build com testes / 🇺🇸 Build with tests
./gradlew :app:externalNativeBuildDebug
```

### Kotlin
```bash
# 🇧🇷 Testes unitários / 🇺🇸 Unit tests
./gradlew test

# 🇧🇷 Testes instrumentados (emulador) / 🇺🇸 Instrumented tests (emulator)
./gradlew connectedAndroidTest
```

---

## 📁 Estrutura de Arquivos / File Structure

```
app/src/main/
├── java/com/eetgw/
│   ├── ui/              # 🇧🇷 Telas Compose / 🇺🇸 Compose screens
│   ├── jni/             # 🇧🇷 Classes JNI / 🇺🇸 JNI bridge classes
│   └── service/         # 🇧🇷 Serviço foreground / 🇺🇸 Foreground service
├── cpp/
│   ├── minimp3/         # 🇧🇷 MP3 decoder (single-header)
│   ├── minimp4/         # 🇧🇷 MP4 demuxer (single-header)
│   ├── fdk-aac/         # 🇧🇷 AAC decoder (precompilado)
│   ├── mp3_engine.cpp   # 🇧🇷 Engine principal / 🇺🇸 Main engine
│   ├── audio_player.cpp # 🇧🇷 Saída de áudio Oboe / 🇺🇸 Oboe audio output
│   └── ...
└── res/                 # 🇧🇷 Recursos / 🇺🇸 Resources
```

---

## 🐛 Reportando Bugs / Reporting Bugs

🇧🇷 Abra uma **Issue** no GitHub com:
🇺🇸 Open an **Issue** on GitHub with:

1. 🇧🇷 Descrição do problema / 🇺🇸 Problem description
2. 🇧🇷 Passos para reproduzir / 🇺🇸 Steps to reproduce
3. 🇧🇷 Comportamento esperado vs atual / 🇺🇸 Expected vs actual behavior
4. 🇧🇷 Logs do logcat (`adb logcat | grep EETGW`) / 🇺🇸 Logcat logs (`adb logcat | grep EETGW`)
5. 🇧🇷 Versão do Wear OS e modelo do relógio / 🇺🇸 Wear OS version and watch model

---

## 💡 Sugestões de Features / Feature Suggestions

🇧🇷 Abra uma **Issue** com label `enhancement` descrevendo:
🇺🇸 Open an **Issue** with label `enhancement` describing:

- 🇧🇷 O que a feature faz / 🇺🇸 What the feature does
- 🇧🇷 Por que é útil / 🇺🇸 Why it's useful
- 🇧🇷 Como poderia ser implementada (opcional) / 🇺🇸 How it could be implemented (optional)

---

## 📝 Documentação / Documentation

🇧🇷 Toda documentação deve ser **bilíngue**: Português primeiro, Inglês entre parênteses ou em seções separadas.
🇺🇸 All documentation must be **bilingual**: Portuguese first, English in parentheses or in separate sections.

- 🇧🇷 Atualize `README.md` se adicionar features visíveis ao usuário
- 🇺🇸 Update `README.md` if you add user-visible features
- 🇧🇷 Atualize `docs/ARCHITECTURE.md` se mudar a arquitetura
- 🇺🇸 Update `docs/ARCHITECTURE.md` if you change architecture
- 🇧🇷 Atualize `docs/BUILD.md` se mudar o processo de build
- 🇺🇸 Update `docs/BUILD.md` if you change build process

---

## 🏷️ Labels das Issues / Issue Labels

| Label | 🇧🇷 Descrição | 🇺🇸 Description |
|-------|-------------|----------------|
| `bug` | Algo está quebrado | Something is broken |
| `enhancement` | Nova feature | New feature |
| `documentation` | Docs | Documentation |
| `good first issue` | Bom para iniciantes | Good for beginners |
| `help wanted` | Precisa de ajuda | Help wanted |
| `performance` | Performance | Performance |
| `wear-os` | Específico do Wear OS | Wear OS specific |
| `i18n` | Internacionalização | Internationalization |

---

## 🙏 Agradecimentos / Acknowledgments

🇧🇷 Contribuidores serão listados no `README.md` na seção "Contributors".
🇺🇸 Contributors will be listed in `README.md` under the "Contributors" section.

---

🇧🇷 Dúvidas? Abra uma issue.
🇺🇸 Questions? Open an issue.
