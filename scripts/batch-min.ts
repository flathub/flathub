for (const file of Deno.readDirSync("./registries.org")) {
  await new Deno.Command("deno", {
    args: [
      "-RW=.",
      import.meta.resolve("./minimize-registry.ts"),
      `registries.org/${file.name}`,
      prompt(`> ${file.name} version: `)!,
    ],
  }).output()
    .then((out) => Deno.writeFileSync(`./registries/${file.name}`, out.stdout));
}

await new Deno.Command("deno", { args: ["fmt", "./registries"] }).spawn()
  .status;
