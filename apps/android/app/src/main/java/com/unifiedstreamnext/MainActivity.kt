package com.unifiedstreamnext

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp

enum class UsnTab(val title: String) {
    HOME("首页"),
    DEVICES("设备"),
    ADD("添加"),
    CONNECTIONS("连接"),
    HISTORY("历史"),
    SETTINGS("设置"),
    ABOUT("关于")
}

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            var darkTheme by remember { mutableStateOf(false) }
            var selectedTab by remember { mutableStateOf(UsnTab.HOME) }
            val colorScheme = if (darkTheme) darkColorScheme() else lightColorScheme()

            MaterialTheme(colorScheme = colorScheme) {
                Scaffold(
                    topBar = {
                        @OptIn(ExperimentalMaterial3Api::class)
                        TopAppBar(
                            title = { Text("UnifiedStreamNext — ${selectedTab.title}") },
                            actions = {
                                Switch(checked = darkTheme, onCheckedChange = { darkTheme = it })
                            }
                        )
                    },
                    bottomBar = {
                        NavigationBar {
                            listOf(UsnTab.HOME, UsnTab.DEVICES, UsnTab.CONNECTIONS, UsnTab.SETTINGS).forEach { tab ->
                                NavigationBarItem(
                                    selected = selectedTab == tab,
                                    onClick = { selectedTab = tab },
                                    icon = { Text(tab.title.take(1)) },
                                    label = { Text(tab.title) }
                                )
                            }
                        }
                    }
                ) { padding ->
                    Column(
                        modifier = Modifier
                            .fillMaxSize()
                            .padding(padding)
                            .padding(16.dp),
                        verticalArrangement = Arrangement.spacedBy(12.dp)
                    ) {
                        Text(
                            text = "UnifiedStreamNext Android Client & Host",
                            style = MaterialTheme.typography.headlineSmall
                        )
                        Text(
                            text = "通过统一连接管理器调度 Moonlight Android（局域网串流）、RustDesk（跨网远程桌面）与独立 AndroidHostEngine。",
                            style = MaterialTheme.typography.bodyMedium
                        )
                    }
                }
            }
        }
    }
}
