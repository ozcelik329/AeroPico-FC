import { useEffect } from "react";
import Footer from "./Footer.jsx";
import Header from "./Header.jsx";

export default function PageShell({ children }) {
  useEffect(() => {
    const revealObserver = new IntersectionObserver(
      (entries, observer) => {
        entries.forEach((entry) => {
          if (!entry.isIntersecting) return;

          entry.target.classList.add("is-visible");
          observer.unobserve(entry.target);
        });
      },
      { threshold: 0.12, rootMargin: "0px 0px -12% 0px" }
    );

    const sections = document.querySelectorAll(".reveal-section");
    sections.forEach((section) => revealObserver.observe(section));

    return () => revealObserver.disconnect();
  }, []);

  return (
    <div className="bg-slate-950 text-slate-100 min-h-screen flex flex-col justify-between selection:bg-cyan-500 selection:text-slate-950 relative overflow-x-hidden">
      <div className="absolute inset-0 bg-[linear-gradient(135deg,rgba(14,165,233,0.10),transparent_26%,rgba(16,185,129,0.08)_46%,transparent_65%,rgba(245,158,11,0.08))] pointer-events-none" />
      <div className="absolute inset-0 opacity-[0.08] bg-[linear-gradient(rgba(255,255,255,0.08)_1px,transparent_1px),linear-gradient(90deg,rgba(255,255,255,0.08)_1px,transparent_1px)] bg-[size:44px_44px] pointer-events-none" />

      <Header />
      <div className="header-spacer" aria-hidden="true" />
      <main className="relative z-10 flex-1">{children}</main>
      <Footer />
    </div>
  );
}
