import { ModalProvider } from "./context/ModalContext.jsx";
import { LanguageProvider } from "./context/LanguageContext.jsx";
import Home from "./pages/Home.jsx";
import ReleasesModal from "./components/ui/ReleasesModal.jsx";
import SpecsModal from "./components/ui/SpecsModal.jsx";

export default function App() {
  return (
    <LanguageProvider>
      <ModalProvider>
        <Home />
        <ReleasesModal />
        <SpecsModal />
      </ModalProvider>
    </LanguageProvider>
  );
}
