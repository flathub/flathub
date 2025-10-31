#!/usr/bin/env python3

import sys
import subprocess
import re
import signal
import textwrap
import os
import logging
import shutil
from logging.handlers import RotatingFileHandler 
from PySide6.QtCore import QTimer, QSettings, QSocketNotifier
from PySide6.QtGui import QIcon, QAction, QActionGroup
from PySide6.QtWidgets import QApplication, QSystemTrayIcon, QMenu

# --- Config ---
UPDATE_INTERVAL_MS = 60000  # 60 seconds

# --- LOGGING SETUP ---
LOG_DIR = os.path.join(
    os.path.expanduser('~'), 
    '.local', 'share', 'HeadsetBatteryIndicator', 'logs'
)
LOG_FILE = os.path.join(LOG_DIR, 'app.log')
MAX_LOG_BYTES = 10 * 1024 * 1024  # 10 MB
BACKUP_COUNT = 1

def setup_logging(debug_to_console=False):
    """Sets up the rotating file logger and optional console output."""
    os.makedirs(LOG_DIR, exist_ok=True)
    
    logger = logging.getLogger(__name__)
    logger.setLevel(logging.DEBUG) 
    
    formatter = logging.Formatter(
        '%(asctime)s - %(levelname)s - %(message)s'
    )
    
    # 1. Rotating File Handler (Always active)
    file_handler = RotatingFileHandler(
        LOG_FILE,
        maxBytes=MAX_LOG_BYTES,
        backupCount=BACKUP_COUNT,
        encoding='utf-8'
    )
    file_handler.setFormatter(formatter)
    
    if not logger.handlers:
        logger.addHandler(file_handler)
    
    # 2. Console Stream Handler (Active only in debug mode)
    if debug_to_console:
        console_handler = logging.StreamHandler(sys.stdout)
        console_handler.setFormatter(logging.Formatter("%(levelname)s: %(message)s"))
        console_handler.setLevel(logging.INFO) 
        logger.addHandler(console_handler)
        
    return logger

logger = setup_logging()
# --- END LOGGING SETUP ---

class HeadsetBatteryTray(QSystemTrayIcon):
    def __init__(self, debug_mode=False, parent=None):
        super().__init__(parent)
        self.debug_mode = debug_mode
        # Reconfigure logger for console output if in debug mode
        if debug_mode:
            global logger
            logger = setup_logging(debug_to_console=True) 
            logger.setLevel(logging.DEBUG) 

        logger.info("Application starting up.")
        
        self.settings = QSettings()
        self.load_settings()

        # Initialize and set headsetcontrol_path using robust search
        self.headsetcontrol_path = self._find_headsetcontrol()
        
        # B. DEPENDENCY CHECK AND ERROR MESSAGE
        if not self.headsetcontrol_path:
            logger.critical("HeadsetControl binary not found. Functionality disabled.")
            self.send_notification(
                "Dependency Error",
                "HeadsetControl binary not found. Please install it."
            )
            self.setIcon(QIcon.fromTheme("dialog-error"))
            self.setToolTip("ERROR: HeadsetControl not found.")
        
        self.apply_saved_settings()

        self.menu = QMenu()
        self.setup_menu()
        self.setContextMenu(self.menu)

        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update_status)
        
        # Only start timer if binary is found
        if self.headsetcontrol_path:
            self.timer.start(UPDATE_INTERVAL_MS)
            
        self.update_status()
        self.setVisible(True)
        
        if debug_mode:
            print("--- Headset Indicator DEBUG MODE ---")
            print(f"Log file location: {LOG_FILE}")
            print("Type commands and press Enter:")
            print("  log-test             (writes sample logs at different levels)")
            print("  setIcon [icon-name]  (e.g., 'battery-100-symbolic')")
            print("  notification         (sends desktop + headset sound)")
            print("  update               (forces a status update)")
            print("  resume               (resumes automatic updates)")
            print("  exit                 (quits the application)")
            print("------------------------------------")
            
            self.stdin_notifier = QSocketNotifier(sys.stdin.fileno(), QSocketNotifier.Type.Read, self)
            self.stdin_notifier.activated.connect(self.handle_debug_command)

    def _find_headsetcontrol(self):
        """Searches for the headsetcontrol binary using AppImage logic and system PATH."""
        
        # 1. Search in AppImage (Bundled path)
        appdir = os.getenv('APPDIR')
        if appdir:
            # Standard location inside the AppImage structure
            bundled_path = os.path.join(appdir, 'usr/bin/headsetcontrol') 
            if os.path.exists(bundled_path):
                logger.info(f"Binary found in AppImage: {bundled_path}")
                return bundled_path
            
        # 2. Search in system PATH (using shutil.which)
        path = shutil.which('headsetcontrol')
        if path:
            logger.info(f"Binary found in system PATH: {path}")
            return path
            
        logger.warning("HeadsetControl binary not found in AppImage or system PATH.")
        return None 
        
    def load_settings(self):
        """Loads user settings from QSettings."""
        self.notify_enabled = self.settings.value("notifyEnabled", False, type=bool)
        self.notify_threshold = self.settings.value("notifyThreshold", 20, type=int)
        self.notified_low_battery = False
        
        self.lights_on = self.settings.value("lightsOn", True, type=bool)
        self.sidetone_level = self.settings.value("sidetoneLevel", 0, type=int)
        self.chatmix_level = self.settings.value("chatmixLevel", 64, type=int)
        self.inactive_time = self.settings.value("inactiveTime", 0, type=int)

        logger.debug(f"Settings loaded: Notify={self.notify_enabled}, Threshold={self.notify_threshold}%, Lights={self.lights_on}")


    def setup_menu(self):
            """Builds the context (right-click) menu."""
            
            # --- Info Section ---
            self.info_name_action = QAction("Device: ...")
            self.info_name_action.setEnabled(False)
            self.menu.addAction(self.info_name_action)

            self.info_status_action = QAction("Status: ...")
            self.info_status_action.setEnabled(False)
            self.menu.addAction(self.info_status_action)
            self.menu.addSeparator()

            # --- Notification Section ---
            self.notify_action = QAction("Notify on low battery", self)
            self.notify_action.setCheckable(True)
            self.notify_action.setChecked(self.notify_enabled)
            self.notify_action.toggled.connect(self.on_notify_toggled)
            self.menu.addAction(self.notify_action)

            self.threshold_menu = QMenu("Set Notification Level")
            self.threshold_group = QActionGroup(self)
            self.threshold_group.setExclusive(True)
            
            threshold_levels = [10, 20, 30, 40, 50]
            for level in threshold_levels:
                action = QAction(f"{level}%", self)
                action.setCheckable(True)
                action.setData(level)
                if level == self.notify_threshold:
                    action.setChecked(True)
                self.threshold_menu.addAction(action)
                self.threshold_group.addAction(action)

            self.threshold_group.triggered.connect(self.on_threshold_changed)
            self.menu.addMenu(self.threshold_menu)
            
            # --- Controls Section ---
            self.menu.addSeparator()

            # 1. Lights Toggle
            self.lights_action = QAction("Enable Headset Lights", self)
            self.lights_action.setCheckable(True)
            self.lights_action.setChecked(self.lights_on)
            self.lights_action.toggled.connect(self.on_lights_toggled)
            self.menu.addAction(self.lights_action)

            # 2. Sidetone Submenu
            self.sidetone_menu = QMenu("Set Sidetone Level")
            self.sidetone_group = QActionGroup(self)
            self.sidetone_group.setExclusive(True)
            
            sidetone_options = {"Off": 0, "Low": 32, "Medium": 64, "High": 96, "Max": 128}
            
            for text, level in sidetone_options.items():
                action = QAction(text, self)
                action.setCheckable(True)
                action.setData(level)
                if level == self.sidetone_level:
                    action.setChecked(True)
                self.sidetone_menu.addAction(action)
                self.sidetone_group.addAction(action)
                
            self.sidetone_group.triggered.connect(self.on_sidetone_changed)
            self.menu.addMenu(self.sidetone_menu)
            
            # 3. ChatMix Submenu
            self.chatmix_menu = QMenu("Set ChatMix Level")
            self.chatmix_group = QActionGroup(self)
            self.chatmix_group.setExclusive(True)
            
            chatmix_options = {
                "Game Max (0)": 0, 
                "Game Bias (32)": 32, 
                "Center (64)": 64, 
                "Chat Bias (96)": 96, 
                "Chat Max (128)": 128
            }
            
            for text, level in chatmix_options.items():
                action = QAction(text, self)
                action.setCheckable(True)
                action.setData(level)
                if level == self.chatmix_level:
                    action.setChecked(True)
                self.chatmix_menu.addAction(action)
                self.chatmix_group.addAction(action)
                
            self.chatmix_group.triggered.connect(self.on_chatmix_changed)
            self.menu.addMenu(self.chatmix_menu)
            
            # 4. Inactive Time Submenu
            self.inactivetime_menu = QMenu("Set Auto-Off Time (Min)")
            self.inactivetime_group = QActionGroup(self)
            self.inactivetime_group.setExclusive(True)
            
            time_options = {"Disabled (0)": 0, "10 min": 10, "30 min": 30, "60 min": 60, "90 min": 90}
            
            for text, minutes in time_options.items():
                action = QAction(text, self)
                action.setCheckable(True)
                action.setData(minutes)
                if minutes == self.inactive_time:
                    action.setChecked(True)
                self.inactivetime_menu.addAction(action)
                self.inactivetime_group.addAction(action)

            self.inactivetime_group.triggered.connect(self.on_inactivetime_changed)
            self.menu.addMenu(self.inactivetime_menu)
            # --- LOG FOLDER UTILITIES (ONLY IN DEBUG MODE) ---
            if self.debug_mode:
                print("DEBUG MODE CHECK:")
                print(self.debug_mode)
                self.menu.addSeparator()
                debug_tools_menu = QMenu("Debug & Logs", self.menu)
                
                # 1. Show log folder
                action_show_log = QAction("Show Log Folder", self.menu)
                action_show_log.triggered.connect(self.open_log_folder)
                debug_tools_menu.addAction(action_show_log)
                
                # 2. Clear log
                action_clear_log = QAction("Clear Log File", self)
                action_clear_log.triggered.connect(self.clear_log_file)
                debug_tools_menu.addAction(action_clear_log)
                
                # ATTACH THE DEBUG MENU TO THE MAIN MENU
                self.menu.addMenu(debug_tools_menu)
            # --- END LOG UTILITIES MENU ---
            # --- Exit Section ---
            self.menu.addSeparator()
            quit_action = QAction("Exit", self)
            quit_action.triggered.connect(QApplication.instance().quit)
            self.menu.addAction(quit_action)
        # --- Log Folder Utilities (For Debugging/Support) ---
        
    def open_log_folder(self):
        """Opens the log folder in the file explorer."""
        logger.info(f"Opening log folder: {LOG_DIR}")
        try:
            subprocess.run(['xdg-open', LOG_DIR], check=True, text=True)
        except Exception as e:
            logger.error(f"Failed to open log folder: {e}")
            self.send_notification("Error", "Could not open log folder.")

    def clear_log_file(self):
        """Clears the content of the log file."""
        try:
            with open(LOG_FILE, 'w') as f:
                f.truncate(0)
            logger.info("Log file cleared successfully.")
            self.send_notification("Logs", "Log file cleared successfully.")
        except Exception as e:
            logger.error(f"Failed to clear log file: {e}")
            self.send_notification("Error", "Could not clear log file.")
    # --- END LOG UTILITIES ---


    # --- NEW: Helper Functions ---
    def run_headset_command(self, args_list):
        """Helper to run headsetcontrol commands safely."""
        
        # If the binary was not found at startup, skip command execution
        if not self.headsetcontrol_path:
            logger.warning("Attempted to run command, but headsetcontrol binary is missing.")
            return

        # Uses the path found in _find_headsetcontrol
        command = [self.headsetcontrol_path] + args_list
        
        try:
            subprocess.run(
                command, 
                check=True, 
                capture_output=True, 
                text=True
            )
            logger.info(f"Ran command: {' '.join(command)}")
        except Exception as e:
            error_msg = f"Failed to run command: {' '.join(command)}. Exception: {e}"
            logger.error(error_msg)
            self.send_notification(
                "Headset Command Failed",
                f"Failed to run: {' '.join(command)}\nIs it connected? Check logs for details."
            )

    def apply_saved_settings(self):
        """Applies saved settings to the headset on startup."""
        logger.info("Applying saved settings on startup.")
        
        if self.headsetcontrol_path:
            light_val = "1" if self.lights_on else "0"
            self.run_headset_command(['-l', light_val])
            self.run_headset_command(['-s', str(self.sidetone_level)])
            
            self.run_headset_command(['-m', str(self.chatmix_level)])
            self.run_headset_command(['-i', str(self.inactive_time)])
            
            logger.info(f"Startup settings applied: Lights={self.lights_on}, Sidetone={self.sidetone_level}, ChatMix={self.chatmix_level}, AutoOff={self.inactive_time}m")
        else:
            logger.info("Skipping settings application: HeadsetControl binary not found.")


    # --- Debug Command Handler ---
    def handle_debug_command(self):
        """Processes commands from stdin in debug mode."""
        try:
            line = sys.stdin.readline().strip()
            if not line: return
            parts = line.split()
            if not parts: return
            
            command = parts[0].lower()
            
            if command == "log-test":
                logger.info("DEBUG: Running log level test...")
                logger.debug("--- LOG TEST START (Only visible in file) ---")
                logger.debug("DEBUG: This message is only visible in the app.log file.")
                logger.info("INFO: This message is visible in the file AND the console.")
                logger.warning("WARNING: This message is visible in the file AND the console.")
                logger.error("ERROR: This message is visible in the file AND the console.")
                logger.critical("CRITICAL: This message is visible in the file AND the console.")
                logger.info("Test logs written to console and file.")
                logger.debug("--- LOG TEST END ---")

            elif command == "seticon":
                if len(parts) > 1:
                    icon_name = parts[1]
                    logger.info(f"Setting icon to '{icon_name}'. (Pause timer)")
                    self.timer.stop() 
                    print("Debug: Automatic updates paused. Type 'resume' to restart.")
                    self.setIcon(QIcon.fromTheme(icon_name))
                    QApplication.processEvents()
                else:
                    logger.error("DEBUG Error: 'setIcon' requires an icon name.")
            
            elif command == "notification":
                logger.info("DEBUG: Sending test notification.")
                self.send_notification("Debug Notification", "This is a test message.")
                self.run_headset_command(['-n', '1'])
            
            elif command == "update":
                logger.info("DEBUG: Forcing single status update.")
                self.update_status()
                QApplication.processEvents()

            elif command == "resume":
                logger.info("DEBUG: Resuming automatic updates.")
                self.timer.start(UPDATE_INTERVAL_MS)
                self.update_status()
                QApplication.processEvents()

            elif command == "exit":
                logger.info("Application exiting via DEBUG command.")
                QApplication.instance().quit()
            else:
                logger.error(f"DEBUG Error: Unknown command '{command}'")

        except Exception as e:
            logger.critical(f"Unhandled exception in debug handler: {e}")

    # --- Menu Callback Functions ---

    def on_notify_toggled(self, checked):
        """Called when the user toggles notifications."""
        self.notify_enabled = checked
        self.settings.setValue("notifyEnabled", self.notify_enabled)
        self.notified_low_battery = False 
        logger.info(f"Notification status changed to: {checked}")

    def on_threshold_changed(self, action):
        """Called when the user changes the threshold."""
        self.notify_threshold = action.data()
        self.settings.setValue("notifyThreshold", self.notify_threshold)
        self.notified_low_battery = False
        logger.info(f"Notification threshold set to: {self.notify_threshold}%")


    def on_lights_toggled(self, checked):
        """Called when the user toggles the lights."""
        self.lights_on = checked
        self.settings.setValue("lightsOn", self.lights_on)
        light_val = "1" if checked else "0"
        self.run_headset_command(['-l', light_val])
        logger.info(f"Lights toggled: {'ON' if checked else 'OFF'}")


    def on_sidetone_changed(self, action):
        """Called when the user changes the sidetone level."""
        level = action.data()
        self.sidetone_level = level
        self.settings.setValue("sidetoneLevel", self.sidetone_level)
        self.run_headset_command(['-s', str(level)])
        logger.info(f"Sidetone level set to: {level}")


    def on_chatmix_changed(self, action):
        """Called when the user changes the ChatMix level."""
        level = action.data()
        self.chatmix_level = level
        self.settings.setValue("chatmixLevel", self.chatmix_level)
        self.run_headset_command(['-m', str(level)])
        logger.info(f"ChatMix level set to: {level}")


    def on_inactivetime_changed(self, action):
        """Called when the user changes the inactive auto-off time."""
        minutes = action.data()
        self.inactive_time = minutes
        self.settings.setValue("inactiveTime", self.inactive_time)
        self.run_headset_command(['-i', str(minutes)])
        logger.info(f"Auto-Off time set to: {minutes} minutes.")


    # --- Main Functions ---

    def send_notification(self, title, message):
        """Sends a desktop notification."""
        icon = QIcon.fromTheme("battery-caution-symbolic")
        self.showMessage(title, message, icon, 10000)

    def get_battery_status(self):
        """Runs headsetcontrol and parses its output."""
        
        # If the binary was not found at startup, skip execution
        if not self.headsetcontrol_path:
            return {"status": "error", "error": "Binary Missing"}
            
        try:
            # Use the pre-determined path
            result = subprocess.run(
                [self.headsetcontrol_path, '-b'], 
                capture_output=True,
                text=True,
                check=True
            )
            output = result.stdout
            
            level_match = re.search(r"Level:\s*(\d+%)", output)
            status_match = re.search(r"Status:\s*(BATTERY_CHARGING)", output)
            
            if not level_match:
                logger.warning("Battery status command successful but failed to parse battery level.")
                return {"status": "error", "error": "Parse Error"}

            level_str = level_match.group(1)
            numeric_level = int(level_str.replace('%', ''))
            is_charging = (status_match is not None)
            
            # Nombre genérico fijo
            device_name = "Headset"
            
            logger.debug(f"Status check: {level_str}, Charging: {is_charging}")

            return {
                "status": "ok",
                "level": numeric_level,
                "level_str": level_str,
                "is_charging": is_charging,
                "name": device_name
            }

        except subprocess.CalledProcessError:
            logger.error("HeadsetControl returned non-zero exit code (possibly device not ready or disconnected).")
            return {"status": "error", "error": "Disconnected"}
        except FileNotFoundError:
             return {"status": "error", "error": "Execution Failed"}


    def update_status(self):
        """Updates the icon, tooltip and checks for notification logic."""
        
        data = self.get_battery_status()
        
        # === PATHS ===
        # Busca la carpeta "icons" primero en el directorio local y luego en el sandbox
        script_dir = os.path.dirname(os.path.abspath(__file__))
        local_icon_dir = os.path.join(script_dir, "icons")
        flatpak_icon_dir = "/app/share/headset-battery-indicator/icons"
        icon_base_path = local_icon_dir if os.path.exists(local_icon_dir) else flatpak_icon_dir

        def load_icon(icon_name):
            """Carga un icono SVG desde la carpeta de iconos local."""
            icon_path = os.path.join(icon_base_path, f"{icon_name}.svg")
            if os.path.exists(icon_path):
                return QIcon(icon_path)
            return QIcon.fromTheme(icon_name)  # fallback si no existe
        
        # === HANDLE ERRORS ===
        if data["status"] == "error":
            if data["error"] == "Binary Missing":
                return 
            
            self.setIcon(load_icon("audio-headset-symbolic"))
            self.setToolTip(f"Headset: {data['error']}")
            self.info_name_action.setText("Headset")
            self.info_status_action.setText(f"Status: {data['error']}")
            self.notified_low_battery = False
            return

        # === NORMAL OPERATION ===
        level = data["level"]
        level_str = data["level_str"]
        icon_name = "audio-headset-symbolic"
        device_name = "Headset"  # Nombre fijo

        if data["is_charging"]:
            # Usar un nombre que existe para carga
            # NOTA: Los iconos de carga en KDE/Adwaita suelen ser battery-level-XX-charging
            icon_name = "battery-charging-symbolic" # Puedes usar un nombre más simple como fallback
            tooltip_status = f"Charging ({level_str})"
            selfity.notified_low_battery = False
        else:
            tooltip_status = f"Discharging ({level_str})"
            # Usar los nombres TEMÁTICOS CORRECTOS (e.g., battery-level-XX-symbolic)
            if level > 90: icon_name = "battery-level-100-symbolic"
            elif level > 70: icon_name = "battery-level-80-symbolic"  # Asumiendo que 80 es la barra más cercana
            elif level > 50: icon_name = "battery-level-60-symbolic"
            elif level > 30: icon_name = "battery-level-40-symbolic"
            elif level > 10: icon_name = "battery-level-20-symbolic"
            else: icon_name = "battery-low-symbolic"

            # --- Low Battery Notification ---
            if self.notify_enabled and level <= self.notify_threshold:
                if not self.notified_low_battery:
                    logger.warning(f"Low battery threshold reached: {level_str}. Notifying user.")
                    self.send_notification(
                        "Low Headset Battery",
                        f"{device_name} is at {level_str}."
                    )
                    self.run_headset_command(['-n', '1'])
                    self.notified_low_battery = True
            elif level > self.notify_threshold:
                self.notified_low_battery = False

        # === UPDATE UI ===
        self.setIcon(load_icon(icon_name))
        self.setToolTip(f"{device_name}\nStatus: {tooltip_status}")
        
        self.info_name_action.setText(device_name)
        self.info_status_action.setText(f"Status: {tooltip_status}")


# --- Main execution block ---
def main():
    """Main function to run the application."""
    
    if "-h" in sys.argv or "--help" in sys.argv:
        HELP_TEXT = textwrap.dedent("""
        Headset Battery Indicator
        
        A system tray icon to monitor headset battery levels,
        based on HeadsetControl.
        
        Usage:
          python3 headsetcontrol_tray.py [options]
        
        Options:
          -h, --help    Show this help message and exit.
          -debug        Run in debug mode, allowing interactive commands
                        (e.g., 'notification', 'setIcon [name]').
        
        This script is a frontend and requires 'headsetcontrol' to be installed.
        """)
        print(HELP_TEXT)
        sys.exit(0)

    debug_mode = "-debug" in sys.argv
    
    signal.signal(signal.SIGINT, signal.SIG_DFL)

    app = QApplication(sys.argv)
    
    app.setOrganizationName("MyScripts")
    app.setApplicationName("HeadsetBatteryIndicator")

    app.setQuitOnLastWindowClosed(False)

    if not QSystemTrayIcon.isSystemTrayAvailable():
        logger.critical("System tray not available. Exiting.")
        print("Error: System tray not available.")
        sys.exit(1)

    tray_icon = HeadsetBatteryTray(debug_mode=debug_mode)
    
    sys.exit(app.exec())


if __name__ == "__main__":
    main()