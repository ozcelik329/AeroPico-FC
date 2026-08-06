import { createContext, useMemo, useState } from "react";
import { useBodyScrollLock } from "../hooks/useBodyScrollLock.js";

export const modalNames = {
  specs: "specs",
};

export const ModalContext = createContext(null);

export function ModalProvider({ children }) {
  const [activeModal, setActiveModal] = useState(null);

  useBodyScrollLock(Boolean(activeModal));

  const value = useMemo(
    () => ({
      activeModal,
      closeModal: () => setActiveModal(null),
      openSpecs: () => setActiveModal(modalNames.specs),
    }),
    [activeModal],
  );

  return <ModalContext.Provider value={value}>{children}</ModalContext.Provider>;
}
