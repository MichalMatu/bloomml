import { useSyncExternalStore } from "react"

function subscribe(onStoreChange: () => void): () => void {
  window.addEventListener("popstate", onStoreChange)
  return () => window.removeEventListener("popstate", onStoreChange)
}

function getPathname(): string {
  return window.location.pathname
}

const base = import.meta.env.BASE_URL.replace(/\/$/, "")

/** Client-side navigation for the static SPA (no router library). */
export function navigate(to: string): void {
  const full = base + to
  if (window.location.pathname === full) return
  window.history.pushState({}, "", full)
  window.dispatchEvent(new PopStateEvent("popstate"))
}

export function usePathname(): string {
  return useSyncExternalStore(
    subscribe,
    () => getPathname().replace(base, "") || "/",
    () => "/",
  )
}

export const ROUTES = {
  configurator: "/",
  legacyV5: "/lab/v5",
  profileV1: "/lab/profile-v1",
  chamber3d: "/chamber-3d",
} as const
