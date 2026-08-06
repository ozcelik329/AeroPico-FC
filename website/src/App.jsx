import { useEffect, useState } from "react";
import SpecsModal from "./components/ui/SpecsModal.jsx";
import { LanguageProvider } from "./context/LanguageContext.jsx";
import { ModalProvider } from "./context/ModalContext.jsx";
import HardwarePage from "./pages/HardwarePage.jsx";
import Home from "./pages/Home.jsx";
import ConfiguratorPage from "./pages/ConfiguratorPage.jsx";
import ReleasesPage from "./pages/ReleasesPage.jsx";

const routes = {
  "/": Home,
  "/configurator": ConfiguratorPage,
  "/hardware": HardwarePage,
  "/releases": ReleasesPage,
};

export default function App() {
  const [path, setPath] = useState(window.location.pathname);
  const Page = routes[path] || Home;

  useEffect(() => {
    const onPopState = () => setPath(window.location.pathname);
    const updateRoute = (url) => {
      const oldURL = window.location.href;
      window.history.pushState({}, "", `${url.pathname}${url.hash}`);
      setPath(url.pathname);

      if (url.hash && url.pathname === window.location.pathname && window.location.hash !== url.hash) {
        window.dispatchEvent(new HashChangeEvent("hashchange", { oldURL, newURL: url.href }));
      }

      requestAnimationFrame(() => {
        if (url.hash) {
          document.querySelector(url.hash)?.scrollIntoView({
            behavior: "smooth",
            block: "start",
            inline: "nearest",
          });
        } else {
          window.scrollTo({ top: 0, behavior: "smooth" });
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

      const isSamePageHash = url.pathname === window.location.pathname && url.hash;
      if (isSamePageHash) {
        updateRoute(url);
        return;
      }

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
