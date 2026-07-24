import { releaseChecks } from "../data/siteData.js";
import { useLanguage } from "../context/LanguageContext.jsx";

export default function ReleaseGate() {
  const { content } = useLanguage();

  return (
    <section id="release" className="site-container reveal-section py-16 border-t border-slate-900/80 relative z-10">
      <div className="mb-8 flex flex-col sm:flex-row sm:items-center justify-between gap-4">
        <div>
          <div className="text-xs uppercase font-mono text-cyan-400 font-semibold tracking-wider bg-cyan-500/10 border border-cyan-500/20 px-3 py-1 rounded-full inline-block">
            {content.release.eyebrow}
          </div>
          <h3 className="text-2xl md:text-3xl font-extrabold text-white mt-2">{content.release.title}</h3>
        </div>
        <div className="bg-slate-900/80 border border-slate-800 px-4 py-2 rounded-xl text-xs font-mono text-slate-300">
          GATE: <span className="text-cyan-400 font-bold">{content.release.gate}</span>
        </div>
      </div>

      <div className="grid lg:grid-cols-12 gap-6 items-start">
        <div className="lg:col-span-4 flat-card p-6 rounded-2xl space-y-3">
          <h4 className="text-amber-400 font-bold text-lg font-mono">v1.0.0-rc1</h4>
          <p className="text-slate-400 text-sm leading-relaxed">
            Gerçek uçuş veya ticari kullanımdan önce bench doğrulama, HIL kanıtı, airframe tuning ve saha test kayıtları tamamlanmalıdır.
          </p>
        </div>

        <div className="lg:col-span-8 flat-card p-6 rounded-2xl grid sm:grid-cols-2 gap-4">
          {releaseChecks.map((check) => (
            <div className="bg-slate-950/60 border border-slate-800/80 p-4 rounded-xl flex items-center gap-3" key={check}>
              <span className="bg-emerald-500/10 text-emerald-400 font-mono text-xs px-2.5 py-1 rounded font-bold border border-emerald-500/20">
                PASS
              </span>
              <span className="text-slate-200 text-sm font-medium">{check}</span>
            </div>
          ))}
        </div>
      </div>
    </section>
  );
}
