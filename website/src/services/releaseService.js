import { fetchReleaseDownloads, listReleaseDownloads } from "../api/releaseApi.js";

export function getReleaseDownloads(fallbackDownloads) {
  return listReleaseDownloads(fallbackDownloads);
}

export async function getLiveReleaseDownloads(language) {
  return fetchReleaseDownloads(language);
}
