### Please confirm your submission meets all the criteria

- [x] Please describe the application briefly.
**PoktCode** is an AI-powered desktop IDE based on Eclipse Theia (Electron). It is a full coding environment with its own branding and welcome flow, plus first-class AI tooling:

- **Providers catalog** — large directory of AI backends with filters for free, connected, API key, OAuth, IDE subscriptions, and local/self-hosted. Connect Ollama/LM Studio, OpenAI-compatible APIs, Anthropic, Google, Azure, Hugging Face, and many others; manage keys/OAuth and choose models for chat, image, and video.
- **Pokt Agent** — agent chat beside the editor with modes for ask, plan, edit, agent, debug, review, and architecture; project context, skills, and tool use.
- **MCP & tools** — configure MCP servers in-app; the agent can list and call tools without dumping everything into context. Host tooling works via Flatpak portal/`flatpak-spawn` when needed.
- **Embedded preview** — isolated browser preview for sites with multimodal screenshots the agent can use.
- **Privacy-first defaults** — telemetry off out of the box; you control which remote providers are allowed. Free/local options are first-class, not an afterthought.

Packaged with `org.electronjs.Electron2.BaseApp` // 25.08.

- [x] Please attach a video showcasing the application on Linux using the Flatpak.
https://github.com/poktweb/poktcode-releases/raw/main/videos/poktcode-0.1.0-linux-flatpak.webm

Video (~54s): Flatpak launch → welcome → Providers catalog → Pokt Agent on Linux.

- [x] The Flatpak ID follows all the rules listed in the [Application ID requirements][appid].
- [x] I have read and followed all the [Submission requirements][reqs] and the [Submission guide][reqs2] and I agree to them.
  - [x] The application has a meaningful development history, evidence of real-world use, and a clear commitment to ongoing maintenance, as required by the [development history requirements][history].
  - [x] I have disclosed any AI-generated material included in the application or its Flathub packaging, as required by the [Generative AI policy][ai]. **Affected parts and approximate extent:** AI assistance was used interactively under human review for parts of the application code and packaging docs. The Flatpak manifest/AppStream were reviewed against Electron BaseApp conventions.
  - [x] I have not used AI tools or agents to generate or automate this submission pull request or its review interactions.
- [x] I am an author/developer of the project.

### Repository

Public binaries/screenshots: https://github.com/poktweb/poktcode-releases  
(Application source remains private; Flathub builds from the published archive.)

### Domain note

`poktweb.com` is not live yet (Vercel custom domain planned). Homepage temporarily points to the releases repository — please grant an exception for `appid-url-not-reachable` until verification can be completed.

### Permission exceptions

See `SUBMISSION.md` in this branch (`filesystem=host`, `flatpak-spawn`, SSH auth, GPU/device, login1, portals) required for IDE workflows.

[appid]: https://docs.flathub.org/docs/for-app-authors/requirements#application-id
[reqs]: https://docs.flathub.org/docs/for-app-authors/requirements
[reqs2]: https://docs.flathub.org/docs/for-app-authors/submission
[history]: https://docs.flathub.org/docs/for-app-authors/requirements#insufficient-development-history
[ai]: https://docs.flathub.org/docs/for-app-authors/requirements#generative-ai-policy
