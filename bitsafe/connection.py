# -*- coding: utf-8 -*-

import base64
import json
import os
import signal
import socket
import sys
import threading
from http.server import HTTPServer
from io import BytesIO
from subprocess import Popen, PIPE
import requests
import qrcode

from bitsafe.CustomHandler import CustomHandler
from bitsafe.myUtils import SharedPreferences, MyIp


class Connection:
    @staticmethod
    def stop_process_by_port(port):
        process = Popen(["lsof", "-i", ":{0}".format(port)], stdout=PIPE, stderr=PIPE)
        stdout, stderr = process.communicate()
        for process in str(stdout.decode("utf-8")).split("\n")[1:]:
            data = [x for x in process.split(" ") if x != '']
            if len(data) <= 1:
                continue
            os.kill(int(data[1]), signal.SIGKILL)

    def check_status(self):
        port = self.preferences.get_shared_state('port', 8000)
        if self.is_available_port(port):
            print("Closed")
            return False
        else:
            print("Running")
            return True

    def __local(self, local):
        if 'start' == local:
            is_runnig = self.check_status()
            if not is_runnig:
                self.port = self.find_available_port()
                self.preferences.set_shared_state('port', self.port)
                server_address = ('0.0.0.0', self.port)
                self.httpd = HTTPServer(server_address, CustomHandler)
                server_thread = threading.Thread(target=self.httpd.serve_forever)
                server_thread.start()
                self.is_running = True
                print(self.__get_local_ip())
        elif 'status' == local:
            self.check_status()
        elif 'stop' == local:
            if self.check_status():
                port = self.preferences.get_shared_state('port', '8000')
                self.stop_process_by_port(port)
                print("server stopped.")

    @staticmethod
    def is_available_port(port):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            result = s.connect_ex(('localhost', int(port)))
            return result != 0

    def find_available_port(self, start_port=8000, end_port=9000):
        for port in range(start_port, end_port + 1):
            result = self.is_available_port(port)
            if result:
                return port

    def __generate_qrcode_bytes(self,data):
        qr = qrcode.QRCode(
            version=1,
            error_correction=qrcode.constants.ERROR_CORRECT_L,
            box_size=1,
            border=1,
        )
        qr.add_data(data)
        qr.make(fit=True)

        img = qr.make_image(fill_color="black", back_color="white")

        # Crie um buffer de bytes para armazenar a imagem
        image_buffer = BytesIO()

        # Salve a imagem no buffer em formato PNG
        img.save(image_buffer, format="PNG")

        # Obtenha os bytes do buffer e codifique em base64
        base64_data = base64.b64encode(image_buffer.getvalue()).decode('utf-8')

        return base64_data
    @staticmethod
    def __generate_ascii_qrcode(data):
        qr = qrcode.QRCode(
            version=1,
            error_correction=qrcode.constants.ERROR_CORRECT_L,
            box_size=1,
            border=1,
        )
        qr.add_data(data)
        qr.make(fit=True)

        img = qr.make_image(fill_color="black", back_color="white")
        pixel_data = img.getdata()
        ascii_qrcode = ""
        ascii_chars = ["  ", "██"]
        index = 0
        for pixel_value in pixel_data:
            if index >= img.height:
                ascii_qrcode += "\n"
                index = 0
            if int(pixel_value) == 0:
                ascii_qrcode += ascii_chars[0]
            else:
                ascii_qrcode += ascii_chars[1]
            index += 1

        return ascii_qrcode

    def __get_local_ip(self):
        return MyIp().get_ip()

    def __command(self, base_url, args):
        if base_url == '':
            base_url = self.preferences.get_shared_state("host_cell", "")
            if base_url == '':
                return False
        if args.insert:
            items = args.insert
            item = {
                'site': items[0],
                'username': items[1],
                'password': items[2]
            }
            response = requests.post(url=f"{base_url}/item", data=json.dumps(item)).json()
            print(json.dumps(response,indent=2))
            return True
        elif args.find:
            find = {
                "term": args.find
            }
            response = requests.get(url=f"{base_url}/search", data=json.dumps(find)).json()
            print(json.dumps(response,indent=2))
            return True
        elif args.select:
            items = args.select
            item = {
                'id': items[0],
                'type': items[1]
            }
            response = requests.get(url=f"{base_url}/item", data=json.dumps(item)).json()
            print(json.dumps(response,indent=2))
            return True
        else:
            return False

    def __action(self, args):
        try:
            if args.ip_address:
                ip_type = args.ip_address
                if  ip_type == 'qrcode_base64':
                    ip = self.__get_local_ip()
                    base = self.__generate_qrcode_bytes(ip)
                    print(base)
                elif ip_type == 'qrcode_module':
                    ip = self.__get_local_ip()
                    qr_ascii = self.__generate_ascii_qrcode(ip)
                    print(qr_ascii)
                else :
                    ip = self.__get_local_ip()
                    print(ip)
            elif args.connection:
                self.__local(args.connection)
            elif self.__command('', args):
                ...
            else:
                self.parser.print_help()
                sys.exit(1)
        except Exception as e:
            self.parser.print_help()
            print(f"Error: {e}")
            sys.exit(1)

    def __init__(self, parser):
        self.preferences = SharedPreferences()
        parser.add_argument(
            "-f", "--find",
            help="search items by term.",
            required=False,
            type=str,
        )
        parser.add_argument(
            "-i", "--insert",
            help="Insert password in the database.",
            required=False,
            nargs=3,
            metavar=('site','username','password')
        )
        parser.add_argument(
            "-s", "--select",
            help="slect item by type and id.",
            required=False,
            nargs=2,
            metavar=('id','type')
        )
        parser.add_argument(
            "-ip", "--ip_address",
            help="show ip into computer.",
            required=False,
            choices=['string', 'qrcode_module', 'qrcode_base64']
        )
        parser.add_argument(
            "-c", "--connection",
            help="local connection.",
            required=False,
            choices=['status', 'start', 'stop'],
            type=str
        )
        parser.set_defaults(func=self.__action)
