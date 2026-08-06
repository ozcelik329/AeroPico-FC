import Architecture from "../components/Architecture.jsx";
import Configurator from "../components/Configurator.jsx";
import Docs from "../components/Docs.jsx";
import Hero from "../components/Hero.jsx";
import HomeOverview from "../components/HomeOverview.jsx";
import QuickStart from "../components/QuickStart.jsx";
import PageShell from "../components/layout/PageShell.jsx";

export default function Home() {
  return (
    <PageShell>
      <Hero />
      <HomeOverview />
      <QuickStart />
      <Architecture />
      <Configurator />
      <Docs />
    </PageShell>
  );
}
