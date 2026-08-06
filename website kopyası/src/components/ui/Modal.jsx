import { AnimatePresence, motion } from "motion/react";
import { useEffect } from "react";
import { cn } from "../../utils/cn.js";
import { spring, springSheet } from "../../utils/motionPresets.js";

export default function Modal({ ariaLabel, children, maxWidth = "max-w-2xl", open, onClose }) {
  useEffect(() => {
    if (!open) {
      return undefined;
    }

    const handleKeyDown = (event) => {
      if (event.key === "Escape") {
        onClose();
      }
    };

    window.addEventListener("keydown", handleKeyDown);

    return () => {
      window.removeEventListener("keydown", handleKeyDown);
    };
  }, [onClose, open]);

  return (
    <AnimatePresence>
      {open ? (
        <motion.div
          className="fixed inset-0 bg-slate-950/80 backdrop-blur-sm z-[80] flex items-center justify-center p-4"
          onClick={onClose}
          role="presentation"
          initial={{ opacity: 0 }}
          animate={{ opacity: 1 }}
          exit={{ opacity: 0 }}
          transition={spring({ duration: 0.25, ease: "easeOut" })}
        >
          <motion.div
            className={cn("glass-card border border-slate-800 w-full rounded-3xl p-6 md:p-8 space-y-6 shadow-2xl relative", maxWidth)}
            onClick={(event) => event.stopPropagation()}
            role="dialog"
            aria-modal="true"
            aria-label={ariaLabel}
            // Materialize, don't just fade: blur + scale + a small rise settle
            // together so the sheet reads as arriving material, not a flat
            // opacity toggle. Spring is interruptible — a rapid re-open while
            // it's still animating out picks up from the live value, not a jump.
            initial={{ opacity: 0, scale: 0.94, y: 16, filter: "blur(8px)" }}
            animate={{ opacity: 1, scale: 1, y: 0, filter: "blur(0px)" }}
            exit={{ opacity: 0, scale: 0.96, y: 10, filter: "blur(6px)" }}
            transition={spring(springSheet)}
          >
            {children}
          </motion.div>
        </motion.div>
      ) : null}
    </AnimatePresence>
  );
}
