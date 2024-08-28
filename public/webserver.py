import socket
from http.server import SimpleHTTPRequestHandler, HTTPServer

def get_local_ip():
    """Get the local IP address of the machine (Machine 2)."""
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        # Doesn't have to be reachable
        s.connect(("10.254.254.254", 1))
        ip = s.getsockname()[0]
    except Exception:
        ip = "127.0.0.1"
    finally:
        s.close()
    return ip

def get_machine1_ip():
    '''
    """Get the IP address of Machine 1 dynamically but if it's the defaulf gateway"""
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        # We assume Machine 1 is the default gateway in the same network
        s.connect(("8.8.8.8", 80))
        machine1_ip = s.getsockname()[0]
    except Exception:
        machine1_ip = "127.0.0.1"
    finally:
        s.close()
    return machine1_ip
    '''

    """Manually provide the IP of Machine 1"""
    machine1_ip = "172.16.80.22" # replace with webrtc machine IP
    return machine1_ip


class CustomHandler(SimpleHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/script.js':
            # Serve script.js with dynamic IP addresses
            machine1_ip = get_machine1_ip()
            with open('script.js', 'r') as file:
                script = file.read()
            script = script.replace('MACHINE_1_IP_PLACEHOLDER', machine1_ip)
            
            self.send_response(200)
            self.send_header('Content-type', 'application/javascript')
            self.end_headers()
            self.wfile.write(script.encode())
        else:
            # For all other requests, serve as usual
            super().do_GET()

if __name__ == "__main__":
    ip = get_local_ip()
    port = 8080
    server_address = (ip, port)
    machine1_ip = get_machine1_ip()

    httpd = HTTPServer(server_address, CustomHandler)
    print(f"Serving HTTP on {ip} port {port} (http://{ip}:{port}/) ...")
    print(f"Local Ip :{ip} and machine1 Ip :{machine1_ip}")
    httpd.serve_forever()
