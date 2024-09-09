# Text adventure

# Main script
from playFunctions import *
import utils
import map
import player

import os
import sys
import time

def play(name):    
    my_map = map.Map(name)
    
    my_player = player.Player(player_name, my_map.rooms[0], 10, [])
    
    utils.output(my_map.opening_text, "bold_pink", 0.03)
    
    time.sleep(0.5)


    while True:
        utils.output("\n", "clear")
        checkhp(my_player, my_map)
        utils.output(my_player.currentroom.name, "bright_cyan")
        utils.output(my_player.currentroom.description, "clear")
        listroomitems(my_player)
        listenemies(my_player)
        listexits(my_player)
        
        print(utils.colourify("magenta"))
        action_input = input(" > ")
        print(utils.colourify("clear"))
    
        if action_input.lower().startswith("take "):
            item_name = action_input[5:]
            trytotake(item_name, my_player)
        elif action_input.lower() == "inventory":
            listinventory(my_player)
        elif action_input.lower().startswith("look "):
            item_name = action_input[5:]
            lookat(item_name, my_player)
        elif action_input.lower().startswith("use "):
            item_name = action_input[4:]
            trytouse(item_name, my_player, my_map)
        elif action_input.lower().startswith("fight "):
            enemy_name = action_input[6:]
            fight(enemy_name, my_player, my_map)
        elif action_input.lower().startswith("move "):
            trytomove(action_input.upper()[5:], my_player)
        elif action_input.lower().startswith("go "):
            trytomove(action_input.upper()[3:], my_player)
        elif action_input.lower().startswith("quit"):
            exit()
        elif action_input.lower().startswith("tutorial"):
            tutorial()
        elif action_input.lower().startswith("cast "):
            castspell(action_input.lower()[5:], my_player, my_map)
        elif action_input.lower() == "next" or action_input.lower() == 'n':
            my_map.next_level()
        elif action_input.lower().startswith("t "):
            item_name = action_input[2:]
            trytotake(item_name, my_player)
        elif action_input.lower() == "i":
            listinventory(my_player)
        elif action_input.lower().startswith("l "):
            item_name = action_input[2:]
            lookat(item_name, my_player)
        elif action_input.lower().startswith("u "):
            item_name = action_input[2:]
            trytouse(item_name, my_player, my_map)
        elif action_input.lower().startswith("f "):
            enemy_name = action_input[2:]
            fight(enemy_name, my_player, my_map)
        elif action_input.lower().startswith("m "):
            trytomove(action_input.upper()[2:], my_player)
        elif action_input.lower() == 'q':
            exit()
        elif action_input.lower().startswith('c '):
            castspell(action_input.lower()[2:], my_player, my_map)
        else:
            utils.output("You can't do that.", "magenta")

if __name__ == "__main__":
    play(input("Which map do you want to play? "))
