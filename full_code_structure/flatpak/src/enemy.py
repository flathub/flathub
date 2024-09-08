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

# Enemy
class Enemy:
    def __init__(self, num, name, alive, description, deaddesc, weapon, hp, loot):
        self.num = num
        self.name = name
        self.alive = alive
        self.description = description
        self.deaddesc = deaddesc
        self.weapon = weapon
        self.hp = hp
        self.loot = loot

class Boss(Enemy):
    def __init__(self, num, name, alive, description, deaddesc, weapon, hp, loot):
        super().__init__(num, name, alive, description, deaddesc, weapon, hp, loot)
        
