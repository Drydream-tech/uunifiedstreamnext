package com.unifiedstreamnext.host

import android.media.MediaCodec
import android.media.MediaFormat
import android.media.projection.MediaProjection

/**
 * Independent Android Host Engine.
 * Sunshine cannot run natively on Android; this module owns Android screen capture
 * (MediaProjection), hardware video encoding (MediaCodec), audio capture, and
 * Accessibility/Input injection behind the unified session lifecycle.
 */
data class HostVideoConfig(
    val width: Int = 1280,
    val height: Int = 720,
    val fps: Int = 60,
    val bitrateBps: Int = 8_000_000,
    val mimeType: String = MediaFormat.MIMETYPE_VIDEO_AVC
)

enum class HostSessionState {
    IDLE,
    AWAITING_PERMISSION,
    ENCODING_ACTIVE,
    STOPPING,
    FAILED
}

interface HostSessionListener {
    fun onStateChanged(state: HostSessionState, detail: String = "")
    fun onEncodedFrame(nalUnit: ByteArray, presentationTimeUs: Long, isKeyFrame: Boolean)
}

class AndroidHostEngine(
    private val listener: HostSessionListener
) {
    private var codec: MediaCodec? = null
    private var projection: MediaProjection? = null
    @Volatile
    var state: HostSessionState = HostSessionState.IDLE
        private set

    fun start(mediaProjection: MediaProjection, config: HostVideoConfig) {
        projection = mediaProjection
        val format = MediaFormat.createVideoFormat(config.mimeType, config.width, config.height).apply {
            setInteger(MediaFormat.KEY_BIT_RATE, config.bitrateBps)
            setInteger(MediaFormat.KEY_FRAME_RATE, config.fps)
            setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, 2)
        }
        val encoder = MediaCodec.createEncoderByType(config.mimeType)
        encoder.configure(format, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE)
        codec = encoder
        state = HostSessionState.ENCODING_ACTIVE
        listener.onStateChanged(state, "MediaCodec configured for ${config.width}x${config.height}@${config.fps}")
    }

    fun stop() {
        state = HostSessionState.STOPPING
        codec?.runCatching { stop(); release() }
        codec = null
        projection?.stop()
        projection = null
        state = HostSessionState.IDLE
        listener.onStateChanged(state, "AndroidHostEngine stopped")
    }
}
