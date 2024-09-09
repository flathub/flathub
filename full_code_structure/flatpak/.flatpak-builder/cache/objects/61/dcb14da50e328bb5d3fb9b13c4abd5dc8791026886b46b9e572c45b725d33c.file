# Scripted Journeys

#
# Copyright (C) 2024 MrPiggy92
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.
#

# Map
import rooms as roomsObject
import items
import enemy as enemyObject
import spell as spellObject
import utils
import playFunctions
import config

import xml.etree.ElementTree as ET
import os
import time

class Map:
    def __init__(self, folder):
        self.folder = os.path.join(config.maps_path, folder)
        self.all_levels = len(os.listdir(self.folder)) - 1
        self.level = 1
        self.name, self.items, self.rooms, self.enemies, self.spells = self.load()
        with open(os.path.join(self.folder, "open.txt")) as txt:
            self.opening_text = txt.read()
        self.bossDefeated = False
    
    def next_level(self, player):
        self.level += 1
        if self.level > self.all_levels:
            utils.output(f"\n\nYou have completed {self.name}!\n\n", "bright_green")
            time.sleep(2)
            raise SystemExit()
        utils.output(f"\n\nLevel {self.level}\n\n", "bright_green")
        self.name, self.items, self.rooms, self.enemies, self.spells = self.load()
        player.currentroom = self.rooms[0]
    
    def load(self):
        tree = ET.parse(os.path.join(self.folder, f"lvl{self.level}.xml"))
        #tree = ET.parse(os.path.join(self.folder, f"lvl{self.level}.xml"))
        root = tree.getroot()
        
        name = list(root.iter("mapname"))[0]
        
        mapitems = []
        for item in root.iter("item"):
            itemdata = []
            for data in item:
                if data.text != "-1":
                    try:
                        if data.attrib["type"] == "weapon":
                            data.text = "weapon" + data.text
                        elif data.attrib["type"] == "statitem":
                            data.text = "statitem" + data.text
                        else:
                            data.text = "item" + data.text
                    except:
                        pass
                try:
                    data.text = int(data.text)
                except:
                    pass
                itemdata.append(None if data.text == -1 else data.text)
                #print(None if data.text == -1 else data.text)
            #print(itemdata)
            mapitems.append(items.Item(itemdata[0], itemdata[1], itemdata[2], itemdata[3], itemdata[4], itemdata[5], itemdata[6], itemdata[7], itemdata[8], itemdata[9], itemdata[10], itemdata[11]))
        
        
        mapstatitems = []
        for statitem in root.iter("statitem"):
            statitemdata = []
            for data in statitem:
                if data.text != "-1":
                    try:
                        if data.attrib["type"] == "weapon":
                            data.text = "weapon" + data.text
                        elif data.attrib["type"] == "statitem":
                            data.text = "statitem" + data.text
                        else:
                            data.text = "item" + data.text
                    except:
                        pass
                try:
                    data.text = int(data.text)
                except:
                    pass
                statitemdata.append(None if data.text == -1 else data.text)
                #print(None if data.text == -1 else data.text)
            #print(statitemdata)
            mapstatitems.append(items.StatItem(statitemdata[0], statitemdata[1], statitemdata[2], statitemdata[3], statitemdata[4], statitemdata[5], statitemdata[6], statitemdata[7], statitemdata[8], statitemdata[9], statitemdata[10], statitemdata[11], statitemdata[12]))
        
        mapweapons = []
        for weapon in root.iter("weapon"):
            weapondata = []
            for data in weapon:
                if data.text != "-1":
                    try:
                        if data.attrib["type"] == "weapon":
                            data.text = "weapon" + data.text
                        elif data.attrib["type"] == "statitem":
                            data.text = "statitem" + data.text
                        else:
                            data.text = "item" + data.text
                    except:
                        pass
                try:
                    data.text = int(data.text)
                except:
                    pass
                weapondata.append(None if data.text == -1 else data.text)
                #print(None if data.text == -1 else data.text)
            #print(weapondata)
            #print(len(weapondata))
            mapweapons.append(items.Weapon(weapondata[0], weapondata[1], weapondata[2], weapondata[3], weapondata[4], weapondata[5], weapondata[6], weapondata[7], weapondata[8], weapondata[9], weapondata[10], weapondata[11], weapondata[12]))
        
        for item in mapitems:
            if item.revealsitem != None:
                if item.revealsitem.startswith("i"):
                    item.revealsitem = mapitems[int(item.revealsitem[4:])]
                elif item.revealsitem.startswith("s"):
                    item.revealsitem = mapstatitems[int(item.revealsitem[8:])]
                else:
                    item.revealsitem = mapweapons[int(item.revealsitem[6:])]
            if item.addsroomitem != None:
                if item.addsroomitem.startswith("i"):
                    item.addsroomitem = mapitems[int(item.addsroomitem[4:])]
                elif item.addsroomitem.startswith("s"):
                    item.addsroomitem = mapstatitems[int(item.addsroomitem[8:])]
                else:
                    item.addsroomitem = mapweapons[int(item.addsroomitem[6:])]
            if item.removesroomitem != None:
                if item.removesroomitem.startswith("i"):
                    item.removesroomitem = mapitems[int(item.removesroomitem[4:])]
                elif item.removesroomitem.startswith("s"):
                    item.removesroomitem = mapstatitems[int(item.removesroomitem[8:])]
                else:
                    item.removesroomitem = mapweapons[int(item.removesroomitem[6:])]
        
        for item in mapstatitems:
            if item.revealsitem != None:
                if item.revealsitem.startswith("i"):
                    item.revealsitem = mapitems[int(item.revealsitem[4:])]
                elif item.revealsitem.startswith("s"):
                    item.revealsitem = mapstatitems[int(item.revealsitem[8:])]
                else:
                    item.revealsitem = mapweapons[int(item.revealsitem[6:])]
            if item.addsroomitem != None:
                if item.addsroomitem.startswith("i"):
                    item.addsroomitem = mapitems[int(item.addsroomitem[4:])]
                elif item.addsroomitem.startswith("s"):
                    item.addsroomitem = mapstatitems[int(item.addsroomitem[8:])]
                else:
                    item.addsroomitem = mapweapons[int(item.addsroomitem[6:])]
            if item.removesroomitem != None:
                if item.removesroomitem.startswith("i"):
                    item.removesroomitem = mapitems[int(item.removesroomitem[4:])]
                elif item.removesroomitem.startswith("s"):
                    item.removesroomitem = mapstatitems[int(item.removesroomitem[8:])]
                else:
                    item.removesroomitem = mapweapons[int(item.removesroomitem[6:])]
        
        for item in mapweapons:
            if item.revealsitem != None:
                if item.revealsitem.startswith("i"):
                    item.revealsitem = mapitems[int(item.revealsitem[4:])]
                elif item.revealsitem.startswith("s"):
                    item.revealsitem = mapstatitems[int(item.revealsitem[8:])]
                else:
                    item.revealsitem = mapweapons[int(item.revealsitem[6:])]
            if item.addsroomitem != None:
                if item.addsroomitem.startswith("i"):
                    item.addsroomitem = mapitems[int(item.addsroomitem[4:])]
                elif item.addsroomitem.startswith("s"):
                    item.addsroomitem = mapstatitems[int(item.addsroomitem[8:])]
                else:
                    item.addsroomitem = mapweapons[int(item.addsroomitem[6:])]
            if item.removesroomitem != None:
                if item.removesroomitem.startswith("i"):
                    item.removesroomitem = mapitems[int(item.removesroomitem[4:])]
                elif item.removesroomitem.startswith("s"):
                    item.removesroomitem = mapstatitems[int(item.removesroomitem[8:])]
                else:
                    item.removesroomitem = mapweapons[int(item.removesroomitem[6:])]
        
        enemies = []
        for enemy in root.iter("enemy"):
            enemydata = []
            for data in enemy:
                if data.tag == "loot":
                    data.text = []
                    for item in data:
                        if item.tag == "lootitem":
                            data.text.append(mapitems[int(item.text)])
                try:
                    data.text = int(data.text)
                except:
                    pass
                enemydata.append(None if data.text == -1 else data.text)
                #print("hi")
            #print(len(enemydata))
            enemies.append(enemyObject.Enemy(enemydata[0], enemydata[1], True, enemydata[2], enemydata[3], mapweapons[enemydata[4]], enemydata[5], enemydata[6]))
        
        bosslist = list(root.iter("boss"))
        bossdata = []
        for data in bosslist[0]:
            if data.tag == "loot":
                data.text = []
                for item in data:
                    if item.tag == "lootitem":
                        data.text.append(mapitems[int(item.text)])
            try:
                data.text = int(data.text)
            except:
                pass
            bossdata.append(None if data.text == -1 else data.text)
        boss = enemyObject.Boss(bossdata[0], bossdata[1], True, bossdata[2], bossdata[3], mapweapons[bossdata[4]], bossdata[5], bossdata[6])
        
        
        rooms = []
        for room in root.iter("room"):
            roomdata = []
            for data in room:
                if data.tag == "items":
                    data.text = []
                    for item in data:
                        if item.tag == "roomitem":
                            if item.text != "-1":
                                data.text.append(mapitems[int(item.text)])
                        elif item.tag == "roomstatitem":
                            if item.text != "-1":
                                data.text.append(mapstatitems[int(item.text)])
                        elif item.tag == "roomweapon":
                            if item.text != "-1":
                                data.text.append(mapweapons[int(item.text)])
                        else:
                            pass
                        
                elif data.tag == "exits":
                    data.text = []
                    for exit in data:
                        data.text.append(int(exit.text))
                elif data.tag == "enemies":
                    data.text = []
                    counter = 0
                    for enemy in data:
                        if enemy.tag == "roomboss":
                            data.text.append(boss)
                        else:
                            data.text.append(enemies[int(enemy.text)] if enemy.text != "-1" else None)
                        counter += 1
                    if counter == 0:
                        data.text = None
                else:
                    try:
                        data.text = int(data.text)
                    except:
                        pass
                roomdata.append(None if data.text == -1 else data.text)
            rooms.append(roomsObject.Room(roomdata[0], roomdata[1], roomdata[2], roomdata[3], roomdata[4], roomdata[5]))
        
        #print(f"h {rooms[1].exits}")
        for room in rooms:
            room.exits[0] = rooms[int(room.exits[0])] if room.exits[0] != -1 else None
            room.exits[1] = rooms[int(room.exits[1])] if room.exits[1] != -1 else None
            room.exits[2] = rooms[int(room.exits[2])] if room.exits[2] != -1 else None
            room.exits[3] = rooms[int(room.exits[3])] if room.exits[3] != -1 else None
        #print(rooms[0].exits)
        
        spells = []
        for spell in root.iter("spell"):
            spelldata = []
            for data in spell:
                try:
                    data.text = int(data.text)
                except:
                    pass
                spelldata.append(data.text)
            spells.append(spellObject.Spell(spelldata[0], spelldata[1], spelldata[2], spelldata[3]))
        
        for spell in spells:
            if playFunctions.safetorun(spell):
                pass
            else:
                utils.output(f"Spell {spell.name} has been disabled due to a potential security hazard.", "magenta")
        
        return name, mapitems, rooms, enemies, spells
