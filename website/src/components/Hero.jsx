import { motion } from "motion/react";
import logo from "../assets/aeropico-logo-hero.webp";
import { useLanguage } from "../context/LanguageContext.jsx";
import { heroStats, telemetryRows } from "../data/siteData.js";
import { useModal } from "../hooks/useModal.js";
import { pressFeedback, spring } from "../utils/motionPresets.js";

function TelemetryPanel() {
  return (
    <motion.div
      className="telemetry-panel glass-card rounded-3xl p-6 glow-effect relative overflow-hidden min-h-[400px] flex flex-col justify-between z-10"
      initial={{ opacity: 0, scale: 0.97, y: 14 }}
      animate={{ opacity: 1, scale: 1, y: 0 }}
      transition={spring({ type: "spring", bounce: 0, duration: 0.5, delay: 0.15 })}
    >
      <div className="absolute inset-0 bg-gradient-to-br from-cyan-500/5 via-transparent to-emerald-500/5 pointer-events-none" />
      <div className="absolute top-4 right-4 w-10 h-10 rounded-full border border-cyan-400/25 flex items-center justify-center">
        <span className="absolute inset-0 rounded-full border border-cyan-300/40 animate-ping" />
        <span className="relative w-2.5 h-2.5 rounded-full bg-cyan-300 shadow-[0_0_12px_rgba(56,189,248,0.35)]" />
      </div>

      <div className="telemetry-header flex items-center justify-between pb-4 border-b border-slate-800/80">
        <div className="flex items-center gap-2">
          <div className="w-3 h-3 rounded-full bg-red-500/85" />
          <div className="w-3 h-3 rounded-full bg-yellow-500/85" />
          <div className="w-3 h-3 rounded-full bg-green-500/85" />
        </div>
        <div className="flex items-center gap-2">
          <span className="text-xs font-mono text-cyan-400 bg-cyan-500/10 px-2.5 py-1 rounded-md border border-cyan-500/20">
            MAVLink Stream: Active
          </span>
        </div>
      </div>

      <div className="telemetry-list flex-grow flex flex-col justify-center p-4 space-y-3 my-4 border border-slate-800/80 rounded-2xl bg-slate-950/60 font-mono text-xs">
        {telemetryRows.map((row, index) => (
          <div
            className={`telemetry-row flex justify-between items-center text-slate-400 ${index < telemetryRows.length - 1 ? "border-b border-slate-900 pb-2" : ""}`}
            key={row.label}
          >
            <span>{row.label}</span>
            <span className={`${row.tone} font-bold`}>{row.value}</span>
          </div>
        ))}
      </div>

      <div className="telemetry-stats grid grid-cols-2 gap-3 pt-2 border-t border-slate-800/80">
        {heroStats.map((stat) => (
          <div className="bg-slate-900/80 p-3 rounded-xl border border-slate-800 text-center hover:border-cyan-500/30 transition" key={stat.label}>
            <div className="text-[10px] text-slate-400 font-mono">{stat.label}</div>
            <div className={`text-lg font-extrabold ${stat.tone} mt-0.5`}>{stat.value}</div>
          </div>
        ))}
      </div>
    </motion.div>
  );
}

// Small stagger container so the eyebrow, title, copy, and CTAs settle in
// sequence rather than all landing on the same frame — hierarchy carried by
// motion, not just layout.
const copyContainer = {
  hidden: {},
  show: { transition: { staggerChildren: 0.07, delayChildren: 0.05 } },
};

const copyItem = {
  hidden: { opacity: 0, y: 12 },
  show: { opacity: 1, y: 0 },
};

export default function Hero() {
  const { content } = useLanguage();
  const { openSpecs } = useModal();

  return (
    <main id="top" className="site-container py-8 sm:py-12 md:py-20 relative z-10">
      <div className="hero-logo-wrap relative mx-auto mb-8 md:mb-10 w-full max-w-3xl">
        <div className="absolute left-1/2 top-1/2 h-[116%] w-[88%] -translate-x-1/2 -translate-y-1/2 rounded-full bg-[radial-gradient(circle,rgba(56,189,248,0.18)_0%,rgba(14,165,233,0.10)_42%,transparent_72%)] blur-3xl pointer-events-none" />
        <img
          src={logo}
          alt="AeroPico"
          className="relative z-10 mx-auto w-full object-contain drop-shadow-[0_0_22px_rgba(14,165,233,0.18)]"
        />
      </div>

      <div className="grid md:grid-cols-2 gap-8 md:gap-12 items-center">
        <motion.section
          className="hero-copy space-y-4 md:space-y-6 relative z-10"
          aria-labelledby="hero-title"
          variants={copyContainer}
          initial="hidden"
          animate="show"
        >
          <motion.div
            variants={copyItem}
            transition={spring()}
            className="inline-flex items-center gap-2 bg-cyan-500/10 border border-cyan-500/20 text-cyan-400 text-xs px-3.5 py-1.5 rounded-full font-semibold"
          >
            <span className="w-2 h-2 rounded-full bg-cyan-400 animate-pulse" />
            {content.hero.eyebrow}
          </motion.div>

          <motion.h1
            variants={copyItem}
            transition={spring()}
            id="hero-title"
            className="text-3xl sm:text-4xl md:text-6xl font-extrabold tracking-tight leading-tight"
          >
            {content.hero.titleA} <br />
            <span className="text-cyan-400">{content.hero.titleB}</span>
          </motion.h1>

          <motion.p
            variants={copyItem}
            transition={spring()}
            className="text-slate-400 text-sm sm:text-base md:text-lg leading-relaxed"
          >
            {content.hero.description}
          </motion.p>

          <motion.div variants={copyItem} transition={spring()} className="flex flex-col sm:flex-row gap-4 pt-2">
            <motion.a
              href="/releases"
              className="bg-cyan-500 hover:bg-cyan-400 text-slate-950 font-bold px-6 py-3.5 rounded-xl text-center shadow-lg shadow-cyan-500/20 cursor-pointer"
              whileHover={{ scale: 1.02 }}
              {...pressFeedback}
            >
              {content.hero.primary}
            </motion.a>
            <motion.button
              type="button"
              onClick={openSpecs}
              className="border border-slate-800 hover:border-cyan-500/50 bg-slate-900/50 hover:bg-slate-900 text-slate-300 font-semibold px-6 py-3.5 rounded-xl text-center cursor-pointer flex items-center justify-center gap-2"
              whileHover={{ scale: 1.02 }}
              {...pressFeedback}
            >
              <span>{content.hero.secondary}</span>
              <span className="text-cyan-400 font-mono text-xs">→</span>
            </motion.button>
          </motion.div>

          <motion.p
            variants={copyItem}
            transition={spring()}
            className="hero-note text-xs text-amber-400/90 font-medium bg-amber-500/10 border border-amber-500/20 px-3 py-1.5 rounded-lg inline-block"
          >
            {content.hero.note}
          </motion.p>
        </motion.section>

        <TelemetryPanel />
      </div>
    </main>
  );
}
