'use client';
import { usePathname } from 'next/navigation';
import Navbar from '@/_components/fixed-components/NavBar/navbar';
import Footer from '@/_components/fixed-components/footer/footer';

export default function ClientLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  const pathname = usePathname();
  const isHomePage = pathname === '/';
  const isLoginPage = pathname === '/login';
  const showNavbarAndFooter = isHomePage || isLoginPage;

  return (
    <>
      {showNavbarAndFooter && <Navbar />}
      {children}
      {showNavbarAndFooter && <Footer />}
    </>
  );
}
