import { useContext } from "react";
import { ModalContext } from "../context/ModalContext.jsx";

export function useModal() {
  const modal = useContext(ModalContext);

  if (!modal) {
    throw new Error("useModal must be used inside ModalProvider");
  }

  return modal;
}
