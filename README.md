# 🎛️ Probador de Potenciómetros

Banco de ensayo automatizado para el análisis de curvas de respuesta, desgaste y control de calidad de potenciómetros rotativos.

<img width="672" height="911" alt="IMG_20261014_135301" src="https://github.com/user-attachments/assets/3280c879-fb27-45ec-b6f9-0dfd92ebbd0d" />

---

## Características Principales

- **Control de Movimiento Automatizado:** Accionamiento mediante motor paso a paso (medio paso / *half-stepping*) con acoplamiento elástico y mordaza de sujeción universal impresa en 3D.
- **Ensayos Multi-Ciclo y Multi-Velocidad:**
  - Ensayos simples de 1 ciclo o pruebas de fatiga/rodaje (*Burn-in* de 5, 10 o 20 ciclos).
  - Ensayo dinámico en 3 velocidades (lenta, media, rápida) para análisis de comportamiento a diferentes tasas de giro.
  - Modo **Detalle / Micromuestreo**: permite seleccionar una ventana angular interactiva en el gráfico y hacer zoom mecánico (muestreo a alta resolución) para detectar saltos o pistas rayadas.
- **Metrología y QC en Tiempo Real:**
  - **Linealidad Independiente (%):** Ajuste de curva por mínimos cuadrados (*Best-Fit Line*).
  - **Histéresis Mecánica (%):** Comparación entre ida y vuelta en idéntico ángulo.
  - **Ruido de Contacto (CRV %):** Detección de fluctuaciones de alta frecuencia del cursor.
  - **Clasificación de Curva (*Taper*):** Reconocimiento automático de tipo **Lineal (B)**, **Audio/Log (A)** o **Antilog (C)**.
  - **Dictamen Aprobado/Rechazado (PASS/FAIL)** según tolerancia configurable.
- **Interfaz Web & PWA Embebida:**
  - Punto de acceso Wi-Fi autónomo (`Probador-ESP8266`) con portal cautivo y mDNS (`http://probador.local`).
  - Sin dependencias de internet: frontend completo alojado en memoria flash (`PROGMEM`).
  - Gráficos interactivos en tiempo real con HTML5 Canvas (tema oscuro y claro).
- **Gestión de Datos & Trazabilidad:**
  - Memoria no volátil mediante sistema de archivos **LittleFS**.
  - Historial de mediciones, comparador multi-curva en pantalla y exportación de ensayos en formato **CSV**.
  - Generador de **Certificado de Ensayo / Ficha Técnica imprimible (PDF)** directa desde el navegador.
  - Calibración de umbrales superior/inferior y actualización de firmware inalámbrica (**Web OTA** y **ArduinoOTA**).

---

## 🛠️ Hardware Utilizado

| Componente | Descripción |
| :--- | :--- |
| **Microcontrolador** | ESP8266 (NodeMCU / Wemos D1 Mini / ESP-12) @ 160 MHz |
| **Motor** | Motor paso a paso 28BYJ-48 (5V o 12V según versión) |
| **Driver de Motor** | Módulo basado en ULN2003 |
| **Mecánica** | Piezas impresas en 3D (chasis, torreta de sujeción, acople y pomo de fijación) |
| **Acoplamiento** | Acople elástico de aluminio para eje motor a eje del potenciómetro |
| **Conexiones de Señal** | 3 cables con pinzas cocodrilo (VCC 3.3V, GND, Wiper hacia A0) |

---

## 🔌 Conexiones y Pinout

```text
ESP8266 GPIO       Driver ULN2003       Descripción
--------------------------------------------------------
D5 (GPIO 14) ----> IN1                  Fase A del motor
D6 (GPIO 12) ----> IN2                  Fase B del motor
D7 (GPIO 13) ----> IN3                  Fase C del motor
D8 (GPIO 15) ----> IN4                  Fase D del motor

ESP8266 ADC        Potenciómetro
--------------------------------------------------------
A0 (TOUT)   <----> Cursor central (Wiper) del potenciómetro
3V3         <----> Terminal extremo 1 (VCC)
GND         <----> Terminal extremo 2 (GND)
```

---

## 🚀 Puesta en Marcha

1. Abre el proyecto en el **Arduino IDE** (asegúrate de tener instalado el core de ESP8266 y las librerías `ArduinoJson` v7 y `LittleFS`).
2. Compila y flashea el archivo `main.ino`.
3. Conecta tu teléfono o PC a la red Wi-Fi:
   - **SSID:** `Probador-ESP8266`
   - **Contraseña:** *(Abierta / sin clave)*
4. El portal cautivo se abrirá automáticamente. Si no, navega a `http://192.168.4.1` o `http://probador.local`.
5. En la sección **Ajustes (⚙️)**, verifica o calibra los límites mecánicos del potenciómetro y ajusta la tolerancia de control de calidad.
6. Presiona **▶ Automático** para comenzar el ensayo.

---

## 📊 Métricas Calculadas

$$\text{Linealidad (\%)} = \frac{\max |V_{\text{real}} - V_{\text{ideal}}|}{\text{Rango Total}} \times 100$$

$$\text{Ruido CRV (\%)} = \frac{\frac{1}{N}\sum |V_i - \frac{V_{i-1} + V_{i+1}}{2}|}{\text{Rango Total}} \times 100$$
