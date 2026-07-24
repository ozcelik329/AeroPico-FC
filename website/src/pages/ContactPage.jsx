import { useLanguage } from "../context/LanguageContext.jsx";
import PageShell from "../components/layout/PageShell.jsx";

export default function ContactPage() {
  const { content } = useLanguage();

  return (
    <PageShell>
      <section className="site-container py-16 md:py-20 relative z-10">
        <div className="max-w-3xl space-y-6">
          <div className="text-xs uppercase font-mono text-cyan-400 font-semibold tracking-wider bg-cyan-500/10 border border-cyan-500/20 px-3 py-1 rounded-full inline-block">
            {content.footer.contact}
          </div>
          <h1 className="text-3xl md:text-5xl font-extrabold text-white">Muhammed Fatih Emre Özçelik</h1>
          <div className="rounded-3xl border border-cyan-400/25 bg-cyan-500/10 px-6 py-5 shadow-lg shadow-cyan-500/10">
            <p className="text-slate-300">
              {content.footer.contactText}{" "}
              <a className="font-bold text-white underline decoration-cyan-400/50 underline-offset-4 hover:text-cyan-200 transition" href="mailto:fatihemreozcelik@gmail.com">
                fatihemreozcelik@gmail.com
              </a>
            </p>
          </div>
        </div>
      </section>
    </PageShell>
  );
}
