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

  const SERVICE_LABELS = Object.freeze({
    CAL_IMU: "IMU kalibrasyon",
    CAL_MAG: "Mag kalibrasyon",
    CAL_RC: "RC kalibrasyon",
    SERVO_TEST: "Servo test",
    RC_MONITOR: "RC kanal kontrol",
    SENSOR_CHECK: "Sensor kontrol",
    PREFLIGHT_CHECK: "Preflight kontrol"
  });

  const MAV_CMD_COMPONENT_ARM_DISARM = 400;
  const COMMAND_BUSY_MS = 2500;
  const COMMAND_ACK_TIMEOUT_MS = 5000;
  const MAV_SENSOR_BITS = Object.freeze({
    gyro: 1 << 0,
    accel: 1 << 1,
    mag: 1 << 2,
    pressure: 1 << 3,
    gps: 1 << 5
  });
  const PIN_ROLE_PARAMS = Object.freeze({
    "Servo 1": "PIN_AIL",
    "Servo 2": "PIN_ELE",
    "Servo 3": "PIN_RUD",
    "ESC": "PIN_THR",
    "ADC Batarya Voltaj": "PIN_BATT_ADC",
    "I2C SDA": "PIN_I2C_SDA",
    "I2C SCL": "PIN_I2C_SCL",
    "Buzzer": "PIN_BUZZER"
  });
  const SERVO_RESERVED_GPIOS = new Set([1, 4, 5, 20, 21, 22]);
  const I2C0_SDA_GPIOS = new Set([4, 8, 16]);
  const I2C0_SCL_GPIOS = new Set([5, 9, 17]);
  const BUZZER_RESERVED_GPIOS = new Set([1, 4, 5, 20, 21, 26]);
  const MODULE_TYPE_OPTIONS = Object.freeze({
    TYPE_IMU: [[0, "Auto"], [1, "MPU6050"]],
    TYPE_BARO: [[0, "Auto"], [1, "BMP180/BMP085"]],
    TYPE_MAG: [[0, "Auto"], [1, "HMC5883L"], [2, "QMC5883"]],
    TYPE_GPS: [[0, "Auto"], [1, "NMEA UART"]],
    TYPE_RC: [[0, "Auto"], [1, "SBUS"]],
    TYPE_BATT: [[0, "Yok"], [1, "ADC voltaj"]]
  });
  const MODULE_SETUP_ITEMS = Object.freeze([
    {
      id: "imu",
      title: "IMU",
      role: "Attitude referansi",
      pins: "I2C bus",
      typeParam: "TYPE_IMU",
      note: "IMU uçuş için zorunludur; type seçimi firmware preflight setup doğrulamasına girer."
    },
    {
      id: "baro",
      title: "Barometre",
      enableParam: "EN_BARO",
      role: "Irtifa kestirimi",
      pins: "I2C bus",
      typeParam: "TYPE_BARO",
      note: "BMP180/BMP085 backend'i. Disable edilirse SYS_STATUS baro capability raporlamaz."
    },
    {
      id: "mag",
      title: "Manyetometre",
      enableParam: "EN_MAG",
      role: "Heading / yaw referansi",
      pins: "I2C bus",
      typeParam: "TYPE_MAG",
      note: "HMC/QMC uyumlu backend. Sabit kanat stabilize testinde opsiyonel tutulabilir."
    },
    {
      id: "gps",
      title: "GPS",
      enableParam: "EN_GPS",
      role: "Konum telemetrisi",
      pins: "UART GPS",
      typeParam: "TYPE_GPS",
      note: "GPS akisi etkinse SYS_STATUS GPS capability ve GPS_RAW_INT anlamli hale gelir."
    },
    {
      id: "rc",
      title: "RC Alıcı",
      enableParam: "EN_RC",
      role: "Pilot komutu",
      pins: "SBUS RX",
      typeParam: "TYPE_RC",
      note: "Kapalı seçilirse RC ve RC failsafe arm checklistinden çıkarılır. Etkinse gerçek protokol SBUS'tur."
    },
    {
      id: "servo",
      title: "Servo / ESC",
      role: "Actuator output",
      pins: "Pin Mapper",
      note: "Servo ve throttle pinleri burada degil, Pin Mapper uzerinden atanir. Setup bu rolu sadece actuator health olarak izler."
    },
    {
      id: "battery",
      title: "Batarya",
      enableParam: "EN_BATT",
      role: "Voltaj izleme",
      pins: "GP26-GP28",
      typeParam: "TYPE_BATT",
      pinParams: [["PIN_BATT_ADC", "ADC Pin"]],
      note: "Yok seçilirse batarya preflight/arm engeli olmaz; ADC seçilirse voltaj sağlıklı olmalıdır."
    }
  ]);
  const PROFILE_STORAGE_KEY = "aeropico-param-profiles-v1";

  const DEFAULT_WIRING = Object.freeze([
    [6, "I2C SDA"],
    [7, "I2C SCL"],
    [32, "Buzzer"],
    [30, "ADC Batarya Voltaj"],
    [40, "Servo 1"],
    [39, "Servo 2"],
    [37, "Servo 3"],
    [36, "ESC"]
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
    PIN_AIL: { min: 0, max: 28, step: 1, integer: true },
    PIN_ELE: { min: 0, max: 28, step: 1, integer: true },
    PIN_RUD: { min: 0, max: 28, step: 1, integer: true },
    PIN_THR: { min: 0, max: 28, step: 1, integer: true },
    PIN_BATT_ADC: { min: 26, max: 28, step: 1, integer: true },
    PIN_I2C_SDA: { min: 0, max: 28, step: 1, integer: true },
    PIN_I2C_SCL: { min: 0, max: 28, step: 1, integer: true },
    PIN_BUZZER: { min: 0, max: 28, step: 1, integer: true },
    EN_BARO: { min: 0, max: 1, step: 1, integer: true },
    EN_MAG: { min: 0, max: 1, step: 1, integer: true },
    EN_GPS: { min: 0, max: 1, step: 1, integer: true },
    EN_BATT: { min: 0, max: 1, step: 1, integer: true },
    EN_RC: { min: 0, max: 1, step: 1, integer: true },
    TYPE_IMU: { min: 0, max: 1, step: 1, integer: true },
    TYPE_BARO: { min: 0, max: 1, step: 1, integer: true },
    TYPE_MAG: { min: 0, max: 2, step: 1, integer: true },
    TYPE_GPS: { min: 0, max: 1, step: 1, integer: true },
    TYPE_RC: { min: 0, max: 1, step: 1, integer: true },
    TYPE_BATT: { min: 0, max: 1, step: 1, integer: true },
    MAV_ATT_HZ: { min: 0, max: 100, step: 1, integer: true },
    MAV_RC_HZ: { min: 0, max: 100, step: 1, integer: true },
    MAV_SYS_HZ: { min: 0, max: 50, step: 1, integer: true },
    BB_LOG_HZ: { min: 0, max: 500, step: 1, integer: true }
  });

  /* ── Pin Mapper data — Raspberry Pi Pico 2 (RP2350) pinout ──
     Physical layout matches original Pico: 40 castellated edge
     pins, 20 per side. Right column numbering wraps from the
     bottom, so pin 20 (bottom-left) sits opposite pin 21
     (bottom-right). */
  const PIN_ROLES = [
    "Kullanılmıyor",
    "Servo 1", "Servo 2", "Servo 3", "ESC",
    "ADC Batarya Voltaj",
    "I2C SDA", "I2C SCL",
    "Buzzer"
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
    connected: false,
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
    lastCommandAtMs: 0,
    pendingCommand: null,
    commandHistory: [],
    mavlinkHistory: [],
    firmwareVersion: null,
    i2cDiagnostics: {
      ack: new Set(),
      reg: new Set(),
      ids: {},
      lastText: "",
      lastSeenMs: 0,
      lastGoodMs: 0,
      lastBadMs: 0,
      lastBadText: ""
    },
    lastSysStatus: null,
    lastPreflightText: "",
    lastHeartbeatMs: 0,
    portDisplay: { name: null, vid: null, pid: null },
    activeBaud: null,
    selectedPin: null,
    pinMap: new Map(), // pinNumber -> role string
    dirtyParams: new Map(),
    txQueue: [],
    txBusy: false
  };

  const encoder = new window.AeroPicoMavlink.Encoder();
  const parser = new window.AeroPicoMavlink.Parser(handleMavlinkMessage);

  const els = {
    splashScreen: document.getElementById("splashScreen"),
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
    moduleSetupGrid: document.getElementById("moduleSetupGrid"),
    moduleSummary: document.getElementById("moduleSummary"),
    preflightText: document.getElementById("preflightText"),
    log: document.getElementById("log"),
    clearLogBtn: document.getElementById("clearLogBtn"),
    exportBtn: document.getElementById("exportBtn"),
    importBtn: document.getElementById("importBtn"),
    importInput: document.getElementById("importInput"),
    themeToggleBtn: document.getElementById("themeToggleBtn"),
    pinMapperBtn: document.getElementById("pinMapperBtn"),
    profileManagerBtn: document.getElementById("profileManagerBtn"),
    applyDirtyParamsBtn: document.getElementById("applyDirtyParamsBtn"),
    profileModal: document.getElementById("profileModal"),
    profileNameInput: document.getElementById("profileNameInput"),
    saveProfileBtn: document.getElementById("saveProfileBtn"),
    profileList: document.getElementById("profileList"),
    pinMapperModal: document.getElementById("pinMapperModal"),
    portPickerModal: document.getElementById("portPickerModal"),
    portPickerList: document.getElementById("portPickerList"),
    cancelPortPickBtn: document.getElementById("cancelPortPickBtn"),
    pinBoard: document.getElementById("pinBoard"),
    pinDetail: document.getElementById("pinDetail"),
    pinAssignmentList: document.getElementById("pinAssignmentList"),
    configAudit: document.getElementById("configAudit"),
    applyDefaultPinsBtn: document.getElementById("applyDefaultPinsBtn"),
    applyDefaultModuleSetupBtn: document.getElementById("applyDefaultModuleSetupBtn"),
    openPinsFromModuleSetupBtn: document.getElementById("openPinsFromModuleSetupBtn"),
    linkSummary: document.getElementById("linkSummary"),
    paramSummary: document.getElementById("paramSummary"),
    moduleSummaryTop: document.getElementById("moduleSummaryTop"),
    heartbeatSummary: document.getElementById("heartbeatSummary"),
    firmwareSummary: document.getElementById("firmwareSummary"),
    armSummary: document.getElementById("armSummary"),
    commandSummary: document.getElementById("commandSummary"),
    commandStatusList: document.getElementById("commandStatusList"),
    mavlinkInspectorSummary: document.getElementById("mavlinkInspectorSummary"),
    mavlinkInspectorList: document.getElementById("mavlinkInspectorList"),
    servoTestSurface: document.getElementById("servoTestSurface"),
    servoTestPulse: document.getElementById("servoTestPulse"),
    servoTestPulseVal: document.getElementById("servoTestPulseVal"),
    servoTestDuration: document.getElementById("servoTestDuration"),
    terminalPreflightBtn: document.getElementById("terminalPreflightBtn"),
    terminalArmChecklistBtn: document.getElementById("terminalArmChecklistBtn"),
    terminalI2cBtn: document.getElementById("terminalI2cBtn"),
    terminalLogBtn: document.getElementById("terminalLogBtn"),
    terminalCommandBtn: document.getElementById("terminalCommandBtn"),
    terminalMavlinkBtn: document.getElementById("terminalMavlinkBtn"),
    preflightPane: document.getElementById("preflightPane"),
    armChecklistPane: document.getElementById("armChecklistPane"),
    i2cPane: document.getElementById("i2cPane"),
    armChecklist: document.getElementById("armChecklist"),
    i2cSummary: document.getElementById("i2cSummary"),
    i2cDiagnosticList: document.getElementById("i2cDiagnosticList"),
    logPane: document.getElementById("logPane"),
    commandPane: document.getElementById("commandPane"),
    mavlinkPane: document.getElementById("mavlinkPane"),
    toastStack: document.getElementById("toastStack")
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

  function toast(message, kind = "info") {
    if (!els.toastStack) return;
    const item = document.createElement("div");
    item.className = `toast ${kind}`;
    item.textContent = message;
    els.toastStack.appendChild(item);
    window.setTimeout(() => item.classList.add("leaving"), 3200);
    window.setTimeout(() => item.remove(), 3800);
  }

  function setLinkStatus(text, cls) {
    els.linkStatus.className = `status-pill ${cls}`;
    els.linkStatus.textContent = text;
    setStatusValue(els.linkSummary, cls, text);
  }

  function setStatusValue(el, stateName, text) {
    if (!el) return;
    if (el.textContent !== text) {
      el.textContent = text;
      el.classList.remove("value-pop");
      void el.offsetWidth;
      el.classList.add("value-pop");
    }
    el.dataset.state = stateName;
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
    els.profileManagerBtn.addEventListener("click", () => {
      renderProfiles();
      openModal(els.profileModal);
    });
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

  function bindSideToolTabs() {
    document.querySelectorAll("[data-tool-tab]").forEach((button) => {
      button.addEventListener("click", () => {
        const target = button.dataset.toolTab;
        document.querySelectorAll("[data-tool-tab]").forEach((tab) => {
          tab.classList.toggle("active", tab.dataset.toolTab === target);
        });
        document.querySelectorAll("[data-tool-pane]").forEach((pane) => {
          pane.classList.toggle("active", pane.dataset.toolPane === target);
        });
      });
    });
  }

  function bindModuleTabs() {
    document.querySelectorAll("[data-module-tab]").forEach((button) => {
      button.addEventListener("click", () => {
        const target = button.dataset.moduleTab;
        document.querySelectorAll("[data-module-tab]").forEach((tab) => {
          tab.classList.toggle("active", tab.dataset.moduleTab === target);
        });
        document.querySelectorAll("[data-module-pane]").forEach((pane) => {
          pane.classList.toggle("active", pane.dataset.modulePane === target);
        });
      });
    });
  }

  function bindFlowTabs() {
    document.querySelectorAll("[data-flow-tab]").forEach((button) => {
      button.addEventListener("click", () => {
        const target = button.dataset.flowTab;
        document.querySelectorAll("[data-flow-tab]").forEach((tab) => {
          tab.classList.toggle("active", tab.dataset.flowTab === target);
        });
        document.querySelectorAll("[data-flow-pane]").forEach((pane) => {
          pane.classList.toggle("active", pane.dataset.flowPane === target);
        });
      });
    });
  }

  function bindRightbarTabs() {
    document.querySelectorAll("[data-rightbar-tab]").forEach((button) => {
      button.addEventListener("click", () => {
        const target = button.dataset.rightbarTab;
        document.querySelectorAll("[data-rightbar-tab]").forEach((tab) => {
          tab.classList.toggle("active", tab.dataset.rightbarTab === target);
        });
        document.querySelectorAll("[data-rightbar-pane]").forEach((pane) => {
          pane.classList.toggle("active", pane.dataset.rightbarPane === target);
        });
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
      card.className = `module ${value === "ok" ? "ok" : value === "bad" ? "bad" : value === "detected" ? "detected" : ""}`;
      const icon = MODULE_ICONS[id] || "";
      const enableParam = moduleEnableParam(id);
      const enabled = !enableParam || getConfigValue(enableParam, 1) >= 0.5;
      card.classList.toggle("disabled", !enabled);
      card.innerHTML = `
        <div class="module-main">
          <strong>${icon} ${label}</strong>
          <span>${desc}<br>${enabled ? moduleText(value) : "Devre dışı"}</span>
        </div>
        ${enableParam ? `
          <label class="module-enable-toggle" title="${enableParam}">
            <input type="checkbox" data-module-enable="${enableParam}" ${enabled ? "checked" : ""} ${state.connected ? "" : "disabled"}>
            <span>Etkin</span>
          </label>` : ""}
      `;
      els.moduleGrid.appendChild(card);
      if (enabled && value === "ok") okCount++;
    }
    els.moduleGrid.querySelectorAll("[data-module-enable]").forEach((input) => {
      input.addEventListener("change", () => {
        const next = input.checked ? 1 : 0;
        if (!setParam(input.dataset.moduleEnable, next)) {
          input.checked = !input.checked;
        }
      });
    });
    els.moduleSummary.textContent = okCount === 0 ? "Bekliyor" : `${okCount}/${MODULES.length}`;
    els.moduleSummary.className = `status-pill ${okCount > 0 ? "ok" : "muted"}`;
    setStatusValue(els.moduleSummaryTop, okCount > 0 ? "ok" : "muted", okCount === 0 ? "Bekliyor" : `${okCount}/${MODULES.length} hazır`);
    renderModuleSetup();
    renderArmChecklist();
  }

  function renderModuleSetup() {
    if (!els.moduleSetupGrid) return;
    els.moduleSetupGrid.innerHTML = "";
    MODULE_SETUP_ITEMS.forEach((item) => {
      const card = document.createElement("div");
      const isDisabled = item.enableParam && getConfigValue(item.enableParam, 1) < 0.5;
      const health = isDisabled
        ? "disabled"
        : item.id === "servo" ? actuatorSetupHealth() : item.id === "battery" ? state.modules.battery : state.modules[item.id];
      card.className = `module-setup-card ${health === "ok" ? "ok" : health === "bad" ? "bad" : health === "detected" ? "detected" : ""}`;
      const enableControl = item.enableParam ? renderModuleEnableControl(item.enableParam) : "";
      const typeControl = item.typeParam ? renderModuleTypeControl(item.typeParam) : "";
      const pinControls = item.pinParams ? renderModulePinControls(item.pinParams) : "";
      const pinMapperAction = item.id === "servo"
        ? '<button class="small module-pinmapper-action" data-open-pinmapper-from-setup type="button">Pin Mapper ile Ayarla</button>'
        : "";
      card.innerHTML = `
        <div class="module-setup-head">
          <div>
            <strong>${item.title}</strong>
            <span>${item.role}</span>
          </div>
          <span class="module-setup-health">${moduleText(health)}</span>
        </div>
        <div class="module-setup-meta">
          <span>${item.pins}</span>
          ${enableControl}
        </div>
        ${typeControl}
        ${pinControls}
        ${pinMapperAction}
        <p>${item.note}</p>
      `;
      els.moduleSetupGrid.appendChild(card);
    });

    els.moduleSetupGrid.querySelectorAll("[data-open-pinmapper-from-setup]").forEach((button) => {
      button.addEventListener("click", () => openModal(els.pinMapperModal));
    });

    els.moduleSetupGrid.querySelectorAll("[data-setup-enable]").forEach((input) => {
      input.addEventListener("change", () => {
        const next = input.checked ? 1 : 0;
        if (!stageParam(input.dataset.setupEnable, next)) {
          input.checked = !input.checked;
          return;
        }
        toast("Modül ayarı hazır. Göndermek için Değişenleri Uygula.", "info");
        renderModules();
      });
    });

    els.moduleSetupGrid.querySelectorAll("[data-setup-pin]").forEach((select) => {
      select.addEventListener("change", () => {
        const paramName = select.dataset.setupPin;
        const value = Number(select.value);
        if (!validatePinRoleGpio(paramName, value)) {
          const current = getParamValue(paramName, "");
          select.value = String(current);
          return;
        }
        if (!stageParam(paramName, value)) {
          const current = getParamValue(paramName, "");
          select.value = String(current);
          return;
        }
        toast("Pin ayarı hazır. Uygula + Flash'a Kaydet + reboot.", "warn");
        renderModules();
      });
    });

    els.moduleSetupGrid.querySelectorAll("[data-setup-type]").forEach((select) => {
      select.addEventListener("change", () => {
        const paramName = select.dataset.setupType;
        const value = Number(select.value);
        if (!stageParam(paramName, value)) {
          select.value = String(getConfigValue(paramName, ""));
          return;
        }
        toast("Modül tipi hazır. Arm akışı uygulamadan sonra firmware tarafında değişir.", "info");
        renderModules();
      });
    });
  }

  function renderModuleEnableControl(paramName) {
    const enabled = getConfigValue(paramName, 1) >= 0.5;
    return `
      <label class="setup-switch" title="${paramName}">
        <input type="checkbox" data-setup-enable="${paramName}" ${enabled ? "checked" : ""}>
        <span>${enabled ? "Etkin" : "Kapalı"}</span>
      </label>
    `;
  }

  function renderModuleTypeControl(paramName) {
    const current = getConfigValue(paramName, 0);
    const options = MODULE_TYPE_OPTIONS[paramName] || [];
    return `
      <label class="module-type-control">
        <span>Tip</span>
        <select data-setup-type="${paramName}">
          ${options.map(([value, label]) => `
            <option value="${value}" ${Number(current) === value ? "selected" : ""}>${label}</option>
          `).join("")}
        </select>
      </label>
    `;
  }

  function renderModulePinControls(pinParams) {
    return `
      <div class="module-pin-controls">
        ${pinParams.map(([paramName, label]) => `
          <label>
            <span>${label}</span>
            <select data-setup-pin="${paramName}">
              ${pinOptionsForParam(paramName)}
            </select>
          </label>
        `).join("")}
      </div>
    `;
  }

  function pinOptionsForParam(paramName) {
    const current = getConfigValue(paramName, "");
    const options = validGpiosForPinParam(paramName).map((gpio) => {
      const label = paramName === "PIN_BATT_ADC" ? `GP${gpio} / ADC${gpio - 26}` : `GP${gpio}`;
      return `<option value="${gpio}" ${Number(current) === gpio ? "selected" : ""}>${label}</option>`;
    });
    if (current === "") options.unshift('<option value="" selected disabled>Parametre bekleniyor</option>');
    return options.join("");
  }

  function validGpiosForPinParam(paramName) {
    if (paramName === "PIN_BATT_ADC") return [26, 27, 28];
    if (paramName === "PIN_I2C_SDA") return [...I2C0_SDA_GPIOS];
    if (paramName === "PIN_I2C_SCL") return [...I2C0_SCL_GPIOS];
    if (paramName === "PIN_BUZZER") {
      const pins = [];
      for (let gpio = 0; gpio <= 28; gpio++) {
        if (!BUZZER_RESERVED_GPIOS.has(gpio)) pins.push(gpio);
      }
      return pins;
    }

    const i2cSda = Number(getConfigValue("PIN_I2C_SDA", 4));
    const i2cScl = Number(getConfigValue("PIN_I2C_SCL", 5));
    const buzzer = Number(getConfigValue("PIN_BUZZER", 22));
    const pins = [];
    for (let gpio = 0; gpio <= 28; gpio++) {
      if (SERVO_RESERVED_GPIOS.has(gpio)) continue;
      if (gpio === i2cSda || gpio === i2cScl || gpio === buzzer) continue;
      pins.push(gpio);
    }
    return pins;
  }

  function validatePinRoleGpio(paramName, gpio) {
    if (!validGpiosForPinParam(paramName).includes(gpio)) {
      const details = {
        PIN_BATT_ADC: "Batarya ADC icin GP26-GP28 secilmeli.",
        PIN_I2C_SDA: "I2C SDA icin desteklenen i2c0 SDA pinleri: GP4, GP8, GP16.",
        PIN_I2C_SCL: "I2C SCL icin desteklenen i2c0 SCL pinleri: GP5, GP9, GP17.",
        PIN_BUZZER: "Buzzer SBUS/I2C/admin/batarya ADC hatlariyla cakismamali."
      };
      toast(details[paramName] || "Bu GPIO bu rol icin uygun degil.", "warn");
      log(`${paramName} reddedildi: GP${gpio} bu rol icin desteklenmiyor.`);
      return false;
    }

    if (paramName === "PIN_I2C_SDA") {
      const scl = Number(getConfigValue("PIN_I2C_SCL", 5));
      if (Number.isFinite(scl) && scl !== gpio + 1) {
        toast("I2C SDA/SCL ayni i2c0 ciftinden olmali.", "warn");
        log(`PIN_I2C_SDA reddedildi: GP${gpio} mevcut SCL GP${scl} ile cift degil.`);
        return false;
      }
    }
    if (paramName === "PIN_I2C_SCL") {
      const sda = Number(getConfigValue("PIN_I2C_SDA", 4));
      if (Number.isFinite(sda) && gpio !== sda + 1) {
        toast("I2C SDA/SCL ayni i2c0 ciftinden olmali.", "warn");
        log(`PIN_I2C_SCL reddedildi: GP${gpio} mevcut SDA GP${sda} ile cift degil.`);
        return false;
      }
    }
    return true;
  }

  function actuatorSetupHealth() {
    const required = ["PIN_AIL", "PIN_ELE", "PIN_RUD", "PIN_THR"];
    const values = required.map((name) => getConfigValue(name, null));
    if (values.some((value) => value === null)) return "unknown";
    return new Set(values).size === values.length ? "ok" : "bad";
  }

  function moduleEnableParam(id) {
    if (id === "baro") return "EN_BARO";
    if (id === "mag") return "EN_MAG";
    if (id === "gps") return "EN_GPS";
    if (id === "rc") return "EN_RC";
    if (id === "battery") return "EN_BATT";
    return null;
  }

  function getParamValue(name, fallback = null) {
    const param = state.params.get(name);
    return param && Number.isFinite(param.value) ? param.value : fallback;
  }

  function getConfigValue(name, fallback = null) {
    if (state.dirtyParams.has(name)) return state.dirtyParams.get(name);
    return getParamValue(name, fallback);
  }

  function stageParam(name, value) {
    const validation = validateParam(name, value);
    if (!validation.ok) {
      log(`${name}: ${validation.reason}`);
      return false;
    }
    const param = state.params.get(name);
    const unchanged = param && Math.abs(validation.value - param.value) < 1e-6;
    if (unchanged) {
      state.dirtyParams.delete(name);
    } else {
      state.dirtyParams.set(name, validation.value);
    }
    updateDirtyButton();
    renderSummary();
    return true;
  }

  function moduleText(value) {
    if (value === "disabled") return "Devre dışı";
    if (value === "ok") return "✓ Algılandı";
    if (value === "detected") return "• I2C'de görüldü";
    if (value === "bad") return "✗ Yok / pasif";
    return "— Bilinmiyor";
  }

  function renderSettings() {
    const group = PARAM_GROUPS.find((item) => item.id === state.activeGroup);
    document.getElementById("sectionTitle").textContent = group.label;
    els.settingsGrid.dataset.group = group.id;
    els.settingsGrid.innerHTML = "";
    for (const [name, label, description] of group.params) {
      const param = state.params.get(name);
      const hasDraft = state.dirtyParams.has(name);
      const displayValue = hasDraft ? state.dirtyParams.get(name) : (param ? param.value : null);
      const card = document.createElement("article");
      card.className = `setting-card ${hasDraft ? "dirty" : ""}`;
      const meta = param
        ? `#${param.index + 1}/${param.count || state.expectedParamCount || "?"}`
        : hasDraft ? "Gönderilmedi" : "Okunmadı";

      const header = document.createElement("header");
      const title = document.createElement("h3");
      title.textContent = label;
      const nameMeta = document.createElement("span");
      nameMeta.className = "value-meta";
      nameMeta.textContent = name;
      header.append(title, nameMeta);

      const desc = document.createElement("p");
      desc.textContent = description;

      const footer = document.createElement("footer");
      const input = document.createElement("input");
      input.type = "number";
      input.placeholder = "Değer yok";
      const rule = PARAM_RULES[name] || { step: 0.001 };
      input.step = String(rule.step || 0.001);
      if (Number.isFinite(rule.min)) input.min = String(rule.min);
      if (Number.isFinite(rule.max)) input.max = String(rule.max);
      if (displayValue !== null) input.value = String(displayValue);

      const button = document.createElement("button");
      button.textContent = "Yaz";
      button.disabled = !state.connected;
      footer.append(input, button);

      const visual = createParamVisual(name, displayValue);

      const metaEl = document.createElement("div");
      metaEl.className = "value-meta";
      metaEl.textContent = meta;

      card.append(header, desc);
      if (visual) card.appendChild(visual);
      card.append(footer, metaEl);
      input.addEventListener("input", () => {
        const next = Number(input.value);
        const validation = validateParam(name, next);
        input.classList.toggle("invalid", !validation.ok);
        const unchanged = param && validation.ok && Math.abs(validation.value - param.value) < 1e-6;
        card.classList.toggle("dirty", validation.ok && !unchanged);
        if (validation.ok && !unchanged) {
          state.dirtyParams.set(name, validation.value);
          updateParamVisual(visual, name, validation.value);
        } else {
          state.dirtyParams.delete(name);
          if (validation.ok) updateParamVisual(visual, name, validation.value);
        }
        updateDirtyButton();
      });
      button.addEventListener("click", () => {
        if (setParam(name, Number(input.value))) {
          state.dirtyParams.delete(name);
          updateDirtyButton();
        }
      });
      els.settingsGrid.appendChild(card);
    }
    renderConfigAudit();
    updateDirtyButton();
  }

  function createParamVisual(name, rawValue) {
    const html = renderParamVisualHtml(name, rawValue);
    if (!html) return null;
    const visual = document.createElement("div");
    visual.className = `setting-visual ${visualClassForParam(name)}`;
    visual.innerHTML = html;
    return visual;
  }

  function updateParamVisual(visual, name, rawValue) {
    if (!visual) return;
    visual.innerHTML = renderParamVisualHtml(name, rawValue);
  }

  function renderParamVisualHtml(name, rawValue) {
    if (isPidParam(name)) return renderResponseCurveSvg(name, rawValue);
    if (isServoRangeParam(name)) return renderServoRangeVisual(name, rawValue);
    if (isTrimParam(name)) return renderTrimVisual(name, rawValue);
    if (name.startsWith("BATT_")) return renderBatteryVisual(name, rawValue);
    if (name.startsWith("MIX_")) return renderMixerVisual(name, rawValue);
    if (name.startsWith("RC_")) return renderRcChannelVisual(name, rawValue);
    if (name === "FS_TIMEOUT" || name === "PREF_Q_MIN") return renderSafetyVisual(name, rawValue);
    if (name.startsWith("MAV_") || name === "BB_LOG_HZ") return renderTelemetryVisual(name, rawValue);
    return "";
  }

  function visualClassForParam(name) {
    if (isPidParam(name)) return "setting-curve";
    if (isServoRangeParam(name) || isTrimParam(name)) return "setting-range";
    if (name.startsWith("BATT_")) return "setting-battery";
    if (name.startsWith("MIX_")) return "setting-mixer";
    if (name.startsWith("RC_")) return "setting-rc";
    if (name === "FS_TIMEOUT" || name === "PREF_Q_MIN") return "setting-safety";
    if (name.startsWith("MAV_") || name === "BB_LOG_HZ") return "setting-telemetry";
    return "";
  }

  function isPidParam(name) {
    return name.startsWith("ANGLE_") || name.startsWith("RATE_");
  }

  function isServoRangeParam(name) {
    return name === "SERVO_MIN" || name === "SERVO_MAX";
  }

  function isTrimParam(name) {
    return name.startsWith("TRIM_");
  }

  function renderResponseCurveSvg(name, rawValue) {
    const value = Number.isFinite(rawValue) ? Number(rawValue) : 0;
    const rule = PARAM_RULES[name] || { min: 0, max: 1 };
    const min = Number.isFinite(rule.min) ? rule.min : 0;
    const max = Number.isFinite(rule.max) && rule.max > min ? rule.max : Math.max(1, value || 1);
    const norm = Math.max(0, Math.min(1, (value - min) / (max - min)));
    const damping = name.includes("_D") ? 0.55 : name.includes("_I") ? 0.35 : 0.72;
    const gain = 0.18 + norm * 0.70;
    const points = [];
    for (let i = 0; i < 24; i++) {
      const t = i / 23;
      const response = 1 - Math.exp(-t * (2.2 + gain * 4.2)) * Math.cos(t * Math.PI * (1.2 + gain) * damping);
      const x = 4 + t * 92;
      const y = 34 - Math.max(0, Math.min(1.25, response)) * 24;
      points.push(`${x.toFixed(1)},${y.toFixed(1)}`);
    }
    return `<svg viewBox="0 0 100 40" aria-hidden="true">
      <path class="curve-grid" d="M4 34H96M4 10H96" />
      <polyline class="curve-line" points="${points.join(" ")}" />
    </svg>`;
  }

  function renderServoRangeVisual(name, rawValue) {
    const value = Number.isFinite(rawValue) ? Number(rawValue) : (name === "SERVO_MIN" ? 1000 : 2000);
    const domainMin = 800;
    const domainMax = 2200;
    const safeMin = paramValue("SERVO_MIN");
    const safeMax = paramValue("SERVO_MAX");
    const minUs = Number.isFinite(safeMin) ? safeMin : 1000;
    const maxUs = Number.isFinite(safeMax) ? safeMax : 2000;
    const left = percentInRange(minUs, domainMin, domainMax);
    const right = percentInRange(maxUs, domainMin, domainMax);
    const marker = percentInRange(value, domainMin, domainMax);
    return `<div class="range-scale" aria-hidden="true">
      <span>800</span><span>1500</span><span>2200 µs</span>
      <i class="range-window" style="left:${left}%;width:${Math.max(2, right - left)}%"></i>
      <i class="range-marker" style="left:${marker}%"></i>
    </div>
    <div class="range-caption"><strong>${Math.round(value)} µs</strong><span>${name === "SERVO_MIN" ? "minimum pulse" : "maximum pulse"}</span></div>`;
  }

  function renderTrimVisual(name, rawValue) {
    const value = Number.isFinite(rawValue) ? Number(rawValue) : 0;
    const marker = percentInRange(value, -400, 400);
    return `<div class="trim-scale" aria-hidden="true">
      <span>-400</span><span>0</span><span>+400</span>
      <i class="trim-zero"></i>
      <i class="range-marker" style="left:${marker}%"></i>
    </div>
    <div class="range-caption"><strong>${Math.round(value)} µs</strong><span>trim offset</span></div>`;
  }

  function renderBatteryVisual(name, rawValue) {
    if (name === "BATT_CELLS") return renderBatteryCellVisual(rawValue);
    if (name === "BATT_CAP_MAH") return renderBatteryCapacityVisual(rawValue);
    if (name === "BATT_C_RATE") return renderBatteryCRateVisual(rawValue);
    if (name === "BATT_NOM_V" || name === "BATT_LOW_V" || name === "BATT_BRN_V") {
      return renderBatteryVoltageVisual(name, rawValue);
    }
    return "";
  }

  function batteryCellCount(rawCells) {
    const cellsFromState = paramValue("BATT_CELLS");
    return clampInteger(Number.isFinite(rawCells) ? rawCells : cellsFromState, 1, 6, 3);
  }

  function renderBatteryCellVisual(rawValue) {
    const cellCount = batteryCellCount(rawValue);
    const cells = Array.from({ length: cellCount }, (_, index) => `<i class="ok">${index + 1}</i>`).join("");
    return `<div class="battery-cells" style="grid-template-columns:repeat(${cellCount}, minmax(0, 1fr))" aria-label="${cellCount}S battery">${cells}</div>
    <div class="range-caption"><strong>${cellCount}S</strong><span>${cellCount} hucre seri paket</span></div>`;
  }

  function renderBatteryVoltageVisual(name, rawValue) {
    const cellsFromState = paramValue("BATT_CELLS");
    const cellCount = batteryCellCount(cellsFromState);
    const fallback = name === "BATT_NOM_V" ? cellCount * 3.7 : name === "BATT_LOW_V" ? cellCount * 3.5 : cellCount * 3.3;
    const volts = Number.isFinite(rawValue) ? Number(rawValue) : fallback;
    const perCell = volts / Math.max(1, cellCount);
    const stateClass = perCell < 3.35 ? "bad" : perCell < 3.55 ? "warn" : "ok";
    const left = percentInRange(perCell, 3.0, 4.25);
    const low = percentInRange(3.5, 3.0, 4.25);
    const brownout = percentInRange(3.3, 3.0, 4.25);
    const label = name === "BATT_NOM_V" ? "nominal" : name === "BATT_LOW_V" ? "low threshold" : "brownout threshold";
    return `<div class="battery-meter ${stateClass}" aria-hidden="true">
      <span>3.0</span><span>3.7</span><span>4.2 V/cell</span>
      <i class="battery-fill" style="width:${left}%"></i>
      <i class="battery-threshold low" style="left:${low}%"></i>
      <i class="battery-threshold brownout" style="left:${brownout}%"></i>
    </div>
    <div class="range-caption"><strong>${volts.toFixed(1)} V</strong><span>${perCell.toFixed(2)} V/cell ${label}</span></div>`;
  }

  function renderBatteryCapacityVisual(rawValue) {
    const value = Number.isFinite(rawValue) ? Number(rawValue) : 3300;
    const fill = percentInRange(value, 500, 10000);
    return `<div class="battery-capacity" aria-hidden="true">
      <i class="battery-capacity-fill" style="width:${fill}%"></i>
      <span>${Math.round(value).toLocaleString("tr-TR")}</span>
    </div>
    <div class="range-caption"><strong>${Math.round(value).toLocaleString("tr-TR")} mAh</strong><span>paket kapasitesi</span></div>`;
  }

  function renderBatteryCRateVisual(rawValue) {
    const cRate = Number.isFinite(rawValue) ? Number(rawValue) : 40;
    const capacity = paramValue("BATT_CAP_MAH");
    const capacityAh = Number.isFinite(capacity) ? capacity / 1000 : 3.3;
    const continuousA = Math.max(0, cRate * capacityAh);
    const bars = 6;
    const lit = clampInteger(Math.ceil(percentInRange(cRate, 1, 120) / (100 / bars)), 1, bars, 2);
    const cells = Array.from({ length: bars }, (_, index) => `<i class="${index < lit ? "ok" : ""}"></i>`).join("");
    return `<div class="battery-c-rate" aria-hidden="true">${cells}</div>
    <div class="range-caption"><strong>${Math.round(cRate)}C</strong><span>~${Math.round(continuousA)} A surekli</span></div>`;
  }

  function renderMixerVisual(name, rawValue) {
    const value = Number.isFinite(rawValue) ? Number(rawValue) : 1;
    const width = percentInRange(value, 0, 2);
    const axis = name === "MIX_ROLL" ? "AIL" : name === "MIX_PITCH" ? "ELE" : "RUD";
    const left = Math.max(8, 50 - width * 0.32);
    const right = Math.min(92, 50 + width * 0.32);
    return `<div class="mixer-visual ${name.toLowerCase()}" aria-hidden="true">
      <i class="mixer-wing"></i>
      <i class="mixer-center"></i>
      <i class="mixer-deflection left" style="left:${left}%"></i>
      <i class="mixer-deflection right" style="left:${right}%"></i>
    </div>
    <div class="range-caption"><strong>${value.toFixed(2)}x</strong><span>${axis} mixer etkisi</span></div>`;
  }

  function renderRcChannelVisual(name, rawValue) {
    const selected = clampInteger(Number.isFinite(rawValue) ? rawValue : defaultRcChannel(name), 0, 15, 0);
    const role = name.replace("RC_", "").replace("_CH", "");
    const cells = Array.from({ length: 8 }, (_, index) => {
      const channel = index + 1;
      const active = index === selected;
      return `<i class="${active ? "active" : ""}">${channel}</i>`;
    }).join("");
    return `<div class="rc-channel-strip" aria-hidden="true">${cells}</div>
    <div class="range-caption"><strong>CH${selected + 1}</strong><span>${role.toLowerCase()} girisi</span></div>`;
  }

  function defaultRcChannel(name) {
    if (name === "RC_ROLL_CH") return 0;
    if (name === "RC_PITCH_CH") return 1;
    if (name === "RC_THR_CH") return 2;
    if (name === "RC_YAW_CH") return 3;
    if (name === "RC_MODE_CH") return 4;
    return 0;
  }

  function renderSafetyVisual(name, rawValue) {
    if (name === "FS_TIMEOUT") {
      const value = Number.isFinite(rawValue) ? Number(rawValue) : 1000;
      const pct = percentInRange(value, 100, 5000);
      const stateClass = value < 350 ? "bad" : value < 700 ? "warn" : "ok";
      return `<div class="safety-timeout ${stateClass}" aria-hidden="true">
        <i class="safety-timeout-fill" style="width:${pct}%"></i>
        <span>RC loss window</span>
      </div>
      <div class="range-caption"><strong>${Math.round(value)} ms</strong><span>failsafe gecikmesi</span></div>`;
    }
    const value = Number.isFinite(rawValue) ? Number(rawValue) : 60;
    const pct = percentInRange(value, 0, 100);
    const stateClass = value < 35 ? "bad" : value < 55 ? "warn" : "ok";
    return `<div class="quality-gauge ${stateClass}" aria-hidden="true">
      <i class="quality-arc"></i>
      <i class="quality-needle" style="transform:rotate(${(-55 + pct * 1.1).toFixed(1)}deg)"></i>
    </div>
    <div class="range-caption"><strong>${Math.round(value)}%</strong><span>minimum preflight kalite</span></div>`;
  }

  function renderTelemetryVisual(name, rawValue) {
    const value = Number.isFinite(rawValue) ? Number(rawValue) : defaultTelemetryHz(name);
    const maxHz = name === "BB_LOG_HZ" ? 500 : name === "MAV_SYS_HZ" ? 50 : 100;
    const pct = percentInRange(value, 0, maxHz);
    const pulses = Math.max(1, Math.min(5, Math.ceil(pct / 20)));
    const dots = Array.from({ length: 5 }, (_, index) => `<i class="${index < pulses && value > 0 ? "active" : ""}"></i>`).join("");
    const label = name === "BB_LOG_HZ" ? "blackbox log" : "MAVLink stream";
    return `<div class="telemetry-flow" aria-hidden="true">
      <span>${dots}</span>
      <b style="width:${pct}%"></b>
    </div>
    <div class="range-caption"><strong>${Math.round(value)} Hz</strong><span>${label}</span></div>`;
  }

  function defaultTelemetryHz(name) {
    if (name === "MAV_ATT_HZ") return 50;
    if (name === "MAV_RC_HZ") return 10;
    if (name === "MAV_SYS_HZ") return 2;
    if (name === "BB_LOG_HZ") return 100;
    return 0;
  }

  function percentInRange(value, min, max) {
    if (!Number.isFinite(value) || max <= min) return 0;
    return Math.max(0, Math.min(100, ((value - min) / (max - min)) * 100));
  }

  function clampInteger(value, min, max, fallback) {
    const next = Math.round(Number(value));
    if (!Number.isFinite(next)) return fallback;
    return Math.max(min, Math.min(max, next));
  }

  function updateDirtyButton() {
    if (!els.applyDirtyParamsBtn) return;
    const count = state.dirtyParams.size;
    els.applyDirtyParamsBtn.disabled = !state.connected || count === 0;
    els.applyDirtyParamsBtn.innerHTML = `<svg class="btn-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"
      stroke-linecap="round" stroke-linejoin="round"><path d="M20 6 9 17l-5-5" /></svg>${count > 0 ? `Değişenleri Uygula (${count})` : "Değişenleri Uygula"}`;
  }

  function updateButtons() {
    const pendingAge = state.pendingCommand ? Date.now() - state.pendingCommand.atMs : 0;
    const commandBusy = state.pendingCommand && pendingAge < COMMAND_BUSY_MS;
    if (state.pendingCommand && pendingAge > COMMAND_ACK_TIMEOUT_MS + 250) {
      state.pendingCommand = null;
      state.lastCommand = null;
    }
    els.connectBtn.disabled = state.connected;
    els.disconnectBtn.disabled = !state.connected;
    els.readParamsBtn.disabled = !state.connected;
    els.saveParamsBtn.disabled = !state.connected;
    document.querySelectorAll("[data-command]").forEach((button) => {
      const command = button.dataset.command;
      const dangerousWhileArmed = command === "CAL_IMU" || command === "CAL_MAG" || command === "SERVO_TEST";
      button.disabled = !state.connected || commandBusy || (dangerousWhileArmed && state.armed === true);
    });
    document.querySelectorAll("[data-arm-command]").forEach((button) => {
      const command = button.dataset.armCommand;
      button.disabled = !state.connected ||
        commandBusy ||
        ((command === "normal" || command === "force") && state.armed === true) ||
        (command === "disarm" && state.armed !== true);
    });
    renderSettings();
    renderSummary();
  }

  function renderSummary() {
    const expected = state.expectedParamCount || 0;
    setStatusValue(
      els.paramSummary,
      state.params.size > 0 ? "ok" : "muted",
      expected > 0 ? `${state.params.size}/${expected}` : `${state.params.size} okunmuş`
    );

    if (state.lastHeartbeatMs > 0) {
      const ageSec = Math.max(0, Math.round((Date.now() - state.lastHeartbeatMs) / 1000));
      setStatusValue(els.heartbeatSummary, ageSec < 5 ? "ok" : "warn", ageSec < 2 ? "Canlı" : `${ageSec} sn önce`);
    } else {
      setStatusValue(els.heartbeatSummary, "muted", "Yok");
    }

    setStatusValue(
      els.firmwareSummary,
      state.firmwareVersion ? "ok" : "muted",
      state.firmwareVersion || "Bilinmiyor"
    );

    if (state.armed === true) {
      setStatusValue(els.armSummary, "bad", "Armed");
      document.body.classList.add("is-armed");
    } else if (state.armed === false) {
      setStatusValue(els.armSummary, "ok", "Disarmed");
      document.body.classList.remove("is-armed");
    } else {
      setStatusValue(els.armSummary, "muted", "Bilinmiyor");
      document.body.classList.remove("is-armed");
    }
    renderArmChecklist();
    renderI2cDiagnostics();
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

  function formatFirmwareVersion(encoded) {
    const value = Number(encoded) >>> 0;
    const major = (value >>> 24) & 0xff;
    const minor = (value >>> 16) & 0xff;
    const patch = (value >>> 8) & 0xff;
    const type = value & 0xff;
    const suffix = type >= 192 ? "-rc" : type >= 128 ? "-beta" : type >= 64 ? "-alpha" : "";
    return `v${major}.${minor}.${patch}${suffix}`;
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

  function finishSplash() {
    if (!els.splashScreen) return;
    window.setTimeout(() => {
      document.body.classList.remove("app-loading");
      document.body.classList.add("app-ready");
      els.splashScreen.classList.add("hidden");
      window.setTimeout(() => els.splashScreen.remove(), 360);
    }, 2020);
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

  function withTimeout(promise, ms, message, onTimeout) {
    let timer = null;
    const timeout = new Promise((_, reject) => {
      timer = window.setTimeout(() => {
        if (typeof onTimeout === "function") onTimeout();
        reject(new Error(message));
      }, ms);
    });
    return Promise.race([promise, timeout]).finally(() => {
      if (timer) window.clearTimeout(timer);
    });
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
        state.portDisplay.name = port.portName || port.displayName || port.portId || null;
        state.portDisplay.vid = port.vendorId || null;
        state.portDisplay.pid = port.productId || null;
        updatePortInfoDisplay();
        closeModal(els.portPickerModal);
        log(`Port secildi: ${state.portDisplay.name || port.portId}`);
        window.aeropicoBridge.chooseSerialPort(port.portId || "");
      });
      els.portPickerList.appendChild(row);
    }
  }

  /* ── Connection ────────────────────────────── */

  async function connect() {
    if (!("serial" in navigator)) {
      log("Web Serial API bulunamadi. Electron/Chromium surumunu kontrol et.");
      return;
    }

    let baudRate = currentBaudRate();
    if (!baudRate) {
      log("Gecerli bir baud rate girin.");
      return;
    }
    if (baudRate === 1200) {
      baudRate = 115200;
      els.baudSelect.value = "115200";
      els.customBaudField.classList.add("hidden");
      log("1200 bps RP2350 bootloader reset tetikler; baglanti 115200 bps ile aciliyor.");
    }

    let openedPort = null;
    try {
      els.connectBtn.classList.add("loading");
      els.connectBtn.disabled = true;
      state.port = await withTimeout(
        navigator.serial.requestPort(),
        12000,
        "Port secimi zaman asimina ugradi.",
        () => {
          if (window.aeropicoBridge && typeof window.aeropicoBridge.cancelSerialPort === "function") {
            window.aeropicoBridge.cancelSerialPort();
          }
          closeModal(els.portPickerModal);
        }
      );
      openedPort = state.port;
      await withTimeout(
        state.port.open({ baudRate }),
        5000,
        `Port acma zaman asimi: ${state.portDisplay.name || "USB serial"}`
      );
      if (typeof state.port.setSignals === "function") {
        await state.port.setSignals({ dataTerminalReady: true, requestToSend: false }).catch(() => {});
      }
      state.writer = state.port.writable.getWriter();
      state.reader = state.port.readable.getReader();
      state.connected = true;
      state.activeBaud = baudRate;

      if (typeof state.port.getInfo === "function") setPortDisplay(normalizeSerialInfo(state.port.getInfo()));

      updatePortInfoDisplay();
      setLinkStatus("Bagli", "ok");
      updateButtons();
      log(`USB serial baglandi @ ${baudRate.toLocaleString("tr-TR")} bps.`);
      toast("USB bağlantısı kuruldu.", "ok");
      readLoop();
      log("Cihaz dinleniyor. Parametre okumak icin 'Parametreleri Oku'ya bas.");
    } catch (error) {
      log(`Baglanti hatasi: ${error.message}`);
      toast("Bağlantı kurulamadı.", "bad");
      setLinkStatus("Hata", "bad");
      state.connected = false;
      state.reader = null;
      state.writer = null;
      state.port = null;
      state.activeBaud = null;
      state.firmwareVersion = null;
      state.txQueue = [];
      state.txBusy = false;
      if (openedPort && typeof openedPort.close === "function") {
        await openedPort.close().catch(() => {});
      }
      updatePortInfoDisplay();
      updateButtons();
    } finally {
      els.connectBtn.classList.remove("loading");
      els.connectBtn.classList.toggle("connected-ok", state.connected);
      if (!state.connected) els.connectBtn.disabled = false;
    }
  }

  async function disconnect() {
    try {
      state.connected = false;
      if (state.reader) {
        await state.reader.cancel().catch(() => { });
        state.reader.releaseLock();
      }
      if (state.writer) state.writer.releaseLock();
      if (state.port) await state.port.close();
    } catch (error) {
      log(`Baglanti kapatma hatasi: ${error.message}`);
    } finally {
      state.reader = null;
      state.writer = null;
      state.port = null;
      state.activeBaud = null;
      state.firmwareVersion = null;
      state.txQueue = [];
      state.txBusy = false;
      setLinkStatus("Kapali", "muted");
      updateButtons();
      updatePortInfoDisplay();
      log("Baglanti kapatildi.");
      toast("Bağlantı kapatıldı.", "info");
      els.connectBtn.classList.remove("connected-ok");
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
    if (state.connected) {
      state.connected = false;
      state.reader = null;
      state.writer = null;
      state.port = null;
      state.activeBaud = null;
      state.firmwareVersion = null;
      setLinkStatus("Kapali", "muted");
      updatePortInfoDisplay();
      updateButtons();
      log("USB serial akisi kapandi.");
      toast("USB bağlantısı kesildi.", "warn");
    }
  }

  function writeFrame(frame, label = "MAVLink") {
    if (!state.writer) return false;
    if (state.txQueue.length >= 64) {
      log(`${label}: gonderim kuyrugu dolu, paket reddedildi.`);
      toast("MAVLink gönderim kuyruğu dolu.", "bad");
      return false;
    }
    state.txQueue.push({ frame, label });
    processTxQueue();
    return true;
  }

  function processTxQueue() {
    if (state.txBusy || !state.writer || state.txQueue.length === 0) return;
    state.txBusy = true;
    const item = state.txQueue.shift();
    sendQueuedFrame(item).finally(() => {
      state.txBusy = false;
      if (state.txQueue.length > 0) {
        window.setTimeout(processTxQueue, 18);
      }
    });
  }

  async function sendQueuedFrame(item) {
    try {
      await state.writer.write(item.frame);
    } catch (error) {
      log(`${item.label}: yazma hatasi: ${error.message}`);
      state.txQueue = [];
    }
  }

  function requestParams() {
    writeFrame(encoder.heartbeat(), "GCS_HEARTBEAT");
    if (!writeFrame(encoder.paramRequestList(), "PARAM_REQUEST_LIST")) return;
    window.setTimeout(() => writeFrame(encoder.paramRequestList(), "PARAM_REQUEST_LIST retry 1"), 250);
    window.setTimeout(() => writeFrame(encoder.paramRequestList(), "PARAM_REQUEST_LIST retry 2"), 750);
    log("PARAM_REQUEST_LIST gonderildi.");
    toast("Parametre okuma isteği gönderildi.", "info");
  }

  function setParam(name, value, notify = true) {
    const validation = validateParam(name, value);
    if (!validation.ok) {
      log(`${name}: ${validation.reason}`);
      return false;
    }
    if (!writeFrame(encoder.paramSet(name, validation.value), `PARAM_SET ${name}`)) return false;
    log(`${name} = ${validation.value} gonderildi.`);
    if (notify) toast(`${name} gönderildi.`, "ok");
    return true;
  }

  function saveParams() {
    setParam("PARAM_SAVE", 1);
    toast("Flash kayıt komutu gönderildi.", "warn");
  }

  function applyDirtyParams() {
    if (!state.connected || !state.writer) {
      log("Degisenleri uygulamak icin once baglan.");
      return;
    }
    const entries = [...state.dirtyParams.entries()];
    let sent = 0;
    for (const [name, value] of entries) {
      if (setParam(name, value, false)) {
        state.dirtyParams.delete(name);
        sent++;
      }
    }
    updateDirtyButton();
    renderSettings();
    log(`${sent} degisen parametre gonderildi.`);
    toast(`${sent} değişiklik uygulandı.`, sent > 0 ? "ok" : "info");
  }

  /* ── Parameter Profiles ───────────────────── */

  function readProfiles() {
    try {
      const parsed = JSON.parse(localStorage.getItem(PROFILE_STORAGE_KEY) || "[]");
      return Array.isArray(parsed) ? parsed.filter((profile) => profile && profile.name && profile.params) : [];
    } catch (error) {
      return [];
    }
  }

  function writeProfiles(profiles) {
    try {
      localStorage.setItem(PROFILE_STORAGE_KEY, JSON.stringify(profiles.slice(0, 12)));
    } catch (error) {
      log(`Profil kaydedilemedi: ${error.message}`);
    }
  }

  function currentParamSnapshot() {
    const params = {};
    for (const [name, param] of state.params.entries()) {
      if (Number.isFinite(param.value) && PARAM_RULES[name]) params[name] = param.value;
    }
    return params;
  }

  function saveCurrentProfile() {
    const name = (els.profileNameInput.value || "").trim();
    if (!name) {
      log("Profil adi girin.");
      return;
    }

    const params = currentParamSnapshot();
    const count = Object.keys(params).length;
    if (count === 0) {
      log("Profil icin okunmus veya ice aktarilmis parametre yok.");
      return;
    }

    const profiles = readProfiles().filter((profile) => profile.name !== name);
    profiles.unshift({
      name,
      params,
      count,
      createdAt: new Date().toISOString()
    });
    writeProfiles(profiles);
    els.profileNameInput.value = "";
    renderProfiles();
    log(`Profil kaydedildi: ${name} (${count} parametre).`);
    toast("Profil kaydedildi.", "ok");
  }

  function loadProfileToUi(profileName) {
    const profile = readProfiles().find((item) => item.name === profileName);
    if (!profile) return;
    let loaded = 0;
    for (const [name, value] of Object.entries(profile.params)) {
      const validation = validateParam(name, Number(value));
      if (!validation.ok) continue;
      state.dirtyParams.set(name, validation.value);
      loaded++;
    }
    renderSettings();
    renderSummary();
    renderProfiles();
    updateDirtyButton();
    log(`Profil degisiklik olarak yuklendi: ${profile.name} (${loaded} parametre).`);
    toast("Profil değişiklik olarak yüklendi; uygulamak için gönder.", "ok");
  }

  function applyProfileToFirmware(profileName) {
    const profile = readProfiles().find((item) => item.name === profileName);
    if (!profile) return;
    if (!state.connected || !state.writer) {
      log(`${profile.name}: firmware'e uygulamak icin once baglan.`);
      return;
    }

    let sent = 0;
    for (const [name, value] of Object.entries(profile.params)) {
      const validation = validateParam(name, Number(value));
      if (!validation.ok) {
        log(`${profile.name}/${name}: uygulanmadi, ${validation.reason}`);
        continue;
      }
      if (writeFrame(encoder.paramSet(name, validation.value), `PROFILE ${name}`)) {
        sent++;
      }
    }
    log(`Profil firmware'e gonderildi: ${profile.name} (${sent} parametre). Flash'a almak icin Kaydet.`);
    toast("Profil firmware'e gönderildi.", "ok");
  }

  function deleteProfile(profileName) {
    const ok = window.confirm(`${profileName} profilini silmek istiyor musun?`);
    if (!ok) return;
    writeProfiles(readProfiles().filter((profile) => profile.name !== profileName));
    renderProfiles();
    log(`Profil silindi: ${profileName}.`);
    toast("Profil silindi.", "info");
  }

  function renderProfiles() {
    if (!els.profileList) return;
    const profiles = readProfiles();
    if (profiles.length === 0) {
      els.profileList.innerHTML = `<div class="profile-empty">Kayitli profil yok.</div>`;
      return;
    }

    els.profileList.innerHTML = "";
    for (const profile of profiles) {
      const card = document.createElement("article");
      card.className = "profile-card";
      const date = profile.createdAt ? new Date(profile.createdAt).toLocaleString("tr-TR", { hour12: false }) : "Tarih yok";
      card.innerHTML = `
        <div class="profile-card-main">
          <strong>${escapeHtml(profile.name)}</strong>
          <span>${Number(profile.count || Object.keys(profile.params || {}).length)} parametre · ${date}</span>
        </div>
        <div class="profile-actions">
          <button class="small" data-profile-load="${escapeAttr(profile.name)}" type="button">Yükle</button>
          <button class="small" data-profile-apply="${escapeAttr(profile.name)}" type="button">Firmware'e Uygula</button>
          <button class="small danger-text" data-profile-delete="${escapeAttr(profile.name)}" type="button">Sil</button>
        </div>`;
      els.profileList.appendChild(card);
    }

    els.profileList.querySelectorAll("[data-profile-load]").forEach((button) => {
      button.addEventListener("click", () => loadProfileToUi(button.dataset.profileLoad));
    });
    els.profileList.querySelectorAll("[data-profile-apply]").forEach((button) => {
      button.addEventListener("click", () => applyProfileToFirmware(button.dataset.profileApply));
    });
    els.profileList.querySelectorAll("[data-profile-delete]").forEach((button) => {
      button.addEventListener("click", () => deleteProfile(button.dataset.profileDelete));
    });
  }

  function escapeHtml(value) {
    return String(value).replace(/[&<>"']/g, (char) => ({
      "&": "&amp;",
      "<": "&lt;",
      ">": "&gt;",
      '"': "&quot;",
      "'": "&#39;"
    }[char]));
  }

  function escapeAttr(value) {
    return escapeHtml(value).replace(/`/g, "&#96;");
  }

  function serviceCommandParams(command) {
    if (command === "SERVO_TEST") {
      const surface = clampInteger(els.servoTestSurface?.value, 0, 4, 0);
      const pulse = clampInteger(els.servoTestPulse?.value, 1000, 2000, 1600);
      const duration = clampInteger(els.servoTestDuration?.value, 100, 1500, 700);
      if (els.servoTestPulse) els.servoTestPulse.value = String(pulse);
      if (els.servoTestPulseVal) els.servoTestPulseVal.textContent = String(pulse);
      if (els.servoTestDuration) els.servoTestDuration.value = String(duration);
      return [AEROPICO_SERVICE[command], surface, pulse, duration];
    }
    return [AEROPICO_SERVICE[command], 0, 0, 0];
  }

  function serviceCommandFromAction(action) {
    for (const [name, value] of Object.entries(AEROPICO_SERVICE)) {
      if (value === action) return name;
    }
    return null;
  }

  function setPendingCommand(command, mavCommand, action = 0) {
    state.lastCommand = command;
    state.lastCommandAtMs = Date.now();
    state.pendingCommand = {
      command,
      mavCommand,
      action,
      atMs: state.lastCommandAtMs
    };
    window.setTimeout(() => markCommandTimeout(state.pendingCommand), COMMAND_ACK_TIMEOUT_MS);
  }

  function markCommandTimeout(pending) {
    if (!pending || state.pendingCommand !== pending) return;
    pushCommandHistory(pending.command, "timeout", "ACK gelmedi (5s)");
    state.pendingCommand = null;
    state.lastCommand = null;
    renderArmChecklist();
    updateButtons();
  }

  function sendServiceCommand(command) {
    const action = AEROPICO_SERVICE[command];
    if (!action) {
      log(`${command}: desteklenmeyen configurator komutu.`);
      return;
    }
    if (!state.connected || !state.writer) {
      log(`${SERVICE_LABELS[command] || command}: once baglan.`);
      return;
    }
    if (state.pendingCommand && (Date.now() - state.pendingCommand.atMs) < 2500) {
      log(`${SERVICE_LABELS[command] || command}: once mevcut komut ACK beklesin.`);
      return;
    }
    const [p1, p2, p3, p4] = serviceCommandParams(command);
    if (!writeFrame(encoder.aeroPicoService(p1, p2, p3, p4), SERVICE_LABELS[command] || command)) return;
    setPendingCommand(command, 31010, p1);
    pushCommandHistory(command, "pending", "ACK bekleniyor");
    updateButtons();
    window.setTimeout(updateButtons, 2600);
    log(`${SERVICE_LABELS[command] || command} komutu gonderildi.`);
  }

  function sendArmCommand(command) {
    if (!state.connected || !state.writer) {
      log("Arm komutu: once baglan.");
      return;
    }
    if (command === "force") {
      const ok = window.confirm("Bench force-arm icin pervane sokulu olmali ve GP20-GP21 jumper takili olmalidir. Devam?");
      if (!ok) return;
      if (!writeFrame(encoder.arm(true), "BENCH_FORCE_ARM")) return;
      setPendingCommand("BENCH_FORCE_ARM", MAV_CMD_COMPONENT_ARM_DISARM);
      pushCommandHistory("BENCH_FORCE_ARM", "pending", "GP20-GP21 jumper ile ACK bekleniyor");
      log("Bench force-arm komutu gonderildi.");
      return;
    }
    if (command === "disarm") {
      if (!writeFrame(encoder.disarm(false), "DISARM")) return;
      setPendingCommand("DISARM", MAV_CMD_COMPONENT_ARM_DISARM);
      pushCommandHistory("DISARM", "pending", "ACK bekleniyor");
      log("Disarm komutu gonderildi.");
      return;
    }
    if (!writeFrame(encoder.arm(false), "ARM")) return;
    setPendingCommand("ARM", MAV_CMD_COMPONENT_ARM_DISARM);
    pushCommandHistory("ARM", "pending", "Preflight gate ACK bekleniyor");
    log("Normal arm komutu gonderildi.");
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

  function updateLastCommandFromAck(message, stateName, detail) {
    const pending = state.pendingCommand;
    if (!pending) return false;
    if (Date.now() - pending.atMs > COMMAND_ACK_TIMEOUT_MS) {
      state.pendingCommand = null;
      state.lastCommand = null;
      return false;
    }
    if (pending.mavCommand !== message.command) return false;
    if (message.command === 31010) {
      if (message.resultParam2 <= 0) {
        log("Etiketsiz servis ACK yok sayildi.");
        return false;
      }
      if (message.resultParam2 !== pending.action) {
        const actionName = serviceCommandFromAction(message.resultParam2) || `Aksiyon ${message.resultParam2}`;
        log(`Gecikmis servis ACK yok sayildi: ${actionName}.`);
        return false;
      }
    }
    pushCommandHistory(pending.command, stateName, detail);
    if (stateName !== "pending") {
      state.pendingCommand = null;
      state.lastCommand = null;
    }
    return true;
  }

  function renderCommandStatus() {
    if (!els.commandStatusList) return;
    const latest = state.commandHistory[0];
    if (latest) {
      const pillClass = latest.state === "accepted" ? "ok" : latest.state === "pending" ? "warn" : "bad";
      els.commandSummary.textContent = latest.state === "pending" ? "Bekliyor" : latest.state === "accepted" ? "OK" : latest.state === "timeout" ? "Timeout" : "Red";
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

  function pushMavlinkInspector(message) {
    const item = describeMavlinkMessage(message);
    if (!item) return;
    state.mavlinkHistory.unshift({
      ...item,
      at: new Date().toLocaleTimeString("tr-TR", { hour12: false })
    });
    state.mavlinkHistory = state.mavlinkHistory.slice(0, 8);
    renderMavlinkInspector();
  }

  function renderMavlinkInspector() {
    if (!els.mavlinkInspectorList) return;
    const latest = state.mavlinkHistory[0];
    if (latest) {
      els.mavlinkInspectorSummary.textContent = latest.kind === "bad" ? "Uyarı" : latest.kind === "warn" ? "Dikkat" : "Canlı";
      els.mavlinkInspectorSummary.className = `status-pill ${latest.kind === "bad" ? "bad" : latest.kind === "warn" ? "warn" : "ok"}`;
    } else {
      els.mavlinkInspectorSummary.textContent = "Bekliyor";
      els.mavlinkInspectorSummary.className = "status-pill muted";
      const empty = `<p class="hint">Cihazdan MAVLink paketi bekleniyor.</p>`;
      if (els.mavlinkInspectorList) els.mavlinkInspectorList.innerHTML = empty;
      return;
    }

    const renderRows = (container) => {
      if (!container) return;
      container.innerHTML = "";
      for (const item of state.mavlinkHistory) {
        const row = document.createElement("div");
        row.className = `mavlink-inspector-row ${item.kind}`;
        row.innerHTML = `<div><strong>${escapeHtml(item.title)}</strong><time>${escapeHtml(item.at)}</time></div><span>${escapeHtml(item.detail)}</span><em>${escapeHtml(item.advice)}</em>`;
        container.appendChild(row);
      }
    };

    renderRows(els.mavlinkInspectorList);
  }

	  function describeMavlinkMessage(message) {
    if (message.type === "heartbeat") {
      const armed = (message.baseMode & 0x80) !== 0;
      const status = systemStatusText(message.systemStatus);
      return {
        kind: armed ? "warn" : "ok",
        title: `HEARTBEAT ${message.mavlinkVersion ? `v${message.mavlinkVersion}` : ""}`,
        detail: `${armed ? "ARMED" : "DISARMED"} · ${status}`,
        advice: armed ? "Pervane/servo hattı güvenli mi kontrol et." : "Bağlantı canlı; preflight ve parametreleri okuyabilirsin."
      };
    }
    if (message.type === "sysStatus") {
      const hasBattery = message.voltageBatteryMv > 0 && message.voltageBatteryMv < 65535;
      const voltage = hasBattery ? `${(message.voltageBatteryMv / 1000).toFixed(2)} V` : "batarya yok";
      return {
        kind: hasBattery ? "ok" : "warn",
        title: "SYS_STATUS",
        detail: `${voltage} · kalan ${message.batteryRemaining}%`,
        advice: hasBattery ? "Batarya izleme akıyor." : "Bench ise normal; uçuş için battery ADC/power module doğrula."
      };
    }
    if (message.type === "param") {
      return {
        kind: message.index + 1 >= message.count ? "ok" : "muted",
        title: "PARAM_VALUE",
        detail: `${message.name} = ${formatParamValue(message.value)} (${message.index + 1}/${message.count})`,
        advice: message.index + 1 >= message.count ? "Parametre listesi tamamlandı; değişiklik yapmadan önce yedek al." : "Parametreler okunuyor."
      };
    }
    if (message.type === "commandAck") {
      const accepted = message.result === 0;
      return {
        kind: accepted ? "ok" : "bad",
        title: "COMMAND_ACK",
        detail: `${commandName(message.command)} · ${mavResultText(message.result)}`,
        advice: accepted ? "Komut firmware tarafından kabul edildi." : "Reddedildiyse preflight, armed state ve safety gate sebebini kontrol et."
      };
    }
    if (message.type === "autopilotVersion") {
      return {
        kind: "ok",
        title: "AUTOPILOT_VERSION",
        detail: `${formatFirmwareVersion(message.flightSwVersion)}${message.flightCustomVersion ? ` · ${message.flightCustomVersion}` : ""}`,
        advice: "Firmware sürümü alındı; destek/debug kayıtlarında bu değeri kullan."
      };
    }
	    if (message.type === "statusText") {
	      const upper = message.text.toUpperCase();
	      const i2c = parseI2cDiagnostics(upper);
	      const bad = upper.includes("FAIL") || upper.includes("MISSING") || upper.includes("MISMATCH") || upper.includes("BROWNOUT") || i2c.ackNone;
	      const warn = bad || upper.includes("WARN") || upper.includes("DROPPED") || upper.includes("INVALID") || i2c.hasI2c;
	      return {
	        kind: i2c.hasUsefulScan ? "ok" : bad ? "bad" : warn ? "warn" : "ok",
	        title: i2c.hasI2c ? "I2C DIAGNOSTIC" : "STATUSTEXT",
	        detail: message.text,
	        advice: adviceForStatusText(upper)
	      };
	    }
	    return null;
	  }

	  function parseI2cDiagnostics(text) {
	    const result = {
	      hasI2c: false,
	      hasUsefulScan: false,
	      ackReportSeen: false,
	      regReportSeen: false,
	      idReportSeen: false,
	      regValReportSeen: false,
	      ackNone: false,
	      regNone: false,
	      ack: new Set(),
	      reg: new Set(),
        ids: {}
	    };
	    const scanGroups = [
	      { key: "ack", patterns: [/I2C_ACK(?:\s+((?:0X[0-9A-F]{2}\s*)+|NONE))/g, /ACK_([0-9A-F_]+|NONE)/g] },
	      { key: "reg", patterns: [/I2C_REG(?:\s+((?:0X[0-9A-F]{2}\s*)+|NONE))/g, /REG_([0-9A-F_]+|NONE)/g] }
	    ];
	    for (const group of scanGroups) {
	      for (const pattern of group.patterns) {
	        let match;
	      while ((match = pattern.exec(text)) !== null) {
	        result.hasI2c = true;
	        if (group.key === "ack") result.ackReportSeen = true;
	        if (group.key === "reg") result.regReportSeen = true;
	        const value = match[1] || "";
	          if (value === "NONE") {
	            if (group.key === "ack") result.ackNone = true;
	            if (group.key === "reg") result.regNone = true;
	            continue;
	          }
	          const addresses = value.includes("0X")
	            ? value.match(/0X[0-9A-F]{2}/g) || []
	            : value.split("_").filter(Boolean).map((part) => `0X${part}`);
	          for (const address of addresses) {
	            result[group.key].add(address.replace("0X", ""));
	          }
	        }
	      }
	    }
      const idMatch = text.match(/I2C_ID\s+MPU=(0X[0-9A-F]{2}|--[0-9A-F]{2}|--)\s+BARO=(0X[0-9A-F]{2}|--[0-9A-F]{2}|--)/i);
	  if (idMatch) {
	    result.hasI2c = true;
	    result.idReportSeen = true;
	    if (idMatch[1].startsWith("0X")) result.ids.mpu = idMatch[1].replace("0X", "");
	    else result.ids.mpu = null;
	    if (idMatch[2].startsWith("0X")) result.ids.baro = idMatch[2].replace("0X", "");
	    else result.ids.baro = null;
	  }
	  const regValMatch = text.match(/I2C_REGVAL\s+68:75=(--[0-9A-F]{2}|[0-9A-F]{2})\s+77:D0=(--[0-9A-F]{2}|[0-9A-F]{2})/i);
	  if (regValMatch) {
	    result.hasI2c = true;
	    result.regValReportSeen = true;
	    if (!regValMatch[1].startsWith("--")) {
	      result.ids.mpu = regValMatch[1];
	      result.reg.add("68");
	    } else result.ids.mpu = null;
	    if (!regValMatch[2].startsWith("--")) {
	      result.ids.baro = regValMatch[2];
	      result.reg.add("77");
	    } else result.ids.baro = null;
	  }
	    result.hasUsefulScan = result.ack.size > 0 || result.reg.size > 0 || Boolean(result.ids.mpu || result.ids.baro);
	    return result;
	  }

	  function formatI2cAddressList(addresses) {
	    const list = [...addresses].sort();
	    return list.length ? list.map((address) => `0x${address}`).join(", ") : "none";
	  }

  function formatI2cAddressEvidence(addresses, hasDiagnostic) {
    if (!hasDiagnostic) return "teşhis bekleniyor";
    return formatI2cAddressList(addresses);
  }

  function logI2cDiagnostics(text) {
	    const i2c = parseI2cDiagnostics(text);
	    if (!i2c.hasI2c) return false;

      const nowMs = Date.now();
      state.i2cDiagnostics.lastText = text;
      state.i2cDiagnostics.lastSeenMs = nowMs;
      const incomingGood = i2c.ack.size > 0 || i2c.reg.size > 0 || i2c.ids.mpu || i2c.ids.baro;
      if (incomingGood) {
        state.i2cDiagnostics.lastGoodMs = nowMs;
      }
      if (i2c.ackNone || i2c.regNone) {
        state.i2cDiagnostics.lastBadMs = nowMs;
        state.i2cDiagnostics.lastBadText = text;
      }
      if (i2c.ackReportSeen) {
        state.i2cDiagnostics.ack = i2c.ack;
      }
      if (i2c.regReportSeen || i2c.regValReportSeen) {
        state.i2cDiagnostics.reg = i2c.reg;
      }
      if (i2c.idReportSeen || i2c.regValReportSeen) {
        state.i2cDiagnostics.ids = { mpu: i2c.ids.mpu || undefined, baro: i2c.ids.baro || undefined };
      }

      renderI2cDiagnostics();

	    if (i2c.ackNone) {
	      els.preflightText.textContent = "I2C ACK yok: Pico bus seviyesinde sensor gormuyor.";
	      return true;
	    }
    const ack = state.i2cDiagnostics.ack;
    const reg = state.i2cDiagnostics.reg;
    if (ack.has("68") && state.modules.imu !== "ok") {
      setModuleState("imu", "detected");
    }
    if (state.i2cDiagnostics.ids.mpu === "68" && state.modules.imu !== "ok") setModuleState("imu", "detected");
    if (ack.has("77") && state.modules.baro !== "ok") {
      setModuleState("baro", "detected");
    }
    if (state.i2cDiagnostics.ids.baro === "55" && state.modules.baro !== "ok") setModuleState("baro", "detected");

	    const summary = `I2C ACK ${formatI2cAddressList(ack)} | REG ${formatI2cAddressList(reg)} | MPU ${formatI2cId(state.i2cDiagnostics.ids.mpu)} | BARO ${formatI2cId(state.i2cDiagnostics.ids.baro)}`;
	    els.preflightText.textContent = summary;
      renderArmChecklist();
	    return true;
	  }

  function moduleStateFromHealthToken(token, moduleId) {
    if (token === "OK") return "ok";
    if (token === "ID" || token === "WARMUP" || token === "STALE" || token === "TIMEOUT" || token === "INVALID") {
      return hasI2cEvidence(moduleId) ? "detected" : "bad";
    }
    if (token === "MISS") {
      return hasI2cEvidence(moduleId) ? "detected" : "bad";
    }
    return null;
  }

  function applySensorCheckStatus(text) {
    if (text.includes("SENSOR_FAIL IMU")) {
      setModuleState("imu", "bad");
      els.preflightText.textContent = "IMU okunamiyor: I2C kimligi ve backend init zinciri kontrol edilmeli.";
      return true;
    }

    let handled = false;
    const checkMatch = text.match(/SENSOR_CHECK\s+IMU_([A-Z]+)\s+BARO_([A-Z]+)\s+MAG_([A-Z]+)/i);
    if (checkMatch) {
      const imuState = moduleStateFromHealthToken(checkMatch[1].toUpperCase(), "imu");
      const baroState = moduleStateFromHealthToken(checkMatch[2].toUpperCase(), "baro");
      const magState = moduleStateFromHealthToken(checkMatch[3].toUpperCase(), "mag");
      if (imuState) setModuleState("imu", imuState);
      if (baroState) setModuleState("baro", baroState);
      if (magState) state.modules.mag = magState;
      els.preflightText.textContent = text.replace(/_/g, " ");
      handled = true;
    }

    const healthMatch = text.match(/SENSOR_HEALTH\s+IMU=([A-Z]+)\s+BARO=([A-Z]+)\s+MAG=([A-Z]+)/i);
    if (healthMatch) {
      const imuState = moduleStateFromHealthToken(healthMatch[1].toUpperCase(), "imu");
      const baroState = moduleStateFromHealthToken(healthMatch[2].toUpperCase(), "baro");
      const magState = moduleStateFromHealthToken(healthMatch[3].toUpperCase(), "mag");
      if (imuState) setModuleState("imu", imuState);
      if (baroState) setModuleState("baro", baroState);
      if (magState) state.modules.mag = magState;
      handled = true;
    }

    const faultMatch = text.match(/SENSOR_FAULT\s+([A-Z0-9_]+)/i);
    if (faultMatch) {
      const fault = faultMatch[1].toUpperCase();
      if (fault !== "NONE") {
        els.preflightText.textContent = `Sensor fault: ${fault}`;
      }
      handled = true;
    }

    if (!handled && text.includes("SENSOR_CHECK")) {
      if (text.includes("IMU_OK")) setModuleState("imu", "ok");
      if (text.includes("IMU_MISS")) setModuleState("imu", "bad");
      if (text.includes("BARO_OK")) setModuleState("baro", "ok");
      if (text.includes("BARO_MISS")) setModuleState("baro", hasI2cEvidence("baro") ? "detected" : "bad");
      if (text.includes("MAG_OK")) state.modules.mag = "ok";
      if (text.includes("MAG_MISS")) state.modules.mag = hasI2cEvidence("mag") ? "detected" : "bad";
      handled = true;
    }
    return handled;
  }

  function formatI2cId(value) {
    return value ? `0x${value}` : "--";
  }

  function renderI2cDiagnostics() {
    if (!els.i2cDiagnosticList || !els.i2cSummary) return;
    const diag = state.i2cDiagnostics;
    const nowMs = Date.now();
    const hasDiagnostic = diag.lastSeenMs > 0;
    const goodAge = diag.lastGoodMs ? `${Math.max(0, ((nowMs - diag.lastGoodMs) / 1000)).toFixed(1)} sn önce` : "yok";
    const badAge = diag.lastBadMs ? `${Math.max(0, ((nowMs - diag.lastBadMs) / 1000)).toFixed(1)} sn önce` : "yok";
    const imuIdentityOk = diag.ids.mpu === "68";
    const baroIdentityOk = diag.ids.baro === "55";
    const imuBusOk = diag.ack.has("68");
    const baroBusOk = diag.ack.has("77");
    const imuRegOk = diag.reg.has("68") || imuIdentityOk;
    const baroRegOk = diag.reg.has("77") || baroIdentityOk;
    const imuRuntime = state.modules.imu === "ok" ? "healthy" : hasI2cEvidence("imu") ? "identity-only" : "missing";
    const baroRuntime = state.modules.baro === "ok" ? "healthy" : hasI2cEvidence("baro") ? "identity-only" : "missing";
    const rows = [
      i2cDiagnosticRow("Bus ACK", formatI2cAddressEvidence(diag.ack, hasDiagnostic), imuBusOk && baroBusOk ? "ok" : diag.ack.size > 0 ? "warn" : hasDiagnostic ? "bad" : "muted", "I2C hattında cevap veren adresler. GY-87 için 0x68 ve 0x77 beklenir."),
      i2cDiagnosticRow("MPU register", `0x68 / 0x75 -> ${hasDiagnostic || diag.ids.mpu ? formatI2cId(diag.ids.mpu) : "teşhis bekleniyor"}`, imuIdentityOk ? "ok" : imuRegOk ? "warn" : imuBusOk ? "warn" : hasDiagnostic ? "bad" : state.modules.imu === "ok" ? "warn" : "muted", "MPU6050 WHOAMI registerı. Doğru kimlik 0x68 olmalı."),
      i2cDiagnosticRow("BARO register", `0x77 / 0xD0 -> ${hasDiagnostic || diag.ids.baro ? formatI2cId(diag.ids.baro) : "teşhis bekleniyor"}`, baroIdentityOk ? "ok" : baroRegOk ? "warn" : baroBusOk ? "warn" : hasDiagnostic ? "bad" : "muted", "BMP180/BMP085 chip ID registerı. Doğru kimlik 0x55 olmalı."),
      i2cDiagnosticRow("Runtime IMU", imuRuntime, state.modules.imu === "ok" ? "ok" : hasI2cEvidence("imu") ? "warn" : "bad", "Firmware SYS_STATUS içinde gerçek IMU health durumu."),
      i2cDiagnosticRow("Runtime BARO", baroRuntime, state.modules.baro === "ok" ? "ok" : hasI2cEvidence("baro") ? "warn" : "bad", "Firmware SYS_STATUS içinde gerçek barometre health durumu."),
      i2cDiagnosticRow("Son iyi teşhis", goodAge, diag.lastGoodMs ? "ok" : "muted", "ACK/REG/ID içeren son geçerli teşhis zamanı."),
      i2cDiagnosticRow("Son kötü teşhis", badAge, diag.lastBadMs ? "warn" : "muted", diag.lastBadText || "Henüz boş/kötü scan yok.")
    ];
    els.i2cDiagnosticList.innerHTML = rows.join("");
    const ok = imuIdentityOk && baroIdentityOk;
    const partial = diag.ack.size > 0 || diag.reg.size > 0 || diag.ids.mpu || diag.ids.baro;
    els.i2cSummary.textContent = ok ? "OK" : partial ? "Kısmi" : "Bekliyor";
    els.i2cSummary.className = `status-pill ${ok ? "ok" : partial ? "warn" : "muted"}`;
  }

  function i2cDiagnosticRow(label, value, kind, note) {
    return `<div class="diagnostic-row ${kind}">
      <span class="diagnostic-dot"></span>
      <div>
        <strong>${escapeHtml(label)}</strong>
        <p>${escapeHtml(note)}</p>
      </div>
      <code>${escapeHtml(value)}</code>
    </div>`;
  }

  function systemStatusText(status) {
    switch (status) {
      case 3: return "standby";
      case 4: return "active";
      case 5: return "critical";
      case 6: return "emergency";
      default: return `status ${status}`;
    }
  }

  function commandName(command) {
    if (command === MAV_CMD_COMPONENT_ARM_DISARM) return "ARM/DISARM";
    if (command === 31010) return "AeroPico service";
    return `CMD ${command}`;
  }

  function formatParamValue(value) {
    if (!Number.isFinite(value)) return String(value);
    if (Math.abs(value) >= 100) return String(Math.round(value));
    return value.toFixed(3).replace(/0+$/, "").replace(/\.$/, "");
  }

	  function adviceForStatusText(text) {
	    const i2c = parseI2cDiagnostics(text);
	    if (i2c.hasI2c) {
	      const ackList = [...i2c.ack].join(", ") || "yok";
	      const regList = [...i2c.reg].join(", ") || "yok";
	      if (i2c.ackNone) return "Pico I2C hattinda ACK gormuyor; GP4/GP5, ortak GND, SDA/SCL ve flash pin parametrelerini kontrol et.";
	      if (i2c.ack.has("68") && !i2c.reg.has("68")) return `ACK var (${ackList}) ama register probe eksik (${regList}); repeated-start/register okuma veya MPU backend akisini kontrol et.`;
	      if (i2c.ack.has("68") && i2c.reg.has("68")) return `I2C temel erisim tamam (${ackList}); IMU hala yoksa init/DMA/backend sonraki adim.`;
	      return `I2C scan: ACK ${ackList}, REG ${regList}.`;
	    }
	    if (text.includes("WHOAMI") || text.includes("IMU")) return "IMU kablo/I2C adresi ve GY-87 beslemesini kontrol et.";
    if (text.includes("BARO") || text.includes("BMP")) return "Barometre adresi 0x77 ve lehim/I2C hattını doğrula.";
    if (text.includes("MAG") || text.includes("HMC") || text.includes("QMC")) return "Mag opsiyonelse preflight politikasını, gerekliyse bypass/profil seçimini kontrol et.";
    if (text.includes("RC")) return "SBUS sinyali, inverter ve mode/throttle kanallarını kontrol et.";
    if (text.includes("BROWNOUT") || text.includes("BATTERY")) return "Bench ise batarya yok sayılabilir; uçuşta ADC/power module kalibrasyonu şart.";
    if (text.includes("DROPPED") || text.includes("BLACKBOX")) return "Log hedefi/telemetri hızı yüksek olabilir; SD/telemetry sink ayarını kontrol et.";
    if (text.includes("PREFLIGHT_OK")) return "Preflight yazılımsal olarak hazır; fiziksel güvenliği ayrıca kontrol et.";
    if (text.includes("PREFLIGHT")) return "Arm reddi için bu mesajı gider; bench force arm sadece pervane sökülüyken.";
    return "Mesaj normal akış bilgisi; anormal tekrar ederse olay kaydına bak.";
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

  function hasI2cEvidence(moduleId) {
    const diag = state.i2cDiagnostics;
    if (moduleId === "imu") {
      return diag.ack.has("68") || diag.reg.has("68") || diag.ids.mpu === "68";
    }
    if (moduleId === "baro") {
      return diag.ack.has("77") || diag.reg.has("77") || diag.ids.baro === "55";
    }
    if (moduleId === "mag") {
      return diag.ack.has("1E") || diag.ack.has("0D") || diag.ack.has("2C") ||
        diag.reg.has("1E") || diag.reg.has("0D") || diag.reg.has("2C");
    }
    return false;
  }

  function setModuleState(moduleId, value) {
    state.modules[moduleId] = value;
  }

  function updateModulesFromSysStatus(message) {
    const present = message.onboardControlSensorsPresent || 0;
    const enabled = message.onboardControlSensorsEnabled || 0;
    const healthy = message.onboardControlSensorsHealth || 0;
    const hasImu = (present & MAV_SENSOR_BITS.gyro) && (present & MAV_SENSOR_BITS.accel);
    const imuHealthy = (healthy & MAV_SENSOR_BITS.gyro) && (healthy & MAV_SENSOR_BITS.accel);
    setModuleState("imu", hasImu
      ? (imuHealthy ? "ok" : hasI2cEvidence("imu") ? "detected" : "bad")
      : hasI2cEvidence("imu") ? "detected" : "bad");
    setModuleState("baro", present & MAV_SENSOR_BITS.pressure
      ? ((enabled & MAV_SENSOR_BITS.pressure) && (healthy & MAV_SENSOR_BITS.pressure)
        ? "ok"
        : hasI2cEvidence("baro") ? "detected" : "bad")
      : hasI2cEvidence("baro") ? "detected" : "bad");
    state.modules.mag = present & MAV_SENSOR_BITS.mag
      ? ((enabled & MAV_SENSOR_BITS.mag) && (healthy & MAV_SENSOR_BITS.mag) ? "ok" : "bad")
      : "bad";
    state.modules.gps = present & MAV_SENSOR_BITS.gps
      ? ((enabled & MAV_SENSOR_BITS.gps) && (healthy & MAV_SENSOR_BITS.gps) ? "ok" : "bad")
      : "bad";
  }

  function handleMavlinkMessage(message) {
    pushMavlinkInspector(message);

    if (message.type === "heartbeat") {
      state.lastHeartbeatMs = Date.now();
      state.armed = (message.baseMode & 0x80) !== 0;
      els.preflightText.textContent = `Heartbeat alindi. System status: ${message.systemStatus}. Parametreleri okuyup preflight sonucunu kontrol et.`;
      renderModules();
      renderArmChecklist();
      updateButtons();
      return;
    }

    if (message.type === "param") {
      state.params.set(message.name, message);
      state.expectedParamCount = message.count;
      renderSettings();
      renderModules();
      if (state.params.size === message.count) log(`${message.count} parametre okundu.`);
      renderConfigAudit();
      renderArmChecklist();
      renderSummary();
      return;
    }

    if (message.type === "sysStatus") {
      state.lastSysStatus = message;
      state.modules.battery = message.voltageBatteryMv > 0 && message.voltageBatteryMv < 65535 ? "ok" : "bad";
      updateModulesFromSysStatus(message);
      renderModules();
      renderConfigAudit();
      renderArmChecklist();
      renderSummary();
      return;
    }

    if (message.type === "autopilotVersion") {
      state.firmwareVersion = formatFirmwareVersion(message.flightSwVersion);
      log(`Firmware surumu: ${state.firmwareVersion}`);
      renderSummary();
      return;
    }

    if (message.type === "commandAck") {
      const accepted = message.result === 0;
      const matched = updateLastCommandFromAck(message, accepted ? "accepted" : "rejected", mavResultText(message.result));
      if (message.command === MAV_CMD_COMPONENT_ARM_DISARM) {
        log(`ARM/DISARM ACK: ${mavResultText(message.result)}.`);
        if (matched) toast(accepted ? "ARM/DISARM kabul edildi." : "ARM/DISARM reddedildi.", accepted ? "ok" : "bad");
      } else if (message.command === 31010) {
        const actionName = serviceCommandFromAction(message.resultParam2) || "etiketsiz";
        log(`Servis komutu ACK (${actionName}): ${mavResultText(message.result)}.`);
        if (matched) toast(accepted ? "Servis komutu kabul edildi." : "Servis komutu reddedildi.", accepted ? "ok" : "bad");
      } else {
        log(`COMMAND_ACK ${message.command}: ${mavResultText(message.result)}.`);
      }
      renderArmChecklist();
      return;
    }

    if (message.type === "statusText") {
      log(`FC: ${message.text}`);
      const text = message.text.toUpperCase();
      const versionMatch = message.text.match(/^FW_VERSION\s+(\S+)/i);
      if (versionMatch) {
        state.firmwareVersion = versionMatch[1];
      }
      const handledSensorCheck = applySensorCheckStatus(text);
      const handledI2c = logI2cDiagnostics(text);
      if (!handledI2c) {
        if (text.includes("IMU CALIBRATION SAVED") || text.includes("SENSOR_CHECK_OK")) setModuleState("imu", "ok");
        if (text.includes("IMU MISSING") || text.includes("WHOAMI")) setModuleState("imu", "bad");
        if (text.includes("BMP") || text.includes("BARO")) setModuleState("baro", text.includes("HAZIR") || text.includes("OK") ? "ok" : "bad");
        if (text.includes("MAG")) state.modules.mag = text.includes("MISSING") || text.includes("FAILED") ? "bad" : "ok";
        if (text.includes("HMC")) state.modules.mag = text.includes("HAZIR") || text.includes("OK") ? "ok" : "bad";
        if (text.includes("GPS")) state.modules.gps = text.includes("FIX") || text.includes("HAZIR") ? "ok" : "bad";
        if (text.includes("RC_MONITOR_OK")) state.modules.rc = "ok";
        if (text.includes("RC_MONITOR_FAIL")) state.modules.rc = "bad";
      }
      if (text.includes("RC_MAP_OK")) {
        state.modules.rc = "ok";
        els.preflightText.textContent = message.text;
      }
      if (text.includes("RC_MAP_FAIL")) {
        state.modules.rc = "bad";
        els.preflightText.textContent = message.text;
      }
      if (text.includes("SENSOR_CHECK_PARTIAL")) {
        els.preflightText.textContent = "Sensor kontrolu kismi basarili: opsiyonel sensorlerden biri eksik.";
      }
      if (handledSensorCheck) {
        renderModules();
        renderI2cDiagnostics();
        renderArmChecklist();
      }
      if (text.includes("PREFLIGHT_OK")) {
        state.lastPreflightText = "PREFLIGHT_OK";
        els.preflightText.textContent = "Preflight OK: sistem arm icin yazilim tarafinda hazir.";
      }
      if (text.includes("PREFLIGHT") && !text.includes("OK")) {
        state.lastPreflightText = message.text;
        els.preflightText.textContent = message.text;
      }
      if (state.commandHistory[0] && state.commandHistory[0].state !== "pending") {
        state.commandHistory[0].detail = message.text;
        renderCommandStatus();
      }
      renderModules();
      renderConfigAudit();
      renderArmChecklist();
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
    if (state.modules.rc === "bad") items.push(["warn", "RC modülü geçerli kanal akışı bildirmiyor."]);

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

  function renderArmChecklist() {
    if (!els.armChecklist) return;
    const heartbeatAgeMs = state.lastHeartbeatMs > 0 ? Date.now() - state.lastHeartbeatMs : Infinity;
    const paramComplete = state.expectedParamCount > 0 && state.params.size >= state.expectedParamCount;
    const rcRequired = getConfigValue("EN_RC", 1) >= 0.5 && getConfigValue("TYPE_RC", 1) !== 0;
    const batteryRequired = getConfigValue("EN_BATT", 1) >= 0.5 && getConfigValue("TYPE_BATT", 1) !== 0;
    const baroRequired = getConfigValue("EN_BARO", 1) >= 0.5;
    const magRequired = getConfigValue("EN_MAG", 1) >= 0.5;
    const gpsRequired = getConfigValue("EN_GPS", 0) >= 0.5;
    const preflightOk = state.lastPreflightText.toUpperCase().includes("PREFLIGHT_OK");
    const preflightBlocked = state.lastPreflightText && !preflightOk;

    const items = [
      checklistItem("Bağlantı", state.connected ? "ok" : "bad", state.connected ? "USB/MAVLink port açık." : "Önce USB Bağlan."),
      checklistItem("Heartbeat", heartbeatAgeMs < 5000 ? "ok" : heartbeatAgeMs < Infinity ? "warn" : "bad", heartbeatAgeMs < 5000 ? "Canlı heartbeat alınıyor." : "Heartbeat yok veya bayat."),
      checklistItem("Parametreler", paramComplete ? "ok" : state.params.size > 0 ? "warn" : "bad", paramComplete ? `${state.params.size}/${state.expectedParamCount} parametre okundu.` : "Parametreleri Oku çalıştır."),
      checklistItem("IMU", state.modules.imu === "ok" ? "ok" : state.modules.imu === "detected" ? "warn" : "bad", state.modules.imu === "ok" ? "MPU sağlıklı raporlandı." : state.modules.imu === "detected" ? "0x68 I2C'de var, register/init zinciri kontrol ediliyor." : "IMU arm için zorunlu."),
      checklistItem("Barometre", optionalModuleState("baro", baroRequired), baroRequired ? moduleChecklistText("baro", "BMP180/BMP085") : "Setup'ta devre dışı, arm engeli değil."),
      checklistItem("Manyetometre", optionalModuleState("mag", magRequired), magRequired ? moduleChecklistText("mag", "HMC/QMC") : "Setup'ta devre dışı, arm engeli değil."),
      checklistItem("GPS", optionalModuleState("gps", gpsRequired), gpsRequired ? moduleChecklistText("gps", "GPS") : "Setup'ta devre dışı, arm engeli değil."),
      checklistItem("Batarya", optionalModuleState("battery", batteryRequired), batteryRequired ? moduleChecklistText("battery", "Batarya ADC") : "Bench/setup gereği batarya arm engeli değil."),
      checklistItem("RC", optionalModuleState("rc", rcRequired), rcRequired ? moduleChecklistText("rc", "SBUS") : "Setup'ta devre dışı, RC failsafe arm engeli değil."),
      checklistItem("Servo/ESC Pinleri", actuatorSetupHealth() === "ok" ? "ok" : "bad", actuatorSetupHealth() === "ok" ? "Pin Mapper servo/ESC çakışması yok." : "Pin Mapper servo/ESC setup çakışması var."),
      checklistItem("Son Preflight", preflightOk ? "ok" : preflightBlocked ? "bad" : "muted", preflightOk ? "Firmware preflight kabul etti." : preflightBlocked ? state.lastPreflightText : "Preflight Kontrol komutu bekleniyor.")
    ];

    els.armChecklist.innerHTML = items.join("");
  }

  function optionalModuleState(moduleId, required) {
    if (!required) return "muted";
    if (state.modules[moduleId] === "ok") return "ok";
    if (state.modules[moduleId] === "detected") return "warn";
    return "bad";
  }

  function moduleChecklistText(moduleId, label) {
    if (state.modules[moduleId] === "ok") return `${label} firmware health içinde sağlıklı.`;
    if (state.modules[moduleId] === "detected") return `${label} I2C'de görünüyor ama firmware healthy demedi.`;
    return `${label} gerekli ama sağlıklı görünmüyor.`;
  }

  function checklistItem(title, kind, detail) {
    return `<div class="checklist-row ${kind}">
      <span class="checklist-mark"></span>
      <div>
        <strong>${escapeHtml(title)}</strong>
        <p>${escapeHtml(detail)}</p>
      </div>
    </div>`;
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
    toast("JSON dışa aktarıldı.", "ok");
  }

  async function importParams(file) {
    const text = await file.text();
    const data = JSON.parse(text);
    for (const [name, value] of Object.entries(data)) {
      if (typeof value !== "number") continue;
      const validation = validateParam(name, value);
      if (validation.ok) {
        state.dirtyParams.set(name, validation.value);
      } else {
        log(`${name}: ice aktarilmadi, ${validation.reason}`);
      }
    }
    renderSettings();
    renderSummary();
    updateDirtyButton();
    log("JSON parametre dosyasi degisiklik olarak yuklendi. Gondermek icin Degisenleri Uygula kullan.");
    toast("JSON içe aktarıldı.", "ok");
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
        if (writePinRoleParam(pin, role)) {
          for (const [mappedPin, mappedRole] of state.pinMap.entries()) {
            if (mappedPin !== pin.n && mappedRole === role) state.pinMap.delete(mappedPin);
          }
          state.pinMap.set(pin.n, role);
          log(`Pin ${pin.n} (${pin.gpio}) -> ${role} olarak atandi.`);
        }
      }
      highlightAssignedPins();
      renderPinAssignments();
    });
  }

  function gpioNumberFromPin(pin) {
    const match = /^GP(\d+)$/.exec(pin && pin.gpio ? pin.gpio : "");
    return match ? Number(match[1]) : null;
  }

  function writePinRoleParam(pin, role) {
    const paramName = PIN_ROLE_PARAMS[role];
    if (!paramName) {
      toast("Bu rol bu firmware'de desteklenmiyor.", "bad");
      log(`${role}: firmware param karsiligi yok, atama uygulanmadi.`);
      return false;
    }
    const gpio = gpioNumberFromPin(pin);
    if (!Number.isFinite(gpio)) {
      toast("Bu pin GPIO değil.", "warn");
      return false;
    }
    if (!validatePinRoleGpio(paramName, gpio)) {
      return false;
    }
    const ok = setParam(paramName, gpio);
    if (ok) {
      toast(`${paramName}=GP${gpio}; Flash'a Kaydet + yeniden baslat.`, "ok");
    }
    return ok;
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
    if (announce) {
      log("AeroPico varsayilan pin haritasi uygulandi.");
      for (const [pinNum, role] of DEFAULT_WIRING) {
        const paramName = PIN_ROLE_PARAMS[role];
        const pin = PIN_DEFS.find((item) => item.n === pinNum);
        if (paramName && pin) writePinRoleParam(pin, role);
      }
      toast("Varsayilan servo/ADC pinleri gonderildi; Flash'a Kaydet + yeniden baslat.", "ok");
    }
  }

  function applyDefaultModuleSetup() {
    applyDefaultPinMap(true);
    const moduleDefaults = [
      ["EN_BARO", 1],
      ["EN_MAG", 1],
      ["EN_GPS", 0],
      ["EN_BATT", 0],
      ["TYPE_IMU", 1],
      ["TYPE_BARO", 1],
      ["TYPE_MAG", 0],
      ["TYPE_GPS", 1],
      ["TYPE_RC", 1],
      ["TYPE_BATT", 0]
    ];
    let staged = 0;
    for (const [name, value] of moduleDefaults) {
      if (stageParam(name, value)) staged++;
    }
    renderModules();
    log(`Pin varsayilanlari gonderildi; modul setup varsayilanlari beklemeye alindi (${staged}/${moduleDefaults.length}).`);
    toast("Pinler gönderildi; modül setup bekliyor. Değişenleri Uygula + Flash'a Kaydet.", "warn");
    els.applyDirtyParamsBtn?.classList.add("attention-pulse");
    window.setTimeout(() => els.applyDirtyParamsBtn?.classList.remove("attention-pulse"), 4000);
  }

  /* ── Bindings ──────────────────────────────── */

  function showTerminalPane(name) {
    const panes = new Set(["preflight", "arm", "i2c", "log", "command", "mavlink"]);
    const pane = panes.has(name) ? name : "preflight";
    els.terminalPreflightBtn.classList.toggle("active", pane === "preflight");
    els.terminalArmChecklistBtn.classList.toggle("active", pane === "arm");
    els.terminalI2cBtn.classList.toggle("active", pane === "i2c");
    els.terminalLogBtn.classList.toggle("active", pane === "log");
    els.terminalCommandBtn.classList.toggle("active", pane === "command");
    els.terminalMavlinkBtn.classList.toggle("active", pane === "mavlink");
    els.preflightPane.classList.toggle("active", pane === "preflight");
    els.armChecklistPane.classList.toggle("active", pane === "arm");
    els.i2cPane.classList.toggle("active", pane === "i2c");
    els.logPane.classList.toggle("active", pane === "log");
    els.commandPane.classList.toggle("active", pane === "command");
    els.mavlinkPane.classList.toggle("active", pane === "mavlink");
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
    els.terminalArmChecklistBtn.addEventListener("click", () => showTerminalPane("arm"));
    els.terminalI2cBtn.addEventListener("click", () => showTerminalPane("i2c"));
    els.terminalLogBtn.addEventListener("click", () => showTerminalPane("log"));
    els.terminalCommandBtn.addEventListener("click", () => showTerminalPane("command"));
    els.terminalMavlinkBtn.addEventListener("click", () => showTerminalPane("mavlink"));
    els.exportBtn.addEventListener("click", exportParams);
    els.importBtn.addEventListener("click", () => els.importInput.click());
    els.importInput.addEventListener("change", () => {
      if (els.importInput.files[0]) importParams(els.importInput.files[0]).catch((error) => log(error.message));
    });
    els.applyDirtyParamsBtn.addEventListener("click", applyDirtyParams);
    els.saveProfileBtn.addEventListener("click", saveCurrentProfile);
    els.profileNameInput.addEventListener("keydown", (event) => {
      if (event.key === "Enter") saveCurrentProfile();
    });
    for (const button of document.querySelectorAll("[data-command]")) {
      button.addEventListener("click", () => sendServiceCommand(button.dataset.command));
    }
    for (const button of document.querySelectorAll("[data-arm-command]")) {
      button.addEventListener("click", () => sendArmCommand(button.dataset.armCommand));
    }
    els.themeToggleBtn.addEventListener("click", toggleTheme);
    els.cancelPortPickBtn.addEventListener("click", () => {
      closeModal(els.portPickerModal);
      if (window.aeropicoBridge && typeof window.aeropicoBridge.cancelSerialPort === "function") {
        window.aeropicoBridge.cancelSerialPort();
      } else if (window.aeropicoBridge) {
        window.aeropicoBridge.chooseSerialPort("");
      }
    });
    if (els.applyDefaultPinsBtn) {
      els.applyDefaultPinsBtn.addEventListener("click", () => {
        applyDefaultPinMap(true);
        highlightAssignedPins();
        renderPinAssignments();
        renderPinDetail();
      });
    }
    if (els.applyDefaultModuleSetupBtn) {
      els.applyDefaultModuleSetupBtn.addEventListener("click", applyDefaultModuleSetup);
    }
    if (els.servoTestPulse && els.servoTestPulseVal) {
      els.servoTestPulse.addEventListener("input", () => {
        els.servoTestPulseVal.textContent = String(clampInteger(els.servoTestPulse.value, 1000, 2000, 1600));
      });
    }
    if (els.openPinsFromModuleSetupBtn) {
      els.openPinsFromModuleSetupBtn.addEventListener("click", () => openModal(els.pinMapperModal));
    }

    bindModals();
    bindCollapsibles();
    bindSideToolTabs();
    bindModuleTabs();
    bindFlowTabs();
    bindRightbarTabs();
    bindBaudSelect();
    bindSerialBridge();
  }

  initTheme();
  bind();
  renderTabs();
  renderModules();
  renderSettings();
  renderCommandStatus();
  renderI2cDiagnostics();
  renderArmChecklist();
  updateButtons();
  updatePortInfoDisplay();
  initPinMapper();
  setInterval(renderSummary, 1000);
  finishSplash();
  log("AeroPico Configurator hazir.");
})();
