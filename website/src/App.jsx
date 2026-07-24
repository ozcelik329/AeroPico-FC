import { ModalProvider } from "./context/ModalContext.jsx";
import { LanguageProvider } from "./context/LanguageContext.jsx";
import HardwarePage from "./pages/HardwarePage.jsx";
import Home from "./pages/Home.jsx";
import ReleasesModal from "./components/ui/ReleasesModal.jsx";
import SpecsModal from "./components/ui/SpecsModal.jsx";

export default function App() {
  const route = window.location.pathname === "/hardware" ? "hardware" : "home";

  return (
    <LanguageProvider>
      <ModalProvider>
        {route === "hardware" ? <HardwarePage /> : <Home />}
        <ReleasesModal />
        <SpecsModal />
      </ModalProvider>
    </LanguageProvider>
  );
}
