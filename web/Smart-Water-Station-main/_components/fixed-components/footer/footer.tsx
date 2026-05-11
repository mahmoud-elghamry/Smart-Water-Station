"use client"
import Link from "next/link";
import React, { Suspense, lazy, useState } from "react";

const DrawerDemo = lazy(() =>
  import("@/app/(pages)/contact/page").then((module) => ({ default: module.DrawerDemo }))
);

export default function Footer() {
  const [open, setOpen] = useState(false)

  return (
    <footer className="w-full bg-blue-950 rounded-base shadow-xs border border-default pt-6">
      <div className="w-full max-w-screen-xl mx-auto p-4 md:py-8">
        <div className="sm:flex sm:items-center sm:justify-between">
          <Link href="/" className="flex items-center mb-4 sm:mb-0">
            <span className="text-heading self-center text-2xl font-semibold whitespace-nowrap">
              SmartWater
            </span>
          </Link>

          <ul className="flex flex-wrap items-center mb-6 text-sm font-medium text-body sm:mb-0">
            <li>
              <Link href="/about" className="hover:underline me-4 md:me-6">About</Link>
            </li>
            <li>
              <Link href="#" className="hover:underline me-4 md:me-6">Privacy Policy</Link>
            </li>
            <li>
              <Link href="#" className="hover:underline me-4 md:me-6">Licensing</Link>
            </li>
            <li>
              <button
                onClick={() => setOpen(true)}
                className="hover:underline"
              >
                Contact
              </button>
            </li>
          </ul>
        </div>

        <hr className="my-6 border-default sm:mx-auto lg:my-8" />

        <span className="block text-sm text-body sm:text-center">
          © 2026{" "}
          <Link href="/" className="hover:underline">SmartWater™</Link>
          . All Rights Reserved.
        </span>
      </div>

      {open && (
        <Suspense fallback={null}>
          <DrawerDemo open={open} onOpenChange={setOpen} />
        </Suspense>
      )}
    </footer>
  );
}
