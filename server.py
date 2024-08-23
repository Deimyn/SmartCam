import socket
from http.server import SimpleHTTPRequestHandler, HTTPServer

def get_local_ip():
    """Get the local IP address of the machine."""
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        # doesn't have to be reachable
        s.connect(("10.254.254.254", 1))
        ip = s.getsockname()[0]
    except Exception:
        ip = "127.0.0.1"
    finally:
        s.close()
    return ip

if __name__ == "__main__":
    ip = get_local_ip()
    port = 8080
    server_address = (ip, port)

    httpd = HTTPServer(server_address, SimpleHTTPRequestHandler)
    print(f"Serving HTTP on {ip} port {port} (http://{ip}:{port}/) ...")
    httpd.serve_forever()
