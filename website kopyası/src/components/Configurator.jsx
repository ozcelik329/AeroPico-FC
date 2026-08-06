import { motion } from "motion/react";
import configuratorPreview from "../assets/aeropico-configurator-dark.webp";
import { useLanguage } from "../context/LanguageContext.jsx";
import { pressFeedback } from "../utils/motionPresets.js";
import PlatformDownloadButtons from "./ui/PlatformDownloadButtons.jsx";

export default function Configurator() {
  const { content } = useLanguage();

  return (
    <section
      id="configurator"
      className="site-container configurator-band reveal-section py-16 border-t border-slate-900/80 grid md:grid-cols-2 gap-12 items-center relative z-10"
    >
      <div className="space-y-6">
        <div className="text-xs font-mono text-cyan-400 uppercase tracking-wider bg-cyan-500/10 border border-cyan-500/20 px-3.5 py-1.5 rounded-full inline-block font-semibold">
          {content.configurator.eyebrow}
        </div>
        <h2 className="text-3xl md:text-4xl font-extrabold text-white leading-tight">{content.configurator.title}</h2>
        <p className="text-slate-400 text-sm leading-relaxed">
          {content.configurator.description}
        </p>

        <div className="config-check-strip pt-2">
          {content.configurator.checks.map((check) => (
            <div className="config-check-pill" key={check}>
              <span>OK</span>
              <span className="text-slate-200 text-xs font-semibold">{check}</span>
            </div>
          ))}
        </div>

        <PlatformDownloadButtons items={content.configuratorPage.downloads.items} className="configurator-inline-downloads" />

        <div className="pt-1">
          <motion.a
            href="/configurator"
            className="border border-cyan-500/35 bg-cyan-500/10 hover:bg-cyan-500/15 text-cyan-200 font-bold px-6 py-3.5 rounded-xl transition-colors inline-flex items-center gap-2 text-sm"
            {...pressFeedback}
          >
            {content.configurator.action}
          </motion.a>
        </div>
      </div>

      <div className="configurator-preview-card glass-card rounded-3xl p-3 shadow-2xl overflow-hidden group border-cyan-500/20">
        <div className="configurator-preview-label">{content.configurator.demoLabel}</div>
        <div className="rounded-2xl overflow-hidden border border-slate-800/80 bg-slate-950 aspect-[4/3] flex items-center justify-center relative">
          <img
            src={configuratorPreview}
            alt={content.configurator.previewAlt}
            loading="lazy"
            decoding="async"
            className="w-full h-full object-cover group-hover:scale-105 transition duration-500"
          />
        </div>
      </div>
    </section>
  );
}
