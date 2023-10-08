"""
A simple thank you
"""
import toga
from toga.style import Pack
from toga.style.pack import COLUMN, ROW


class SimpleThanks(toga.App):
    def startup(self):
        """
        Construct and show the Toga application.

        Usually, you would add your application to a main content box.
        We then create a main window (with a name matching the app), and
        show the main window.
        """
        main_box = toga.Box()

        # widgets
        thxtext = toga.Label(text="Thank you, ...")
        birthdaybtn = toga.Button(text="Birthday", on_press=self.pressed_birthdaybtn)
        mothersdaybtn = toga.Button(text="Mothers Day", on_press=self.pressed_mothersdaybtn)
        fathersdaybtn = toga.Button(text="Fathers day", on_press=self.pressed_fathersdaybtn)

        # add
        main_box.add(thxtext)
        main_box.add(birthdaybtn)
        main_box.add(mothersdaybtn)
        main_box.add(fathersdaybtn)

        self.main_window = toga.MainWindow(title=self.formal_name)
        self.main_window.content = main_box
        self.main_window.show()

    def pressed_birthdaybtn(self, widget):
        pass

    def pressed_mothersdaybtn(self, widget):
        pass

    def pressed_fathersdaybtn(self, widget):
        pass


def main():
    return SimpleThanks()
