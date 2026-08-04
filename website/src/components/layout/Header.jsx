import { useState } from "react";
import logo from "../../assets/aeropico-logo-full.webp";
import { useLanguage } from "../../context/LanguageContext.jsx";
import { repoUrl } from "../../data/siteData.js";
import { useModal } from "../../hooks/useModal.js";
import { externalLinkProps } from "../../utils/externalLinkProps.js";

export default function Header() {
  const { content, toggleLanguage } = useLanguage();
  const { openSpecs } = useModal();
  const [menuOpen, setMenuOpen] = useState(false);
  const navItems = content.navItems;

  const closeMenu = () => setMenuOpen(false);

  return (
    <header className="header-shell bg-slate-950/35 border border-white/10 shadow-2xl shadow-black/35 backdrop-blur-2xl rounded-3xl">
      <a className="header-brand" href="/#top" aria-label={content.header.homeLabel}>
        <span className="rounded-2xl bg-slate-900/45 border border-cyan-400/20 px-3 py-2 shadow-lg shadow-cyan-500/10">
          <img src={logo} alt="AeroPico Flight Control Software" className="header-logo object-contain drop-shadow-[0_0_12px_rgba(34,211,238,0.20)]" />
        </span>
        <span className="header-mobile-title">AeroPico-FC</span>
      </a>

      <nav className="header-nav" aria-label={content.header.navLabel}>
        <button
          type="button"
          onClick={openSpecs}
          className="header-nav-item hover:text-cyan-400 transition cursor-pointer font-semibold text-cyan-300 bg-cyan-500/10 border border-cyan-500/20 rounded-xl shadow-sm flex items-center gap-2 group"
        >
          <span className="w-2 h-2 rounded-full bg-cyan-400 animate-pulse" />
          {content.header.features}
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
        <button
          type="button"
          onClick={toggleLanguage}
          className="border border-slate-700 bg-slate-900/70 hover:border-cyan-500/35 text-slate-300 font-bold px-3 py-2 rounded-xl transition text-xs cursor-pointer"
          aria-label={content.header.languageToggle}
        >
          {content.languageLabel}
        </button>
        <a
          href={repoUrl}
          className="bg-slate-900/75 border border-slate-800 hover:border-cyan-500/40 text-slate-300 font-semibold px-4 py-2 rounded-xl transition text-sm flex items-center gap-2 active:scale-95 duration-150"
          {...externalLinkProps}
        >
          {content.header.github}
        </a>
        <a
          href="/releases"
          className="border border-cyan-500/40 bg-cyan-500/10 hover:bg-cyan-500/15 active:scale-95 text-cyan-200 font-bold px-4 py-2 rounded-xl transition-all duration-150 text-sm flex items-center gap-2 cursor-pointer"
        >
          {content.header.releases}
        </a>
        <button
          type="button"
          onClick={() => setMenuOpen((value) => !value)}
          className="mobile-menu-button border border-slate-700 bg-slate-900/80 text-slate-200 font-bold px-3 py-2 rounded-xl transition text-xs cursor-pointer"
          aria-expanded={menuOpen}
          aria-controls="mobile-menu"
        >
          {content.header.menu}
        </button>
      </div>

      <div id="mobile-menu" className={`mobile-menu-panel ${menuOpen ? "is-open" : ""}`}>
        <button type="button" onClick={() => { openSpecs(); closeMenu(); }} className="mobile-menu-item text-cyan-300">
          {content.header.features}
        </button>
        {navItems.map((item) => (
          <a href={item.href} onClick={closeMenu} className="mobile-menu-item" key={`mobile-${item.href}`}>
            {item.label}
          </a>
        ))}
      </div>
    </header>
  );
}
