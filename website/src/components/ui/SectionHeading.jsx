export default function SectionHeading({ eyebrow, title, description, align = "center" }) {
  const alignment = align === "left" ? "text-left" : "text-center mx-auto";

  return (
    <div className={`mb-10 max-w-2xl space-y-3 ${alignment}`}>
      <div className="text-xs uppercase font-mono text-cyan-400 font-semibold tracking-wider bg-cyan-500/10 border border-cyan-500/20 px-3.5 py-1.5 rounded-full inline-block">
        {eyebrow}
      </div>
      <h2 className="text-3xl font-extrabold text-white">{title}</h2>
      {description ? <p className="text-slate-400 text-sm">{description}</p> : null}
    </div>
  );
}
