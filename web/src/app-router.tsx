import { lazy, Suspense } from "react"

import { IpoSetupPage } from "@/pages/ipo-setup-page"
import { AppPageLoading } from "@/components/app-chrome"
import { ROUTES, usePathname } from "@/lib/routing"

/** Keep legacy v1/v5 and 3D out of the operator IPO editor bundle. */
const LegacyV1Page = lazy(async () => {
  const module = await import("@/pages/setup-page")
  return { default: module.SetupPage }
})

const ResearchV5Page = lazy(async () => {
  const module = await import("@/App")
  return { default: module.App }
})

const Chamber3dPage = lazy(async () => {
  const module = await import("@/pages/chamber-3d-page")
  return { default: module.Chamber3dPage }
})

/** Operator setup at `/`, original research v5 at `/lab/v5`, optional R3F preview. */
export function AppRouter() {
  const pathname = usePathname()

  if (pathname === ROUTES.chamber3d || pathname.startsWith(`${ROUTES.chamber3d}/`)) {
    return (
      <Suspense fallback={<AppPageLoading>Ładowanie podglądu 3D…</AppPageLoading>}>
        <Chamber3dPage />
      </Suspense>
    )
  }

  if (pathname === ROUTES.legacyV5) {
    return (
      <Suspense fallback={<AppPageLoading>Ładowanie laboratorium v5…</AppPageLoading>}>
        <ResearchV5Page />
      </Suspense>
    )
  }

  if (pathname === ROUTES.profileV1) {
    return (
      <Suspense fallback={<AppPageLoading>Ładowanie archiwalnego profilu v1…</AppPageLoading>}>
        <LegacyV1Page />
      </Suspense>
    )
  }

  return <IpoSetupPage />
}
