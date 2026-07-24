import { docCards } from "../data/siteData.js";
import { externalLinkProps } from "../utils/externalLinkProps.js";

const toneClasses = {
  amber: "bg-amber-500/10 border-amber-500/20 text-amber-300",
  cyan: "bg-cyan-500/10 border-cyan-500/20 text-cyan-300",
  emerald: "bg-emerald-500/10 border-emerald-500/20 text-emerald-300",
  red: "bg-red-500/10 border-red-500/20 text-red-300",
};

export default function Docs() {
  return (
    <section id="docs" className="site-container py-16 border-t border-slate-900/80 relative z-10">
      <div className="mb-10 flex flex-col sm:flex-row sm:items-center justify-between gap-4">
        <div>
          <div className="text-xs uppercase font-mono text-cyan-400 font-semibold tracking-wider bg-cyan-500/10 border border-cyan-500/20 px-3 py-1 rounded-full inline-block">
            DÖKÜMANTASYON & PDF
          </div>
          <h3 className="text-2xl md:text-3xl font-extrabold text-white mt-2">Teknik Raporlar ve Kullanım Kılavuzları</h3>
        </div>
      </div>

      <div className="grid md:grid-cols-3 gap-6">
        {docCards.map((doc) => (
          <article className="glass-card p-6 rounded-2xl flex flex-col justify-between space-y-4 hover:border-cyan-500/40 transition" key={doc.title}>
            <div className="space-y-3">
              <div className={`w-10 h-10 rounded-xl border flex items-center justify-center font-mono font-bold text-xs ${toneClasses[doc.tone]}`}>
                {doc.type}
              </div>
              <h4 className="text-base font-bold text-white">{doc.title}</h4>
              <p className="text-slate-400 text-xs leading-relaxed">{doc.text}</p>
            </div>
            <a
              href={doc.href}
              className="bg-slate-900 hover:bg-slate-800 text-slate-200 border border-slate-800 font-semibold text-xs px-4 py-2.5 rounded-xl transition flex items-center justify-between"
              {...externalLinkProps}
            >
              <span>{doc.actionLabel}</span>
              <span className="text-cyan-400">↓</span>
            </a>
          </article>
        ))}
      </div>
    </section>
  );
}
