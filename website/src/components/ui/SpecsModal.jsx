import { modalNames } from "../../context/ModalContext.jsx";
import { useLanguage } from "../../context/LanguageContext.jsx";
import { useModal } from "../../hooks/useModal.js";
import { cn } from "../../utils/cn.js";
import Modal from "./Modal.jsx";

export default function SpecsModal() {
  const { activeModal, closeModal } = useModal();
  const { content } = useLanguage();
  const open = activeModal === modalNames.specs;
  const specifications = content.specs;

  return (
    <Modal ariaLabel={content.modals.specsTitle} maxWidth="max-w-4xl max-h-[90vh] flex flex-col" open={open} onClose={closeModal}>
      <div className="flex justify-between items-center pb-4 border-b border-slate-800">
        <div>
          <h3 className="text-xl font-extrabold text-white">{content.modals.specsTitle}</h3>
          <p className="text-slate-400 text-xs mt-1">{content.modals.specsDescription}</p>
        </div>
        <button
          type="button"
          onClick={closeModal}
          className="w-8 h-8 rounded-full bg-slate-900 border border-slate-800 text-slate-400 hover:text-white flex items-center justify-center font-bold transition cursor-pointer"
          aria-label={content.modals.close}
        >
          x
        </button>
      </div>

      <div className="space-y-6 overflow-y-auto pr-2 flex-grow">
        <div className="bg-gradient-to-r from-cyan-500/10 via-blue-500/10 to-transparent border border-cyan-500/20 p-5 rounded-2xl flex items-center justify-between">
          <div>
            <div className="text-[10px] font-mono text-cyan-400 uppercase tracking-widest">{content.modals.stack}</div>
            <div className="text-xl font-extrabold text-white mt-1">{content.modals.stackTitle}</div>
          </div>
          <div className="text-right hidden sm:block">
            <div className="text-[10px] font-mono text-slate-400">TARGET</div>
            <div className="text-sm font-bold text-cyan-400">RP2350 / Pico 2</div>
          </div>
        </div>

        <div className="grid md:grid-cols-2 lg:grid-cols-3 gap-4">
          {specifications.map((spec) => (
            <article
              className={cn("bg-slate-900/60 border border-slate-800 p-4 rounded-xl space-y-1", spec.wide && "md:col-span-2 lg:col-span-3")}
              key={`${spec.label}-${spec.value}`}
            >
              <div className="text-[10px] font-mono text-cyan-400">{spec.label}</div>
              <h4 className="font-bold text-white text-sm">{spec.value}</h4>
              <p className="text-slate-400 text-xs leading-relaxed">{spec.text}</p>
            </article>
          ))}
        </div>
      </div>

      <div className="pt-2 border-t border-slate-800 flex justify-end">
        <button type="button" onClick={closeModal} className="bg-slate-800 hover:bg-slate-700 text-slate-200 text-xs font-semibold px-5 py-2.5 rounded-xl transition cursor-pointer">
          {content.modals.close}
        </button>
      </div>
    </Modal>
  );
}
