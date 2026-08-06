import { AnimatePresence, motion } from "motion/react";
import { useEffect, useState } from "react";
import logo from "../../assets/aeropico-logo-full.webp";
import { useLanguage } from "../../context/LanguageContext.jsx";
import { repoUrl } from "../../data/siteData.js";
import { useModal } from "../../hooks/useModal.js";
import { externalLinkProps } from "../../utils/externalLinkProps.js";
import { pressFeedback, spring, springSheet } from "../../utils/motionPresets.js";

export default function Header() {
  const { content, toggleLanguage } = useLanguage();
  const { openSpecs } = useModal();
  const [menuOpen, setMenuOpen] = useState(false);
  const [activeLocation, setActiveLocation] = useState(() => `${window.location.pathname}${window.location.hash}`);
  const navItems = content.navItems;

  const closeMenu = () => setMenuOpen(false);
  const isActive = (href) => {
    const [path, hash = ""] = href.split("#");
    const targetPath = path || "/";
    const [currentPath, currentHash = ""] = activeLocation.split("#");

    if (hash) {
      return currentPath === targetPath && currentHash === hash;
    }

    return currentPath === targetPath;
  };

  useEffect(() => {
    const updateActiveLocation = () => setActiveLocation(`${window.location.pathname}${window.location.hash}`);
    const onScroll = () => {
      if (window.location.pathname !== "/") {
        updateActiveLocation();
        return;
      }

      const sections = ["quickstart", "docs", "contact"];
      const current = sections.find((id) => {
        const element = document.getElementById(id);
        if (!element) return false;
        const box = element.getBoundingClientRect();
        return box.top <= 150 && box.bottom > 150;
      });

      if (current) {
        setActiveLocation(`/#${current}`);
      } else {
        updateActiveLocation();
      }
    };

    updateActiveLocation();
    window.addEventListener("popstate", updateActiveLocation);
    window.addEventListener("hashchange", updateActiveLocation);
    window.addEventListener("scroll", onScroll, { passive: true });

    return () => {
      window.removeEventListener("popstate", updateActiveLocation);
      window.removeEventListener("hashchange", updateActiveLocation);
      window.removeEventListener("scroll", onScroll);
    };
  }, []);

  return (
    <header className="header-shell bg-slate-950/35 border border-white/10 shadow-2xl shadow-black/35 backdrop-blur-2xl rounded-3xl">
      <div className="header-left">
        <a className="header-brand" href="/#top" aria-label={content.header.homeLabel}>
          <span className="rounded-2xl bg-slate-900/45 border border-cyan-400/20 px-3 py-2 shadow-lg shadow-cyan-500/10">
            <img src={logo} alt="AeroPico Flight Control Software" className="header-logo object-contain drop-shadow-[0_0_12px_rgba(34,211,238,0.20)]" />
          </span>
          <span className="header-mobile-title">AeroPico-FC</span>
        </a>
        <motion.button
          type="button"
          onClick={openSpecs}
          className="header-nav-item header-feature-button hover:text-cyan-400 transition cursor-pointer font-semibold text-cyan-300 bg-cyan-500/10 border border-cyan-500/20 rounded-xl shadow-sm flex items-center gap-2 group"
          {...pressFeedback}
        >
          <span className="w-2 h-2 rounded-full bg-cyan-400 animate-pulse" />
          {content.header.features}
        </motion.button>
      </div>

      <nav className="header-nav" aria-label={content.header.navLabel}>
        {navItems.map((item) => (
          <a
            className={`header-nav-link ${item.highlighted ? "font-semibold" : ""} ${isActive(item.href) ? "is-active" : ""}`}
            href={item.href}
            key={item.href}
            aria-current={isActive(item.href) ? "page" : undefined}
          >
            {item.label}
          </a>
        ))}
      </nav>

      <div className="header-actions">
        <motion.button
          type="button"
          onClick={toggleLanguage}
          className="border border-slate-700 bg-slate-900/70 hover:border-cyan-500/35 text-slate-300 font-bold px-3 py-2 rounded-xl transition text-xs cursor-pointer"
          aria-label={content.header.languageToggle}
          {...pressFeedback}
        >
          {content.languageLabel}
        </motion.button>
        <motion.a
          href={repoUrl}
          className="bg-slate-900/75 border border-slate-800 hover:border-cyan-500/40 text-slate-300 font-semibold px-4 py-2 rounded-xl transition text-sm flex items-center gap-2"
          {...externalLinkProps}
          {...pressFeedback}
        >
          {content.header.github}
        </motion.a>
        <motion.a
          href="/releases"
          className="border border-cyan-500/40 bg-cyan-500/10 hover:bg-cyan-500/15 text-cyan-200 font-bold px-4 py-2 rounded-xl transition-colors text-sm flex items-center gap-2 cursor-pointer"
          {...pressFeedback}
        >
          {content.header.releases}
        </motion.a>
        <motion.button
          type="button"
          onClick={() => setMenuOpen((value) => !value)}
          className="mobile-menu-button border border-slate-700 bg-slate-900/80 text-slate-200 font-bold px-3 py-2 rounded-xl transition text-xs cursor-pointer"
          aria-expanded={menuOpen}
          aria-controls="mobile-menu"
          {...pressFeedback}
        >
          {content.header.menu}
        </motion.button>
      </div>

      <AnimatePresence>
        {menuOpen ? (
          <motion.div
            id="mobile-menu"
            className="mobile-menu-panel is-open"
            // Anchored to the menu button that opened it (top-right of the
            // header), and dismisses back the same way it arrived.
            style={{ transformOrigin: "top right" }}
            initial={{ opacity: 0, scale: 0.92, y: -8 }}
            animate={{ opacity: 1, scale: 1, y: 0 }}
            exit={{ opacity: 0, scale: 0.94, y: -6 }}
            transition={spring(springSheet)}
          >
            <motion.button
              type="button"
              onClick={() => { openSpecs(); closeMenu(); }}
              className="mobile-menu-item text-cyan-300"
              whileTap={{ scale: 0.97 }}
            >
              {content.header.features}
            </motion.button>
            {navItems.map((item) => (
              <a href={item.href} onClick={closeMenu} className={`mobile-menu-item ${isActive(item.href) ? "is-active" : ""}`} key={`mobile-${item.href}`}>
                {item.label}
              </a>
            ))}
          </motion.div>
        ) : null}
      </AnimatePresence>
    </header>
  );
}
