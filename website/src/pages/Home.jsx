import Architecture from "../components/Architecture.jsx";
import Configurator from "../components/Configurator.jsx";
import Docs from "../components/Docs.jsx";
import Hero from "../components/Hero.jsx";
import QuickStart from "../components/QuickStart.jsx";
import ReleaseGate from "../components/ReleaseGate.jsx";
import Footer from "../components/layout/Footer.jsx";
import Header from "../components/layout/Header.jsx";

export default function Home() {
  return (
    <div className="bg-slate-950 text-slate-100 min-h-screen flex flex-col justify-between selection:bg-cyan-500 selection:text-slate-950 relative overflow-x-hidden">
      <div className="absolute inset-0 bg-[linear-gradient(135deg,rgba(14,165,233,0.10),transparent_26%,rgba(16,185,129,0.08)_46%,transparent_65%,rgba(245,158,11,0.08))] pointer-events-none" />
      <div className="absolute inset-0 opacity-[0.08] bg-[linear-gradient(rgba(255,255,255,0.08)_1px,transparent_1px),linear-gradient(90deg,rgba(255,255,255,0.08)_1px,transparent_1px)] bg-[size:44px_44px] pointer-events-none" />

      <Header />
      <div className="h-20 md:h-20" />
      <Hero />
      <QuickStart />
      <Architecture />
      <Configurator />
      <Docs />
      <ReleaseGate />
      <Footer />
    </div>
  );
}
