import { useLanguage } from "../context/LanguageContext.jsx";

export default function HomeOverview() {
  const { content } = useLanguage();
  const featuredRelease = content.release.downloads[0];
  const statusItems = content.home.statusItems;

  return (
    <section className="site-container reveal-section py-16 border-t border-slate-900/80 relative z-10">
      <div className="mb-10 max-w-3xl">
        <div className="text-xs uppercase font-mono text-cyan-400 font-semibold tracking-wider bg-cyan-500/10 border border-cyan-500/20 px-3 py-1 rounded-full inline-block">
          {content.home.overviewEyebrow}
        </div>
        <h2 className="mt-4 text-3xl md:text-4xl font-extrabold text-white">{content.home.overviewTitle}</h2>
        {content.home.overviewDescription ? (
          <p className="mt-3 text-sm leading-relaxed text-slate-400">{content.home.overviewDescription}</p>
        ) : null}
      </div>

      <div className="grid gap-6 lg:grid-cols-[1.35fr_0.9fr]">
        <div className="home-route-list">
          {content.home.cards.map((card) => (
            <a
              href={card.href}
              className="home-route-item group"
              key={card.href}
            >
              <div>
                <h3 className="text-base font-extrabold text-white group-hover:text-cyan-200 transition">{card.title}</h3>
                <p className="mt-2 text-xs leading-relaxed text-slate-400">{card.text}</p>
              </div>
              <span className="mt-5 text-xs font-bold text-cyan-400">{card.action} →</span>
            </a>
          ))}
        </div>

        <div className="space-y-4">
          <div className="rounded-2xl border border-slate-800 bg-slate-900/60 p-5">
            <h3 className="text-sm font-extrabold text-white">{content.home.statusTitle}</h3>
            <div className="mt-4 space-y-3">
              {statusItems.map((item) => (
                <div className="flex items-center justify-between gap-4 border-b border-slate-800/70 pb-3 last:border-b-0 last:pb-0" key={`${item.label}-${item.value}`}>
                  <span className="text-[10px] font-mono text-slate-500">{item.label}</span>
                  <span className="text-sm font-bold text-cyan-300 text-right">{item.value}</span>
                </div>
              ))}
            </div>
          </div>

          <a href="/releases" className="block rounded-2xl border border-cyan-500/20 bg-cyan-500/10 p-5 hover:bg-cyan-500/15 transition">
            <div className="text-[10px] font-mono uppercase tracking-wider text-cyan-300">{content.home.releaseTitle}</div>
            <h3 className="mt-2 text-base font-extrabold text-white">{featuredRelease.title}</h3>
            <div className="mt-1 text-[11px] font-mono text-slate-400">{featuredRelease.version} · {featuredRelease.published}</div>
            <p className="mt-3 text-xs leading-relaxed text-slate-300">{featuredRelease.text}</p>
            <span className="mt-4 inline-block text-xs font-bold text-cyan-200">{content.home.releaseAction} →</span>
          </a>

          <div className="home-proof-strip">
            <h3>{content.home.proofTitle}</h3>
            <div>
              {content.home.proofItems.map((item) => (
                <span key={item}>{item}</span>
              ))}
            </div>
          </div>
        </div>
      </div>
    </section>
  );
}
