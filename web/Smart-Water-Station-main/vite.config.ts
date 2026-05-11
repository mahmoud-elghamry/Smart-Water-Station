import path from "node:path";
import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";

export default defineConfig({
  plugins: [react()],
  resolve: {
    alias: {
      "@": path.resolve(__dirname, "."),
      "next/link": path.resolve(__dirname, "shims/next-link.tsx"),
      "next/navigation": path.resolve(__dirname, "shims/next-navigation.ts"),
      "next/image": path.resolve(__dirname, "shims/next-image.tsx"),
    },
  },
  build: {
    outDir: "dist",
    emptyOutDir: true,
    assetsInlineLimit: 0,
    rollupOptions: {
      output: {
        assetFileNames: "assets/asset-[hash][extname]",
        chunkFileNames: "assets/chunk-[hash].js",
        entryFileNames: "assets/app-[hash].js",
        manualChunks: {
          react: ["react", "react-dom"],
        },
      },
    },
  },
});
