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

# Items
class Item:
    def __init__(self, number, name, itemdesc, updroomdesc, portable, revealsitem, usedin, usedesc, removesroomitem, addsroomitem, useroomdesc, disposable):
        self.number = number
        self.name = name
        self.itemdesc = itemdesc
        self.updroomdesc = updroomdesc
        self.portable = portable
        self.revealsitem = revealsitem
        self.usedin = usedin
        self.removesroomitem = removesroomitem
        self.addsroomitem = addsroomitem
        self.useroomdesc = useroomdesc
        self.disposable = disposable
        self.usedesc = usedesc


class StatItem(Item):
    def __init__(self, number, name, itemdesc, updroomdesc, portable, revealsitem, usedin, usedesc, removesroomitem, addsroomitem, useroomdesc, disposable, hp_change):
        super().__init__(number, name, itemdesc, updroomdesc, portable, revealsitem, usedin, usedesc, removesroomitem, addsroomitem, useroomdesc, disposable)
        self.hp_change = hp_change


class Weapon(Item):
    def __init__(self, number, name, itemdesc, updroomdesc, portable, revealsitem, usedin, usedesc, removesroomitem, addsroomitem, useroomdesc, disposable, damage):
        super().__init__(number, name, itemdesc, updroomdesc, portable, revealsitem, usedin, usedesc, removesroomitem, addsroomitem, useroomdesc, disposable)
        self.damage = damage
        #self.sound_path = sound_path
