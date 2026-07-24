(function () {
  const PARAM_GROUPS = [
    {
      id: "pid",
      label: "Flight Tuning",
      params: [
        ["ANGLE_P", "Angle P", "Stabilize aci kontrol P kazanci."],
        ["ANGLE_I", "Angle I", "Aci kontrol integral kazanci."],
        ["ANGLE_D", "Angle D", "Aci kontrol turev kazanci."],
        ["RATE_P", "Rate P", "Rate kontrol P kazanci."],
        ["RATE_I", "Rate I", "Rate kontrol integral kazanci."],
        ["RATE_D", "Rate D", "Rate kontrol turev kazanci."]
      ]
    },
    {
      id: "servo",
      label: "Servo Setup",
      params: [
        ["SERVO_MIN", "Servo Min", "PWM minimum mikro-saniye."],
        ["SERVO_MAX", "Servo Max", "PWM maksimum mikro-saniye."],
        ["TRIM_AIL", "Aileron Trim", "Aileron merkez ofseti."],
        ["TRIM_ELE", "Elevator Trim", "Elevator merkez ofseti."],
        ["TRIM_RUD", "Rudder Trim", "Rudder merkez ofseti."],
        ["TRIM_THR", "Throttle Trim", "Throttle ofseti."],
        ["REV_AIL", "Aileron Reverse", "0 normal, 1 ters."],
        ["REV_ELE", "Elevator Reverse", "0 normal, 1 ters."],
        ["REV_RUD", "Rudder Reverse", "0 normal, 1 ters."]
      ]
    },
    {
      id: "mixer",
      label: "Mixer",
      params: [
        ["MIX_ROLL", "Roll Gain", "Aileron mixer carpani."],
        ["MIX_PITCH", "Pitch Gain", "Elevator mixer carpani."],
        ["MIX_YAW", "Yaw Gain", "Rudder mixer carpani."]
      ]
    },
    {
      id: "rc",
      label: "RC Mapping",
      params: [
        ["RC_ROLL_CH", "Roll Channel", "0 tabanli kanal indeksi."],
        ["RC_PITCH_CH", "Pitch Channel", "0 tabanli kanal indeksi."],
        ["RC_THR_CH", "Throttle Channel", "0 tabanli kanal indeksi."],
        ["RC_YAW_CH", "Yaw Channel", "0 tabanli kanal indeksi."],
        ["RC_MODE_CH", "Mode Channel", "Manual/Stabilize secim kanali."]
      ]
    },
    {
      id: "safety",
      label: "Safety",
      params: [
        ["FS_TIMEOUT", "Failsafe Timeout", "RC kaybi zaman asimi, ms."],
        ["PREF_Q_MIN", "Preflight Quality", "Minimum sensor kalite skoru."]
      ]
    },
    {
      id: "battery",
      label: "Battery",
      params: [
        ["BATT_CELLS", "Cell Count", "LiPo seri hucre sayisi. Varsayilan 3S."],
        ["BATT_NOM_V", "Nominal Volt", "Paket nominal voltaji. 3S icin 11.1 V."],
        ["BATT_CAP_MAH", "Capacity mAh", "Paket kapasitesi. Varsayilan 3300 mAh."],
        ["BATT_C_RATE", "C Rating", "Paket C degeri. Varsayilan 40C."],
        ["BATT_LOW_V", "Low Voltage", "Preflight/failsafe dusuk voltaj esigi."],
        ["BATT_BRN_V", "Brownout Voltage", "Kritik brownout esigi."]
      ]
    },
    {
      id: "streams",
      label: "Telemetry",
      params: [
        ["MAV_ATT_HZ", "Attitude Hz", "Attitude telemetry frekansi."],
        ["MAV_RC_HZ", "RC Hz", "RC telemetry frekansi."],
        ["MAV_SYS_HZ", "System Hz", "System telemetry frekansi."],
        ["BB_LOG_HZ", "Blackbox Hz", "Blackbox kayit frekansi."]
      ]
    }
  ];

  const MODULES = [
    ["imu", "IMU", "MPU6050 / gyro+accel"],
    ["mag", "MAG", "Manyetometre"],
    ["baro", "BARO", "Barometre"],
    ["gps", "GPS", "Opsiyonel"],
    ["battery", "BAT", "Battery monitor"],
    ["rc", "RC", "SBUS alici"]
  ];

  const AEROPICO_SERVICE = Object.freeze({
    CAL_IMU: 1,
    CAL_MAG: 2,
    CAL_RC: 3,
    SERVO_TEST: 4,
    RC_MONITOR: 5,
    SENSOR_CHECK: 6,
    PREFLIGHT_CHECK: 7
  });

  const MAV_CMD = Object.freeze({
    COMPONENT_ARM_DISARM: 400
  });

  const MAV_SENSOR = Object.freeze({
    GYRO: 1 << 0,
    ACCEL: 1 << 1,
    MAG: 1 << 2,
    ABS_PRESSURE: 1 << 3,
    GPS: 1 << 5
  });

  const SERVICE_LABELS = Object.freeze({
    CAL_IMU: "IMU kalibrasyon",
    CAL_MAG: "Mag kalibrasyon",
    CAL_RC: "RC kalibrasyon",
    SERVO_TEST: "Servo test",
    RC_MONITOR: "RC kanal kontrol",
    SENSOR_CHECK: "Sensor kontrol",
    PREFLIGHT_CHECK: "Preflight kontrol",
    ARM: "Manuel arm",
    FORCE_ARM: "Bench force arm",
    DISARM: "Disarm"
  });

  const SERVICE_STEPS = Object.freeze({
    CAL_IMU: [
      "Kartı düz ve titreşimsiz zeminde sabit tut.",
      "Servo/throttle hattının güvenli ve pervanenin sökülü olduğundan emin ol.",
      "Başlat sonrası kartı hareket ettirme; sonuç Olay Kaydı ve Komut Durumu alanına düşer."
    ],
    CAL_MAG: [
      "Kartı metal yüzeylerden, motor/ESC kablolarından ve mıknatıslardan uzak tut.",
      "Başlat sonrası kartı yavaşça farklı yönlere çevir.",
      "Kalibrasyon sonucu firmware tarafından ACK/STATUSTEXT ile raporlanır."
    ],
    CAL_RC: [
      "RC veya sanal RC sinyalinin bağlı ve hareket ürettiğinden emin ol.",
      "Stickleri sırayla minimum, maksimum ve merkeze getir.",
      "Tamamlandığında kanal durumu Komut Durumu ve modül panelinde güncellenir."
    ],
    SERVO_TEST: [
      "Pervane mutlaka sökülü olmalı.",
      "Servo sinyal hattı ve ortak GND bağlantısı kontrol edilmeli.",
      "Firmware armed veya güvenli değilse komutu reddeder."
    ],
    RC_MONITOR: [
      "RC/SBUS hattında sinyal olduğundan emin ol.",
      "Kanal hareketlerini kontrol et.",
      "Firmware geçerli kanal görürse RC modülü OK olur."
    ],
    SENSOR_CHECK: [
      "IMU, baro ve opsiyonel mag/GPS bağlantılarını sabit tut.",
      "Firmware bağlı modülleri ve sağlık durumunu raporlar.",
      "Eksik opsiyonel modüller preflight sebebi olarak açıkça görünür."
    ],
    PREFLIGHT_CHECK: [
      "Pervane sökülü, throttle düşük ve kart sabit olmalı.",
      "Firmware arm kapısını gerçek sensör/RC/failsafe durumuna göre değerlendirir.",
      "Red sebebi Preflight panelinde gösterilir."
    ]
  });

  const DEFAULT_WIRING = Object.freeze([
    [2, "RC Giriş (SBUS/PPM)"],
    [6, "I2C SDA (IMU/MAG/BARO)"],
    [7, "I2C SCL (IMU/MAG/BARO)"],
    [11, "UART TX (GPS/Telemetri)"],
    [12, "UART RX (GPS/Telemetri)"],
    [16, "ADC Batarya Voltaj"],
    [36, "Servo 1"],
    [37, "Servo 2"],
    [39, "Servo 3"],
    [40, "Servo 4"]
  ]);

  const PARAM_RULES = Object.freeze({
    ANGLE_P: { min: 0, max: 10, step: 0.001 },
    ANGLE_I: { min: 0, max: 5, step: 0.001 },
    ANGLE_D: { min: 0, max: 2, step: 0.001 },
    RATE_P: { min: 0, max: 10, step: 0.001 },
    RATE_I: { min: 0, max: 5, step: 0.001 },
    RATE_D: { min: 0, max: 2, step: 0.001 },
    SERVO_MIN: { min: 800, max: 1500, step: 1, integer: true },
    SERVO_MAX: { min: 1500, max: 2200, step: 1, integer: true },
    TRIM_AIL: { min: -400, max: 400, step: 1 },
    TRIM_ELE: { min: -400, max: 400, step: 1 },
    TRIM_RUD: { min: -400, max: 400, step: 1 },
    TRIM_THR: { min: -400, max: 400, step: 1 },
    REV_AIL: { min: 0, max: 1, step: 1, integer: true },
    REV_ELE: { min: 0, max: 1, step: 1, integer: true },
    REV_RUD: { min: 0, max: 1, step: 1, integer: true },
    MIX_ROLL: { min: 0, max: 2, step: 0.001 },
    MIX_PITCH: { min: 0, max: 2, step: 0.001 },
    MIX_YAW: { min: 0, max: 2, step: 0.001 },
    RC_ROLL_CH: { min: 0, max: 15, step: 1, integer: true },
    RC_PITCH_CH: { min: 0, max: 15, step: 1, integer: true },
    RC_THR_CH: { min: 0, max: 15, step: 1, integer: true },
    RC_YAW_CH: { min: 0, max: 15, step: 1, integer: true },
    RC_MODE_CH: { min: 0, max: 15, step: 1, integer: true },
    FS_TIMEOUT: { min: 100, max: 5000, step: 1, integer: true },
    PREF_Q_MIN: { min: 0, max: 100, step: 1, integer: true },
    BATT_CELLS: { min: 1, max: 6, step: 1, integer: true },
    BATT_NOM_V: { min: 3.0, max: 26.0, step: 0.1 },
    BATT_CAP_MAH: { min: 100, max: 30000, step: 10, integer: true },
    BATT_C_RATE: { min: 1, max: 200, step: 1, integer: true },
    BATT_LOW_V: { min: 3.0, max: 26.0, step: 0.1 },
    BATT_BRN_V: { min: 3.0, max: 26.0, step: 0.1 },
    MAV_ATT_HZ: { min: 0, max: 100, step: 1, integer: true },
    MAV_RC_HZ: { min: 0, max: 100, step: 1, integer: true },
    MAV_SYS_HZ: { min: 0, max: 50, step: 1, integer: true },
    BB_LOG_HZ: { min: 0, max: 500, step: 1, integer: true }
  });

  const FACTORY_DEFAULTS = Object.freeze({
    ANGLE_P: 2.0,
    ANGLE_I: 0.05,
    ANGLE_D: 0.08,
    RATE_P: 0.18,
    RATE_I: 0.02,
    RATE_D: 0.004,
    SERVO_MIN: 1000,
    SERVO_MAX: 2000,
    TRIM_AIL: 0,
    TRIM_ELE: 0,
    TRIM_RUD: 0,
    TRIM_THR: 0,
    REV_AIL: 0,
    REV_ELE: 0,
    REV_RUD: 0,
    MIX_ROLL: 1.0,
    MIX_PITCH: 1.0,
    MIX_YAW: 0.7,
    RC_ROLL_CH: 0,
    RC_PITCH_CH: 1,
    RC_THR_CH: 2,
    RC_YAW_CH: 3,
    RC_MODE_CH: 4,
    FS_TIMEOUT: 500,
    PREF_Q_MIN: 55,
    BATT_CELLS: 3,
    BATT_NOM_V: 11.1,
    BATT_CAP_MAH: 3300,
    BATT_C_RATE: 40,
    BATT_LOW_V: 10.5,
    BATT_BRN_V: 9.6,
    MAV_ATT_HZ: 50,
    MAV_RC_HZ: 20,
    MAV_SYS_HZ: 2,
    BB_LOG_HZ: 50
  });

  const PARAMETER_PROFILES = Object.freeze([
    {
      id: "safe-default",
      name: "Güvenli Varsayılan",
      description: "3S 3300 mAh sabit kanat için sakin, bench dostu başlangıç ayarı.",
      params: FACTORY_DEFAULTS
    },
    {
      id: "trainer-soft",
      name: "Trainer / Yumuşak",
      description: "Daha düşük rate kazancı ve yumuşak yaw karışımıyla ilk saha denemeleri.",
      params: {
        ...FACTORY_DEFAULTS,
        ANGLE_P: 1.6,
        ANGLE_D: 0.05,
        RATE_P: 0.12,
        RATE_D: 0.002,
        MIX_YAW: 0.55,
        PREF_Q_MIN: 45
      }
    },
    {
      id: "bench-fast",
      name: "Bench / Hızlı Tepki",
      description: "Servo ve IMU görsel testlerinde daha belirgin tepki görmek için.",
      params: {
        ...FACTORY_DEFAULTS,
        ANGLE_P: 2.4,
        RATE_P: 0.24,
        RATE_D: 0.006,
        MAV_ATT_HZ: 50,
        MAV_RC_HZ: 30,
        BB_LOG_HZ: 50
      }
    }
  ]);

  const RAD_TO_DEG = 180 / Math.PI;

  /* ── Pin Mapper data — Raspberry Pi Pico 2 (RP2350) pinout ──
     Physical layout matches original Pico: 40 castellated edge
     pins, 20 per side. Right column numbering wraps from the
     bottom, so pin 20 (bottom-left) sits opposite pin 21
     (bottom-right). */
  const PIN_ROLES = [
    "Kullanılmıyor",
    "Servo 1", "Servo 2", "Servo 3", "Servo 4", "Servo 5", "Servo 6",
    "RC Giriş (SBUS/PPM)",
    "RC Kanal 1", "RC Kanal 2", "RC Kanal 3", "RC Kanal 4",
    "I2C SDA (IMU/MAG/BARO)", "I2C SCL (IMU/MAG/BARO)",
    "UART TX (GPS/Telemetri)", "UART RX (GPS/Telemetri)",
    "ADC Batarya Voltaj", "ADC Batarya Akım",
    "Buzzer", "Status LED", "Kill Switch"
  ];

  const PIN_DEFS = [
    { n: 1, side: "L", gpio: "GP0", fn: ["UART0 TX", "I2C0 SDA", "PWM0 A"] },
    { n: 2, side: "L", gpio: "GP1", fn: ["UART0 RX", "I2C0 SCL", "PWM0 B"] },
    { n: 3, side: "L", gpio: "GND", power: true },
    { n: 4, side: "L", gpio: "GP2", fn: ["I2C1 SDA", "PWM1 A"] },
    { n: 5, side: "L", gpio: "GP3", fn: ["I2C1 SCL", "PWM1 B"] },
    { n: 6, side: "L", gpio: "GP4", fn: ["UART1 TX", "I2C0 SDA", "PWM2 A"] },
    { n: 7, side: "L", gpio: "GP5", fn: ["I2C0 SCL", "PWM2 B"] },
    { n: 8, side: "L", gpio: "GND", power: true },
    { n: 9, side: "L", gpio: "GP6", fn: ["I2C1 SDA", "PWM3 A"] },
    { n: 10, side: "L", gpio: "GP7", fn: ["I2C1 SCL", "PWM3 B"] },
    { n: 11, side: "L", gpio: "GP8", fn: ["I2C0 SDA", "PWM4 A"] },
    { n: 12, side: "L", gpio: "GP9", fn: ["I2C0 SCL", "PWM4 B"] },
    { n: 13, side: "L", gpio: "GND", power: true },
    { n: 14, side: "L", gpio: "GP10", fn: ["I2C1 SDA", "PWM5 A"] },
    { n: 15, side: "L", gpio: "GP11", fn: ["I2C1 SCL", "PWM5 B"] },
    { n: 16, side: "L", gpio: "GP12", fn: ["I2C0 SDA", "PWM6 A"] },
    { n: 17, side: "L", gpio: "GP13", fn: ["I2C0 SCL", "PWM6 B"] },
    { n: 18, side: "L", gpio: "GND", power: true },
    { n: 19, side: "L", gpio: "GP14", fn: ["I2C1 SDA", "PWM7 A"] },
    { n: 20, side: "L", gpio: "GP15", fn: ["I2C1 SCL", "PWM7 B"] },

    { n: 21, side: "R", gpio: "VBUS", power: true },
    { n: 22, side: "R", gpio: "VSYS", power: true },
    { n: 23, side: "R", gpio: "GND", power: true },
    { n: 24, side: "R", gpio: "3V3_EN", power: true },
    { n: 25, side: "R", gpio: "3V3(OUT)", power: true },
    { n: 26, side: "R", gpio: "ADC_VREF", power: true },
    { n: 27, side: "R", gpio: "GP28", fn: ["ADC2", "PWM6 A"] },
    { n: 28, side: "R", gpio: "GND", power: true },
    { n: 29, side: "R", gpio: "GP27", fn: ["ADC1", "PWM5 B"] },
    { n: 30, side: "R", gpio: "GP26", fn: ["ADC0", "PWM5 A"] },
    { n: 31, side: "R", gpio: "RUN", power: true },
    { n: 32, side: "R", gpio: "GP22", fn: ["PWM3 A"] },
    { n: 33, side: "R", gpio: "GND", power: true },
    { n: 34, side: "R", gpio: "GP21", fn: ["I2C0 SCL", "PWM2 B"] },
    { n: 35, side: "R", gpio: "GP20", fn: ["I2C0 SDA", "PWM2 A"] },
    { n: 36, side: "R", gpio: "GP19", fn: ["I2C1 SCL", "PWM1 B"] },
    { n: 37, side: "R", gpio: "GP18", fn: ["I2C1 SDA", "PWM1 A"] },
    { n: 38, side: "R", gpio: "GND", power: true },
    { n: 39, side: "R", gpio: "GP17", fn: ["I2C0 SCL", "PWM0 B"] },
    { n: 40, side: "R", gpio: "GP16", fn: ["I2C0 SDA", "PWM0 A"] }
  ];

  const state = {
    port: null,
    reader: null,
    writer: null,
    nativeSerial: false,
    connected: false,
    appShell: document.querySelector(".app-shell"),
    activeGroup: "pid",
    params: new Map(),
    expectedParamCount: 0,
    modules: {
      imu: "unknown",
      mag: "unknown",
      baro: "unknown",
      gps: "unknown",
      battery: "unknown",
      rc: "unknown"
    },
    armed: null,
    lastCommand: null,
    commandHistory: [],
    lastHeartbeatMs: 0,
    portDisplay: { name: null, vid: null, pid: null },
    activeBaud: null,
    selectedPin: null,
    pinMap: new Map(), // pinNumber -> role string
    paramFeedback: new Map(),
    terminalCollapsed: false,
    telemetry: {
      rollDeg: 0,
      pitchDeg: 0,
      yawDeg: 0,
      altM: 0,
      climbMps: 0,
      samples: []
    }
  };

  const encoder = new window.AeroPicoMavlink.Encoder();
  const parser = new window.AeroPicoMavlink.Parser(handleMavlinkMessage);

  const els = {
    connectBtn: document.getElementById("connectBtn"),
    disconnectBtn: document.getElementById("disconnectBtn"),
    readParamsBtn: document.getElementById("readParamsBtn"),
    saveParamsBtn: document.getElementById("saveParamsBtn"),
    baudSelect: document.getElementById("baudSelect"),
    customBaudField: document.getElementById("customBaudField"),
    customBaudInput: document.getElementById("customBaudInput"),
    linkStatus: document.getElementById("linkStatus"),
    portName: document.getElementById("portName"),
    portIds: document.getElementById("portIds"),
    portBaudActive: document.getElementById("portBaudActive"),
    tabs: document.getElementById("tabs"),
    settingsGrid: document.getElementById("settingsGrid"),
    moduleGrid: document.getElementById("moduleGrid"),
    moduleSummary: document.getElementById("moduleSummary"),
    preflightText: document.getElementById("preflightText"),
    log: document.getElementById("log"),
    clearLogBtn: document.getElementById("clearLogBtn"),
    factoryResetBtn: document.getElementById("factoryResetBtn"),
    exportBtn: document.getElementById("exportBtn"),
    importBtn: document.getElementById("importBtn"),
    importInput: document.getElementById("importInput"),
    themeToggleBtn: document.getElementById("themeToggleBtn"),
    profileManagerBtn: document.getElementById("profileManagerBtn"),
    profileManagerModal: document.getElementById("profileManagerModal"),
    profileList: document.getElementById("profileList"),
    telemetryTestBtn: document.getElementById("telemetryTestBtn"),
    telemetryTestModal: document.getElementById("telemetryTestModal"),
    serviceCommandModal: document.getElementById("serviceCommandModal"),
    serviceModalTitle: document.getElementById("serviceModalTitle"),
    serviceModalSteps: document.getElementById("serviceModalSteps"),
    serviceModalCancelBtn: document.getElementById("serviceModalCancelBtn"),
    serviceModalStartBtn: document.getElementById("serviceModalStartBtn"),
    pinMapperBtn: document.getElementById("pinMapperBtn"),
    pinMapperModal: document.getElementById("pinMapperModal"),
    portPickerModal: document.getElementById("portPickerModal"),
    portPickerList: document.getElementById("portPickerList"),
    cancelPortPickBtn: document.getElementById("cancelPortPickBtn"),
    pinBoard: document.getElementById("pinBoard"),
    pinDetail: document.getElementById("pinDetail"),
    pinAssignmentList: document.getElementById("pinAssignmentList"),
    configAudit: document.getElementById("configAudit"),
    applyDefaultPinsBtn: document.getElementById("applyDefaultPinsBtn"),
    linkSummary: document.getElementById("linkSummary"),
    paramSummary: document.getElementById("paramSummary"),
    moduleSummaryTop: document.getElementById("moduleSummaryTop"),
    heartbeatSummary: document.getElementById("heartbeatSummary"),
    armSummary: document.getElementById("armSummary"),
    commandSummary: document.getElementById("commandSummary"),
    commandStatusList: document.getElementById("commandStatusList"),
    terminalPanel: document.getElementById("terminalPanel"),
    terminalToggleBtn: document.getElementById("terminalToggleBtn"),
    terminalPreflightBtn: document.getElementById("terminalPreflightBtn"),
    terminalLogBtn: document.getElementById("terminalLogBtn"),
    preflightPane: document.getElementById("preflightPane"),
    logPane: document.getElementById("logPane"),
    horizonRoll: document.getElementById("horizonRoll"),
    horizonPitch: document.getElementById("horizonPitch"),
    rollValue: document.getElementById("rollValue"),
    pitchValue: document.getElementById("pitchValue"),
    yawValue: document.getElementById("yawValue"),
    plotValue: document.getElementById("plotValue"),
    telemetryPlot: document.getElementById("telemetryPlot")
  };

  const TAB_ICONS = {
    pid: '<svg class="tab-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="22 12 18 12 15 21 9 3 6 12 2 12"/></svg>',
    servo: '<svg class="tab-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="3"/><path d="M12 1v4M12 19v4M4.22 4.22l2.83 2.83M16.95 16.95l2.83 2.83M1 12h4M19 12h4M4.22 19.78l2.83-2.83M16.95 7.05l2.83-2.83"/></svg>',
    mixer: '<svg class="tab-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="4" y1="21" x2="4" y2="14"/><line x1="4" y1="10" x2="4" y2="3"/><line x1="12" y1="21" x2="12" y2="12"/><line x1="12" y1="8" x2="12" y2="3"/><line x1="20" y1="21" x2="20" y2="16"/><line x1="20" y1="12" x2="20" y2="3"/><line x1="1" y1="14" x2="7" y2="14"/><line x1="9" y1="8" x2="15" y2="8"/><line x1="17" y1="16" x2="23" y2="16"/></svg>',
    rc: '<svg class="tab-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="2" y="7" width="20" height="14" rx="2" ry="2"/><path d="M16 21V5a2 2 0 0 0-2-2h-4a2 2 0 0 0-2 2v16"/></svg>',
    safety: '<svg class="tab-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"/></svg>',
    battery: '<svg class="tab-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="2" y="7" width="18" height="10" rx="2"/><path d="M22 11v2"/><path d="M6 11h6"/></svg>',
    streams: '<svg class="tab-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"/><circle cx="12" cy="12" r="3"/></svg>'
  };

  const MODULE_ICONS = {
    imu: '<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><rect x="4" y="4" width="16" height="16" rx="2"/><line x1="12" y1="4" x2="12" y2="20"/><line x1="4" y1="12" x2="20" y2="12"/></svg>',
    mag: '<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"/><polygon points="16.24 7.76 14.12 14.12 7.76 16.24 9.88 9.88 16.24 7.76"/></svg>',
    baro: '<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><path d="M12 2v20M2 12h20"/><circle cx="12" cy="12" r="4"/></svg>',
    gps: '<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><path d="M21 10c0 7-9 13-9 13s-9-6-9-13a9 9 0 0 1 18 0z"/><circle cx="12" cy="10" r="3"/></svg>',
    battery: '<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><rect x="1" y="6" width="18" height="12" rx="2" ry="2"/><line x1="23" y1="13" x2="23" y2="11"/></svg>',
    rc: '<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><polyline points="22 12 18 12 15 21 9 3 6 12 2 12"/></svg>'
  };

  function log(line) {
    const time = new Date().toLocaleTimeString("tr-TR", { hour12: false });
    els.log.textContent += `[${time}] ${line}\n`;
    els.log.scrollTop = els.log.scrollHeight;
  }

  function setLinkStatus(text, cls) {
    els.linkStatus.textContent = text;
    els.linkStatus.className = `status-pill ${cls}`;
    els.linkSummary.textContent = text;
    els.linkSummary.dataset.state = cls;
  }

  /* ── Theme ────────────────────────────────── */

  function initTheme() {
    let saved = null;
    try { saved = localStorage.getItem("aeropico-theme"); } catch (error) { saved = null; }
    const theme = saved === "dark" || saved === "light" ? saved : "light";
    applyTheme(theme);
  }

  function applyTheme(theme) {
    document.documentElement.setAttribute("data-theme", theme);
    try { localStorage.setItem("aeropico-theme", theme); } catch (error) { /* ignore */ }
  }

  function toggleTheme() {
    const current = document.documentElement.getAttribute("data-theme") === "dark" ? "dark" : "light";
    applyTheme(current === "dark" ? "light" : "dark");
  }

  /* ── Generic modal handling ───────────────── */

  function openModal(modalEl) {
    modalEl.classList.remove("hidden");
  }

  function closeModal(modalEl) {
    modalEl.classList.add("hidden");
  }

  function bindModals() {
    document.querySelectorAll("[data-close-modal]").forEach((btn) => {
      btn.addEventListener("click", () => {
        const target = document.getElementById(btn.dataset.closeModal);
        if (target && target.id === "portPickerModal" && window.aeropicoBridge) {
          window.aeropicoBridge.chooseSerialPort("");
        }
        if (target) closeModal(target);
      });
    });

    document.querySelectorAll(".modal-overlay").forEach((overlay) => {
      overlay.addEventListener("click", (event) => {
        if (event.target === overlay) {
          if (overlay.id === "portPickerModal" && window.aeropicoBridge) {
            window.aeropicoBridge.chooseSerialPort("");
          }
          closeModal(overlay);
        }
      });
    });

    document.addEventListener("keydown", (event) => {
      if (event.key === "Escape") {
        document.querySelectorAll(".modal-overlay:not(.hidden)").forEach((modal) => {
          if (modal.id === "portPickerModal" && window.aeropicoBridge) {
            window.aeropicoBridge.chooseSerialPort("");
          }
          closeModal(modal);
        });
      }
    });

    els.pinMapperBtn.addEventListener("click", () => openModal(els.pinMapperModal));
  }

  /* ── Collapsible panels ───────────────────── */

  function bindCollapsibles() {
    document.querySelectorAll("[data-collapsible]").forEach((panel) => {
      const trigger = panel.querySelector(".collapse-trigger");
      if (!trigger) return;
      trigger.addEventListener("click", () => {
        panel.classList.toggle("collapsed");
      });
    });
  }

  /* ── Sections (formerly sidebar tabs) ─────── */

  function renderTabs() {
    els.tabs.innerHTML = "";
    for (const group of PARAM_GROUPS) {
      const button = document.createElement("button");
      button.className = `tab ${group.id === state.activeGroup ? "active" : ""}`;
      const icon = TAB_ICONS[group.id] || "";
      button.type = "button";
      button.innerHTML = `${icon}<span>${group.label}</span><small>${group.params.length} ayar</small>`;
      button.addEventListener("click", () => {
        state.activeGroup = group.id;
        renderTabs();
        renderSettings();
      });
      els.tabs.appendChild(button);
    }
  }

  function renderModules() {
    els.moduleGrid.innerHTML = "";
    let okCount = 0;
    for (const [id, label, desc] of MODULES) {
      const value = state.modules[id];
      const card = document.createElement("div");
      card.className = `module ${value === "ok" ? "ok" : value === "bad" ? "bad" : value === "unsupported" ? "warn" : ""}`;
      const icon = MODULE_ICONS[id] || "";
      card.innerHTML = `<strong>${icon} ${label}</strong><span>${desc}<br>${moduleText(value)}</span>`;
      els.moduleGrid.appendChild(card);
      if (value === "ok") okCount++;
    }
    els.moduleSummary.textContent = okCount === 0 ? "Bekliyor" : `${okCount}/${MODULES.length}`;
    els.moduleSummary.className = `status-pill ${okCount > 0 ? "ok" : "muted"}`;
    els.moduleSummaryTop.textContent = okCount === 0 ? "Bekliyor" : `${okCount}/${MODULES.length} hazır`;
    els.moduleSummaryTop.dataset.state = okCount > 0 ? "ok" : "muted";
  }

  function moduleText(value) {
    if (value === "ok") return "✓ Algılandı";
    if (value === "bad") return "⚠ Algılandı / sağlıksız";
    if (value === "unsupported") return "⚠ Bulundu / destek bekliyor";
    if (value === "missing") return "✗ Yok / pasif";
    return "— Bilinmiyor";
  }

  function applyBusProbeSummary(text) {
    const upper = text.toUpperCase();
    if (!upper.includes("I2C:") && !upper.includes("AUX:")) return false;

    if (upper.includes("0X68/IMU")) state.modules.imu = "ok";
    if (upper.includes("0X77/BARO")) state.modules.baro = "ok";
    if (upper.includes("0X2C/MAG?")) state.modules.mag = "unsupported";
    else if (upper.includes("0X1E/MAG") || upper.includes("0X2C/MAG")) state.modules.mag = "ok";
    if (upper.includes("0X2C/UNK")) state.modules.mag = "unsupported";
    return true;
  }

  function renderSettings() {
    const group = PARAM_GROUPS.find((item) => item.id === state.activeGroup);
    document.getElementById("sectionTitle").textContent = group.label;
    els.settingsGrid.innerHTML = "";
    for (const [name, label, description] of group.params) {
      const param = state.params.get(name);
      const feedback = state.paramFeedback.get(name);
      const card = document.createElement("article");
      card.className = `setting-card ${feedback ? `param-${feedback.kind}` : ""}`;
      const meta = param ? `#${param.index + 1}/${param.count || state.expectedParamCount || "?"}` : "Okunmadı";

      const header = document.createElement("header");
      const title = document.createElement("h3");
      title.textContent = label;
      const nameMeta = document.createElement("span");
      nameMeta.className = "value-meta";
      nameMeta.textContent = name;
      header.append(title, nameMeta);

      const desc = document.createElement("p");
      desc.textContent = description;

      const preview = group.id === "pid" ? buildPidPreview(name, param ? param.value : FACTORY_DEFAULTS[name]) : null;

      const footer = document.createElement("footer");
      const input = document.createElement("input");
      input.type = "number";
      input.placeholder = "Değer yok";
      const rule = PARAM_RULES[name] || { step: 0.001 };
      input.step = String(rule.step || 0.001);
      if (Number.isFinite(rule.min)) input.min = String(rule.min);
      if (Number.isFinite(rule.max)) input.max = String(rule.max);
      if (param) input.value = String(param.value);

      const button = document.createElement("button");
      button.textContent = feedback && feedback.kind === "ok" ? "✓" : "Yaz";
      button.disabled = !state.connected;
      footer.append(input, button);

      if (Number.isFinite(rule.min) && Number.isFinite(rule.max)) {
        const slider = document.createElement("input");
        slider.className = "param-slider";
        slider.type = "range";
        slider.min = String(rule.min);
        slider.max = String(rule.max);
        slider.step = String(rule.step || 0.001);
        slider.value = input.value || String(FACTORY_DEFAULTS[name] ?? rule.min);
        slider.disabled = !state.connected;
        slider.addEventListener("input", () => {
          input.value = slider.value;
          clearParamFeedback(name);
        });
        input.addEventListener("input", () => {
          if (input.value !== "") slider.value = input.value;
          clearParamFeedback(name);
        });
        footer.appendChild(slider);
      } else {
        input.addEventListener("input", () => clearParamFeedback(name));
      }

      const metaEl = document.createElement("div");
      metaEl.className = "value-meta";
      metaEl.textContent = meta;

      card.append(header, desc);
      if (preview) card.appendChild(preview);
      card.append(footer, metaEl);
      if (feedback) {
        const feedbackEl = document.createElement("div");
        feedbackEl.className = `param-message ${feedback.kind}`;
        feedbackEl.textContent = feedback.text;
        card.appendChild(feedbackEl);
      }
      button.addEventListener("click", () => setParam(name, Number(input.value)));
      els.settingsGrid.appendChild(card);
    }
    renderConfigAudit();
  }

  function buildPidPreview(name, rawValue) {
    const value = Number.isFinite(rawValue) ? rawValue : 0;
    const rule = PARAM_RULES[name] || { min: 0, max: 1 };
    const span = Math.max(0.001, (rule.max || 1) - (rule.min || 0));
    const normalized = Math.max(0, Math.min(1, (value - (rule.min || 0)) / span));
    const damping = name.endsWith("_D") ? 0.62 : name.endsWith("_I") ? 0.32 : 0.46;
    const lift = 1.0 + normalized * 0.85;
    const points = [];
    for (let i = 0; i < 42; i++) {
      const t = i / 41;
      const response = 1 - Math.exp(-lift * 4.2 * t) * Math.cos((1.4 + normalized) * Math.PI * t);
      const y = 26 - Math.max(0, Math.min(1.25, response * damping + normalized * 0.2)) * 20;
      const x = 2 + t * 96;
      points.push(`${i === 0 ? "M" : "L"}${x.toFixed(1)} ${y.toFixed(1)}`);
    }

    const svg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
    svg.setAttribute("class", "pid-preview");
    svg.setAttribute("viewBox", "0 0 100 34");
    svg.setAttribute("aria-hidden", "true");
    svg.innerHTML = `<line class="pid-grid" x1="0" y1="26" x2="100" y2="26"/><line class="pid-grid" x1="0" y1="10" x2="100" y2="10"/><path d="${points.join(" ")}"/>`;
    return svg;
  }

  function setParamFeedback(name, kind, text, autoClear = false) {
    state.paramFeedback.set(name, { kind, text });
    renderSettings();
    if (autoClear) {
      window.setTimeout(() => {
        const current = state.paramFeedback.get(name);
        if (current && current.kind === kind && current.text === text) {
          state.paramFeedback.delete(name);
          renderSettings();
        }
      }, 1600);
    }
  }

  function clearParamFeedback(name) {
    if (!state.paramFeedback.has(name)) return;
    state.paramFeedback.delete(name);
  }

  function updateButtons() {
    els.connectBtn.disabled = state.connected;
    els.disconnectBtn.disabled = !state.connected;
    els.readParamsBtn.disabled = !state.connected;
    els.saveParamsBtn.disabled = !state.connected;
    document.querySelectorAll("[data-command]").forEach((button) => {
      const command = button.dataset.command;
      const dangerousWhileArmed = command === "CAL_IMU" || command === "CAL_MAG" || command === "SERVO_TEST";
      button.disabled = !state.connected || (dangerousWhileArmed && state.armed === true);
    });
    document.querySelectorAll("[data-arm-command]").forEach((button) => {
      const action = button.dataset.armCommand;
      button.disabled = !state.connected ||
        ((action === "arm" || action === "force-arm") && state.armed === true) ||
        (action === "disarm" && state.armed !== true);
    });
    renderSettings();
    renderSummary();
  }

  function renderSummary() {
    const expected = state.expectedParamCount || 0;
    els.paramSummary.textContent = expected > 0 ? `${state.params.size}/${expected}` : `${state.params.size} okunmuş`;
    els.paramSummary.dataset.state = state.params.size > 0 ? "ok" : "muted";

    if (state.lastHeartbeatMs > 0) {
      const ageSec = Math.max(0, Math.round((Date.now() - state.lastHeartbeatMs) / 1000));
      els.heartbeatSummary.textContent = ageSec < 2 ? "Canlı" : `${ageSec} sn önce`;
      els.heartbeatSummary.dataset.state = ageSec < 5 ? "ok" : "warn";
    } else {
      els.heartbeatSummary.textContent = "Yok";
      els.heartbeatSummary.dataset.state = "muted";
    }

    if (state.armed === true) {
      els.armSummary.textContent = "Armed";
      els.armSummary.dataset.state = "bad";
    } else if (state.armed === false) {
      els.armSummary.textContent = "Disarmed";
      els.armSummary.dataset.state = "ok";
    } else {
      els.armSummary.textContent = "Bilinmiyor";
      els.armSummary.dataset.state = "muted";
    }
  }

  /* ── Baud rate select / custom ────────────── */

  function currentBaudRate() {
    if (els.baudSelect.value === "custom") {
      const custom = Number(els.customBaudInput.value);
      return Number.isFinite(custom) && custom > 0 ? custom : null;
    }
    return Number(els.baudSelect.value);
  }

  function bindBaudSelect() {
    els.baudSelect.addEventListener("change", () => {
      const isCustom = els.baudSelect.value === "custom";
      els.customBaudField.classList.toggle("hidden", !isCustom);
    });
  }

  /* ── Port info display ────────────────────── */

  function updatePortInfoDisplay() {
    els.portName.textContent = state.portDisplay.name || "—";
    els.portName.classList.toggle("muted", !state.portDisplay.name);

    const ids = [];
    if (state.portDisplay.vid) ids.push(`VID ${formatHex(state.portDisplay.vid)}`);
    if (state.portDisplay.pid) ids.push(`PID ${formatHex(state.portDisplay.pid)}`);
    els.portIds.textContent = ids.length ? ids.join(" / ") : "—";
    els.portIds.classList.toggle("muted", ids.length === 0);

    els.portBaudActive.textContent = state.activeBaud ? `${Number(state.activeBaud).toLocaleString("tr-TR")} bps` : "—";
    els.portBaudActive.classList.toggle("muted", !state.activeBaud);
  }

  function formatHex(value) {
    const num = typeof value === "string" ? parseInt(value, 16) || Number(value) : value;
    if (!Number.isFinite(num)) return String(value);
    return `0x${num.toString(16).padStart(4, "0").toUpperCase()}`;
  }

  function bindSerialBridge() {
    if (!window.aeropicoBridge) return;
    window.aeropicoBridge.onPortList((ports) => {
      const list = Array.isArray(ports) ? ports : [];
      if (list.length > 0) {
        renderPortPicker(list);
        openModal(els.portPickerModal);
        return;
      }

      log("Uygun serial port bulunamadi.");
      window.aeropicoBridge.chooseSerialPort("");
    });
  }

  function hasNativeSerialBridge() {
    return window.aeropicoBridge &&
      typeof window.aeropicoBridge.nativeConnect === "function" &&
      typeof window.aeropicoBridge.nativeListPorts === "function";
  }

  function bindNativeSerialBridge() {
    if (!window.aeropicoBridge || typeof window.aeropicoBridge.onNativeData !== "function") return;
    window.aeropicoBridge.onNativeData((bytes) => {
      if (state.connected && bytes) parser.pushBytes(bytes);
    });
    window.aeropicoBridge.onNativeError((message) => {
      if (state.connected) log(`Native serial hatasi: ${message}`);
    });
  }

  function chooseLikelyAeroPicoPort(ports) {
    if (ports.length === 0) return null;
    const picoVid = new Set(["2e8a", "0x2e8a", 0x2e8a]);
    return ports.find((port) => picoVid.has(String(port.vendorId).toLowerCase()) || picoVid.has(port.vendorId)) ||
      ports.find((port) => /pico|rp2350|aeropico|usb serial/i.test(`${port.portName || ""} ${port.displayName || ""}`)) ||
      ports[0];
  }

  function setPortDisplay(info) {
    if (info) {
      state.portDisplay.name = info.portName || info.displayName || null;
      state.portDisplay.vid = info.vendorId || null;
      state.portDisplay.pid = info.productId || null;
      updatePortInfoDisplay();
    }
  }

  function renderPortPicker(ports) {
    const recommended = chooseLikelyAeroPicoPort(ports);
    els.portPickerList.innerHTML = "";

    for (const port of ports) {
      const row = document.createElement("button");
      row.type = "button";
      row.className = `port-picker-row ${recommended && port.portId === recommended.portId ? "recommended" : ""}`;

      const title = document.createElement("strong");
      title.textContent = port.portName || port.displayName || port.portId || "USB Serial Port";

      const meta = document.createElement("span");
      const ids = [];
      if (port.vendorId) ids.push(`VID ${formatHex(port.vendorId)}`);
      if (port.productId) ids.push(`PID ${formatHex(port.productId)}`);
      if (port.serialNumber) ids.push(`SN ${port.serialNumber}`);
      meta.textContent = ids.length ? ids.join(" / ") : "Kimlik bilgisi yok";

      const badge = document.createElement("em");
      badge.textContent = recommended && port.portId === recommended.portId ? "Önerilen" : "Seç";

      row.append(title, meta, badge);
      row.addEventListener("click", () => {
        state.portDisplay.name = port.displayName || port.portName || port.portId || null;
        state.portDisplay.vid = port.vendorId || null;
        state.portDisplay.pid = port.productId || null;
        updatePortInfoDisplay();
        closeModal(els.portPickerModal);
        log(`Port secildi: ${state.portDisplay.name || port.portId}. Native baglanti deneniyor.`);
        if (window.aeropicoBridge && typeof window.aeropicoBridge.nativeConnect !== "function") {
          window.aeropicoBridge.chooseSerialPort(port.portId || "");
        } else {
          setTimeout(() => {
            if (!state.connected) connect();
          }, 100);
        }
      });
      els.portPickerList.appendChild(row);
    }
  }

  /* ── Connection ────────────────────────────── */

  async function connect() {
    const baudRate = currentBaudRate();
    if (!baudRate) {
      log("Gecerli bir baud rate girin.");
      return;
    }

    if (hasNativeSerialBridge() && !state.portDisplay.name) {
      const ports = await window.aeropicoBridge.nativeListPorts();
      if (ports.length === 0) {
        log("Native serial port bulunamadi.");
        setLinkStatus("Hata", "bad");
        return;
      }
      renderPortPicker(ports);
      openModal(els.portPickerModal);
      return;
    }

    if (hasNativeSerialBridge() && state.portDisplay.name) {
      try {
        const info = await window.aeropicoBridge.nativeConnect({
          portName: state.portDisplay.name,
          baudRate
        });
        state.nativeSerial = true;
        state.connected = true;
        state.activeBaud = baudRate;
        if (info && info.portName) state.portDisplay.name = info.portName;
        updatePortInfoDisplay();
        setLinkStatus("Bagli", "ok");
        updateButtons();
        log(`Native serial baglandi @ ${baudRate.toLocaleString("tr-TR")} bps.`);
        requestParams();
        return;
      } catch (error) {
        log(`Native serial baglanti hatasi: ${error.message}`);
        setLinkStatus("Hata", "bad");
        updateButtons();
        return;
      }
    }

    log("Native serial bridge yuklenemedi; Configurator yeniden baslatilmali.");
    setLinkStatus("Hata", "bad");
    return;

    if (!("serial" in navigator)) {
      log("Web Serial API bulunamadi. Electron/Chromium surumunu kontrol et.");
      return;
    }

    try {
      state.port = await navigator.serial.requestPort();
      await state.port.open({ baudRate });
      state.writer = state.port.writable.getWriter();
      state.reader = state.port.readable.getReader();
      state.connected = true;
      state.nativeSerial = false;
      state.activeBaud = baudRate;

      if (typeof state.port.getInfo === "function") setPortDisplay(normalizeSerialInfo(state.port.getInfo()));

      updatePortInfoDisplay();
      setLinkStatus("Bagli", "ok");
      updateButtons();
      log(`USB serial baglandi @ ${baudRate.toLocaleString("tr-TR")} bps.`);
      readLoop();
      requestParams();
    } catch (error) {
      log(`Baglanti hatasi: ${error.message}`);
      setLinkStatus("Hata", "bad");
    }
  }

  async function disconnect() {
    try {
      state.connected = false;
      if (state.nativeSerial && window.aeropicoBridge && typeof window.aeropicoBridge.nativeDisconnect === "function") {
        await window.aeropicoBridge.nativeDisconnect();
      } else if (state.reader) {
        await state.reader.cancel().catch(() => { });
        state.reader.releaseLock();
      }
      if (!state.nativeSerial && state.writer) state.writer.releaseLock();
      if (!state.nativeSerial && state.port) await state.port.close();
    } catch (error) {
      log(`Baglanti kapatma hatasi: ${error.message}`);
    } finally {
      state.reader = null;
      state.writer = null;
      state.port = null;
      state.nativeSerial = false;
      state.activeBaud = null;
      setLinkStatus("Kapali", "muted");
      updateButtons();
      updatePortInfoDisplay();
      log("Baglanti kapatildi.");
    }
  }

  async function readLoop() {
    while (state.connected && state.reader) {
      try {
        const { value, done } = await state.reader.read();
        if (done) break;
        if (value) parser.pushBytes(value);
      } catch (error) {
        if (state.connected) log(`Okuma hatasi: ${error.message}`);
        break;
      }
    }
  }

  async function writeFrame(frame) {
    if (state.nativeSerial && window.aeropicoBridge && typeof window.aeropicoBridge.nativeWrite === "function") {
      try {
        const ok = await window.aeropicoBridge.nativeWrite(frame);
        if (!ok) log("Yazma hatasi: native serial port hazir degil.");
      } catch (error) {
        log(`Yazma hatasi: ${error.message}`);
      }
      return;
    }
    if (!state.writer) return;
    try {
      await state.writer.write(frame);
    } catch (error) {
      log(`Yazma hatasi: ${error.message}`);
    }
  }

  function requestParams() {
    writeFrame(encoder.paramRequestList());
    log("PARAM_REQUEST_LIST gonderildi.");
  }

  function setParam(name, value) {
    const validation = validateParam(name, value);
    if (!validation.ok) {
      setParamFeedback(name, "bad", validation.reason);
      log(`${name}: ${validation.reason}`);
      return;
    }
    writeFrame(encoder.paramSet(name, validation.value));
    setParamFeedback(name, "ok", "Gönderildi", true);
    log(`${name} = ${validation.value} gonderildi.`);
  }

  function saveParams() {
    setParam("PARAM_SAVE", 1);
  }

  async function applyFactoryDefaults() {
    if (!window.confirm("Fabrika ayarları aktif parametre alanlarına yüklensin ve bağlı cihaza gönderilsin mi?")) {
      return;
    }

    await applyParameterProfile("Fabrika ayarları", FACTORY_DEFAULTS);
  }

  async function applyParameterProfile(label, params) {
    let sent = 0;
    for (const [name, value] of Object.entries(params)) {
      const validation = validateParam(name, value);
      if (!validation.ok) {
        setParamFeedback(name, "bad", validation.reason);
        continue;
      }
      state.params.set(name, {
        name,
        value: validation.value,
        index: 0,
        count: state.expectedParamCount || Object.keys(FACTORY_DEFAULTS).length
      });
      if (state.connected) {
        await writeFrame(encoder.paramSet(name, validation.value));
        sent++;
      }
    }
    renderSettings();
    renderSummary();
    renderConfigAudit();
    log(state.connected
      ? `${label} yuklendi; ${sent} PARAM_SET gonderildi.`
      : `${label} yerel profile yuklendi; cihaza baglaninca yaz.`);
  }

  function renderProfiles() {
    if (!els.profileList) return;
    els.profileList.innerHTML = "";
    for (const profile of PARAMETER_PROFILES) {
      const card = document.createElement("article");
      card.className = "profile-card";
      const highlights = Object.entries(profile.params)
        .filter(([name]) => name.startsWith("ANGLE_") || name.startsWith("RATE_") || name.startsWith("BATT_"))
        .slice(0, 5)
        .map(([name, value]) => `${name}=${value}`)
        .join(" · ");
      card.innerHTML = `<h4>${profile.name}</h4><p>${profile.description}</p><code>${highlights}</code>`;
      const button = document.createElement("button");
      button.type = "button";
      button.textContent = "Profili Yükle";
      button.addEventListener("click", async () => {
        if (!window.confirm(`${profile.name} profili aktif parametrelere yüklensin mi?`)) return;
        await applyParameterProfile(profile.name, profile.params);
        closeModal(els.profileManagerModal);
      });
      card.appendChild(button);
      els.profileList.appendChild(card);
    }
  }

  function serviceCommandParams(command) {
    if (command === "SERVO_TEST") {
      return [AEROPICO_SERVICE[command], 0, 1600, 700];
    }
    return [AEROPICO_SERVICE[command], 0, 0, 0];
  }

  function sendServiceCommand(command) {
    const action = AEROPICO_SERVICE[command];
    if (!action) {
      log(`${command}: desteklenmeyen configurator komutu.`);
      return;
    }
    if (!state.connected || (!state.writer && !state.nativeSerial)) {
      log(`${SERVICE_LABELS[command] || command}: once baglan.`);
      return;
    }
    const [p1, p2, p3, p4] = serviceCommandParams(command);
    writeFrame(encoder.aeroPicoService(p1, p2, p3, p4));
    state.lastCommand = command;
    pushCommandHistory(command, "pending", "ACK bekleniyor");
    updateButtons();
    log(`${SERVICE_LABELS[command] || command} komutu gonderildi.`);
  }

  function sendArmCommand(action) {
    const arm = action !== "disarm";
    const force = action === "force-arm";
    if (!state.connected || (!state.writer && !state.nativeSerial)) {
      log(`${force ? "Bench force arm" : arm ? "Manuel arm" : "Disarm"}: once baglan.`);
      return;
    }

    const commandKey = force ? "FORCE_ARM" : arm ? "ARM" : "DISARM";
    writeFrame(encoder.commandLong(MAV_CMD.COMPONENT_ARM_DISARM, [arm ? 1 : 0, force ? 21196 : 0, 0, 0, 0, 0, 0]));
    state.lastCommand = commandKey;
    pushCommandHistory(commandKey, "pending", "COMMAND_ACK bekleniyor");
    updateButtons();
    log(`${SERVICE_LABELS[commandKey]} komutu gonderildi.`);
  }

  function pushCommandHistory(command, stateName, detail) {
    const label = SERVICE_LABELS[command] || command;
    state.commandHistory.unshift({
      command,
      label,
      state: stateName,
      detail,
      at: new Date().toLocaleTimeString("tr-TR", { hour12: false })
    });
    state.commandHistory = state.commandHistory.slice(0, 5);
    renderCommandStatus();
  }

  function updateLastCommand(stateName, detail) {
    if (!state.lastCommand) return;
    pushCommandHistory(state.lastCommand, stateName, detail);
    if (stateName !== "pending") state.lastCommand = null;
  }

  function renderCommandStatus() {
    if (!els.commandStatusList) return;
    const latest = state.commandHistory[0];
    if (latest) {
      const pillClass = latest.state === "accepted" ? "ok" : latest.state === "pending" ? "warn" : "bad";
      els.commandSummary.textContent = latest.state === "pending" ? "Bekliyor" : latest.state === "accepted" ? "OK" : "Red";
      els.commandSummary.className = `status-pill ${pillClass}`;
    } else {
      els.commandSummary.textContent = "Bekliyor";
      els.commandSummary.className = "status-pill muted";
    }

    if (state.commandHistory.length === 0) {
      els.commandStatusList.innerHTML = `<p class="hint">Henüz komut gönderilmedi.</p>`;
      return;
    }
    els.commandStatusList.innerHTML = "";
    for (const item of state.commandHistory) {
      const row = document.createElement("div");
      row.className = `command-status-row ${item.state}`;
      row.innerHTML = `<strong>${item.label}</strong><span>${item.detail}</span><time>${item.at}</time>`;
      els.commandStatusList.appendChild(row);
    }
  }

  function mavResultText(result) {
    switch (result) {
      case 0: return "ACCEPTED";
      case 1: return "TEMPORARILY_REJECTED";
      case 2: return "DENIED";
      case 3: return "UNSUPPORTED";
      case 4: return "FAILED";
      default: return `RESULT_${result}`;
    }
  }

  function updateAttitudeTelemetry(message) {
    state.telemetry.rollDeg = message.roll * RAD_TO_DEG;
    state.telemetry.pitchDeg = message.pitch * RAD_TO_DEG;
    state.telemetry.yawDeg = message.yaw * RAD_TO_DEG;
    addTelemetrySample();
    renderLiveTelemetry();
  }

  function updateVfrTelemetry(message) {
    state.telemetry.altM = Number.isFinite(message.alt) ? message.alt : state.telemetry.altM;
    state.telemetry.climbMps = Number.isFinite(message.climb) ? message.climb : state.telemetry.climbMps;
    addTelemetrySample();
    renderLiveTelemetry();
  }

  function addTelemetrySample() {
    const samples = state.telemetry.samples;
    const last = samples[samples.length - 1];
    const now = performance.now();
    if (last && now - last.t < 80) return;
    samples.push({
      t: now,
      roll: state.telemetry.rollDeg,
      pitch: state.telemetry.pitchDeg,
      alt: state.telemetry.altM
    });
    if (samples.length > 140) samples.shift();
  }

  function renderLiveTelemetry() {
    const roll = Math.max(-60, Math.min(60, state.telemetry.rollDeg));
    const pitch = Math.max(-45, Math.min(45, state.telemetry.pitchDeg));
    if (els.horizonRoll) els.horizonRoll.style.transform = `rotate(${-roll}deg)`;
    if (els.horizonPitch) els.horizonPitch.style.transform = `translateY(${pitch * 1.55}px)`;
    if (els.rollValue) els.rollValue.textContent = state.telemetry.rollDeg.toFixed(1);
    if (els.pitchValue) els.pitchValue.textContent = state.telemetry.pitchDeg.toFixed(1);
    if (els.yawValue) els.yawValue.textContent = String(Math.round(normalizeDegrees(state.telemetry.yawDeg)));
    if (els.plotValue) {
      els.plotValue.textContent = `R ${state.telemetry.rollDeg.toFixed(1)} / P ${state.telemetry.pitchDeg.toFixed(1)} / Alt ${state.telemetry.altM.toFixed(1)} m`;
    }
    drawTelemetryPlot();
  }

  function normalizeDegrees(value) {
    let out = value % 360;
    if (out < 0) out += 360;
    return out;
  }

  function drawTelemetryPlot() {
    const canvas = els.telemetryPlot;
    if (!canvas) return;
    const rect = canvas.getBoundingClientRect();
    const dpr = window.devicePixelRatio || 1;
    const width = Math.max(1, Math.round(rect.width * dpr));
    const height = Math.max(1, Math.round(rect.height * dpr));
    if (canvas.width !== width || canvas.height !== height) {
      canvas.width = width;
      canvas.height = height;
    }
    const ctx = canvas.getContext("2d");
    if (!ctx) return;
    ctx.clearRect(0, 0, width, height);
    ctx.lineWidth = Math.max(1, dpr);
    ctx.strokeStyle = getCssColor("--line");
    for (let i = 1; i < 4; i++) {
      const y = (height * i) / 4;
      ctx.beginPath();
      ctx.moveTo(0, y);
      ctx.lineTo(width, y);
      ctx.stroke();
    }
    drawTelemetrySeries(ctx, width, height, "roll", 60, "#2f6feb");
    drawTelemetrySeries(ctx, width, height, "pitch", 45, "#16a34a");
    drawTelemetrySeries(ctx, width, height, "alt", 20, "#f59e0b");
  }

  function drawTelemetrySeries(ctx, width, height, field, span, color) {
    const samples = state.telemetry.samples;
    if (samples.length < 2) return;
    ctx.strokeStyle = color;
    ctx.beginPath();
    samples.forEach((sample, index) => {
      const x = (index / (samples.length - 1)) * width;
      const value = field === "alt" ? sample[field] % span : sample[field];
      const y = height * 0.5 - Math.max(-span, Math.min(span, value)) / span * height * 0.42;
      if (index === 0) ctx.moveTo(x, y);
      else ctx.lineTo(x, y);
    });
    ctx.stroke();
  }

  function getCssColor(name) {
    return getComputedStyle(document.documentElement).getPropertyValue(name).trim() || "#d8e0ee";
  }

  function handleMavlinkMessage(message) {
    if (message.type === "heartbeat") {
      state.lastHeartbeatMs = Date.now();
      state.armed = (message.baseMode & 0x80) !== 0;
      els.preflightText.textContent = `Heartbeat alindi. System status: ${message.systemStatus}. Parametreleri okuyup preflight sonucunu kontrol et.`;
      renderModules();
      updateButtons();
      return;
    }

    if (message.type === "param") {
      state.params.set(message.name, message);
      state.expectedParamCount = message.count;
      renderSettings();
      if (state.params.size === message.count) log(`${message.count} parametre okundu.`);
      renderConfigAudit();
      renderSummary();
      return;
    }

    if (message.type === "sysStatus") {
      const present = message.sensorsPresent >>> 0;
      const health = message.sensorsHealth >>> 0;
      const imuPresent = (present & MAV_SENSOR.GYRO) !== 0 && (present & MAV_SENSOR.ACCEL) !== 0;
      const imuHealthy = (health & MAV_SENSOR.GYRO) !== 0 && (health & MAV_SENSOR.ACCEL) !== 0;
      state.modules.imu = imuPresent ? (imuHealthy ? "ok" : "bad") : "missing";
      state.modules.baro = (present & MAV_SENSOR.ABS_PRESSURE) !== 0
        ? ((health & MAV_SENSOR.ABS_PRESSURE) !== 0 ? "ok" : "bad")
        : "missing";
      state.modules.mag = (present & MAV_SENSOR.MAG) !== 0
        ? ((health & MAV_SENSOR.MAG) !== 0 ? "ok" : "bad")
        : "missing";
      state.modules.gps = (present & MAV_SENSOR.GPS) !== 0
        ? ((health & MAV_SENSOR.GPS) !== 0 ? "ok" : "bad")
        : "unknown";
      if (message.voltageBatteryMv > 0 && message.voltageBatteryMv < 65535) {
        state.modules.battery = "ok";
      } else if (message.voltageBatteryMv === 65535 || message.voltageBatteryMv === 0) {
        state.modules.battery = "unknown";
      } else {
        state.modules.battery = "bad";
      }
      renderModules();
      renderConfigAudit();
      renderSummary();
      return;
    }

    if (message.type === "rcChannels") {
      const usableChannels = message.channels
        .slice(0, Math.max(0, Math.min(message.channelCount, 16)))
        .filter((value) => value >= 800 && value <= 2200);
      state.modules.rc = usableChannels.length >= 4 ? "ok" : "bad";
      renderModules();
      renderConfigAudit();
      renderSummary();
      return;
    }

    if (message.type === "attitude") {
      updateAttitudeTelemetry(message);
      return;
    }

    if (message.type === "vfrHud") {
      updateVfrTelemetry(message);
      return;
    }

    if (message.type === "commandAck") {
      const accepted = message.result === 0;
      updateLastCommand(accepted ? "accepted" : "rejected", mavResultText(message.result));
      if (message.command === 31010) {
        log(`Servis komutu ACK: ${mavResultText(message.result)}.`);
      } else {
        log(`COMMAND_ACK ${message.command}: ${mavResultText(message.result)}.`);
      }
      return;
    }

    if (message.type === "statusText") {
      log(`FC: ${message.text}`);
      const text = message.text.toUpperCase();
      const parsedBusProbe = applyBusProbeSummary(text);
      if (text.includes("IMU CALIBRATION SAVED") || text.includes("SENSOR_CHECK_OK") || text.includes("PREFLIGHT_OK")) state.modules.imu = "ok";
      if (text.includes("IMU MISSING") || text.includes("WHOAMI")) state.modules.imu = "bad";
      if (!parsedBusProbe && (text.includes("BMP") || text.includes("BARO"))) state.modules.baro = text.includes("HAZIR") || text.includes("OK") ? "ok" : "bad";
      if (!parsedBusProbe && text.includes("MAG")) state.modules.mag = text.includes("MISSING") || text.includes("FAILED") ? "bad" : "ok";
      if (!parsedBusProbe && text.includes("HMC")) state.modules.mag = text.includes("HAZIR") || text.includes("OK") ? "ok" : "bad";
      if (text.includes("GPS")) state.modules.gps = text.includes("FIX") || text.includes("HAZIR") ? "ok" : "bad";
      if (text.includes("RC_MONITOR_OK")) state.modules.rc = "ok";
      if (text.includes("RC_MONITOR_FAIL")) state.modules.rc = "bad";
      if (text.includes("RC SIGNAL INVALID") || text.includes("RC INVALID")) state.modules.rc = "bad";
      if (text.includes("SENSOR_CHECK_PARTIAL")) {
        state.modules.imu = "ok";
        els.preflightText.textContent = "Sensor kontrolu kismi basarili: opsiyonel sensorlerden biri eksik.";
      }
      if (text.includes("PREFLIGHT_OK")) els.preflightText.textContent = "Preflight OK: sistem arm icin yazilim tarafinda hazir.";
      if (text.includes("PREFLIGHT") && !text.includes("OK")) els.preflightText.textContent = message.text;
      if (text.includes("ARM") || text.includes("RC SIGNAL") || text.includes("FAILSAFE") ||
          text.includes("BATTERY") || text.includes("ACTUATOR") || text.includes("TIMING")) {
        els.preflightText.textContent = message.text;
      }
      if (state.commandHistory[0] && state.commandHistory[0].state !== "pending") {
        state.commandHistory[0].detail = message.text;
        renderCommandStatus();
      }
      renderModules();
      renderConfigAudit();
      renderSummary();
    }
  }

  function normalizeSerialInfo(info) {
    if (!info) return null;
    return {
      portName: null,
      vendorId: info.usbVendorId || null,
      productId: info.usbProductId || null
    };
  }

  function validateParam(name, value) {
    if (!Number.isFinite(value)) return { ok: false, reason: "gecersiz sayi." };
    const rule = PARAM_RULES[name];
    if (!rule) return { ok: true, value };
    let next = value;
    if (rule.integer) next = Math.round(next);
    if (Number.isFinite(rule.min) && next < rule.min) return { ok: false, reason: `minimum ${rule.min}.` };
    if (Number.isFinite(rule.max) && next > rule.max) return { ok: false, reason: `maksimum ${rule.max}.` };
    return { ok: true, value: next };
  }

  function paramValue(name) {
    const param = state.params.get(name);
    return param && Number.isFinite(param.value) ? param.value : null;
  }

  function renderConfigAudit() {
    if (!els.configAudit) return;
    const items = [];
    const servoMin = paramValue("SERVO_MIN");
    const servoMax = paramValue("SERVO_MAX");
    const batteryCells = paramValue("BATT_CELLS");
    const batteryNominal = paramValue("BATT_NOM_V");
    const batteryCapacity = paramValue("BATT_CAP_MAH");
    const batteryCRate = paramValue("BATT_C_RATE");
    const batteryLow = paramValue("BATT_LOW_V");
    const batteryBrownout = paramValue("BATT_BRN_V");
    const rcChannels = ["RC_ROLL_CH", "RC_PITCH_CH", "RC_THR_CH", "RC_YAW_CH", "RC_MODE_CH"]
      .map((name) => paramValue(name))
      .filter((value) => value !== null);

    if (servoMin !== null && servoMax !== null && servoMin >= servoMax) {
      items.push(["bad", "Servo minimum, maksimumdan küçük olmalı."]);
    } else if (servoMin !== null || servoMax !== null) {
      items.push(["ok", "Servo PWM aralığı tutarlı görünüyor."]);
    }

    if (new Set(rcChannels).size !== rcChannels.length) {
      items.push(["bad", "RC kanal eşlemesinde tekrar eden kanal var."]);
    } else if (rcChannels.length > 0) {
      items.push(["ok", "RC kanal eşlemesi çakışmasız."]);
    }

    if (batteryCells !== null && batteryNominal !== null) {
      const expectedNominal = batteryCells * 3.7;
      const diff = Math.abs(batteryNominal - expectedNominal);
      items.push([
        diff <= 0.25 ? "ok" : "warn",
        `${batteryCells}S batarya profili: nominal ${batteryNominal.toFixed(1)} V, beklenen yaklaşık ${expectedNominal.toFixed(1)} V.`
      ]);
    }

    if (batteryLow !== null && batteryBrownout !== null) {
      if (batteryBrownout >= batteryLow) {
        items.push(["bad", "Brownout voltajı low voltage eşiğinden küçük olmalı."]);
      } else {
        items.push(["ok", "Batarya low/brownout eşikleri tutarlı."]);
      }
    }

    if (batteryCapacity !== null && batteryCRate !== null) {
      const maxCurrent = (batteryCapacity / 1000) * batteryCRate;
      items.push(["muted", `Paket teorik sürekli akım limiti yaklaşık ${maxCurrent.toFixed(0)} A.`]);
    }

    if (state.modules.battery === "bad") items.push(["warn", "Batarya ölçümü yok veya geçersiz görünüyor."]);
    if (state.modules.imu === "bad") items.push(["bad", "IMU algılanmadıysa arming yapılmamalı."]);
    if (!hasAssignedRole("ADC Batarya Voltaj")) items.push(["warn", "Pin Mapper'da batarya ADC ataması yok."]);
    if (!hasAssignedRole("RC Giriş (SBUS/PPM)")) items.push(["warn", "Pin Mapper'da SBUS/RC giriş ataması yok."]);

    if (items.length === 0) items.push(["muted", "Parametre ve modül verisi bekleniyor."]);

    els.configAudit.innerHTML = "";
    for (const [kind, text] of items.slice(0, 6)) {
      const row = document.createElement("div");
      row.className = `audit-row ${kind}`;
      const dot = document.createElement("span");
      dot.className = "audit-dot";
      const label = document.createElement("span");
      label.textContent = text;
      row.append(dot, label);
      els.configAudit.appendChild(row);
    }
  }

  function hasAssignedRole(role) {
    for (const value of state.pinMap.values()) {
      if (value === role) return true;
    }
    return false;
  }

  function exportParams() {
    const data = {};
    for (const [name, param] of state.params.entries()) data[name] = param.value;
    const blob = new Blob([JSON.stringify(data, null, 2)], { type: "application/json" });
    const url = URL.createObjectURL(blob);
    const a = document.createElement("a");
    a.href = url;
    a.download = "aeropico-params.json";
    a.click();
    URL.revokeObjectURL(url);
  }

  async function importParams(file) {
    const text = await file.text();
    const data = JSON.parse(text);
    for (const [name, value] of Object.entries(data)) {
      if (typeof value !== "number") continue;
      const validation = validateParam(name, value);
      if (validation.ok) {
        state.params.set(name, { name, value: validation.value, index: 0, count: 0 });
      } else {
        log(`${name}: ice aktarilmadi, ${validation.reason}`);
      }
    }
    renderSettings();
    log("JSON parametre dosyasi yuklendi. Yazmak icin her parametrede Yaz butonunu kullan.");
  }

  /* ── Pin Mapper ────────────────────────────── */

  function buildPinBoardSvg() {
    const leftPins = PIN_DEFS.filter((p) => p.side === "L");
    const rightPins = PIN_DEFS.filter((p) => p.side === "R");
    const rowH = 21;
    const topPad = 46;
    const boardW = 150;
    const boardX = 75;
    const boardTop = topPad - 14;
    const boardH = rowH * 20 + 20;
    const width = 300;
    const height = boardTop + boardH + 24;

    const padW = 40;
    const padH = 13;

    function pinY(indexInColumn) {
      return topPad + indexInColumn * rowH;
    }

    function pinGroup(pin, indexInColumn) {
      const y = pinY(indexInColumn);
      const isLeft = pin.side === "L";
      const padX = isLeft ? boardX - padW + 6 : boardX + boardW - 6;
      const textAnchor = isLeft ? "end" : "start";
      const labelX = isLeft ? padX - 6 : padX + padW + 6;
      const fill = pin.power ? "var(--muted)" : "var(--accent)";
      return `
        <g class="pin-pad" data-pin="${pin.n}" tabindex="0" role="button" aria-label="Pin ${pin.n} ${pin.gpio}">
          <rect x="${padX}" y="${y - padH / 2}" width="${padW}" height="${padH}" rx="3"
            fill="${pin.power ? 'var(--surface-2)' : 'var(--accent-dim)'}"
            stroke="${fill}" stroke-width="1.1"/>
          <text class="pin-pad-num" x="${padX + padW / 2}" y="${y + 2}" text-anchor="middle">${pin.n}</text>
          <text class="pin-pad-label" x="${labelX}" y="${y + 2.4}" text-anchor="${textAnchor}">${pin.gpio}</text>
        </g>`;
    }

    let svg = `<svg viewBox="0 0 ${width} ${height}" xmlns="http://www.w3.org/2000/svg" font-family="Inter, sans-serif">`;

    // Board body
    svg += `<rect x="${boardX}" y="${boardTop}" width="${boardW}" height="${boardH}" rx="10"
        fill="var(--surface-3)" stroke="var(--glass-border)" stroke-width="1.4"/>`;

    // USB connector notch at top center
    svg += `<rect x="${boardX + boardW / 2 - 13}" y="${boardTop - 12}" width="26" height="14" rx="2"
        fill="var(--surface-1)" stroke="var(--glass-border)" stroke-width="1"/>`;
    svg += `<text class="pin-chip-label" x="${boardX + boardW / 2}" y="${boardTop - 3}" text-anchor="middle" font-size="5" opacity="0.6">USB</text>`;

    // Chip label
    svg += `<text class="pin-chip-label" x="${boardX + boardW / 2}" y="${boardTop + boardH / 2 - 6}" text-anchor="middle" font-size="9" font-weight="700">RP2350</text>`;
    svg += `<text class="pin-chip-label" x="${boardX + boardW / 2}" y="${boardTop + boardH / 2 + 8}" text-anchor="middle" font-size="6.4" opacity="0.65">Raspberry Pi</text>`;
    svg += `<text class="pin-chip-label" x="${boardX + boardW / 2}" y="${boardTop + boardH / 2 + 18}" text-anchor="middle" font-size="6.4" opacity="0.65">Pico 2</text>`;

    leftPins.forEach((pin, i) => { svg += pinGroup(pin, i); });
    rightPins.forEach((pin, i) => { svg += pinGroup(pin, i); });

    svg += `</svg>`;
    return svg;
  }

  function renderPinBoard() {
    els.pinBoard.innerHTML = buildPinBoardSvg();
    els.pinBoard.querySelectorAll(".pin-pad").forEach((group) => {
      const pinNum = Number(group.dataset.pin);
      group.addEventListener("click", () => selectPin(pinNum));
      group.addEventListener("keydown", (event) => {
        if (event.key === "Enter" || event.key === " ") {
          event.preventDefault();
          selectPin(pinNum);
        }
      });
    });
    highlightAssignedPins();
  }

  function highlightAssignedPins() {
    els.pinBoard.querySelectorAll(".pin-pad").forEach((group) => {
      const pinNum = Number(group.dataset.pin);
      group.classList.toggle("selected", state.selectedPin === pinNum || state.pinMap.has(pinNum));
    });
  }

  function selectPin(pinNum) {
    state.selectedPin = pinNum;
    highlightAssignedPins();
    renderPinDetail();
  }

  function renderPinDetail() {
    const pin = PIN_DEFS.find((p) => p.n === state.selectedPin);
    if (!pin) {
      els.pinDetail.innerHTML = `<p class="hint">Atama yapmak için diyagramdan bir pin seçin.</p>`;
      return;
    }

    const chips = (pin.fn || []).map((f) => `<span class="pin-function-chip">${f}</span>`).join("");
    const currentRole = state.pinMap.get(pin.n) || "Kullanılmıyor";

    if (pin.power) {
      els.pinDetail.innerHTML = `
        <div class="pin-detail-title">
          <strong>Pin ${pin.n} · ${pin.gpio}</strong>
          <span class="pin-detail-badge">Güç / Referans</span>
        </div>
        <p class="hint">Bu bir güç veya referans pinidir, fonksiyon ataması gerekmez.</p>
      `;
      return;
    }

    els.pinDetail.innerHTML = `
      <div class="pin-detail-title">
        <strong>Pin ${pin.n} · ${pin.gpio}</strong>
        <span class="pin-detail-badge">GPIO</span>
      </div>
      <div class="pin-detail-functions">${chips}</div>
      <div class="pin-role-select">
        <label for="pinRoleSelect">Fonksiyon Ata</label>
        <select id="pinRoleSelect">
          ${PIN_ROLES.map((role) => `<option value="${role}" ${role === currentRole ? "selected" : ""}>${role}</option>`).join("")}
        </select>
        <button id="pinRoleApplyBtn" class="primary small">Ata</button>
      </div>
    `;

    document.getElementById("pinRoleApplyBtn").addEventListener("click", () => {
      const select = document.getElementById("pinRoleSelect");
      const role = select.value;
      if (role === "Kullanılmıyor") {
        state.pinMap.delete(pin.n);
        log(`Pin ${pin.n} (${pin.gpio}) atamasi kaldirildi.`);
      } else {
        state.pinMap.set(pin.n, role);
        log(`Pin ${pin.n} (${pin.gpio}) -> ${role} olarak atandi.`);
      }
      highlightAssignedPins();
      renderPinAssignments();
    });
  }

  function renderPinAssignments() {
    if (state.pinMap.size === 0) {
      els.pinAssignmentList.innerHTML = `<p class="hint">Henüz atama yok.</p>`;
      renderConfigAudit();
      return;
    }

    const rows = [...state.pinMap.entries()]
      .sort((a, b) => a[0] - b[0])
      .map(([pinNum, role]) => {
        const pin = PIN_DEFS.find((p) => p.n === pinNum);
        return `
          <div class="pin-assignment-row" data-pin="${pinNum}">
            <span class="pin-assignment-gpio">${pin ? pin.gpio : pinNum}</span>
            <span class="pin-assignment-role">${role}</span>
            <button class="small" data-remove-pin="${pinNum}">Kaldır</button>
          </div>`;
      })
      .join("");

    els.pinAssignmentList.innerHTML = rows;
    els.pinAssignmentList.querySelectorAll("[data-remove-pin]").forEach((btn) => {
      btn.addEventListener("click", () => {
        const pinNum = Number(btn.dataset.removePin);
        state.pinMap.delete(pinNum);
        highlightAssignedPins();
        renderPinAssignments();
        if (state.selectedPin === pinNum) renderPinDetail();
      });
    });
    renderConfigAudit();
  }

  function initPinMapper() {
    applyDefaultPinMap(false);
    renderPinBoard();
    renderPinDetail();
    renderPinAssignments();
  }

  function applyDefaultPinMap(announce = true) {
    state.pinMap.clear();
    for (const [pin, role] of DEFAULT_WIRING) state.pinMap.set(pin, role);
    if (announce) log("AeroPico varsayilan pin haritasi uygulandi.");
  }

  /* ── Bindings ──────────────────────────────── */

  function showTerminalPane(name) {
    const showLog = name === "log";
    els.terminalPreflightBtn.classList.toggle("active", !showLog);
    els.terminalLogBtn.classList.toggle("active", showLog);
    els.preflightPane.classList.toggle("active", !showLog);
    els.logPane.classList.toggle("active", showLog);
  }

  function toggleTerminalDrawer() {
    state.terminalCollapsed = !state.terminalCollapsed;
    state.appShell.classList.toggle("terminal-collapsed", state.terminalCollapsed);
    els.terminalPanel.classList.toggle("collapsed", state.terminalCollapsed);
    els.terminalToggleBtn.textContent = state.terminalCollapsed ? "Aç" : "Gizle";
  }

  function openServiceCommandModal(command) {
    const label = SERVICE_LABELS[command] || command;
    const steps = SERVICE_STEPS[command] || ["Firmware servis komutu gönderilecek."];
    els.serviceModalTitle.textContent = label;
    els.serviceModalSteps.innerHTML = "";
    steps.forEach((step, index) => {
      const row = document.createElement("div");
      row.className = "service-step";
      row.innerHTML = `<strong>${index + 1}</strong><span>${step}</span>`;
      els.serviceModalSteps.appendChild(row);
    });
    els.serviceModalStartBtn.onclick = () => {
      closeModal(els.serviceCommandModal);
      sendServiceCommand(command);
    };
    openModal(els.serviceCommandModal);
  }

  function bindHoldToConfirm(button, callback) {
    let timer = null;
    const textNode = Array.from(button.childNodes).find((node) => node.nodeType === Node.TEXT_NODE && node.textContent.trim());
    const original = textNode ? textNode.textContent.trim() : button.textContent.trim();
    const setLabel = (value) => {
      if (textNode) textNode.textContent = ` ${value}`;
      else button.textContent = value;
    };

    const reset = () => {
      if (timer) window.clearTimeout(timer);
      timer = null;
      button.classList.remove("holding");
      setLabel(original);
    };

    const start = (event) => {
      if (button.disabled || timer) return;
      event.preventDefault();
      button.classList.add("holding");
      setLabel("Basılı tut");
      timer = window.setTimeout(() => {
        timer = null;
        button.classList.remove("holding");
        setLabel(original);
        callback();
      }, 2000);
    };

    button.addEventListener("pointerdown", start);
    button.addEventListener("pointerup", reset);
    button.addEventListener("pointerleave", reset);
    button.addEventListener("pointercancel", reset);
    button.addEventListener("keydown", (event) => {
      if (event.key === " " || event.key === "Enter") start(event);
    });
    button.addEventListener("keyup", reset);
  }

  function bind() {
    els.connectBtn.addEventListener("click", connect);
    els.disconnectBtn.addEventListener("click", disconnect);
    els.readParamsBtn.addEventListener("click", requestParams);
    els.saveParamsBtn.addEventListener("click", saveParams);
    els.clearLogBtn.addEventListener("click", () => {
      els.log.textContent = "";
    });
    els.terminalPreflightBtn.addEventListener("click", () => showTerminalPane("preflight"));
    els.terminalLogBtn.addEventListener("click", () => showTerminalPane("log"));
    els.terminalToggleBtn.addEventListener("click", toggleTerminalDrawer);
    els.profileManagerBtn.addEventListener("click", () => {
      renderProfiles();
      openModal(els.profileManagerModal);
    });
    els.telemetryTestBtn.addEventListener("click", () => {
      openModal(els.telemetryTestModal);
      renderLiveTelemetry();
    });
    els.exportBtn.addEventListener("click", exportParams);
    els.importBtn.addEventListener("click", () => els.importInput.click());
    els.factoryResetBtn.addEventListener("click", applyFactoryDefaults);
    els.importInput.addEventListener("change", () => {
      if (els.importInput.files[0]) importParams(els.importInput.files[0]).catch((error) => log(error.message));
    });
    for (const button of document.querySelectorAll("[data-command]")) {
      button.addEventListener("click", () => openServiceCommandModal(button.dataset.command));
    }
    for (const button of document.querySelectorAll("[data-arm-command]")) {
      bindHoldToConfirm(button, () => sendArmCommand(button.dataset.armCommand));
    }
    els.themeToggleBtn.addEventListener("click", toggleTheme);
    els.cancelPortPickBtn.addEventListener("click", () => {
      closeModal(els.portPickerModal);
      if (window.aeropicoBridge) window.aeropicoBridge.chooseSerialPort("");
    });
    els.serviceModalCancelBtn.addEventListener("click", () => closeModal(els.serviceCommandModal));
    if (els.applyDefaultPinsBtn) {
      els.applyDefaultPinsBtn.addEventListener("click", () => {
        applyDefaultPinMap(true);
        highlightAssignedPins();
        renderPinAssignments();
        renderPinDetail();
      });
    }

    bindModals();
    bindCollapsibles();
    bindBaudSelect();
    bindSerialBridge();
    bindNativeSerialBridge();
  }

  initTheme();
  bind();
  renderTabs();
  renderModules();
  renderSettings();
  renderCommandStatus();
  updateButtons();
  updatePortInfoDisplay();
  initPinMapper();
  setInterval(renderSummary, 1000);
  log("AeroPico Configurator hazir.");
})();
