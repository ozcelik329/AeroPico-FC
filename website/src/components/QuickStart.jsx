import configuratorPreview from "../assets/aeropico-configurator-dark.webp";
import { useLanguage } from "../context/LanguageContext.jsx";
import { cn } from "../utils/cn.js";
import { externalLinkProps } from "../utils/externalLinkProps.js";
import SectionHeading from "./ui/SectionHeading.jsx";
import PlatformDownloadButtons from "./ui/PlatformDownloadButtons.jsx";

const themeClasses = {
  cyan: {
    badge: "bg-cyan-500/10 text-cyan-400 border-cyan-500/20",
    icon: "bg-cyan-500/10 border-cyan-500/20 text-cyan-400",
    title: "text-cyan-400",
    link: "bg-cyan-500/20 hover:bg-cyan-500/30 text-cyan-300 border-cyan-500/30",
  },
  blue: {
    badge: "bg-slate-800/80 text-slate-300 border-slate-700",
    icon: "bg-slate-900 border-slate-700 text-slate-300",
    title: "text-slate-200",
    link: "bg-slate-900 hover:bg-slate-800 text-slate-200 border-slate-700",
  },
};

function QuickStartCard({ option }) {
  const theme = themeClasses[option.theme];

  return (
    <article className="flat-card p-8 rounded-3xl flex flex-col justify-between space-y-6 relative overflow-hidden group transition">
      <div className={cn("absolute top-0 right-0 text-xs font-mono px-4 py-2 rounded-bl-2xl border-l border-b font-bold", theme.badge)}>
        {option.badge}
      </div>

      <div className="space-y-4">
        <div className={cn("w-12 h-12 rounded-2xl border flex items-center justify-center font-extrabold text-lg", theme.icon)}>{option.number}</div>
        <div>
          <h3 className="text-xl font-extrabold text-white">{option.title}</h3>
          <p className="text-slate-400 text-xs leading-relaxed mt-2">{option.description}</p>
        </div>

        {option.preview ? (
          <div className="quickstart-preview">
            <img src={configuratorPreview} alt={option.previewAlt} loading="lazy" decoding="async" />
            <div>
              <span>{option.previewEyebrow}</span>
              <strong>{option.previewTitle}</strong>
              <p>{option.previewText}</p>
            </div>
          </div>
        ) : null}

        {option.steps ? (
          <div className="space-y-3 pt-2 text-xs text-slate-300">
            {option.steps.map((step) => (
              <div className="bg-slate-900/90 border border-slate-800 p-3.5 rounded-xl space-y-2" key={step.title}>
                <span className={cn("font-bold block", theme.title)}>{step.title}</span>
                <p className="text-slate-400 leading-relaxed">{step.body}</p>
                {step.code ? (
                  <code className="block bg-slate-950 p-2.5 rounded-lg text-emerald-400 font-mono text-[11px] select-all border border-slate-800/80">
                    {step.code}
                  </code>
                ) : null}
                {step.link ? (
                  <a className={cn("inline-block border px-3 py-1.5 rounded-lg font-semibold transition", theme.link)} href={step.link.href} {...externalLinkProps}>
                    {step.link.label}
                  </a>
                ) : null}
              </div>
            ))}
          </div>
        ) : null}

        {option.downloads ? <PlatformDownloadButtons items={option.downloads} /> : null}
      </div>

      <div className="pt-4 border-t border-slate-800/80 flex items-center justify-between">
        <span className="text-xs text-slate-500 font-mono">{option.footer}</span>
        <a href={option.action.href} className={cn("text-xs font-bold hover:underline", theme.title)} {...(option.action.href.startsWith("http") ? externalLinkProps : {})}>
          {option.action.label} →
        </a>
      </div>
    </article>
  );
}

export default function QuickStart() {
  const { content } = useLanguage();

  return (
    <section id="quickstart" className="site-container reveal-section py-16 border-t border-slate-900/80 relative z-10">
      <SectionHeading
        eyebrow={content.quickStart.eyebrow}
        title={content.quickStart.title}
        description={content.quickStart.description}
      />

      <div className="grid md:grid-cols-2 gap-8">
        {content.quickStart.options.map((option) => (
          <QuickStartCard option={option} key={option.id} />
        ))}
      </div>
    </section>
  );
}
