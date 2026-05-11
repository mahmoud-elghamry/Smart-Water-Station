"use client";
import logo from "../../../Project photos/logo.png"
import Image from "next/image";
import Link from "next/link";
import { usePathname } from "next/navigation";
import { Beaker, Bell, Cpu, Gauge, Home, LogIn, Microscope, Waves } from "lucide-react";

// navbar items & icons & links
const navItems = [
  { label: "Home", href: "/", icon: Home },
  { label: "Main Analytics", href: "/home", icon: Waves },
  { label: "Sensors", href: "/sensors", icon: Waves },
  { label: "Control", href: "/control", icon: Gauge },
  { label: "Ai", href: "/ai", icon: Cpu },
  { label: "Prototype", href: "/prototype", icon: Beaker },
  { label: "Reports", href: "/reports", icon: Microscope },
  { label: "Alerts", href: "/alerts", icon: Bell },
  { label: "Login", href: "/login", icon: LogIn },
];

export default function Navbar() {
  const pathname = usePathname();

  return (
    <>
      <header className="sticky top-0 z-50 border-b border-white/20 bg-[#6f89b8]/95 backdrop-blur">
        <nav className="mx-auto flex h-16 max-w-7xl items-center gap-2 px-4 sm:gap-4 sm:px-6">
          <Link href="/" className="mr-2 flex shrink-0 items-center sm:mr-4">
            <Image
              src={logo}
              alt="SmarterWater"
              width={300}
              height={200}
              className="h-24 w-auto object-contain brightness-0 invert"
              priority
            />
          </Link>

          <div className="no-scrollbar flex min-w-0 flex-1 items-center justify-start gap-2 overflow-x-auto py-2 sm:justify-center sm:gap-2 lg:gap-3">
            {navItems.map((item) => {
              const Icon = item.icon;
              const isActive = pathname === item.href;

              return (
                <Link
                  key={item.href}
                  href={item.href}
                  className={`inline-flex shrink-0 items-center gap-1 rounded-md px-2 py-1.5 text-xs sm:text-sm font-semibold transition ${
                    isActive ? "bg-blue-900 text-[#4f74b6]" : "text-white hover:bg-white/15"
                  }`}
                >
                  <Icon className="h-3.5 w-3.5" />
                  <span className="hidden sm:inline">{item.label}</span>
                </Link>
              );
            })}
          </div>
        </nav>
      </header>
    </>
  );
}
