package com.eetgw.ui.theme

import androidx.compose.runtime.Composable
import androidx.wear.compose.material.MaterialTheme

// =============================================================================
// EETGW — Essa e pra tocar no Galaxy Watch
// Theme.kt — Tema principal / Main theme
// =============================================================================

@Composable
fun EETGWTheme(content: @Composable () -> Unit) {
    MaterialTheme(
        colors = EETGWColors,
        typography = EETGWTypography,
        content = content
    )
}
