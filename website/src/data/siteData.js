export const repoUrl = "https://github.com/ozcelik329/AeroPico-FC";

export const navItems = [
  { label: "Proje Haritası", href: "#project-map" },
  { label: "Hızlı Başlangıç", href: "#quickstart", highlighted: true },
  { label: "Configurator", href: "#configurator" },
  { label: "Dökümanlar", href: "#docs" },
];

export const telemetryRows = [
  { label: "TARGET MCU:", value: "RP2350 (Dual Cortex-M33)", tone: "text-white" },
  { label: "LOOP FREQUENCY:", value: "500.0 Hz (Jitter < 0.2ms)", tone: "text-cyan-400" },
  { label: "SENSOR BUS (DMA):", value: "I2C Fast Path (OK)", tone: "text-emerald-400" },
  { label: "ACTUATOR OUTPUT:", value: "PIO PWM (8 Channel)", tone: "text-amber-400" },
];

export const heroStats = [
  { label: "CONTROL LOOP", value: "500 Hz", tone: "text-emerald-300" },
  { label: "GCS LİNK", value: "MAVLink", tone: "text-cyan-400" },
];

export const quickStartOptions = [
  {
    id: "developer",
    theme: "cyan",
    badge: "YOL 1: GELİŞTİRİCİ",
    number: "01",
    title: "Depo Kopyalama & PlatformIO ile Derleme",
    description:
      "Kaynak kodları bilgisayarınıza indirip PlatformIO üzerinden derleyerek kendi .uf2 dosyanızı oluşturun.",
    steps: [
      {
        title: "Adım A: Depoyu Kopyalama (Clone)",
        body: "Bilgisayarınızda terminali açın ve projeyi klonlamak için şu komutu girin:",
        code: "git clone https://github.com/ozcelik329/AeroPico-FC.git",
      },
      {
        title: "Adım B: PlatformIO ile Derleme",
        body: "VS Code içerisinde PlatformIO eklentisiyle projeyi açın, sol menüden Build butonuna basarak .uf2 çıktısını üretin.",
      },
      {
        title: "Adım C: Pico 2'ye Yükleme",
        body: "Raspberry Pi Pico 2 kartınızın üzerindeki BOOTSEL butonuna basılı tutarak bilgisayara USB ile bağlayın. Sürücü olarak açılan diske derlenen .uf2 dosyasını atın.",
      },
    ],
    footer: "Önerilen: Özelleştirilebilir",
    action: { type: "link", label: "Depoya Git", href: repoUrl },
  },
  {
    id: "quick-install",
    theme: "blue",
    badge: "YOL 2: HIZLI KURULUM",
    number: "02",
    title: "Hazır Sürüm İndirme ve Doğrudan Yükleme",
    description:
      "Kod derleme süreçleriyle uğraşmadan, doğrudan derlenmiş resmi sürüm paketini indirip Pico 2 kartınıza saniyeler içinde flash'layın.",
    steps: [
      {
        title: "Adım A: Sürüm Paketini İndirme",
        body: "Aşağıdaki bağlantıyı kullanarak en güncel kararlı sürüm paketini (.zip) indirin ve arşivden çıkarın:",
        link: {
          label: "Releases Sayfasına Git",
          href: `${repoUrl}/releases/latest`,
        },
      },
      {
        title: "Adım B: Boot Modunda Yükleme",
        body: "Pico 2 kartınızı BOOTSEL modunda bilgisayara bağlayın ve indirdiğiniz .uf2 dosyasını doğrudan bu sürücüye kopyalayın.",
      },
      {
        title: "Adım C: Configurator ile Bağlanma",
        body: "AeroPico Configurator uygulamasını indirin, kartınızı takıp uygun seri portu seçerek MAVLink üzerinden bağlantı kurun.",
      },
    ],
    footer: "En Hızlı Yöntem",
    action: { type: "modal", label: "Sürümleri Aç" },
  },
];

export const architectureCards = [
  {
    title: "Sabit Kanat Kontrol Yolu",
    text: "RP2350 üzerinde küçük, takip edilebilir ve genişletilebilir bir fixed-wing çekirdek. Kontrol döngüsü, mixer ve failsafe akışı okunabilir sınırlarla ayrılır.",
    tone: "cyan",
  },
  {
    title: "Flight-Critical Yol Sade",
    text: "Kontrol döngüsü, mixer, failsafe ve watchdog sorumlulukları ayrıdır; kritik patikada heap kullanımı hedeflenmez.",
    tone: "amber",
  },
  {
    title: "Sensör Rolleri Net",
    text: "IMU, mag ve baro rolleri; chip backendleri ve health monitor ile temiz sınırlar üzerinden çalışır.",
    tone: "emerald",
  },
  {
    title: "GCS Uyumluluğu Hazır",
    text: "QGroundControl, Mission Planner ve AeroPico Configurator aynı MAVLink Common akışından beslenir.",
    tone: "blue",
  },
  {
    title: "Ayarlar Kalıcı",
    text: "Parametre ve kalibrasyon zarfları iki slotlu generation + checksum modeliyle flash üzerinde korunur.",
    tone: "violet",
  },
  {
    title: "Bench Odaklı Release",
    text: "v1.0.0-rc1 yazılım RCI seviyesidir; HIL, logic analyzer ve saha kaydı final kabulün parçasıdır.",
    tone: "rose",
  },
];

export const configuratorChecks = [
  "USB Serial bağlantı",
  "MAVLink parametre okuma/yazma",
  "IMU ve mag kalibrasyon",
  "Preflight ve sensor check",
  "RC monitor",
  "Disarmed-only servo test",
];

export const docCards = [
  {
    type: "MD",
    title: "Geliştirici Kullanma Kılavuzu",
    text: "PlatformIO, build, görev mimarisi ve geliştirme akışı için gerçek depo dokümanı.",
    href: `${repoUrl}/blob/main/docs/AeroPico_FC_Gelistirici_Kullanma_Kilavuzu.md`,
    actionLabel: "Dokümanı Aç",
    tone: "cyan",
  },
  {
    type: "MD",
    title: "v1.0 RCI Release Notes",
    text: "v1.0.0-rc1 kapsamı, kabul kanıtları ve doğrulama sonuçlarını içeren gerçek release notu.",
    href: `${repoUrl}/blob/main/docs/AeroPico_FC_v1_0_RCI_Release_Notes.md`,
    actionLabel: "Notları Aç",
    tone: "amber",
  },
  {
    type: "MD",
    title: "Bench Test Checklist",
    text: "Uçuş öncesi masaüstü doğrulama ve bench kabul adımları için gerçek kontrol listesi.",
    href: `${repoUrl}/blob/main/docs/Bench_Test_Checklist.md`,
    actionLabel: "Checklist'i Aç",
    tone: "emerald",
  },
];

export const releaseChecks = [
  "native test paketi",
  "native full-link entegrasyon",
  "mimari politika kontrolu",
  "fault-injection smoke",
  "Pico firmware build",
  "Configurator statik kontrol",
];

export const releaseDownloads = [
  {
    title: "AeroPico-FC v1.0.0-rc1 Software RCI",
    version: "v1.0.0-rc1",
    badge: "Pre-release",
    badgeTone: "cyan",
    published: "13 Temmuz 2026",
    text: "MAVLink servis komutları, Configurator entegrasyonu, sensör backend ayrımı ve RCI doğrulama sonuçlarını içerir.",
    href: `${repoUrl}/releases/tag/v1.0.0-rc1`,
    downloadHref: `${repoUrl}/archive/refs/tags/v1.0.0-rc1.zip`,
    actionLabel: "Release'i Aç",
  },
  {
    title: "picoport2",
    version: "picoport2",
    badge: "Release",
    badgeTone: "blue",
    published: "1 Temmuz 2026",
    text: "Pico 2 portlama süreci ve temel firmware güncellemelerini içeren ara sürüm.",
    href: `${repoUrl}/releases/tag/picoport2`,
    downloadHref: `${repoUrl}/archive/refs/tags/picoport2.zip`,
    actionLabel: "Release'i Aç",
  },
  {
    title: "alpha_v0.3",
    version: "v0.2.0",
    badge: "Release",
    badgeTone: "emerald",
    published: "24 Haziran 2026",
    text: "FreeRTOS, PIO PWM, DMA, MAVLink, watchdog, failsafe ve mimari temel güncellemelerini taşır.",
    href: `${repoUrl}/releases/tag/v0.2.0`,
    downloadHref: `${repoUrl}/archive/refs/tags/v0.2.0.zip`,
    actionLabel: "Release'i Aç",
  },
  {
    title: "Alpha Release v0.2",
    version: "alpha_v0.2",
    badge: "Release",
    badgeTone: "amber",
    published: "23 Haziran 2026",
    text: "İlk stabil temel sürüm olarak FreeRTOS görev izolasyonu, PIO PWM, DMA sensör okuma ve MAVLink v2 entegrasyonu içerir.",
    href: `${repoUrl}/releases/tag/alpha_v0.2`,
    downloadHref: `${repoUrl}/archive/refs/tags/alpha_v0.2.zip`,
    actionLabel: "Release'i Aç",
  },
  {
    title: "Alpha Release v0.1",
    version: "alpha",
    badge: "Release",
    badgeTone: "rose",
    published: "22 Haziran 2026",
    text: "src2 refactor tabanlı ilk alpha release kaydı.",
    href: `${repoUrl}/releases/tag/alpha`,
    downloadHref: `${repoUrl}/archive/refs/tags/alpha.zip`,
    actionLabel: "Release'i Aç",
  },
];

export const specifications = [
  { label: "CONTROL LOOP", value: "500 Hz", text: "Yüksek frekanslı kararlı kontrol döngüsü." },
  { label: "OUTPUT", value: "PIO PWM", text: "Donanımsal esneklik sağlayan PIO tabanlı PWM sinyal üretimi." },
  { label: "GCS LİNK", value: "MAVLink", text: "Yer istasyonu haberleşmesi için standart protokol entegrasyonu." },
  { label: "GATE", value: "Preflight", text: "Uçuş öncesi zorunlu güvenlik ve doğrulama denetimleri." },
  { label: "HARDWARE", value: "DUAL-CORE", text: "Çift çekirdekli işlem gücü ile iş yükü dağılımı." },
  { label: "OS MİMARİSİ", value: "FreeRTOS", text: "Gerçek zamanlı işletim sistemi altyapısı." },
  { label: "GÖREV YÖNETİMİ", value: "TASK İSOLATİON", text: "Kritik görevlerin birbirinden bağımsız çalıştırılması." },
  { label: "PROTOKOL STANDARDI", value: "MAVLink Common", text: "Ortak mesaj setleriyle genişletilebilir telemetri." },
  { label: "HABERLEŞME", value: "USB + UART", text: "Esnek seri bağlantı ve hata ayıklama kanalları." },
  { label: "AKTÜATÖR", value: "SERVO OUTPUT", text: "Sabit kanat kontrol yüzeyleri için optimize çıkışlar." },
  { label: "VERİ AKIŞI", value: "DMA I2C", text: "İşlemciye yük bindirmeyen donanımsal bellek erişimli sensör okuma." },
  { label: "SENSÖR YOLU", value: "SENSOR FAST PATH", text: "IMU verileri için en kısa gecikmeli işleme yolu." },
  {
    label: "BUILD SİSTEMİ",
    value: "PlatformIO & REPEATABLE BUILD",
    text: "Tekrarlanabilir, hatasız ve modüler derleme süreçleri.",
    wide: true,
  },
];
