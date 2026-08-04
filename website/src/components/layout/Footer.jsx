import { repoUrl } from "../../data/siteData.js";
import { useLanguage } from "../../context/LanguageContext.jsx";
import { externalLinkProps } from "../../utils/externalLinkProps.js";

export default function Footer() {
  const { content } = useLanguage();

  return (
    <footer id="contact" className="site-container py-10 border-t border-slate-900/80 relative z-10 scroll-mt-28">
      <div className="grid gap-8 md:grid-cols-[1fr_auto] md:items-center text-sm text-slate-500">
        <div className="space-y-4">
          <p className="text-lg font-extrabold text-white">Muhammed Fatih Emre Özçelik</p>
          <div className="max-w-xl rounded-2xl border border-cyan-400/25 bg-cyan-500/10 px-5 py-4 shadow-lg shadow-cyan-500/10">
            <h2 className="text-sm font-extrabold uppercase tracking-wider text-cyan-300">{content.footer.contact}</h2>
            <p className="mt-2 text-slate-300">
              {content.footer.contactText}{" "}
              <a className="font-bold text-white underline decoration-cyan-400/50 underline-offset-4 hover:text-cyan-200 transition" href="mailto:fatihemreozcelik@gmail.com">
                fatihemreozcelik@gmail.com
              </a>
            </p>
          </div>
        </div>
        <div className="flex gap-6 font-medium">
          <a href={repoUrl} className="hover:text-cyan-400 transition" {...externalLinkProps}>
            GitHub
          </a>
          <a href="/downloads" className="hover:text-cyan-400 transition">
            {content.footer.releasePolicy}
          </a>
        </div>
      </div>
      <p className="mt-8 border-t border-slate-900/80 pt-6 text-xs text-slate-600">
        {content.footer.copyright}
      </p>
    </footer>
  );
}
