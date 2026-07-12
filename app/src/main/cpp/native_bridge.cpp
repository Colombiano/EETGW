#include <jni.h>
#include <string>
#include <memory>
#include <android/log.h>
#include <oboe/Oboe.h>

#include "JniHelpers.h"
#include "DataTypes.h"
#include "Mp3Engine.h"
#include "AudioPlayer.h"
#include "MetadataCache.h"

// =============================================================================
// EETGW — Essa e pra tocar no Galaxy Watch
// native_bridge.cpp — JNI Bridge: Kotlin <-> C++
// =============================================================================

#define LOG_TAG "EETGW_JNI"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

using namespace eetgw;

// =============================================================================
// Estado global / Global state
// =============================================================================

struct GlobalState {
    std::unique_ptr<Mp3Engine> engine;
    std::unique_ptr<AudioPlayer> player;
    
    struct {
        JniGlobalRef object;
        jmethodID onProgress = nullptr;
        jmethodID onStateChanged = nullptr;
        jmethodID onError = nullptr;
        jmethodID onMetadata = nullptr;
    } callback;
    
    JavaVM* javaVM = nullptr;
};

static GlobalState g;

// =============================================================================
// Helpers
// =============================================================================

static JNIEnv* getEnv() {
    JNIEnv* env = nullptr;
    jint ret = g.javaVM->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    if (ret == JNI_EDETACHED) {
        g.javaVM->AttachCurrentThread(&env, nullptr);
    }
    return env;
}

static void sendProgress(int64_t position, int64_t duration) {
    if (g.callback.object.isNull() || !g.callback.onProgress) return;
    JNIEnv* env = getEnv();
    if (!env) return;
    env->CallVoidMethod(g.callback.object.get(), g.callback.onProgress,
                        static_cast<jlong>(position), static_cast<jlong>(duration));
}

static void sendStateChanged(PlaybackState state) {
    if (g.callback.object.isNull() || !g.callback.onStateChanged) return;
    JNIEnv* env = getEnv();
    if (!env) return;
    env->CallVoidMethod(g.callback.object.get(), g.callback.onStateChanged, static_cast<jint>(state));
}

static void sendError(const std::string& message) {
    if (g.callback.object.isNull() || !g.callback.onError) return;
    JNIEnv* env = getEnv();
    if (!env) return;
    jstring jMsg = env->NewStringUTF(message.c_str());
    env->CallVoidMethod(g.callback.object.get(), g.callback.onError, jMsg);
    env->DeleteLocalRef(jMsg);
}

// =============================================================================
// JNI_OnLoad
// =============================================================================

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    g.javaVM = vm;
    LOGI("EETGW JNI loaded");
    return JNI_VERSION_1_6;
}

// =============================================================================
// NativeBridge.kt — Metodos nativos / Native methods
// =============================================================================

extern "C" {

JNIEXPORT jboolean JNICALL
Java_com_eetgw_jni_NativeBridge_nativeInitialize(
        JNIEnv* env, jobject thiz, jint sampleRate, jint channels) {
    
    g.engine = std::make_unique<Mp3Engine>();
    g.player = std::make_unique<AudioPlayer>();
    
    AudioConfig config;
    config.sampleRate = sampleRate;
    config.channels = channels;
    
    if (!g.player->initialize(config)) {
        g.engine.reset();
        g.player.reset();
        return JNI_FALSE;
    }
    
    g.player->setEngine(g.engine.get());
    g.player->setProgressCallback(sendProgress);
    
    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_com_eetgw_jni_NativeBridge_nativeShutdown(JNIEnv* env, jobject thiz) {
    g.player.reset();
    g.engine.reset();
    g.callback.object.reset();
    g.callback.onProgress = nullptr;
    g.callback.onStateChanged = nullptr;
    g.callback.onError = nullptr;
    g.callback.onMetadata = nullptr;
    MetadataCache::instance().clear();
}

JNIEXPORT void JNICALL
Java_com_eetgw_jni_NativeBridge_nativeSetCallback(JNIEnv* env, jobject thiz, jobject callback) {
    JniClass callbackClass(env, "com/eetgw/jni/NativeBridge$NativeCallback");
    if (callbackClass.isNull()) {
        LOGE("NativeCallback interface not found");
        return;
    }
    
    g.callback.object = JniGlobalRef(env, callback);
    
    g.callback.onProgress = env->GetMethodID(callbackClass.get(), "onProgress", "(JJ)V");
    g.callback.onStateChanged = env->GetMethodID(callbackClass.get(), "onStateChanged", "(I)V");
    g.callback.onError = env->GetMethodID(callbackClass.get(), "onError", "(Ljava/lang/String;)V");
    g.callback.onMetadata = env->GetMethodID(callbackClass.get(), "onMetadata", 
                                              "(Lcom/eetgw/jni/NativeBridge$Metadata;)V");
}

JNIEXPORT jboolean JNICALL
Java_com_eetgw_jni_NativeBridge_nativeLoadFile(JNIEnv* env, jobject thiz, jstring path) {
    JniString jPath(env, path);
    if (jPath.isNull()) return JNI_FALSE;
    
    if (!g.engine || !g.player) {
        return JNI_FALSE;
    }
    
    return g.engine->loadFile(jPath.get()) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_eetgw_jni_NativeBridge_nativeUnload(JNIEnv* env, jobject thiz) {
    if (g.engine) {
        g.engine->unload();
    }
}

JNIEXPORT jboolean JNICALL
Java_com_eetgw_jni_NativeBridge_nativePlay(JNIEnv* env, jobject thiz) {
    if (!g.player) return JNI_FALSE;
    bool result = g.player->play();
    if (result) sendStateChanged(PlaybackState::PLAYING);
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_eetgw_jni_NativeBridge_nativePause(JNIEnv* env, jobject thiz) {
    if (!g.player) return JNI_FALSE;
    bool result = g.player->pause();
    if (result) sendStateChanged(PlaybackState::PAUSED);
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_eetgw_jni_NativeBridge_nativeStop(JNIEnv* env, jobject thiz) {
    if (!g.player) return JNI_FALSE;
    bool result = g.player->stop();
    if (result) sendStateChanged(PlaybackState::STOPPED);
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_eetgw_jni_NativeBridge_nativeSeekTo(JNIEnv* env, jobject thiz, jlong positionMs) {
    if (!g.player) return JNI_FALSE;
    return g.player->seekTo(positionMs) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_eetgw_jni_NativeBridge_nativeSeekToRatio(JNIEnv* env, jobject thiz, jfloat ratio) {
    if (!g.engine || !g.player) return JNI_FALSE;
    
    bool wasPlaying = (g.player->getState() == PlaybackState::PLAYING);
    if (wasPlaying) g.player->pause();
    
    bool result = g.engine->seekToRatio(ratio);
    if (result && wasPlaying) g.player->play();
    
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_eetgw_jni_NativeBridge_nativeSetVolume(JNIEnv* env, jobject thiz, jfloat volume) {
    if (g.player) g.player->setVolume(volume);
}

JNIEXPORT jfloat JNICALL
Java_com_eetgw_jni_NativeBridge_nativeGetVolume(JNIEnv* env, jobject thiz) {
    if (!g.player) return 0.0f;
    return g.player->getVolume();
}

JNIEXPORT void JNICALL
Java_com_eetgw_jni_NativeBridge_nativeVolumeUp(JNIEnv* env, jobject thiz) {
    if (g.player) g.player->volumeUp();
}

JNIEXPORT void JNICALL
Java_com_eetgw_jni_NativeBridge_nativeVolumeDown(JNIEnv* env, jobject thiz) {
    if (g.player) g.player->volumeDown();
}

JNIEXPORT void JNICALL
Java_com_eetgw_jni_NativeBridge_nativeSetVolumeBoost(JNIEnv* env, jobject thiz, jboolean enabled) {
    if (g.player) g.player->setVolumeBoost(enabled);
}

JNIEXPORT jint JNICALL
Java_com_eetgw_jni_NativeBridge_nativeGetState(JNIEnv* env, jobject thiz) {
    if (!g.player) return static_cast<jint>(PlaybackState::STOPPED);
    return static_cast<jint>(g.player->getState());
}

JNIEXPORT jlong JNICALL
Java_com_eetgw_jni_NativeBridge_nativeGetPosition(JNIEnv* env, jobject thiz) {
    if (!g.player) return 0;
    return g.player->getCurrentPositionMs();
}

JNIEXPORT jlong JNICALL
Java_com_eetgw_jni_NativeBridge_nativeGetDuration(JNIEnv* env, jobject thiz) {
    if (!g.player) return 0;
    return g.player->getDurationMs();
}

JNIEXPORT jdouble JNICALL
Java_com_eetgw_jni_NativeBridge_nativeGetLatency(JNIEnv* env, jobject thiz) {
    if (!g.player) return 0.0;
    return g.player->getLatencyMs();
}

JNIEXPORT jfloatArray JNICALL
Java_com_eetgw_jni_NativeBridge_nativeGetVisualizerData(JNIEnv* env, jobject thiz, jint points) {
    if (!g.player) return nullptr;
    
    auto data = g.player->getVisualizerData(points);
    
    jfloatArray result = env->NewFloatArray(data.size());
    env->SetFloatArrayRegion(result, 0, data.size(), data.data());
    
    return result;
}

JNIEXPORT void JNICALL
Java_com_eetgw_jni_NativeBridge_nativeClearCache(JNIEnv* env, jobject thiz) {
    MetadataCache::instance().clear();
}

JNIEXPORT jlong JNICALL
Java_com_eetgw_jni_NativeBridge_nativeGetCacheSize(JNIEnv* env, jobject thiz) {
    return static_cast<jlong>(MetadataCache::instance().size());
}

JNIEXPORT jstring JNICALL
Java_com_eetgw_jni_NativeBridge_nativeGetVersion(JNIEnv* env, jobject thiz) {
    std::string version = "EETGW v1.0.0 (C++17/Oboe/minimp3/minimp4/fdk-aac)";
    return env->NewStringUTF(version.c_str());
}

JNIEXPORT jstring JNICALL
Java_com_eetgw_jni_NativeBridge_nativeGetOboeVersion(JNIEnv* env, jobject thiz) {
    return env->NewStringUTF(oboe::Version::Text);
}

JNIEXPORT void JNICALL
Java_com_eetgw_jni_NativeBridge_nativeOnSwipeLeft(JNIEnv* env, jobject thiz) {
    LOGD("Swipe left -> Next");
}

JNIEXPORT void JNICALL
Java_com_eetgw_jni_NativeBridge_nativeOnSwipeRight(JNIEnv* env, jobject thiz) {
    LOGD("Swipe right -> Previous");
}

JNIEXPORT void JNICALL
Java_com_eetgw_jni_NativeBridge_nativeOnSwipeUp(JNIEnv* env, jobject thiz) {
    if (g.player) {
        g.player->volumeUp(0.1f);
    }
}

JNIEXPORT void JNICALL
Java_com_eetgw_jni_NativeBridge_nativeOnSwipeDown(JNIEnv* env, jobject thiz) {
    if (g.player) {
        g.player->volumeDown(0.1f);
    }
}

JNIEXPORT void JNICALL
Java_com_eetgw_jni_NativeBridge_nativeOnDoubleTap(JNIEnv* env, jobject thiz) {
    if (!g.player) return;
    if (g.player->getState() == PlaybackState::PLAYING) {
        g.player->pause();
        sendStateChanged(PlaybackState::PAUSED);
    } else {
        g.player->play();
        sendStateChanged(PlaybackState::PLAYING);
    }
}

JNIEXPORT void JNICALL
Java_com_eetgw_jni_NativeBridge_nativeOnLongPress(JNIEnv* env, jobject thiz) {
    if (g.player) {
        g.player->stop();
        sendStateChanged(PlaybackState::STOPPED);
    }
}

} // extern "C"
