import QuickStart from "../components/QuickStart.jsx";
import ReleaseList from "../components/ReleaseList.jsx";
import PageShell from "../components/layout/PageShell.jsx";

export default function DownloadsPage() {
  return (
    <PageShell>
      <QuickStart />
      <ReleaseList />
    </PageShell>
  );
}
