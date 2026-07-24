import { releaseDownloads, repoUrl } from "../data/siteData.js";

export function listReleaseDownloads() {
  return releaseDownloads;
}

function formatReleaseDate(value) {
  return new Intl.DateTimeFormat("tr-TR", {
    day: "numeric",
    month: "long",
    year: "numeric",
  }).format(new Date(value));
}

function summarizeRelease(release) {
  if (release.body) {
    return release.body.replace(/\s+/g, " ").trim().slice(0, 180);
  }

  return release.prerelease ? "GitHub pre-release kaydı." : "GitHub release kaydı.";
}

export async function fetchReleaseDownloads() {
  const response = await fetch("https://api.github.com/repos/ozcelik329/AeroPico-FC/releases", {
    headers: { Accept: "application/vnd.github+json" },
  });

  if (!response.ok) {
    throw new Error(`GitHub releases request failed: ${response.status}`);
  }

  const releases = await response.json();

  return releases.slice(0, 8).map((release, index) => ({
    title: release.name || release.tag_name,
    version: release.tag_name,
    badge: release.prerelease ? "Pre-release" : "Release",
    badgeTone: index === 0 ? "cyan" : "emerald",
    published: formatReleaseDate(release.published_at || release.created_at),
    text: summarizeRelease(release),
    href: release.html_url,
    downloadHref: release.zipball_url || `${repoUrl}/archive/refs/tags/${release.tag_name}.zip`,
    actionLabel: "Release'i Aç",
  }));
}
