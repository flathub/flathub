#!/usr/bin/env python3
"""Rumble Checker — detect gamepads, test force feedback, fix permissions."""

import tkinter as tk
from tkinter import ttk, messagebox
import re
import subprocess
import os
import grp
import pwd
import wave
import struct
import threading
import tempfile

# ─── SEGA GENESIS PALETTE ────────────────────────────────────────
BG        = "#000000"
PANEL     = "#141414"
BORDER    = "#e60012"
WHITE     = "#f0f0f0"
RED       = "#e60012"
GREEN     = "#00d812"
YELLOW    = "#f8b800"
ORANGE    = "#f87858"
GRAY      = "#8a8a8a"
DARKGRAY  = "#1e1e1e"
SELECT    = "#2a2a2a"

FONT       = ("Courier", 10)
FONT_BOLD  = ("Courier", 10, "bold")
FONT_TITLE = ("Courier", 14, "bold")
FONT_SUB   = ("Courier", 10)
FONT_SMALL = ("Courier", 9)

# ─── MODE MAPPING ────────────────────────────────────────────────
DRIVER_MODES = {
    "xpad": "XInput (Xbox 360/One)",
    "hid-xpadneo": "XInput (Xbox One)",
    "hid-generic": "HID / DirectInput (Native)",
    "hid-nintendo": "Switch Pro",
    "hid-playstation": "PlayStation Native",
    "hid-sony": "PlayStation Native",
    "hid-steam": "Steam Input",
}

VENDOR_PRODUCT_MODES = {
    "2dc8:3106": "8BitDo XInput",
    "2dc8:3105": "8BitDo DInput (Native)",
    "054c:05c4": "8BitDo Mac",
    "054c:0cda": "8BitDo PSC (PlayStation Classic)",
    "0079:0011": "8BitDo Genesis",
    "054c:02fd": "Xbox 360 (XInput)",
    "054c:0e6f": "Xbox One (XInput)",
    "054c:0e03": "Xbox One S (XInput)",
    "054c:09cc": "PS4 DualShock 4 (Native)",
    "054c:0ce6": "PS5 DualSense (Native)",
    "057e:2009": "Switch Pro Controller",
    "057e:200e": "Switch Joy-Con (L)",
    "057e:200f": "Switch Joy-Con (R)",
}

# ─── 8-BIT SOUND ENGINE ─────────────────────────────────────────
class SoundFX:
    def __init__(self):
        self.enabled = True

    def _play(self, freqs, duration_ms=100, volume=0.3):
        if not self.enabled:
            return
        try:
            sample_rate = 22050
            total_samples = int(sample_rate * duration_ms / 1000)
            samples_per_freq = total_samples // len(freqs)
            with tempfile.NamedTemporaryFile(suffix='.wav', delete=False) as f:
                tmp_path = f.name
            with wave.open(tmp_path, 'w') as wav:
                wav.setnchannels(1)
                wav.setsampwidth(2)
                wav.setframerate(sample_rate)
                for freq in freqs:
                    for i in range(samples_per_freq):
                        val = int(volume * 32767 * (1 if (i * 2 * freq / sample_rate) % 2 < 1 else -1))
                        wav.writeframesraw(struct.pack('<h', val))
            subprocess.run(['paplay', tmp_path],
                          stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            os.unlink(tmp_path)
        except Exception:
            pass

    def power_on(self):
        threading.Thread(target=lambda: self._play([262, 330, 392, 523], 400), daemon=True).start()

    def ding(self):
        threading.Thread(target=lambda: self._play([880, 1100], 150), daemon=True).start()

    def error(self):
        threading.Thread(target=lambda: self._play([220, 185], 200), daemon=True).start()

    def level_complete(self):
        threading.Thread(target=lambda: self._play([523, 659, 784, 1047], 600), daemon=True).start()

    def click(self):
        threading.Thread(target=lambda: self._play([440], 50, 0.15), daemon=True).start()


sfx = SoundFX()


# ─── CORE FUNCTIONS ─────────────────────────────────────────────
def parse_input_devices():
    gamepads = []
    with open('/proc/bus/input/devices') as f:
        content = f.read()
    blocks = re.split(r'\n\n', content)
    for block in blocks:
        lines = block.strip().split('\n')
        name = ""
        handlers = ""
        ff = ""
        vendor = ""
        product = ""
        sysfs = ""
        uniq = ""
        for line in lines:
            if line.startswith('N: Name='):
                name = line.split('Name=', 1)[1]
            elif line.startswith('H: Handlers='):
                handlers = line.split('Handlers=', 1)[1]
            elif line.startswith('B: FF='):
                ff = line.split('FF=', 1)[1]
            elif line.startswith('I:'):
                if 'Vendor=' in line:
                    vendor = line.split('Vendor=')[1].split()[0]
                if 'Product=' in line:
                    product = line.split('Product=')[1].split()[0]
            elif line.startswith('S: Sysfs='):
                sysfs = line.split('Sysfs=', 1)[1]
            elif line.startswith('U: Uniq='):
                uniq = line.split('Uniq=', 1)[1]
        if 'js' in handlers and name:
            event_match = re.search(r'event(\d+)', handlers)
            event_node = f"/dev/input/event{event_match.group(1)}" if event_match else None
            gamepads.append({
                'name': name, 'event': event_node, 'has_ff': bool(ff),
                'vendor': vendor, 'product': product, 'sysfs': sysfs, 'uniq': uniq,
            })
    return gamepads


def get_controller_mode(sysfs_path):
    if not sysfs_path:
        return "unknown"
    full_path = "/sys" + sysfs_path
    device_dir = os.path.dirname(os.path.dirname(full_path))
    driver_link = os.path.join(device_dir, "driver")
    if os.path.islink(driver_link):
        return os.path.basename(os.readlink(driver_link))
    return "unknown"


def get_mode_display(vendor, product, driver):
    vp = f"{vendor}:{product}"
    if vp in VENDOR_PRODUCT_MODES:
        return VENDOR_PRODUCT_MODES[vp]
    if driver in DRIVER_MODES:
        return DRIVER_MODES[driver]
    return driver if driver != "unknown" else "Unknown"


def get_battery_level(uniq, sysfs_path):
    if uniq:
        mac_lower = uniq.lower()
        ps_entry = f"/sys/class/power_supply/ps-controller-battery-{mac_lower}"
        if os.path.isdir(ps_entry):
            cap_file = os.path.join(ps_entry, "capacity")
            if os.path.exists(cap_file):
                try:
                    with open(cap_file) as f:
                        return f"{f.read().strip()}%"
                except:
                    pass
    if sysfs_path:
        match = re.search(r'input/input(\d+)', sysfs_path)
        if match:
            input_num = match.group(1)
            ps_link = f"/sys/class/input/input{input_num}/power_supply"
            if os.path.islink(ps_link):
                ps_path = os.path.realpath(ps_link)
                cap_file = os.path.join(ps_path, "capacity")
                if os.path.exists(cap_file):
                    try:
                        with open(cap_file) as f:
                            return f"{f.read().strip()}%"
                    except:
                        pass
    ps_base = "/sys/class/power_supply"
    if os.path.isdir(ps_base):
        for entry in os.listdir(ps_base):
            entry_path = os.path.join(ps_base, entry)
            cap_file = os.path.join(entry_path, "capacity")
            if not os.path.exists(cap_file):
                continue
            dev_link = os.path.join(entry_path, "device")
            if os.path.islink(dev_link):
                real = os.path.realpath(dev_link)
                if sysfs_path and sysfs_path.lstrip("/") in real:
                    try:
                        with open(cap_file) as f:
                            return f"{f.read().strip()}%"
                    except:
                        pass
    return "N/A"


def get_vendor_product(sysfs_path):
    full_path = "/sys" + sysfs_path
    input_dir = os.path.dirname(full_path)
    device_dir = os.path.dirname(input_dir)
    vendor, product = "0000", "0000"
    try:
        with open(os.path.join(device_dir, 'idVendor')) as f:
            vendor = f.read().strip()
        with open(os.path.join(device_dir, 'idProduct')) as f:
            product = f.read().strip()
    except (FileNotFoundError, PermissionError):
        pass
    return vendor, product


def test_rumble(event_node, duration_ms=500):
    script = f'''
from evdev import InputDevice, ecodes, ff
import time, sys
dev = InputDevice("{event_node}")
caps = dev.capabilities()
if ecodes.EV_FF not in caps:
    print("NO_FF")
    sys.exit(0)
rumble = ff.Rumble(strong_magnitude=0xffff, weak_magnitude=0xffff)
effect = ff.Effect(ecodes.FF_RUMBLE, -1, 0, ff.Trigger(0, 0), ff.Replay({duration_ms}, 0), ff.EffectType(ff_rumble_effect=rumble))
effect_id = dev.upload_effect(effect)
dev.write(ecodes.EV_FF, effect_id, 1)
time.sleep({duration_ms} / 1000 + 0.1)
dev.erase_effect(effect_id)
print("OK")
'''
    try:
        result = subprocess.run(['python3', '-c', script], capture_output=True, text=True, timeout=10)
        if result.stdout.strip() == "OK":
            return "OK"
        elif "NO_FF" in result.stdout:
            return "NO_FF"
        else:
            return result.stderr.strip() or result.stdout.strip()
    except Exception as e:
        return f"ERROR: {e}"


def is_in_input_group():
    try:
        groups = grp.getgrnam('input').gr_mem
        return pwd.getpwuid(os.getuid()).pw_name in groups
    except KeyError:
        return False


def is_readable(event_node):
    return os.access(event_node, os.R_OK)


def fix_permissions(event_node, name, vendor, product):
    messages = []
    if not is_in_input_group():
        try:
            result = subprocess.run(
                ['pkexec', 'usermod', '-aG', 'input', pwd.getpwuid(os.getuid()).pw_name],
                capture_output=True, text=True, timeout=30)
            if result.returncode == 0:
                messages.append("Added you to the 'input' group.")
                messages.append("NOTE: Log out and back in for full effect.")
            else:
                messages.append(f"usermod failed: {result.stderr.strip()}")
        except FileNotFoundError:
            messages.append("pkexec not found. Run manually: sudo usermod -aG input $USER")
        except Exception as e:
            messages.append(f"usermod error: {e}")

    rule_line = (
        f'SUBSYSTEM=="input", ATTRS{{idVendor}}=="{vendor}", '
        f'ATTRS{{idProduct}}=="{product}", '
        f'MODE="0666", ENV{{ID_INPUT_JOYSTICK}}="1"')
    rule_path = "/etc/udev/rules.d/99-rumble-checker.rules"

    try:
        result = subprocess.run(
            ['pkexec', 'bash', '-c',
             f'grep -q "idVendor.*{vendor}.*idProduct.*{product}" {rule_path} 2>/dev/null || '
             f'echo "{rule_line}" >> {rule_path}'],
            capture_output=True, text=True, timeout=10)
        if result.returncode == 0:
            messages.append(f"Udev rule for {name} (ID {vendor}:{product}) added.")
            messages.append("Applies to ALL controllers with this ID.")
        else:
            messages.append(f"Rule write failed: {result.stderr.strip()}")
    except FileNotFoundError:
        messages.append(f"Run manually: sudo bash -c 'echo \"{rule_line}\" >> {rule_path}'")
    except Exception as e:
        messages.append(f"Rule error: {e}")

    try:
        subprocess.run(['pkexec', 'udevadm', 'control', '--reload-rules'], capture_output=True, timeout=10)
        subprocess.run(['pkexec', 'udevadm', 'trigger', '--subsystem-match=input'], capture_output=True, timeout=10)
        messages.append("Udev rules reloaded.")
    except Exception as e:
        messages.append(f"Udev reload failed: {e}")

    if is_readable(event_node):
        messages.append("Device is now readable.")
    else:
        messages.append("Device still not readable. Log out and back in.")

    return is_readable(event_node), "\n".join(messages)


def battery_color(pct_str):
    if pct_str == "N/A":
        return "gray"
    try:
        pct = int(pct_str.replace("%", ""))
        if pct > 50: return "green"
        elif pct > 20: return "yellow"
        else: return "red"
    except:
        return "gray"


# ─── MAIN APP ───────────────────────────────────────────────────
class RumbleChecker:
    def __init__(self, root):
        self.root = root
        root.title("Rumble Checker — by Modman247")
        root.geometry("960x520")
        root.resizable(False, False)
        root.configure(bg=BG)

        # ── "M" LOGO ICON (16x16 pixel art) ──────────────────────
        m_pattern = [
            "XX........XX",
            "XX........XX",
            "XXX......XXX",
            "X.XX....XX.X",
            "X..XX..XX..X",
            "X...XXXX...X",
            "X....XX....X",
            "X..........X",
            "X..........X",
            "X..........X",
            "X..........X",
            "X..........X",
            "X..........X",
            "X..........X",
            "X..........X",
            "X..........X",
        ]
        try:
            icon = tk.PhotoImage(width=16, height=16)
            for y, row in enumerate(m_pattern):
                for x, ch in enumerate(row):
                    icon.put(BORDER if ch == 'X' else BG, (x, y))
            root.iconphoto(True, icon)
        except:
            pass

        tk.Label(root, text="Controller Rumble Checker", font=FONT_TITLE,
                 bg=BG, fg=WHITE).pack(pady=(10, 2))
        tk.Label(root, text="Detects gamepads, tests force feedback, fixes permissions | Modman247",
                 font=FONT_SUB, bg=BG, fg=GRAY).pack(pady=(0, 8))

        # ── CONTROLLER LIST ──────────────────────────────────────
        self.frame = tk.Frame(root, bg=PANEL, relief="flat", bd=0,
                              highlightbackground=DARKGRAY, highlightthickness=2)
        self.frame.pack(fill="both", expand=True, padx=10)

        self.tree = ttk.Treeview(self.frame,
            columns=("name", "node", "id", "mode", "batt", "ff", "access"),
            show="headings")
        self.tree.heading("name", text="Controller")
        self.tree.heading("node", text="Device")
        self.tree.heading("id", text="Vendor:Product")
        self.tree.heading("mode", text="Mode")
        self.tree.heading("batt", text="Battery")
        self.tree.heading("ff", text="Rumble")
        self.tree.heading("access", text="Access")
        self.tree.column("name", width=160)
        self.tree.column("node", width=160)
        self.tree.column("id", width=160)
        self.tree.column("mode", width=170)
        self.tree.column("batt", width=60)
        self.tree.column("ff", width=55)
        self.tree.column("access", width=60)

        style = ttk.Style()
        style.theme_use("clam")
        style.configure("Retro.Treeview",
                        background=BG, foreground=WHITE,
                        fieldbackground=BG, font=FONT, rowheight=26)
        style.configure("Retro.Treeview.Heading",
                        background=PANEL, foreground=RED, font=FONT_BOLD)
        style.map("Retro.Treeview",
                  background=[("selected", SELECT)],
                  foreground=[("selected", WHITE)])
        self.tree.configure(style="Retro.Treeview")

        scrollbar = ttk.Scrollbar(self.frame, orient="vertical", command=self.tree.yview)
        self.tree.configure(yscrollcommand=scrollbar.set)
        self.tree.pack(side="left", fill="both", expand=True, padx=6, pady=6)
        scrollbar.pack(side="right", fill="y", pady=6)
        self.tree.bind("<<TreeviewSelect>>", self.on_select)

        # ── BUTTONS ──────────────────────────────────────────────
        btn_frame = tk.Frame(root, bg=BG)
        btn_frame.pack(pady=8)

        self.refresh_btn = self._make_btn(btn_frame, "Rescan", self.scan)
        self.refresh_btn.pack(side="left", padx=5)
        self.test_btn = self._make_btn(btn_frame, "Test Rumble", self.test_selected, state="disabled")
        self.test_btn.pack(side="left", padx=5)
        self.fix_btn = self._make_btn(btn_frame, "Fix Access", self.fix_selected, state="disabled")
        self.fix_btn.pack(side="left", padx=5)

        # ── DURATION SLIDER ──────────────────────────────────────
        slider_frame = tk.Frame(root, bg=BG)
        slider_frame.pack(pady=(0, 6))
        tk.Label(slider_frame, text="Duration:", font=FONT_SMALL,
                 bg=BG, fg=GRAY).pack(side="left")
        self.duration = tk.IntVar(value=500)
        self.slider = tk.Scale(slider_frame, from_=100, to=3000, variable=self.duration,
                               orient="horizontal", length=180,
                               bg=BG, fg=WHITE, troughcolor=DARKGRAY,
                               activebackground=RED, highlightthickness=0,
                               font=FONT_SMALL, showvalue=False)
        self.slider.pack(side="left", padx=5)
        self.duration_label = tk.Label(slider_frame, text="500ms", font=FONT_SMALL,
                                       bg=BG, fg=RED)
        self.duration_label.pack(side="left")
        self.slider.configure(command=lambda v: self.duration_label.config(text=f"{int(float(v))}ms"))

        # ── STATUS BAR ───────────────────────────────────────────
        self.status = tk.Label(root, text=" ", font=FONT_BOLD, bg=DARKGRAY,
                               fg=WHITE, anchor="w", padx=10, pady=5, wraplength=920)
        self.status.pack(fill="x", side="bottom")
        self.cursor_visible = True
        self.status_text = ""

        self.gamepads = []
        self._boot_sequence()

    def _boot_sequence(self):
        sfx.power_on()
        self._set_status("POWER ON...", YELLOW)
        self.root.after(600, lambda: self._set_status("LOADING MODULES...", YELLOW))
        self.root.after(1200, lambda: self._set_status("SCANNING INPUT DEVICES...", YELLOW))
        self.root.after(1800, self._boot_done)

    def _boot_done(self):
        self.scan()
        self._blink_cursor()

    def _set_status(self, text, color=WHITE):
        self.status_text = text
        self.status.config(fg=color)
        self._update_status_display()

    def _update_status_display(self):
        cursor = " " if self.cursor_visible else " "
        self.status.config(text=f"{self.status_text}{cursor}")

    def _blink_cursor(self):
        self.cursor_visible = not self.cursor_visible
        self._update_status_display()
        self.root.after(500, self._blink_cursor)

    def _make_btn(self, parent, text, command, state="normal"):
        def wrapped():
            sfx.click()
            command()
        return tk.Button(parent, text=text, command=wrapped,
                         font=FONT_BOLD, bg=DARKGRAY, fg=WHITE,
                         activebackground=BORDER, activeforeground=WHITE,
                         relief="flat", bd=2, padx=14, pady=5,
                         state=state, cursor="hand2",
                         highlightthickness=1, highlightbackground=GRAY)

    def log(self, text, tag="info"):
        self.log_text.configure(state="normal")
        self.log_text.insert("end", text, tag)
        self.log_text.see("end")
        self.log_text.configure(state="disabled")
        self.root.update_idletasks()

    def scan(self):
        self.tree.delete(*self.tree.get_children())
        self.gamepads = parse_input_devices()

        if not self.gamepads:
            self._set_status("NO CONTROLLERS FOUND — PLUG ONE IN AND RESCAN", RED)
            self.test_btn.config(state="disabled")
            self.fix_btn.config(state="disabled")
            return

        for g in self.gamepads:
            ff_status = "Yes" if g['has_ff'] else "No"
            access_status = "OK" if (g['event'] and is_readable(g['event'])) else "DENIED"
            id_str = f"{g['vendor']}:{g['product']}" if g['vendor'] not in ("", "0000") else "N/A"
            driver = get_controller_mode(g['sysfs'])
            mode_str = get_mode_display(g['vendor'], g['product'], driver)
            batt_str = get_battery_level(g['uniq'], g['sysfs'])
            batt_tag = battery_color(batt_str)
            tags = []
            if g['has_ff']: tags.append('ff')
            else: tags.append('no_ff')
            if access_status == "DENIED": tags.append('denied')
            tags.append(f'batt_{batt_tag}')

            self.tree.insert("", "end", values=(
                g['name'], g['event'] or 'N/A', id_str,
                mode_str, batt_str, ff_status, access_status
            ), tags=tags)

        self.tree.tag_configure('ff', foreground=GREEN)
        self.tree.tag_configure('no_ff', foreground=RED)
        self.tree.tag_configure('denied', foreground=ORANGE)
        self.tree.tag_configure('batt_green', foreground=GREEN)
        self.tree.tag_configure('batt_yellow', foreground=YELLOW)
        self.tree.tag_configure('batt_red', foreground=RED)
        self.tree.tag_configure('batt_gray', foreground=GRAY)
        self._set_status(f"FOUND {len(self.gamepads)} CONTROLLER(S) — READY", GREEN)

    def test_selected(self):
        selection = self.tree.selection()
        if not selection:
            messagebox.showwarning("No selection", "Select a controller first.")
            return
        item = self.tree.item(selection[0])
        event_node = item['values'][1]
        if event_node == 'N/A':
            messagebox.showerror("Error", "No device node found.")
            return

        duration = int(self.duration.get())
        self._set_status(f"TESTING {event_node} ({duration}ms)...", YELLOW)
        self.test_btn.config(state="disabled")
        result = test_rumble(event_node, duration)

        if result == "OK":
            sfx.ding()
            self._set_status("RUMBLE TEST PASSED — YOU SHOULD HAVE FELT IT!", GREEN)
        elif result == "NO_FF":
            sfx.error()
            self._set_status("DEVICE DOES NOT SUPPORT FORCE FEEDBACK", RED)
        else:
            sfx.error()
            self._set_status(f"FAILED: {result}", RED)
        self.test_btn.config(state="normal")

    def fix_selected(self):
        selection = self.tree.selection()
        if not selection:
            messagebox.showwarning("No selection", "Select a controller first.")
            return
        item = self.tree.item(selection[0])
        event_node = item['values'][1]
        name = item['values'][0]
        if event_node == 'N/A':
            messagebox.showerror("Error", "No device node found.")
            return
        if is_readable(event_node):
            messagebox.showinfo("No fix needed", "This device is already accessible.")
            return

        gp = next((g for g in self.gamepads if g['event'] == event_node), None)
        vendor, product = "0000", "0000"
        if gp and gp['sysfs']:
            vendor, product = get_vendor_product(gp['sysfs'])

        self._set_status("APPLYING FIX (MAY PROMPT FOR PASSWORD)...", YELLOW)
        self.fix_btn.config(state="disabled")
        success, messages = fix_permissions(event_node, name, vendor, product)

        if success:
            sfx.level_complete()
            self._set_status("FIX APPLIED! ALL CONTROLLERS WITH THIS ID ARE NOW COVERED.", GREEN)
        else:
            sfx.error()
            self._set_status("FIX ATTEMPTED BUT DEVICE STILL NOT ACCESSIBLE.", ORANGE)

        messagebox.showinfo("Fix Results", messages)
        self.fix_btn.config(state="normal")
        self.scan()

    def on_select(self, event):
        if self.tree.selection():
            self.test_btn.config(state="normal")
            item = self.tree.item(self.tree.selection()[0])
            if item['values'][6] == "DENIED":
                self.fix_btn.config(state="normal")
            else:
                self.fix_btn.config(state="disabled")


if __name__ == "__main__":
    try:
        import evdev
    except ImportError:
        print("python3-evdev not found. Install with:")
        print("  sudo pacman -S python-evdev")
        raise SystemExit(1)

    root = tk.Tk()
    app = RumbleChecker(root)
    root.mainloop()
