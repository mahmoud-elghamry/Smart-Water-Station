const { mkdirSync, readFileSync, readdirSync, rmSync, statSync, writeFileSync } = require("node:fs");
const path = require("node:path");
const { gzipSync } = require("node:zlib");

const frontendRoot = path.resolve(__dirname, "..");
const workspaceRoot = path.resolve(frontendRoot, "..", "..");
const distDir = path.join(frontendRoot, "dist");
const dataDir = path.join(workspaceRoot, "firmware", "MTU", "data");
const compressible = new Set([".html", ".css", ".js", ".json", ".svg", ".txt"]);

function copyTree(sourceDir, destinationDir) {
  mkdirSync(destinationDir, { recursive: true });

  for (const entry of readdirSync(sourceDir)) {
    const sourcePath = path.join(sourceDir, entry);
    const destinationPath = path.join(destinationDir, entry);
    const info = statSync(sourcePath);

    if (info.isDirectory()) {
      copyTree(sourcePath, destinationPath);
      continue;
    }

    writeFileSync(destinationPath, readFileSync(sourcePath));
  }
}

function gzipFile(filePath) {
  writeFileSync(`${filePath}.gz`, gzipSync(readFileSync(filePath), { level: 9 }));
}

function walk(dir) {
  for (const entry of readdirSync(dir)) {
    const fullPath = path.join(dir, entry);
    const info = statSync(fullPath);

    if (info.isDirectory()) {
      walk(fullPath);
      continue;
    }

    if (compressible.has(path.extname(fullPath))) {
      gzipFile(fullPath);
    }
  }
}

rmSync(dataDir, { recursive: true, force: true });
copyTree(distDir, dataDir);
walk(dataDir);
console.log(`Copied Vite dist to ${dataDir}`);
