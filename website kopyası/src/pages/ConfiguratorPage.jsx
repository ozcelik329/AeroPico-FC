import configuratorPreview from "../assets/aeropico-configurator-dark.webp";
import PageShell from "../components/layout/PageShell.jsx";
import PlatformDownloadButtons from "../components/ui/PlatformDownloadButtons.jsx";
import { useLanguage } from "../context/LanguageContext.jsx";
import { repoUrl } from "../data/siteData.js";
import { externalLinkProps } from "../utils/externalLinkProps.js";

function FlowChart({ items }) {
  return (
    <div className="config-flow" aria-label={items.label}>
      <div className="config-flow-node config-flow-source">
        <span>{items.source.kicker}</span>
        <strong>{items.source.title}</strong>
        <small>{items.source.meta}</small>
      </div>
      <div className="config-flow-link">
        <span>{items.transport}</span>
      </div>
      <div className="config-flow-node config-flow-target">
        <span>{items.target.kicker}</span>
        <strong>{items.target.title}</strong>
        <small>{items.target.meta}</small>
      </div>
      <div className="config-flow-branches">
        {items.branches.map((branch) => (
          <div className="config-flow-branch" key={branch}>
            {branch}
          </div>
        ))}
      </div>
    </div>
  );
}

export default function ConfiguratorPage() {
  const { content } = useLanguage();
  const page = content.configuratorPage;

  return (
    <PageShell>
      <section className="site-container configurator-page-hero py-14 md:py-20 relative z-10">
        <div className="grid lg:grid-cols-[0.86fr_1.14fr] gap-10 items-center">
          <div className="space-y-6">
            <div className="text-xs font-mono text-cyan-400 uppercase tracking-wider bg-cyan-500/10 border border-cyan-500/20 px-3.5 py-1.5 rounded-full inline-block font-semibold">
              {page.eyebrow}
            </div>
            <div>
              <h1 className="text-4xl md:text-6xl font-extrabold text-white leading-tight">{page.title}</h1>
              <p className="text-slate-400 text-base md:text-lg leading-relaxed mt-5">{page.lead}</p>
            </div>
            <div className="grid sm:grid-cols-2 gap-3">
              {page.summary.map((item) => (
                <div className="config-mini-row" key={typeof item === "string" ? item : item.title}>
                  <span />
                  <div>
                    <strong>{typeof item === "string" ? item : item.title}</strong>
                    {typeof item === "string" ? null : <p>{item.text}</p>}
                  </div>
                </div>
              ))}
            </div>
            <div className="flex flex-col sm:flex-row gap-3">
              <a
                href={`${repoUrl}/tree/main/tools/aeropico-configurator`}
                className="bg-cyan-500 hover:bg-cyan-400 active:scale-95 text-slate-950 font-bold px-5 py-3 rounded-xl transition text-sm text-center"
                {...externalLinkProps}
              >
                {page.sourceAction}
              </a>
              <a
                href="/releases"
                className="border border-slate-700 bg-slate-900/70 hover:border-cyan-500/35 text-slate-200 font-bold px-5 py-3 rounded-xl transition text-sm text-center"
              >
                {page.releaseAction}
              </a>
            </div>
          </div>

          <div className="config-preview-shell">
            <div className="config-video-slot">
              <span>{page.videoLabel}</span>
              <strong>{page.videoTitle}</strong>
              <small>{page.videoNote}</small>
            </div>
            <img
              src={configuratorPreview}
              alt={page.previewAlt}
              loading="eager"
              decoding="async"
              className="config-preview-image"
            />
          </div>
        </div>
      </section>

      <section className="site-container py-14 border-t border-slate-900/80 relative z-10">
        <div className="grid lg:grid-cols-[0.78fr_1.22fr] gap-8 items-start">
          <div className="space-y-4">
            <div className="text-xs font-mono text-cyan-400 uppercase tracking-wider">{page.flow.eyebrow}</div>
            <h2 className="text-3xl md:text-4xl font-extrabold text-white">{page.flow.title}</h2>
            <p className="text-slate-400 text-sm leading-relaxed">{page.flow.text}</p>
          </div>
          <FlowChart items={page.flow.chart} />
        </div>
      </section>

      <section className="site-container py-14 border-t border-slate-900/80 relative z-10">
        <div className="mb-8 max-w-3xl">
          <h2 className="text-3xl md:text-4xl font-extrabold text-white">{page.experience.title}</h2>
        </div>
        <div className="config-feature-list">
          {page.experience.items.map((item) => (
            <article className="config-feature-card" key={item.title}>
              <h3>{item.title}</h3>
              <p>{item.text}</p>
            </article>
          ))}
        </div>
      </section>

      <section className="site-container py-14 border-t border-slate-900/80 relative z-10">
        <div className="config-list-strip">
          {page.groups.map((group) => (
            <article className="config-list-panel" key={group.title}>
              <h2>{group.title}</h2>
              {group.text ? (
                <p>{group.text}</p>
              ) : (
                <ul>
                  {group.items.map((item) => (
                    <li key={item}>{item}</li>
                  ))}
                </ul>
              )}
            </article>
          ))}
        </div>
      </section>

      <section className="site-container py-14 border-t border-slate-900/80 relative z-10">
        <div className="grid lg:grid-cols-2 gap-5">
          <article className="config-text-panel">
            <h2>{page.security.title}</h2>
            <p>{page.security.text}</p>
            <ul>
              {page.security.items.map((item) => (
                <li key={item}>{item}</li>
              ))}
            </ul>
          </article>
          <article className="config-text-panel">
            <h2>{page.purpose.title}</h2>
            <p>{page.purpose.text}</p>
            <ul>
              {page.purpose.items.map((item) => (
                <li key={item}>{item}</li>
              ))}
            </ul>
          </article>
        </div>
      </section>

      <section className="site-container py-14 border-t border-slate-900/80 relative z-10">
        <div className="config-download-panel">
          <div>
            <h2>{page.downloads.title}</h2>
          </div>
          <PlatformDownloadButtons items={page.downloads.items} />
        </div>
      </section>
    </PageShell>
  );
}
