import { createContext, useCallback, useContext, useEffect, useMemo, useState } from "react";
import { content } from "../data/i18n.js";

export const LanguageContext = createContext(null);

export function LanguageProvider({ children }) {
  const [language, setLanguage] = useState("tr");
  const activeContent = content[language];

  const toggleLanguage = useCallback(() => {
    setLanguage((current) => (current === "tr" ? "en" : "tr"));
  }, []);

  const value = useMemo(
    () => ({
      content: activeContent,
      language,
      setLanguage,
      toggleLanguage,
    }),
    [activeContent, language, toggleLanguage],
  );

  useEffect(() => {
    document.documentElement.lang = language;
    document.title = activeContent.seo.title;
    document.querySelector("meta[name='description']")?.setAttribute("content", activeContent.seo.description);
  }, [activeContent, language]);

  return <LanguageContext.Provider value={value}>{children}</LanguageContext.Provider>;
}

export function useLanguage() {
  const value = useContext(LanguageContext);

  if (!value) {
    throw new Error("useLanguage must be used within LanguageProvider");
  }

  return value;
}
