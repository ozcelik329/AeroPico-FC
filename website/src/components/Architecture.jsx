import { architectureCards } from "../data/siteData.js";
import SectionHeading from "./ui/SectionHeading.jsx";

const toneClasses = {
  amber: "bg-amber-500/10 border-amber-500/20 text-amber-300 group-hover:border-amber-500/40",
  blue: "bg-blue-500/10 border-blue-500/20 text-blue-300 group-hover:border-blue-500/40",
  cyan: "bg-cyan-500/10 border-cyan-500/20 text-cyan-300 group-hover:border-cyan-500/40",
  emerald: "bg-emerald-500/10 border-emerald-500/20 text-emerald-300 group-hover:border-emerald-500/40",
  rose: "bg-rose-500/10 border-rose-500/20 text-rose-300 group-hover:border-rose-500/40",
  violet: "bg-violet-500/10 border-violet-500/20 text-violet-300 group-hover:border-violet-500/40",
};

export default function Architecture() {
  return (
    <section id="architecture" className="site-container py-16 border-t border-slate-900/80 relative z-10">
      <SectionHeading
        eyebrow="SİSTEM ÖZELLİKLERİ"
        title="Çekirdek Tasarım ve Güvenceler"
        description="Gereksiz karmaşadan arındırılmış, deterministik ve modüler uçuş yazılımı mimarisi."
      />

      <div className="grid md:grid-cols-2 lg:grid-cols-3 gap-6">
        {architectureCards.map((card, index) => (
          <article className="glass-card p-6 rounded-2xl space-y-3 border border-slate-800 hover:-translate-y-0.5 transition group" key={card.title}>
            <div className={`w-10 h-10 rounded-xl border flex items-center justify-center font-bold text-sm ${toneClasses[card.tone]}`}>
              {String(index + 1).padStart(2, "0")}
            </div>
            <h3 className="text-base font-bold text-white">{card.title}</h3>
            <p className="text-slate-400 text-xs leading-relaxed">{card.text}</p>
          </article>
        ))}
      </div>
    </section>
  );
}
