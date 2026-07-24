import { useEffect, useState } from "react";
import { LanguageProvider } from "./context/LanguageContext.jsx";
import ArchitecturePage from "./pages/ArchitecturePage.jsx";
import ContactPage from "./pages/ContactPage.jsx";
import DocsPage from "./pages/DocsPage.jsx";
import FeaturesPage from "./pages/FeaturesPage.jsx";
import HardwarePage from "./pages/HardwarePage.jsx";
import Home from "./pages/Home.jsx";
import ReleasesPage from "./pages/ReleasesPage.jsx";

const routes = {
  "/": Home,
  "/architecture": ArchitecturePage,
  "/contact": ContactPage,
  "/docs": DocsPage,
  "/features": FeaturesPage,
  "/hardware": HardwarePage,
  "/releases": ReleasesPage,
};

export default function App() {
  const [path, setPath] = useState(window.location.pathname);
  const Page = routes[path] || Home;

  useEffect(() => {
    const onPopState = () => setPath(window.location.pathname);
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

    window.addEventListener("popstate", onPopState);
    document.addEventListener("click", onClick);

    return () => {
      window.removeEventListener("popstate", onPopState);
      document.removeEventListener("click", onClick);
    };
  }, []);

  return (
    <LanguageProvider>
      <Page />
    </LanguageProvider>
  );
}
