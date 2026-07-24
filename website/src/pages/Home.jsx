import Configurator from "../components/Configurator.jsx";
import Hero from "../components/Hero.jsx";
import QuickStart from "../components/QuickStart.jsx";
import PageShell from "../components/layout/PageShell.jsx";

export default function Home() {
  return (
    <PageShell>
      <Hero />
      <QuickStart />
      <Configurator />
    </PageShell>
  );
}
