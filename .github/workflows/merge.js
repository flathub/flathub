// Note: This is a GitHub Actions script
// It is not meant to be executed directly on your machine without modifications

const fs = require("fs");

const check_query = `query($name: String!){
  repository(name: $name, owner: "flathub") {
    pullRequests(states: OPEN, first: 50, baseRefName: "master", orderBy: {field: CREATED_AT, direction: DESC}) {
      nodes {
        id number headRefName mergeable
        author { login }
      }
    }
  }
}`;

async function closePullRequests(should_close, github) {
  if (should_close.length > 0) {
    console.log("Closing other pull requests ...");
    let mut = "mutation cpm {";
    for (let i = 0; i < should_close.length; i++) {
      mut += `  _${i}: closePullRequest(input: {pullRequestId: "${should_close[i].id}"}) { clientMutationId }`;
      mut += "\n";
    }
    mut += "}\n";
    await github.graphql(mut);
    console.log("Pull requests closed.");
  }
}

async function incrementVersion() {
  const manifest = fs.readFileSync("org.lemonade_emu.lemonade.json", {
    encoding: "utf8",
  });
  const version = /"url": .+?download\/nightly-(\d+)/.exec(manifest)[1];
  const new_manifest = manifest.replace(/nightly-[0-9]+/, `nightly-${version}`);
  fs.writeFileSync("org.lemonade_emu.lemonade.json", new_manifest);
}

async function mergeChanges(branch, execa) {
  async function amendCommit() {
    await execa("git", ["config", "--global", "user.name", "lemonadebot"]);
    await execa("git", [
      "config",
      "--global",
      "user.email",
      "lemonade\x40lemonade-emu\x2eorg", // prevent email harvesters from scraping the address
    ]);
    await execa("git", ["add", "org.lemonade_emu.lemonade.json"]);
    // amend the commit to include the version change
    const p1 = execa("git", ["commit", "--amend", "-C", "HEAD"]);
    p1.stdout.pipe(process.stdout);
    await p1;
  }

  async function hasChanges() {
    const p = await execa("git", [
      "status",
      "--porcelain",
      "org.lemonade_emu.lemonade.json",
    ]);
    if (p.stdout.length > 20) {
      console.info("Version was bumped by this script.");
      return true;
    }
    return false;
  }

  try {
    const p = execa("git", ["merge", "--ff-only", `origin/${branch}`]);
    p.stdout.pipe(process.stdout);
    await p;
    // bump the version number
    await incrementVersion();
    if (await hasChanges()) await amendCommit();
  } catch (err) {
    console.log(
      `::error title=Merge failed::Failed to merge pull request: ${err}`
    );
    return;
  }

  const p = execa("git", ["push", "origin", `master:${branch}`, "-f"]);
  p.stdout.pipe(process.stdout);
  await p;
  await new Promise((r) => setTimeout(r, 2000));
  // wait a while for GitHub to process the pull request update
  const p1 = execa("git", ["push", "origin"]);
  p1.stdout.pipe(process.stdout);
  await p1;
}

async function checkChanges(github, context) {
  const variables = {
    name: context.repo.repo,
  };
  const result = await github.graphql(check_query, variables);
  const prs = result.repository.pullRequests.nodes;
  const auto_prs = prs.filter(
    (pr) => pr.author.login === "flathubbot" && pr.mergeable === "MERGEABLE"
  );
  if (auto_prs.length < 1) {
    console.warn("No pull requests available for merge.");
    return null;
  }
  const chosen = auto_prs[0];
  const should_close = auto_prs.slice(1);
  console.log(`Selected pull request: #${chosen.number}`);
  await closePullRequests(should_close, github);
  return chosen.headRefName;
}

module.exports.checkChanges = checkChanges;
module.exports.mergeChanges = mergeChanges;
