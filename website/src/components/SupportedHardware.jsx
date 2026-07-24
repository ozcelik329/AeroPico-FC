import { useLanguage } from "../context/LanguageContext.jsx";
import SectionHeading from "./ui/SectionHeading.jsx";

export default function SupportedHardware() {
  const { content } = useLanguage();
  const { hardware } = content;

  return (
    <section id="hardware" className="site-container reveal-section py-16 md:py-20 relative z-10">
      <SectionHeading eyebrow={hardware.eyebrow} title={hardware.title} description={hardware.description} />

      <div className="hardware-list overflow-hidden rounded-3xl border border-slate-800/90 bg-slate-950/55 shadow-2xl shadow-cyan-950/20">
        <div className="hidden md:grid grid-cols-[1.1fr_1.1fr_0.7fr_1.7fr] gap-4 border-b border-slate-800/90 bg-slate-900/70 px-5 py-3 text-[11px] font-mono uppercase tracking-wider text-slate-500">
          <span>{hardware.columns.role}</span>
          <span>{hardware.columns.component}</span>
          <span>{hardware.columns.status}</span>
          <span>{hardware.columns.note}</span>
        </div>
        {hardware.items.map((item) => (
          <article
            className="grid gap-3 border-b border-slate-800/70 px-5 py-4 last:border-b-0 md:grid-cols-[1.1fr_1.1fr_0.7fr_1.7fr] md:items-center md:gap-4 hover:bg-slate-900/45 transition"
            key={`${item.role}-${item.name}`}
          >
            <div>
              <div className="md:hidden text-[10px] font-mono uppercase tracking-wider text-slate-500">{hardware.columns.role}</div>
              <div className="text-sm font-bold text-cyan-300">{item.role}</div>
            </div>
            <div>
              <div className="md:hidden text-[10px] font-mono uppercase tracking-wider text-slate-500">{hardware.columns.component}</div>
              <h3 className="text-base font-extrabold text-white">{item.name}</h3>
            </div>
            <div>
              <div className="md:hidden text-[10px] font-mono uppercase tracking-wider text-slate-500">{hardware.columns.status}</div>
              <span className="inline-flex rounded-full border border-emerald-500/25 bg-emerald-500/10 px-2.5 py-1 text-[10px] font-bold text-emerald-300">
                {item.status}
              </span>
            </div>
            <p className="text-sm leading-relaxed text-slate-400">
              <span className="md:hidden block text-[10px] font-mono uppercase tracking-wider text-slate-500">{hardware.columns.note}</span>
              {item.note}
            </p>
          </article>
        ))}
      </div>
    </section>
  );
}
