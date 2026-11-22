package com.unifiedstreamnext.host

import android.app.Activity
import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.Service
import android.content.Intent
import android.content.pm.ServiceInfo
import android.media.projection.MediaProjectionManager
import android.os.Build
import android.os.IBinder

/**
 * Foreground Service managing Android MediaProjection permission lifecycle and AndroidHostEngine.
 */
class AndroidHostService : Service() {
    private var hostEngine: AndroidHostEngine? = null

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        ensureNotificationChannel()
        val notification = Notification.Builder(this, CHANNEL_ID)
            .setContentTitle("UnifiedStreamNext Host")
            .setContentText("正在提供远程画面共享服务")
            .setSmallIcon(android.R.drawable.stat_sys_data_bluetooth)
            .build()

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            startForeground(NOTIFICATION_ID, notification, ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PROJECTION)
        } else {
            startForeground(NOTIFICATION_ID, notification)
        }

        val resultCode = intent?.getIntExtra(EXTRA_RESULT_CODE, Activity.RESULT_CANCELED) ?: Activity.RESULT_CANCELED
        val data: Intent? = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            intent?.getParcelableExtra(EXTRA_RESULT_DATA, Intent::class.java)
        } else {
            @Suppress("DEPRECATION")
            intent?.getParcelableExtra(EXTRA_RESULT_DATA)
        }

        if (resultCode == Activity.RESULT_OK && data != null) {
            val mpm = getSystemService(MEDIA_PROJECTION_SERVICE) as MediaProjectionManager
            val projection = mpm.getMediaProjection(resultCode, data)
            if (projection != null) {
                val engine = AndroidHostEngine(object : HostSessionListener {
                    override fun onStateChanged(state: HostSessionState, detail: String) {}
                    override fun onEncodedFrame(nalUnit: ByteArray, presentationTimeUs: Long, isKeyFrame: Boolean) {}
                })
                engine.start(projection, HostVideoConfig())
                hostEngine = engine
            }
        }
        return START_NOT_STICKY
    }

    override fun onDestroy() {
        hostEngine?.stop()
        hostEngine = null
        super.onDestroy()
    }

    override fun onBind(intent: Intent?): IBinder? = null

    private fun ensureNotificationChannel() {
        val mgr = getSystemService(NotificationManager::class.java)
        if (mgr.getNotificationChannel(CHANNEL_ID) == null) {
            mgr.createNotificationChannel(
                NotificationChannel(CHANNEL_ID, "UnifiedStreamNext Host", NotificationManager.IMPORTANCE_LOW)
            )
        }
    }

    companion object {
        const val CHANNEL_ID = "usn_android_host"
        const val NOTIFICATION_ID = 4101
        const val EXTRA_RESULT_CODE = "extra_result_code"
        const val EXTRA_RESULT_DATA = "extra_result_data"
    }
}
