import http.server
import socketserver
import os
import json
import subprocess
import urllib.parse

PORT = 8085
# Root is 3 levels up from this script (agentic_devops/tools/software_map -> project root)
ROOT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), '../../../'))
MMD_FILE_LPAD = os.path.join(ROOT_DIR, "agentic_devops/tools/feature_graph_lpad.mmd")
MMD_FILE_AGENTIC = os.path.join(ROOT_DIR, "agentic_devops/tools/feature_graph_agentic.mmd")
FEATURES_DIR_LPAD = os.path.join(ROOT_DIR, "features")
FEATURES_DIR_AGENTIC = os.path.join(ROOT_DIR, "agentic_devops/features")
GENERATE_SCRIPT = os.path.join(os.path.dirname(__file__), "generate_tree.py")

def trigger_regeneration():
    """Run the generate_tree.py script to sync files."""
    try:
        subprocess.run(["python3", GENERATE_SCRIPT], check=True, capture_output=True)
    except Exception as e:
        print(f"Error triggering regeneration: {e}")

class GraphHandler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header('Cache-Control', 'no-store, no-cache, must-revalidate')
        self.send_header('Pragma', 'no-cache')
        self.send_header('Expires', '0')
        super().end_headers()

    def do_GET(self):
        parsed_path = urllib.parse.urlparse(self.path)
        query = urllib.parse.parse_qs(parsed_path.query)
        
        # Trigger regeneration on every API request
        if parsed_path.path == '/api/graph':
            trigger_regeneration()
            
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.end_headers()
            
            graph_type = query.get('type', ['lpad'])[0]
            mmd_file = MMD_FILE_AGENTIC if graph_type == 'agentic' else MMD_FILE_LPAD
            
            try:
                with open(mmd_file, 'r', encoding='utf-8') as f:
                    content = f.read()
                mtime = os.path.getmtime(mmd_file)
                response = {
                    "content": content,
                    "mtime": mtime
                }
                self.wfile.write(json.dumps(response).encode())
            except Exception as e:
                self.wfile.write(json.dumps({"error": str(e)}).encode())
        
        elif parsed_path.path.startswith('/api/feature/'):
            feature_name = parsed_path.path.split('/')[-1]
            feature_name = os.path.basename(feature_name)
            possible_paths = [
                os.path.join(FEATURES_DIR_LPAD, feature_name),
                os.path.join(FEATURES_DIR_LPAD, f"{feature_name}.md"),
                os.path.join(FEATURES_DIR_AGENTIC, feature_name),
                os.path.join(FEATURES_DIR_AGENTIC, f"{feature_name}.md")
            ]
            
            found_path = next((p for p in possible_paths if os.path.exists(p)), None)
            
            if found_path:
                try:
                    with open(found_path, 'r', encoding='utf-8') as f:
                        file_content = f.read()
                    self.send_response(200)
                    self.send_header('Content-type', 'text/plain; charset=utf-8')
                    self.end_headers()
                    self.wfile.write(file_content.encode('utf-8'))
                except Exception as e:
                    self.send_error(500, f"Error reading file: {str(e)}")
            else:
                self.send_error(404, "Feature file not found")

        elif parsed_path.path == '/' or parsed_path.path == '/index.html':
            self.send_response(200)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            index_path = os.path.join(os.path.dirname(__file__), 'index.html')
            with open(index_path, 'rb') as f:
                self.copyfile(f, self.wfile)
        else:
            super().do_GET()

if __name__ == "__main__":
    print(f"Starting Software Map Server at http://localhost:{PORT}")
    os.chdir(os.path.dirname(__file__))
    socketserver.TCPServer.allow_reuse_address = True
    with socketserver.TCPServer(("", PORT), GraphHandler) as httpd:
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            pass
        httpd.server_close()
