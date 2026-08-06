import { useLanguage } from "../context/LanguageContext.jsx";
import SectionHeading from "./ui/SectionHeading.jsx";

export default function Architecture() {
  const { content } = useLanguage();

  return (
    <section id="architecture" className="site-container reveal-section py-16 border-t border-slate-900/80 relative z-10">
      <SectionHeading
        eyebrow={content.architecture.eyebrow}
        title={content.architecture.title}
        description={content.architecture.description}
      />

      <div className="grid md:grid-cols-2 lg:grid-cols-3 gap-6">
        {content.architecture.cards.map((card, index) => (
          <article className="flat-card p-6 rounded-2xl space-y-3 hover:-translate-y-0.5 transition group" key={card.title}>
            <div className="w-10 h-10 rounded-xl border border-cyan-500/20 bg-cyan-500/10 text-cyan-300 flex items-center justify-center font-bold text-sm group-hover:border-cyan-500/45 transition">
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
