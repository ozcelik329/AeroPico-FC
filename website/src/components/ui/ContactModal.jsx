import { motion } from "motion/react";
import { modalNames } from "../../context/ModalContext.jsx";
import { useLanguage } from "../../context/LanguageContext.jsx";
import { useModal } from "../../hooks/useModal.js";
import { pressFeedback } from "../../utils/motionPresets.js";
import Modal from "./Modal.jsx";

export default function ContactModal() {
  const { activeModal, closeModal } = useModal();
  const { content } = useLanguage();
  const open = activeModal === modalNames.contact;

  return (
    <Modal ariaLabel={content.modals.contactTitle} maxWidth="max-w-3xl max-h-[90vh] flex flex-col" open={open} onClose={closeModal}>
      <div className="flex justify-between items-center pb-4 border-b border-slate-800">
        <div>
          <h3 className="text-xl font-extrabold text-white">{content.modals.contactTitle}</h3>
          <p className="text-slate-400 text-xs mt-1">{content.modals.contactDescription}</p>
        </div>
        <motion.button
          type="button"
          onClick={closeModal}
          className="w-8 h-8 rounded-full bg-slate-900 border border-slate-800 text-slate-400 hover:text-white flex items-center justify-center font-bold cursor-pointer"
          aria-label={content.modals.close}
          {...pressFeedback}
        >
          x
        </motion.button>
      </div>

      <div className="space-y-6 overflow-y-auto pr-2 flex-grow">
        <div className="rounded-3xl border border-cyan-500/15 bg-slate-950/70 p-5">
          <p className="text-slate-300 text-sm leading-relaxed">{content.modals.contactText}</p>
        </div>

        <div className="grid gap-4 sm:grid-cols-2">
          <div className="rounded-3xl border border-slate-800 bg-slate-900/80 p-5">
            <div className="text-xs uppercase tracking-[0.24em] text-cyan-400 font-semibold mb-3">{content.modals.contactEmailLabel}</div>
            <a href={`mailto:${content.modals.contactEmail}`} className="text-slate-100 font-semibold hover:text-cyan-300 transition">{content.modals.contactEmail}</a>
          </div>
          <div className="rounded-3xl border border-slate-800 bg-slate-900/80 p-5">
            <div className="text-xs uppercase tracking-[0.24em] text-cyan-400 font-semibold mb-3">{content.modals.contactLinksLabel}</div>
            <a href={content.modals.contactGithub} className="block text-slate-100 font-semibold hover:text-cyan-300 transition" {...{ target: "_blank", rel: "noreferrer" }}>
              GitHub
            </a>
          </div>
        </div>
      </div>

      <div className="pt-2 border-t border-slate-800 flex justify-end">
        <motion.button
          type="button"
          onClick={closeModal}
          className="bg-slate-800 hover:bg-slate-700 text-slate-200 text-xs font-semibold px-5 py-2.5 rounded-xl cursor-pointer"
          {...pressFeedback}
        >
          {content.modals.close}
        </motion.button>
      </div>
    </Modal>
  );
}
