package com.eetgw.jni

import android.util.Log
import androidx.compose.runtime.mutableStateOf
import java.io.File

// =============================================================================
// EETGW — Essa e pra tocar no Galaxy Watch
// NativeBridge.kt — JNI Bridge: Kotlin <-> C++
// =============================================================================

class NativeBridge private constructor() {

    companion object {
        private const val TAG = "EETGW_Bridge"
        val SUPPORTED_EXTENSIONS = setOf("mp3", "mp4", "m4a", "aac")

        @JvmStatic
        fun loadLibrary() {
            try {
                System.loadLibrary("eetgw")
                Log.i(TAG, "Native library loaded: ${nativeGetVersion()}")
            } catch (e: UnsatisfiedLinkError) {
                Log.e(TAG, "Failed to load native library", e)
                throw RuntimeException("Cannot load EETGW native library", e)
            }
        }

        @Volatile
        private var instance: NativeBridge? = null

        fun getInstance(): NativeBridge {
            return instance ?: synchronized(this) {
                instance ?: NativeBridge().also { instance = it }
            }
        }

        @JvmStatic
        external fun nativeGetVersion(): String

        @JvmStatic
        external fun nativeGetOboeVersion(): String
    }

    // ————————————————————————————————————————————————————————
    // Estado reativo / Reactive state (para Compose)
    // ————————————————————————————————————————————————————————
    val playbackState = mutableStateOf(PlaybackState.STOPPED)
    val currentPositionMs = mutableStateOf(0L)
    val durationMs = mutableStateOf(0L)
    val currentVolume = mutableStateOf(1.0f)
    val metadata = mutableStateOf(Metadata())
    val latencyMs = mutableStateOf(0.0)
    val errorMessage = mutableStateOf<String?>(null)

    val progress: Float
        get() = if (durationMs.value > 0) {
            (currentPositionMs.value.toFloat() / durationMs.value.toFloat()).coerceIn(0f, 1f)
        } else 0f

    // ————————————————————————————————————————————————————————
    // Callback nativo / Native callback
    // ————————————————————————————————————————————————————————
    private val nativeCallback = object : NativeCallback {
        override fun onProgress(position: Long, duration: Long) {
            currentPositionMs.value = position
            durationMs.value = duration
        }

        override fun onStateChanged(state: Int) {
            playbackState.value = PlaybackState.fromInt(state)
        }

        override fun onError(message: String) {
            Log.e(TAG, "Native error: $message")
            errorMessage.value = message
        }

        override fun onMetadata(meta: Metadata) {
            metadata.value = meta
            durationMs.value = meta.durationMs
        }
    }

    // ————————————————————————————————————————————————————————
    // Ciclo de vida / Lifecycle
    // ————————————————————————————————————————————————————————
    fun initialize(sampleRate: Int = 44100, channels: Int = 2): Boolean {
        val result = nativeInitialize(sampleRate, channels)
        if (result) {
            nativeSetCallback(nativeCallback)
        }
        return result
    }

    fun shutdown() {
        nativeShutdown()
    }

    // ————————————————————————————————————————————————————————
    // Arquivo / File
    // ————————————————————————————————————————————————————————
    fun loadFile(file: File): Boolean {
        errorMessage.value = null
        return nativeLoadFile(file.absolutePath)
    }

    fun loadFile(path: String): Boolean {
        errorMessage.value = null
        return nativeLoadFile(path)
    }

    fun unload() {
        nativeUnload()
        metadata.value = Metadata()
        currentPositionMs.value = 0
        durationMs.value = 0
    }

    // ————————————————————————————————————————————————————————
    // Playback / Playback
    // ————————————————————————————————————————————————————————
    fun play(): Boolean {
        errorMessage.value = null
        return nativePlay()
    }

    fun pause(): Boolean {
        return nativePause()
    }

    fun stop(): Boolean {
        return nativeStop()
    }

    fun seekTo(positionMs: Long): Boolean {
        return nativeSeekTo(positionMs)
    }

    fun seekToRatio(ratio: Float): Boolean {
        return nativeSeekToRatio(ratio.coerceIn(0f, 1f))
    }

    // ————————————————————————————————————————————————————————
    // Volume / Volume
    // ————————————————————————————————————————————————————————
    fun setVolume(volume: Float) {
        val v = volume.coerceIn(0f, 1f)
        nativeSetVolume(v)
        currentVolume.value = v
    }

    fun volumeUp() {
        nativeVolumeUp()
        currentVolume.value = nativeGetVolume()
    }

    fun volumeDown() {
        nativeVolumeDown()
        currentVolume.value = nativeGetVolume()
    }

    fun setVolumeBoost(enabled: Boolean) {
        nativeSetVolumeBoost(enabled)
    }

    // ————————————————————————————————————————————————————————
    // Gestos / Gestures
    // ————————————————————————————————————————————————————————
    fun onSwipeLeft() = nativeOnSwipeLeft()
    fun onSwipeRight() = nativeOnSwipeRight()
    fun onSwipeUp() = nativeOnSwipeUp()
    fun onSwipeDown() = nativeOnSwipeDown()
    fun onDoubleTap() = nativeOnDoubleTap()
    fun onLongPress() = nativeOnLongPress()

    // ————————————————————————————————————————————————————————
    // Info / Info
    // ————————————————————————————————————————————————————————
    fun getLatency(): Double {
        val lat = nativeGetLatency()
        latencyMs.value = lat
        return lat
    }

    fun getVisualizerData(points: Int): FloatArray {
        return nativeGetVisualizerData(points) ?: FloatArray(points) { 0f }
    }

    fun clearCache() = nativeClearCache()

    // ————————————————————————————————————————————————————————
    // Formatos suportados / Supported formats
    // ————————————————————————————————————————————————————————
    fun isSupportedFile(file: File): Boolean {
        val ext = file.extension.lowercase()
        return ext in SUPPORTED_EXTENSIONS
    }

    // =============================================================================
    // Dados / Data
    // =============================================================================
    data class Metadata(
        val path: String = "",
        val title: String = "Desconhecido",
        val artist: String = "Artista Desconhecido",
        val album: String = "",
        val genre: String = "",
        val durationMs: Long = 0,
        val bitrate: Long = 0,
        val sampleRate: Int = 44100,
        val channels: Int = 2
    ) {
        val durationText: String
            get() = formatTime(durationMs)

        val titleDisplay: String
            get() = title.ifBlank { File(path).nameWithoutExtension }

        private fun formatTime(ms: Long): String {
            val totalSeconds = ms / 1000
            val minutes = totalSeconds / 60
            val seconds = totalSeconds % 60
            return "%d:%02d".format(minutes, seconds)
        }
    }

    enum class PlaybackState {
        STOPPED, PLAYING, PAUSED, BUFFERING, ERROR;

        companion object {
            fun fromInt(value: Int) = entries.getOrElse(value) { STOPPED }
        }
    }

    interface NativeCallback {
        fun onProgress(position: Long, duration: Long)
        fun onStateChanged(state: Int)
        fun onError(message: String)
        fun onMetadata(meta: Metadata)
    }

    // =============================================================================
    // Metodos nativos / Native methods
    // =============================================================================
    private external fun nativeInitialize(sampleRate: Int, channels: Int): Boolean
    private external fun nativeShutdown()
    private external fun nativeSetCallback(callback: NativeCallback)

    private external fun nativeLoadFile(path: String): Boolean
    private external fun nativeUnload()

    private external fun nativePlay(): Boolean
    private external fun nativePause(): Boolean
    private external fun nativeStop(): Boolean
    private external fun nativeSeekTo(positionMs: Long): Boolean
    private external fun nativeSeekToRatio(ratio: Float): Boolean

    private external fun nativeSetVolume(volume: Float)
    private external fun nativeGetVolume(): Float
    private external fun nativeVolumeUp()
    private external fun nativeVolumeDown()
    private external fun nativeSetVolumeBoost(enabled: Boolean)

    private external fun nativeGetState(): Int
    private external fun nativeGetPosition(): Long
    private external fun nativeGetDuration(): Long
    private external fun nativeGetLatency(): Double

    private external fun nativeGetVisualizerData(points: Int): FloatArray?

    private external fun nativeClearCache()
    private external fun nativeGetCacheSize(): Long

    private external fun nativeOnSwipeLeft()
    private external fun nativeOnSwipeRight()
    private external fun nativeOnSwipeUp()
    private external fun nativeOnSwipeDown()
    private external fun nativeOnDoubleTap()
    private external fun nativeOnLongPress()
}
