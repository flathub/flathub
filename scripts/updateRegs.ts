for (
  const [pkgName, fileName] of Object.entries(
    JSON.parse(Deno.readTextFileSync("./scripts/_regs.json")),
  )
) {
  const newData = await fetch(`https://registry.npmjs.org/${pkgName}`).then(
    (d) => d.text(),
  );
  await Deno.writeTextFile(
    `./registries.org/${fileName}`,
    newData,
  );
  console.log(`Updated ${pkgName}`);
}
