import { useEffect } from "react";
import { cn } from "../../utils/cn.js";

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
    <div
      className={cn(
        "fixed inset-0 bg-slate-950/80 backdrop-blur-sm z-[80] flex items-center justify-center p-4 transition-opacity duration-300",
        open ? "opacity-100 pointer-events-auto" : "opacity-0 pointer-events-none",
      )}
      onClick={onClose}
      role="presentation"
    >
      <div
        className={cn(
          "glass-card border border-slate-800 w-full rounded-3xl p-6 md:p-8 space-y-6 shadow-2xl relative transform transition-transform duration-300",
          maxWidth,
          open ? "scale-100" : "scale-95",
        )}
        onClick={(event) => event.stopPropagation()}
        role="dialog"
        aria-modal="true"
        aria-label={ariaLabel}
      >
        {children}
      </div>
    </div>
  );
}
