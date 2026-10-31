#include <vector>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <ESP8266HTTPUpdateServer.h>
#include "logo.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
DNSServer dnsServer;
const byte DNS_PORT = 53;

// ==========================================
// 1. HARDWARE Y CONSTANTES
// ==========================================
const int motor_pins[4] = {14, 12, 13, 15}; // D5, D6, D7, D8
const int step_sequence[8][4] = {
    {1,0,0,0}, {1,1,0,0}, {0,1,0,0}, {0,1,1,0},
    {0,0,1,0}, {0,0,1,1}, {0,0,0,1}, {1,0,0,1}
};

const int PASOS_POR_MUESTRA = 32;
const int UMBRAL_DESCONEXION = 220;
const long MAX_PASOS_SEGURIDAD = 60000;

// Variables de Configuración con valores por defecto
int cfg_ui = 100;
int cfg_us = 920;
int cfg_vel = 7;
float cfg_tol_lin = 2.0;

inline int calcular_delay_us(int nivel) {
    nivel = constrain(nivel, 1, 10);
    return map(nivel, 1, 10, 3200, 800);
}
int cfg_delay = 1466;

// ==========================================
// 2. INTERFACES WEB (Frontend en PROGMEM)
// ==========================================
const char HTML_HOME[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Probador de Potenciómetros</title>
<link rel="icon" type="image/png" href="/icon.png">
<link rel="apple-touch-icon" href="/icon.png">
<link rel="manifest" href="/manifest.json">
<meta name="mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
<meta name="theme-color" content="#8b5cf6">
<style>
:root { 
    --bg-base: #0f1115; 
    --bg-panel: #1a1d24; 
    --bg-input: #252932; 
    --text-main: #e2e8f0; 
    --accent: #8b5cf6; 
    --border: #334155; 
}
* { box-sizing: border-box; }
html, body { 
    height: 100%; 
    height: 100dvh; 
    margin: 0; 
    padding: 0; 
    overflow: hidden; 
    background: var(--bg-base); 
    color: var(--text-main); 
    font-family: system-ui, -apple-system, sans-serif; 
}
.dashboard { 
    position: relative; 
    width: 100%; 
    height: 100%; 
    height: 100dvh; 
    display: flex; 
    flex-direction: column; 
    background: var(--bg-panel); 
    padding: 10px 12px; 
    overflow: hidden;
}
.header-container { text-align: center; margin-bottom: 2px; }
.logo-img { max-width: 220px; width: 55%; max-height: 38px; object-fit: contain; margin-bottom: 2px; }
h2 { margin: 0; color: var(--accent); text-transform: uppercase; font-size: 13px; letter-spacing: 0.5px; }

.theme-btn {
    position: absolute !important;
    top: 10px !important;
    left: 10px !important;
    background: none !important;
    border: none !important;
    font-size: 18px !important;
    cursor: pointer;
    color: #94a3b8;
    padding: 0 !important;
    width: auto !important;
    z-index: 20;
}
.gear-btn { 
    position: absolute !important; 
    top: 10px !important; 
    right: 10px !important; 
    background: none !important; 
    border: none !important; 
    font-size: 20px !important; 
    cursor: pointer; 
    color: #94a3b8; 
    padding: 0 !important; 
    width: auto !important;
    z-index: 20;
}

body.light-theme { 
    --bg-base: #e2e8f0; 
    --bg-panel: #f8fafc; 
    --bg-input: #ffffff; 
    --text-main: #0f172a; 
    --border: #cbd5e1; 
}
body.light-theme .gear-btn, body.light-theme .theme-btn { color: #64748b; }
body.light-theme canvas { background: #ffffff; border-color: #cbd5e1; }
.status-box { 
    background: var(--bg-input); 
    border-left: 4px solid var(--accent); 
    padding: 5px 10px; 
    border-radius: 4px; 
    margin-bottom: 4px; 
    font-family: monospace; 
    font-size: 11px; 
}
canvas { 
    flex: 1; 
    width: 100%; 
    min-height: 155px; 
    background: #0b0f19; 
    border: 1px solid var(--border); 
    border-radius: 8px; 
    margin-bottom: 4px; 
    display: block; 
}
#qcBox {
    display: none;
    background: var(--bg-input);
    padding: 5px 8px;
    border-radius: 6px;
    margin-bottom: 5px;
    border: 1px solid var(--border);
}
.qc-grid {
    display: grid;
    grid-template-columns: repeat(4, 1fr);
    gap: 4px;
    text-align: center;
}
.qc-item { font-size: 10px; color: #94a3b8; }
.qc-val { font-size: 12px; font-weight: bold; color: #38bdf8; display: block; }
.pass-badge {
    display: inline-block;
    padding: 2px 6px;
    border-radius: 4px;
    font-weight: bold;
    font-size: 11px;
}
.badge-pass { background: #166534; color: #4ade80; border: 1px solid #22c55e; }
.badge-fail { background: #991b1b; color: #f87171; border: 1px solid #ef4444; }

input[type="text"], select { 
    width: 100%; 
    padding: 6px 8px; 
    background: var(--bg-input); 
    border: 1px solid var(--border); 
    color: var(--text-main); 
    border-radius: 6px; 
    margin-bottom: 5px; 
    font-size: 12px; 
}
.row { display: flex; gap: 6px; }
button { 
    padding: 9px 8px; 
    font-weight: bold; 
    border: none; 
    border-radius: 6px; 
    cursor: pointer; 
    color: white; 
    text-transform: uppercase; 
    width: 100%; 
    transition: opacity 0.2s; 
    font-size: 11px; 
}
button:hover { opacity: 0.9; }
.btn-primary { background: var(--accent); }
.btn-history { background: var(--bg-input); border: 1px solid var(--border); color: #94a3b8; font-size: 10px; padding: 5px; }
#sectionDesc {
    font-size: 11px;
    color: #a78bfa;
    margin: 0 0 2px 0;
    text-align: center;
    min-height: 12px;
}
</style>
</head>
<body>
<div class="dashboard">
    <button class="theme-btn" id="themeBtn" onclick="toggleTheme()" title="Modo Claro / Oscuro">🌙</button>
    <button class="gear-btn" onclick="window.location.href='/ajustes'" title="Ajustes y Modo Manual">⚙️</button>
    <div class="header-container">
        <img src="/logo.png" class="logo-img" alt="Logo">
        <h2>Probador de potenciómetros</h2>
    </div>
    <div class="status-box" id="estado">> Sistema listo.</div>
    <canvas id="grafico"></canvas>
    
    <div id="qcBox">
        <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:4px;">
            <span id="qcVerdict" class="pass-badge badge-pass">✅ PASS</span>
            <span id="qcTipo" style="font-size:11px; font-weight:bold; color:#c084fc;">Tipo: Lineal B</span>
        </div>
        <div class="qc-grid">
            <div class="qc-item">Linealidad<span class="qc-val" id="qcLin">0.0%</span></div>
            <div class="qc-item">Histéresis<span class="qc-val" id="qcHist">0.0%</span></div>
            <div class="qc-item">Ruido CRV<span class="qc-val" id="qcRuido">0.0%</span></div>
            <div class="qc-item">Recorrido<span class="qc-val" id="qcSpan">---</span></div>
        </div>
    </div>

    <div class="row">
        <div style="flex: 1.2;"><input type="text" id="p_nombre" placeholder="Nombre / ID"></div>
        <div style="flex: 1;"><input type="text" id="p_modelo" placeholder="Modelo / Parte"></div>
        <div style="flex: 1.1;">
            <select id="p_modo_vel" title="Modo de Velocidad y Ciclos">
                <option value="1">1 Ciclo (Vel. Normal)</option>
                <option value="multi">⚡ 3 Velocidades (Lenta/Media/Rápida)</option>
                <option value="5">5 Ciclos (Burn-in)</option>
                <option value="10">10 Ciclos (Burn-in)</option>
                <option value="20">20 Ciclos (Burn-in)</option>
            </select>
        </div>
    </div>
    <div id="sectionDesc"></div>
    <div class="row" style="margin-bottom: 6px;">
        <button class="btn-primary" id="btn-ciclo" onclick="iniciarCiclo()" style="flex: 1;">▶ Automático</button>
        <button id="btn-section" onclick="toggleSectionMode()" style="flex: 1; background: #8b5cf6;">🔍 Detalle</button>
    </div>
    <button class="btn-history" onclick="window.location.href='/historial'">Ver Historial de Registros</button>
</div>

<script>
const statusBox = document.getElementById('estado');
function actualizarEstado(msg, err=false) { 
    statusBox.innerHTML = `> ${msg}`; 
    statusBox.style.borderLeftColor = err ? '#ef4444' : 'var(--accent)'; 
    statusBox.style.color = err ? '#fca5a5' : 'var(--text-main)'; 
}

let statusInterval = null;
let autoData = [], autoDataV = [], autoDataH = [];
let pasadasPrevias = [];
let ultimoCicloReportado = 1;
let cicloActivo = false, isSectionMode = false, selectingZone = false;
let last_max_pos = parseInt(localStorage.getItem('pote_max_pos') || '0', 10);
let cfg_ui_val = 100, cfg_us_val = 920;
let markerL = 0.25, markerR = 0.75, dragging = null;
let bgDataH = [], bgData = [], bgDataV = [];
let sec_adc_min = 0, sec_adc_max = 1024;
const GRAB_PX = 20;

function aplicarTema(t) {
    let btn = document.getElementById('themeBtn');
    if (t === 'light') {
        document.body.classList.add('light-theme');
        if (btn) btn.innerText = '☀️';
        localStorage.setItem('pote_theme', 'light');
    } else {
        document.body.classList.remove('light-theme');
        if (btn) btn.innerText = '🌙';
        localStorage.setItem('pote_theme', 'dark');
    }
    if (isSectionMode) dibujarCurvaSec([], [], []);
    else dibujarCurva(autoDataH, autoData, autoDataV);
}

function toggleTheme() {
    let actual = localStorage.getItem('pote_theme') || 'dark';
    aplicarTema(actual === 'dark' ? 'light' : 'dark');
}

function getCanvasX(e) {
    let c = document.getElementById('grafico');
    let r = c.getBoundingClientRect();
    let clientX = (e.touches && e.touches.length > 0) ? e.touches[0].clientX : e.clientX;
    return Math.max(0, Math.min(c.width, (clientX - r.left) * (c.width / r.width)));
}

function onPointerDown(e) {
    if (cicloActivo || last_max_pos === 0 || !selectingZone) return;
    let c = document.getElementById('grafico');
    let x = getCanvasX(e);
    let lx = markerL * c.width, rx = markerR * c.width;
    if (Math.abs(x - lx) <= GRAB_PX) { dragging = 'L'; e.preventDefault(); }
    else if (Math.abs(x - rx) <= GRAB_PX) { dragging = 'R'; e.preventDefault(); }
}

function onPointerMove(e) {
    if (!dragging) return;
    let c = document.getElementById('grafico');
    let x = getCanvasX(e);
    let frac = Math.max(0, Math.min(1, x / c.width));
    const MIN_ZONE_FRAC = 0.15;
    if (dragging === 'L') markerL = Math.max(0, Math.min(frac, markerR - MIN_ZONE_FRAC));
    else                  markerR = Math.min(1, Math.max(frac, markerL + MIN_ZONE_FRAC));
    dibujarCurva(autoDataH, autoData, autoDataV);
    e.preventDefault();
}

function onPointerUp(e) { dragging = null; }

(function attachDragListeners() {
    let c = document.getElementById('grafico');
    c.addEventListener('mousedown',  onPointerDown);
    c.addEventListener('mousemove',  onPointerMove);
    window.addEventListener('mouseup',    onPointerUp);
    c.addEventListener('touchstart', onPointerDown, {passive: false});
    c.addEventListener('touchmove',  onPointerMove, {passive: false});
    window.addEventListener('touchend',   onPointerUp);
})();

function toggleSectionMode() {
    if (cicloActivo) return;
    if (isSectionMode && !selectingZone) {
        isSectionMode = false;
        document.getElementById('btn-section').innerText = "🔍 Detalle";
        document.getElementById('btn-section').style.background = "#8b5cf6";
        document.getElementById('sectionDesc').innerText = "";
        if (bgData && bgData.length > 0) {
            autoDataH = [...bgDataH]; autoData = [...bgData]; autoDataV = [...bgDataV];
            bgData = []; bgDataH = []; bgDataV = [];
        }
        dibujarCurva(autoDataH, autoData, autoDataV);
        return;
    }
    if (last_max_pos === 0) {
        alert("Primero realiza una prueba automática completa para calibrar el recorrido.");
        return;
    }
    if (!selectingZone) {
        selectingZone = true;
        document.getElementById('btn-section').innerText = "▶ Muestrear Zona";
        document.getElementById('btn-section').style.background = "#10b981";
        document.getElementById('sectionDesc').innerText = "Arrastra los límites morados y presiona Muestrear.";
        dibujarCurva(autoDataH, autoData, autoDataV);
    } else {
        selectingZone = false;
        iniciarSeccionVisual();
    }
}

function iniciarSeccionVisual() {
    let c = document.getElementById('grafico');
    let start = Math.round(Math.min(markerL, markerR) * last_max_pos);
    let end   = Math.round(Math.max(markerL, markerR) * last_max_pos);
    let startFrac = Math.min(markerL, markerR);
    let endFrac   = Math.max(markerL, markerR);

    bgDataH = [...autoDataH]; bgData = [...autoData]; bgDataV = [...autoDataV];

    let adc_start = "N/A", adc_end = "N/A";
    if (bgData && bgData.length > 1) {
        let i_start = Math.max(0, Math.min(bgData.length - 1, Math.floor(startFrac * (bgData.length - 1))));
        let i_end   = Math.max(0, Math.min(bgData.length - 1, Math.ceil(endFrac * (bgData.length - 1))));
        adc_start = bgData[i_start];
        adc_end   = bgData[i_end];
        let raw_min = Math.min(adc_start, adc_end), raw_max = Math.max(adc_start, adc_end);
        let mid = (raw_min + raw_max) / 2;
        let MIN_ADC_SPAN = 220;
        let span = Math.max(raw_max - raw_min, MIN_ADC_SPAN);
        sec_adc_min = Math.max(0, Math.round(mid - span / 2));
        sec_adc_max = Math.min(1024, Math.round(mid + span / 2));
    } else {
        sec_adc_min = 0; sec_adc_max = 1024;
    }
    document.getElementById('sectionDesc').innerText = `Zona zoom: ADC ~${adc_start} a ~${adc_end}`;

    let secH = [], secIda = [], secVuelta = [];
    isSectionMode = true;
    document.getElementById('qcBox').style.display = 'none';

    fetch('/section_sweep?start=' + start + '&end=' + end).then(r => r.text()).then(res => {
        cicloActivo = true;
        actualizarEstado("Iniciando análisis de zona...");
        statusBox.style.borderLeftColor = '#3b82f6';
        let isFetching = false;
        statusInterval = setInterval(() => {
            if (isFetching) return;
            isFetching = true;
            let controller = new AbortController();
            let tid = setTimeout(() => controller.abort(), 1200);
            fetch('/status?last=' + secIda.length + '&last_v=' + secVuelta.length + '&last_h=' + secH.length, { signal: controller.signal })
            .then(r => r.json()).then(st => {
                clearTimeout(tid);
                if (st.state === -1) {
                    actualizarEstado("ERROR: " + (st.error || "Fallo"), true);
                    clearInterval(statusInterval);
                    resetBtn();
                } else {
                    if (st.state === 1) actualizarEstado("Buscando inicio (Homing)...");
                    else if (st.state === 8) actualizarEstado("Navegando a la zona...");
                    else if (st.state === 9) actualizarEstado("Micromuestreo (Ida)...");
                    else if (st.state === 10) actualizarEstado("Micromuestreo (Vuelta)...");
                    else if (st.state === 4 || (st.state === 0 && cicloActivo && (secIda.length > 5 || secVuelta.length > 5))) {
                        actualizarEstado("Análisis de detalle completado.", false);
                        statusBox.style.borderLeftColor = '#10b981';
                        clearInterval(statusInterval);
                        cicloActivo = false;
                        document.getElementById('btn-section').innerText = "🔙 Vista General";
                        document.getElementById('btn-section').style.background = "#8b5cf6";
                    }

                    if (st.nuevos_h && st.nuevos_h.length > 0) secH.push(...st.nuevos_h);
                    if (st.nuevos && st.nuevos.length > 0) secIda.push(...st.nuevos);
                    if (st.nuevos_v && st.nuevos_v.length > 0) secVuelta.push(...st.nuevos_v);
                    dibujarCurvaSec(secH, secIda, secVuelta);
                }
                isFetching = false;
            }).catch(() => { clearTimeout(tid); isFetching = false; });
        }, 150);
    }).catch(() => {
        actualizarEstado("Error de conexión", true);
        resetBtn();
    });
}

function suavizarTraza(datos) {
    if (!datos || datos.length < 5) return datos || [];
    let res = [];
    for (let i = 0; i < datos.length; i++) {
        if (i < 2 || i >= datos.length - 2) res.push(datos[i]);
        else res.push((datos[i-2] + 2*datos[i-1] + 4*datos[i] + 2*datos[i+1] + datos[i+2]) / 10);
    }
    return res;
}

function dibujarCurvaSec(homing, ida, vuelta) {
    let c = document.getElementById('grafico');
    if (!c) return;
    let ctx = c.getContext('2d');
    let w = c.clientWidth || c.offsetWidth || 360, h = c.clientHeight || c.offsetHeight || 280;
    if (c.width !== w || c.height !== h) { c.width = w; c.height = h; }

    let isLight = document.body.classList.contains('light-theme');
    ctx.fillStyle = isLight ? '#ffffff' : '#0b0f19';
    ctx.fillRect(0, 0, c.width, c.height);

    ctx.strokeStyle = isLight ? '#e2e8f0' : '#1e293b'; 
    ctx.lineWidth = 1;
    for (let i = 1; i < 10; i++) {
        ctx.beginPath(); ctx.moveTo(0, Math.round(c.height * i / 10)); ctx.lineTo(c.width, Math.round(c.height * i / 10)); ctx.stroke();
        ctx.beginPath(); ctx.moveTo(Math.round(c.width * i / 10), 0); ctx.lineTo(Math.round(c.width * i / 10), c.height); ctx.stroke();
    }

    let startFrac = Math.min(markerL, markerR), endFrac = Math.max(markerL, markerR);
    let spanFrac = Math.max(0.001, endFrac - startFrac), adcSpan = Math.max(1, sec_adc_max - sec_adc_min);

    function getZoomX(frac) { return ((frac - startFrac) / spanFrac) * c.width; }
    function getZoomY(adcVal) { return c.height - (((adcVal - sec_adc_min) / adcSpan) * c.height); }

    ctx.globalAlpha = 0.35;
    let sBgData = suavizarTraza(bgData);
    if (sBgData && sBgData.length > 1) {
        ctx.beginPath(); ctx.strokeStyle = '#06b6d4'; ctx.lineWidth = 2;
        let started = false;
        sBgData.forEach((v, i) => {
            let f = i / (sBgData.length - 1);
            if (f >= startFrac - 0.05 && f <= endFrac + 0.05) {
                let x = getZoomX(f), y = getZoomY(v);
                if (!started) { ctx.moveTo(x, y); started = true; } else ctx.lineTo(x, y);
            }
        });
        ctx.stroke();
    }
    ctx.globalAlpha = 1.0;

    let start = Math.round(startFrac * last_max_pos), end = Math.round(endFrac * last_max_pos);
    let totalSecPuntos = Math.max(Math.round((end - start) / 16), 1);

    let sIda = suavizarTraza(ida);
    if (sIda && sIda.length > 0) {
        ctx.beginPath(); ctx.strokeStyle = '#06b6d4'; ctx.lineWidth = 3;
        sIda.forEach((v, i) => {
            let x = Math.min(1, i / totalSecPuntos) * c.width, y = getZoomY(v);
            i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
        });
        ctx.stroke();
    }

    let sVuelta = suavizarTraza(vuelta);
    if (sVuelta && sVuelta.length > 0) {
        ctx.beginPath(); ctx.strokeStyle = '#f97316'; ctx.lineWidth = 3;
        let startPt = (sIda && sIda.length > 0) ? (sIda.length - 1) : totalSecPuntos;
        sVuelta.forEach((v, i) => {
            let x = Math.max(0, (startPt - i) / totalSecPuntos) * c.width, y = getZoomY(v);
            i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
        });
        ctx.stroke();
    }
}

function resetBtn() {
    cicloActivo = false;
    let btn = document.getElementById('btn-ciclo');
    btn.innerText = "▶ Automático";
    btn.style.background = "";
    if (!isSectionMode) dibujarCurva(autoDataH, autoData, autoDataV);
}

function iniciarCiclo() {
    if (cicloActivo) {
        fetch('/stop').catch(() => {});
        clearInterval(statusInterval);
        resetBtn();
        actualizarEstado("Prueba detenida por el usuario.", false);
        return;
    }
    
    isSectionMode = false;
    selectingZone = false;
    document.getElementById('btn-section').innerText = "🔍 Detalle";
    document.getElementById('btn-section').style.background = "#8b5cf6";
    document.getElementById('sectionDesc').innerText = "";
    
    cicloActivo = true;
    let btn = document.getElementById('btn-ciclo');
    btn.innerText = "🛑 Detener";
    btn.style.background = "#ef4444";
    
    actualizarEstado("Iniciando ciclo...");
    autoData = []; autoDataV = []; autoDataH = [];
    pasadasPrevias = [];
    ultimoCicloReportado = 1;
    document.getElementById('qcBox').style.display = 'none';
    
    let n = document.getElementById('p_nombre').value || "NA";
    let m = document.getElementById('p_modelo').value || "NA";
    let mVel = document.getElementById('p_modo_vel').value || "1";
    let isMulti = (mVel === "multi") ? "1" : "0";
    let c = (mVel === "multi") ? "3" : mVel;

    fetch(`/start?n=${encodeURIComponent(n)}&m=${encodeURIComponent(m)}&c=${c}&multi=${isMulti}`)
    .then(r => r.text()).then(res => {
        if (res.toLowerCase().includes("error")) {
            actualizarEstado("ERROR: " + res, true);
            resetBtn();
        } else {
            let isFetching = false;
            statusInterval = setInterval(() => {
                if (isFetching) return;
                isFetching = true;
                let controller = new AbortController();
                let tid = setTimeout(() => controller.abort(), 2500);
                fetch('/status?last=' + autoData.length + '&last_v=' + autoDataV.length + '&last_h=' + autoDataH.length, { signal: controller.signal })
                .then(r => r.json()).then(st => {
                    clearTimeout(tid);
                    if (st.state === -1) {
                        actualizarEstado("ERROR: " + (st.error || "Fallo"), true);
                        clearInterval(statusInterval);
                        resetBtn();
                    } else {
                        if (st.ui) cfg_ui_val = st.ui; if (st.us) cfg_us_val = st.us;
                        
                        if (st.cur_c && st.cur_c > ultimoCicloReportado) {
                            if (autoData.length > 0 || autoDataV.length > 0) {
                                pasadasPrevias.push({ ida: [...autoData], vuelta: [...autoDataV] });
                            }
                            autoData = [];
                            autoDataV = [];
                            ultimoCicloReportado = st.cur_c;
                        }
                        
                        let cicloTxt = (st.tot_c > 1) ? ` [Ciclo ${st.cur_c}/${st.tot_c}${st.vel_lbl ? ' - ' + st.vel_lbl : ''}]` : "";
                        if (st.state === 1) actualizarEstado("Buscando inicio (Homing)..." + cicloTxt);
                        else if (st.state === 2) actualizarEstado("Muestreando curva (Ida)..." + cicloTxt);
                        else if (st.state === 3) { 
                            actualizarEstado("Retornando al origen (Vuelta)..." + cicloTxt); 
                            if (autoData.length > 1) { 
                                last_max_pos = (autoData.length - 1) * 32; 
                                localStorage.setItem("pote_max_pos", last_max_pos); 
                            } 
                        }
                        else if (st.state === 4 || (st.state === 0 && cicloActivo && (autoData.length > 5 || autoDataV.length > 5))) {
                            actualizarEstado("Prueba completada con éxito." + cicloTxt, false);
                            statusBox.style.borderLeftColor = '#10b981';
                            clearInterval(statusInterval);
                            resetBtn();
                            
                            // Mostrar panel de Control de Calidad y Metrología
                            document.getElementById('qcLin').innerText = (st.lin !== undefined ? Number(st.lin).toFixed(2) : "0.00") + "%";
                            document.getElementById('qcHist').innerText = (st.hist !== undefined ? Number(st.hist).toFixed(2) : "0.00") + "%";
                            document.getElementById('qcRuido').innerText = (st.ruido !== undefined ? Number(st.ruido).toFixed(2) : "0.00") + "%";
                            document.getElementById('qcSpan').innerText = (st.min_adc !== undefined ? `${st.min_adc} a ${st.max_adc}` : "---");
                            document.getElementById('qcTipo').innerText = "Tipo: " + (st.tipo || "Lineal B");
                            
                            let isPass = (st.pass === true || st.pass === 1);
                            let vBadge = document.getElementById('qcVerdict');
                            vBadge.className = "pass-badge " + (isPass ? "badge-pass" : "badge-fail");
                            vBadge.innerText = isPass ? "✅ APROBADO (PASS)" : "❌ RECHAZADO (FAIL)";
                            document.getElementById('qcBox').style.display = 'block';

                            if (!st.is_section && autoData.length > 1) { 
                                last_max_pos = (autoData.length - 1) * 32; 
                                localStorage.setItem('pote_max_pos', last_max_pos); 
                            }
                        }
                        
                        if (st.nuevos_h && st.nuevos_h.length > 0) autoDataH.push(...st.nuevos_h);
                        if (st.nuevos && st.nuevos.length > 0) autoData.push(...st.nuevos);
                        if (st.nuevos_v && st.nuevos_v.length > 0) autoDataV.push(...st.nuevos_v);
                        
                        dibujarCurva(autoDataH, autoData, autoDataV);
                    }
                    isFetching = false;
                }).catch(() => { clearTimeout(tid); isFetching = false; });
            }, 150); 
        }
    }).catch(e => { actualizarEstado("Error de conexión", true); resetBtn(); });
}

function dibujarCurva(homing, ida, vuelta) {
    let c = document.getElementById('grafico');
    if (!c) return;
    let ctx = c.getContext('2d');
    let w = c.clientWidth || c.offsetWidth || 360, h = c.clientHeight || c.offsetHeight || 280;
    if (c.width !== w || c.height !== h) { c.width = w; c.height = h; }

    let isLight = document.body.classList.contains('light-theme');
    ctx.fillStyle = isLight ? '#ffffff' : '#0b0f19';
    ctx.fillRect(0, 0, c.width, c.height);

    ctx.strokeStyle = isLight ? '#e2e8f0' : '#1e293b'; 
    ctx.lineWidth = 1;
    for (let i = 1; i < 10; i++) {
        ctx.beginPath(); ctx.moveTo(0, Math.round(c.height * i / 10)); ctx.lineTo(c.width, Math.round(c.height * i / 10)); ctx.stroke();
        ctx.beginPath(); ctx.moveTo(Math.round(c.width * i / 10), 0); ctx.lineTo(Math.round(c.width * i / 10), c.height); ctx.stroke();
    }

    let totalEscala = (last_max_pos > 0) ? Math.max(Math.round(last_max_pos / 32), 1) : 100;
    if (ida && ida.length > totalEscala) totalEscala = ida.length - 1;

    // 0. TRAZAS DE PASADAS PREVIAS (Transparentadas con efecto persistencia)
    if (pasadasPrevias && pasadasPrevias.length > 0) {
        ctx.globalAlpha = 0.28;
        pasadasPrevias.forEach((p) => {
            if (p.ida && p.ida.length > 0) {
                ctx.beginPath(); ctx.strokeStyle = '#06b6d4'; ctx.lineWidth = 2;
                p.ida.forEach((val, i) => {
                    let x = Math.min(1, i / totalEscala) * c.width;
                    let y = c.height - ((val / 1024) * c.height);
                    i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
                });
                ctx.stroke();
            }
            if (p.vuelta && p.vuelta.length > 0) {
                ctx.beginPath(); ctx.strokeStyle = '#f97316'; ctx.lineWidth = 2;
                let startPt = (p.ida && p.ida.length > 0) ? (p.ida.length - 1) : totalEscala;
                p.vuelta.forEach((val, i) => {
                    let x = Math.max(0, (startPt - i) / totalEscala) * c.width;
                    let y = c.height - ((val / 1024) * c.height);
                    i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
                });
                ctx.stroke();
            }
        });
        ctx.globalAlpha = 1.0;
    }
    if (ida && ida.length > totalEscala) totalEscala = ida.length - 1;

    // 1. HOMING (Gris punteado)
    if (homing && homing.length > 0) {
        ctx.beginPath(); ctx.strokeStyle = '#94a3b8'; ctx.lineWidth = 2; ctx.setLineDash([5,5]);
        let frac0 = Math.max(0, Math.min(1, (homing[0] - cfg_ui_val) / Math.max(1, (cfg_us_val - cfg_ui_val))));
        let startH = Math.round(frac0 * totalEscala);
        homing.forEach((val, i) => {
            let x = Math.max(0, (startH - i) / totalEscala) * c.width;
            let y = c.height - ((val / 1024) * c.height);
            i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
        });
        ctx.stroke(); ctx.setLineDash([]);
    }

    // 2. SUBIDA (Azul)
    if (ida && ida.length > 0) {
        ctx.beginPath(); ctx.strokeStyle = '#06b6d4'; ctx.lineWidth = 3;
        ida.forEach((val, i) => {
            let x = Math.min(1, i / totalEscala) * c.width;
            let y = c.height - ((val / 1024) * c.height);
            i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
        });
        ctx.stroke();
    }

    // 3. BAJADA (Naranja)
    if (vuelta && vuelta.length > 0) {
        ctx.beginPath(); ctx.strokeStyle = '#f97316'; ctx.lineWidth = 3;
        let startPt = (ida && ida.length > 0) ? (ida.length - 1) : totalEscala;
        vuelta.forEach((val, i) => {
            let x = Math.max(0, (startPt - i) / totalEscala) * c.width;
            let y = c.height - ((val / 1024) * c.height);
            i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
        });
        ctx.stroke();
    }

    // 4. Líneas de selección
    if (last_max_pos > 0 && selectingZone) {
        let lx = markerL * c.width, rx = markerR * c.width;
        ctx.fillStyle = 'rgba(139,92,246,0.18)';
        ctx.fillRect(lx, 0, rx - lx, c.height);

        ctx.beginPath(); ctx.strokeStyle = '#a78bfa'; ctx.lineWidth = 2; ctx.setLineDash([6,3]);
        ctx.moveTo(lx, 0); ctx.lineTo(lx, c.height); ctx.stroke(); ctx.setLineDash([]);
        ctx.fillStyle = '#a78bfa';
        ctx.beginPath(); ctx.moveTo(lx, 12); ctx.lineTo(lx + 12, 20); ctx.lineTo(lx, 28); ctx.closePath(); ctx.fill();

        ctx.beginPath(); ctx.strokeStyle = '#a78bfa'; ctx.lineWidth = 2; ctx.setLineDash([6,3]);
        ctx.moveTo(rx, 0); ctx.lineTo(rx, c.height); ctx.stroke(); ctx.setLineDash([]);
        ctx.fillStyle = '#a78bfa';
        ctx.beginPath(); ctx.moveTo(rx, 12); ctx.lineTo(rx - 12, 20); ctx.lineTo(rx, 28); ctx.closePath(); ctx.fill();
    }
}

window.addEventListener('load', () => { aplicarTema(localStorage.getItem('pote_theme') || 'dark'); dibujarCurva([], [], []); });
window.addEventListener('resize', () => {
    if (isSectionMode) dibujarCurvaSec([], [], []);
    else dibujarCurva(autoDataH, autoData, autoDataV);
});
dibujarCurva([], [], []);
</script>
</body>
</html>
)=====";

const char HTML_AJUSTES[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Ajustes y Modo Manual</title>
<link rel="icon" type="image/png" href="/icon.png">
<link rel="apple-touch-icon" href="/icon.png">
<meta name="theme-color" content="#8b5cf6">
<style>
:root { --bg-base: #0f1115; --bg-panel: #1a1d24; --bg-input: #252932; --text-main: #e2e8f0; --accent: #8b5cf6; --border: #334155; }
body.light-theme { --bg-base: #e2e8f0; --bg-panel: #f8fafc; --bg-input: #ffffff; --text-main: #0f172a; --border: #cbd5e1; }
body { font-family: system-ui, sans-serif; background: var(--bg-base); color: var(--text-main); margin: 0; padding: 0; display: flex; justify-content: center; align-items: center; min-height: 100vh; }
.dashboard { position: relative; width: 100%; max-width: 480px; min-height: 100vh; display: flex; flex-direction: column; background: var(--bg-panel); padding: 14px 18px; box-sizing: border-box; }
h2 { margin: 0 0 10px 0; color: var(--accent); text-transform: uppercase; font-size: 15px; text-align: center; }
.status-box { background: var(--bg-input); border-left: 4px solid var(--accent); padding: 8px 10px; border-radius: 4px; margin-bottom: 10px; font-family: monospace; font-size: 12px; }
input[type="text"], input[type="number"] { width: 100%; padding: 8px 10px; background: var(--bg-input); border: 1px solid var(--border); color: var(--text-main); border-radius: 6px; box-sizing: border-box; margin-bottom: 8px; font-size: 13px; }
.row { display: flex; gap: 8px; }
button { padding: 10px; font-weight: bold; border: none; border-radius: 6px; cursor: pointer; color: white; text-transform: uppercase; width: 100%; transition: opacity 0.2s; font-size: 11px; }
button:hover { opacity: 0.9; }
.btn-primary { background: var(--accent); }
.btn-back { background: #475569; }
label { font-size: 11px; color: #94a3b8; display: block; margin-bottom: 4px; }
</style>
</head>
<body>
<div class="dashboard">
    <h2>Ajustes y Modo Manual</h2>
    <div class="status-box" id="estado">> Motor libre para giro manual</div>

    <div style="background:var(--bg-input); border:1px solid var(--border); border-radius:8px; padding:10px 12px; margin-bottom:12px;">
        <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:4px;">
            <span style="font-size:11px; font-weight:bold; color:#a78bfa;">🖐️ VISUALIZACIÓN MANUAL EN VIVO</span>
            <span id="live_v" style="font-size:12px; color:#38bdf8; font-family:monospace; font-weight:bold;">0.00 V</span>
        </div>
        <div style="display:flex; align-items:baseline; justify-content:center; gap:6px; margin:4px 0;">
            <span style="font-size:11px; color:#94a3b8;">ADC:</span>
            <span id="live_adc" style="font-size:26px; font-weight:bold; color:#06b6d4; font-family:monospace;">---</span>
            <span style="font-size:11px; color:#94a3b8;">/ 1024</span>
        </div>
        <div style="width:100%; height:12px; background:#0b0f19; border-radius:6px; overflow:hidden; border:1px solid var(--border); margin-bottom:8px;">
            <div id="adc_bar" style="height:100%; width:0%; background:linear-gradient(90deg, #3b82f6, #06b6d4); transition:width 0.08s;"></div>
        </div>
        <canvas id="live_plot" style="width:100%; height:60px; background:#0b0f19; border:1px solid var(--border); border-radius:6px; display:block; margin-bottom:8px;"></canvas>
        <div class="row">
            <button type="button" onclick="setInf()" style="background:#334155; font-size:10px; padding:7px 4px;">📍 Fijar como Límite Inf</button>
            <button type="button" onclick="setSup()" style="background:#334155; font-size:10px; padding:7px 4px;">📍 Fijar como Límite Sup</button>
        </div>
    </div>

    <div class="row">
        <div style="flex: 1;"><label>Umbral Inferior (ADC)</label><input type="number" id="cfg_inf" value="100"></div>
        <div style="flex: 1;"><label>Umbral Superior (ADC)</label><input type="number" id="cfg_sup" value="920"></div>
    </div>
    
    <label style="margin-top:4px;">Tolerancia Control Calidad (Lin. Máxima % para PASS)</label>
    <input type="number" id="cfg_tol" step="0.1" value="2.0">

    <label style="margin-top:4px;">Velocidad del Motor (1: Lento / 10: Rápido)</label>
    <div style="display:flex; align-items:center; gap:12px; margin-bottom:10px;">
        <input type="range" id="cfg_vel" min="1" max="10" value="7" oninput="document.getElementById('vel_txt').innerText = this.value;" style="flex:1; accent-color:#06b6d4; cursor:pointer;">
        <span id="vel_txt" style="font-size:20px; font-weight:bold; color:#06b6d4; min-width:30px; text-align:center;">7</span>
    </div>

    <div class="row" style="margin-top:6px;">
        <button class="btn-back" onclick="window.location.href='/'">🔙 Volver</button>
        <button class="btn-primary" onclick="guardar()">💾 Guardar</button>
    </div>

    <div style="margin-top:14px; border-top:1px solid var(--border); padding-top:10px; text-align:center;">
        <label style="margin-bottom:6px;">Actualización Inalámbrica de Firmware</label>
        <button type="button" onclick="window.location.href='/update'" style="background:#0284c7; font-size:11px; padding:9px;">🚀 Actualizar Firmware (OTA Web)</button>
    </div>
</div>

<script>
let currentADC = 100, adcHistory = [];
function setInf() { document.getElementById('cfg_inf').value = currentADC; }
function setSup() { document.getElementById('cfg_sup').value = currentADC; }

function cargarConfig() {
    fetch('/config_data').then(r => r.json()).then(cfg => {
        document.getElementById('cfg_inf').value = cfg.ui;
        document.getElementById('cfg_sup').value = cfg.us;
        document.getElementById('cfg_vel').value = cfg.vel;
        document.getElementById('vel_txt').innerText = cfg.vel;
        if (cfg.tol !== undefined) document.getElementById('cfg_tol').value = cfg.tol;
    }).catch(() => {});
}

function guardar() {
    let ui = document.getElementById('cfg_inf').value;
    let us = document.getElementById('cfg_sup').value;
    let v  = document.getElementById('cfg_vel').value;
    let tol = document.getElementById('cfg_tol').value;
    document.getElementById('estado').innerText = '> Guardando...';
    fetch(`/save_settings?ui=${ui}&us=${us}&vel=${v}&tol=${tol}`).then(r => r.json()).then(res => {
        document.getElementById('estado').innerText = '> ' + res.msg;
        document.getElementById('estado').style.borderLeftColor = '#10b981';
    });
}

let cvs = document.getElementById('live_plot');
let ctx = cvs ? cvs.getContext('2d') : null;
function drawTrace() {
    if (!ctx) return;
    let w = cvs.clientWidth || cvs.offsetWidth || 300, h = cvs.clientHeight || cvs.offsetHeight || 60;
    if (cvs.width !== w || cvs.height !== h) { cvs.width = w; cvs.height = h; }
    ctx.fillStyle = '#0b0f19'; ctx.fillRect(0, 0, w, h);
    ctx.strokeStyle = '#1e293b'; ctx.lineWidth = 1;
    ctx.beginPath(); ctx.moveTo(0, h/2); ctx.lineTo(w, h/2); ctx.stroke();
    if (adcHistory.length > 1) {
        ctx.beginPath(); ctx.strokeStyle = '#06b6d4'; ctx.lineWidth = 2;
        for (let i = 0; i < adcHistory.length; i++) {
            let x = (i / (adcHistory.length - 1)) * w;
            let y = h - ((adcHistory[i] / 1024) * h);
            i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
        }
        ctx.stroke();
    }
}

if (localStorage.getItem('pote_theme') === 'light') document.body.classList.add('light-theme');
cargarConfig();

setInterval(() => {
    fetch('/adc').then(r => r.text()).then(val => {
        let v = parseInt(val, 10);
        if (!isNaN(v)) {
            currentADC = v;
            document.getElementById('live_adc').innerText = v;
            document.getElementById('live_v').innerText = ((v / 1024) * 3.3).toFixed(2) + ' V';
            let pct = Math.max(0, Math.min(100, (v / 1024) * 100));
            document.getElementById('adc_bar').style.width = pct + '%';
            adcHistory.push(v);
            if (adcHistory.length > 50) adcHistory.shift();
            drawTrace();
        }
    }).catch(() => {});
}, 90);
</script>
</body>
</html>
)=====";

const char HTML_HISTORIAL[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Historial de Registros</title>
    <link rel="icon" type="image/png" href="/icon.png">
    <link rel="apple-touch-icon" href="/icon.png">
    <meta name="theme-color" content="#8b5cf6">
    <style>
        :root { --bg-base: #0f1115; --bg-panel: #1a1d24; --bg-input: #252932; --text-main: #e2e8f0; --accent: #8b5cf6; --border: #334155; }
        body.light-theme { --bg-base: #e2e8f0; --bg-panel: #f8fafc; --bg-input: #ffffff; --text-main: #0f172a; --border: #cbd5e1; }
        * { box-sizing: border-box; }
        body { font-family: system-ui, -apple-system, sans-serif; background: var(--bg-base); color: var(--text-main); margin: 0; padding: 12px; }
        body.light-theme .record { background: #ffffff; border-color: #cbd5e1; }
        body.light-theme .record h4 { color: #0f172a; }
        body.light-theme canvas { background: #ffffff; border-color: #cbd5e1; }
        .container { max-width: 540px; margin: 0 auto; }
        h2 { color: var(--accent); text-align: center; margin: 4px 0 12px 0; font-size: 15px; text-transform: uppercase; letter-spacing: 0.5px; }
        .row { display: flex; gap: 6px; margin-bottom: 10px; }
        button { flex: 1; padding: 9px; border: none; border-radius: 6px; font-weight: bold; cursor: pointer; color: white; text-transform: uppercase; font-size: 11px; }
        button:hover { opacity: 0.9; }
        .btn-back { background: #475569; }
        .btn-export { background: #10b981; }
        .btn-compare { background: #3b82f6; }
        .btn-danger { background: #ef4444; }
        .record { background: var(--bg-panel); padding: 10px 12px; margin-bottom: 8px; border-radius: 8px; border: 1px solid var(--border); cursor: pointer; }
        .record h4 { margin: 0; font-size: 14px; color: #f8fafc; font-weight: 700; }
        canvas { width: 100%; height: 210px; background: #0b0f19; border-radius: 6px; margin-top: 8px; display: none; border: 1px solid var(--border); }
        .action-row { display: flex; gap: 6px; margin-top: 8px; }
        .btn-small { padding: 6px 8px; font-size: 10px; border-radius: 4px; font-weight: bold; }
        .btn-pdf { background: #0ea5e9; color: white; }
        .delete-btn { background: transparent; color: #ef4444; border: 1px solid #ef4444; }
        .delete-btn:hover { background: #ef4444; color: white; }
        #lista { margin-top: 6px; }
        .empty-msg { text-align: center; color: #64748b; font-size: 13px; padding: 30px 0; }
        .badge-lin { background: rgba(56,189,248,0.15); color: #38bdf8; border: 1px solid #0ea5e9; font-weight: bold; font-size: 11px; padding: 2px 6px; border-radius: 4px; }
        .badge-pass { background: rgba(34,197,94,0.15); color: #4ade80; border: 1px solid #22c55e; font-weight: bold; font-size: 11px; padding: 2px 6px; border-radius: 4px; }
        .badge-fail { background: rgba(239,68,68,0.15); color: #f87171; border: 1px solid #ef4444; font-weight: bold; font-size: 11px; padding: 2px 6px; border-radius: 4px; }
        
        /* Modal Comparador */
        #compareModal {
            display: none;
            position: fixed;
            top: 0; left: 0; width: 100vw; height: 100vh;
            background: rgba(0,0,0,0.85);
            z-index: 100;
            padding: 12px;
            box-sizing: border-box;
        }
        .modal-content {
            max-width: 520px;
            background: var(--bg-panel);
            border-radius: 8px;
            border: 1px solid var(--border);
            padding: 14px;
            margin: 20px auto 0 auto;
        }
    </style>
</head>
<body>
    <div class="container">
        <h2>Historial de Registros</h2>
        <div class="row">
            <button class="btn-back" onclick="window.location.href='/'">🔙 Volver</button>
            <button class="btn-compare" onclick="abrirComparador()">📊 Comparar</button>
            <button class="btn-export" onclick="window.location.href='/exportar_csv'">⬇️ CSV</button>
            <button class="btn-danger" onclick="borrarTodo()">🗑 Vaciar</button>
        </div>
        <div id="lista">Cargando registros...</div>
    </div>

    <!-- Modal Comparador Multi-Curva -->
    <div id="compareModal">
        <div class="modal-content">
            <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:8px;">
                <h3 style="margin:0; font-size:14px; color:#38bdf8;">Comparación Multi-Curva</h3>
                <button style="width:auto; padding:4px 8px; background:#475569;" onclick="cerrarComparador()">✕ Cerrar</button>
            </div>
            <canvas id="compareCanvas" style="display:block; width:100%; height:240px; margin-bottom:8px;"></canvas>
            <div id="compareLegend" style="font-size:11px; display:flex; flex-wrap:wrap; gap:8px;"></div>
        </div>
    </div>

    <script>
        var historial = [];

        function cargar() {
            fetch('/historial.json')
            .then(function(r) { return r.json(); })
            .then(function(data) {
                historial = Array.isArray(data) ? data : [];
                historial.sort(function(a, b) { return (a.file_id || 0) - (b.file_id || 0); });
                render();
            })
            .catch(function(e) {
                document.getElementById('lista').innerHTML = "<div class='empty-msg'>No hay registros guardados.</div>";
            });
        }
        
        function render() {
            var lista = document.getElementById('lista');
            lista.innerHTML = "";
            if (!historial || historial.length === 0) {
                lista.innerHTML = "<div class='empty-msg'>No hay registros guardados en el dispositivo.</div>";
                return;
            }
            for (var i = 0; i < historial.length; i++) {
                var item = historial[i];
                var pts_ida = item.curva || item.datos || [];
                var pts_vuelta = item.vuelta || [];
                
                function calcLinBestFit(d) {
                    if (!d || d.length < 5) return 0.0;
                    var s_i = (d.length > 10 && Math.abs(d[0] - d[1]) > 25) ? 1 : 0;
                    var e_i = (d.length > 10 && Math.abs(d[d.length - 1] - d[d.length - 2]) > 25) ? (d.length - 2) : (d.length - 1);
                    var n = e_i - s_i + 1;
                    var sx = 0, sy = 0, sxy = 0, sx2 = 0;
                    for (var i = 0; i < n; i++) {
                        var x = i / (n - 1), y = d[s_i + i];
                        sx += x; sy += y; sxy += (x * y); sx2 += (x * x);
                    }
                    var xm = sx / n, ym = sy / n;
                    var den = sx2 - (sx * xm);
                    if (Math.abs(den) < 0.0001) den = 1.0;
                    var m = (sxy - (sx * ym)) / den, b = ym - (m * xm);
                    var rng = Math.abs(m) || Math.abs(d[e_i] - d[s_i]) || 1;
                    var max_d = 0;
                    for (var i = 0; i < n; i++) {
                        var ideal = m * (i / (n - 1)) + b;
                        var dev = Math.abs(d[s_i + i] - ideal);
                        if (dev > max_d) max_d = dev;
                    }
                    return ((max_d / rng) * 100).toFixed(2);
                }

                function calcRuidoCRV(d) {
                    if (!d || d.length < 5) return "0.00";
                    var s_i = 1, e_i = d.length - 2;
                    var sum = 0, cnt = 0;
                    var rng = Math.abs(d[d.length - 1] - d[0]) || 1;
                    for (var i = s_i; i <= e_i; i++) {
                        var exp = (d[i - 1] + d[i + 1]) / 2;
                        sum += Math.abs(d[i] - exp);
                        cnt++;
                    }
                    return cnt > 0 ? (((sum / cnt) / rng) * 100).toFixed(2) : "0.00";
                }

                function calcHyst(ida, vta) {
                    if (!ida || !vta || ida.length < 5 || vta.length < 5) return "0.00";
                    var max_d = 0;
                    var rng = Math.abs(ida[ida.length - 1] - ida[0]) || 1;
                    for (var i = 0; i < ida.length; i++) {
                        var idx_v = Math.round((1 - (i / (ida.length - 1))) * (vta.length - 1));
                        idx_v = Math.max(0, Math.min(vta.length - 1, idx_v));
                        var diff = Math.abs(ida[i] - vta[idx_v]);
                        if (diff > max_d) max_d = diff;
                    }
                    return ((max_d / rng) * 100).toFixed(2);
                }

                var linVal = (item.linealidad !== undefined && item.linealidad !== null) ? Number(item.linealidad).toFixed(2) + "%" : calcLinBestFit(pts_ida) + "%";
                var histVal = (item.histeresis !== undefined && item.histeresis !== null && Number(item.histeresis) > 0) ? Number(item.histeresis).toFixed(2) + "%" : calcHyst(pts_ida, pts_vuelta) + "%";
                var ruidoVal = (item.ruido !== undefined && item.ruido !== null && Number(item.ruido) > 0) ? Number(item.ruido).toFixed(2) + "%" : calcRuidoCRV(pts_ida) + "%";
                var pts_ida = item.curva || item.datos || [];
                var pts_vuelta = item.vuelta || [];
                var isPass = (item.pass === true || item.pass === 1);
                
                var div = document.createElement('div');
                div.className = 'record';
                div.innerHTML = "<div style='display:flex; justify-content:space-between; align-items:center;'>" +
                    "<div style='display:flex; align-items:center; gap:8px;'>" +
                    "<input type='checkbox' class='cmp-check' data-idx='" + i + "' onclick='event.stopPropagation()' style='accent-color:#3b82f6; width:16px; height:16px; cursor:pointer;'>" +
                    "<h4>" + (item.id || item.nombre || ("Pote #" + (i + 1))) + "</h4>" +
                    "</div>" +
                    "<div>" +
                    "<span class='" + (isPass ? "badge-pass" : "badge-fail") + "' style='margin-right:4px;'>" + (isPass ? "PASS" : "FAIL") + "</span>" +
                    "<span class='badge-lin'>Lin: " + linVal + "</span>" +
                    "</div></div>" +
                    "<div style='display:flex; justify-content:space-between; margin:6px 0 0 0; font-size:11px; color:#94a3b8;'>" +
                    "<span>Mod: <b style='color:#a855f7;'>" + (item.modelo || "General") + "</b> (" + (item.tipo || "Lineal B") + ")</span>" +
                    "<span>Hyst: <b>" + histVal + "</b> | CRV: <b>" + ruidoVal + "</b></span>" +
                    "</div>" +
                    "<canvas id='canvas-" + i + "'></canvas>" +
                    "<div class='action-row'>" +
                    "<button class='btn-small' style='background:#8b5cf6;' onclick='editarNombre(" + item.file_id + ", \"" + (item.id || "") + "\", event)'>✏️ Renombrar</button>" +
                    "<button class='btn-small btn-pdf' onclick='imprimirFicha(" + i + ", event)'>🖨️ Ficha / PDF</button>" +
                    "<button class='btn-small delete-btn' onclick='borrarUno(" + item.file_id + ", event)'>🗑 Eliminar</button>" +
                    "</div>";
                div.onclick = (function(idx, d, v) {
                    return function(e) { 
                        if (e.target.tagName !== 'BUTTON' && e.target.tagName !== 'INPUT') toggleCanvas(idx, d, v); 
                    };
                })(i, pts_ida, pts_vuelta);
                lista.appendChild(div);
            }
        }
        
        function toggleCanvas(index, datos, vuelta) {
            var canvas = document.getElementById("canvas-" + index);
            if (!canvas) return;
            if (canvas.style.display === "block") {
                canvas.style.display = "none";
            } else {
                canvas.style.display = "block";
                dibujarCurvaHistorial(canvas, datos, vuelta);
            }
        }
        
        function dibujarCurvaHistorial(c, datos, vuelta) {
            var ctx = c.getContext("2d");
            var dpr = window.devicePixelRatio || 1;
            var w = c.clientWidth || c.offsetWidth || 340, h = c.clientHeight || c.offsetHeight || 210;
            c.width = w * dpr; c.height = h * dpr;
            ctx.scale(dpr, dpr);
            
            var isLight = document.body.classList.contains('light-theme');
            ctx.fillStyle = isLight ? "#ffffff" : "#0b0f19";
            ctx.fillRect(0, 0, w, h);
            
            ctx.strokeStyle = isLight ? "#e2e8f0" : "#1e293b"; ctx.lineWidth = 1;
            for (var i = 1; i < 10; i++) {
                ctx.beginPath(); ctx.moveTo(0, Math.round(h * i / 10)); ctx.lineTo(w, Math.round(h * i / 10)); ctx.stroke();
                ctx.beginPath(); ctx.moveTo(Math.round(w * i / 10), 0); ctx.lineTo(Math.round(w * i / 10), h); ctx.stroke();
            }
            
            if (datos && datos.length > 1) {
                ctx.beginPath(); ctx.strokeStyle = "#06b6d4"; ctx.lineWidth = 2.5; ctx.lineJoin = "round";
                for (var i = 0; i < datos.length; i++) {
                    var x = (i / (datos.length - 1)) * w, y = h - ((datos[i] / 1024) * h);
                    i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
                }
                ctx.stroke();
            }
            
            if (vuelta && vuelta.length > 1) {
                ctx.beginPath(); ctx.strokeStyle = "#f97316"; ctx.lineWidth = 2.5; ctx.lineJoin = "round";
                for (var i = 0; i < vuelta.length; i++) {
                    var x = (1 - (i / (vuelta.length - 1))) * w, y = h - ((vuelta[i] / 1024) * h);
                    i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
                }
                ctx.stroke();
            }
        }

        // Comparador Multi-Curva
        const PALETTE = ['#38bdf8', '#f43f5e', '#10b981', '#a855f7', '#fbbf24', '#ec4899', '#14b8a6'];
        function abrirComparador() {
            var checks = document.querySelectorAll('.cmp-check:checked');
            if (checks.length < 2) {
                alert("Selecciona al menos 2 curvas con los casilleros de verificación para comparar.");
                return;
            }
            var selectedIdxs = [];
            checks.forEach(function(ck) { selectedIdxs.push(parseInt(ck.getAttribute('data-idx'), 10)); });
            
            var modal = document.getElementById('compareModal');
            modal.style.display = 'block';
            
            var c = document.getElementById('compareCanvas');
            var ctx = c.getContext('2d');
            var dpr = window.devicePixelRatio || 1;
            var w = c.clientWidth || c.offsetWidth || 480, h = c.clientHeight || c.offsetHeight || 240;
            c.width = w * dpr; c.height = h * dpr;
            ctx.scale(dpr, dpr);
            
            ctx.fillStyle = '#0b0f19'; ctx.fillRect(0, 0, w, h);
            ctx.strokeStyle = '#1e293b'; ctx.lineWidth = 1;
            for (var i = 1; i < 10; i++) {
                ctx.beginPath(); ctx.moveTo(0, Math.round(h * i / 10)); ctx.lineTo(w, Math.round(h * i / 10)); ctx.stroke();
                ctx.beginPath(); ctx.moveTo(Math.round(w * i / 10), 0); ctx.lineTo(Math.round(w * i / 10), h); ctx.stroke();
            }
            
            var legend = document.getElementById('compareLegend');
            legend.innerHTML = "";
            
            selectedIdxs.forEach(function(sIdx, colorIdx) {
                var item = historial[sIdx];
                var color = PALETTE[colorIdx % PALETTE.length];
                var pts = item.curva || item.datos || [];
                
                if (pts.length > 1) {
                    ctx.beginPath(); ctx.strokeStyle = color; ctx.lineWidth = 2.5;
                    for (var i = 0; i < pts.length; i++) {
                        var x = (i / (pts.length - 1)) * w, y = h - ((pts[i] / 1024) * h);
                        i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
                    }
                    ctx.stroke();
                }
                
                legend.innerHTML += "<span style='color:" + color + "; font-weight:bold;'>■ " + item.id + " (Lin: " + Number(item.linealidad||0).toFixed(2) + "%)</span>";
            });
        }
        function cerrarComparador() { document.getElementById('compareModal').style.display = 'none'; }

        // Impresión / Ficha Técnica PDF
        function imprimirFicha(index, event) {
            if (event) event.stopPropagation();
            var item = historial[index];
            var pts = item.curva || item.datos || [];
            var isPass = (item.pass === true || item.pass === 1);
            
            var win = window.open('', '_blank');
            win.document.write("<!DOCTYPE html><html><head><title>Ficha Técnica - " + item.id + "</title><style>" +
                "body { font-family: system-ui, sans-serif; padding: 25px; color: #0f172a; max-width: 650px; margin: 0 auto; }" +
                "h2 { margin: 0; color: #6d28d9; text-transform: uppercase; }" +
                ".header { display: flex; justify-content: space-between; align-items: center; border-bottom: 2px solid #e2e8f0; padding-bottom: 12px; margin-bottom: 16px; }" +
                ".table { width: 100%; border-collapse: collapse; margin-bottom: 20px; font-size: 13px; }" +
                ".table td, .table th { border: 1px solid #cbd5e1; padding: 8px 12px; text-align: left; }" +
                ".table th { background: #f1f5f9; }" +
                ".badge { font-weight: bold; padding: 3px 8px; border-radius: 4px; font-size: 12px; }" +
                ".pass { background: #dcfce7; color: #15803d; border: 1px solid #22c55e; }" +
                ".fail { background: #fee2e2; color: #b91c1c; border: 1px solid #ef4444; }" +
                "canvas { width: 100%; height: 260px; background: #ffffff; border: 1px solid #cbd5e1; border-radius: 6px; }" +
                "</style></head><body>" +
                "<div class='header'><div><h2>Certificado de Ensayo</h2><p style='margin:4px 0 0 0; color:#64748b; font-size:12px;'>Banco de Pruebas Automatizado</p></div>" +
                "<div><span class='badge " + (isPass ? "pass" : "fail") + "'>" + (isPass ? "✅ APROBADO (PASS)" : "❌ RECHAZADO (FAIL)") + "</span></div></div>" +
                "<table class='table'>" +
                "<tr><th>Identificador / ID</th><td>" + item.id + "</td><th>Modelo / Parte</th><td>" + item.modelo + "</td></tr>" +
                "<tr><th>Tipo de Pista</th><td>" + (item.tipo || "Lineal B") + "</td><th>Linealidad (Max Error)</th><td><b>" + Number(item.linealidad||0).toFixed(2) + "%</b></td></tr>" +
                "<tr><th>Histéresis Mecánica</th><td>" + Number(item.histeresis||0).toFixed(2) + "%</td><th>Ruido de Contacto (CRV)</th><td>" + Number(item.ruido||0).toFixed(2) + "%</td></tr>" +
                "</table>" +
                "<canvas id='pdfCanvas'></canvas>" +
                "<script>" +
                "var c = document.getElementById('pdfCanvas'); var ctx = c.getContext('2d');" +
                "c.width = 600; c.height = 260;" +
                "ctx.fillStyle = '#ffffff'; ctx.fillRect(0,0,600,260);" +
                "ctx.strokeStyle = '#e2e8f0'; ctx.lineWidth = 1;" +
                "for(var i=1; i<10; i++){ ctx.beginPath(); ctx.moveTo(0, 26*i); ctx.lineTo(600, 26*i); ctx.stroke(); ctx.beginPath(); ctx.moveTo(60*i, 0); ctx.lineTo(60*i, 260); ctx.stroke(); }" +
                "var pts = " + JSON.stringify(pts) + ";" +
                "if(pts.length>1){ ctx.beginPath(); ctx.strokeStyle = '#0284c7'; ctx.lineWidth = 3;" +
                "for(var i=0; i<pts.length; i++){ var x = (i/(pts.length-1))*600; var y = 260 - ((pts[i]/1024)*260); i===0?ctx.moveTo(x,y):ctx.lineTo(x,y); } ctx.stroke(); }" +
                "setTimeout(function(){ window.print(); }, 400);" +
                "</scr" + "ipt></body></html>");
            win.document.close();
        }
        
        function editarNombre(file_id, nombreActual, event) {
            if (event) event.stopPropagation();
            var nuevo = prompt("Ingrese el nuevo nombre para este potenciómetro:", nombreActual);
            if (nuevo !== null) {
                nuevo = nuevo.trim();
                if (nuevo.length > 0) {
                    fetch("/renombrar_registro?file_id=" + file_id + "&nombre=" + encodeURIComponent(nuevo))
                    .then(function() { cargar(); })
                    .catch(function() { cargar(); });
                }
            }
        }
        
        function borrarUno(file_id, event) {
            if (event) event.stopPropagation();
            if (confirm("¿Deseas eliminar este registro del dispositivo?")) {
                document.getElementById('lista').innerHTML = "<div class='empty-msg'>Eliminando registro...</div>";
                fetch("/borrar_registro?file_id=" + file_id)
                .then(function() { setTimeout(cargar, 150); })
                .catch(function() { setTimeout(cargar, 150); });
            }
        }
        
        function borrarTodo() {
            if (confirm("¿Deseas vaciar TODO el historial de registros?")) {
                document.getElementById("lista").innerHTML = "<div class='empty-msg'>Vaciando historial...</div>";
                fetch("/borrar_historial")
                .then(function() { historial = []; render(); })
                .catch(function() { historial = []; render(); });
            }
        }
        
        if (localStorage.getItem('pote_theme') === 'light') document.body.classList.add('light-theme');
        cargar();
    </script>
</body>
</html>
)=====";

const char MANIFEST_JSON[] PROGMEM = R"=====(
{
  "name": "Probador de Potenciómetros",
  "short_name": "Probador",
  "start_url": "/",
  "display": "standalone",
  "background_color": "#0f1115",
  "theme_color": "#8b5cf6",
  "icons": [
    {
      "src": "/icon.png",
      "sizes": "192x192 512x512",
      "type": "image/png",
      "purpose": "any maskable"
    }
  ]
}
)=====";

// ==========================================
// 3. FUNCIONES DE CONTROL MECÁNICO Y ADC
// ==========================================
int leerADC_filtrado() {
    int buf[17];
    for (int i = 0; i < 17; i++) {
        buf[i] = analogRead(A0);
        delayMicroseconds(20);
    }
    
    // Filtro Mediana por inserción (17 muestras)
    for (int i = 1; i < 17; i++) {
        int key = buf[i];
        int j = i - 1;
        while (j >= 0 && buf[j] > key) {
            buf[j + 1] = buf[j];
            j--;
        }
        buf[j + 1] = key;
    }
    
    long suma = 0;
    for (int i = 5; i < 12; i++) suma += buf[i];
    return (int)(suma / 7);
}

void apagar_motor() {
    for (int i = 0; i < 4; i++) digitalWrite(motor_pins[i], LOW);
}

int current_step = 0;

void mover_motor(long pasos, int direccion, int delay_us) {
    for (long p = 0; p < pasos; p++) {
        if (direccion == 1) current_step = (current_step + 1) & 7;
        else current_step = (current_step - 1) & 7;
        
        for (int i = 0; i < 4; i++) {
            digitalWrite(motor_pins[i], step_sequence[current_step][i]);
        }
        delayMicroseconds(delay_us);
        if ((p & 31) == 0) yield();
    }
}

void cargar_config() {
    if (LittleFS.exists("/config.json")) {
        File f = LittleFS.open("/config.json", "r");
        if (f) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, f);
            if (!error) {
                cfg_ui = doc["ui"] | 100;
                cfg_us = doc["us"] | 920;
                cfg_vel = doc["vel"] | 7;
                cfg_tol_lin = doc["tol"] | 2.0;
                cfg_delay = calcular_delay_us(cfg_vel);
            }
            f.close();
        }
    } else {
        cfg_delay = calcular_delay_us(cfg_vel);
    }
}

// ==========================================
// ESTADOS DEL SISTEMA Y METROLOGÍA
// ==========================================
int obtener_siguiente_id_pote() {
    int next_id = 1;
    if (LittleFS.exists("/next_id.txt")) {
        File f = LittleFS.open("/next_id.txt", "r");
        if (f) {
            next_id = f.readStringUntil('\n').toInt();
            f.close();
        }
    }
    if (next_id < 1) next_id = 1;
    File f = LittleFS.open("/next_id.txt", "w");
    if (f) {
        f.println(next_id + 1);
        f.close();
    }
    return next_id;
}

int system_state = 0; // 0=IDLE, 1=HOMING, 2=SAMPLING, 3=RETURNING, 4=DONE, -1=ERROR
String error_msg = "";
long sys_pasos = 0;
long pos_absoluta = 0;
long pos_maxima = 1;
int lec_inicial = 0;
int lec_act = 0, lec_ant = 0;
std::vector<int> datos_ida;
std::vector<int> datos_vuelta;
std::vector<int> datos_homing;
long sweep_start = 0;
long sweep_end = 0;
bool is_section_sweep = false;
String current_nombre = "";
String current_modelo = "";

// Variables de Multi-Ciclo y Multi-Velocidad
int total_ciclos_req = 1;
int ciclo_actual = 1;
bool is_multi_speed = false;
int delay_ciclo_actual = 1466;
String label_vel_actual = "";

float last_lin = 0.0, last_hist = 0.0, last_ruido = 0.0;
String last_tipo = "Lineal (B)";
bool last_pass = true;
int last_min_adc = 0, last_max_adc = 0;

void actualizar_velocidad_ciclo() {
    if (is_multi_speed) {
        if (ciclo_actual == 1) {
            delay_ciclo_actual = calcular_delay_us(3); // Lenta
            label_vel_actual = "Lenta (Nivel 3)";
        } else if (ciclo_actual == 2) {
            delay_ciclo_actual = calcular_delay_us(7); // Media
            label_vel_actual = "Media (Nivel 7)";
        } else {
            delay_ciclo_actual = calcular_delay_us(10); // Rápida
            label_vel_actual = "Rápida (Nivel 10)";
        }
    } else {
        delay_ciclo_actual = cfg_delay;
        label_vel_actual = "Nivel " + String(cfg_vel);
    }
}

void calcular_metrologia_completa(float &lin, float &hist, float &ruido, String &tipo, bool &pass, int &min_adc, int &max_adc) {
    int tam_ida = datos_ida.size();
    int tam_vta = datos_vuelta.size();
    
    if (tam_ida < 5) {
        lin = 0.0; hist = 0.0; ruido = 0.0; tipo = "N/A"; pass = false; min_adc = 0; max_adc = 0;
        return;
    }
    
    // Descartar posibles transitorios de arranque mecánicos iniciales/finales (primeros 2 puntos si hay rebote)
    int start_i = (tam_ida > 10 && abs(datos_ida[0] - datos_ida[1]) > 25) ? 1 : 0;
    int end_i = (tam_ida > 10 && abs(datos_ida[tam_ida - 1] - datos_ida[tam_ida - 2]) > 25) ? (tam_ida - 2) : (tam_ida - 1);
    
    min_adc = datos_ida[start_i];
    max_adc = datos_ida[end_i];
    int rango_bruto = abs(max_adc - min_adc);
    if (rango_bruto == 0) rango_bruto = 1;
    
    // 1. Linealidad Independiente Industrial (Regresión por Mínimos Cuadrados / Best-Fit Line)
    int n_pts = end_i - start_i + 1;
    float sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
    for (int i = 0; i < n_pts; i++) {
        float x = (float)i / (float)(n_pts - 1);
        float y = (float)datos_ida[start_i + i];
        sum_x += x;
        sum_y += y;
        sum_xy += (x * y);
        sum_x2 += (x * x);
    }
    float x_mean = sum_x / n_pts;
    float y_mean = sum_y / n_pts;
    float denom = sum_x2 - (sum_x * x_mean);
    if (abs(denom) < 0.0001) denom = 1.0;
    float slope = (sum_xy - (sum_x * y_mean)) / denom;
    float intercept = y_mean - (slope * x_mean);
    float rango_ajuste = abs(slope);
    if (rango_ajuste < 1.0) rango_ajuste = (float)rango_bruto;

    float max_dev = 0;
    for (int i = 0; i < n_pts; i++) {
        float x = (float)i / (float)(n_pts - 1);
        float ideal = (slope * x) + intercept;
        float dev = abs((float)datos_ida[start_i + i] - ideal);
        if (dev > max_dev) max_dev = dev;
    }
    lin = (max_dev / rango_ajuste) * 100.0;
    
    // 2. Histéresis Robusta (Diferencia máxima entre Ida y Vuelta en zona útil)
    float max_hyst_diff = 0;
    if (tam_vta >= 5) {
        int v_start = (tam_vta > 10 && abs(datos_vuelta[0] - datos_vuelta[1]) > 25) ? 1 : 0;
        int v_end = (tam_vta > 10 && abs(datos_vuelta[tam_vta - 1] - datos_vuelta[tam_vta - 2]) > 25) ? (tam_vta - 2) : (tam_vta - 1);
        int n_vta = v_end - v_start + 1;
        
        for (int i = 0; i < n_pts; i++) {
            float frac = (float)i / (float)(n_pts - 1);
            int idx_vta = v_start + round((1.0 - frac) * (n_vta - 1));
            idx_vta = constrain(idx_vta, v_start, v_end);
            float diff = abs((float)datos_ida[start_i + i] - (float)datos_vuelta[idx_vta]);
            if (diff > max_hyst_diff) max_hyst_diff = diff;
        }
    }
    hist = (max_hyst_diff / rango_ajuste) * 100.0;
    
    // 3. Ruido de Contacto / CRV (Variación de 2da derivada punto a punto)
    float suma_ruido = 0;
    int count_ruido = 0;
    for (int i = start_i + 1; i < end_i; i++) {
        float expected = (datos_ida[i - 1] + datos_ida[i + 1]) / 2.0;
        suma_ruido += abs(datos_ida[i] - expected);
        count_ruido++;
    }
    ruido = count_ruido > 0 ? ((suma_ruido / (float)count_ruido) / rango_ajuste) * 100.0 : 0.0;
    
    // 4. Tipo de Curva (Taper A / B / C por valor en 50% de carrera)
    int mid_idx = start_i + (n_pts / 2);
    float mid_pct = ((float)(datos_ida[mid_idx] - min_adc) / (float)rango_bruto) * 100.0;
    if (mid_pct >= 38.0 && mid_pct <= 62.0) tipo = "Lineal (B)";
    else if (mid_pct < 38.0) tipo = "Log / Audio (A)";
    else tipo = "Antilog (C)";
    
    // 5. Criterio PASS / FAIL
    pass = (lin <= cfg_tol_lin && ruido <= 2.0);
}

void guardar_historial(String nombre, String modelo) {
    if (is_section_sweep) return;
    if (datos_ida.size() < 5) return;
    
    nombre.replace("\"", ""); nombre.replace("\\", ""); nombre.replace("\n", ""); nombre.replace("\r", ""); nombre.trim();
    modelo.replace("\"", ""); modelo.replace("\\", ""); modelo.replace("\n", ""); modelo.replace("\r", ""); modelo.trim();

    int rec_id = obtener_siguiente_id_pote();
    if (nombre.length() == 0 || nombre == "NA" || nombre == "Sin Nombre") {
        nombre = "Pote #" + String(rec_id);
    }
    if (modelo.length() == 0 || modelo == "NA") {
        modelo = "General";
    }
    if (is_multi_speed) {
        modelo += " [3-Velocidades]";
    }
    
    calcular_metrologia_completa(last_lin, last_hist, last_ruido, last_tipo, last_pass, last_min_adc, last_max_adc);

    String path = "/r_" + String(rec_id) + ".json";
    File f = LittleFS.open(path, "w");
    if (f) {
        f.print("{\"file_id\":");
        f.print(rec_id);
        f.print(",\"id\":\"");
        f.print(nombre);
        f.print("\",\"modelo\":\"");
        f.print(modelo);
        f.print("\",\"linealidad\":");
        f.print(last_lin, 2);
        f.print(",\"histeresis\":");
        f.print(last_hist, 2);
        f.print(",\"ruido\":");
        f.print(last_ruido, 2);
        f.print(",\"tipo\":\"");
        f.print(last_tipo);
        f.print("\",\"pass\":");
        f.print(last_pass ? "true" : "false");
        f.print(",\"min_adc\":");
        f.print(last_min_adc);
        f.print(",\"max_adc\":");
        f.print(last_max_adc);
        f.print(",\"curva\":[");
        for (size_t i = 0; i < datos_ida.size(); i++) {
            if (i > 0) f.print(",");
            f.print(datos_ida[i]);
        }
        f.print("],\"vuelta\":[");
        for (size_t i = 0; i < datos_vuelta.size(); i++) {
            if (i > 0) f.print(",");
            f.print(datos_vuelta[i]);
        }
        f.println("]}");
        f.flush();
        f.close();
    }
}

// ==========================================
// 4. CONTROLADORES DE ENDPOINTS HTTP
// ==========================================
void handleNotFound() {
    String html = "<!DOCTYPE html><html><head><meta http-equiv='refresh' content='0;url=http://192.168.4.1/'><script>window.location.href='http://192.168.4.1/';</script></head><body><p>Redirigiendo al probador... <a href='http://192.168.4.1/'>Click aqui</a></p></body></html>";
    server.sendHeader("Location", "http://192.168.4.1/", true);
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.send(302, "text/html", html);
}

void handleHome() {
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.send_P(200, "text/html", HTML_HOME);
}

void handleStop() {
    system_state = -1;
    error_msg = "Prueba cancelada por el usuario";
    apagar_motor();
    server.send(200, "text/plain", "OK");
}

void handleSectionSweep() {
    if (system_state != 0 && system_state != 4 && system_state != -1) {
        server.send(400, "application/json", "{\"error\":\"El sistema ya esta corriendo\"}");
        return;
    }
    sweep_start = server.hasArg("start") ? server.arg("start").toInt() : 0;
    sweep_end = server.hasArg("end") ? server.arg("end").toInt() : 0;
    is_section_sweep = true;
    total_ciclos_req = 1;
    ciclo_actual = 1;
    is_multi_speed = false;
    actualizar_velocidad_ciclo();
    
    apagar_motor();
    sys_pasos = 0;
    pos_absoluta = 0;
    pos_maxima = 1;
    datos_ida.clear();
    datos_vuelta.clear();
    datos_homing.clear();
    
    system_state = 1; // Homing
    server.send(200, "text/plain", "Iniciando");
}

void handleStart() {
    if (system_state != 0 && system_state != 4 && system_state != -1) {
        server.send(400, "application/json", "{\"error\":\"El sistema ya esta corriendo\"}");
        return;
    }
    current_nombre = server.hasArg("n") ? server.arg("n") : "";
    current_nombre.trim();
    if (current_nombre.length() == 0 || current_nombre == "NA" || current_nombre == "Sin Nombre") {
        current_nombre = "Pote #" + String(obtener_siguiente_id_pote());
    }
    
    current_modelo = server.hasArg("m") ? server.arg("m") : "";
    current_modelo.trim();
    if (current_modelo.length() == 0 || current_modelo == "NA") {
        current_modelo = "General";
    }
    
    is_multi_speed = (server.hasArg("multi") && server.arg("multi") == "1");
    total_ciclos_req = server.hasArg("c") ? constrain(server.arg("c").toInt(), 1, 50) : 1;
    if (is_multi_speed) total_ciclos_req = 3;
    ciclo_actual = 1;
    actualizar_velocidad_ciclo();
    
    datos_ida.clear();
    datos_vuelta.clear();
    datos_homing.clear();
    error_msg = "";
    sys_pasos = 0;
    pos_absoluta = 0;
    pos_maxima = 1;
    is_section_sweep = false;
    lec_inicial = leerADC_filtrado();
    
    system_state = 1; // HOMING
    server.send(200, "text/plain", "Iniciando");
}

void handleStatus() {
    JsonDocument doc;
    doc["state"] = system_state;
    if (error_msg.length() > 0) doc["error"] = error_msg;
    doc["adc"] = (system_state == 0 || system_state == 4 || system_state == -1) ? leerADC_filtrado() : lec_act;
    doc["pos"] = pos_absoluta;
    doc["max_pos"] = pos_maxima;
    doc["is_section"] = is_section_sweep;
    doc["ui"] = cfg_ui;
    doc["us"] = cfg_us;
    doc["cur_c"] = ciclo_actual;
    doc["tot_c"] = total_ciclos_req;
    doc["vel_lbl"] = label_vel_actual;
    
    if (system_state == 4 || (system_state == 0 && (datos_ida.size() > 5 || datos_vuelta.size() > 5))) {
        calcular_metrologia_completa(last_lin, last_hist, last_ruido, last_tipo, last_pass, last_min_adc, last_max_adc);
        doc["lin"] = last_lin;
        doc["hist"] = last_hist;
        doc["ruido"] = last_ruido;
        doc["tipo"] = last_tipo;
        doc["pass"] = last_pass;
        doc["min_adc"] = last_min_adc;
        doc["max_adc"] = last_max_adc;
    }
    
    if (system_state >= 1) {
        int last = server.hasArg("last") ? server.arg("last").toInt() : 0;
        int last_v = server.hasArg("last_v") ? server.arg("last_v").toInt() : 0;
        int last_h = server.hasArg("last_h") ? server.arg("last_h").toInt() : 0;
        
        JsonArray nuevos = doc.createNestedArray("nuevos");
        int tam = datos_ida.size();
        for (int i = last; i < tam; i++) nuevos.add(datos_ida[i]);
        
        JsonArray nuevos_v = doc.createNestedArray("nuevos_v");
        int tam_v = datos_vuelta.size();
        for (int i = last_v; i < tam_v; i++) nuevos_v.add(datos_vuelta[i]);
        
        JsonArray nuevos_h = doc.createNestedArray("nuevos_h");
        int tam_h = datos_homing.size();
        for (int i = last_h; i < tam_h; i++) nuevos_h.add(datos_homing[i]);
    }
    String out;
    serializeJson(doc, out);
    server.send(200, "application/json", out);
}

void handleADC() {
    server.send(200, "text/plain", String(leerADC_filtrado()));
}

void handleConfigData() {
    char json[96];
    snprintf(json, sizeof(json), "{\"ui\":%d,\"us\":%d,\"vel\":%d,\"tol\":%.1f}", cfg_ui, cfg_us, cfg_vel, cfg_tol_lin);
    server.send(200, "application/json", json);
}

void handleManifest() {
    server.send_P(200, "application/json", MANIFEST_JSON);
}

void handleLogo() {
    server.send_P(200, "image/png", (const char*)LOGO_PNG, sizeof(LOGO_PNG));
}

void handleIcon() {
    server.send_P(200, "image/png", (const char*)ICON_PNG, sizeof(ICON_PNG));
}

void handleAjustes() {
    apagar_motor();
    if (system_state != 0 && system_state != 4) system_state = 0;
    server.send_P(200, "text/html", HTML_AJUSTES);
}

void handleSaveSettings() {
    cfg_ui = server.hasArg("ui") ? server.arg("ui").toInt() : cfg_ui;
    cfg_us = server.hasArg("us") ? server.arg("us").toInt() : cfg_us;
    if (server.hasArg("vel")) {
        cfg_vel = constrain(server.arg("vel").toInt(), 1, 10);
    }
    if (server.hasArg("tol")) {
        cfg_tol_lin = server.arg("tol").toFloat();
    }
    cfg_delay = calcular_delay_us(cfg_vel);
    
    JsonDocument doc;
    doc["ui"] = cfg_ui;
    doc["us"] = cfg_us;
    doc["vel"] = cfg_vel;
    doc["tol"] = cfg_tol_lin;
    doc["delay"] = cfg_delay;
    
    File f = LittleFS.open("/config.json", "w");
    if (f) {
        serializeJson(doc, f);
        f.close();
    }
    server.send(200, "application/json", "{\"msg\":\"Ajustes guardados correctamente.\"}");
}

void handleHistorial() {
    server.send_P(200, "text/html", HTML_HISTORIAL);
}

void handleHistorialJson() {
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "application/json", "");
    server.sendContent("[");
    
    Dir dir = LittleFS.openDir("/");
    bool first = true;
    while (dir.next()) {
        String fn = dir.fileName();
        if (fn.indexOf("r_") >= 0 && fn.endsWith(".json")) {
            File f = dir.openFile("r");
            if (f) {
                if (!first) server.sendContent(",");
                first = false;
                while (f.available()) {
                    String chunk = f.readStringUntil('\n');
                    chunk.trim();
                    server.sendContent(chunk);
                    yield();
                }
                f.close();
            }
        }
    }
    server.sendContent("]");
    server.sendContent("");
}

void handleBorrarHistorial() {
    Dir dir = LittleFS.openDir("/");
    while (dir.next()) {
        String fn = dir.fileName();
        if (fn.indexOf("r_") >= 0 && fn.endsWith(".json")) {
            LittleFS.remove(dir.fileName());
            LittleFS.remove("/" + dir.fileName());
        }
    }
    if (LittleFS.exists("/historial.json")) LittleFS.remove("/historial.json");
    if (LittleFS.exists("historial.json")) LittleFS.remove("historial.json");
    if (LittleFS.exists("/next_id.txt")) LittleFS.remove("/next_id.txt");
    if (LittleFS.exists("next_id.txt")) LittleFS.remove("next_id.txt");
    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handleRenombrarRegistro() {
    if (server.hasArg("file_id") && server.hasArg("nombre")) {
        int id = server.arg("file_id").toInt();
        String nuevo_nombre = server.arg("nombre");
        nuevo_nombre.replace("\"", ""); nuevo_nombre.replace("\\", ""); nuevo_nombre.replace("\n", ""); nuevo_nombre.replace("\r", ""); nuevo_nombre.trim();
        
        String p1 = "/r_" + String(id) + ".json";
        String p2 = "r_" + String(id) + ".json";
        String path = LittleFS.exists(p1) ? p1 : p2;
        
        if (LittleFS.exists(path)) {
            File f = LittleFS.open(path, "r");
            if (f) {
                JsonDocument doc;
                DeserializationError err = deserializeJson(doc, f);
                f.close();
                if (!err) {
                    doc["id"] = nuevo_nombre;
                    File f_w = LittleFS.open(path, "w");
                    if (f_w) {
                        serializeJson(doc, f_w);
                        f_w.close();
                    }
                }
            }
        }
    }
    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handleBorrarRegistro() {
    if (server.hasArg("file_id")) {
        int id = server.arg("file_id").toInt();
        String p1 = "/r_" + String(id) + ".json";
        String p2 = "r_" + String(id) + ".json";
        if (LittleFS.exists(p1)) LittleFS.remove(p1);
        if (LittleFS.exists(p2)) LittleFS.remove(p2);
    }
    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handleExportarCSV() {
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.sendHeader("Content-Disposition", "attachment; filename=\"reporte_potenciometros.csv\"");
    server.send(200, "text/csv", "");
    server.sendContent("ID_Nombre,Modelo,Tipo,Linealidad_Porc,Histeresis_Porc,Ruido_CRV_Porc,Dictamen,Datos_ADC_Ida_1_a_N...\n");

    Dir dir = LittleFS.openDir("/");
    while (dir.next()) {
        String fn = dir.fileName();
        if (fn.indexOf("r_") >= 0 && fn.endsWith(".json")) {
            File f = dir.openFile("r");
            if (f) {
                String content = f.readString();
                f.close();
                JsonDocument doc;
                DeserializationError err = deserializeJson(doc, content);
                if (!err) {
                    bool passVal = doc["pass"] | false;
                    String csvLine = doc["id"].as<String>() + "," + doc["modelo"].as<String>() + "," + doc["tipo"].as<String>() + "," + 
                                     String(doc["linealidad"].as<float>(), 2) + "," + String(doc["histeresis"].as<float>(), 2) + "," + 
                                     String(doc["ruido"].as<float>(), 2) + "," + (passVal ? "PASS" : "FAIL");
                    JsonArray curva = doc["curva"].as<JsonArray>();
                    for (JsonVariant v : curva) {
                        csvLine += "," + String(v.as<int>());
                    }
                    csvLine += "\n";
                    server.sendContent(csvLine);
                }
            }
        }
        yield();
    }
    server.sendContent("");
}

// ==========================================
// 5. SETUP Y LOOP PRINCIPAL
// ==========================================
void setup() {
    system_update_cpu_freq(160);
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
    Serial.begin(115200);
    
    if(!LittleFS.begin()){
        Serial.println("Error montando LittleFS");
    }
    
    for (int i = 0; i < 4; i++) {
        pinMode(motor_pins[i], OUTPUT);
    }
    apagar_motor();
    
    cargar_config();
    
    WiFi.mode(WIFI_AP);
    WiFi.softAP("Probador-ESP8266", "");
    
    // Rutas HTTP
    server.on("/", HTTP_GET, handleHome);
    server.on("/start", HTTP_GET, handleStart);
    server.on("/section_sweep", HTTP_GET, handleSectionSweep);
    server.on("/stop", HTTP_GET, handleStop);
    server.on("/status", HTTP_GET, handleStatus);
    server.on("/adc", HTTP_GET, handleADC);
    server.on("/config_data", HTTP_GET, handleConfigData);
    server.on("/manifest.json", HTTP_GET, handleManifest);
    server.on("/logo.png", HTTP_GET, handleLogo);
    server.on("/icon.png", HTTP_GET, handleIcon);
    server.on("/favicon.ico", HTTP_GET, handleIcon);
    server.on("/ajustes", HTTP_GET, handleAjustes);
    server.on("/save_settings", HTTP_GET, handleSaveSettings);
    server.on("/historial", HTTP_GET, handleHistorial);
    server.on("/historial.json", HTTP_GET, handleHistorialJson);
    server.on("/renombrar_registro", HTTP_GET, handleRenombrarRegistro);
    server.on("/borrar_registro", HTTP_GET, handleBorrarRegistro);
    server.on("/borrar_historial", HTTP_GET, handleBorrarHistorial);
    server.on("/exportar_csv", HTTP_GET, handleExportarCSV);
    
    // Portal Cautivo
    server.on("/generate_204", HTTP_GET, handleHome);
    server.on("/gen_204", HTTP_GET, handleHome);
    server.on("/hotspot-detect.html", HTTP_GET, handleHome);
    server.on("/canonical.html", HTTP_GET, handleHome);
    server.on("/connecttest.txt", HTTP_GET, handleHome);
    server.on("/ncsi.txt", HTTP_GET, handleHome);
    server.onNotFound(handleNotFound);
    
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
    if (MDNS.begin("probador")) {
        MDNS.addService("http", "tcp", 80);
    }
    
    // Configuración OTA por Web (/update)
    httpUpdater.setup(&server, "/update");
    
    // Configuración ArduinoOTA (Actualización directa desde Arduino IDE por Wi-Fi)
    ArduinoOTA.setHostname("probador-potes");
    ArduinoOTA.onStart([]() {
        apagar_motor();
        system_state = 0;
    });
    ArduinoOTA.begin();
    
    server.begin();
}

void loop() {
    dnsServer.processNextRequest();
    MDNS.update();
    ArduinoOTA.handle();
    server.handleClient();    
    
    if (system_state == 1) { // HOMING
        int v_adc = leerADC_filtrado();
        lec_act = v_adc;
        
        if (v_adc > cfg_ui && sys_pasos < MAX_PASOS_SEGURIDAD) {
            mover_motor(1, -1, delay_ciclo_actual);
            sys_pasos++;
            if (sys_pasos % PASOS_POR_MUESTRA == 0) {
                datos_homing.push_back(v_adc);
            }
        } else {
            if (sys_pasos >= MAX_PASOS_SEGURIDAD) {
                error_msg = "Fallo Home: No se alcanzo umbral inferior (Timeout)";
                system_state = -1;
                apagar_motor();
            } else {
                mover_motor(15, 1, delay_ciclo_actual);
                sys_pasos = 0;
                pos_absoluta = 0;
                if (is_section_sweep) {
                    system_state = 8;
                } else {
                    pos_maxima = 1;
                    lec_act = leerADC_filtrado();
                    lec_ant = lec_act;
                    system_state = 2; // SAMPLING
                }
            }
        }
    } 
    else if (system_state == 2) { // SAMPLING (Subida)
        if (lec_act < cfg_us && sys_pasos < MAX_PASOS_SEGURIDAD) {
            mover_motor(1, 1, delay_ciclo_actual);
            sys_pasos++;
            pos_absoluta++;
            if (pos_absoluta > pos_maxima) pos_maxima = pos_absoluta;
            
            if (sys_pasos % PASOS_POR_MUESTRA == 0) {
                lec_act = leerADC_filtrado();
                
                // 1. Detección de salto resistivo brusco (desconexión instantánea)
                if (abs(lec_act - lec_ant) > UMBRAL_DESCONEXION) {
                    error_msg = "Desconexion o salto resistivo brusco";
                    system_state = -1;
                    apagar_motor();
                    return;
                }
                
                datos_ida.push_back(lec_act);
                lec_ant = lec_act;
            }
        } else {
            if (sys_pasos >= MAX_PASOS_SEGURIDAD) {
                error_msg = "No se alcanzo el umbral superior (Pista cortada o limite mecanico)";
                system_state = -1;
                apagar_motor();
            } else {
                sys_pasos = 0;
                system_state = 3; // RETURNING
            }
        }
    }
    else if (system_state == 3) { // RETURNING (Vuelta)
        if (pos_absoluta > 0 && lec_act > cfg_ui && sys_pasos < MAX_PASOS_SEGURIDAD) {
            mover_motor(1, -1, delay_ciclo_actual);
            sys_pasos++;
            pos_absoluta--;
            
            if (sys_pasos % PASOS_POR_MUESTRA == 0) {
                lec_act = leerADC_filtrado();
                datos_vuelta.push_back(lec_act);
                lec_ant = lec_act;
            }
        } else {
            // Fin de vuelta del ciclo actual
            if (ciclo_actual < total_ciclos_req) {
                ciclo_actual++;
                actualizar_velocidad_ciclo();
                sys_pasos = 0;
                pos_absoluta = 0;
                datos_ida.clear();
                datos_vuelta.clear();
                lec_act = leerADC_filtrado();
                lec_ant = lec_act;
                system_state = 2; // Siguiente ciclo de subida inmediato
            } else {
                apagar_motor();
                system_state = 4; // DONE
                yield();
                guardar_historial(current_nombre, current_modelo);
            }
        }
    }
    else if (system_state == 8) { // NAVIGATE TO SELECTION START
        if (pos_absoluta < sweep_start) {
            mover_motor(1, 1, delay_ciclo_actual);
            pos_absoluta++;
        } else if (pos_absoluta > sweep_start) {
            mover_motor(1, -1, delay_ciclo_actual);
            pos_absoluta--;
        } else {
            pos_absoluta = sweep_start;
            lec_act = leerADC_filtrado();
            lec_ant = lec_act;
            sys_pasos = 0;
            system_state = 9;
        }
    }
    else if (system_state == 9) { // SWEEP FORWARD (Detalle Ida)
        int delay_det = (delay_ciclo_actual < 2000) ? 2000 : delay_ciclo_actual;
        mover_motor(1, 1, delay_det);
        pos_absoluta++;
        sys_pasos++;
        if (sys_pasos % 16 == 0) {
            lec_act = leerADC_filtrado();
            datos_ida.push_back(lec_act);
            lec_ant = lec_act;
        }
        if (pos_absoluta >= sweep_end) {
            sys_pasos = 0;
            system_state = 10;
        }
    }
    else if (system_state == 10) { // SWEEP BACKWARD (Detalle Vuelta)
        int delay_det = (delay_ciclo_actual < 2000) ? 2000 : delay_ciclo_actual;
        mover_motor(1, -1, delay_det);
        pos_absoluta--;
        sys_pasos++;
        if (sys_pasos % 16 == 0) {
            lec_act = leerADC_filtrado();
            datos_vuelta.push_back(lec_act);
            lec_ant = lec_act;
        }
        if (pos_absoluta <= sweep_start) {
            sys_pasos = 0;
            apagar_motor();
            system_state = 4; // DONE
        }
    }
}
