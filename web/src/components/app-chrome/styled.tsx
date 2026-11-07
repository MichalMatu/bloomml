import { createElement, type HTMLAttributes, type ReactNode } from "react"
import { CardContent } from "@/components/ui/card"

// Central, typed layout presets. Feature pages choose semantic tokens, never Tailwind.
const STYLES = {
  cursorPointerTextSmFontMedium: "cursor-pointer text-sm font-medium",
  flexFlexWrapItemsEndGap3: "flex flex-wrap items-end gap-3",
  flexItemsCenterGap2: "flex items-center gap-2",
  flexItemsCenterJustifyBetween: "flex items-center justify-between",
  flexItemsCenterJustifyBetweenGap2: "flex items-center justify-between gap-2",
  flexItemsCenterJustifyBetweenGap3: "flex items-center justify-between gap-3",
  flexItemsCenterJustifyBetweenGap3RoundedLgBorderP3: "flex items-center justify-between gap-3 rounded-lg border p-3",
  flexItemsCenterJustifyBetweenGap3RoundedLgBorderPx3Py2: "flex items-center justify-between gap-3 rounded-lg border px-3 py-2",
  flexItemsStartJustifyBetweenGap4RoundedLgBorderP4: "flex items-start justify-between gap-4 rounded-lg border p-4",
  fontMedium: "font-medium",
  fontSemibold: "font-semibold",
  gridGap2LgGridCols2: "grid gap-2 lg:grid-cols-2",
  gridGap3LgGridCols2: "grid gap-3 lg:grid-cols-2",
  gridGap3MdGridCols2: "grid gap-3 md:grid-cols-2",
  gridGap3MdGridCols3: "grid gap-3 md:grid-cols-3",
  gridGap3MdGridCols4: "grid gap-3 md:grid-cols-4",
  gridGap3RoundedLgBorderP3: "grid gap-3 rounded-lg border p-3",
  gridGap3SmGridCols2: "grid gap-3 sm:grid-cols-2",
  gridGap4MdGridCols2: "grid gap-4 md:grid-cols-2",
  gridGap4MdGridCols3: "grid gap-4 md:grid-cols-3",
  gridItemsEndGap3LgGridColsCustom1fr1frAuto: "grid items-end gap-3 app-ipo-link-grid",
  minW0SpaceY1: "min-w-0 space-y-1",
  minW48Flex1: "min-w-48 flex-1",
  minW64Flex1: "min-w-64 flex-1",
  mt2ListDiscSpaceY1Pl5TextSmTextMutedForeground: "mt-2 list-disc space-y-1 pl-5 text-sm text-muted-foreground",
  mt3TextSmTextMutedForeground: "mt-3 text-sm text-muted-foreground",
  roundedLgBorderP3: "rounded-lg border p-3",
  spaceY2: "space-y-2",
  spaceY2RoundedLgBorderP3: "space-y-2 rounded-lg border p-3",
  spaceY2RoundedLgBorderP4: "space-y-2 rounded-lg border p-4",
  spaceY3RoundedLgBorderP3: "space-y-3 rounded-lg border p-3",
  spaceY4: "space-y-4",
  spaceY4RoundedLgBorderP4: "space-y-4 rounded-lg border p-4",
  text2xlFontSemibold: "text-2xl font-semibold",
  textSm: "text-sm",
  textSmFontMedium: "text-sm font-medium",
  textSmFontSemibold: "text-sm font-semibold",
  textSmTextMutedForeground: "text-sm text-muted-foreground",
  textXsFontMonoTextMutedForeground: "text-xs font-mono text-muted-foreground",
  textXsTextMutedForeground: "text-xs text-muted-foreground",
} as const

type Layout = keyof typeof STYLES
type Tag = "div" | "p" | "span" | "summary" | "ul" | "details" | "CardContent"

export function AppStyled({
  as = "div", layout, children, ...rest
}: Omit<HTMLAttributes<HTMLElement>, "className" | "style"> & {
  as?: Tag
  layout: Layout
  children?: ReactNode
}) {
  const className = STYLES[layout]
  if (as === "CardContent") {
    return <CardContent {...rest} className={className}>{children}</CardContent>
  }
  return createElement(as, { ...rest, className }, children)
}
