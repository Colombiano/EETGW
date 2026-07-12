package com.eetgw.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
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
// RunningModeScreen.kt — Modo Running simplificado / Simplified Running mode
// =============================================================================

@Composable
fun RunningModeScreen(
    bridge: NativeBridge,
    onBack: () -> Unit
) {
    val state by bridge.playbackState
    val position by bridge.currentPositionMs
    val duration by bridge.durationMs
    val meta by bridge.metadata

    EETGWTheme {
        Scaffold(
            modifier = Modifier.fillMaxSize(),
            vignette = { Vignette(vignettePosition = VignettePosition.TopAndBottom) }
        ) {
            Box(
                modifier = Modifier
                    .fillMaxSize()
                    .background(TerminalColors.Background)
                    .clickable { onBack() },
                contentAlignment = Alignment.Center
            ) {
                Column(
                    horizontalAlignment = Alignment.CenterHorizontally,
                    verticalArrangement = Arrangement.Center,
                    modifier = Modifier.fillMaxSize()
                ) {
                    Text(
                        text = formatTime(position),
                        style = MaterialTheme.typography.display1,
                        color = TerminalColors.Cyan,
                        fontSize = 36.sp,
                        textAlign = TextAlign.Center
                    )

                    Spacer(modifier = Modifier.height(4.dp))

                    Text(
                        text = "${formatTime(position)} / ${formatTime(duration)}",
                        style = MaterialTheme.typography.caption1,
                        color = TerminalColors.TextSecondary,
                        fontSize = 12.sp
                    )

                    Spacer(modifier = Modifier.height(16.dp))

                    Box(
                        modifier = Modifier
                            .size(80.dp)
                            .clip(CircleShape)
                            .background(
                                if (state == NativeBridge.PlaybackState.PLAYING)
                                    TerminalColors.Paused
                                else
                                    TerminalColors.Playing
                            )
                            .clickable {
                                when (state) {
                                    NativeBridge.PlaybackState.PLAYING -> bridge.pause()
                                    else -> bridge.play()
                                }
                            },
                        contentAlignment = Alignment.Center
                    ) {
                        Text(
                            text = when (state) {
                                NativeBridge.PlaybackState.PLAYING -> "❚❚"
                                else -> "▶"
                            },
                            fontSize = 32.sp,
                            color = Color.Black,
                            textAlign = TextAlign.Center
                        )
                    }

                    Spacer(modifier = Modifier.height(12.dp))

                    Text(
                        text = meta.titleDisplay,
                        style = MaterialTheme.typography.caption1,
                        color = TerminalColors.TextPrimary,
                        maxLines = 1,
                        overflow = TextOverflow.Ellipsis,
                        fontSize = 10.sp,
                        textAlign = TextAlign.Center,
                        modifier = Modifier.padding(horizontal = 16.dp)
                    )

                    Spacer(modifier = Modifier.height(4.dp))

                    Text(
                        text = "toque p/ voltar",
                        style = MaterialTheme.typography.caption3,
                        color = TerminalColors.TextDisabled,
                        fontSize = 6.sp
                    )
                }
            }
        }
    }
}
