'use client';
import React from "react";
import Link from "next/link";
import { usePathname } from "next/navigation";
import {
  Bell,
  Cpu,
  Gauge,
  Microscope,
  Waves,
  Beaker,
  Home,
} from "lucide-react";
import {
  Sidebar,
  SidebarContent,
  SidebarFooter,
  SidebarGroup,
  SidebarGroupLabel,
  SidebarHeader,
  SidebarMenu,
  SidebarMenuButton,
  SidebarMenuItem,
} from "../../../_components/ui/sidebar";

const menu_items = [
  { label: "Main Analytics", href: "/home", icon: Home },
  { label: "Sensors", href: "/sensors", icon: Waves },
  { label: "Control", href: "/control", icon: Gauge },
  { label: "AI", href: "/ai", icon: Cpu },
  { label: "Prototype", href: "/prototype", icon: Beaker },
  { label: "Reports", href: "/reports", icon: Microscope },
  { label: "Alerts", href: "/alerts", icon: Bell },
];

export default function SideBar() {
  const pathname = usePathname();

  return (
    <Sidebar variant="sidebar" className="bg-gradient-to-b from-[#4169b3] to-[#2d4a7f]">
      <SidebarHeader>
        <div className="flex items-center justify-center py-4">
          <h2 className="text-xl font-bold text-white">📊 Dashboard</h2>
        </div>
      </SidebarHeader>
      <SidebarContent>
        <SidebarGroup>
          <SidebarGroupLabel className="text-white/70 text-xs font-semibold uppercase">
            Navigation
          </SidebarGroupLabel>
          <SidebarMenu className="gap-2">
            {menu_items.map((item) => {
              const Icon = item.icon;
              const isActive = pathname === item.href;

              return (
                <SidebarMenuItem key={item.label}>
                  <SidebarMenuButton
                    asChild
                    className={`rounded-lg transition-all ${
                      isActive
                        ? "bg-white/20 text-white"
                        : "text-white/80 hover:bg-white/10 hover:text-white"
                    }`}
                  >
                    <a href={item.href} className="flex items-center gap-3">
                      <Icon className="w-5 h-5" />
                      <span className="font-medium">{item.label}</span>
                    </a>
                  </SidebarMenuButton>
                </SidebarMenuItem>
              );
            })}
          </SidebarMenu>
        </SidebarGroup>
      </SidebarContent>
      <SidebarFooter>
        <div className="pt-4 border-t border-white/20">
          <button className="w-full rounded-lg bg-white/10 px-4 py-2 text-center font-medium text-white transition-all hover:bg-white/20">
            Log Out
          </button>
        </div>
      </SidebarFooter>
    </Sidebar>
  );
}
