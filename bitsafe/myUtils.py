# -*- coding: utf-8 -*-

import json
import os
import platform
import re

class SharedPreferences:
    def __init__(self):
        self.state_file = "cli_bitsafe_state.json"

    def get_shared_state(self, name, default):
        if os.path.exists(self.state_file):
            with open(self.state_file, 'r') as file:
                try:
                    state_data = json.load(file)
                    return state_data.get(name, default)
                except Exception:
                    return default
        return default

    def set_shared_state(self, name, value):
        if os.path.exists(self.state_file):
            with open(self.state_file, 'r') as read_file:
                try:
                    read_data = json.load(read_file)
                except Exception:
                    read_data = json.loads("{}")
                with open(self.state_file, 'w') as file:
                    read_data[name] = value
                    json.dump(read_data, file, indent=2)
class MyIp:
    def is_valdi_first(self, address):
        first_int = address.split('.')[0]
        return int(first_int) >= 192
    def is_ipv4_address(self, address):
        pattern = re.compile(r'^(\d{1,3}\.){3}\d{1,3}$')
        return bool(pattern.match(address)) and not address.startswith("127.")

    def __get_list_ips(self):
        addresses = []
        system_platform = platform.system()

        if system_platform == "Windows":
            # Windows
            import subprocess
            result = subprocess.run(["ipconfig"], capture_output=True, text=True)
            output = result.stdout
            addresses = [line.split(":")[1].strip() for line in output.splitlines() if "IPv4 Address" in line]

        elif system_platform in ["Linux", "Darwin"]:
            # Linux ou macOS
            import os
            result = os.popen("/sbin/ifconfig").read()
            addresses = [line.split()[1] for line in result.splitlines() if "inet" in line]
        ipv4_addresses = [address for address in addresses if self.is_ipv4_address(address) and self.is_valdi_first(address)]
        return ipv4_addresses

    def get_ip(self):
        preferences = SharedPreferences()
        port = preferences.get_shared_state("port", "8000")
        ip_list = self.__get_list_ips()
        ip_valid_list = [f"http://{ip}:{port}" for ip in ip_list]
        if len(ip_valid_list) > 0:
            return ip_valid_list[0]
        else:
            return None