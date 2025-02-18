const file = Deno.args[0];
const version = Deno.args[1];
const registry = JSON.parse(Deno.readTextFileSync(file));
const data = {
  name: registry.name,
  "dist-tags": {},
  versions: {
    [version]: registry.versions[version],
  },
};

console.log(JSON.stringify(data));

function pkgName(name: string): string {
  return name.includes("/") ? name.split("/")[1] : name;
}
