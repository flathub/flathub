const fs = require("fs");
const path = require("path");

const root = process.argv[2] || ".";
const template = fs.readFileSync(path.join(root, "crates-tauri/yaak-app/template.desktop"), "utf-8");
const conf = JSON.parse(fs.readFileSync(path.join(root, "crates-tauri/yaak-app/tauri.conf.json"), "utf-8"));
const release = JSON.parse(fs.readFileSync(path.join(root, "crates-tauri/yaak-app/tauri.release.conf.json"), "utf-8"));

const values = {
  name: conf.productName,
  exec: conf.productName.toLowerCase() + "-app",
  icon: conf.productName.toLowerCase() + "-app",
  comment: release.bundle.shortDescription,
  categories: "Development;",
};

const output = template.replace(/\{\{(\w+)\}\}/g, (_, key) => values[key] || "");
fs.writeFileSync("yaak.desktop", output);
