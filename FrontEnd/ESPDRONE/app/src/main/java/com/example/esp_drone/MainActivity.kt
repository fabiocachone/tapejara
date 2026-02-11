package com.example.esp_drone

import android.content.Context.MODE_PRIVATE
import android.net.Uri
import android.os.Bundle
import android.view.ViewGroup
import android.view.WindowManager
import android.widget.Toast
import android.widget.VideoView
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.gestures.detectDragGestures
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Home
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.alpha
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.PasswordVisualTransformation
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.ui.viewinterop.AndroidView
import kotlinx.coroutines.*
import java.net.DatagramPacket
import java.net.DatagramSocket
import java.net.InetAddress
import java.nio.ByteBuffer
import java.nio.ByteOrder
import kotlin.math.abs
import kotlin.math.hypot
import kotlin.math.min
import kotlin.math.roundToInt

/* ===================== MAIN ===================== */

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        setContent {
            var screen by remember { mutableStateOf<Screen>(Screen.Control) }
            when (screen) {
                is Screen.Control -> ControlScreen(onOpenSettings = { screen = Screen.Settings })
                is Screen.Settings -> SettingsScreen(onBack = { screen = Screen.Control })
            }
        }
    }
}

sealed class Screen {
    data object Control : Screen()
    data object Settings : Screen()
}

/* ===================== PROTO / UDP ===================== */

data class ControlState(
    var roll: Float = 0f,
    var pitch: Float = 0f,
    var yaw: Float = 0f,
    var throttle: Float = 0f,
    var buttons: Int = 0
)

class UdpClient(private var host: String = "192.168.4.1", private var port: Int = 9000) {
    private var socket: DatagramSocket? = null
    private var seq = 0
    fun configure(host: String, port: Int) { this.host = host; this.port = port }
    fun open() { if (socket == null || socket!!.isClosed) socket = DatagramSocket() }
    fun close() { socket?.close() }

    fun send(state: ControlState) {
        val bb = ByteBuffer.allocate(24).order(ByteOrder.LITTLE_ENDIAN)
        bb.put(0xED.toByte()); bb.put(0x01); bb.put(0x00); bb.put(0x00)
        bb.putShort(seq.toShort())
        bb.putFloat(state.roll); bb.putFloat(state.pitch); bb.putFloat(state.yaw); bb.putFloat(state.throttle)
        bb.putShort(state.buttons.toShort())
        val arr = bb.array()
        var sum = 0
        for (i in 0 until arr.size - 2) sum = (sum + (arr[i].toInt() and 0xFF)) and 0xFFFF
        arr[arr.size - 2] = (sum and 0xFF).toByte()
        arr[arr.size - 1] = ((sum ushr 8) and 0xFF).toByte()
        seq = (seq + 1) and 0xFFFF

        val pkt = DatagramPacket(arr, arr.size, InetAddress.getByName(host), port)
        socket?.send(pkt)
    }
}

/* ===================== CONTROLE ===================== */

@Composable
fun ControlScreen(onOpenSettings: () -> Unit) {
    val scope = rememberCoroutineScope()
    val client = remember { UdpClient() }
    val state = remember { mutableStateOf(ControlState()) }
    var sending by remember { mutableStateOf(false) }
    var battery by remember { mutableFloatStateOf(0.82f) }

    DisposableEffect(Unit) {
        client.open(); sending = true
        val job = scope.launch(Dispatchers.IO) {
            while (isActive && sending) { client.send(state.value); delay(33) }
        }
        onDispose { sending = false; job.cancel(); client.close() }
    }

    Box(Modifier.fillMaxSize().background(Color(0xFF0B0F14))) {

        // >>> FUNDO COM VÍDEO (URL RAW DO GITHUB)
        VideoBackground(
            url = "https://raw.githubusercontent.com/fabiocachone/tapejara/main/tapejara_demo_flight_5s.mp4"
        )

        Text(
            "TAPEJARA",
            color = Color.White,
            fontWeight = FontWeight.SemiBold,
            modifier = Modifier.align(Alignment.TopStart).padding(start = 16.dp, top = 12.dp).alpha(0.9f)
        )
        Box(Modifier.align(Alignment.TopEnd).padding(16.dp)) { BatteryChip(battery) }

        GlassJoystick(
            size = 200.dp,
            modifier = Modifier.align(Alignment.BottomStart).padding(start = 16.dp, bottom = 24.dp)
        ) { x, y -> state.value = state.value.copy(roll = dz(x), pitch = dz(-y)) }
        Text("Rolagem / Arfagem", color = Color.White.copy(alpha = 0.9f),
            modifier = Modifier.align(Alignment.BottomStart).padding(start = 24.dp, bottom = 8.dp))

        GlassJoystick(
            size = 200.dp,
            modifier = Modifier.align(Alignment.BottomEnd).padding(end = 16.dp, bottom = 24.dp)
        ) { x, y ->
            val thr = ((-y + 1f) / 2f).coerceIn(0f, 1f)
            state.value = state.value.copy(yaw = dz(x), throttle = thr)
        }
        Text("Guinada / Altitude", color = Color.White.copy(alpha = 0.9f),
            modifier = Modifier.align(Alignment.BottomEnd).padding(end = 24.dp, bottom = 8.dp))

        BottomControls(
            onHome = {
                val mask = 1 shl 0
                state.value = state.value.copy(buttons = state.value.buttons or mask)
                scope.launch { delay(120); state.value = state.value.copy(buttons = state.value.buttons and mask.inv()) }
            },
            onLand = {
                val mask = 1 shl 1
                state.value = state.value.copy(buttons = state.value.buttons or mask)
                scope.launch { delay(120); state.value = state.value.copy(buttons = state.value.buttons and mask.inv()) }
            },
            onSettings = onOpenSettings,
            modifier = Modifier.align(Alignment.BottomCenter).padding(bottom = 18.dp)
        )

        DebugHUD(state.value, Modifier.align(Alignment.TopCenter).padding(top = 10.dp))
    }
}

/* ===================== FUNDO EM VÍDEO ===================== */

@Composable
private fun VideoBackground(url: String) {
    Box(
        Modifier
            .fillMaxSize()
            .padding(horizontal = 16.dp, vertical = 24.dp)
            .clip(RoundedCornerShape(18.dp))
            .background(Color(0xFF121A21))
    ) {
        AndroidView(
            modifier = Modifier.matchParentSize(),
            factory = { context ->
                VideoView(context).apply {
                    layoutParams = ViewGroup.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        ViewGroup.LayoutParams.MATCH_PARENT
                    )

                    // Marca a URL no tag, para evitar re-set desnecessário
                    tag = url
                    setVideoURI(Uri.parse(url))

                    setOnPreparedListener { mp ->
                        mp.isLooping = true
                        start()
                    }

                    // Se der erro, não trava o app (fica só o fundo escuro)
                    setOnErrorListener { _, _, _ ->
                        false
                    }
                }
            },
            update = { vv ->
                val current = vv.tag as? String
                if (current != url) {
                    vv.tag = url
                    vv.setVideoURI(Uri.parse(url))
                    vv.start()
                }
            }
        )

        // vignette/escurecido leve pra deixar o overlay legível
        Canvas(Modifier.matchParentSize()) {
            drawRect(
                brush = Brush.radialGradient(
                    colors = listOf(Color.Transparent, Color(0x66000000)),
                    center = Offset(size.width / 2f, size.height / 2f),
                    radius = maxOf(size.width, size.height) * 0.85f
                ),
                size = size
            )
        }

        Text(
            "Pré-visualização (demo)",
            color = Color.White.copy(alpha = 0.75f),
            modifier = Modifier.align(Alignment.TopStart).padding(12.dp)
        )
    }
}

/* ===================== CONFIGURAÇÕES (3 ABAS) ===================== */

@Composable
fun SettingsScreen(onBack: () -> Unit) {
    val ctx = LocalContext.current
    val prefs = remember { ctx.getSharedPreferences("tapejara_prefs", MODE_PRIVATE) }

    var tab by remember { mutableStateOf(0) }

    // Máx
    var maxPitch by remember { mutableStateOf(prefs.getInt("max_pitch", 10)) }
    var maxRoll by remember { mutableStateOf(prefs.getInt("max_roll", 20)) }
    var maxYaw by remember { mutableStateOf(prefs.getInt("max_yaw", 30)) }
    var maxAccel by remember { mutableStateOf(prefs.getInt("max_accel", 10)) }
    var hoverPwm by remember { mutableStateOf(prefs.getInt("hover_pwm", 2350)) }

    // PID (strings para aceitar vírgula/ponto)
    fun getF(key: String, def: String) = prefs.getString(key, def)!!
    var pidPitchRateP by remember { mutableStateOf(getF("pid_pitch_rate_p", "5")) }
    var pidPitchRateI by remember { mutableStateOf(getF("pid_pitch_rate_i", "0.5")) }
    var pidPitchRateD by remember { mutableStateOf(getF("pid_pitch_rate_d", "0.01")) }

    var pidRollRateP by remember { mutableStateOf(getF("pid_roll_rate_p", "5")) }
    var pidRollRateI by remember { mutableStateOf(getF("pid_roll_rate_i", "0.5")) }
    var pidRollRateD by remember { mutableStateOf(getF("pid_roll_rate_d", "0.01")) }

    var pidYawRateP by remember { mutableStateOf(getF("pid_yaw_rate_p", "5")) }
    var pidYawRateI by remember { mutableStateOf(getF("pid_yaw_rate_i", "0.5")) }
    var pidYawRateD by remember { mutableStateOf(getF("pid_yaw_rate_d", "0.01")) }

    var pidPitchAngP by remember { mutableStateOf(getF("pid_pitch_ang_p", "5")) }
    var pidPitchAngI by remember { mutableStateOf(getF("pid_pitch_ang_i", "0.5")) }
    var pidPitchAngD by remember { mutableStateOf(getF("pid_pitch_ang_d", "0.01")) }

    var pidRollAngP by remember { mutableStateOf(getF("pid_roll_ang_p", "5")) }
    var pidRollAngI by remember { mutableStateOf(getF("pid_roll_ang_i", "0.5")) }
    var pidRollAngD by remember { mutableStateOf(getF("pid_roll_ang_d", "0.01")) }

    var pidAltRateP by remember { mutableStateOf(getF("pid_alt_rate_p", "5")) }
    var pidAltRateI by remember { mutableStateOf(getF("pid_alt_rate_i", "0.5")) }
    var pidAltRateD by remember { mutableStateOf(getF("pid_alt_rate_d", "0.01")) }

    // Wi-Fi
    var wifiSsid by remember { mutableStateOf(prefs.getString("wifi_ssid", "") ?: "") }
    var wifiPass by remember { mutableStateOf(prefs.getString("wifi_pass", "") ?: "") }

    fun saveAll() {
        prefs.edit()
            .putInt("max_pitch", maxPitch)
            .putInt("max_roll", maxRoll)
            .putInt("max_yaw", maxYaw)
            .putInt("max_accel", maxAccel)
            .putInt("hover_pwm", hoverPwm)
            .putString("pid_pitch_rate_p", pidPitchRateP)
            .putString("pid_pitch_rate_i", pidPitchRateI)
            .putString("pid_pitch_rate_d", pidPitchRateD)
            .putString("pid_roll_rate_p", pidRollRateP)
            .putString("pid_roll_rate_i", pidRollRateI)
            .putString("pid_roll_rate_d", pidRollRateD)
            .putString("pid_yaw_rate_p", pidYawRateP)
            .putString("pid_yaw_rate_i", pidYawRateI)
            .putString("pid_yaw_rate_d", pidYawRateD)
            .putString("pid_pitch_ang_p", pidPitchAngP)
            .putString("pid_pitch_ang_i", pidPitchAngI)
            .putString("pid_pitch_ang_d", pidPitchAngD)
            .putString("pid_roll_ang_p", pidRollAngP)
            .putString("pid_roll_ang_i", pidRollAngI)
            .putString("pid_roll_ang_d", pidRollAngD)
            .putString("pid_alt_rate_p", pidAltRateP)
            .putString("pid_alt_rate_i", pidAltRateI)
            .putString("pid_alt_rate_d", pidAltRateD)
            .putString("wifi_ssid", wifiSsid)
            .putString("wifi_pass", wifiPass)
            .apply()
        Toast.makeText(ctx, "Configurações salvas", Toast.LENGTH_SHORT).show()
    }

    Scaffold(
        bottomBar = {
            Row(
                Modifier.fillMaxWidth().padding(16.dp),
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                Button(onClick = { saveAll() }) { Text("Salvar") }
                OutlinedButton(onClick = onBack) { Text("Voltar") }
            }
        }
    ) { inner ->
        Column(
            Modifier.fillMaxSize().padding(inner).padding(horizontal = 16.dp)
        ) {
            Text("Configuração", fontWeight = FontWeight.SemiBold, modifier = Modifier.padding(vertical = 12.dp))
            TabRow(selectedTabIndex = tab) {
                Tab(selected = tab == 0, onClick = { tab = 0 }, text = { Text("Máx") })
                Tab(selected = tab == 1, onClick = { tab = 1 }, text = { Text("PID") })
                Tab(selected = tab == 2, onClick = { tab = 2 }, text = { Text("Wi-Fi") })
            }
            when (tab) {
                0 -> {
                    Column(
                        Modifier.fillMaxSize().verticalScroll(rememberScrollState()).padding(vertical = 12.dp)
                    ) {
                        MaxSlider("Arfagem", maxPitch, 0..100) { maxPitch = it }
                        MaxSlider("Rolagem", maxRoll, 0..100) { maxRoll = it }
                        MaxSlider("Guinada", maxYaw, 0..100) { maxYaw = it }
                        MaxSlider("Aceleração", maxAccel, 0..100) { maxAccel = it }
                        MaxSlider("Pairar (PWM)", hoverPwm, 1000..2500, step = 10) { hoverPwm = it }
                        Spacer(Modifier.height(80.dp))
                    }
                }
                1 -> {
                    Column(
                        Modifier.fillMaxSize().verticalScroll(rememberScrollState()).padding(vertical = 12.dp)
                    ) {
                        PIDRow("Arfagem (taxa)", pidPitchRateP, pidPitchRateI, pidPitchRateD,
                            { pidPitchRateP = it }, { pidPitchRateI = it }, { pidPitchRateD = it })
                        PIDRow("Rolagem (taxa)", pidRollRateP, pidRollRateI, pidRollRateD,
                            { pidRollRateP = it }, { pidRollRateI = it }, { pidRollRateD = it })
                        PIDRow("Guinada (taxa)", pidYawRateP, pidYawRateI, pidYawRateD,
                            { pidYawRateP = it }, { pidYawRateI = it }, { pidYawRateD = it })
                        PIDRow("Arfagem (âng.)", pidPitchAngP, pidPitchAngI, pidPitchAngD,
                            { pidPitchAngP = it }, { pidPitchAngI = it }, { pidPitchAngD = it })
                        PIDRow("Rolagem (âng.)", pidRollAngP, pidRollAngI, pidRollAngD,
                            { pidRollAngP = it }, { pidRollAngI = it }, { pidRollAngD = it })
                        PIDRow("Altitude (taxa)", pidAltRateP, pidAltRateI, pidAltRateD,
                            { pidAltRateP = it }, { pidAltRateI = it }, { pidAltRateD = it })
                        Spacer(Modifier.height(80.dp))
                    }
                }
                2 -> {
                    Column(
                        Modifier.fillMaxSize().verticalScroll(rememberScrollState())
                            .padding(vertical = 12.dp),
                        verticalArrangement = Arrangement.spacedBy(12.dp)
                    ) {
                        Text("Credenciais", style = MaterialTheme.typography.titleMedium)
                        OutlinedTextField(
                            value = wifiSsid, onValueChange = { wifiSsid = it },
                            label = { Text("Rede (SSID)") }, singleLine = true,
                            modifier = Modifier.fillMaxWidth()
                        )
                        OutlinedTextField(
                            value = wifiPass, onValueChange = { wifiPass = it },
                            label = { Text("Senha") }, singleLine = true,
                            visualTransformation = PasswordVisualTransformation(),
                            modifier = Modifier.fillMaxWidth()
                        )
                        Spacer(Modifier.height(80.dp))
                    }
                }
            }
        }
    }
}

/* ---------- Componentes auxiliares das abas ---------- */

@Composable
private fun MaxSlider(
    label: String,
    value: Int,
    range: IntRange,
    step: Int = 1,
    onChange: (Int) -> Unit
) {
    Column(Modifier.fillMaxWidth().padding(vertical = 6.dp)) {
        Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween) {
            Text(label)
            OutlinedTextField(
                value = value.toString(),
                onValueChange = { txt ->
                    txt.filter { it.isDigit() }.toIntOrNull()?.let { v ->
                        val snapped = ((v - range.first) / step) * step + range.first
                        onChange(snapped.coerceIn(range.first, range.last))
                    }
                },
                singleLine = true,
                modifier = Modifier.width(90.dp)
            )
        }
        Slider(
            value = value.toFloat(),
            onValueChange = { f ->
                val v = f.toInt()
                val snapped = ((v - range.first) / step) * step + range.first
                onChange(snapped.coerceIn(range.first, range.last))
            },
            valueRange = range.first.toFloat()..range.last.toFloat()
        )
    }
}

@Composable
private fun PIDRow(
    title: String,
    pVal: String, iVal: String, dVal: String,
    onP: (String) -> Unit, onI: (String) -> Unit, onD: (String) -> Unit
) {
    Column(Modifier.fillMaxWidth().padding(vertical = 6.dp)) {
        Text(title)
        Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            PIDField("P", pVal, onP)
            PIDField("I", iVal, onI)
            PIDField("D", dVal, onD)
        }
    }
}

@Composable
private fun PIDField(label: String, value: String, onChange: (String) -> Unit) {
    OutlinedTextField(
        value = value,
        onValueChange = { s ->
            val clean = s.filter { it.isDigit() || it == '.' || it == ',' }
            onChange(clean)
        },
        label = { Text(label) },
        singleLine = true,
        modifier = Modifier.width(110.dp)
    )
}

/* ===================== VISUAIS DA TELA DE CONTROLE ===================== */

@Composable
fun BottomControls(
    onHome: () -> Unit,
    onLand: () -> Unit,
    onSettings: () -> Unit,
    modifier: Modifier = Modifier
) {
    Surface(modifier, color = Color.Black.copy(alpha = 0.35f), shape = RoundedCornerShape(16.dp)) {
        Row(
            Modifier.padding(horizontal = 18.dp, vertical = 10.dp),
            horizontalArrangement = Arrangement.spacedBy(28.dp, Alignment.CenterHorizontally),
            verticalAlignment = Alignment.CenterVertically
        ) {
            ControlIcon(Icons.Filled.Home, "Home", onHome)
            ControlButtonGlyph(label = "Pousar", onClick = onLand) { DroneLandGlyph(Modifier.size(24.dp)) }
            ControlIcon(Icons.Filled.Settings, "Config", onSettings)
        }
    }
}

@Composable
fun ControlIcon(icon: ImageVector, label: String, onClick: () -> Unit) {
    Column(horizontalAlignment = Alignment.CenterHorizontally) {
        IconButton(
            onClick = onClick,
            modifier = Modifier.size(54.dp).clip(RoundedCornerShape(12.dp)).background(Color(0x33000000))
        ) { Icon(icon, contentDescription = label, tint = Color.White) }
        Spacer(Modifier.height(6.dp))
        Text(label, color = Color.White, style = MaterialTheme.typography.labelMedium)
    }
}

@Composable
fun ControlButtonGlyph(label: String, onClick: () -> Unit, glyph: @Composable () -> Unit) {
    Column(horizontalAlignment = Alignment.CenterHorizontally) {
        IconButton(
            onClick = onClick,
            modifier = Modifier.size(54.dp).clip(RoundedCornerShape(12.dp)).background(Color(0x33000000))
        ) { glyph() }
        Spacer(Modifier.height(6.dp))
        Text(label, color = Color.White, style = MaterialTheme.typography.labelMedium)
    }
}

@Composable
fun DroneLandGlyph(modifier: Modifier = Modifier) {
    Canvas(modifier) {
        val w = size.width; val h = size.height
        val stroke = w * 0.07f
        val bodyW = w * 0.36f; val bodyH = h * 0.22f
        drawRoundRect(
            color = Color.White,
            topLeft = Offset(w / 2f - bodyW / 2f, h * 0.25f),
            size = Size(bodyW, bodyH),
            cornerRadius = CornerRadius(bodyH * 0.45f, bodyH * 0.45f)
        )
        val r = min(w, h) * 0.12f
        val rotors = listOf(
            Offset(w * 0.22f, h * 0.20f),
            Offset(w * 0.78f, h * 0.20f),
            Offset(w * 0.22f, h * 0.55f),
            Offset(w * 0.78f, h * 0.55f),
        )
        rotors.forEach { drawCircle(Color.White, radius = r, center = it) }
        val y0 = h * 0.65f
        drawLine(Color.White, Offset(w / 2f, y0), Offset(w / 2f, h * 0.88f), strokeWidth = stroke)
        drawLine(Color.White, Offset(w / 2f, h * 0.88f), Offset(w * 0.42f, h * 0.80f), strokeWidth = stroke)
        drawLine(Color.White, Offset(w / 2f, h * 0.88f), Offset(w * 0.58f, h * 0.80f), strokeWidth = stroke)
        drawLine(Color.White.copy(alpha = 0.75f), Offset(w * 0.28f, h * 0.95f), Offset(w * 0.72f, h * 0.95f), strokeWidth = stroke)
    }
}

/* ===================== EXTRAS / HUD ===================== */

@Composable
fun BatteryChip(level: Float) {
    val pct = (level * 100).roundToInt()
    Surface(color = Color.Black.copy(alpha = 0.35f), shape = RoundedCornerShape(12.dp)) {
        Row(Modifier.padding(horizontal = 10.dp, vertical = 6.dp), verticalAlignment = Alignment.CenterVertically) {
            Box(Modifier.width(40.dp).height(18.dp).clip(RoundedCornerShape(4.dp)).background(Color.DarkGray)) {
                Box(Modifier.fillMaxHeight().width((40 * level).dp)
                    .background(if (level > 0.3f) Color(0xFF62D26F) else Color(0xFFE54D4D)))
            }
            Spacer(Modifier.width(8.dp)); Text("$pct%", color = Color.White)
        }
    }
}

@Composable
fun GlassJoystick(size: Dp, modifier: Modifier = Modifier, onChange: (x: Float, y: Float) -> Unit) {
    val radiusPx = with(LocalDensity.current) { (size / 2).toPx() }
    var knob by remember { mutableStateOf(Offset.Zero) }
    Box(modifier.size(size).clip(CircleShape).background(Color(0xAA111418))) {
        Canvas(Modifier.matchParentSize().pointerInput(Unit) {
            detectDragGestures(
                onDragStart = { offset ->
                    knob = (offset - Offset(radiusPx, radiusPx)).limit(radiusPx)
                    onChange(knob.x / radiusPx, knob.y / radiusPx)
                },
                onDragEnd = { knob = Offset.Zero; onChange(0f, 0f) }
            ) { _, dragAmount ->
                knob = (knob + dragAmount).limit(radiusPx)
                onChange(knob.x / radiusPx, knob.y / radiusPx)
            }
        }) {
            drawCircle(
                color = Color(0xFFC7D1D9),
                radius = radiusPx / 3.2f,
                center = Offset(radiusPx, radiusPx) + knob
            )
        }
    }
}

@Composable
fun DebugHUD(s: ControlState, modifier: Modifier = Modifier) {
    Surface(modifier, color = Color.Black.copy(alpha = 0.45f), shape = RoundedCornerShape(10.dp)) {
        Text(
            text = "roll=${fmt(s.roll)}   pitch=${fmt(s.pitch)}   yaw=${fmt(s.yaw)}   thr=${fmt(s.throttle)}",
            color = Color.White,
            modifier = Modifier.padding(horizontal = 10.dp, vertical = 6.dp)
        )
    }
}

/* ===================== HELPERS ===================== */

private fun dz(v: Float, dead: Float = 0.06f): Float = if (abs(v) < dead) 0f else v
private fun fmt(v: Float) = String.format("%.2f", v)
private fun Offset.limit(r: Float): Offset { val d = hypot(x, y); return if (d <= r) this else this * (r / d) }
