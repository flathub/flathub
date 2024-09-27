# window.py
#
# Copyright 2024 philipp
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
# SPDX-License-Identifier: GPL-2.0-or-later

from gi.repository import Adw
from gi.repository import Gtk
from gi.repository import Gdk

class BmiWindow(Adw.ApplicationWindow):
    __gtype_name__ = 'BmiWindow'

    label = Gtk.Template.Child()

    def __init__(self, **kwargs):
        super().__init__(**kwargs)


        # Basic properties
        self.set_title("BMI")
        self.set_default_size(600, 290)
        #self.set_size_request(440, 240)
        self.set_resizable(False)

        # Window structure
        self.content = Adw.ToolbarView()
        self.set_content(self.content)

        # Headerbar
        self.header = Adw.HeaderBar()
        self.content.add_top_bar(self.header)

        self.about_button = Gtk.Button()
        self.about_button.set_tooltip_text("Show About")
        self.about_button.set_icon_name("help-about-symbolic")
        self.about_button.connect('clicked', self.show_about)
        self.header.pack_start(self.about_button)

        # Main box
        self.drag = Gtk.WindowHandle()
        self.content.set_content(self.drag)
        self.toast_overlay = Adw.ToastOverlay()
        self.drag.set_child(self.toast_overlay)
        self.main_box = Gtk.Box(valign=Gtk.Align.CENTER, spacing=20)
        self.main_box.set_margin_bottom(28)
        self.main_box.set_margin_start(16)
        self.main_box.set_margin_end(30)
        self.toast_overlay.set_child(self.main_box)

        # User inputs
        self.left_page = Adw.PreferencesPage(halign=Gtk.Align.FILL, valign=Gtk.Align.CENTER)
        self.left_page.set_hexpand(True)
        self.left_page.set_vexpand(True)
        self.left_page.set_size_request(230, 0)
        self.main_box.append(self.left_page)

        self.left_group = Adw.PreferencesGroup()
        self.left_page.add(self.left_group)

        self.height_adjustment = Adw.SpinRow()
        self.height_adjustment.set_digits(1)
        adjustment = Gtk.Adjustment(lower= 50, upper=267, step_increment=1, page_increment=10, value=175)
        self.height_adjustment.props.adjustment = adjustment
        self.height_adjustment.props.title = "CM"
        self.height_adjustment.connect('changed', self.on_value_changed)
        self.left_group.add(self.height_adjustment)

        self.weight_adjustment = Adw.SpinRow()
        self.weight_adjustment.set_digits(1)
        adjustment = Gtk.Adjustment(lower= 10, upper=650, step_increment=1, page_increment=10, value=65)
        self.weight_adjustment.props.adjustment = adjustment
        self.weight_adjustment.props.title = "KG"
        self.weight_adjustment.connect('changed', self.on_value_changed)
        self.left_group.add(self.weight_adjustment)

        # Icon
        self.center_box = Gtk.Box(halign=Gtk.Align.CENTER, valign=Gtk.Align.CENTER)
        self.main_box.append(self.center_box)

        self.icon = Gtk.Image()
        self.icon.props.icon_name = "go-next-symbolic"
        self.icon.set_pixel_size(42)
        self.center_box.append(self.icon)

        # Results
        self.right_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, halign=Gtk.Align.CENTER, valign=Gtk.Align.CENTER, spacing=6)
        self.right_box.set_hexpand(True)
        self.right_box.set_vexpand(True)
        self.right_box.set_size_request(175, 0)
        self.main_box.append(self.right_box)

        self.result_label = Gtk.Label()
        self.result_label.set_css_classes(["title-2"])
        self.result_label.set_label("BMI:")
        self.right_box.append(self.result_label)

        self.calc_bmi()

        self.result_button = Gtk.Button(halign=Gtk.Align.CENTER)
        self.result_button.set_tooltip_text("Copy BMI")
        self.result_button.set_css_classes(["pill", "title-1"])
        self.result_button.set_label(str(self.bmi))
        self.result_button.connect('clicked', self.on_result_button_pressed)
        self.right_box.append(self.result_button)

        self.result_feedback_label = Gtk.Label()
        self.result_feedback_label.set_css_classes(["title-2", "success"])
        self.result_feedback_label.set_label("Healthy")
        self.right_box.append(self.result_feedback_label)

    def on_result_button_pressed(self, _button):
        clipboard = Gdk.Display.get_default().get_clipboard()
        Gdk.Clipboard.set(clipboard, self.bmi);
        self.toast = Adw.Toast()
        self.toast.set_title("Result copied")
        self.toast_overlay.add_toast(self.toast)

    def calc_bmi(self):
        self.bmi = self.height_adjustment.get_value() / 100 # converting cm to meters
        self.bmi = self.bmi ** 2
        self.bmi = self.weight_adjustment.get_value() / self.bmi
        self.bmi = "%.0f" % self.bmi # removing numbers after decimal point

    def on_value_changed(self, _scroll):
        self.calc_bmi()

        self.result_button.set_label(self.bmi)
        if int(self.bmi) < 18:
            self.result_feedback_label.set_css_classes(["title-2", "accent"])
            self.result_feedback_label.set_label("Underweight")
        elif int(self.bmi) > 18  and int(self.bmi) < 24:
            self.result_feedback_label.set_css_classes(["title-2", "success"])
            self.result_feedback_label.set_label("Healthy")
        elif int(self.bmi) > 24 and int(self.bmi) < 29:
            self.result_feedback_label.set_css_classes(["title-2", "warning"])
            self.result_feedback_label.set_label("Overweight")
        elif int(self.bmi) > 29 and int(self.bmi) < 39:
            self.result_feedback_label.set_css_classes(["title-2", "error"])
            self.result_feedback_label.set_label("Obese")
        elif int(self.bmi) > 39:
            self.result_feedback_label.set_css_classes(["title-2"])
            self.result_feedback_label.set_label("Extremely obese")

        self.height_adjustment.set_title("CM")
        self.weight_adjustment.set_title("KG")
        if self.height_adjustment.get_value() == 267:
            self.height_adjustment.set_title("Robert Wadlow")
        if self.weight_adjustment.get_value() == 650:
            self.weight_adjustment.set_title("Jon Brower Minnoch")

    def show_about(self, _button):
        self.about = Adw.AboutWindow(application_name='BMI',
                                application_icon='com.github.philippkosarev.bmi',
                                developer_name='Philipp Kosarev',
                                version='1.0',
                                developers=['Philipp Kosarev'],
                                artists=['Philipp Kosarev'],
                                copyright='© 2024 Philipp Kosarev',
                                license_type="GTK_LICENSE_GPL_2_0",
                                website="https://github.com/philippkosarev/bmi",
                                issue_url="https://github.com/philippkosarev/bmi/issues")
        self.about.present()
