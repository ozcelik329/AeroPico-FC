import { useEffect, useState } from "react";
import { useLanguage } from "../context/LanguageContext.jsx";
import { repoUrl } from "../data/siteData.js";
import { getLiveReleaseDownloads, getReleaseDownloads } from "../services/releaseService.js";
import { cn } from "../utils/cn.js";
import { externalLinkProps } from "../utils/externalLinkProps.js";

const badgeClasses = {
  cyan: "bg-cyan-500/10 text-cyan-400 border-cyan-500/20",
  blue: "bg-blue-500/10 text-blue-400 border-blue-500/20",
  emerald: "bg-emerald-500/10 text-emerald-400 border-emerald-500/20",
  amber: "bg-amber-500/10 text-amber-400 border-amber-500/20",
  rose: "bg-rose-500/10 text-rose-400 border-rose-500/20",
};

export default function ReleaseList() {
  const { content, language } = useLanguage();
  const fallbackReleases = content.release.downloads;
  const [releases, setReleases] = useState(() => getReleaseDownloads(fallbackReleases));
  const [status, setStatus] = useState("idle");

  useEffect(() => {
    let cancelled = false;
    setReleases(getReleaseDownloads(fallbackReleases));
    setStatus("loading");

    getLiveReleaseDownloads(language)
      .then((items) => {
        if (!cancelled && items.length) {
          setReleases(items);
          setStatus("live");
        }
      })
      .catch(() => {
        if (!cancelled) {
          setReleases(getReleaseDownloads(fallbackReleases));
          setStatus("fallback");
        }
      });

    return () => {
      cancelled = true;
    };
  }, [fallbackReleases, language]);

  return (
    <section className="site-container py-16 border-t border-slate-900/80 relative z-10">
      <div className="mb-10 flex flex-col sm:flex-row sm:items-end justify-between gap-4">
        <div className="max-w-3xl">
          <div className="text-xs uppercase font-mono text-cyan-400 font-semibold tracking-wider bg-cyan-500/10 border border-cyan-500/20 px-3 py-1 rounded-full inline-block">
            {content.modals.releasesTitle}
          </div>
          <h2 className="text-3xl md:text-5xl font-extrabold text-white mt-3">{content.downloadsPage.releasesHeading}</h2>
          <p className="text-slate-400 text-sm md:text-base mt-4 leading-relaxed">{content.modals.releasesDescription}</p>
        </div>
        <a href={repoUrl} className="text-cyan-400 hover:underline text-sm font-semibold" {...externalLinkProps}>
          {content.modals.viewGithub}
        </a>
      </div>

      {status === "loading" ? <div className="mb-4 rounded-xl border border-slate-800 bg-slate-900/70 p-4 text-xs text-slate-400">{content.modals.loading}</div> : null}
      {status === "fallback" ? <div className="mb-4 rounded-xl border border-amber-500/20 bg-amber-500/10 p-4 text-xs text-amber-200">{content.modals.fallback}</div> : null}

      <div className="space-y-4">
        {releases.map((release) => (
          <article
            className="bg-slate-900/70 border border-slate-800 p-5 rounded-2xl flex flex-col sm:flex-row justify-between items-start sm:items-center gap-4 hover:border-cyan-500/40 transition"
            key={release.title}
          >
            <div className="space-y-1">
              <div className="flex flex-wrap items-center gap-2">
                <span className="text-white font-extrabold text-base">{release.title}</span>
                <span className={cn("text-xs px-2.5 py-0.5 rounded-full font-semibold border", badgeClasses[release.badgeTone])}>{release.badge}</span>
              </div>
              <div className="text-[11px] font-mono text-slate-500">{release.version} · {release.published}</div>
              <p className="text-slate-400 text-xs leading-relaxed">{release.text}</p>
            </div>
            <div className="flex items-center gap-2 w-full sm:w-auto">
              <a
                href={release.href}
                className="bg-cyan-500 hover:bg-cyan-400 active:scale-95 text-slate-950 font-bold text-xs px-4 py-2.5 rounded-xl transition flex-1 sm:flex-initial text-center shadow-md shadow-cyan-500/20"
                {...externalLinkProps}
              >
                {content.header.releases}
              </a>
              <a
                href={release.downloadHref}
                className="bg-slate-800 hover:bg-slate-700 active:scale-95 text-slate-200 font-semibold text-xs px-4 py-2.5 rounded-xl transition flex-1 sm:flex-initial text-center border border-slate-700"
                {...externalLinkProps}
              >
                ZIP
              </a>
            </div>
          </article>
        ))}
      </div>
    </section>
  );
}
