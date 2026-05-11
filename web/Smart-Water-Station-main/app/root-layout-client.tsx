'use client';
import { usePathname } from 'next/navigation';
import { lazy, Suspense, useEffect, useState } from 'react';
import ClientLayout from './client-layout';

const DashboardShell = lazy(() => import('./dashboard-shell'));

export default function RootLayoutClient({
  children,
}: {
  children: React.ReactNode;
}) {
  const pathname = usePathname();
  const dashboardPages = ['/home', '/sensors', '/control', '/ai', '/prototype', '/reports', '/alerts'];
  const isDashboardPage = dashboardPages.includes(pathname);
  const [sidebarOpen, setSidebarOpen] = useState(isDashboardPage);

  // Sync sidebar state with pathname
  useEffect(() => {
    setSidebarOpen(isDashboardPage);
  }, [isDashboardPage]);

  return (
    <>
      {isDashboardPage ? (
        <Suspense fallback={<main className="min-h-screen bg-[#d8e2f4]" />}>
          <DashboardShell open={sidebarOpen} onOpenChange={setSidebarOpen}>
            <ClientLayout>{children}</ClientLayout>
          </DashboardShell>
        </Suspense>
      ) : (
        <ClientLayout>{children}</ClientLayout>
      )}
    </>
  );
}
