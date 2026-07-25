import { useEffect, useState } from "react";
import SpecsModal from "./components/ui/SpecsModal.jsx";
import { LanguageProvider } from "./context/LanguageContext.jsx";
import { ModalProvider } from "./context/ModalContext.jsx";
import HardwarePage from "./pages/HardwarePage.jsx";
import Home from "./pages/Home.jsx";
import ReleasesPage from "./pages/ReleasesPage.jsx";

const routes = {
  "/": Home,
  "/hardware": HardwarePage,
  "/releases": ReleasesPage,
};

export default function App() {
  const [path, setPath] = useState(window.location.pathname);
  const Page = routes[path] || Home;

  useEffect(() => {
    const onPopState = () => setPath(window.location.pathname);
    const updateRoute = (url) => {
      window.history.pushState({}, "", `${url.pathname}${url.hash}`);
      setPath(url.pathname);

      requestAnimationFrame(() => {
        if (url.hash) {
          document.querySelector(url.hash)?.scrollIntoView();
        } else {
          window.scrollTo({ top: 0 });
        }
      });
    };
    const onClick = (event) => {
      const link = event.target.closest("a");

      if (!link || link.target || event.metaKey || event.ctrlKey || event.shiftKey || event.altKey) {
        return;
      }

      const url = new URL(link.href);
      const isLocal = url.origin === window.location.origin;
      const isKnownRoute = Boolean(routes[url.pathname]);

      if (!isLocal || !isKnownRoute) {
        return;
      }

      event.preventDefault();

      if (document.startViewTransition && !window.matchMedia("(prefers-reduced-motion: reduce)").matches) {
        document.startViewTransition(() => updateRoute(url));
      } else {
        updateRoute(url);
      }
    };

    window.addEventListener("popstate", onPopState);
    document.addEventListener("click", onClick);

    return () => {
      window.removeEventListener("popstate", onPopState);
      document.removeEventListener("click", onClick);
    };
  }, []);

  return (
    <LanguageProvider>
      <ModalProvider>
        <div className="page-transition" key={path}>
          <Page />
        </div>
        <SpecsModal />
      </ModalProvider>
    </LanguageProvider>
  );
}
