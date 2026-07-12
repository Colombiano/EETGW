package com.eetgw.ui

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.gestures.detectHorizontalDragGestures
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.wear.compose.material.*
import com.eetgw.jni.NativeBridge
import com.eetgw.ui.theme.EETGWTheme
import com.eetgw.ui.theme.TerminalColors
import kotlinx.coroutines.delay

// =============================================================================
// EETGW — Essa e pra tocar no Galaxy Watch
// PlayerScreen.kt — Tela principal do player / Main player screen
// =============================================================================

@Composable
fun PlayerScreen(
    bridge: NativeBridge,
    onRunningModeClick: () -> Unit
) {
    val state by bridge.playbackState
    val position by bridge.currentPositionMs
    val duration by bridge.durationMs
    val volume by bridge.currentVolume
    val meta by bridge.metadata
    val progress = if (duration > 0) position.toFloat() / duration.toFloat() else 0f

    LaunchedEffect(state) {
        while (state == NativeBridge.PlaybackState.PLAYING) {
            delay(500)
        }
    }

    EETGWTheme {
        Scaffold(
            modifier = Modifier.fillMaxSize(),
            vignette = { Vignette(vignettePosition = VignettePosition.TopAndBottom) }
        ) {
            Box(
                modifier = Modifier
                    .fillMaxSize()
                    .background(TerminalColors.Background),
                contentAlignment = Alignment.Center
            ) {
                ProgressRing(
                    progress = progress,
                    volume = volume,
                    modifier = Modifier.size(180.dp)
                )

                Column(
                    horizontalAlignment = Alignment.CenterHorizontally,
                    verticalArrangement = Arrangement.Center,
                    modifier = Modifier.padding(16.dp)
                ) {
                    Text(
                        text = formatTime(position),
                        style = MaterialTheme.typography.display3,
                        color = TerminalColors.Cyan,
                        fontSize = 24.sp
                    )

                    Spacer(modifier = Modifier.height(2.dp))

                    Text(
                        text = "${formatTime(position)} / ${formatTime(duration)}",
                        style = MaterialTheme.typography.caption1,
                        color = TerminalColors.TextSecondary,
                        fontSize = 10.sp
                    )

                    Spacer(modifier = Modifier.height(4.dp))

                    Text(
                        text = meta.titleDisplay,
                        style = MaterialTheme.typography.title3,
                        color = TerminalColors.TextPrimary,
                        maxLines = 1,
                        overflow = TextOverflow.Ellipsis,
                        textAlign = TextAlign.Center,
                        fontSize = 11.sp
                    )

                    Text(
                        text = meta.artist,
                        style = MaterialTheme.typography.caption1,
                        color = TerminalColors.TextSecondary,
                        maxLines = 1,
                        overflow = TextOverflow.Ellipsis,
                        textAlign = TextAlign.Center,
                        fontSize = 9.sp
                    )
                }

                Box(
                    modifier = Modifier
                        .align(Alignment.BottomCenter)
                        .padding(bottom = 8.dp)
                ) {
                    PlayerControls(
                        state = state,
                        onPlay = { bridge.play() },
                        onPause = { bridge.pause() },
                        onStop = { bridge.stop() },
                        onSeekBackward = {
                            bridge.seekTo((position - 10000).coerceAtLeast(0))
                        },
                        onSeekForward = {
                            bridge.seekTo((position + 10000).coerceAtMost(duration))
                        },
                        onRunningMode = onRunningModeClick
                    )
                }

                StateIndicator(
                    state = state,
                    modifier = Modifier.align(Alignment.TopCenter)
                )
            }
        }
    }
}

@Composable
fun ProgressRing(
    progress: Float,
    volume: Float,
    modifier: Modifier = Modifier
) {
    Canvas(
        modifier = modifier
            .pointerInput(Unit) {
                detectHorizontalDragGestures { change, dragAmount ->
                    change.consume()
                }
            }
    ) {
        val strokeWidth = 8.dp.toPx()
        val diameter = size.minDimension - strokeWidth
        val topLeft = Offset(strokeWidth / 2, strokeWidth / 2)
        val arcSize = Size(diameter, diameter)

        drawArc(
            color = TerminalColors.ProgressTrack,
            startAngle = -90f,
            sweepAngle = 360f,
            useCenter = false,
            topLeft = topLeft,
            size = arcSize,
            style = Stroke(width = strokeWidth, cap = StrokeCap.Round)
        )

        val sweep = progress * 360f
        if (sweep > 0) {
            drawArc(
                color = TerminalColors.Seek,
                startAngle = -90f,
                sweepAngle = sweep,
                useCenter = false,
                topLeft = topLeft,
                size = arcSize,
                style = Stroke(width = strokeWidth, cap = StrokeCap.Round)
            )
        }

        val volumeSweep = volume * 90f
        drawArc(
            color = TerminalColors.Volume.copy(alpha = 0.6f),
            startAngle = 180f,
            sweepAngle = volumeSweep,
            useCenter = false,
            topLeft = Offset(topLeft.x + strokeWidth, topLeft.y + strokeWidth),
            size = Size(diameter - strokeWidth * 2, diameter - strokeWidth * 2),
            style = Stroke(width = strokeWidth / 2, cap = StrokeCap.Round)
        )
    }
}

@Composable
fun PlayerControls(
    state: NativeBridge.PlaybackState,
    onPlay: () -> Unit,
    onPause: () -> Unit,
    onStop: () -> Unit,
    onSeekBackward: () -> Unit,
    onSeekForward: () -> Unit,
    onRunningMode: () -> Unit
) {
    Row(
        horizontalArrangement = Arrangement.Center,
        verticalAlignment = Alignment.CenterVertically,
        modifier = Modifier.fillMaxWidth()
    ) {
        CompactButton(
            onClick = onSeekBackward,
            colors = ButtonDefaults.buttonColors(
                backgroundColor = TerminalColors.Surface
            ),
            modifier = Modifier.size(32.dp)
        ) {
            Text("◄◄", fontSize = 10.sp, color = TerminalColors.TextSecondary)
        }

        Spacer(modifier = Modifier.width(8.dp))

        Button(
            onClick = {
                when (state) {
                    NativeBridge.PlaybackState.PLAYING -> onPause()
                    else -> onPlay()
                }
            },
            colors = ButtonDefaults.buttonColors(
                backgroundColor = if (state == NativeBridge.PlaybackState.PLAYING)
                    TerminalColors.Paused else TerminalColors.Playing
            ),
            modifier = Modifier.size(44.dp)
        ) {
            Text(
                text = when (state) {
                    NativeBridge.PlaybackState.PLAYING -> "❚❚"
                    else -> "▶"
                },
                fontSize = 16.sp,
                color = Color.Black
            )
        }

        Spacer(modifier = Modifier.width(8.dp))

        CompactButton(
            onClick = onSeekForward,
            colors = ButtonDefaults.buttonColors(
                backgroundColor = TerminalColors.Surface
            ),
            modifier = Modifier.size(32.dp)
        ) {
            Text("►►", fontSize = 10.sp, color = TerminalColors.TextSecondary)
        }

        Spacer(modifier = Modifier.width(8.dp))

        CompactButton(
            onClick = onStop,
            colors = ButtonDefaults.buttonColors(
                backgroundColor = TerminalColors.Surface
            ),
            modifier = Modifier.size(32.dp)
        ) {
            Text("■", fontSize = 12.sp, color = TerminalColors.Red)
        }

        Spacer(modifier = Modifier.width(4.dp))

        CompactButton(
            onClick = onRunningMode,
            colors = ButtonDefaults.buttonColors(
                backgroundColor = TerminalColors.Surface
            ),
            modifier = Modifier.size(28.dp)
        ) {
            Text("🏃", fontSize = 10.sp)
        }
    }
}

@Composable
fun StateIndicator(
    state: NativeBridge.PlaybackState,
    modifier: Modifier = Modifier
) {
    val color = when (state) {
        NativeBridge.PlaybackState.PLAYING -> TerminalColors.Playing
        NativeBridge.PlaybackState.PAUSED -> TerminalColors.Paused
        NativeBridge.PlaybackState.BUFFERING -> TerminalColors.Cyan
        NativeBridge.PlaybackState.ERROR -> TerminalColors.Error
        else -> Color.Transparent
    }

    if (color != Color.Transparent) {
        Box(
            modifier = modifier
                .padding(top = 4.dp)
                .size(6.dp)
                .clip(CircleShape)
                .background(color)
        )
    }
}

fun formatTime(ms: Long): String {
    val totalSeconds = ms / 1000
    val minutes = totalSeconds / 60
    val seconds = totalSeconds % 60
    return "%d:%02d".format(minutes, seconds)
}
