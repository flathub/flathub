# -*- coding: utf-8 -*-


import json
import os
import signal
from http.server import BaseHTTPRequestHandler
from subprocess import Popen, PIPE

from bitsafe.constants import PUBLIC_KEY_PATH
from bitsafe.myUtils import MyIp
from bitsafe.myUtils import SharedPreferences


class CustomHandler(BaseHTTPRequestHandler):
    def stop_process_by_port(self,port):
        process = Popen(["lsof", "-i", ":{0}".format(port)], stdout=PIPE, stderr=PIPE)
        stdout, stderr = process.communicate()
        for process in str(stdout.decode("utf-8")).split("\n")[1:]:
            data = [x for x in process.split(" ") if x != '']
            if len(data) <= 1:
                continue
            os.kill(int(data[1]), signal.SIGKILL)
    def do_GET(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/plain')
        self.end_headers()
        my_ip = MyIp().get_ip()
        public_key = open(PUBLIC_KEY_PATH, 'rb').read()
        response = {
            'public_key': str(public_key),
            'route': my_ip
        }
        self.wfile.write(bytes(json.dumps(response, indent=2), 'utf-8'))

    def do_POST(self):
        content_length = int(self.headers['Content-Length'])
        post_data = self.rfile.read(content_length).decode('utf-8')
        print(f"Received POST data: {post_data}")
        req = json.loads(post_data)
        print(type(req))
        user_id = req.get('user_id', '')
        public_key_cell = req.get('public_key', '')
        host = req.get('host', '')
        preferences = SharedPreferences()
        try:
            if user_id and public_key_cell and host:
                preferences.set_shared_state('user_id', user_id)
                preferences.set_shared_state('host_public_key', public_key_cell)
                preferences.set_shared_state('host_cell', host)
                self.send_response(200)
                self.send_header('Content-type', 'application/json')
                self.end_headers()
                my_ip = MyIp().get_ip()
                public_key = open(PUBLIC_KEY_PATH, 'rb').read()
                port = preferences.get_shared_state('port','8000')
                response = {
                    'public_key': str(public_key),
                    'route': my_ip
                }
                self.wfile.write(bytes(json.dumps(response, indent=2), 'utf-8'))
                port = preferences.get_shared_state("port", "8000")
                self.stop_process_by_port(port)
            else:
                self.send_response(400)
                self.send_header('Content-type','application/json')
                self.end_headers()
                self.wfile.write(bytes(json.dumps('{}'),'utf-8'))
        except Exception:
            self.send_response(400)
            self.send_header('Content-type','application/json')
            self.end_headers()
            self.wfile.write(bytes(json.dumps('{}'),'utf-8'))

