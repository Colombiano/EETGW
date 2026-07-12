package com.eetgw

import android.app.Application
import android.util.Log

// =============================================================================
// EETGW — Essa e pra tocar no Galaxy Watch
// EETGWApplication.kt — Application class
// =============================================================================

class EETGWApplication : Application() {

    companion object {
        private const val TAG = "EETGW_App"
    }

    override fun onCreate() {
        super.onCreate()
        Log.i(TAG, "EETGW Application started")
    }
}
