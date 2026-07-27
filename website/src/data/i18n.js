import { repoUrl } from "./siteData.js";

export const content = {
  tr: {
    languageLabel: "EN",
    seo: {
      title: "AeroPico-FC",
      description: "RP2350 / Raspberry Pi Pico 2 tabanlı sabit kanat uçuş kontrol yazılımı; FreeRTOS görev yapısı, PIO servo PWM, DMA sensör yolu ve MAVLink telemetri içerir.",
    },
    navItems: [
      { label: "Başlangıç", href: "/#quickstart", highlighted: true },
      { label: "Donanım", href: "/hardware" },
      { label: "Mimari", href: "/#architecture" },
      { label: "Configurator", href: "/#configurator" },
      { label: "Dökümanlar", href: "/#docs" },
      { label: "Koşullar", href: "/releases" },
      { label: "İletişim", href: "/#contact" },
    ],
    header: {
      menu: "Menü",
      features: "Özellikler",
      github: "GitHub",
      releases: "Sürümler",
      homeLabel: "AeroPico FC ana sayfa",
      navLabel: "Ana menü",
      languageToggle: "Dil değiştir",
    },
    hero: {
      eyebrow: "Sabit kanat uçuş kontrol yazılımı",
      titleA: "Küçük Kart",
      titleB: "Ciddi Mimari",
      description:
        "RP2350 / Raspberry Pi Pico 2 için FreeRTOS, PIO, DMA ve MAVLink temelli okunabilir uçuş kontrol yazılımı. Manuel ve stabilize altyapı; masaüstü, HIL ve mühendislik testleri için hazır.",
      primary: "İndir",
      secondary: "Specifications",
      note: "Uçuş öncesi masaüstü doğrulaması zorunludur.",
    },
    home: {
      overviewEyebrow: "PROJE HARİTASI",
      cards: [
        { title: "Desteklenen Donanım", text: "Pico 2, sensörler, RC ve GCS uyumluluk listesini tablo düzeninde inceleyin.", href: "/hardware", action: "Donanımı Aç" },
        { title: "Mimari", text: "Kontrol yolu, sensör rolleri, kalıcı ayarlar ve release gate yaklaşımı.", href: "/#architecture", action: "Mimariye Git" },
        { title: "Sürümler", text: "GitHub release kayıtları, ZIP indirmeleri ve uçuş öncesi kabul koşulları.", href: "/releases", action: "Sürümleri Aç" },
        { title: "Dökümanlar", text: "Geliştirici kılavuzu, RCI release notları ve bench test checklist bağlantıları.", href: "/#docs", action: "Dökümanlara Git" },
      ],
      statusTitle: "Kısa Teknik Durum",
      statusItems: [
        { label: "CONTROL LOOP", value: "500 Hz" },
        { label: "OUTPUT", value: "PIO PWM" },
        { label: "GCS LINK", value: "MAVLink" },
        { label: "GATE", value: "Preflight" },
      ],
      releaseTitle: "Güncel Release",
      releaseAction: "Release Sayfası",
    },
    quickStart: {
      eyebrow: "HIZLI BAŞLANGIÇ",
      title: "Yazılımı Kurun ve Pico 2'ye Yükleyin",
      description: "Projeyi derlemek ya da hazır sürümle başlamak için iki net yol.",
      options: [
        {
          id: "developer",
          theme: "cyan",
          badge: "YOL 1: GELİŞTİRİCİ",
          number: "01",
          title: "Depo Kopyalama & PlatformIO ile Derleme",
          description: "Kaynak kodları bilgisayarınıza indirip PlatformIO üzerinden derleyerek kendi .uf2 dosyanızı oluşturun.",
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
          description: "Kod derleme süreçleriyle uğraşmadan, doğrudan derlenmiş resmi sürüm paketini indirip Pico 2 kartınıza saniyeler içinde flash'layın.",
          steps: [
            {
              title: "Adım A: Sürüm Paketini İndirme",
              body: "Aşağıdaki bağlantıyı kullanarak en güncel kararlı sürüm paketini (.zip) indirin ve arşivden çıkarın:",
              link: { label: "Releases Sayfasına Git", href: "/releases" },
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
          action: { type: "link", label: "Sürümleri Aç", href: "/releases" },
        },
      ],
    },
    architecture: {
      eyebrow: "SİSTEM ÖZELLİKLERİ",
      title: "Çekirdek Tasarım ve Güvenceler",
      description: "Deterministik, modüler ve okunabilir uçuş yazılımı mimarisi.",
      cards: [
        {
          title: "Sabit Kanat Kontrol Yolu",
          text: "RP2350 üzerinde küçük, takip edilebilir ve genişletilebilir bir fixed-wing çekirdek. Kontrol döngüsü, mixer ve failsafe akışı okunabilir sınırlarla ayrılır.",
        },
        {
          title: "Flight-Critical Yol Sade",
          text: "Kontrol döngüsü, mixer, failsafe ve watchdog sorumlulukları ayrıdır; kritik patikada heap kullanımı hedeflenmez.",
        },
        {
          title: "Sensör Rolleri Net",
          text: "IMU, mag ve baro rolleri; chip backendleri ve health monitor ile temiz sınırlar üzerinden çalışır.",
        },
        {
          title: "GCS Uyumluluğu Hazır",
          text: "QGroundControl, Mission Planner ve AeroPico Configurator aynı MAVLink Common akışından beslenir.",
        },
        {
          title: "Ayarlar Kalıcı",
          text: "Parametre ve kalibrasyon zarfları iki slotlu generation + checksum modeliyle flash üzerinde korunur.",
        },
        {
          title: "Bench Odaklı Release",
          text: "v1.0.0-rc1 yazılım RCI seviyesidir; HIL, logic analyzer ve saha kaydı final kabulün parçasıdır.",
        },
      ],
    },
    hardware: {
      eyebrow: "SUPPORTED HARDWARE",
      title: "Hedef Donanım ve Sensör Seti",
      description: "AeroPico-FC, Pico 2 / RP2350 merkezli sabit kanat geliştirme ve bench doğrulama akışı için tasarlanır.",
      columns: {
        role: "Rol",
        component: "Bileşen",
        status: "Durum",
        note: "Not",
      },
      items: [
        { role: "Flight Controller", name: "Raspberry Pi Pico 2", status: "Hedef", note: "RP2350, USB serial ve PIO PWM çıkışları." },
        { role: "IMU", name: "MPU6050", status: "Destek", note: "Gyro + accel backend ve health monitor yolu." },
        { role: "Magnetometer", name: "HMC5883L", status: "Destek", note: "Heading referansı için ayrılmış mag backend." },
        { role: "Barometer", name: "BMP085", status: "Opsiyonel", note: "Bench ve altitude estimator geliştirme yolu." },
        { role: "RC Input", name: "SBUS", status: "Destek", note: "RC pipeline, mapper ve failsafe entegrasyonu." },
        { role: "GCS", name: "MAVLink Common", status: "Destek", note: "QGroundControl, Mission Planner ve Configurator akışı." },
      ],
    },
    configurator: {
      eyebrow: "AEROPICO CONFIGURATOR",
      title: "Kurulum, kalibrasyon ve bench komutları tek arayüzde.",
      description:
        "Configurator, firmware ile ACK-gated MAVLink servis komutları üzerinden konuşur. Parametreleri okur, flash üzerine kaydeder, sensör sağlığını kontrol eder ve güvenli testleri sadece disarmed durumda çalıştırır.",
      action: "Configurator İndir (.zip)",
      checks: [
        "USB Serial bağlantı",
        "MAVLink parametre okuma/yazma",
        "IMU ve mag kalibrasyon",
        "Preflight ve sensor check",
        "RC monitor",
        "Disarmed-only servo test",
      ],
      previewAlt: "AeroPico Configurator arayüzü",
    },
    docs: {
      eyebrow: "DÖKÜMANTASYON",
      title: "Teknik Raporlar ve Kullanım Kılavuzları",
      cards: [
        {
          type: "MD",
          title: "Geliştirici Kullanma Kılavuzu",
          text: "PlatformIO, build, görev mimarisi ve geliştirme akışı için gerçek depo dokümanı.",
          href: `${repoUrl}/blob/main/docs/AeroPico_FC_Gelistirici_Kullanma_Kilavuzu.md`,
          actionLabel: "Dokümanı Aç",
        },
        {
          type: "MD",
          title: "v1.0 RCI Release Notes",
          text: "v1.0.0-rc1 kapsamı, kabul kanıtları ve doğrulama sonuçlarını içeren gerçek release notu.",
          href: `${repoUrl}/blob/main/docs/AeroPico_FC_v1_0_RCI_Release_Notes.md`,
          actionLabel: "Notları Aç",
        },
        {
          type: "MD",
          title: "Bench Test Checklist",
          text: "Uçuş öncesi masaüstü doğrulama ve bench kabul adımları için gerçek kontrol listesi.",
          href: `${repoUrl}/blob/main/docs/Bench_Test_Checklist.md`,
          actionLabel: "Checklist'i Aç",
        },
      ],
    },
    release: {
      eyebrow: "RELEASE GATE",
      title: "Yazılım RCI, uçuş öncesi kanıt ister.",
      gate: "Preflight Verified",
      gateLabel: "GATE",
      body: "Gerçek uçuş veya ticari kullanımdan önce bench doğrulama, HIL kanıtı, airframe tuning ve saha test kayıtları tamamlanmalıdır.",
      pass: "PASS",
      checks: [
        "native test paketi",
        "native full-link entegrasyon",
        "mimari politika kontrolü",
        "fault-injection smoke",
        "Pico firmware build",
        "Configurator statik kontrol",
      ],
      downloads: [
        {
          title: "AeroPico-FC v1.0.0-rc1 Software RCI",
          version: "v1.0.0-rc1",
          badge: "Pre-release",
          badgeTone: "cyan",
          published: "13 Temmuz 2026",
          text: "Son gerçek GitHub release kaydı. MAVLink servis komutları, Configurator entegrasyonu, sensör backend ayrımı ve RCI doğrulama sonuçlarını içerir.",
          href: `${repoUrl}/releases/tag/v1.0.0-rc1`,
          downloadHref: `${repoUrl}/archive/refs/tags/v1.0.0-rc1.zip`,
        },
        {
          title: "picoport2",
          version: "picoport2",
          badge: "Release",
          badgeTone: "blue",
          published: "1 Temmuz 2026",
          text: "GitHub release listesindeki picoport2 kaydı. Release gövdesi kısa: major update.",
          href: `${repoUrl}/releases/tag/picoport2`,
          downloadHref: `${repoUrl}/archive/refs/tags/picoport2.zip`,
        },
        {
          title: "alpha_v0.3",
          version: "v0.2.0",
          badge: "Release",
          badgeTone: "emerald",
          published: "24 Haziran 2026",
          text: "FreeRTOS, PIO PWM, DMA, MAVLink, watchdog, failsafe ve mimari temel güncellemelerini taşıyan gerçek v0.2.0 release kaydı.",
          href: `${repoUrl}/releases/tag/v0.2.0`,
          downloadHref: `${repoUrl}/archive/refs/tags/v0.2.0.zip`,
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
        },
      ],
    },
    footer: {
      contact: "İletişim",
      contactText: "Daha fazla bilgi, sorular ve geri bildirim için",
      releasePolicy: "Yayın Koşulları",
      copyright: "© 2026 AeroPico Projesi. MIT Lisanslı Uçuş Kontrol Yazılımı.",
    },
    modals: {
      releasesTitle: "Yazılım Sürümleri",
      releasesDescription: "GitHub release listesinden canlı çekilir; bağlantı yoksa yerel liste kullanılır.",
      loading: "Sürümler yükleniyor...",
      fallback: "GitHub API geçici olarak kullanılamıyor; yerel release listesi gösteriliyor.",
      source: "Kaynak kod deposu",
      viewGithub: "GitHub'da Görüntüle →",
      closeReleases: "Sürümler penceresini kapat",
      specsTitle: "Specifications",
      specsDescription: "AeroPico-FC sistem tasarım ilkeleri, donanım katmanı ve yazılım yığını detayları.",
      stack: "UÇUŞ YAZILIMI STACK",
      stackTitle: "RP2350 / Pico 2 için uçuş kontrol yazılımı.",
      hardwareLinkTitle: "Hardware Platform",
      hardwareLinkText: "Donanım uyumluluğu, hedef kart ve sensör desteği ayrı sayfada listelenir.",
      hardwareLinkAction: "Hardware Sayfasını Aç",
      close: "Kapat",
    },
    specsIntro:
      "AeroPico-FC, RP2350 tabanlı sabit kanat uçuş kontrolcüsü mimarisi üzerine geliştirilmiş, düşük gecikmeli ve deterministik çalışmayı hedefleyen açık kaynak bir flight controller projesidir.",
    specSections: [
      {
        title: "Uçuş Yazılımı Stack",
        intro: "RP2350 / Pico 2 için uçuş kontrol yazılımı.",
        items: [
          {
            title: "RP2350 / Pico 2 Tabanlı Mimari",
            text: "Dual-core Cortex-M33 yapısı üzerine kurulu, düşük gecikmeli sabit kanat uçuş kontrol altyapısı. Gerçek zamanlı iş yükleri core/task ayrımıyla düzenlenir.",
          },
          {
            title: "Deterministik Uçuş Döngüsü",
            text: "500 Hz sınıfı kontrol döngüsü, scheduler tabanlı görev ayrımı, phase drift kontrolü, timing health izleme ve watchdog gate mantığıyla çalışır.",
          },
          {
            title: "Jitter Kontrollü Servo Çıkışı",
            text: "Servo sinyalleri RP2350 PIO altyapısı ile üretilir. Dinamik divider ile 1000-2000 µs pulse aralığı korunur ve frame zamanlaması deterministik tutulur.",
          },
          {
            title: "Sabit Kanat Kontrol Sistemi",
            text: "MANUAL ve STABILIZE modları, cascaded PID kontrol, fixed-wing mixer, servo trim/reverse/min/max, mixer gain ve failsafe output desteği.",
          },
        ],
      },
      {
        title: "Sensör Altyapısı",
        intro: "MPU6050 IMU, BMP180/BMP085 barometre ve HMC/QMC uyumlu manyetometre backend mimarisi. Role/backend ayrımı yeni sensörlerin eklenmesini kolaylaştırır.",
        items: [
          {
            title: "DMA Destekli Sensör Okuma",
            text: "IMU ve yardımcı sensör okumalarında DMA destekli I2C yolu, bounded polling fallback, fault-code ayrımı ve sensor health izleme kullanılır.",
          },
          {
            title: "Attitude ve İrtifa Kestirimi",
            text: "Madgwick tabanlı yönelim kestirimi, adaptif beta altyapısı ve BaroVerticalKalman ile irtifa/dikey hız filtresi.",
          },
          {
            title: "Dikey İvme Destekli Altitude Estimator",
            text: "BaroVerticalKalman yalnızca barometreye dayanmaz; attitude üzerinden dünya çerçevesine projekte edilen dikey ivme girdisini de kullanır.",
          },
        ],
      },
      {
        title: "İletişim ve Güvenlik",
        items: [
          {
            title: "MAVLink Uyumluluğu",
            text: "USB üzerinden AeroPico Configurator, QGroundControl ve Mission Planner ile temel MAVLink bağlantısı. HEARTBEAT, SYS_STATUS, ATTITUDE, VFR_HUD, GPS_RAW_INT, STATUSTEXT, parametre ve command akışları desteklenir.",
          },
          {
            title: "Güvenlik ve Preflight Sistemi",
            text: "Pre-arm checks, RC failsafe, sensor health, battery state, watchdog gate, timing monitor, actuator readiness ve safe output davranışı.",
          },
          {
            title: "Watchdog Gate Mimarisi",
            text: "Watchdog yalnızca flight loop, sensor loop, telemetry heartbeat ve scheduler health sağlıklıysa beslenir. Tek bir task canlı diye sistem sağlıklı kabul edilmez.",
          },
        ],
      },
      {
        title: "Test, Yapılandırma ve Mimari",
        items: [
          {
            title: "Blackbox ve Test Altyapısı",
            text: "Blackbox event logging, native unit tests, fault-injection smoke testleri, HIL/bench checklist, MAVLink probe ve logic analyzer doğrulama akışı.",
          },
          {
            title: "Configurator Desteği",
            text: "Parametre yönetimi, servo setup, RC mapping, battery setup, preflight reason paneli, bench test araçları ve modül durum izleme.",
          },
          {
            title: "Statik Bellek ve Düşük Gecikme",
            text: "Kritik uçuş yolunda heap kullanımından kaçınılır. Ring buffer, latest-value mailbox, bounded execution ve task-owned state yaklaşımı kullanılır.",
          },
          {
            title: "Modüler ve Sürdürülebilir Tasarım",
            text: "HAL/driver ayrımı, sensor backend sistemi, typed blackboard, service command mailbox ve uzun vadeli geliştirilebilir mimari.",
          },
        ],
      },
    ],
  },
  en: {
    languageLabel: "TR",
    seo: {
      title: "AeroPico-FC",
      description: "Fixed-wing flight control software for RP2350 / Raspberry Pi Pico 2 with FreeRTOS tasking, PIO servo PWM, DMA sensor acquisition, and MAVLink telemetry.",
    },
    navItems: [
      { label: "Start", href: "/#quickstart", highlighted: true },
      { label: "Hardware", href: "/hardware" },
      { label: "Architecture", href: "/#architecture" },
      { label: "Configurator", href: "/#configurator" },
      { label: "Docs", href: "/#docs" },
      { label: "Releases", href: "/releases" },
      { label: "Contact", href: "/#contact" },
    ],
    header: {
      menu: "Menu",
      features: "Specs",
      github: "GitHub",
      releases: "Releases",
      homeLabel: "AeroPico FC home",
      navLabel: "Main menu",
      languageToggle: "Change language",
    },
    hero: {
      eyebrow: "Fixed-wing flight control software",
      titleA: "Small Board",
      titleB: "Serious Architecture",
      description:
        "Readable flight-control software for RP2350 / Raspberry Pi Pico 2, built around FreeRTOS, PIO, DMA, and MAVLink. Ready for manual/stabilized workflows, desktop validation, HIL, and engineering tests.",
      primary: "Download",
      secondary: "Specifications",
      note: "Desktop validation is required before flight.",
    },
    home: {
      overviewEyebrow: "PROJECT MAP",
      cards: [
        { title: "Supported Hardware", text: "Review Pico 2, sensors, RC input, and GCS compatibility in a list layout.", href: "/hardware", action: "Open Hardware" },
        { title: "Architecture", text: "Control path, sensor roles, persistent settings, and release-gate strategy.", href: "/#architecture", action: "Open Architecture" },
        { title: "Releases", text: "GitHub releases, ZIP downloads, and preflight acceptance conditions.", href: "/releases", action: "Open Releases" },
        { title: "Docs", text: "Developer guide, RCI release notes, and bench test checklist links.", href: "/#docs", action: "Open Docs" },
      ],
      statusTitle: "Technical Snapshot",
      statusItems: [
        { label: "CONTROL LOOP", value: "500 Hz" },
        { label: "OUTPUT", value: "PIO PWM" },
        { label: "GCS LINK", value: "MAVLink" },
        { label: "GATE", value: "Preflight" },
      ],
      releaseTitle: "Current Release",
      releaseAction: "Release Page",
    },
    quickStart: {
      eyebrow: "QUICK START",
      title: "Build the Software and Flash Pico 2",
      description: "Two clear paths: build from source or start from an official release package.",
      options: [
        {
          id: "developer",
          theme: "cyan",
          badge: "PATH 1: DEVELOPER",
          number: "01",
          title: "Clone Repository & Build with PlatformIO",
          description: "Download the source and build your own .uf2 file through PlatformIO.",
          steps: [
            {
              title: "Step A: Clone the Repository",
              body: "Open a terminal and clone the project with:",
              code: "git clone https://github.com/ozcelik329/AeroPico-FC.git",
            },
            {
              title: "Step B: Build with PlatformIO",
              body: "Open the project in VS Code with PlatformIO and run Build to generate the .uf2 output.",
            },
            {
              title: "Step C: Flash Pico 2",
              body: "Hold BOOTSEL while connecting the Raspberry Pi Pico 2 over USB, then copy the generated .uf2 file onto the mounted drive.",
            },
          ],
          footer: "Recommended: customizable",
          action: { type: "link", label: "Open Repository", href: repoUrl },
        },
        {
          id: "quick-install",
          theme: "blue",
          badge: "PATH 2: QUICK INSTALL",
          number: "02",
          title: "Download a Release and Flash Directly",
          description: "Skip local builds and flash an official release package to Pico 2 in seconds.",
          steps: [
            {
              title: "Step A: Download the Release Package",
              body: "Use the link below to download the latest stable package and extract it:",
              link: { label: "Open Releases", href: "/releases" },
            },
            {
              title: "Step B: Flash in Boot Mode",
              body: "Connect Pico 2 in BOOTSEL mode and copy the downloaded .uf2 file directly to the mounted drive.",
            },
            {
              title: "Step C: Connect with Configurator",
              body: "Open AeroPico Configurator, select the serial port, and connect through MAVLink.",
            },
          ],
          footer: "Fastest path",
          action: { type: "link", label: "Open Releases", href: "/releases" },
        },
      ],
    },
    architecture: {
      eyebrow: "SYSTEM DESIGN",
      title: "Core Architecture and Guarantees",
      description: "A deterministic, modular, and readable flight software architecture.",
      cards: [
        {
          title: "Fixed-Wing Control Path",
          text: "A small, traceable, and extensible fixed-wing core on RP2350. The control loop, mixer, and failsafe flow are separated with readable boundaries.",
        },
        {
          title: "Clean Flight-Critical Path",
          text: "Control loop, mixer, failsafe, and watchdog responsibilities are separated; heap usage is avoided on the critical path.",
        },
        {
          title: "Clear Sensor Roles",
          text: "IMU, magnetometer, and barometer roles run through clean chip backends and health-monitor boundaries.",
        },
        {
          title: "GCS Ready",
          text: "QGroundControl, Mission Planner, and AeroPico Configurator consume the same MAVLink Common stream.",
        },
        {
          title: "Persistent Settings",
          text: "Parameters and calibration envelopes are protected in flash with a two-slot generation + checksum model.",
        },
        {
          title: "Bench-Focused Release",
          text: "v1.0.0-rc1 is a software RCI; HIL, logic-analyzer evidence, and field logs are part of final acceptance.",
        },
      ],
    },
    hardware: {
      eyebrow: "SUPPORTED HARDWARE",
      title: "Target Board and Sensor Set",
      description: "AeroPico-FC is shaped around a Pico 2 / RP2350 fixed-wing bench and validation workflow.",
      columns: {
        role: "Role",
        component: "Component",
        status: "Status",
        note: "Note",
      },
      items: [
        { role: "Flight Controller", name: "Raspberry Pi Pico 2", status: "Target", note: "RP2350, USB serial, and PIO PWM outputs." },
        { role: "IMU", name: "MPU6050", status: "Supported", note: "Gyro + accel backend with health monitoring." },
        { role: "Magnetometer", name: "HMC5883L", status: "Supported", note: "Dedicated mag backend for heading reference." },
        { role: "Barometer", name: "BMP085", status: "Optional", note: "Bench and altitude-estimator development path." },
        { role: "RC Input", name: "SBUS", status: "Supported", note: "RC pipeline, mapper, and failsafe integration." },
        { role: "GCS", name: "MAVLink Common", status: "Supported", note: "Feeds QGroundControl, Mission Planner, and Configurator." },
      ],
    },
    configurator: {
      eyebrow: "AEROPICO CONFIGURATOR",
      title: "Setup, calibration, and bench commands in one interface.",
      description:
        "Configurator talks to the firmware through ACK-gated MAVLink service commands. It reads parameters, saves to flash, checks sensor health, and runs safe tests only while disarmed.",
      action: "Download Configurator (.zip)",
      checks: [
        "USB serial connection",
        "MAVLink parameter read/write",
        "IMU and magnetometer calibration",
        "Preflight and sensor checks",
        "RC monitor",
        "Disarmed-only servo test",
      ],
      previewAlt: "AeroPico Configurator interface",
    },
    docs: {
      eyebrow: "DOCUMENTATION",
      title: "Technical Reports and Usage Guides",
      cards: [
        {
          type: "MD",
          title: "Developer User Guide",
          text: "Real repository document for PlatformIO, build flow, task architecture, and development workflow.",
          href: `${repoUrl}/blob/main/docs/AeroPico_FC_Gelistirici_Kullanma_Kilavuzu.md`,
          actionLabel: "Open Document",
        },
        {
          type: "MD",
          title: "v1.0 RCI Release Notes",
          text: "Real release notes covering v1.0.0-rc1 scope, acceptance evidence, and validation results.",
          href: `${repoUrl}/blob/main/docs/AeroPico_FC_v1_0_RCI_Release_Notes.md`,
          actionLabel: "Open Notes",
        },
        {
          type: "MD",
          title: "Bench Test Checklist",
          text: "Real checklist for preflight desktop validation and bench acceptance.",
          href: `${repoUrl}/blob/main/docs/Bench_Test_Checklist.md`,
          actionLabel: "Open Checklist",
        },
      ],
    },
    release: {
      eyebrow: "RELEASE GATE",
      title: "The software RCI requires evidence before flight.",
      gate: "Preflight Verified",
      gateLabel: "GATE",
      body: "Before real flight or commercial use, bench validation, HIL evidence, airframe tuning, and field test logs must be completed.",
      pass: "PASS",
      checks: [
        "native test package",
        "native full-link integration",
        "architecture policy check",
        "fault-injection smoke",
        "Pico firmware build",
        "Configurator static check",
      ],
      downloads: [
        {
          title: "AeroPico-FC v1.0.0-rc1 Software RCI",
          version: "v1.0.0-rc1",
          badge: "Pre-release",
          badgeTone: "cyan",
          published: "July 13, 2026",
          text: "Latest real GitHub release entry. Includes MAVLink service commands, Configurator integration, sensor backend separation, and RCI validation results.",
          href: `${repoUrl}/releases/tag/v1.0.0-rc1`,
          downloadHref: `${repoUrl}/archive/refs/tags/v1.0.0-rc1.zip`,
        },
        {
          title: "picoport2",
          version: "picoport2",
          badge: "Release",
          badgeTone: "blue",
          published: "July 1, 2026",
          text: "The picoport2 entry from the GitHub releases list. Release body is short: major update.",
          href: `${repoUrl}/releases/tag/picoport2`,
          downloadHref: `${repoUrl}/archive/refs/tags/picoport2.zip`,
        },
        {
          title: "alpha_v0.3",
          version: "v0.2.0",
          badge: "Release",
          badgeTone: "emerald",
          published: "June 24, 2026",
          text: "Real v0.2.0 release entry with FreeRTOS, PIO PWM, DMA, MAVLink, watchdog, failsafe, and core architecture updates.",
          href: `${repoUrl}/releases/tag/v0.2.0`,
          downloadHref: `${repoUrl}/archive/refs/tags/v0.2.0.zip`,
        },
        {
          title: "Alpha Release v0.2",
          version: "alpha_v0.2",
          badge: "Release",
          badgeTone: "amber",
          published: "June 23, 2026",
          text: "Early stable base release with FreeRTOS task isolation, PIO PWM, DMA sensor reads, and MAVLink v2 integration.",
          href: `${repoUrl}/releases/tag/alpha_v0.2`,
          downloadHref: `${repoUrl}/archive/refs/tags/alpha_v0.2.zip`,
        },
        {
          title: "Alpha Release v0.1",
          version: "alpha",
          badge: "Release",
          badgeTone: "rose",
          published: "June 22, 2026",
          text: "First alpha release entry based on the src2 refactor.",
          href: `${repoUrl}/releases/tag/alpha`,
          downloadHref: `${repoUrl}/archive/refs/tags/alpha.zip`,
        },
      ],
    },
    footer: {
      contact: "Contact",
      contactText: "For more information, questions, and feedback:",
      releasePolicy: "Release Gate",
      copyright: "© 2026 AeroPico Project. MIT licensed flight-control software.",
    },
    modals: {
      releasesTitle: "Software Releases",
      releasesDescription: "Loaded live from GitHub releases; falls back to the local list when unavailable.",
      loading: "Loading releases...",
      fallback: "GitHub API is temporarily unavailable; showing the local release list.",
      source: "Source repository",
      viewGithub: "View on GitHub →",
      closeReleases: "Close releases dialog",
      specsTitle: "Specifications",
      specsDescription: "AeroPico-FC system design principles, hardware layer, and software stack details.",
      stack: "FLIGHT SOFTWARE STACK",
      stackTitle: "Flight control software for RP2350 / Pico 2.",
      hardwareLinkTitle: "Hardware Platform",
      hardwareLinkText: "Hardware compatibility, target board, and sensor support are listed on a dedicated page.",
      hardwareLinkAction: "Open Hardware Page",
      close: "Close",
    },
    specsIntro:
      "AeroPico-FC is an open-source flight controller project built around an RP2350 fixed-wing control architecture, targeting low latency and deterministic operation.",
    specSections: [
      {
        title: "Flight Software Stack",
        intro: "Flight control software for RP2350 / Pico 2.",
        items: [
          {
            title: "RP2350 / Pico 2 Based Architecture",
            text: "A low-latency fixed-wing flight-control foundation built on the dual-core Cortex-M33 RP2350. Real-time workloads are separated by core and task ownership.",
          },
          {
            title: "Deterministic Flight Loop",
            text: "A 500 Hz class control loop with scheduler-based task separation, phase-drift control, timing-health monitoring, and watchdog-gate logic.",
          },
          {
            title: "Jitter-Controlled Servo Output",
            text: "Servo signals are generated through RP2350 PIO. A dynamic divider preserves the 1000-2000 µs pulse range and keeps servo-frame timing deterministic.",
          },
          {
            title: "Fixed-Wing Control System",
            text: "MANUAL and STABILIZE modes, cascaded PID control, fixed-wing mixer, servo trim/reverse/min/max, mixer gain, and failsafe output support.",
          },
        ],
      },
      {
        title: "Sensor Architecture",
        intro: "MPU6050 IMU, BMP180/BMP085 barometer, and HMC/QMC-compatible magnetometer backend architecture. Role/backend separation keeps future sensor additions manageable.",
        items: [
          {
            title: "DMA Assisted Sensor Read Path",
            text: "IMU and auxiliary sensor reads use a DMA-assisted I2C path, bounded polling fallback, fault-code separation, and sensor-health monitoring.",
          },
          {
            title: "Attitude and Altitude Estimation",
            text: "Madgwick-based attitude estimation, adaptive beta infrastructure, and BaroVerticalKalman altitude/vertical-speed filtering.",
          },
          {
            title: "Vertical Acceleration Assisted Altitude Estimator",
            text: "BaroVerticalKalman does not rely only on the barometer; it also uses vertical acceleration projected into the world frame through attitude.",
          },
        ],
      },
      {
        title: "Communication and Safety",
        items: [
          {
            title: "MAVLink Compatibility",
            text: "Basic MAVLink connectivity over USB for AeroPico Configurator, QGroundControl, and Mission Planner. HEARTBEAT, SYS_STATUS, ATTITUDE, VFR_HUD, GPS_RAW_INT, STATUSTEXT, parameter, and command flows are supported.",
          },
          {
            title: "Safety and Preflight System",
            text: "Pre-arm checks, RC failsafe, sensor health, battery state, watchdog gate, timing monitor, actuator readiness, and safe-output behavior.",
          },
          {
            title: "Watchdog Gate Architecture",
            text: "The watchdog is fed only when the flight loop, sensor loop, telemetry heartbeat, and scheduler health are all valid. A single alive task is not treated as system health.",
          },
        ],
      },
      {
        title: "Test, Configuration, and Architecture",
        items: [
          {
            title: "Blackbox and Test Infrastructure",
            text: "Blackbox event logging, native unit tests, fault-injection smoke tests, HIL/bench checklist, MAVLink probe, and logic-analyzer validation flow.",
          },
          {
            title: "Configurator Support",
            text: "Parameter management, servo setup, RC mapping, battery setup, preflight reason panel, bench-test tools, and module status monitoring.",
          },
          {
            title: "Static Memory and Low-Latency Discipline",
            text: "Heap usage is avoided on the critical flight path. Ring buffers, latest-value mailboxes, bounded execution, and task-owned state keep data flow controlled.",
          },
          {
            title: "Modular and Maintainable Design",
            text: "HAL/driver separation, sensor backend system, typed blackboard, service command mailbox, and a long-term extensible architecture.",
          },
        ],
      },
    ],
  },
};
