import { modalNames } from "../../context/ModalContext.jsx";
import { repoUrl } from "../../data/siteData.js";
import { useModal } from "../../hooks/useModal.js";
import { getReleaseDownloads } from "../../services/releaseService.js";
import { cn } from "../../utils/cn.js";
import { externalLinkProps } from "../../utils/externalLinkProps.js";
import Modal from "./Modal.jsx";

const badgeClasses = {
  cyan: "bg-cyan-500/10 text-cyan-400 border-cyan-500/20",
  blue: "bg-blue-500/10 text-blue-400 border-blue-500/20",
  emerald: "bg-emerald-500/10 text-emerald-400 border-emerald-500/20",
  amber: "bg-amber-500/10 text-amber-400 border-amber-500/20",
  rose: "bg-rose-500/10 text-rose-400 border-rose-500/20",
};

export default function ReleasesModal() {
  const { activeModal, closeModal } = useModal();
  const releases = getReleaseDownloads();
  const open = activeModal === modalNames.releases;

  return (
    <Modal ariaLabel="Yazılım sürümleri" open={open} onClose={closeModal}>
      <div className="flex justify-between items-center pb-4 border-b border-slate-800">
        <div>
          <h3 className="text-xl font-extrabold text-white">Yazılım Sürümleri (Releases)</h3>
          <p className="text-slate-400 text-xs mt-1">Kararlı ve geliştirme aşamasındaki paket listesi.</p>
        </div>
        <button
          type="button"
          onClick={closeModal}
          className="w-8 h-8 rounded-full bg-slate-900 border border-slate-800 text-slate-400 hover:text-white flex items-center justify-center font-bold transition cursor-pointer"
          aria-label="Sürümler penceresini kapat"
        >
          ×
        </button>
      </div>

      <div className="space-y-4 max-h-[60vh] overflow-y-auto pr-2">
        {releases.map((release) => (
          <article
            className="bg-slate-900/80 border border-slate-800 p-5 rounded-2xl flex flex-col sm:flex-row justify-between items-start sm:items-center gap-4 hover:border-cyan-500/40 transition"
            key={release.title}
          >
            <div className="space-y-1">
              <div className="flex items-center gap-2">
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
                {release.actionLabel}
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

      <div className="pt-2 border-t border-slate-800 flex justify-between items-center text-xs text-slate-500">
        <span>Kaynak kod deposu</span>
        <a href={repoUrl} className="text-cyan-400 hover:underline" {...externalLinkProps}>
          GitHub'da Görüntüle →
        </a>
      </div>
    </Modal>
  );
}
