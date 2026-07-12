#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <chrono>

// =============================================================================
// EETGW — Essa e pra tocar no Galaxy Watch
// DataTypes.h — Tipos de dados compartilhados / Shared data types
// =============================================================================

namespace eetgw {

// Forward declaration
struct Mp3Metadata;

// ———————————————————————————————————————————————————————————————
// Configuracao de Audio / Audio Configuration
// ———————————————————————————————————————————————————————————————
struct AudioConfig {
    int32_t sampleRate = 44100;       // Hz
    int32_t channels = 2;             // 1 = mono, 2 = stereo
    int32_t bitsPerSample = 16;       // 16 ou 32
    int32_t bufferSize = 2048;        // Frames por callback / Frames per callback
    double targetLatencyMs = 20.0;    // Latencia alvo / Target latency
};

// ———————————————————————————————————————————————————————————————
// Estado de Reproducao / Playback State
// —————————————————————————————————————————————————————————————————
enum class PlaybackState {
    STOPPED,
    PLAYING,
    PAUSED,
    BUFFERING,
    ERROR
};

// ———————————————————————————————————————————————————————————————
// Formato de Audio Detectado / Detected Audio Format
// ———————————————————————————————————————————————————————————————
enum class AudioFormat {
    MP3,
    MP4_AAC,
    MP4_MP3,
    UNKNOWN
};

// ———————————————————————————————————————————————————————————————
// Metadados do Arquivo / File Metadata
// ———————————————————————————————————————————————————————————————
struct Mp3Metadata {
    std::string path;                 // Caminho completo / Full path
    std::string title;                // Titulo / Title
    std::string artist;               // Artista / Artist
    std::string album;                // Album / Album
    std::string genre;                // Genero / Genre
    int64_t durationMs = 0;           // Duracao em ms / Duration in ms
    int64_t bitrate = 0;              // Bitrate bps / Bitrate in bps
    int32_t sampleRate = 44100;       // Sample rate Hz
    int32_t channels = 2;             // Numero de canais / Channel count
    AudioFormat format = AudioFormat::UNKNOWN;  // Formato detectado / Detected format
    bool hasVideo = false;            // Tem stream de video? / Has video stream?
    std::chrono::steady_clock::time_point cachedAt;  // Quando foi cacheado / When cached
};

// ———————————————————————————————————————————————————————————————
// Chunk para Streaming MP4 / MP4 Streaming Chunk
// ———————————————————————————————————————————————————————————————
struct AudioChunk {
    int64_t offset;                   // Offset no arquivo / File offset
    int64_t size;                     // Tamanho do chunk / Chunk size
    int64_t sampleIndex;              // Indice do primeiro sample / First sample index
    int64_t durationMs;               // Duracao em ms / Duration in ms
};

// ———————————————————————————————————————————————————————————————
// Contexto MP4 para Streaming / MP4 Streaming Context
// ———————————————————————————————————————————————————————————————
struct Mp4Context {
    std::vector<AudioChunk> chunks;           // Indice de chunks / Chunk index
    std::vector<uint8_t> chunkBuffer;         // Buffer de 1 chunk (64KB) / 1 chunk buffer
    FILE* fileHandle = nullptr;               // FILE* para streaming / FILE* for streaming
    // minimp4 state (opaque pointer to avoid including minimp4.h everywhere)
    void* demuxState = nullptr;               // Estado do demuxer / Demuxer state
    
    ~Mp4Context() {
        if (fileHandle) {
            fclose(fileHandle);
            fileHandle = nullptr;
        }
        // IMPORTANTE: O demuxState deve ser liberado pelo Mp4Engine
        // usando MP4D_close() antes do reset do unique_ptr.
        // Se ainda existir aqui, eh um safety net.
        if (demuxState) {
            // Nao podemos chamar MP4D_close aqui pois nao temos acesso ao minimp4.h
            // O Mp4Engine deve chamar MP4D_close antes de destruir o contexto.
            // Este ponteiro sera nullptr se Mp4Engine::unload() for chamado corretamente.
            demuxState = nullptr;
        }
    }
};

// ———————————————————————————————————————————————————————————————
// Callbacks para Kotlin / Callbacks to Kotlin
// ———————————————————————————————————————————————————————————————
class PlayerCallbacks {
public:
    virtual ~PlayerCallbacks() = default;
    
    // Progresso: posicao atual, duracao total / Progress: current position, total duration
    virtual void onProgress(int64_t positionMs, int64_t durationMs) = 0;
    
    // Estado mudou / State changed
    virtual void onStateChanged(PlaybackState state) = 0;
    
    // Erro ocorreu / Error occurred
    virtual void onError(const std::string& message) = 0;
    
    // Metadados carregados / Metadata loaded
    virtual void onMetadataLoaded(const Mp3Metadata& metadata) = 0;
};

} // namespace eetgw