import { fetchReleaseDownloads, listReleaseDownloads } from "../api/releaseApi.js";

export function getReleaseDownloads() {
  return listReleaseDownloads();
}

export async function getLiveReleaseDownloads() {
  return fetchReleaseDownloads();
}
