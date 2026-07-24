import Architecture from "../components/Architecture.jsx";
import ReleaseGate from "../components/ReleaseGate.jsx";
import PageShell from "../components/layout/PageShell.jsx";

export default function ArchitecturePage() {
  return (
    <PageShell>
      <Architecture />
      <ReleaseGate />
    </PageShell>
  );
}
