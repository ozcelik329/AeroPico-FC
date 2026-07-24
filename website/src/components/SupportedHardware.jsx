import { useLanguage } from "../context/LanguageContext.jsx";
import SectionHeading from "./ui/SectionHeading.jsx";

export default function SupportedHardware() {
  const { content } = useLanguage();
  const { hardware } = content;

  return (
    <section id="hardware" className="site-container reveal-section py-16 border-t border-slate-900/80 relative z-10">
      <SectionHeading eyebrow={hardware.eyebrow} title={hardware.title} description={hardware.description} />

      <div className="grid md:grid-cols-2 lg:grid-cols-3 gap-5">
        {hardware.items.map((item) => (
          <article className="flat-card rounded-2xl p-5 space-y-3" key={`${item.role}-${item.name}`}>
            <div className="flex items-start justify-between gap-4">
              <div>
                <div className="text-[10px] font-mono uppercase tracking-wider text-cyan-400">{item.role}</div>
                <h3 className="mt-1 text-base font-extrabold text-white">{item.name}</h3>
              </div>
              <span className="rounded-full border border-emerald-500/25 bg-emerald-500/10 px-2.5 py-1 text-[10px] font-bold text-emerald-300">
                {item.status}
              </span>
            </div>
            <p className="text-xs leading-relaxed text-slate-400">{item.note}</p>
          </article>
        ))}
      </div>
    </section>
  );
}
