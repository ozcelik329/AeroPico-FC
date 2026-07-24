import logo from "../../assets/aeropico-logo-full.png";
import { navItems, repoUrl } from "../../data/siteData.js";
import { useModal } from "../../hooks/useModal.js";
import { externalLinkProps } from "../../utils/externalLinkProps.js";

export default function Header() {
  const { openReleases, openSpecs } = useModal();

  return (
    <header className="site-container header-shell fixed left-1/2 top-3 md:top-4 -translate-x-1/2 bg-slate-950/35 border border-white/10 shadow-2xl shadow-black/35 backdrop-blur-2xl rounded-3xl z-50">
      <a className="header-brand" href="#top" aria-label="AeroPico FC ana sayfa">
        <span className="rounded-2xl bg-slate-900/45 border border-cyan-400/20 px-3 py-2 shadow-lg shadow-cyan-500/10">
          <img src={logo} alt="AeroPico Flight Control Software" className="header-logo object-contain drop-shadow-[0_0_12px_rgba(34,211,238,0.20)]" />
        </span>
        <span className="header-mobile-title">AeroPico-FC</span>
      </a>

      <nav className="header-nav" aria-label="Ana menü">
        <button
          type="button"
          onClick={openSpecs}
          className="header-nav-item hover:text-cyan-400 transition cursor-pointer font-semibold text-cyan-300 bg-cyan-500/10 border border-cyan-500/20 rounded-xl shadow-sm flex items-center gap-2 group"
        >
          <span className="w-2 h-2 rounded-full bg-cyan-400 animate-pulse" />
          Özellikler
        </button>
        {navItems.map((item) => (
          <a
            className={`header-nav-link ${item.highlighted ? "text-cyan-400 font-semibold" : ""}`}
            href={item.href}
            key={item.href}
          >
            {item.label}
          </a>
        ))}
      </nav>

      <div className="header-actions">
        <a
          href={repoUrl}
          className="bg-slate-900/90 border border-slate-800 hover:border-cyan-500/50 text-slate-200 font-semibold px-4 py-2 rounded-xl transition text-sm flex items-center gap-2 active:scale-95 duration-150"
          {...externalLinkProps}
        >
          GitHub
        </a>
        <button
          type="button"
          onClick={openReleases}
          className="bg-cyan-500 hover:bg-cyan-400 active:scale-95 text-slate-950 font-bold px-4 py-2 rounded-xl transition-all duration-150 text-sm shadow-lg shadow-cyan-500/20 flex items-center gap-2 cursor-pointer"
        >
          Sürümler
        </button>
      </div>
    </header>
  );
}
