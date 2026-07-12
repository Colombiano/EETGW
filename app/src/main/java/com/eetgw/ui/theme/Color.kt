package com.eetgw.ui.theme

import androidx.compose.ui.graphics.Color
import androidx.wear.compose.material.Colors

// =============================================================================
// EETGW — Essa e pra tocar no Galaxy Watch
// Color.kt — Tema "Northern Lights" (Terminal Linux style)
// =============================================================================

object TerminalColors {
    val Background = Color(0xFF0A0A0F)
    val Surface = Color(0xFF12121A)
    val SurfaceVariant = Color(0xFF1A1A24)

    val TextPrimary = Color(0xFFE0E0E0)
    val TextSecondary = Color(0xFF808080)
    val TextDisabled = Color(0xFF505050)

    val Cyan = Color(0xFF00F0FF)
    val Magenta = Color(0xFF00FF9C)
    val Lime = Color(0xFF39FF14)
    val Yellow = Color(0xFFFFD700)
    val Red = Color(0xFFFF3040)

    val Seek = Cyan
    val Volume = Magenta

    val ProgressTrack = Color(0xFF2A2A35)
    val ProgressIndicator = Cyan

    val Playing = Lime
    val Paused = Yellow
    val Error = Red
}

val EETGWColors = Colors(
    primary = TerminalColors.Cyan,
    primaryVariant = TerminalColors.Cyan.copy(alpha = 0.7f),
    secondary = TerminalColors.Magenta,
    secondaryVariant = TerminalColors.Magenta.copy(alpha = 0.7f),
    background = TerminalColors.Background,
    surface = TerminalColors.Surface,
    error = TerminalColors.Red,
    onPrimary = Color.Black,
    onSecondary = Color.Black,
    onBackground = TerminalColors.TextPrimary,
    onSurface = TerminalColors.TextPrimary,
    onError = Color.White
)
