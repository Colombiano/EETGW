# EETGW — Essa é pra tocar no Galaxy Watch

> 🇧🇷 **Essa é pra tocar no Galaxy Watch** — uma homenagem à canção *"Essa é pra tocar no rádio"* de Gilberto Gil, do álbum *Gil e Jorge* (1975), gravado ao vivo no Festival de Montreux com Jorge Ben Jor. Assim como a música era para o rádio, este player é para o seu pulso.

> 🇺🇸 **This one's for playing on the Galaxy Watch** — a tribute to the song *"Essa é pra tocar no rádio"* by Gilberto Gil, from the album *Gil e Jorge* (1975), recorded live at the Montreux Festival with Jorge Ben Jor. Just as the song was for the radio, this player is for your wrist.

<p align="center">
  <img src="docs/screenshots/player_normal.png" width="200" alt="Modo Normal / Normal Mode"/>
  <img src="docs/screenshots/player_running.png" width="200" alt="Modo Running / Running Mode"/>
</p>

---

## 🎸 A Referência Musical / The Musical Reference

🇧🇷 O nome **EETGW** é um acrônimo de *"Essa é pra tocar no Galaxy Watch"*, uma brincadeira com a faixa *"Essa é pra tocar no rádio"* do álbum **Gil e Jorge** (1975). Gravado ao vivo no Festival de Montreux, o álbum é um marco da música brasileira — uma jam session improvisada de quase 40 minutos entre dois gigantes. Assim como Gilberto Gil e Jorge Ben Jor transformaram o rádio em algo especial, queremos transformar seu Galaxy Watch em um player de música nativo, poderoso e elegante.

🇺🇸 The name **EETGW** is an acronym for *"Essa é pra tocar no Galaxy Watch"* (Portuguese for "This one's for playing on the Galaxy Watch"), a play on the track *"Essa é pra tocar no rádio"* from the album **Gil e Jorge** (1975). Recorded live at the Montreux Festival, the album is a landmark of Brazilian music — an improvised jam session of almost 40 minutes between two giants. Just as Gilberto Gil and Jorge Ben Jor made the radio special, we want to turn your Galaxy Watch into a native, powerful, and elegant music player.

---

## 🛠️ Tecnologias / Tech Stack

<p align="center">
  <img src="https://img.shields.io/badge/Kotlin-1.9.0-7F52FF?logo=kotlin&logoColor=white" alt="Kotlin"/>
  <img src="https://img.shields.io/badge/C++-17-00599C?logo=c%2B%2B&logoColor=white" alt="C++17"/>
  <img src="https://img.shields.io/badge/Android-NDK%20r26b-3DDC84?logo=android&logoColor=white" alt="Android NDK"/>
  <img src="https://img.shields.io/badge/Wear%20OS-3.5+-4285F4?logo=wear-os&logoColor=white" alt="Wear OS"/>
  <img src="https://img.shields.io/badge/Jetpack%20Compose-Wear-4285F4?logo=jetpack-compose&logoColor=white" alt="Jetpack Compose"/>
  <img src="https://img.shields.io/badge/Oboe-1.8.0-4285F4?logo=google&logoColor=white" alt="Oboe"/>
  <img src="https://img.shields.io/badge/minimp3-Single--header-FF6B6B?logo=c&logoColor=white" alt="minimp3"/>
  <img src="https://img.shields.io/badge/minimp4-Single--header-FF6B6B?logo=c&logoColor=white" alt="minimp4"/>
  <img src="https://img.shields.io/badge/fdk--aac-Fraunhofer-00A4EF?logo=audio-technica&logoColor=white" alt="fdk-aac"/>
  <img src="https://img.shields.io/badge/CMake-3.22.1+-064F8C?logo=cmake&logoColor=white" alt="CMake"/>
  <img src="https://img.shields.io/badge/Gradle-8.2-02303A?logo=gradle&logoColor=white" alt="Gradle"/>
  <img src="https://img.shields.io/badge/License-MIT-yellow.svg" alt="License: MIT"/>
</p>

---

## ✨ Funcionalidades / Features

| 🇧🇷 Funcionalidade | 🇺🇸 Feature | Descrição / Description | Status |
|---------------------|------------|-------------------------|--------|
| Reprodução MP3 | MP3 Playback | Decodificação via minimp3 / Decoding via minimp3 | ✅ |
| AAC/MP4 | AAC/MP4 | Demuxing minimp4 + fdk-aac | ✅ |
| HE-AAC v2 | HE-AAC v2 | Suporte a 48kbps / 48kbps support | ✅ |
| Streaming | Streaming | Chunks de 64KB, não carrega tudo / 64KB chunks, doesn't load entire file | ✅ |
| Seek Slice | Seek Slice | Deslize horizontal para navegar / Horizontal swipe to navigate | ✅ |
| Volume Slice | Volume Slice | Deslize horizontal para volume / Horizontal swipe for volume | ✅ |
| Modo Running | Running Mode | Interface simplificada para exercício / Simplified interface for exercise | ✅ |
| Cache | Cache | Metadados em cache com TTL / Metadata cache with TTL | ✅ |
| Tema Terminal | Terminal Theme | Paleta "Northern Lights" / "Northern Lights" palette | ✅ |

---

## 🎨 Interface

### 🇧🇷 Modo Normal / 🇺🇸 Normal Mode
- Anel de progresso com tempo central / Progress ring with central time
- Dual slices: Seek (ciano) e Volume (magenta) / Dual slices: Seek (cyan) and Volume (magenta)
- Controles: Play, Pause, Stop, Previous, Next / Controls: Play, Pause, Stop, Previous, Next
- Metadados: título, artista, álbum, formato / Metadata: title, artist, album, format

### 🇧🇷 Modo Running / 🇺🇸 Running Mode
- Play/Pause gigante (80dp) / Giant Play/Pause (80dp)
- Seek slice expandido (ocupa metade da tela) / Expanded seek slice (half screen)
- Números grandes, mínimo texto / Large numbers, minimal text
- Volume via bezel físico / Volume via physical bezel

---

## 🏗️ Arquitetura / Architecture

```
Kotlin UI (Jetpack Compose)
    ↓ JNI
C++17 Engine
    ├── minimp3  → MP3 decoding
    ├── minimp4  → MP4 demuxing
    ├── fdk-aac  → AAC decoding (HE-AAC v2)
    └── Oboe     → Audio playback (AAudio/OpenSL ES)
```

🇧🇷 Veja [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) para detalhes completos.
🇺🇸 See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for complete details.

---

## 🚀 Quick Start

### 🇧🇷 Pré-requisitos / 🇺🇸 Prerequisites
- Android Studio Hedgehog+
- Android NDK r26b
- CMake 3.22.1+

### 1. Clone

```bash
git clone https://github.com/Colombiano/eetgw.git
cd eetgw
```

### 2. 🇧🇷 Compile fdk-aac / 🇺🇸 Build fdk-aac

```bash
chmod +x scripts/build-fdk-aac.sh
./scripts/build-fdk-aac.sh
```

### 3. 🇧🇷 Build / 🇺🇸 Build

```bash
./gradlew assembleDebug
```

### 4. 🇧🇷 Instale / 🇺🇸 Install

```bash
adb install app/build/outputs/apk/debug/app-debug.apk
```

🇧🇷 Veja [docs/BUILD.md](docs/BUILD.md) para guia detalhado por plataforma.
🇺🇸 See [docs/BUILD.md](docs/BUILD.md) for detailed platform guide.

---

## 📋 Roadmap

- [ ] FLAC decoder (libflac)
- [ ] 🇧🇷 Playlist com shuffle / 🇺🇸 Playlist with shuffle
- [ ] 🇧🇷 Equalizador 5-band / 🇺🇸 5-band equalizer
- [ ] 🇧🇷 Sincronização com celular / 🇺🇸 Phone sync
- [ ] 🇧🇷 Widget para tela inicial / 🇺🇸 Home screen widget
- [ ] 🇧🇷 Suporte a Opus / 🇺🇸 Opus support

---

## 🤝 Contributing

🇧🇷 Veja [CONTRIBUTING.md](CONTRIBUTING.md) para guia de contribuição.
🇺🇸 See [CONTRIBUTING.md](CONTRIBUTING.md) for contribution guidelines.

---

## 📄 License

[MIT License](LICENSE) — Luiz Paulo Colombiano

---

## 🙏 Agradecimentos / Acknowledgments

- [Gilberto Gil](https://www.gilbertogil.com.br/) & [Jorge Ben Jor](https://jorgebenjor.com.br/) — pela inspiração musical / for the musical inspiration
- [minimp3](https://github.com/lieff/minimp3) — Dmitry
- [minimp4](https://github.com/lieff/minimp4) — Dmitry
- [fdk-aac](https://github.com/mstorsjo/fdk-aac) — Martin Storsjö
- [Oboe](https://github.com/google/oboe) — Google
- [Jetpack Compose](https://developer.android.com/jetpack/compose) — Google

---

<p align="center">
  <em>🇧🇷 "Essa é pra tocar no Galaxy Watch..."</em><br/>
  <em>🇺🇸 "This one's for playing on the Galaxy Watch..."</em>
</p>
