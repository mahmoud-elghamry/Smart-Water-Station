'use client';
import Navbar from '@/_components/fixed-components/NavBar/navbar';
import Footer from '@/_components/fixed-components/footer/footer';

interface LayoutClientProps {
  children: React.ReactNode;
}

export default function LayoutClient({ children }: LayoutClientProps) {
  return (
    <div className="flex flex-col min-h-screen">
      <Navbar />
      <main className="flex-1">
        {children}
      </main>
      <Footer />
    </div>
  );
}
