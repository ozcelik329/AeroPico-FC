import configuratorPreview from "../assets/aeropico-configurator-dark.webp";
import { useLanguage } from "../context/LanguageContext.jsx";
import { repoUrl } from "../data/siteData.js";
import { externalLinkProps } from "../utils/externalLinkProps.js";

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

        <div className="grid sm:grid-cols-2 gap-3 pt-2">
          {content.configurator.checks.map((check) => (
            <div className="bg-slate-900/80 border border-slate-800 hover:border-cyan-500/40 p-4 rounded-xl flex items-center gap-3 transition" key={check}>
              <span className="bg-emerald-500/10 text-emerald-400 font-mono text-xs px-2 py-1 rounded font-bold border border-emerald-500/20">OK</span>
              <span className="text-slate-200 text-xs font-semibold">{check}</span>
            </div>
          ))}
        </div>

        <div className="pt-2">
          <a
            href={repoUrl}
            className="border border-cyan-500/35 bg-cyan-500/10 hover:bg-cyan-500/15 active:scale-95 text-cyan-200 font-bold px-6 py-3.5 rounded-xl transition-all duration-150 inline-flex items-center gap-2 text-sm"
            {...externalLinkProps}
          >
            {content.configurator.action}
          </a>
        </div>
      </div>

      <div className="glass-card rounded-3xl p-3 shadow-2xl overflow-hidden group border-cyan-500/20">
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
