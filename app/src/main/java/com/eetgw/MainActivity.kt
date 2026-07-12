package com.eetgw

import android.Manifest
import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.content.ServiceConnection
import android.content.pm.PackageManager
import android.os.Bundle
import android.os.IBinder
import android.util.Log
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.runtime.*
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import androidx.wear.compose.navigation.SwipeDismissableNavHost
import androidx.wear.compose.navigation.composable
import androidx.wear.compose.navigation.rememberSwipeDismissableNavController
import com.eetgw.jni.NativeBridge
import com.eetgw.service.PlaybackService
import com.eetgw.ui.PlayerScreen
import com.eetgw.ui.RunningModeScreen
import com.eetgw.ui.theme.EETGWTheme

// =============================================================================
// EETGW — Essa e pra tocar no Galaxy Watch
// MainActivity.kt — Activity principal / Main activity
// =============================================================================

class MainActivity : ComponentActivity() {

    companion object {
        private const val TAG = "EETGW_Main"
        private const val REQUEST_PERMISSIONS = 100
        private val REQUIRED_PERMISSIONS = arrayOf(
            Manifest.permission.READ_EXTERNAL_STORAGE,
            Manifest.permission.READ_MEDIA_AUDIO,
            Manifest.permission.FOREGROUND_SERVICE
        )
    }

    private lateinit var bridge: NativeBridge
    private var playbackService: PlaybackService? = null
    private var serviceBound = false

    private val serviceConnection = object : ServiceConnection {
        override fun onServiceConnected(name: ComponentName?, service: IBinder?) {
            val binder = service as PlaybackService.PlaybackBinder
            playbackService = binder.getService()
            serviceBound = true
            Log.d(TAG, "PlaybackService connected")
        }

        override fun onServiceDisconnected(name: ComponentName?) {
            playbackService = null
            serviceBound = false
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        NativeBridge.loadLibrary()
        bridge = NativeBridge.getInstance()

        checkPermissions()
        bridge.initialize()
        loadDefaultMusic()
        startPlaybackService()

        setContent {
            EETGWTheme {
                val navController = rememberSwipeDismissableNavController()
                var isRunningMode by remember { mutableStateOf(false) }

                SwipeDismissableNavHost(
                    navController = navController,
                    startDestination = "player"
                ) {
                    composable("player") {
                        PlayerScreen(
                            bridge = bridge,
                            onRunningModeClick = { isRunningMode = true }
                        )
                    }
                }

                if (isRunningMode) {
                    RunningModeScreen(
                        bridge = bridge,
                        onBack = { isRunningMode = false }
                    )
                }
            }
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        bridge.shutdown()
        if (serviceBound) {
            unbindService(serviceConnection)
        }
    }

    private fun checkPermissions() {
        val missingPermissions = REQUIRED_PERMISSIONS.filter {
            ContextCompat.checkSelfPermission(this, it) != PackageManager.PERMISSION_GRANTED
        }

        if (missingPermissions.isNotEmpty()) {
            ActivityCompat.requestPermissions(
                this,
                missingPermissions.toTypedArray(),
                REQUEST_PERMISSIONS
            )
        }
    }

    private fun loadDefaultMusic() {
        val musicDir = getExternalFilesDir(android.os.Environment.DIRECTORY_MUSIC)
        if (musicDir != null && musicDir.exists()) {
            val files = musicDir.listFiles { file ->
                file.extension.lowercase() in NativeBridge.SUPPORTED_EXTENSIONS
            }
            if (!files.isNullOrEmpty()) {
                bridge.loadFile(files[0])
                Log.i(TAG, "Loaded default music: ${files[0].name}")
                return
            }
        }

        val sdcardMusic = java.io.File("/sdcard/Music")
        if (sdcardMusic.exists()) {
            val files = sdcardMusic.listFiles { file ->
                file.extension.lowercase() in NativeBridge.SUPPORTED_EXTENSIONS
            }
            if (!files.isNullOrEmpty()) {
                bridge.loadFile(files[0])
                Log.i(TAG, "Loaded music from sdcard: ${files[0].name}")
            }
        }
    }

    private fun startPlaybackService() {
        val intent = Intent(this, PlaybackService::class.java)
        startForegroundService(intent)
        bindService(intent, serviceConnection, Context.BIND_AUTO_CREATE)
    }
}
