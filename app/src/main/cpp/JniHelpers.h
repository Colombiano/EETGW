#pragma once

#include <jni.h>
#include <string>
#include <vector>
#include <cstring>

// =============================================================================
// EETGW — Essa e pra tocar no Galaxy Watch
// JniHelpers.h — RAII helpers para JNI / RAII helpers for JNI
// =============================================================================

namespace eetgw {

// ————————————————————————————————————————————————————————
// JniString — RAII para jstring UTF-8 / RAII for jstring UTF-8
// ————————————————————————————————————————————————————————
class JniString {
public:
    JniString(JNIEnv* env, jstring str)
        : env_(env), str_(str), cstr_(nullptr), released_(false) {
        if (str_ != nullptr) {
            cstr_ = env_->GetStringUTFChars(str_, nullptr);
        }
    }

    ~JniString() {
        release();
    }

    JniString(const JniString&) = delete;
    JniString& operator=(const JniString&) = delete;

    JniString(JniString&& other) noexcept
        : env_(other.env_), str_(other.str_), cstr_(other.cstr_), released_(other.released_) {
        other.released_ = true;
        other.cstr_ = nullptr;
    }

    const char* get() const { return cstr_; }
    bool isNull() const { return cstr_ == nullptr; }
    
    std::string toString() const {
        return cstr_ ? std::string(cstr_) : std::string();
    }

    void release() {
        if (!released_ && str_ != nullptr && cstr_ != nullptr) {
            env_->ReleaseStringUTFChars(str_, cstr_);
            released_ = true;
            cstr_ = nullptr;
        }
    }

private:
    JNIEnv* env_;
    jstring str_;
    const char* cstr_;
    bool released_;
};

// ————————————————————————————————————————————————————————
// JniByteArray — RAII para jbyteArray / RAII for jbyteArray
// ————————————————————————————————————————————————————————
class JniByteArray {
public:
    JniByteArray(JNIEnv* env, jbyteArray arr)
        : env_(env), arr_(arr), data_(nullptr), size_(0), released_(false) {
        if (arr_ != nullptr) {
            data_ = env_->GetByteArrayElements(arr_, nullptr);
            size_ = env_->GetArrayLength(arr_);
        }
    }

    ~JniByteArray() {
        release();
    }

    JniByteArray(const JniByteArray&) = delete;
    JniByteArray& operator=(const JniByteArray&) = delete;

    JniByteArray(JniByteArray&& other) noexcept
        : env_(other.env_), arr_(other.arr_), data_(other.data_), 
          size_(other.size_), released_(other.released_) {
        other.released_ = true;
        other.data_ = nullptr;
    }

    uint8_t* data() { return reinterpret_cast<uint8_t*>(data_); }
    const uint8_t* data() const { return reinterpret_cast<const uint8_t*>(data_); }
    jsize size() const { return size_; }
    bool isNull() const { return data_ == nullptr; }

    void release() {
        if (!released_ && arr_ != nullptr && data_ != nullptr) {
            env_->ReleaseByteArrayElements(arr_, data_, JNI_ABORT);
            released_ = true;
            data_ = nullptr;
        }
    }

private:
    JNIEnv* env_;
    jbyteArray arr_;
    jbyte* data_;
    jsize size_;
    bool released_;
};

// ————————————————————————————————————————————————————————
// JniGlobalRef — RAII para jobject global / RAII for global jobject
// ————————————————————————————————————————————————————————
class JniGlobalRef {
public:
    JniGlobalRef() : env_(nullptr), ref_(nullptr) {}
    
    JniGlobalRef(JNIEnv* env, jobject obj)
        : env_(env), ref_(obj ? env_->NewGlobalRef(obj) : nullptr) {}

    ~JniGlobalRef() {
        reset();
    }

    JniGlobalRef(const JniGlobalRef&) = delete;
    JniGlobalRef& operator=(const JniGlobalRef&) = delete;

    JniGlobalRef(JniGlobalRef&& other) noexcept
        : env_(other.env_), ref_(other.ref_) {
        other.ref_ = nullptr;
    }

    JniGlobalRef& operator=(JniGlobalRef&& other) noexcept {
        if (this != &other) {
            reset();
            env_ = other.env_;
            ref_ = other.ref_;
            other.ref_ = nullptr;
        }
        return *this;
    }

    jobject get() const { return ref_; }
    bool isNull() const { return ref_ == nullptr; }

    void reset() {
        if (ref_ != nullptr && env_ != nullptr) {
            env_->DeleteGlobalRef(ref_);
            ref_ = nullptr;
        }
    }

private:
    JNIEnv* env_;
    jobject ref_;
};

} // namespace eetgw
