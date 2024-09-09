import config
import utils

import os
import json

utils.output("Welcome to Scripted Journeys, an enthralling text-based adventure game where every decision shapes the narrative, and the wrong path could lead to death.", "bold_pink", 0.03)

utils.output("What is your name, brave adventurer?", "magenta")
print(utils.colourify("magenta"))
name = input(" > ")
print(utils.colourify("clear"))
utils.output(f"Greetings {name}!\n\n", "magenta")

playerdata = {"name": name}

with open(os.path.join(config.playerdata_path, "playerdata.json"), 'w') as file:
    json.dump(playerdata, file)

