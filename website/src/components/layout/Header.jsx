import { useState } from "react";
import logo from "../../assets/aeropico-logo-full.webp";
import { useLanguage } from "../../context/LanguageContext.jsx";

export default function Header() {
  const { content, toggleLanguage } = useLanguage();
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
        {navItems.map((item) => (
          <a href={item.href} onClick={closeMenu} className="mobile-menu-item" key={`mobile-${item.href}`}>
            {item.label}
          </a>
        ))}
      </div>
    </header>
  );
}
