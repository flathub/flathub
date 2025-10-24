#!/usr/bin/env python3
"""
CliA - AI-Powered Terminal Assistant
GTK4 application with integrated terminal and AI chat interface
"""

import sys
import os

# Debug: Print typelib search paths
if os.getenv('FLATPAK_ID'):
    print("Running in Flatpak")
    print(f"GI_TYPELIB_PATH: {os.getenv('GI_TYPELIB_PATH')}")
    typelib_paths = []
    for path in ['/app/lib/girepository-1.0', '/usr/lib/girepository-1.0', '/usr/lib/x86_64-linux-gnu/girepository-1.0']:
        if os.path.exists(path):
            typelib_paths.append(path)
            files = [f for f in os.listdir(path) if 'Vte' in f]
            if files:
                print(f"Found in {path}: {files}")

import gi
gi.require_version('Gtk', '4.0')
gi.require_version('Adw', '1')

# Try to find VTE
try:
    gi.require_version('Vte', '3.91')
except ValueError as e:
    print(f"Error loading Vte: {e}")
    sys.exit(1)

from gi.repository import Gtk, Adw, Vte, GLib, Gdk, Pango
import json
import subprocess
import time
import threading
from pathlib import Path
from datetime import datetime
import requests


class ConfigManager:
    """Manages application configuration and API key storage"""
    
    def __init__(self):
        self.config_dir = Path.home() / ".config" / "clia"
        self.config_file = self.config_dir / "config.json"
        self.config_dir.mkdir(parents=True, exist_ok=True)
        self.config = self.load_config()
    
    def load_config(self):
        """Load configuration from file"""
        if self.config_file.exists():
            try:
                with open(self.config_file, 'r') as f:
                    return json.load(f)
            except Exception as e:
                print(f"Error loading config: {e}")
                return {}
        return {}
    
    def save_config(self):
        """Save configuration to file"""
        try:
            with open(self.config_file, 'w') as f:
                json.dump(self.config, f, indent=2)
            # Set file permissions to 600 (read/write only for owner)
            self.config_file.chmod(0o600)
        except Exception as e:
            print(f"Error saving config: {e}")
    
    def get_api_key(self):
        """Get stored API key"""
        return self.config.get('api_key', '')
    
    def set_api_key(self, api_key):
        """Set and save API key"""
        self.config['api_key'] = api_key
        self.save_config()
    
    def get_model(self):
        """Get stored model name"""
        return self.config.get('model', 'Qwen/Qwen3-Next-80B-A3B-Instruct')
    
    def set_model(self, model):
        """Set and save model name"""
        self.config['model'] = model
        self.save_config()

    def get_auto_allow_tools(self):
        """Get the auto-allow tools setting"""
        return self.config.get('auto_allow_tools', False)

    def set_auto_allow_tools(self, allow):
        """Set and save the auto-allow tools setting"""
        self.config['auto_allow_tools'] = allow
        self.save_config()

    def get_theme(self):
        """Get the color scheme setting"""
        return self.config.get('theme', 'system')

    def set_theme(self, theme):
        """Set and save the color scheme setting"""
        self.config['theme'] = theme
        self.save_config()


class DoMdManager:
    """Manages the Do.md file for AI to track tasks and thoughts"""
    
    def __init__(self, workspace_path=None):
        if workspace_path:
            self.do_md_path = Path(workspace_path) / "Do.md"
        else:
            self.do_md_path = Path.home() / ".clia" / "Do.md"
        
        self.do_md_path.parent.mkdir(parents=True, exist_ok=True)
        
        if not self.do_md_path.exists():
            self.do_md_path.write_text("# AI Work Log\n\n")
    
    def read(self):
        """Read current Do.md contents"""
        if self.do_md_path.exists():
            return self.do_md_path.read_text()
        return ""
    
    def write(self, content):
        """Write content to Do.md"""
        self.do_md_path.write_text(content)
    
    def append(self, content):
        """Append content to Do.md"""
        current = self.read()
        self.write(current + "\n" + content)


class ToolExecutor:
    """Executes AI tools on the terminal"""
    
    def __init__(self, terminal_widget, do_md_manager):
        self.terminal = terminal_widget
        self.do_md_manager = do_md_manager
    
    def execute_tool(self, tool_name, content):
        """Execute a tool and return success status"""
        try:
            if tool_name == "Input":
                return self.execute_input(content)
            elif tool_name == "Text":
                return self.execute_text(content)
            elif tool_name == "Do.md":
                return self.execute_domd(content)
            elif tool_name == "Success":
                return self.execute_success(content)
            else:
                return False, f"Unknown tool: {tool_name}"
        except Exception as e:
            return False, f"Error executing {tool_name}: {str(e)}"
    
    def execute_input(self, keys):
        """Send keyboard input to terminal"""
        keys = keys.strip()
        
        # Parse key combinations like "Control+C", "Alt+A", etc.
        if "+" in keys:
            parts = keys.split("+")
            modifiers = []
            key = parts[-1].strip()
            
            for modifier in parts[:-1]:
                modifier = modifier.strip().lower()
                if modifier in ["ctrl", "control"]:
                    modifiers.append("ctrl")
                elif modifier in ["alt", "meta"]:
                    modifiers.append("alt")
                elif modifier in ["shift"]:
                    modifiers.append("shift")
            
            # Handle Control key combinations
            if "ctrl" in modifiers:
                key_lower = key.lower()
                
                # Map Control+letter to control characters (Ctrl+A = 0x01, Ctrl+B = 0x02, etc.)
                if len(key_lower) == 1 and 'a' <= key_lower <= 'z':
                    ctrl_char = ord(key_lower) - ord('a') + 1
                    self.terminal.feed_child(bytes([ctrl_char]))
                    return True, f"Sent key: Ctrl+{key.upper()}"
                
                # Special control combinations
                elif key_lower in ["space", " "]:
                    self.terminal.feed_child(b'\x00')  # Ctrl+Space (NUL)
                    return True, f"Sent key: Ctrl+Space"
                elif key_lower in ["[", "{"]:
                    self.terminal.feed_child(b'\x1b')  # Ctrl+[ (ESC)
                    return True, f"Sent key: Ctrl+["
                elif key_lower in ["\\", "|"]:
                    self.terminal.feed_child(b'\x1c')  # Ctrl+\
                    return True, f"Sent key: Ctrl+\\"
                elif key_lower in ["]", "}"]:
                    self.terminal.feed_child(b'\x1d')  # Ctrl+]
                    return True, f"Sent key: Ctrl+]"
                elif key_lower in ["^", "~"]:
                    self.terminal.feed_child(b'\x1e')  # Ctrl+^
                    return True, f"Sent key: Ctrl+^"
                elif key_lower in ["_", "-"]:
                    self.terminal.feed_child(b'\x1f')  # Ctrl+_
                    return True, f"Sent key: Ctrl+_"
                elif key_lower == "?":
                    self.terminal.feed_child(b'\x7f')  # Ctrl+? (DEL)
                    return True, f"Sent key: Ctrl+?"
                else:
                    return False, f"Unsupported Control+{key} combination"
            
            # Handle Alt key combinations
            elif "alt" in modifiers:
                key_lower = key.lower()
                
                # Alt+key sends ESC followed by the key
                if len(key_lower) == 1:
                    self.terminal.feed_child(b'\x1b' + key.encode('utf-8'))
                    return True, f"Sent key: Alt+{key}"
                elif key_lower in ["enter", "return"]:
                    self.terminal.feed_child(b'\x1b\r')
                    return True, f"Sent key: Alt+Enter"
                else:
                    return False, f"Unsupported Alt+{key} combination"
            
            # Handle Shift combinations (for special keys)
            elif "shift" in modifiers:
                key_lower = key.lower()
                if key_lower == "tab":
                    self.terminal.feed_child(b'\x1b[Z')  # Shift+Tab (reverse tab)
                    return True, f"Sent key: Shift+Tab"
                else:
                    # For letters, just send uppercase
                    if len(key) == 1:
                        self.terminal.feed_child(key.upper().encode('utf-8'))
                        return True, f"Sent key: {key.upper()}"
                    return False, f"Unsupported Shift+{key} combination"
            
            return False, f"Unsupported key combination: {keys}"
        
        else:
            # Simple key press (no modifiers)
            key_lower = keys.lower()
            
            # Special keys
            if key_lower in ["enter", "return"]:
                self.terminal.feed_child(b'\r')
                return True, "Sent key: Enter"
            elif key_lower == "tab":
                self.terminal.feed_child(b'\t')
                return True, "Sent key: Tab"
            elif key_lower in ["esc", "escape"]:
                self.terminal.feed_child(b'\x1b')
                return True, "Sent key: Escape"
            elif key_lower == "backspace":
                self.terminal.feed_child(b'\x7f')
                return True, "Sent key: Backspace"
            elif key_lower == "delete":
                self.terminal.feed_child(b'\x1b[3~')
                return True, "Sent key: Delete"
            
            # Arrow keys
            elif key_lower == "up":
                self.terminal.feed_child(b'\x1b[A')
                return True, "Sent key: Up"
            elif key_lower == "down":
                self.terminal.feed_child(b'\x1b[B')
                return True, "Sent key: Down"
            elif key_lower == "right":
                self.terminal.feed_child(b'\x1b[C')
                return True, "Sent key: Right"
            elif key_lower == "left":
                self.terminal.feed_child(b'\x1b[D')
                return True, "Sent key: Left"
            
            # Home/End/PageUp/PageDown
            elif key_lower == "home":
                self.terminal.feed_child(b'\x1b[H')
                return True, "Sent key: Home"
            elif key_lower == "end":
                self.terminal.feed_child(b'\x1b[F')
                return True, "Sent key: End"
            elif key_lower in ["pageup", "pgup"]:
                self.terminal.feed_child(b'\x1b[5~')
                return True, "Sent key: PageUp"
            elif key_lower in ["pagedown", "pgdn", "pgdown"]:
                self.terminal.feed_child(b'\x1b[6~')
                return True, "Sent key: PageDown"
            
            # Insert
            elif key_lower == "insert":
                self.terminal.feed_child(b'\x1b[2~')
                return True, "Sent key: Insert"
            
            # Function keys (F1-F12)
            elif key_lower.startswith("f") and len(key_lower) <= 3:
                try:
                    fkey_num = int(key_lower[1:])
                    if 1 <= fkey_num <= 12:
                        fkey_sequences = {
                            1: b'\x1bOP',    2: b'\x1bOQ',    3: b'\x1bOR',    4: b'\x1bOS',
                            5: b'\x1b[15~',  6: b'\x1b[17~',  7: b'\x1b[18~',  8: b'\x1b[19~',
                            9: b'\x1b[20~', 10: b'\x1b[21~', 11: b'\x1b[23~', 12: b'\x1b[24~',
                        }
                        self.terminal.feed_child(fkey_sequences[fkey_num])
                        return True, f"Sent key: F{fkey_num}"
                except ValueError:
                    pass
            
            # Space
            elif key_lower == "space":
                self.terminal.feed_child(b' ')
                return True, "Sent key: Space"
            
            # Single character (letter, number, symbol, etc.)
            elif len(keys) == 1:
                self.terminal.feed_child(keys.encode('utf-8'))
                return True, f"Sent key: {keys}"
            
            # Unknown key
            else:
                return False, f"Unsupported key: {keys}"
        
        return True, f"Input executed: {keys}"
    
    def execute_text(self, text):
        """Send text to terminal"""
        text = text.strip()
        self.terminal.feed_child(text.encode('utf-8'))
        return True, f"Text sent: {text[:50]}..."
    
    def execute_domd(self, content):
        """Update Do.md file"""
        content = content.strip()
        
        # Simple implementation: replace entire content
        # Could be enhanced to support specific operations
        if content.startswith("APPEND:"):
            self.do_md_manager.append(content[7:].strip())
        else:
            self.do_md_manager.write(content)
        
        return True, "Do.md updated"
    
    def execute_success(self, message):
        """Mark task as successfully completed"""
        return True, f"Success: {message}"


class AIAgent:
    """Handles AI interactions with Hugging Face API"""
    
    def __init__(self, api_key="", model_name=""):
        self.api_key = api_key
        self.model_name = model_name or "Qwen/Qwen3-Next-80B-A3B-Instruct"
        self.conversation_history = []  # Will store full conversation
        self.system_prompt = None
        self.api_url = "https://router.huggingface.co/v1/chat/completions"
    
    def set_api_key(self, api_key):
        self.api_key = api_key
    
    def set_model(self, model_name):
        self.model_name = model_name
    
    def reset_conversation(self):
        """Reset conversation history for a new task"""
        self.conversation_history = []
        self.system_prompt = None
    
    def build_prompt(self, user_command, terminal_output, do_md_content, previous_actions):
        """Build the prompt to send to AI"""
        # System prompt (only created once per task)
        if self.system_prompt is None:
            self.system_prompt = f"""You are an AI assistant helping users accomplish tasks in a terminal. You must act like a REAL HUMAN carefully operating a terminal, not an automated script.

User's Task: {user_command}

You can use ONE tool per response in the format [ToolName]content[/ToolName].

Available tools:

1. [Wait]seconds[/Wait] - Wait for specified seconds (use this to observe terminal output)
   Example: [Wait]2[/Wait]

2. [Input]key[/Input] - Send keyboard input
   Supported inputs:
   - Control combinations: Ctrl+A through Ctrl+Z (e.g., Ctrl+C, Ctrl+D, Ctrl+Z)
   - Alt combinations: Alt+A through Alt+Z, Alt+Enter
   - Shift+Tab for reverse tab
   - Arrow keys: Up, Down, Left, Right
   - Special keys: Enter, Tab, Escape, Backspace, Delete
   - Navigation: Home, End, PageUp, PageDown, Insert
   - Function keys: F1 through F12
   - Single characters: any letter, number, or symbol
   - Space
   Examples: [Input]Ctrl+C[/Input], [Input]Enter[/Input], [Input]Up[/Input], [Input]Tab[/Input]

3. [Text]string[/Text] - Type text into terminal (for multiple characters)
   Example: [Text]ls -la[/Text]

4. [Do.md]content[/Do.md] - Update your work log
   - Use APPEND: prefix to append to existing content
   - Otherwise replaces entire content
   Example: [Do.md]APPEND: Step 1 completed[/Do.md]

5. [Success]message[/Success] - Mark task as complete
   Example: [Success]File successfully created[/Success]

CRITICAL RULES - YOU MUST FOLLOW THESE LIKE A REAL HUMAN:

1. **WORK EXTREMELY SLOWLY AND CAREFULLY**
   - Take ONE tiny step at a time
   - After EVERY action, wait and observe the result
   - Never rush or skip verification steps

2. **THINK BEFORE YOU ACT**
   - Carefully analyze the current terminal output
   - Consider what could go wrong
   - Double-check your next action makes sense
   - Explain your reasoning clearly before taking action

3. **OBSERVE AND VERIFY**
   - After typing a command, use [Wait] to see the output
   - After pressing Enter, use [Wait] to see the result
   - Check if your action had the expected effect
   - If something unexpected happens, pause and reconsider

4. **USE Do.md RELIGIOUSLY**
   - Before starting, write your plan
   - After each step, update your progress
   - Track what worked and what didn't
   - Use it to think through problems

5. **ONE TOOL PER RESPONSE - NO EXCEPTIONS**
   - You can ONLY use ONE tool in each response
   - Don't try to chain actions
   - Each response = one deliberate action

6. **SEPARATE TYPING FROM EXECUTING**
   - First use [Text] to type a command
   - Then in NEXT response, use [Input]Enter[/Input] to execute it
   - Then in NEXT response, use [Wait] to observe the result
   - This mimics how a real human works

7. **BE CONSERVATIVE AND SAFE**
   - If unsure, use [Wait] to observe more
   - If something looks wrong, stop and analyze
   - Don't proceed if you're not 100% certain
   - It's better to be slow than to make mistakes

8. **ONLY USE [Success] WHEN TRULY DONE**
   - Verify the task is completely finished
   - Check that the output confirms success
   - Don't assume - wait and verify

Example of GOOD behavior:
- Response 1: "I need to list files. Let me type the ls command." → [Text]ls -la[/Text]
- Response 2: "Now I'll execute it." → [Input]Enter[/Input]
- Response 3: "Let me wait to see the output." → [Wait]2[/Wait]
- Response 4: "I can see the file list. Let me update my notes." → [Do.md]APPEND: Listed files successfully[/Do.md]

Example of BAD behavior (DON'T DO THIS):
- Response 1: "I'll type and execute ls" → [Text]ls -la[/Text] (Missing the Enter step!)

9. **CRITICAL: LANGUAGE MATCHING - THIS IS MANDATORY**
   - **YOU MUST ALWAYS RESPOND IN THE SAME LANGUAGE THE USER USED IN THEIR COMMAND**
   - This is NOT optional - if the user writes in Korean (한국어), you MUST respond in Korean
   - If the user writes in English, you MUST respond in English
   - If the user writes in Japanese (日本語), you MUST respond in Japanese
   - Match the user's language in ALL parts of your response: reasoning, explanations, Do.md updates, everything
   - The ONLY exception is if the user explicitly asks you to use a different language
   - This rule applies to EVERY SINGLE RESPONSE you give
   - **ALWAYS CHECK: What language did the user use in their original task/command? Use that exact language.**

Respond with your careful reasoning and then EXACTLY ONE tool to use."""
        
        # Build current state message
        current_state = f"""Current Terminal Output:
{terminal_output}

Do.md (your work log):
{do_md_content}

What is your next action?"""
        
        return current_state
    
    def call_api(self, prompt):
        """Call Hugging Face API using requests with full conversation history"""
        if not self.api_key:
            return None, "API key not set"
        
        try:
            headers = {
                "Authorization": f"Bearer {self.api_key}",
                "Content-Type": "application/json"
            }
            
            # Build messages array with full conversation history
            messages = []
            
            # Add system prompt
            if self.system_prompt:
                messages.append({
                    "role": "system",
                    "content": self.system_prompt
                })
            
            # Add all previous conversation history
            messages.extend(self.conversation_history)
            
            # Add current user prompt
            messages.append({
                "role": "user",
                "content": prompt
            })
            
            payload = {
                "messages": messages,
                "model": self.model_name,
                "max_tokens": 2000,
                "temperature": 0.7,
                "top_p": 0.95
            }
            
            response = requests.post(self.api_url, headers=headers, json=payload)
            response.raise_for_status()
            
            result = response.json()
            
            # Extract the generated text from the response
            if result and "choices" in result and len(result["choices"]) > 0:
                generated_text = result["choices"][0]["message"]["content"]
                
                # Add to conversation history
                self.conversation_history.append({
                    "role": "user",
                    "content": prompt
                })
                self.conversation_history.append({
                    "role": "assistant",
                    "content": generated_text
                })
                
                return generated_text, None
            else:
                return None, "No response from API"
                
        except Exception as e:
            return None, f"API Error: {str(e)}"
    
    def parse_tool_from_response(self, response):
        """Parse tool usage from AI response"""
        import re
        
        # Look for [ToolName]content[/ToolName] pattern
        pattern = r'\[([\w.]+)\](.*?)\[/\1\]'
        match = re.search(pattern, response, re.DOTALL)
        
        if match:
            tool_name = match.group(1)
            content = match.group(2)
            reasoning = response[:match.start()].strip()
            return tool_name, content, reasoning
        
        return None, None, response


class SettingsDialog(Gtk.Window):
    """Settings dialog for API key configuration"""
    
    def __init__(self, parent, config_manager):
        super().__init__()
        self.set_transient_for(parent)
        self.set_modal(True)
        self.set_title("API Settings")
        self.set_default_size(450, 200)
        
        self.config_manager = config_manager
        
        # Main box
        main_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=12)
        main_box.set_margin_start(20)
        main_box.set_margin_end(20)
        main_box.set_margin_top(20)
        main_box.set_margin_bottom(20)
        
        # Title
        title = Gtk.Label()
        title.set_markup("<big><b>API Key Settings</b></big>")
        title.set_halign(Gtk.Align.START)
        main_box.append(title)
        
        # API Key section
        api_frame = Gtk.Frame()
        api_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
        api_box.set_margin_start(12)
        api_box.set_margin_end(12)
        api_box.set_margin_top(12)
        api_box.set_margin_bottom(12)
        
        api_label = Gtk.Label(label="Hugging Face API Key")
        api_label.set_halign(Gtk.Align.START)
        api_label.set_markup("<b>Hugging Face API Key</b>")
        
        self.api_entry = Gtk.Entry()
        self.api_entry.set_placeholder_text("hf_...")
        self.api_entry.set_visibility(False)
        self.api_entry.set_input_purpose(Gtk.InputPurpose.PASSWORD)
        self.api_entry.set_text(config_manager.get_api_key())
        
        # Info label
        info_label = Gtk.Label()
        info_label.set_markup("<small>You can select the model on the main screen.</small>")
        info_label.set_halign(Gtk.Align.START)
        info_label.add_css_class("dim-label")
        
        api_box.append(api_label)
        api_box.append(self.api_entry)
        api_box.append(info_label)
        api_frame.set_child(api_box)
        main_box.append(api_frame)
        
        # Auto-allow tools section
        auto_allow_frame = Gtk.Frame()
        auto_allow_frame.set_margin_top(10)
        auto_allow_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
        auto_allow_box.set_margin_start(12)
        auto_allow_box.set_margin_end(12)
        auto_allow_box.set_margin_top(12)
        auto_allow_box.set_margin_bottom(12)

        auto_allow_hbox = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
        
        auto_allow_label = Gtk.Label(label="Auto-allow AI interaction")
        auto_allow_label.set_halign(Gtk.Align.START)
        auto_allow_label.set_hexpand(True)
        auto_allow_label.set_tooltip_text("Allow AI to use terminal interaction tools (Input, Text) without asking for confirmation.")
        
        self.auto_allow_switch = Gtk.Switch()
        self.auto_allow_switch.set_active(config_manager.get_auto_allow_tools())
        self.auto_allow_switch.connect("notify::active", self.on_auto_allow_toggled)

        auto_allow_hbox.append(auto_allow_label)
        auto_allow_hbox.append(self.auto_allow_switch)
        auto_allow_box.append(auto_allow_hbox)
        auto_allow_frame.set_child(auto_allow_box)
        main_box.append(auto_allow_frame)
        
        # Theme selection
        theme_frame = Gtk.Frame()
        theme_frame.set_margin_top(10)
        theme_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
        theme_box.set_margin_start(12)
        theme_box.set_margin_end(12)
        theme_box.set_margin_top(12)
        theme_box.set_margin_bottom(12)
        
        theme_hbox = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
        theme_label = Gtk.Label(label="Color Scheme")
        theme_label.set_halign(Gtk.Align.START)
        theme_label.set_hexpand(True)
        
        self.theme_combo = Gtk.ComboBoxText()
        self.theme_combo.append_text("System")
        self.theme_combo.append_text("Light")
        self.theme_combo.append_text("Dark")
        
        current_theme = self.config_manager.get_theme()
        if current_theme == 'light':
            self.theme_combo.set_active(1)
        elif current_theme == 'dark':
            self.theme_combo.set_active(2)
        else: # system
            self.theme_combo.set_active(0)
            
        theme_hbox.append(theme_label)
        theme_hbox.append(self.theme_combo)
        theme_box.append(theme_hbox)
        theme_frame.set_child(theme_box)
        main_box.append(theme_frame)
        
        # Buttons
        button_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=6)
        button_box.set_halign(Gtk.Align.END)
        button_box.set_margin_top(20)
        
        cancel_button = Gtk.Button(label="Cancel")
        cancel_button.connect("clicked", lambda w: self.close())
        
        save_button = Gtk.Button(label="Save")
        save_button.add_css_class("suggested-action")
        save_button.connect("clicked", self.on_save)
        
        button_box.append(cancel_button)
        button_box.append(save_button)
        main_box.append(button_box)
        
        self.set_child(main_box)
    
    def on_save(self, button):
        """Save settings"""
        api_key = self.api_entry.get_text().strip()
        
        if not api_key:
            dialog = Gtk.AlertDialog()
            dialog.set_message("Please enter an API key")
            dialog.set_buttons(["OK"])
            dialog.choose(self, None, None, None)
            return
        
        self.config_manager.set_api_key(api_key)
        
        # Save theme setting
        theme_text = self.theme_combo.get_active_text().lower()
        self.config_manager.set_theme(theme_text)
        
        # Apply the theme immediately
        app = self.get_transient_for().get_application()
        if app:
            app.apply_theme()
        
        self.close()

    def on_auto_allow_toggled(self, switch, a):
        is_active = switch.get_active()
        self.config_manager.set_auto_allow_tools(is_active)
        if is_active:
            # Show a warning when enabling
            dialog = Gtk.AlertDialog()
            dialog.set_modal(True)
            dialog.set_message("Security Warning")
            dialog.set_detail("Enabling auto-allow means the AI can execute any command in your terminal without your permission. This could be dangerous. Only enable this if you fully trust the AI and the model you are using.")
            dialog.set_buttons(["OK"])
            dialog.choose(self, None, None)


class ToolConfirmationDialog:
    """Handles the tool confirmation dialog using Adw.MessageDialog."""

    def __init__(self, parent, tool_name, content, config_manager):
        self.parent = parent
        self.tool_name = tool_name
        self.content = content
        self.config_manager = config_manager
        self.response = "deny"
        self.response_event = threading.Event()

    def run(self):
        """Shows the dialog and blocks the calling thread until a response is received."""
        GLib.idle_add(self.show_dialog)
        self.response_event.wait()
        return self.response

    def show_dialog(self):
        """Creates and presents the Adw.MessageDialog on the main thread."""
        dialog = Adw.MessageDialog(
            transient_for=self.parent,
            modal=True,
            heading=f"AI wants to use the '{self.tool_name}' tool",
            body=self.content
        )
        
        self.auto_allow_check = Gtk.CheckButton(label="Auto-allow future interactions")
        self.auto_allow_check.set_tooltip_text("If checked, all future terminal interactions in this session will be allowed automatically.")
        
        extra_content = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=12)
        extra_content.append(Gtk.Separator())
        extra_content.append(self.auto_allow_check)
        
        dialog.set_extra_child(extra_content)
        
        dialog.add_response("deny", "Deny")
        dialog.add_response("allow", "Allow")
        dialog.set_response_appearance("allow", Adw.ResponseAppearance.SUGGESTED)
        dialog.set_default_response("allow")
        
        dialog.connect("response", self.on_response)
        dialog.present()
        return False # So GLib.idle_add doesn't call it again

    def on_response(self, dialog, response_id):
        """Handles the user's response from the dialog."""
        if response_id == "allow":
            self.response = "allow"
            if self.auto_allow_check.get_active():
                if not self.config_manager.get_auto_allow_tools():
                    self.show_warning()
                self.config_manager.set_auto_allow_tools(True)
        else:
            self.response = "deny"
        
        dialog.close()
        self.response_event.set()

    def show_warning(self):
        """Shows a security warning dialog."""
        warning = Adw.MessageDialog(
            transient_for=self.parent,
            modal=True,
            heading="Security Warning",
            body="Enabling auto-allow means the AI can execute any command in your terminal without your permission. This could be dangerous.",
        )
        warning.add_response("ok", "OK")
        warning.set_default_response("ok")
        warning.connect("response", lambda d, r: d.close())
        warning.present()


class ChatPanel(Gtk.Box):
    """Chat interface panel"""
    
    def __init__(self, on_send_callback, on_stop_callback, config_manager):
        super().__init__(orientation=Gtk.Orientation.VERTICAL, spacing=6)
        self.on_send_callback = on_send_callback
        self.on_stop_callback = on_stop_callback
        self.config_manager = config_manager
        
        self.set_hexpand(True)
        self.set_vexpand(True)
        
        # Header with clear button
        header_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=6)
        header_box.set_margin_start(10)
        header_box.set_margin_end(10)
        header_box.set_margin_top(6)
        
        chat_label = Gtk.Label()
        chat_label.set_markup("<b> </b>")
        chat_label.set_halign(Gtk.Align.START)
        chat_label.set_hexpand(True)

        clear_button = Gtk.Button.new_from_icon_name("edit-clear-all-symbolic")
        clear_button.set_tooltip_text("Clear Chat")
        clear_button.connect("clicked", self.on_clear_chat)
        clear_button.set_halign(Gtk.Align.END)
        
        header_box.append(chat_label)
        header_box.append(clear_button)
        self.append(header_box)
        
        # Chat display area
        self.scrolled_window = Gtk.ScrolledWindow()
        self.scrolled_window.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.AUTOMATIC)
        self.scrolled_window.set_vexpand(True)
        self.scrolled_window.set_hexpand(True)
        self.scrolled_window.set_size_request(300, 200)  # Set minimum size
        
        self.chat_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=12)
        self.chat_box.set_margin_start(10)
        self.chat_box.set_margin_end(10)
        self.chat_box.set_margin_top(10)
        self.chat_box.set_margin_bottom(10)
        self.chat_box.set_valign(Gtk.Align.END) # To make new messages appear at the bottom
        
        self.scrolled_window.set_child(self.chat_box)
        
        self.append(self.scrolled_window)
        
        # Model selection area
        model_container = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
        model_container.set_margin_start(6)
        model_container.set_margin_end(6)
        model_container.set_margin_top(6)
        
        # First row: label and dropdown
        model_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=6)
        
        model_label = Gtk.Label(label="Model:")
        model_label.set_width_chars(6)
        
        # Dropdown with common models
        self.model_combo = Gtk.ComboBoxText()
        self.model_combo.set_hexpand(True)
        
        model_list = [
            "Qwen/Qwen3-Next-80B-A3B-Instruct",
            "deepseek-ai/DeepSeek-V3.2-Exp",
            "zai-org/GLM-4.6",
            "Qwen/Qwen3-235B-A22B-Thinking-2507",
            "openai/gpt-oss-120b",
            "meta-llama/Llama-4-Maverick-17B-128E-Instruct",
            "Qwen/Qwen3-Next-80B-A3B-Thinking",
            "moonshotai/Kimi-K2-Instruct",
            "openai/gpt-oss-20b",
            "google/gemma-3-27b-it",
            "HuggingFaceTB/SmolLM3-3B"
        ]
        
        for model in model_list:
            self.model_combo.append_text(model)
        self.model_combo.append_text("Custom...")
        
        # Set saved model
        saved_model = config_manager.get_model()
        found = False
        for i, model_text in enumerate(model_list):
            if model_text == saved_model:
                self.model_combo.set_active(i)
                found = True
                break
        
        if not found:
            self.model_combo.set_active(len(model_list))  # Custom
        
        self.model_combo.connect("changed", self.on_model_changed)
        
        model_box.append(model_label)
        model_box.append(self.model_combo)
        
        # Second row: custom model entry (full width, hidden by default)
        self.custom_model_entry = Gtk.Entry()
        self.custom_model_entry.set_placeholder_text("Enter custom model name")
        self.custom_model_entry.set_hexpand(True)
        self.custom_model_entry.connect("changed", self.on_custom_model_changed)
        if not found:
            self.custom_model_entry.set_text(saved_model)
            self.custom_model_entry.set_visible(True)
        else:
            self.custom_model_entry.set_visible(False)
        
        model_container.append(model_box)
        model_container.append(self.custom_model_entry)
        
        self.append(model_container)
        
        # Input area
        input_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=6)
        input_box.set_margin_start(6)
        input_box.set_margin_end(6)
        input_box.set_margin_bottom(6)
        
        # Use a TextView for multiline input, inside a ScrolledWindow for border and scrolling
        input_frame = Gtk.Frame()
        input_frame.set_hexpand(True)
        
        self.input_view = Gtk.TextView()
        self.input_view.set_wrap_mode(Gtk.WrapMode.WORD_CHAR)
        self.input_view.set_vexpand(False)
        self.input_view.set_accepts_tab(False)
        self.input_view.get_buffer().connect("changed", self.on_input_buffer_changed)
        self.input_view.set_pixels_above_lines(5)
        self.input_view.set_pixels_below_lines(5)
        self.input_view.set_left_margin(10)
        self.input_view.set_right_margin(10)
        
        # Handle Enter key press
        key_controller = Gtk.EventControllerKey.new()
        key_controller.connect("key-pressed", self.on_key_pressed)
        self.input_view.add_controller(key_controller)
        
        input_frame.set_child(self.input_view)

        self.action_button = Gtk.Button(label="Send")
        self.action_button.connect("clicked", self.on_action_clicked)
        self.action_button.set_valign(Gtk.Align.END)
        
        input_box.append(input_frame)
        input_box.append(self.action_button)
        
        self.append(input_box)
        
        # Initial size setting
        self.on_input_buffer_changed(self.input_view.get_buffer())
        
        self.active_waits = {}
        
        # CSS for chat bubbles
        css_provider = Gtk.CssProvider()
        css_provider.load_from_string("""
            .chat-bubble {
                padding: 10px;
                border-radius: 12px;
                max-width: 80%;
            }
            .user-bubble {
                background-color: #3584e4;
                color: white;
            }
            .ai-bubble, .system-bubble, .error-bubble {
                background-color: @window_bg_color;
                border: 1px solid @window_border_color;
            }
            .success-bubble {
                background-color: #4caf50;
                color: white;
            }
            .code-snippet {
                background-color: rgba(0, 0, 0, 0.1);
                padding: 2px 6px;
                border-radius: 4px;
                font-family: monospace;
            }
            :root.dark .code-snippet {
                background-color: rgba(255, 255, 255, 0.1);
            }
            .keyboard-key {
                background-color: @window_bg_color;
                border: 1px solid @window_border_color;
                border-radius: 4px;
                padding: 4px 8px;
                margin: 0 2px;
                box-shadow: 0 2px 0px rgba(0, 0, 0, 0.2);
            }
            .spinning-icon {
                animation: spin 2s infinite linear;
            }
            spinner {
                animation: spin 2s infinite linear;
            }
            @keyframes spin {
                to { -gtk-icon-transform: rotate(1turn); }
            }
        """)
        Gtk.StyleContext.add_provider_for_display(
            Gdk.Display.get_default(),
            css_provider,
            Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
        )

        self.thinking_spinner = None
    
    def on_model_changed(self, combo):
        """Handle model selection change"""
        if combo.get_active_text() == "Custom...":
            self.custom_model_entry.set_visible(True)
        else:
            self.custom_model_entry.set_visible(False)
        
        # Save model selection
        model = self.get_model_name()
        if model:
            self.config_manager.set_model(model)
    
    def on_custom_model_changed(self, entry):
        """Handle custom model entry change"""
        # Save custom model name when user types
        if self.model_combo.get_active_text() == "Custom...":
            model = entry.get_text().strip()
            if model:
                self.config_manager.set_model(model)
    
    def get_model_name(self):
        """Get selected model name"""
        if self.model_combo.get_active_text() == "Custom...":
            return self.custom_model_entry.get_text().strip()
        return self.model_combo.get_active_text()
    
    def get_api_key(self):
        """Get API key"""
        return self.config_manager.get_api_key()
    
    def on_action_clicked(self, widget):
        """Handle send/stop button click"""
        if self.action_button.get_label() == "Send":
            buffer = self.input_view.get_buffer()
            start_iter, end_iter = buffer.get_bounds()
            command = buffer.get_text(start_iter, end_iter, True)
            if command.strip():
                if self.on_send_callback(command):
                    self.add_message("User", command)
                    buffer.set_text("", -1)
        else:
            self.on_stop_callback()

    def on_key_pressed(self, controller, keyval, keycode, state):
        """Handle key presses in the input view"""
        if keyval == Gdk.KEY_Return and not (state & Gdk.ModifierType.SHIFT_MASK):
            # Enter pressed without Shift: send message
            self.on_action_clicked(self.action_button)
            return True  # Stop the event from propagating
        return False

    def on_clear_chat(self, button):
        """Show a confirmation dialog before clearing the chat."""
        dialog = Adw.MessageDialog(
            transient_for=self.get_root(),
            modal=True,
            heading="Clear Chat History?",
            body="Are you sure you want to permanently delete all messages in this chat?"
        )
        dialog.add_response("cancel", "Cancel")
        dialog.add_response("clear", "Clear")
        dialog.set_response_appearance("clear", Adw.ResponseAppearance.DESTRUCTIVE)
        dialog.set_default_response("cancel")
        
        dialog.connect("response", self.on_clear_chat_response)
        dialog.present()

    def on_clear_chat_response(self, dialog, response_id):
        """Handle the response from the clear chat confirmation dialog."""
        if response_id == "clear":
            while child := self.chat_box.get_first_child():
                self.chat_box.remove(child)
        dialog.close()

    def on_input_buffer_changed(self, buffer):
        """Auto-resize the text view based on content"""
        line_count = buffer.get_line_count()
        num_lines = min(max(1, line_count), 5)
        
        # Calculate height based on font metrics
        pango_context = self.input_view.get_pango_context()
        font_metrics = pango_context.get_metrics(None)
        line_height = (font_metrics.get_ascent() + font_metrics.get_descent()) / Pango.SCALE
        
        # Get vertical padding/border of the text view
        style_context = self.input_view.get_style_context()
        border = style_context.get_border()
        padding = style_context.get_padding()
        v_spacing = border.top + border.bottom + padding.top + padding.bottom
        
        new_height = int(num_lines * line_height + v_spacing)
        self.input_view.set_size_request(-1, new_height)
    
    def set_processing_state(self, is_processing):
        """Toggle button between Send and Stop state"""
        if is_processing:
            self.action_button.set_label("Stop")
            self.action_button.get_style_context().add_class("destructive-action")
            self.input_view.set_editable(False)
            self.input_view.set_sensitive(False)
        else:
            self.action_button.set_label("Send")
            self.action_button.get_style_context().remove_class("destructive-action")
            self.input_view.set_editable(True)
            self.input_view.set_sensitive(True)

    def add_message(self, sender, message):
        """Add message to chat display as a bubble"""
        bubble = Gtk.Label(label=message)
        bubble.set_wrap(True)
        bubble.set_wrap_mode(Pango.WrapMode.WORD_CHAR)
        bubble.set_xalign(0)
        bubble.set_halign(Gtk.Align.START)
        bubble.add_css_class("chat-bubble")

        if sender.lower() == "user":
            bubble.add_css_class("user-bubble")
            box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
            box.set_halign(Gtk.Align.END)
            box.append(bubble)
        else:
            bubble.add_css_class(f"{sender.lower()}-bubble")
            box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
            box.set_halign(Gtk.Align.START)
            box.append(bubble)
            
        self.chat_box.append(box)
        
        # Auto-scroll to bottom
        GLib.timeout_add(50, self._scroll_to_bottom)

    def _scroll_to_bottom(self):
        """Scrolls the chat view to the bottom."""
        adj = self.scrolled_window.get_vadjustment()
        # By setting the value to the upper limit, we scroll to the end.
        # The adjustment will clamp this to upper - page_size internally.
        adj.set_value(adj.get_upper())
        return False

    def add_tool_execution(self, tool_name, content, result):
        """Add tool execution result to chat"""
        # Special display for Text tool
        if tool_name == "Text":
            bubble = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=6)
            bubble.add_css_class("chat-bubble")
            bubble.add_css_class("ai-bubble")
            
            code_label = Gtk.Label(label=content)
            code_label.add_css_class("code-snippet")
            code_label.set_xalign(0)
            bubble.append(code_label)

            box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
            box.set_halign(Gtk.Align.START)
            box.append(bubble)
            self.chat_box.append(box)
            GLib.timeout_add(50, self._scroll_to_bottom)
            return

        # Special display for Input tool
        if tool_name == "Input":
            bubble = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=6)
            bubble.add_css_class("chat-bubble")
            bubble.add_css_class("ai-bubble")
            
            key_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
            keys = content.split('+')
            for key in keys:
                key_label = Gtk.Label(label=key.strip())
                key_label.add_css_class("keyboard-key")
                key_box.append(key_label)
            bubble.append(key_box)

            box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
            box.set_halign(Gtk.Align.START)
            box.append(bubble)
            self.chat_box.append(box)
            GLib.timeout_add(50, self._scroll_to_bottom)
            return

        tool_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
        tool_box.add_css_class("chat-bubble")
        tool_box.add_css_class("system-bubble")

        header = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=6)
        icon = Gtk.Image.new_from_icon_name("system-run-symbolic")
        header.append(icon)
        title = Gtk.Label()
        title.set_markup(f"<b>Tool: {tool_name}</b>")
        title.set_halign(Gtk.Align.START)
        header.append(title)
        tool_box.append(header)

        if content:
            content_label = Gtk.Label(label=f"Content: {content}")
            content_label.set_wrap(True)
            content_label.set_xalign(0)
            content_label.set_halign(Gtk.Align.START)
            tool_box.append(content_label)

        result_label = Gtk.Label(label=f"→ {result}")
        result_label.set_wrap(True)
        result_label.set_xalign(0)
        result_label.set_halign(Gtk.Align.START)
        tool_box.append(result_label)
        
        box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
        box.set_halign(Gtk.Align.START)
        box.append(tool_box)

        self.chat_box.append(box)
        GLib.timeout_add(50, self._scroll_to_bottom)
    
    def add_success_message(self, message):
        """Add success message to chat"""
        success_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=6)
        success_box.add_css_class("chat-bubble")
        success_box.add_css_class("success-bubble")
        
        icon = Gtk.Image.new_from_icon_name("emblem-ok-symbolic")
        success_box.append(icon)
        
        label = Gtk.Label(label=message)
        label.set_wrap(True)
        label.set_xalign(0)
        success_box.append(label)

        box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
        box.set_halign(Gtk.Align.START)
        box.append(success_box)

        self.chat_box.append(box)
        GLib.timeout_add(50, self._scroll_to_bottom)

    def show_thinking_indicator(self):
        """Show a spinner while the AI is thinking"""
        if self.thinking_spinner is not None:
            return

        spinner = Gtk.Spinner()
        spinner.start()
        
        spinner_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
        spinner_box.set_halign(Gtk.Align.START)
        spinner_box.add_css_class("chat-bubble")
        spinner_box.add_css_class("system-bubble")
        spinner_box.append(spinner)
        
        self.thinking_spinner = spinner_box
        self.chat_box.append(self.thinking_spinner)
        GLib.timeout_add(50, self._scroll_to_bottom)

    def hide_thinking_indicator(self):
        """Hide the thinking spinner"""
        if self.thinking_spinner is not None:
            self.chat_box.remove(self.thinking_spinner)
            self.thinking_spinner = None

    def show_wait_countdown(self, seconds, completion_event):
        """Add a real-time countdown for the Wait tool."""
        wait_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=6)
        wait_box.add_css_class("chat-bubble")
        wait_box.add_css_class("system-bubble")

        icon = Gtk.Image.new_from_icon_name("document-open-recent-symbolic")
        icon.add_css_class("spinning-icon")
        
        label = Gtk.Label()
        
        wait_box.append(icon)
        wait_box.append(label)

        container = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
        container.set_halign(Gtk.Align.START)
        container.append(wait_box)
        self.chat_box.append(container)
        
        wait_id = id(completion_event)
        self.active_waits[wait_id] = {
            "label": label,
            "icon": icon,
            "remaining": seconds,
            "event": completion_event
        }
        
        # Initial text update
        self.update_wait_countdown(wait_id)
        
        GLib.timeout_add_seconds(1, self.update_wait_countdown, wait_id)
        GLib.timeout_add(50, self._scroll_to_bottom)

    def update_wait_countdown(self, wait_id):
        """Callback to update the countdown label every second."""
        if wait_id not in self.active_waits:
            return False

        wait_data = self.active_waits[wait_id]
        remaining = wait_data["remaining"]
        
        wait_data["label"].set_text(f"Waiting for {int(remaining)} seconds...")

        if remaining <= 0:
            wait_data["icon"].remove_css_class("spinning-icon")
            wait_data["label"].set_text("Wait complete.")
            wait_data["event"].set()
            del self.active_waits[wait_id]
            return False

        wait_data["remaining"] -= 1
        return True


class TerminalPanel(Gtk.Box):
    """Terminal panel with VTE widget"""
    
    def __init__(self):
        super().__init__(orientation=Gtk.Orientation.VERTICAL)
        
        self.set_hexpand(True)
        self.set_vexpand(True)
        
        # Create VTE terminal
        self.terminal = Vte.Terminal()
        self.terminal.set_hexpand(True)
        self.terminal.set_vexpand(True)
        self.terminal.set_size_request(400, 300)  # Set minimum size
        
        # Check if running in Flatpak and spawn appropriate shell
        if os.getenv('FLATPAK_ID'):
            # Running in Flatpak - use flatpak-spawn to run host shell
            # This allows access to the host system instead of the sandbox
            working_dir = os.path.expanduser("~")
            
            # Use flatpak-spawn --host to execute the host shell
            # This gives us access to the actual user's environment
            self.terminal.spawn_async(
                Vte.PtyFlags.DEFAULT,
                working_dir,
                ["flatpak-spawn", "--host", "--watch-bus", "bash", "-l"],
                None,  # environment
                GLib.SpawnFlags.DO_NOT_REAP_CHILD,
                None,  # child_setup
                None,  # child_setup_data
                -1,    # timeout
                None,  # cancellable
                None,  # callback
                None   # user_data
            )
        else:
            # Not in Flatpak - spawn shell normally
            self.terminal.spawn_async(
                Vte.PtyFlags.DEFAULT,
                os.environ.get("HOME"),
                [os.environ.get("SHELL", "/bin/bash")],
                [],
                GLib.SpawnFlags.DO_NOT_REAP_CHILD,
                None,
                None,
                -1,
                None,
                None,
                None
            )
        
        scrolled = Gtk.ScrolledWindow()
        scrolled.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        scrolled.set_vexpand(True)
        scrolled.set_hexpand(True)
        scrolled.set_child(self.terminal)
        
        self.append(scrolled)
    
    def get_visible_text(self):
        """Get currently visible terminal text (only what's actually on screen)"""
        try:
            import tempfile
            import os
            from gi.repository import Gio
            
            # Get terminal widget's actual pixel dimensions
            widget_height = self.terminal.get_height()
            widget_width = self.terminal.get_width()
            
            # Get character cell dimensions
            char_height = self.terminal.get_char_height()
            char_width = self.terminal.get_char_width()
            
            # Calculate actual visible rows and columns based on widget size
            if char_height > 0 and char_width > 0:
                visible_rows = widget_height // char_height
                visible_cols = widget_width // char_width
            else:
                # Fallback to terminal's row/col count
                visible_rows = self.terminal.get_row_count()
                visible_cols = self.terminal.get_column_count()
            
            print(f"DEBUG: Visible - rows: {visible_rows}, cols: {visible_cols}")
            
            # Create a temporary file to capture terminal contents
            with tempfile.NamedTemporaryFile(mode='w+', delete=False, suffix='.txt') as tmp:
                tmp_path = tmp.name
            
            try:
                # Create a GFile for the temp file
                gfile = Gio.File.new_for_path(tmp_path)
                
                # Create an output stream from the file
                # write_contents_sync expects a Gio.OutputStream, not a GFile
                output_stream = gfile.replace(
                    None,  # etag
                    False,  # make_backup
                    Gio.FileCreateFlags.NONE,
                    None  # cancellable
                )
                
                # Write terminal contents to the output stream
                # This is the most reliable method in VTE 3.91+
                success = self.terminal.write_contents_sync(
                    output_stream,
                    Vte.WriteFlags.DEFAULT,
                    None  # cancellable
                )
                
                # Close the output stream
                output_stream.close(None)
                
                if success:
                    # Read the file
                    with open(tmp_path, 'r', encoding='utf-8', errors='replace') as f:
                        terminal_text = f.read()
                    
                    print(f"DEBUG: Successfully read {len(terminal_text)} characters from file")
                    
                    # Split by lines and get only the visible portion
                    lines = terminal_text.split('\n')
                    
                    # Get approximately the last visible_rows lines
                    # This represents what's actually visible on screen
                    if len(lines) > visible_rows:
                        visible_lines = lines[-visible_rows:]
                        terminal_text = '\n'.join(visible_lines)
                    
                    print(f"DEBUG: After filtering - {len(terminal_text)} characters, {len(lines)} total lines")
                    print(f"DEBUG: First 200 chars: {repr(terminal_text[:200])}")
                    
                    if terminal_text.strip():
                        return terminal_text
                    else:
                        return "(Terminal is empty or contains only whitespace)"
                else:
                    print("DEBUG: write_contents_sync returned False")
                    return "(Failed to write terminal contents)"
                    
            finally:
                # Clean up temp file
                if os.path.exists(tmp_path):
                    try:
                        os.unlink(tmp_path)
                    except:
                        pass
                
        except Exception as e:
            print(f"Error capturing terminal text: {e}")
            import traceback
            traceback.print_exc()
            return f"(Error reading terminal: {str(e)})"


class MainWindow(Adw.ApplicationWindow):
    """Main application window"""
    
    def __init__(self, app):
        super().__init__(application=app)
        
        self.set_title("CliA - AI Terminal Agent")
        self.set_default_size(1200, 700)
        
        # Initialize managers
        self.config_manager = app.config_manager
        self.do_md_manager = DoMdManager()
        
        # Terminal panel (left side)
        self.terminal_panel = TerminalPanel()
        
        # Chat panel (right side)
        self.chat_panel = ChatPanel(self.on_user_command, self.on_stop_task, self.config_manager)
        
        # Create paned layout for resizable split
        paned = Gtk.Paned(orientation=Gtk.Orientation.HORIZONTAL)
        paned.set_start_child(self.terminal_panel)
        paned.set_end_child(self.chat_panel)
        paned.set_wide_handle(True)  # Make handle easier to see and grab
        paned.set_resize_start_child(True)
        paned.set_resize_end_child(True)
        paned.set_shrink_start_child(False)
        paned.set_shrink_end_child(False)
        paned.set_hexpand(True)
        paned.set_vexpand(True)
        
        # Use Adw.ToolbarView for proper Adwaita layout
        toolbar_view = Adw.ToolbarView()
        toolbar_view.set_content(paned)
        
        # Create header bar with settings button
        header_bar = Adw.HeaderBar()
        
        # Settings button
        settings_button = Gtk.Button()
        settings_button.set_icon_name("preferences-system-symbolic")
        settings_button.set_tooltip_text("Settings")
        settings_button.connect("clicked", self.on_settings_clicked)
        header_bar.pack_end(settings_button)
        
        toolbar_view.add_top_bar(header_bar)
        
        self.set_content(toolbar_view)
        
        # Set initial split position after window is realized
        GLib.idle_add(lambda: paned.set_position(700))
        
        # Initialize AI agent
        self.ai_agent = None
        self.tool_executor = ToolExecutor(
            self.terminal_panel.terminal,
            self.do_md_manager
        )
        
        # Task state
        self.current_task = None
        self.task_history = []
        self.is_processing = False
        self.stop_requested = False
        
    def _filter_reasoning(self, reasoning_text):
        import re
        
        tags = ['think', 'thinking', 'thought', 'reasoning', 'reason']
        pattern = re.compile(r'(<({tags})>)(.*?)(</\2>)'.format(tags='|'.join(tags)), re.DOTALL)
        
        def replacer(match):
            content = match.group(3)
            tool_pattern = re.compile(r'\[[^\]]+\]')
            cleaned_content = tool_pattern.sub('', content)
            return match.group(1) + cleaned_content + match.group(4)
            
        return pattern.sub(replacer, reasoning_text)

    def show_api_key_warning(self):
        """Shows a dialog warning the user to set an API key."""
        dialog = Adw.MessageDialog(
            transient_for=self,
            modal=True,
            heading="API Key Not Set",
            body="Please set your Hugging Face API key in the settings before starting a conversation."
        )
        dialog.add_response("ok", "OK")
        dialog.set_default_response("ok")
        dialog.connect("response", lambda d, r: d.close())
        dialog.present()

    def on_settings_clicked(self, button):
        """Open settings dialog"""
        settings_dialog = SettingsDialog(self, self.config_manager)
        settings_dialog.present()
    
    def on_stop_task(self):
        """Request to stop the current AI task"""
        if self.is_processing:
            self.chat_panel.add_message("System", "Stop request received. The task will stop after the current step.")
            self.stop_requested = True

    def on_user_command(self, command):
        """
        Handle user command from chat. Returns True if the command is accepted for processing.
        """
        if not self.chat_panel.get_api_key():
            self.show_api_key_warning()
            return False
            
        if self.is_processing:
            self.chat_panel.add_message("System", "Already processing a task. Please wait.")
            return False
        
        # Initialize AI agent with current settings
        model_name = self.chat_panel.get_model_name()
        
        if not model_name:
            self.chat_panel.add_message("System", "Please select a model.")
            return False
        
        api_key = self.chat_panel.get_api_key()
        self.ai_agent = AIAgent(api_key, model_name)
        
        # Reset conversation for new task
        self.ai_agent.reset_conversation()
        
        # Start task execution
        self.current_task = command
        self.task_history = []
        
        # Run in thread to avoid blocking UI
        thread = threading.Thread(target=self.execute_task_loop)
        thread.daemon = True
        thread.start()
        return True
    
    def execute_task_loop(self):
        """Main loop for executing AI task"""
        self.is_processing = True
        self.stop_requested = False
        GLib.idle_add(self.chat_panel.set_processing_state, True)
        
        try:
            max_iterations = 50  # Prevent infinite loops
            
            for iteration in range(max_iterations):
                if self.stop_requested:
                    self.chat_panel.add_message("System", "Task stopped by user.")
                    break

                # Get current context
                terminal_output = self.terminal_panel.get_visible_text()
                do_md_content = self.do_md_manager.read()
                previous_actions = "\n".join(self.task_history[-10:])  # Last 10 actions
                
                # Build prompt
                prompt = self.ai_agent.build_prompt(
                    self.current_task,
                    terminal_output,
                    do_md_content,
                    previous_actions
                )
                
                # Call AI
                GLib.idle_add(self.chat_panel.show_thinking_indicator)
                
                response, error = self.ai_agent.call_api(prompt)

                GLib.idle_add(self.chat_panel.hide_thinking_indicator)
                
                if error:
                    GLib.idle_add(self.chat_panel.add_message, "Error", error)
                    break
                
                # Parse tool from response
                tool_name, tool_content, reasoning = self.ai_agent.parse_tool_from_response(response)
                
                if reasoning:
                    filtered_reasoning = self._filter_reasoning(reasoning)
                    GLib.idle_add(self.chat_panel.add_message, "AI", filtered_reasoning)
                
                if not tool_name:
                    GLib.idle_add(self.chat_panel.add_message, "System", "No tool found in AI response. Stopping.")
                    break
                
                # Execute tool
                if tool_name in ["Input", "Text"] and not self.config_manager.get_auto_allow_tools():
                    dialog = ToolConfirmationDialog(self, tool_name, tool_content, self.config_manager)
                    user_response = dialog.run()
                    
                    if user_response == "allow":
                        success, result = self.tool_executor.execute_tool(tool_name, tool_content)
                    else: # deny
                        success, result = False, "Action denied by user."
                        GLib.idle_add(self.chat_panel.add_message, "System", result)
                        break # Stop the loop on denial
                
                elif tool_name == "Wait":
                    try:
                        seconds = float(tool_content.strip())
                        completion_event = threading.Event()
                        
                        GLib.idle_add(self.chat_panel.show_wait_countdown, seconds, completion_event)
                        
                        # Block AI thread until UI timer is done
                        completion_event.wait(timeout=seconds + 2)

                        success, result = True, f"Waited {seconds} seconds"
                    except ValueError:
                        success, result = False, "Invalid wait time"
                else:
                    success, result = self.tool_executor.execute_tool(tool_name, tool_content)
                    
                # Don't show generic tool execution for "Success"
                if tool_name != "Success":
                    GLib.idle_add(self.chat_panel.add_tool_execution, tool_name, tool_content, result)
                
                # Record action
                action_record = f"[{tool_name}] {tool_content[:100]} -> {result}"
                self.task_history.append(action_record)
                
                # Add tool execution result to conversation history so AI knows what happened
                tool_feedback = f"Tool [{tool_name}] executed. Result: {result}"
                if not success:
                    tool_feedback += " (Failed)"
                
                self.ai_agent.conversation_history.append({
                    "role": "user",
                    "content": tool_feedback
                })
                
                # Check if task is complete
                if tool_name == "Success":
                    GLib.idle_add(self.chat_panel.add_success_message, tool_content)
                    break
                
                if not success:
                    GLib.idle_add(self.chat_panel.add_message, "System", f"Tool execution failed: {result}")
                    # Continue anyway, let AI handle the error
                
                # Small delay between iterations
                time.sleep(1)
            else:
                GLib.idle_add(self.chat_panel.add_message, "System", "Max iterations reached. Task may be incomplete.")
        
        finally:
            self.is_processing = False
            self.stop_requested = False
            GLib.idle_add(self.chat_panel.set_processing_state, False)


class CliAApplication(Adw.Application):
    """Main application class"""
    
    def __init__(self):
        super().__init__(application_id="net.bloupla.clia")
        self.config_manager = ConfigManager()
        self.apply_theme()
        self.connect("startup", self.on_startup)

    def on_startup(self, app):
        """Called when application starts"""
        self.config_manager.config = self.config_manager.load_config()
        self.apply_theme()

    def apply_theme(self):
        """Apply the saved theme"""
        theme = self.config_manager.get_theme()
        style_manager = Adw.StyleManager.get_default()
        if theme == 'dark':
            style_manager.set_color_scheme(Adw.ColorScheme.FORCE_DARK)
        elif theme == 'light':
            style_manager.set_color_scheme(Adw.ColorScheme.FORCE_LIGHT)
        else: # system
            style_manager.set_color_scheme(Adw.ColorScheme.DEFAULT)
    
    def do_activate(self):
        """Activate the application"""
        win = self.props.active_window
        if not win:
            win = MainWindow(self)
        win.present()


def main():
    """Main entry point"""
    app = CliAApplication()
    return app.run(sys.argv)


if __name__ == "__main__":
    sys.exit(main())

