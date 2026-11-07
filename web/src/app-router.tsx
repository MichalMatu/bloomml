import { lazy, Suspense } from "react"

import { SetupPage } from "@/pages/setup-page"
import { AppPageLoading } from "@/components/app-chrome"
import { ROUTES, usePathname } from "@/lib/routing"

/** Keep research and 3D modules out of the operator setup entry bundle. */
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

  return <SetupPage />
}
