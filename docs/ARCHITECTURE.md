# Arquitetura / Architecture

🇧🇷 Documentação da arquitetura do EETGW (Essa é pra tocar no Galaxy Watch).
🇺🇸 Architecture documentation for EETGW (This one's for playing on the Galaxy Watch).

---

## 📐 Visão Geral / Overview

```
┌─────────────────────────────────────────────────────────────┐
│                     Wear OS (Android)                       │
│  ┌─────────────────┐    ┌─────────────────┐                │
│  │  Jetpack Compose │    │  Foreground     │                │
│  │  UI Layer        │    │  Service        │                │
│  │  (Kotlin)        │    │  (Kotlin)       │                │
│  └────────┬────────┘    └─────────────────┘                │
│           │                                                 │
│  ┌────────▼─────────────────────────────────────────────┐ │
│  │              JNI Bridge (Kotlin + C++)                │ │
│  │  NativeBridge.kt  ↔  native_bridge.cpp               │ │
│  │  Declara métodos    ↔  Implementa métodos              │ │
│  │  native             ↔  extern "C" JNIEXPORT           │ │
│  └────────┬──────────────────────────────────────────────┘ │
│           │ JNI (Java Native Interface)                     │
└───────────┼─────────────────────────────────────────────────┘
            │
┌───────────▼─────────────────────────────────────────────────┐
│                    Native Layer (C++17)                        │
│  ┌──────────────┐  ┌──────────────┐  ┌─────────────────┐  │
│  │   Mp3Engine  │  │ AudioPlayer  │  │  MetadataCache  │  │
│  │  (mp3_engine)│  │ (audio_player)│  │ (metadata_cache)│  │
│  │              │  │              │  │                 │  │
│  │  ┌────────┐  │  │  ┌────────┐  │  │  ┌──────────┐  │  │
│  │  │minimp3 │  │  │  │  Oboe  │  │  │  │  LRU     │  │  │
│  │  │(MP3)   │  │  │  │(Audio) │  │  │  │  Cache   │  │  │
│  │  └────────┘  │  │  └────────┘  │  │  └──────────┘  │  │
│  │  ┌────────┐  │  │              │  │                 │  │
│  │  │minimp4 │  │  │  ┌────────┐  │  │                 │  │
│  │  │(MP4    │  │  │  │AAudio/ │  │  │                 │  │
│  │  │demux)  │  │  │  │OpenSL  │  │  │                 │  │
│  │  └────────┘  │  │  │(fallback)│  │                 │  │
│  │  ┌────────┐  │  │  └────────┘  │  │                 │  │
│  │  │fdk-aac │  │  │              │  │                 │  │
│  │  │(AAC    │  │  └──────────────┘  └─────────────────┘  │
│  │  │decode) │  │                                         │
│  │  └────────┘  │                                         │
│  └──────────────┘                                         │
│  ┌──────────────┐                                          │
│  │ JniHelpers   │  RAII: JniString, JniByteArray, etc.    │
│  │ (jni_helpers)│  Gerenciamento seguro de memória JNI     │
│  └──────────────┘                                          │
└────────────────────────────────────────────────────────────┘
```

---

## 🔄 Fluxo de Dados / Data Flow

### 1. 🇧🇷 Carregamento de Arquivo / 🇺🇸 File Loading

```
🇧🇷 Usuário toca "Load" / 🇺🇸 User taps "Load"
    │
    ▼
🇧🇷 Kotlin: NativeBridge.loadMp3(handle, "/path/file.mp4")
🇺🇸 Kotlin: NativeBridge.loadMp3(handle, "/path/file.mp4")
    │
    ▼
JNI: Java_com_example_eetgw_jni_NativeBridge_loadMp3()
    │
    ▼
C++: Mp3Engine::loadFile()
    │
    ├── detectFormat() → MP4? MP3?
    │
    ├── MP3: loadMp3()
    │   └── minimp3 → parse metadata
    │
    └── MP4: loadMp4()
        ├── minimp4 → demuxing (separa áudio / separates audio)
        ├── fdk-aac → initialize decoder (ASC)
        └── buildChunkIndex() → streaming index
    │
    ▼
C++: AudioPlayer::initialize(sampleRate, channels)
    │
    ▼
Oboe: opens AudioStream (AAudio preferido/preferred, OpenSL ES fallback)
    │
    ▼
🇧🇷 Kotlin: UI atualiza com metadados / 🇺🇸 Kotlin: UI updates with metadata
```

### 2. 🇧🇷 Reprodução / 🇺🇸 Playback

```
🇧🇷 Usuário toca "Play" / 🇺🇸 User taps "Play"
    │
    ▼
Kotlin: NativeBridge.play(handle)
    │
    ▼
JNI: Java_..._NativeBridge_play()
    │
    ▼
C++: AudioPlayer::play()
    │
    ▼
Oboe: AudioStream::start()
    │
    ▼
Callback: AudioPlayer::onAudioReady()
    │ (🇧🇷 chamado pelo sistema a cada ~10ms / 🇺🇸 called by system every ~10ms)
    ▼
C++: Mp3Engine::decodeNextFrame()
    │
    ├── MP3: minimp3 → PCM
    ├── MP4: minimp4 → AAC raw → fdk-aac → PCM
    │
    ▼
Oboe: PCM → Hardware de áudio → Fone/Speaker
    │
    ▼
🇧🇷 Kotlin: LaunchedEffect atualiza posição a cada 500ms
🇺🇸 Kotlin: LaunchedEffect updates position every 500ms
```

### 3. 🇧🇷 Seek (Slice Horizontal) / 🇺🇸 Seek (Horizontal Slice)

```
🇧🇷 Usuário desliza no slice SEEK / 🇺🇸 User swipes on SEEK slice
    │
    ▼
🇧🇷 Kotlin: detectHorizontalDragGestures → ratio (0.0 a 1.0)
🇺🇸 Kotlin: detectHorizontalDragGestures → ratio (0.0 to 1.0)
    │
    ▼
Kotlin: NativeBridge.seekToRatio(handle, ratio)
    │
    ▼
JNI: Java_..._NativeBridge_seekToRatio()
    │
    ▼
C++: Mp3Engine::seekToRatio(ratio)
    │
    ├── MP3: seek por byte offset proporcional / seek by proportional byte offset
    └── MP4: seek por sample index proporcional / seek by proportional sample index
    │
    ▼
C++: AudioPlayer::reset buffers
    │
    ▼
Oboe: Continua playback do novo ponto / Continues playback from new point
    │
    ▼
🇧🇷 Kotlin: UI atualiza tempo / 🇺🇸 Kotlin: UI updates time
```

---

## 🏗️ Componentes Detalhados / Detailed Components

### Mp3Engine

```cpp
class Mp3Engine {
    // 🇧🇷 Formato detectado automaticamente
    // 🇺🇸 Automatically detected format
    enum AudioFormat { MP3, MP4_AAC, MP4_MP3, UNKNOWN };

    // MP3: carrega tudo em memória (arquivos pequenos)
    // MP3: loads everything in memory (small files)
    std::vector<uint8_t> mp3FileData_;
    std::unique_ptr<mp3dec_t> mp3Decoder_;

    // MP4: streaming por chunks (arquivos grandes)
    // MP4: streaming by chunks (large files)
    struct Mp4Context {
        std::vector<AudioChunk> chunks;      // 🇧🇷 Índice de chunks / 🇺🇸 Chunk index
        std::vector<uint8_t> chunkBuffer;    // 🇧🇷 Buffer de 1 chunk / 🇺🇸 1 chunk buffer
        FILE* fileHandle;                     // 🇧🇷 File handle para streaming
        mp4d_demux_t demux;                   // 🇧🇷 Demuxer minimp4
    };

    // AAC decoder
    std::unique_ptr<AacDecoder> aacDecoder_;

    // Metadados / Metadata
    Mp3Metadata metadata_;
};
```

### AudioPlayer (Oboe)

```cpp
class AudioPlayer : public oboe::AudioStreamDataCallback {
    // 🇧🇷 Stream de áudio de baixa latência
    // 🇺🇸 Low latency audio stream
    std::shared_ptr<oboe::AudioStream> stream_;

    // 🇧🇷 Callback: sistema pede mais áudio
    // 🇺🇸 Callback: system requests more audio
    oboe::DataCallbackResult onAudioReady(
        AudioStream* stream,
        void* audioData,           // 🇧🇷 Buffer a preencher / 🇺🇸 Buffer to fill
        int32_t numFrames          // 🇧🇷 Frames necessários / 🇺🇸 Frames needed
    );

    // 🇧🇷 Buffer PCM temporário
    // 🇺🇸 Temporary PCM buffer
    std::vector<int16_t> pcmBuffer_;
};
```

### MetadataCache (Singleton)

```cpp
class MetadataCache {
    // 🇧🇷 Cache LRU com TTL de 1 hora
    // 🇺🇸 LRU cache with 1 hour TTL
    std::unordered_map<std::string, CachedMetadata> cache_;
    std::mutex mutex_;

    static constexpr auto CACHE_TTL = std::chrono::hours(1);

    // 🇧🇷 Métodos thread-safe
    // 🇺🇸 Thread-safe methods
    bool get(const std::string& path, CachedMetadata& out);
    void put(const std::string& path, const CachedMetadata& meta);
};
```

---

## 🧵 Threads / Threading

```
🇧🇷 Thread Principal (Kotlin/UI) / 🇺🇸 Main Thread (Kotlin/UI)
    ├── LaunchedEffect: atualiza posição a cada 500ms
    │   LaunchedEffect: updates position every 500ms
    └── Gestos: seek, volume, play/pause
        Gestures: seek, volume, play/pause

🇧🇷 Thread de Áudio Oboe (C++ - alta prioridade)
🇺🇸 Oboe Audio Thread (C++ - high priority)
    ├── onAudioReady(): decode + output
    └── 🇧🇷 Latência alvo: < 20ms / 🇺🇸 Target latency: < 20ms

🇧🇷 Background (implícito) / 🇺🇸 Background (implicit)
    ├── Prefetch de próximo chunk (MP4)
    │   Prefetch of next chunk (MP4)
    └── Cache cleanup (MetadataCache)
        Cache cleanup (MetadataCache)
```

---

## 📊 Decisões de Design / Design Decisions

### 🇧🇷 Por que minimp3 + minimp4 + fdk-aac? / 🇺🇸 Why minimp3 + minimp4 + fdk-aac?

| Biblioteca / Library | Alternativa / Alternative | 🇧🇷 Por que escolhemos / 🇺🇸 Why we chose |
|----------------------|--------------------------|------------------------------------------|
| minimp3 | libmpg123 | 🇧🇷 Single-header, 20KB, zero dependências / 🇺🇸 Single-header, 20KB, zero dependencies |
| minimp4 | FFmpeg | 🇧🇷 Single-header, 50KB, só MP4 / 🇺🇸 Single-header, 50KB, MP4 only |
| fdk-aac | FFmpeg (libavcodec) | 🇧🇷 Qualidade Fraunhofer, HE-AAC v2, menor que FFmpeg / 🇺🇸 Fraunhofer quality, HE-AAC v2, smaller than FFmpeg |
| Oboe | OpenSL ES direto / direct | 🇧🇷 Abstração AAudio/OpenSL, Google-maintained / 🇺🇸 AAudio/OpenSL abstraction, Google-maintained |

### 🇧🇷 Por que C++17 e não C++20/23? / 🇺🇸 Why C++17 and not C++20/23?

| Feature | C++17 | C++20 | C++23 |
|---------|-------|-------|-------|
| RAII | ✅ | ✅ | ✅ |
| std::unique_ptr | ✅ | ✅ | ✅ |
| std::optional | ✅ | ✅ | ✅ |
| std::span | ❌ | ✅ | ✅ |
| std::expected | ❌ | ❌ | ✅ |
| 🇧🇷 Suporte NDK / 🇺🇸 NDK Support | ✅ Stable | ⚠️ Parcial / Partial | ❌ Limitado / Limited |

**🇧🇷 Decisão:** C++17 = estável, bem suportado, suficiente para RAII e smart pointers.
**🇺🇸 Decision:** C++17 = stable, well supported, sufficient for RAII and smart pointers.

### 🇧🇷 Por que dois slices (seek + volume)? / 🇺🇸 Why two slices (seek + volume)?

| Aspecto | Seek | Volume |
|---------|------|--------|
| 🇧🇷 Cor / 🇺🇸 Color | Ciano (frio) / Cyan (cold) | Magenta (quente) / Magenta (warm) |
| 🇧🇷 Função / 🇺🇸 Function | Navegação temporal / Time navigation | Intensidade sonora / Sound intensity |
| 🇧🇷 Interação / 🇺🇸 Interaction | Deslizar horizontal / Horizontal swipe | Deslizar horizontal / Horizontal swipe |
| 🇧🇷 Feedback / 🇺🇸 Feedback | Posição na música / Position in song | Porcentagem / Percentage |

**🇧🇷 Princípio:** Cores complementares (180° no círculo cromático) = máximo contraste sem competição visual.
**🇺🇸 Principle:** Complementary colors (180° on color wheel) = maximum contrast without visual competition.

---

## 🔐 Segurança de Memória / Memory Safety

### RAII Helpers JNI

```cpp
// JniString: libera automaticamente / JniString: automatically releases
{
    JniString path(env, jstringPath);  // GetStringUTFChars
    // ... usa path.get() / uses path.get() ...
} // ~JniString() → ReleaseStringUTFChars

// JniByteArray: libera automaticamente / JniByteArray: automatically releases
{
    JniByteArray bytes(env, jbyteArray);  // GetByteArrayElements
    // ... usa bytes.data() / uses bytes.data() ...
} // ~JniByteArray() → ReleaseByteArrayElements
```

### Smart Pointers C++

```cpp
std::unique_ptr<mp3dec_t> mp3Decoder_;      // 🇧🇷 Ownership único / 🇺🇸 Unique ownership
std::unique_ptr<Mp4Context> mp4Context_;    // 🇧🇷 Ownership único / 🇺🇸 Unique ownership
std::unique_ptr<AacDecoder> aacDecoder_;    // 🇧🇷 Ownership único / 🇺🇸 Unique ownership
std::shared_ptr<oboe::AudioStream> stream_; // 🇧🇷 Shared com Oboe / 🇺🇸 Shared with Oboe
```

---

## 📈 Performance / Performance

### 🇧🇷 Métricas Alvo / 🇺🇸 Target Metrics

| Métrica / Metric | 🇧🇷 Alvo / 🇺🇸 Target | 🇧🇷 Como medir / 🇺🇸 How to measure |
|-------------------|----------------------|-----------------------------------|
| 🇧🇷 Latência áudio / 🇺🇸 Audio latency | < 20ms | Oboe::getLatency() |
| 🇧🇷 CPU decode / 🇺🇸 Decode CPU | < 15% | systrace / simpleperf |
| 🇧🇷 Memória total / 🇺🇸 Total memory | < 50MB | Android Studio Profiler |
| 🇧🇷 Bateria (1h) / 🇺🇸 Battery (1h) | < 10% | Battery Historian |
| 🇧🇷 Startup / 🇺🇸 Startup | < 2s | Log timestamps |

### 🇧🇷 Otimizações / 🇺🇸 Optimizations

1. **Streaming**: 🇧🇷 carrega 64KB chunks vs arquivo inteiro / 🇺🇸 loads 64KB chunks vs entire file
2. **HE-AAC v2**: 🇧🇷 48kbps = qualidade 128kbps AAC / 🇺🇸 48kbps = 128kbps AAC quality
3. **Cache**: 🇧🇷 metadados em memória, não reprocessa / 🇺🇸 metadata in memory, doesn't reprocess
4. **Prefetch**: 🇧🇷 próximo chunk carregado em background / 🇺🇸 next chunk loaded in background
5. **Buffer pooling**: 🇧🇷 reutiliza buffers PCM / 🇺🇸 reuses PCM buffers

---

## 🗺️ Roadmap Técnico / Technical Roadmap

### v0.2.0
- [ ] FLAC decoder (libflac)
- [ ] 🇧🇷 Playlist com shuffle / 🇺🇸 Playlist with shuffle
- [ ] 🇧🇷 Equalizador 5-band / 🇺🇸 5-band equalizer

### v0.3.0
- [ ] 🇧🇷 Sincronização com celular (Wear Data Layer) / 🇺🇸 Phone sync (Wear Data Layer)
- [ ] 🇧🇷 Download de música do celular / 🇺🇸 Download music from phone
- [ ] 🇧🇷 Widget para tela inicial / 🇺🇸 Home screen widget

### v0.4.0
- [ ] 🇧🇷 Suporte a Opus (libopus) / 🇺🇸 Opus support (libopus)
- [ ] 🇧🇷 Crossfade entre músicas / 🇺🇸 Crossfade between songs
- [ ] 🇧🇷 ReplayGain (normalização de volume) / 🇺🇸 ReplayGain (volume normalization)

---

🇧🇷 Dúvidas sobre a arquitetura? Abra uma issue com label `documentation`.
🇺🇸 Questions about architecture? Open an issue with label `documentation`.
