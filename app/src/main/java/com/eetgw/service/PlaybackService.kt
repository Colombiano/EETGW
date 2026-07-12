package com.eetgw.service

import android.app.*
import android.content.Intent
import android.os.Binder
import android.os.IBinder
import android.util.Log
import androidx.core.app.NotificationCompat
import com.eetgw.MainActivity
import com.eetgw.R
import com.eetgw.jni.NativeBridge

// =============================================================================
// EETGW — Essa e pra tocar no Galaxy Watch
// PlaybackService.kt — Foreground Service para reproducao persistente
// =============================================================================

class PlaybackService : Service() {

    companion object {
        private const val TAG = "EETGW_Service"
        private const val NOTIFICATION_ID = 1
        private const val CHANNEL_ID = "eetgw_playback"
    }

    private val binder = PlaybackBinder()
    private lateinit var bridge: NativeBridge

    inner class PlaybackBinder : Binder() {
        fun getService(): PlaybackService = this@PlaybackService
    }

    override fun onBind(intent: Intent): IBinder = binder

    override fun onCreate() {
        super.onCreate()
        Log.i(TAG, "PlaybackService created")
        bridge = NativeBridge.getInstance()
        createNotificationChannel()
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        Log.d(TAG, "onStartCommand: ${intent?.action}")
        
        when (intent?.action) {
            ACTION_PLAY -> bridge.play()
            ACTION_PAUSE -> bridge.pause()
            ACTION_STOP -> {
                bridge.stop()
                stopForeground(STOP_FOREGROUND_REMOVE)
                stopSelf()
            }
            ACTION_PREVIOUS -> bridge.onSwipeRight()
            ACTION_NEXT -> bridge.onSwipeLeft()
        }
        
        startForeground(NOTIFICATION_ID, buildNotification())
        return START_STICKY
    }

    override fun onDestroy() {
        Log.i(TAG, "PlaybackService destroyed")
        bridge.stop()
        super.onDestroy()
    }

    private fun createNotificationChannel() {
        val channel = NotificationChannel(
            CHANNEL_ID,
            "EETGW Playback",
            NotificationManager.IMPORTANCE_LOW
        ).apply {
            description = "Controles de reproducao EETGW / EETGW playback controls"
            setShowBadge(false)
        }
        
        val manager = getSystemService(NotificationManager::class.java)
        manager.createNotificationChannel(channel)
    }

    private fun buildNotification(): Notification {
        val contentIntent = Intent(this, MainActivity::class.java).apply {
            flags = Intent.FLAG_ACTIVITY_SINGLE_TOP
        }
        val pendingContent = PendingIntent.getActivity(
            this, 0, contentIntent, PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
        )

        val playIntent = PendingIntent.getService(this, 1,
            Intent(this, PlaybackService::class.java).setAction(ACTION_PLAY),
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE)

        val pauseIntent = PendingIntent.getService(this, 2,
            Intent(this, PlaybackService::class.java).setAction(ACTION_PAUSE),
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE)

        val stopIntent = PendingIntent.getService(this, 3,
            Intent(this, PlaybackService::class.java).setAction(ACTION_STOP),
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE)

        val meta = bridge.metadata.value
        val isPlaying = bridge.playbackState.value == NativeBridge.PlaybackState.PLAYING

        return NotificationCompat.Builder(this, CHANNEL_ID)
            .setContentTitle(meta.titleDisplay)
            .setContentText(meta.artist)
            .setSmallIcon(R.drawable.ic_music_note)
            .setContentIntent(pendingContent)
            .setOngoing(true)
            .setOnlyAlertOnce(true)
            .addAction(R.drawable.ic_skip_prev, "Previous", 
                PendingIntent.getService(this, 4,
                    Intent(this, PlaybackService::class.java).setAction(ACTION_PREVIOUS),
                    PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE))
            .addAction(
                if (isPlaying) R.drawable.ic_pause else R.drawable.ic_play,
                if (isPlaying) "Pause" else "Play",
                if (isPlaying) pauseIntent else playIntent
            )
            .addAction(R.drawable.ic_stop, "Stop", stopIntent)
            .addAction(R.drawable.ic_skip_next, "Next",
                PendingIntent.getService(this, 5,
                    Intent(this, PlaybackService::class.java).setAction(ACTION_NEXT),
                    PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE))
            .setStyle(androidx.media.app.NotificationCompat.MediaStyle()
                .setShowActionsInCompactView(1, 2))
            .build()
    }

    fun updateNotification() {
        val manager = getSystemService(NotificationManager::class.java)
        manager.notify(NOTIFICATION_ID, buildNotification())
    }

    companion object {
        const val ACTION_PLAY = "com.eetgw.action.PLAY"
        const val ACTION_PAUSE = "com.eetgw.action.PAUSE"
        const val ACTION_STOP = "com.eetgw.action.STOP"
        const val ACTION_PREVIOUS = "com.eetgw.action.PREVIOUS"
        const val ACTION_NEXT = "com.eetgw.action.NEXT"
    }
}
