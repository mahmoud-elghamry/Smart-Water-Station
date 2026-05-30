import js from "@eslint/js";
import { defineConfig, globalIgnores } from "eslint/config";

const eslintConfig = defineConfig([
  {
    ...js.configs.recommended,
    files: ["**/*.js", "**/*.mjs", "**/*.cjs"],
    languageOptions: {
      globals: {
        __dirname: "readonly",
        console: "readonly",
        require: "readonly",
      },
    },
  },
  globalIgnores([
    "dist/**",
    "node_modules/**",
    "next.config.ts",
    "**/*.ts",
    "**/*.tsx",
  ]),
]);

export default eslintConfig;
