import http.server
import socketserver
import os
import json
import time

PORT = 8000
MMD_FILE = "feature_graph.mmd"

class GraphHandler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/api/graph':
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.end_headers()
            
            try:
                with open(MMD_FILE, 'r') as f:
                    content = f.read()
                mtime = os.path.getmtime(MMD_FILE)
                response = {
                    "content": content,
                    "mtime": mtime
                }
                self.wfile.write(json.dumps(response).encode())
            except Exception as e:
                self.wfile.write(json.dumps({"error": str(e)}).encode())
        elif self.path == '/' or self.path == '/index.html':
            self.send_response(200)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            with open('scripts/graph_viewer.html', 'rb') as f:
                self.copyfile(f, self.wfile)
        else:
            super().do_GET()

if __name__ == "__main__":
    print(f"Starting Graph Server at http://localhost:{PORT}")
    print(f"Watching {MMD_FILE} for changes...")
    with socketserver.TCPServer(("", PORT), GraphHandler) as httpd:
        httpd.serve_forever()
