'use client';
import { SidebarProvider, SidebarInset } from '@/_components/ui/sidebar';
import SideBar from '@/_components/fixed-components/SideBar/SideBar';

export default function DashboardShell({
  open,
  onOpenChange,
  children,
}: {
  open: boolean;
  onOpenChange: (open: boolean) => void;
  children: React.ReactNode;
}) {
  return (
    <SidebarProvider open={open} onOpenChange={onOpenChange}>
      <SideBar />
      <SidebarInset>{children}</SidebarInset>
    </SidebarProvider>
  );
}
