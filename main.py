#!/usr/bin/python3

import dbus # type: ignore
import gi   # type: ignore
gi.require_version("Gtk", "4.0")
gi.require_version("Adw", "1")
from gi.repository import Gtk, Adw, GLib, Gio # type: ignore
import math

bus = dbus.SystemBus()
bat_obj = bus.get_object('org.freedesktop.UPower', '/org/freedesktop/UPower/devices/battery_BAT0')
props = dbus.Interface(bat_obj, 'org.freedesktop.DBus.Properties')
iface = dbus.Interface(bat_obj, 'org.freedesktop.UPower.Device')

def get_history(seconds):
    raw = iface.GetHistory('charge', seconds, 500)
    return [(int(t), float(v), int(s)) for t, v, s in raw]

def get_prop(name):
    return props.Get('org.freedesktop.UPower.Device', name)

TIME_SPANS = {
    "1 hour":   60 * 60,
    "6 hours":  60 * 60 * 6,
    "24 hours": 60 * 60 * 24,
    "7 days":   60 * 60 * 24 * 7,
}


class BatteryChart(Gtk.DrawingArea):
    def __init__(self):
        super().__init__()
        self._period_seconds = 60 * 60
        self._points = []
        self.set_draw_func(self._draw)
        self.set_hexpand(True)
        self.set_vexpand(True)
        self.set_size_request(-1, 250)

        mc = Gtk.EventControllerMotion()
        mc.connect('motion', self._on_motion)
        mc.connect('leave',  self._on_leave)
        self.add_controller(mc)
        self._hover_x = -1

    def set_period(self, seconds):
        self._period_seconds = seconds
        self.refresh()

    def refresh(self):
        self._points = [(t, v) for t, v, s in get_history(self._period_seconds)]
        self.queue_draw()

    def _on_motion(self, ctrl, x, y):
        self._hover_x = x
        self.queue_draw()

    def _on_leave(self, ctrl):
        self._hover_x = -1
        self.queue_draw()

    def _get_accent(self):
        rgba = Adw.StyleManager.get_default().get_accent_color().to_rgba()
        return rgba.red, rgba.green, rgba.blue

    def _draw(self, area, cr, w, h):
        import time
        PAD_L, PAD_R, PAD_T, PAD_B = 48, 16, 16, 32
        cw = w - PAD_L - PAD_R
        ch = h - PAD_T - PAD_B
        points = self._points
        ar, ag, ab = self._get_accent()

        now = int(time.time())
        t_min = now - self._period_seconds
        t_max = now

        def px(t): return PAD_L + (t - t_min) / (t_max - t_min) * cw
        def py(v): return PAD_T + ch - (v / 100) * ch

        cr.set_source_rgba(0.5, 0.5, 0.5, 0.15)
        cr.set_line_width(1)
        for pct in [0, 25, 50, 75, 100]:
            y = PAD_T + ch - (pct / 100) * ch
            cr.set_dash([4, 6])
            cr.move_to(PAD_L, y)
            cr.line_to(w - PAD_R, y)
            cr.stroke()
            cr.set_dash([])
            cr.set_source_rgba(0.5, 0.5, 0.5, 0.5)
            cr.select_font_face('monospace', 0, 0)
            cr.set_font_size(10)
            cr.move_to(4, y + 4)
            cr.show_text(f"{pct}%")
            cr.set_source_rgba(0.5, 0.5, 0.5, 0.15)

        if len(points) < 2:
            cr.set_source_rgba(0.5, 0.5, 0.5, 0.4)
            cr.select_font_face('sans-serif', 0, 0)
            cr.set_font_size(13)
            msg = "Not enough data for this period"
            te = cr.text_extents(msg)
            cr.move_to((w - te[2]) / 2, h / 2)
            cr.show_text(msg)
            return

        from cairo import LinearGradient

        # Spezza i punti in segmenti contigui (gap > soglia = buco)
        gap_threshold = self._period_seconds / 10
        segments = []
        current = [points[0]]
        for i in range(1, len(points)):
            if points[i][0] - points[i-1][0] > gap_threshold:
                segments.append(current)
                current = [points[i]]
            else:
                current.append(points[i])
        segments.append(current)

        for seg in segments:
            if len(seg) < 2:
                continue

            cr.move_to(px(seg[0][0]), py(seg[0][1]))
            for t, v in seg[1:]:
                cr.line_to(px(t), py(v))
            cr.line_to(px(seg[-1][0]), PAD_T + ch)
            cr.line_to(px(seg[0][0]),  PAD_T + ch)
            cr.close_path()
            grad = LinearGradient(0, PAD_T, 0, PAD_T + ch)
            grad.add_color_stop_rgba(0.0, ar, ag, ab, 0.35)
            grad.add_color_stop_rgba(1.0, ar, ag, ab, 0.02)
            cr.set_source(grad)
            cr.fill()

            cr.set_source_rgba(ar, ag, ab, 1.0)
            cr.set_line_width(2.5)
            cr.set_line_cap(1)
            cr.set_line_join(1)
            cr.move_to(px(seg[0][0]), py(seg[0][1]))
            for t, v in seg[1:]:
                cr.line_to(px(t), py(v))
            cr.stroke()

        if self._hover_x >= PAD_L:
            best = min(points, key=lambda p: abs(px(p[0]) - self._hover_x))
            hx = px(best[0])
            hy = py(best[1])

            cr.set_source_rgba(1, 1, 1, 0.12)
            cr.set_line_width(1)
            cr.set_dash([4, 4])
            cr.move_to(hx, PAD_T)
            cr.line_to(hx, PAD_T + ch)
            cr.stroke()
            cr.set_dash([])

            cr.arc(hx, hy, 5, 0, 2 * math.pi)
            cr.set_source_rgba(ar, ag, ab, 1.0)
            cr.fill()
            cr.arc(hx, hy, 5, 0, 2 * math.pi)
            cr.set_source_rgba(1, 1, 1, 0.9)
            cr.set_line_width(2)
            cr.stroke()

            import datetime
            ts_str = datetime.datetime.fromtimestamp(best[0]).strftime('%d/%m %H:%M')
            label = f"{best[1]:.0f}%  {ts_str}"
            cr.select_font_face('sans-serif', 0, 1)
            cr.set_font_size(12)
            te = cr.text_extents(label)
            tw, th2 = te[2] + 16, 28
            tx = min(hx + 10, w - PAD_R - tw - 2)
            ty = max(PAD_T + 2, hy - 14)

            cr.set_source_rgba(0.1, 0.1, 0.12, 0.9)
            self._rrect(cr, tx, ty, tw, th2, 6)
            cr.fill()
            cr.set_source_rgba(ar, ag, ab, 0.6)
            self._rrect(cr, tx, ty, tw, th2, 6)
            cr.set_line_width(1)
            cr.stroke()

            cr.set_source_rgba(1, 1, 1, 0.9)
            cr.move_to(tx + 8, ty + 18)
            cr.show_text(label)

    def _rrect(self, cr, x, y, w, h, r):
        cr.new_sub_path()
        cr.arc(x+w-r, y+r,   r, -math.pi/2, 0)
        cr.arc(x+w-r, y+h-r, r, 0,           math.pi/2)
        cr.arc(x+r,   y+h-r, r, math.pi/2,   math.pi)
        cr.arc(x+r,   y+r,   r, math.pi,      3*math.pi/2)
        cr.close_path()


class MainWindow(Adw.ApplicationWindow):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.set_title("Battery Usage")
        self.set_default_size(800, 600)

        header = Adw.HeaderBar()

        menu = Gio.Menu()
        menu.append("About", "app.about")
        popover = Gtk.PopoverMenu.new_from_model(menu)
        hamburger = Gtk.MenuButton()
        hamburger.set_popover(popover)
        hamburger.set_icon_name("open-menu-symbolic")
        header.pack_end(hamburger)

        about_action = Gio.SimpleAction.new("about", None)
        about_action.connect("activate", self._on_about)
        self.get_application().add_action(about_action)

        self.main_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=16)
        self.main_box.set_margin_top(24)
        self.main_box.set_margin_bottom(24)
        self.main_box.set_margin_start(24)
        self.main_box.set_margin_end(24)

        self.cards_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
        self.cards_box.set_homogeneous(True)

        percentage   = float(get_prop('Percentage'))
        full_design  = float(get_prop('EnergyFullDesign'))
        full_current = float(get_prop('EnergyFull'))
        energy_rate  = float(get_prop('EnergyRate'))
        state        = int(get_prop('State'))
        is_charging  = state == 1

        perc_card,     _,                 _,               self.lbl_percentage = self._make_card("Battery Charge", f"{percentage:.1f}%", "battery-full-symbolic")
        full_cap_card, _,                 _,               self.lbl_capacity   = self._make_card("Full Capacity", f"{full_current:.1f} Wh / {full_design:.1f} Wh", "battery-full-charged-symbolic")
        rate_card,     self.charge_icon,  self.lbl_charge, self.lbl_rate       = self._make_card("Charge Rate" if is_charging else "Discharge Rate", f"{energy_rate:.2f} W", "battery-good-symbolic" if is_charging else "battery-caution-symbolic")

        for card in [perc_card, full_cap_card, rate_card]:
            self.cards_box.append(card)

        period_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=4)
        period_box.set_halign(Gtk.Align.CENTER)

        self._period_buttons = {}
        first = True
        for label in TIME_SPANS:
            btn = Gtk.ToggleButton(label=label)
            btn.add_css_class("pill")
            if first:
                btn.set_active(True)
                first = False
            btn.connect("toggled", self._on_period_toggled, label)
            self._period_buttons[label] = btn
            period_box.append(btn)

        self.graph = BatteryChart()
        self.graph.refresh()

        self.main_box.append(self.cards_box)
        self.main_box.append(Gtk.Separator())
        self.main_box.append(period_box)
        self.main_box.append(self.graph)

        outer_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        outer_box.append(header)
        outer_box.append(self.main_box)
        self.set_content(outer_box)

        GLib.timeout_add_seconds(30, self._refresh)

    def _on_about(self, action, param):
        dialog = Adw.AboutDialog()
        dialog.set_application_name("Battery Usage")
        dialog.set_version("1.0.0")
        dialog.set_developer_name("Simone Ancona")
        dialog.set_application_icon("battery-full")
        dialog.set_license_type(Gtk.License.GPL_3_0)
        dialog.set_website("https://github.com/SimoneAncona/gtk4-battery-usage")
        dialog.set_issue_url("https://github.com/SimoneAncona/gtk4-battery-usage/issues")
        dialog.set_developers(["Simone Ancona"])
        dialog.present(self)

    def _refresh(self):
        percentage  = float(get_prop('Percentage'))
        energy_rate = float(get_prop('EnergyRate'))
        state       = int(get_prop('State'))
        is_charging = state == 1

        self.lbl_percentage.set_label(f"{percentage:.1f}%")
        self.lbl_rate.set_label(f"{energy_rate:.2f} W")
        self.lbl_charge.set_label("Charge Rate" if is_charging else "Discharge Rate")
        self.charge_icon.set_from_icon_name("battery-good-symbolic" if is_charging else "battery-caution-symbolic")
        self.graph.refresh()
        return True

    def _make_card(self, title, value, icon_name):
        frame = Gtk.Frame()
        frame.add_css_class("card")

        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=8)
        box.set_margin_top(16)
        box.set_margin_bottom(16)
        box.set_margin_start(16)
        box.set_margin_end(16)

        icon = Gtk.Image.new_from_icon_name(icon_name)
        icon.set_pixel_size(32)
        icon.set_halign(Gtk.Align.START)
        icon.add_css_class("dim-label")

        value_lbl = Gtk.Label(label=value)
        value_lbl.set_halign(Gtk.Align.START)
        value_lbl.add_css_class("title-2")

        title_lbl = Gtk.Label(label=title)
        title_lbl.set_halign(Gtk.Align.START)
        title_lbl.add_css_class("dim-label")
        title_lbl.add_css_class("caption")

        box.append(icon)
        box.append(value_lbl)
        box.append(title_lbl)
        frame.set_child(box)
        return frame, icon, title_lbl, value_lbl

    def _on_period_toggled(self, btn, period):
        if btn.get_active():
            for p, b in self._period_buttons.items():
                if p != period:
                    b.handler_block_by_func(self._on_period_toggled)
                    b.set_active(False)
                    b.handler_unblock_by_func(self._on_period_toggled)
            self.graph.set_period(TIME_SPANS[period])


class MyApp(Adw.Application):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.connect('activate', self.on_activate)

    def on_activate(self, app):
        self.win = MainWindow(application=app)
        self.win.present()

app = MyApp(application_id="io.github.SimoneAncona.gtk4-battery-usage")
app.run(None)