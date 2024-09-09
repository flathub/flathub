# Text adventure

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
