import http.server
import socketserver
import os
import json
import urllib.parse

PORT = 8085
# MMD_FILE is relative to the root, which is 2 levels up from this script
ROOT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), '../../'))
MMD_FILE = os.path.join(ROOT_DIR, "feature_graph.mmd")
FEATURES_DIR = os.path.join(ROOT_DIR, "features")

class GraphHandler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        # Serve the raw graph data
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
        
        # Serve specific feature file content
        elif self.path.startswith('/api/feature/'):
            # Extract filename from URL
            feature_name = self.path.split('/')[-1]
            # Sanitize filename to prevent directory traversal
            feature_name = os.path.basename(feature_name)
            
            # Try with .md extension if not provided, or as is
            possible_paths = [
                os.path.join(FEATURES_DIR, feature_name),
                os.path.join(FEATURES_DIR, f"{feature_name}.md")
            ]
            
            file_content = None
            found_path = None
            
            for p in possible_paths:
                if os.path.exists(p) and os.path.isfile(p):
                    found_path = p
                    break
            
            if found_path:
                try:
                    with open(found_path, 'r') as f:
                        file_content = f.read()
                    self.send_response(200)
                    self.send_header('Content-type', 'text/plain; charset=utf-8')
                    self.end_headers()
                    self.wfile.write(file_content.encode('utf-8'))
                except Exception as e:
                    self.send_error(500, f"Error reading file: {str(e)}")
            else:
                self.send_error(404, "Feature file not found")

        # Serve the main page
        elif self.path == '/' or self.path == '/index.html':
            self.send_response(200)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            # Serve the index.html from the same directory as this script
            index_path = os.path.join(os.path.dirname(__file__), 'index.html')
            with open(index_path, 'rb') as f:
                self.copyfile(f, self.wfile)
                
        else:
            # For other static files (like JS/CSS if we add them separately), 
            # serve from the script directory by default
            # But we might want to serve relative to project root? 
            # For now, let's keep it simple. If we need files from root, we can handle it.
            super().do_GET()

if __name__ == "__main__":
    print(f"Starting Software Map Server at http://localhost:{PORT}")
    print(f"Serving graph from: {MMD_FILE}")
    print(f"Serving features from: {FEATURES_DIR}")
    
    # Change working directory to script location to make serving relative static files easier
    os.chdir(os.path.dirname(__file__))
    
    socketserver.TCPServer.allow_reuse_address = True
    with socketserver.TCPServer(("", PORT), GraphHandler) as httpd:
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            pass
        httpd.server_close()
