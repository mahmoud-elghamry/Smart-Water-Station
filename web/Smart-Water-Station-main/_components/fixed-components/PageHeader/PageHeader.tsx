'use client';
import { usePathname } from 'next/navigation';
import Link from 'next/link';
import { ArrowLeft, LogIn } from 'lucide-react';

const pageLabels: { [key: string]: string } = {
  '/sensors': 'Sensors',
  '/control': 'Control',
  '/ai': 'AI',
  '/prototype': 'Prototype',
  '/reports': 'Reports',
  '/alerts': 'Alerts',
};

export default function PageHeader() {
  const pathname = usePathname();
  const pageTitle = pageLabels[pathname] || 'Dashboard';

  return (
    <header className="sticky top-0 z-40 border-b border-white/20 bg-gradient-to-r from-[#4169b3] to-[#5a7ec9]/95 backdrop-blur">
      <div className="flex h-14 items-center gap-4 px-4 sm:px-6">
        <Link
          href="/"
          className="flex items-center gap-2 text-white hover:opacity-80 transition-opacity"
        >
          <ArrowLeft className="h-4 w-4" />
          <span className="text-sm font-medium hidden sm:inline">Home</span>
        </Link>
        
        <div className="flex-1">
          <h1 className="text-xl sm:text-2xl font-bold text-white">{pageTitle}</h1>
        </div>

        <Link
          href="/login"
          className="flex items-center gap-2 text-white hover:opacity-80 transition-opacity ml-auto"
        >
          <span className="text-sm font-medium hidden sm:inline">Login</span>
          <LogIn className="h-4 w-4" />
        </Link>
      </div>
    </header>
  );
}
