/*
  AeroPico Virtual RC Bench - ESP32 DevKit V1

  Purpose:
    - Opens a WiFi access point and serves a two-joystick bench RC page.
    - Converts browser controls into SBUS frames for AeroPico-FC bench tests.

  Wiring:
    ESP32 GPIO17  -> existing transistor inverter input
    inverter out  -> AeroPico Pico 2 GP1 / UART0 RX
    ESP32 GND     -> AeroPico GND

  If wiring ESP32 GPIO17 directly to Pico GP1, set SBUS_TX_INVERTED to 0.

  Safety:
    - Bench only. Do not fly with WiFi/browser RC.
    - Throttle defaults to minimum.
    - If browser updates stop, outputs return to safe values and failsafe can be asserted.
*/

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

#ifndef SBUS_TX_PIN
#define SBUS_TX_PIN 17
#endif

#ifndef SBUS_TX_INVERTED
#define SBUS_TX_INVERTED 1
#endif

static constexpr const char* AP_SSID = "AeroPico-VirtualRC";
static constexpr const char* AP_PASS = "aeropico";
static constexpr uint32_t SBUS_BAUD = 100000;
static constexpr uint32_t SBUS_PERIOD_US = 14000;
static constexpr uint32_t CONTROL_TIMEOUT_MS = 700;
static constexpr uint16_t SBUS_MIN = 172;
static constexpr uint16_t SBUS_MID = 992;
static constexpr uint16_t SBUS_MAX = 1811;

static HardwareSerial SbusSerial(2);
static WebServer server(80);
static uint8_t sbusFrame[25];
static uint16_t channels[16];
static uint32_t lastFrameUs = 0;
static uint32_t lastControlMs = 0;
static uint32_t lastPrintMs = 0;
static bool uiConnected = false;
static bool requestedFailsafe = false;

static const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
<title>AeroPico Virtual RC</title>
<style>
body{margin:0;font-family:-apple-system,BlinkMacSystemFont,Segoe UI,sans-serif;background:#0f172a;color:#e5e7eb;touch-action:none}
header{padding:14px 18px;border-bottom:1px solid #23304f;display:flex;justify-content:space-between;gap:12px;align-items:center}
h1{font-size:18px;margin:0}.badge{font-size:12px;padding:5px 9px;border-radius:999px;background:#14532d;color:#bbf7d0}
.wrap{display:grid;grid-template-columns:1fr 1fr;gap:14px;padding:14px}.panel{background:#111c33;border:1px solid #253653;border-radius:12px;padding:12px}
.joy{height:260px;border-radius:14px;background:radial-gradient(circle,#263b62 0,#17243d 65%,#111827 100%);position:relative;overflow:hidden;box-shadow:inset 0 0 0 1px #334155}
.knob{width:72px;height:72px;border-radius:50%;background:#60a5fa;position:absolute;left:50%;top:50%;transform:translate(-50%,-50%);box-shadow:0 8px 24px #0008}
.label{display:flex;justify-content:space-between;font-size:13px;color:#93a4bd;margin:8px 2px}.row{display:flex;gap:10px;flex-wrap:wrap;align-items:center;margin-top:12px}
button,select{background:#1d4ed8;color:white;border:0;border-radius:9px;padding:10px 12px;font-weight:700}button.danger{background:#dc2626}button.safe{background:#15803d}
code{color:#bfdbfe}.grid{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin-top:10px}.cell{background:#0b1222;border:1px solid #24324d;border-radius:8px;padding:9px;font-size:12px}
@media(max-width:780px){.wrap{grid-template-columns:1fr}.joy{height:220px}}
</style>
</head>
<body>
<header><h1>AeroPico Virtual RC Bench</h1><span class="badge" id="link">link pending</span></header>
<div class="wrap">
  <section class="panel"><div class="label"><b>Left stick</b><span>Throttle / Yaw</span></div><div class="joy" id="left"><div class="knob"></div></div></section>
  <section class="panel"><div class="label"><b>Right stick</b><span>Pitch / Roll</span></div><div class="joy" id="right"><div class="knob"></div></div></section>
  <section class="panel">
    <div class="row">
      <label>Mode <select id="mode"><option value="manual">MANUAL</option><option value="stabilize">STABILIZE</option></select></label>
      <button class="safe" id="center">Center sticks</button>
      <button class="danger" id="cut">Throttle cut</button>
      <button class="danger" id="failsafe">Failsafe gap</button>
    </div>
    <div class="grid" id="ch"></div>
    <p>Bench-only RC. Remove propellers. ESP32 emits SBUS on <code>GPIO17</code>.</p>
  </section>
</div>
<script>
let state={roll:0,pitch:0,yaw:0,thr:-100,mode:'manual',fs:0};
function clamp(v,a,b){return Math.max(a,Math.min(b,v))}
function bindJoy(id,onMove,throttle=false){
 const el=document.getElementById(id), knob=el.querySelector('.knob');
 function setFromEvent(e){
  const r=el.getBoundingClientRect(), x=clamp((e.clientX-r.left)/r.width*2-1,-1,1), y=clamp((e.clientY-r.top)/r.height*2-1,-1,1);
  knob.style.left=((x+1)*50)+'%'; knob.style.top=((y+1)*50)+'%'; onMove(x,y);
 }
 function reset(){ if(!throttle){knob.style.left='50%';knob.style.top='50%';onMove(0,0)} }
 el.addEventListener('pointerdown',e=>{el.setPointerCapture(e.pointerId);setFromEvent(e)});
 el.addEventListener('pointermove',e=>{if(e.buttons)setFromEvent(e)});
 el.addEventListener('pointerup',reset); el.addEventListener('pointercancel',reset);
}
bindJoy('left',(x,y)=>{state.yaw=x*100;state.thr=(-y*50)+50},true);
bindJoy('right',(x,y)=>{state.roll=x*100;state.pitch=-y*100});
document.getElementById('mode').onchange=e=>state.mode=e.target.value;
document.getElementById('center').onclick=()=>{state.roll=0;state.pitch=0;state.yaw=0};
document.getElementById('cut').onclick=()=>{state.thr=0};
document.getElementById('failsafe').onclick=()=>{state.fs=1;setTimeout(()=>state.fs=0,1200)};
function send(){
 const q=new URLSearchParams({r:state.roll|0,p:state.pitch|0,y:state.yaw|0,t:state.thr|0,m:state.mode,fs:state.fs});
 fetch('/api?'+q).then(r=>r.json()).then(j=>{
  document.getElementById('link').textContent='SBUS live';
  document.getElementById('ch').innerHTML=j.ch.map((v,i)=>'<div class="cell">CH'+(i+1)+'<br><b>'+v+'</b></div>').join('');
 }).catch(()=>document.getElementById('link').textContent='offline');
}
setInterval(send,80);
</script>
</body>
</html>
)HTML";

static uint16_t clampSbus(int value) {
  if (value < static_cast<int>(SBUS_MIN)) return SBUS_MIN;
  if (value > static_cast<int>(SBUS_MAX)) return SBUS_MAX;
  return static_cast<uint16_t>(value);
}

static uint16_t percentToSbus(int percent, bool center = true) {
  percent = constrain(percent, center ? -100 : 0, 100);
  if (center) return clampSbus(static_cast<int>(SBUS_MID) + (percent * 819) / 100);
  return clampSbus(static_cast<int>(SBUS_MIN) + (percent * 1639) / 100);
}

static void setSafeChannels() {
  for (uint8_t i = 0; i < 16; ++i) channels[i] = SBUS_MID;
  channels[2] = SBUS_MIN;
  channels[4] = SBUS_MIN;
}

static void packSbusFrame(bool failsafe) {
  sbusFrame[0] = 0x0F;
  for (uint8_t i = 1; i < 23; ++i) sbusFrame[i] = 0;
  uint16_t bitIndex = 0;
  for (uint8_t ch = 0; ch < 16; ++ch) {
    const uint16_t value = channels[ch] & 0x07FF;
    for (uint8_t bit = 0; bit < 11; ++bit) {
      if (value & (1U << bit)) {
        sbusFrame[1 + ((bitIndex + bit) >> 3)] |= static_cast<uint8_t>(1U << ((bitIndex + bit) & 0x07));
      }
    }
    bitIndex += 11;
  }
  sbusFrame[23] = failsafe ? 0x08 : 0x00;
  sbusFrame[24] = 0x00;
}

static void handleApi() {
  const int roll = server.arg("r").toInt();
  const int pitch = server.arg("p").toInt();
  const int yaw = server.arg("y").toInt();
  const int throttle = server.arg("t").toInt();
  const bool stabilize = server.arg("m") == "stabilize";
  requestedFailsafe = server.arg("fs").toInt() != 0;

  channels[0] = percentToSbus(roll);
  channels[1] = percentToSbus(pitch);
  channels[2] = percentToSbus(throttle, false);
  channels[3] = percentToSbus(yaw);
  channels[4] = stabilize ? SBUS_MAX : SBUS_MIN;
  lastControlMs = millis();
  uiConnected = true;

  String json = "{\"ch\":[";
  for (uint8_t i = 0; i < 6; ++i) {
    if (i) json += ',';
    json += channels[i];
  }
  json += "]}";
  server.send(200, "application/json", json);
}

static void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  setSafeChannels();

  SbusSerial.begin(SBUS_BAUD, SERIAL_8E2, -1, SBUS_TX_PIN, SBUS_TX_INVERTED);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  server.on("/", handleRoot);
  server.on("/api", handleApi);
  server.begin();

  Serial.println();
  Serial.println("AeroPico Virtual RC Bench ready");
  Serial.printf("AP: %s password: %s IP: %s\n", AP_SSID, AP_PASS, WiFi.softAPIP().toString().c_str());
  Serial.printf("SBUS TX GPIO%d inverted=%s\n", SBUS_TX_PIN, SBUS_TX_INVERTED ? "yes" : "no");
}

void loop() {
  server.handleClient();
  const uint32_t nowMs = millis();
  const bool stale = !uiConnected || static_cast<uint32_t>(nowMs - lastControlMs) > CONTROL_TIMEOUT_MS;
  if (stale) setSafeChannels();

  const uint32_t nowUs = micros();
  if (static_cast<uint32_t>(nowUs - lastFrameUs) >= SBUS_PERIOD_US) {
    lastFrameUs += SBUS_PERIOD_US;
    packSbusFrame(stale || requestedFailsafe);
    SbusSerial.write(sbusFrame, sizeof(sbusFrame));
  }

  if (nowMs - lastPrintMs >= 1000UL) {
    lastPrintMs = nowMs;
    Serial.printf("CH1=%u CH2=%u CH3=%u CH4=%u CH5=%u %s\n",
                  channels[0], channels[1], channels[2], channels[3], channels[4],
                  stale ? "STALE_SAFE" : requestedFailsafe ? "FAILSAFE" : "LIVE");
  }
}
