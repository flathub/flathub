import tkinter as tk
from tkinter import simpledialog, messagebox, font as tkfont
import os

import matplotlib
matplotlib.use("TkAgg")
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure
from matplotlib.patches import Rectangle

from PIL import Image, ImageTk

try:
    RESAMPLE = Image.Resampling.LANCZOS
except AttributeError:
    RESAMPLE = Image.Resampling.BICUBIC

def get_resource_path(filename):
    import sys
    if getattr(sys, 'frozen', False):
        base_path = getattr(sys, '_MEIPASS', os.path.dirname(os.path.abspath(__file__)))
    else:
        base_path = os.path.dirname(os.path.abspath(__file__))
    return os.path.join(base_path, filename)

def get_splash_path():
    flatpak_path = "/app/share/candlegenie/splash.png"
    if os.path.exists(flatpak_path):
        return flatpak_path
    local_path = get_resource_path("splash.png")
    if os.path.exists(local_path):
        return local_path
    fallback_path = get_resource_path("logo.png")
    return fallback_path

SPLASH_PATH = get_splash_path()

CANDLE_PRESETS = {
    "short green candle": {
        "body_percent": 0.25, "body_centre": 0.5, "body_colour": "green", "classification": "short green candle",
        "top_shadow_percent": 0.375, "bottom_shadow_percent": 0.375
    },
    "short red candle": {
        "body_percent": 0.25, "body_centre": 0.5, "body_colour": "red", "classification": "short red candle",
        "top_shadow_percent": 0.375, "bottom_shadow_percent": 0.375
    },
    "long green candle": {
        "body_percent": 0.5, "body_centre": 0.5, "body_colour": "green", "classification": "long green candle",
        "top_shadow_percent": 0.25, "bottom_shadow_percent": 0.25
    },
    "long red candle": {
        "body_percent": 0.5, "body_centre": 0.5, "body_colour": "red", "classification": "long red candle",
        "top_shadow_percent": 0.25, "bottom_shadow_percent": 0.25
    }
}
PATTERN_LIST = list(CANDLE_PRESETS.keys())

class CandleConfig:
    def __init__(self):
        self.pattern_name = PATTERN_LIST[0] if PATTERN_LIST else ""
        self.body_height = 0.3
        self.body_center = 0.5
        self.body_color = "green"
        self.classification = ""
        self.top_shadow_percent = 0.35
        self.bottom_shadow_percent = 0.35

    def apply_preset(self, preset):
        self.body_height = preset["body_percent"]
        self.body_center = preset["body_centre"]
        self.body_color = preset["body_colour"]
        self.classification = preset["classification"]
        self.top_shadow_percent = preset.get("top_shadow_percent", 0)
        self.bottom_shadow_percent = preset.get("bottom_shadow_percent", 0)

class CandleMakerApp:
    def __init__(self, master):
        self.master = master
        master.title("Candlestick Parameter Creator")
        try:
            master.attributes('-zoomed', True)
        except tk.TclError:
            try:
                master.state('zoomed')
            except tk.TclError:
                master.attributes('-fullscreen', True)

        master.rowconfigure(0, weight=1)
        master.columnconfigure(0, weight=4)
        master.columnconfigure(1, weight=1)

        self.fig = Figure(figsize=(7, 10), dpi=100)
        self.ax = self.fig.add_subplot(111)
        self.ax.axis("off")
        self.canvas = FigureCanvasTkAgg(self.fig, master)
        self.canvas_widget = self.canvas.get_tk_widget()
        self.canvas_widget.grid(row=0, column=0, sticky="nsew", padx=10, pady=10)
        self.canvas.mpl_connect("button_press_event", self.on_plot_click)
        self.canvas.mpl_connect("motion_notify_event", self.on_motion)
        self.canvas.mpl_connect("button_release_event", self.on_release)

        controls_outer = tk.Frame(master)
        controls_outer.grid(row=0, column=1, sticky="nsew", padx=10, pady=10)
        controls_outer.rowconfigure(0, weight=1)
        controls_outer.columnconfigure(0, weight=1)

        canvas = tk.Canvas(controls_outer, highlightthickness=0)
        v_scroll = tk.Scrollbar(controls_outer, orient="vertical", command=canvas.yview)
        controls = tk.Frame(canvas)

        controls.bind("<Configure>", lambda e: canvas.configure(scrollregion=canvas.bbox("all")))
        canvas.create_window((0, 0), window=controls, anchor="nw")
        canvas.configure(yscrollcommand=v_scroll.set)
        canvas.pack(side="left", fill="both", expand=True)
        v_scroll.pack(side="right", fill="y")
        controls.columnconfigure(0, weight=1)
        controls.rowconfigure(99, weight=1)
        self.controls = controls

        header_font = tkfont.Font(size=18, weight="bold")
        regular_font = tkfont.Font(size=14)

        tk.Label(controls, text="Number of Candlesticks", font=header_font).grid(row=2, column=0, sticky="w")
        self.num_candles = tk.IntVar(value=1)
        self.num_spinbox = tk.Spinbox(controls, from_=1, to=2, width=5, font=regular_font,  # LIMIT to 2
                                      textvariable=self.num_candles, command=self.update_candle_count)
        self.num_spinbox.grid(row=3, column=0, sticky="w")

        # --- Single dynamic control frame ---
        self.selected_candle = 0
        self.candle_configs = [CandleConfig() for _ in range(self.num_candles.get())]
        self.candle_control_frame = tk.Frame(controls, borderwidth=2, relief="groove", padx=2, pady=2)
        self.candle_control_frame.grid(row=10, column=0, rowspan=20, sticky="ew", pady=2)
        self.candle_control_frame.columnconfigure(0, weight=1)
        self.candle_header = tk.Label(self.candle_control_frame, text="Candle 1", font=header_font)
        self.candle_header.grid(row=0, column=0, sticky="w", pady=(0, 6))

        # Pattern dropdown
        self.pattern_var = tk.StringVar(value=PATTERN_LIST[0] if PATTERN_LIST else "")
        self.pattern_menu = tk.OptionMenu(self.candle_control_frame, self.pattern_var, *PATTERN_LIST, command=self.on_pattern_select)
        self.pattern_menu.config(font=regular_font)
        self.pattern_menu.grid(row=1, column=0, sticky="ew")

        self.body_height_var = tk.DoubleVar()
        tk.Label(self.candle_control_frame, text="Body Height (%)", font=regular_font).grid(row=2, column=0, sticky="w")
        self.body_slider = tk.Scale(self.candle_control_frame, from_=1, to=99, orient=tk.HORIZONTAL, resolution=1,
                                    font=regular_font, variable=self.body_height_var, command=self.on_body_height_change)
        self.body_slider.grid(row=3, column=0, sticky="ew")

        self.top_shadow_var = tk.DoubleVar()
        tk.Label(self.candle_control_frame, text="Top Shadow (%)", font=regular_font).grid(row=4, column=0, sticky="w")
        self.top_shadow_slider = tk.Scale(self.candle_control_frame, from_=0, to=99, orient=tk.HORIZONTAL, resolution=1,
                                          font=regular_font, variable=self.top_shadow_var, command=self.on_top_shadow_change)
        self.top_shadow_slider.grid(row=5, column=0, sticky="ew")

        self.bottom_shadow_var = tk.DoubleVar()
        tk.Label(self.candle_control_frame, text="Bottom Shadow (%)", font=regular_font).grid(row=6, column=0, sticky="w")
        self.bottom_shadow_slider = tk.Scale(self.candle_control_frame, from_=0, to=99, orient=tk.HORIZONTAL, resolution=1,
                                             font=regular_font, variable=self.bottom_shadow_var, command=self.on_bottom_shadow_change)
        self.bottom_shadow_slider.grid(row=7, column=0, sticky="ew")

        self.colour_btn = tk.Button(self.candle_control_frame, text="Select Color", font=regular_font,
                                    command=self.choose_color)
        self.colour_btn.grid(row=8, column=0, sticky="ew")
        self.colour_lbl = tk.Label(self.candle_control_frame, text="", font=regular_font)
        self.colour_lbl.grid(row=9, column=0, sticky="w")

        tk.Label(self.candle_control_frame, text="Classification / Label:", font=regular_font).grid(row=10, column=0, sticky="w")
        self.class_entry = tk.Entry(self.candle_control_frame, font=regular_font, width=18)
        self.class_entry.grid(row=11, column=0, sticky="ew")

        self.stats_label = tk.Label(self.candle_control_frame, text="", font=regular_font, justify="left")
        self.stats_label.grid(row=12, column=0, sticky="w")

        self.update_controls_from_selected()
        self.update_plot()

    def update_candle_count(self):
        n = self.num_candles.get()
        curr_n = len(self.candle_configs)
        if n > curr_n:
            for _ in range(n - curr_n):
                self.candle_configs.append(CandleConfig())
        elif n < curr_n:
            self.candle_configs = self.candle_configs[:n]
            if self.selected_candle >= n:
                self.selected_candle = n - 1
        self.update_controls_from_selected()
        self.update_plot()

    def update_controls_from_selected(self):
        cfg = self.candle_configs[self.selected_candle]
        self.candle_header.config(text=f"Candle {self.selected_candle + 1}")
        self.pattern_var.set(cfg.pattern_name)
        self.body_height_var.set(cfg.body_height * 100)
        self.top_shadow_var.set(cfg.top_shadow_percent * 100)
        self.bottom_shadow_var.set(cfg.bottom_shadow_percent * 100)
        self.colour_lbl.config(text=cfg.body_color, fg=cfg.body_color)
        self.class_entry.delete(0, tk.END)
        self.class_entry.insert(0, cfg.classification)
        self.update_stats_label(cfg)

    def push_controls_to_config(self):
        cfg = self.candle_configs[self.selected_candle]
        cfg.pattern_name = self.pattern_var.get()
        cfg.body_height = self.body_height_var.get() / 100
        cfg.top_shadow_percent = self.top_shadow_var.get() / 100
        cfg.bottom_shadow_percent = self.bottom_shadow_var.get() / 100
        cfg.body_color = self.colour_lbl.cget("text")
        cfg.classification = self.class_entry.get().strip()
        self.update_stats_label(cfg)

    def on_body_height_change(self, val):
        self.push_controls_to_config()
        self.update_plot()

    def on_top_shadow_change(self, val):
        self.push_controls_to_config()
        self.update_plot()

    def on_bottom_shadow_change(self, val):
        self.push_controls_to_config()
        self.update_plot()

    def choose_color(self):
        color = simpledialog.askstring("Color", "Enter 'red' or 'green':")
        if color and color.lower() in ("red", "green"):
            self.colour_lbl.config(text=color.lower(), fg=color.lower())
            self.push_controls_to_config()
            self.update_plot()
        else:
            messagebox.showinfo("Invalid", "Please enter 'red' or 'green'.")

    def on_pattern_select(self, pattern):
        if pattern in CANDLE_PRESETS:
            cfg = self.candle_configs[self.selected_candle]
            cfg.apply_preset(CANDLE_PRESETS[pattern])
            cfg.pattern_name = pattern
            self.update_controls_from_selected()
            self.update_plot()

    def on_plot_click(self, event):
        if event.xdata is None:
            return
        num = len(self.candle_configs)
        if num == 1:
            idx = 0
        else:
            spacing = 0.13
            total_span = spacing * (num - 1)
            margin = (1 - total_span) / 2
            x_centers = [margin + i * spacing for i in range(num)]
            distances = [abs(event.xdata - x) for x in x_centers]
            idx = distances.index(min(distances))
        if idx != self.selected_candle:
            self.selected_candle = idx
            self.update_controls_from_selected()
            self.update_plot()
        # Start dragging
        if event.ydata is not None:
            self.dragging = idx
        else:
            self.dragging = None

    def update_stats_label(self, cfg):
        stats = (
            f"Body %: {cfg.body_height:.2%}\n"
            f"Top Shadow %: {cfg.top_shadow_percent:.2%}\n"
            f"Bottom Shadow %: {cfg.bottom_shadow_percent:.2%}\n"
            f"Body Center Y: {cfg.body_center:.2%}"
        )
        self.stats_label.config(text=stats)

    def on_motion(self, event):
        if hasattr(self, "dragging") and self.dragging is not None and event.ydata is not None:
            cfg = self.candle_configs[self.dragging]
            cfg.body_center = min(max(event.ydata, cfg.body_height / 2), 1 - cfg.body_height / 2)
            self.update_stats_label(cfg)
            self.update_plot()

    def on_release(self, event):
        self.dragging = None

    def update_plot(self):
        self.ax.clear()
        self.ax.set_ylim(0, 1)
        self.ax.set_xlim(0, 1)
        self.ax.axis("off")
        num = len(self.candle_configs)
        if num == 1:
            x_centers = [0.5]
            width = 0.10
        else:
            spacing = 0.13
            width = 0.11
            total_span = spacing * (num - 1)
            margin = (1 - total_span) / 2
            x_centers = [margin + i * spacing for i in range(num)]
        for idx, (cfg, x_center) in enumerate(zip(self.candle_configs, x_centers)):
            top = min(cfg.body_center + cfg.body_height / 2 + cfg.top_shadow_percent, 1)
            bottom = max(cfg.body_center - cfg.body_height / 2 - cfg.bottom_shadow_percent, 0)
            self.ax.plot([x_center, x_center], [bottom, top], color="black", lw=6, zorder=1)
            body_top = cfg.body_center + cfg.body_height / 2
            body_bot = cfg.body_center - cfg.body_height / 2
            rect = Rectangle(
                (x_center - width / 2, body_bot),
                width,
                cfg.body_height,
                color=cfg.body_color,
                zorder=2,
                linewidth=4 if idx == self.selected_candle else 1,
                edgecolor='orange' if idx == self.selected_candle else 'black'
            )
            self.ax.add_patch(rect)
        self.canvas.draw()

if __name__ == "__main__":
    import time
    from dataclasses import dataclass

    try:
        from screeninfo import get_monitors
        HAS_SCREENINFO = True
    except ImportError:
        HAS_SCREENINFO = False

    def debug(msg):
        print(f"[{time.strftime('%H:%M:%S')}] {msg}", flush=True)

    def get_primary_monitor():
        if HAS_SCREENINFO:
            monitors = get_monitors()
            primary = next((m for m in monitors if getattr(m, "is_primary", False)), None)
            if not primary:
                primary = max(monitors, key=lambda mon: mon.x)
            return primary
        @dataclass
        class Dummy:
            x: int
            y: int
            width: int
            height: int
            is_primary: bool
        temp = tk.Tk()
        w, h = temp.winfo_screenwidth(), temp.winfo_screenheight()
        temp.destroy()
        return Dummy(x=0, y=0, width=w, height=h, is_primary=True)

    class SplashScreen(tk.Toplevel):
        def __init__(self, parent, image_path, timeout=3000):
            super().__init__(parent)
            self.timeout = timeout
            self.overrideredirect(True)
            self.lift()
            self.attributes("-topmost", True)
            m = get_primary_monitor()
            sx, sy, sw, sh = m.x, m.y, m.width, m.height
            self.geometry(f"{sw}x{sh}+{sx}+{sy}")
            self.update()
            bg = tk.Frame(self, bg="#fdfaf2")
            bg.pack(fill="both", expand=True)
            try:
                img = Image.open(image_path)
                w, h = img.size
                maxw, maxh = sw - 80, sh - 80
                scale = min(maxw / w, maxh / h, 1)
                img = img.resize((int(w * scale), int(h * scale)), RESAMPLE)
                self.logo = ImageTk.PhotoImage(img)
                panel = tk.Label(bg, image=self.logo, bg="#fdfaf2")
                panel.place(relx=0.5, rely=0.5, anchor="center")
            except Exception as e:
                debug(f"SPLASH IMAGE LOAD ERROR: {e}")
                panel = tk.Label(bg, text=f"Splash Image Missing\n{e}", bg="#fdfaf2", fg="red", font=("Arial", 32))
                panel.place(relx=0.5, rely=0.5, anchor="center")

    def launch_main(root, m):
        root.update_idletasks()
        sw, sh = m.width, m.height
        sx, sy = m.x, m.y
        root.geometry(f"{sw}x{sh}+{sx}+{sy}")
        root.update()
        root.deiconify()
        root.lift()
        root.focus_force()
        root.after(100, lambda: root.attributes('-topmost', False))
        global app
        app = CandleMakerApp(root)

    root = tk.Tk()
    root.withdraw()
    primary_monitor = get_primary_monitor()
    splash = SplashScreen(root, SPLASH_PATH, timeout=3000)

    def after_splash():
        splash.destroy()
        launch_main(root, primary_monitor)

    root.after(3000, after_splash)
    root.mainloop()
