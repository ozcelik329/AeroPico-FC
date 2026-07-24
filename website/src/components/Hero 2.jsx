import logo from "../assets/aeropico-logo-full.png";
import { heroStats, telemetryRows } from "../data/siteData.js";
import { useModal } from "../hooks/useModal.js";

export default function Hero() {
  const { openReleases, openSpecs } = useModal();

  return (
    <main id="top" className="site-container py-16 md:py-24 grid md:grid-cols-2 gap-12 items-center relative z-10">
      <div className="space-y-6 relative z-10">
        
        {/* Beyaz Şeffaf / Cam Efektli Logo Kutusu */}
        <div className="relative w-full max-w-[340px] p-6 rounded-2xl bg-white/10 border border-white/20 backdrop-blur-md shadow-[0_0_30px_rgba(255,255,255,0.15)]">
          {/* Arkadaki Yumuşak Parlama Efekti */}
          <div className="absolute left-1/2 top-1/2 h-[130%] w-[130%] -translate-x-1/2 -translate-y-1/2 rounded-full bg-[radial-gradient(circle,rgba(255,255,255,0.25)_0%,rgba(34,211,238,0.15)_50%,transparent_70%)] blur-lg pointer-events-none" />
          
          <img
            src={logo}
            alt="AeroPico Flight Control Software"
            className="relative z-10 w-full object-contain drop-shadow-[0_4px_12px_rgba(0,0,0,0.3)]"
          />
        </div>

        <div className="inline-flex items-center gap-2 bg-cyan-500/10 border border-cyan-500/20 text-cyan-400 text-xs px-3.5 py-1.5 rounded-full font-semibold">
          <span className="w-2 h-2 rounded-full bg-cyan-400 animate-pulse" />
          Sabit kanat uçuş kontrol yazılımı
        </div>
        <h1 className="text-4xl md:text-6xl font-extrabold tracking-tight leading-tight">
          Küçük Kart <br />
          <span className="text-cyan-400">Ciddi Mimari</span>
        </h1>
        <p className="text-slate-400 text-base md:text-lg leading-relaxed">
          RP2350 / Raspberry Pi Pico 2 için FreeRTOS, PIO, DMA ve MAVLink temelli okunabilir uçuş kontrol yazılımı. Manuel ve stabilize altyapı;
          masaüstü, HIL ve mühendislik testleri için hazır.
        </p>
        <div className="flex flex-col sm:flex-row gap-4 pt-2">
          <button
            type="button"
            onClick={openReleases}
            className="bg-cyan-500 hover:bg-cyan-400 active:scale-95 text-slate-950 font-bold px-6 py-3.5 rounded-xl text-center transition-all duration-150 shadow-lg shadow-cyan-500/20 cursor-pointer"
          >
            İndir
          </button>
          <button
            type="button"
            onClick={openSpecs}
            className="border border-slate-800 hover:border-cyan-500/50 bg-slate-900/50 hover:bg-slate-900 active:scale-95 text-slate-300 font-semibold px-6 py-3.5 rounded-xl text-center transition-all duration-150 cursor-pointer flex items-center justify-center gap-2"
          >
            <span>Specifications</span>
            <span className="text-cyan-400 font-mono text-xs">→</span>
          </button>
        </div>
        <p className="text-xs text-amber-400/90 font-medium bg-amber-500/10 border border-amber-500/20 px-3 py-1.5 rounded-lg inline-block">
          Uçuş öncesi masaüstü doğrulaması zorunludur.
        </p>
      </div>

      <div className="glass-card rounded-3xl p-6 glow-effect relative overflow-hidden min-h-[400px] flex flex-col justify-between z-10">
        <div className="absolute inset-0 bg-gradient-to-br from-cyan-500/5 to-transparent pointer-events-none" />
        <div className="flex items-center justify-between pb-4 border-b border-slate-800/80">
          <div className="flex items-center gap-2">
            <div className="w-3 h-3 rounded-full bg-red-500/85" />
            <div className="w-3 h-3 rounded-full bg-yellow-500/85" />
            <div className="w-3 h-3 rounded-full bg-green-500/85" />
          </div>
          <div className="flex items-center gap-2">
            <span className="w-2 h-2 rounded-full bg-emerald-400 animate-ping" />
            <span className="text-xs font-mono text-cyan-400 bg-cyan-500/10 px-2.5 py-1 rounded-md border border-cyan-500/20">
              MAVLink Stream: Active
            </span>
          </div>
        </div>

        <div className="flex-grow flex flex-col justify-center p-4 space-y-3 my-4 border border-slate-800/80 rounded-2xl bg-slate-950/60 font-mono text-xs">
          {telemetryRows.map((row, index) => (
            <div
              className={`flex justify-between items-center text-slate-400 ${index < telemetryRows.length - 1 ? "border-b border-slate-900 pb-2" : ""}`}
              key={row.label}
            >
              <span>{row.label}</span>
              <span className={`${row.tone} font-bold`}>{row.value}</span>
            </div>
          ))}
        </div>

        <div className="grid grid-cols-2 gap-3 pt-2 border-t border-slate-800/80">
          {heroStats.map((stat) => (
            <div className="bg-slate-900/80 p-3 rounded-xl border border-slate-800 text-center hover:border-cyan-500/30 transition" key={stat.label}>
              <div className="text-[10px] text-slate-400 font-mono">{stat.label}</div>
              <div className={`text-lg font-extrabold ${stat.tone} mt-0.5`}>{stat.value}</div>
            </div>
          ))}
        </div>
      </div>
    </main>
  );
}