import http.server
import socketserver
import subprocess
import os
import re
from datetime import datetime

PORT = 8086
PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '../../../'))

def run_command(command):
    """Runs a shell command and returns its stdout."""
    try:
        result = subprocess.run(
            command,
            shell=True,
            capture_output=True,
            text=True,
            check=True,
            cwd=PROJECT_ROOT
        )
        return result.stdout.strip()
    except subprocess.CalledProcessError as e:
        print(f"Error running command '{command}': {e}")
        print(f"Stderr: {e.stderr}")
        return ""

def get_feature_status():
    """Gathers the status of all features."""
    features_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), '../../../features'))
    if not os.path.isdir(features_dir):
        return [], [], []

    done_features, testing_features, todo_features = [], [], []
    
    feature_files = [f for f in os.listdir(features_dir) if f.endswith('.md')]

    for fname in feature_files:
        f_path = os.path.join('features', fname)
        
        complete_commit_ts_str = run_command(f"git log -1 --grep='\\[Complete {f_path}\\]' --format=%ct")
        test_commit_ts_str = run_command(f"git log -1 --grep='\\[Ready for HIL Test {f_path}\\]' --format=%ct")
        file_mod_ts_str = run_command(f"git log -1 --format=%ct -- '{f_path}'")

        complete_timestamp = int(complete_commit_ts_str) if complete_commit_ts_str else 0
        test_timestamp = int(test_commit_ts_str) if test_commit_ts_str else 0
        file_timestamp = int(file_mod_ts_str) if file_mod_ts_str else 0

        status = "TODO"
        if complete_timestamp > test_timestamp:
            if file_timestamp <= complete_timestamp:
                status = "DONE"
        elif test_timestamp > 0:
            if file_timestamp <= test_timestamp:
                status = "TESTING"
        
        if status == "DONE":
            done_features.append(fname)
        elif status == "TESTING":
            testing_features.append(fname)
        else:
            todo_features.append(fname)

    return sorted(done_features), sorted(testing_features), sorted(todo_features)

def get_git_status():
    """Gets the current git status."""
    return run_command("git status --porcelain | grep -v '.DS_Store' | grep -v '.cache/'")

def get_last_commit():
    """Gets the last commit message."""
    return run_command("git log -1 --format='%h %s (%cr)'")

def get_test_status():
    """Gets the PlatformIO test status."""
    summary_file = os.path.join(PROJECT_ROOT, '.pio/testing/last_summary.json')
    if not os.path.exists(summary_file):
        return "UNKNOWN", "No Test History"
    
    with open(summary_file, 'r') as f:
        content = f.read()
        # Check for errors or failures in the JSON
        if '"error_nums": 0' in content and '"failure_nums": 0' in content:
            return "PASS", "Systems Nominal"
        else:
            return "FAIL", "Logic Broken"

def generate_html():
    """Generates the HTML page content."""
    done, testing, todo = get_feature_status()
    git_status = get_git_status()
    last_commit = get_last_commit()
    test_status, test_message = get_test_status()

    # --- Status HTML ---
    git_status_html = ""
    if not git_status:
        git_status_html = '<p class="clean">Clean State <span class="dim">(Ready for next task)</span></p>'
    else:
        git_status_html = '<p class="wip">Work in Progress:</p><pre>' + git_status + '</pre>'

    # --- Feature Lists HTML ---
    def create_feature_list(features, status):
        if not features:
            return ""
        items = ''.join(f'<li><span class="square {status.lower()}"></span>{fname}</li>' for fname in features)
        return f'<ul class="feature-list">{items}</ul>'

    done_html = create_feature_list(done, "DONE")
    testing_html = create_feature_list(testing, "TESTING")
    todo_html = create_feature_list(todo, "TODO")

    # --- Test Status HTML ---
    test_status_html = f'<p><span class="status-{test_status.lower()}">{test_status}</span> - {test_message}</p>'

    html = f"""
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Project Monitor</title>
        <meta http-equiv="refresh" content="5">
        <style>
            body {{
                background-color: #14191F;
                color: #B0B0B0;
                font-family: 'Menlo', 'Monaco', 'Consolas', monospace;
                font-size: 13px;
                margin: 0;
                padding: 10px 20px;
            }}
            .container {{
                max-width: 1400px;
                margin: auto;
            }}
            h2 {{
                font-size: 1.2em;
                color: #FFFFFF;
                margin-top: 10px;
                margin-bottom: 8px;
            }}
            h3 {{
                font-size: 1em;
                color: #888;
                margin-top: 12px;
                margin-bottom: 4px;
                text-transform: uppercase;
                letter-spacing: 0.5px;
            }}
            .header {{
                display: flex;
                justify-content: flex-end;
                margin-bottom: 5px;
            }}
            .main-content {{
                display: flex;
                flex-wrap: wrap;
                gap: 30px;
            }}
            .left-column {{
                flex: 2;
                min-width: 300px;
            }}
            .right-column {{
                flex: 1;
                min-width: 250px;
            }}
            .dim {{ color: #666; font-size: 0.9em; }}
            .clean {{ color: #32CD32; }}
            .wip {{ color: #FFD700; margin-bottom: 4px; }}
            pre {{
                background-color: #1E242B;
                padding: 8px;
                border-radius: 5px;
                white-space: pre-wrap;
                word-wrap: break-word;
                max-height: 150px;
                overflow-y: auto;
                margin-top: 0;
            }}
            .feature-list {{
                list-style: none;
                padding: 0;
                margin-top: 2px;
            }}
            .feature-list li {{
                display: flex;
                align-items: center;
                margin-bottom: 2px;
            }}
            .square {{
                width: 8px;
                height: 8px;
                margin-right: 8px;
                flex-shrink: 0;
                border-radius: 1px;
            }}
            .square.done {{ background-color: #32CD32; }}
            .square.testing {{ background-color: #4A90E2; }}
            .square.todo {{ background-color: #FFD700; }}

            .status-pass {{ color: #32CD32; font-weight: bold;}}
            .status-fail {{ color: #FF4500; font-weight: bold;}}
            .status-unknown {{ color: #666; font-weight: bold;}}

            .section {{
                margin-bottom: 15px;
            }}
        </style>
    </head>
    <body>
        <div class="container">
            <div class="header">
                <span class="dim">{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}</span>
            </div>

            <div class="main-content">
                <div class="left-column">
                    <div class="section">
                        <h2>Workspace Context (Git Status)</h2>
                        {git_status_html}
                    </div>

                    <div class="section">
                        <h2>Feature Queue</h2>
                        <h3>TODO</h3>
                        {todo_html if todo_html else '<p class="dim">No features are pending implementation.</p>'}
                        <h3>TESTING</h3>
                        {testing_html if testing_html else '<p class="dim">No features are currently in testing.</p>'}
                        <h3>DONE</h3>
                        {done_html if done_html else '<p class="dim">No features are complete.</p>'}
                    </div>
                </div>

                <div class="right-column">
                    <div class="section">
                        <h2>Latest Save (Last Commit)</h2>
                        <pre>{last_commit}</pre>
                    </div>

                    <div class="section">
                        <h2>Test Status</h2>
                        {test_status_html}
                    </div>
                </div>
            </div>
        </div>
    </body>
    </html>
    """
    return html

class Handler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.send_header("Content-type", "text/html")
        self.end_headers()
        content = generate_html()
        self.wfile.write(content.encode('utf-8'))

if __name__ == "__main__":
    with socketserver.TCPServer(("", PORT), Handler) as httpd:
        print(f"Serving at port {PORT}")
        httpd.serve_forever()