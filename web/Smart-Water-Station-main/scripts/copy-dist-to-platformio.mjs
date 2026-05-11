import { createGzip } from "node:zlib";
import { createReadStream, createWriteStream } from "node:fs";
import { cp, mkdir, readdir, rm, stat } from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";

const here = path.dirname(fileURLToPath(import.meta.url));
const frontendRoot = path.resolve(here, "..");
const workspaceRoot = path.resolve(frontendRoot, "..");
const distDir = path.join(frontendRoot, "dist");
const dataDir = path.join(workspaceRoot, "data");

await rm(dataDir, { recursive: true, force: true });
await mkdir(dataDir, { recursive: true });
await cp(distDir, dataDir, { recursive: true });

const compressible = new Set([".html", ".css", ".js", ".json", ".svg", ".txt"]);

async function gzipFile(filePath) {
  await new Promise((resolve, reject) => {
    const input = createReadStream(filePath);
    const output = createWriteStream(`${filePath}.gz`);
    input.pipe(createGzip({ level: 9 })).pipe(output);
    output.on("finish", resolve);
    output.on("error", reject);
    input.on("error", reject);
  });
}

async function walk(dir) {
  const entries = await readdir(dir);
  for (const entry of entries) {
    const fullPath = path.join(dir, entry);
    const info = await stat(fullPath);
    if (info.isDirectory()) {
      await walk(fullPath);
      continue;
    }

    if (compressible.has(path.extname(fullPath))) {
      await gzipFile(fullPath);
    }
  }
}

await walk(dataDir);
console.log(`Copied Vite dist to ${dataDir}`);
