import { useLanguage } from "../context/LanguageContext.jsx";
import { cn } from "../utils/cn.js";
import PageShell from "../components/layout/PageShell.jsx";

export default function FeaturesPage() {
  const { content } = useLanguage();
  const specifications = content.specs;

  return (
    <PageShell>
      <section className="site-container py-16 md:py-20 relative z-10">
        <div className="mb-10 max-w-3xl">
          <div className="text-xs uppercase font-mono text-cyan-400 font-semibold tracking-wider bg-cyan-500/10 border border-cyan-500/20 px-3 py-1 rounded-full inline-block">
            {content.modals.specsTitle}
          </div>
          <h1 className="text-3xl md:text-5xl font-extrabold text-white mt-3">{content.modals.stackTitle}</h1>
          <p className="text-slate-400 text-sm md:text-base mt-4 leading-relaxed">{content.modals.specsDescription}</p>
        </div>

        <div className="mb-6 bg-gradient-to-r from-cyan-500/10 via-blue-500/10 to-transparent border border-cyan-500/20 p-5 rounded-2xl flex items-center justify-between">
          <div>
            <div className="text-[10px] font-mono text-cyan-400 uppercase tracking-widest">{content.modals.stack}</div>
            <div className="text-xl font-extrabold text-white mt-1">RP2350 / Pico 2</div>
          </div>
          <div className="text-right hidden sm:block">
            <div className="text-[10px] font-mono text-slate-400">TARGET</div>
            <div className="text-sm font-bold text-cyan-400">FreeRTOS + MAVLink</div>
          </div>
        </div>

        <div className="grid md:grid-cols-2 lg:grid-cols-3 gap-4">
          {specifications.map((spec) => (
            <article
              className={cn("bg-slate-900/60 border border-slate-800 p-4 rounded-xl space-y-1 hover:border-cyan-500/30 transition", spec.wide && "md:col-span-2 lg:col-span-3")}
              key={`${spec.label}-${spec.value}`}
            >
              <div className="text-[10px] font-mono text-cyan-400">{spec.label}</div>
              <h2 className="font-bold text-white text-sm">{spec.value}</h2>
              <p className="text-slate-400 text-xs leading-relaxed">{spec.text}</p>
            </article>
          ))}
        </div>
      </section>
    </PageShell>
  );
}
