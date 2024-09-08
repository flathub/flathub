# ScriptedJourneys

We are now in beta! If you want to test the game, please do so and file any bugs before 08/09/2024 - the release date!

Welcome to Scripted Journeys, an enthralling text-based adventure where your decisions shape the narrative and uncover hidden mysteries across diverse realms. Each map teems with unique challenges, intricate plots, and fascinating characters, all waiting for you to explore and interact with. Your choices will determine your path, unlocking secrets, and altering the course of your journey in unexpected ways. Embark on a quest that combines storytelling, strategy, and imagination, where every script you write crafts your destiny. Are you ready to dive into a world where every decision is a step towards a new adventure?

## Getting Started

### Prerequisites
- Nothing!

### Installation
1. Download the Flatpak:
```bash
curl https://github.com/MrPiggy92/ScriptedJourneys/raw/master/ScriptedJourneys.flatpak
```
2. Install the Flatpak:
```bash
flatpak --user install ScriptedJourneys.flatpak
```
3. Run the Flatpak:
```bash
flatpak run io.github.MrPiggy92.ScriptedJourneys
``` 
Or you can use the applications menu
We are working on a Flathub submission, so it should be coming shortly.


## Gameplay

### Overview
This game is a multi-map text-based adventure where you navigate through various scenarios, make decisions, and solve puzzles to progress. Each choice you make influences the outcome of the game.

### Controls
- Use text commands to interact with the game.
- Navigate through different locations using `move <direction>`
- Look at items using `look <item>`
- Take items using `take <item>`
- Use items using `use <item>`
- Fight enemies using `fight <enemy>`
- Check your inventory using `inventory`
- Cast spells using `cast <spell>`

### Features
- **Multiple Maps:** Explore different environments such as dungeons, forests, and towns.
- **Item Management:** Collect and use items to overcome obstacles.
- **Mini-bosses:** Fight small bosses as you work your way up the levels.
- **Spells:** Cast powerful spells to do everything from instantly killing an enemy to teleporting through the map.

## Contributing

We welcome contributions to improve the game! To contribute:
1. Fork the repository and clone it locally.
2. Create a new branch for your feature (`git checkout -b feature/new-feature`).
3. Commit your changes (`git commit -am 'Add new feature'`).
4. Push to the branch (`git push origin feature/new-feature`).
5. Create a new Pull Request.

## Authors

- [MrPiggy92](https://github.com/MrPiggy92) - Most work
- [Linux Format](https://linuxformat.com) - Inspiration and some basic functions

## License

This project is licensed under the [GNU GPL License](LICENSE).
