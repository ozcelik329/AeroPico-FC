import { listReleaseDownloads } from "../api/releaseApi.js";

export function getReleaseDownloads() {
  return listReleaseDownloads();
}
