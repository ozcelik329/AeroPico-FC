import { repoUrl } from "../data/siteData.js";

export function listReleaseDownloads(fallbackDownloads) {
  return fallbackDownloads;
}

function formatReleaseDate(value, language) {
  return new Intl.DateTimeFormat(language === "en" ? "en-US" : "tr-TR", {
    day: "numeric",
    month: "long",
    year: "numeric",
  }).format(new Date(value));
}

function summarizeRelease(release, language) {
  if (release.body) {
    return release.body.replace(/\s+/g, " ").trim().slice(0, 180);
  }

  if (language === "en") {
    return release.prerelease ? "GitHub pre-release entry." : "GitHub release entry.";
  }

  return release.prerelease ? "GitHub pre-release kaydı." : "GitHub release kaydı.";
}

export async function fetchReleaseDownloads(language = "tr") {
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
    published: formatReleaseDate(release.published_at || release.created_at, language),
    text: summarizeRelease(release, language),
    href: release.html_url,
    downloadHref: release.zipball_url || `${repoUrl}/archive/refs/tags/${release.tag_name}.zip`,
  }));
}
