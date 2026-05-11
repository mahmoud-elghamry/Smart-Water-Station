import { lazy, Suspense } from "react";
import RootLayoutClient from "./root-layout-client";
import { usePathname } from "next/navigation";
import type { LazyExoticComponent, ComponentType } from "react";

const routes: Record<string, LazyExoticComponent<ComponentType>> = {
  "/": lazy(() => import("./page")),
  "/about": lazy(() => import("./(pages)/about/page")),
  "/ai": lazy(() => import("./(pages)/ai/page")),
  "/alerts": lazy(() => import("./(pages)/alerts/page")),
  "/contact": lazy(() => import("./(pages)/contact/page")),
  "/control": lazy(() => import("./(pages)/control/page")),
  "/home": lazy(() => import("./(pages)/home/page")),
  "/login": lazy(() => import("./(pages)/login/page")),
  "/prototype": lazy(() => import("./(pages)/prototype/page")),
  "/reports": lazy(() => import("./(pages)/reports/page")),
  "/sensors": lazy(() => import("./(pages)/sensors/page")),
};

export default function App() {
  const pathname = usePathname();
  const Page = routes[pathname] ?? routes["/"];

  return (
    <RootLayoutClient>
      <Suspense fallback={<div className="min-h-screen bg-[#d8e2f4] p-6 text-[#4169b3]">Loading...</div>}>
        <Page />
      </Suspense>
    </RootLayoutClient>
  );
}
