import http.server
import socketserver
import subprocess
import os
from datetime import datetime

PORT = 8086
PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '../../../'))

DOMAINS = [
    {
        "label": "LPad Application",
        "features_rel": "features",
        "features_abs": os.path.join(PROJECT_ROOT, "features"),
        "test_summary": os.path.join(PROJECT_ROOT, ".pio/testing/last_summary.json"),
        "test_label": "Firmware Tests",
    },
    {
        "label": "Agentic DevOps",
        "features_rel": "agentic_devops/features",
        "features_abs": os.path.join(PROJECT_ROOT, "agentic_devops/features"),
        "test_summary": os.path.join(PROJECT_ROOT, "agentic_devops/tools/test_summary.json"),
        "test_label": "DevOps Tests",
    },
]

DONE_CAP = 10


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
    except subprocess.CalledProcessError:
        return ""


def get_feature_status(features_rel, features_abs):
    """Gathers the status of all features for a given domain directory."""
    if not os.path.isdir(features_abs):
        return [], [], []

    done, testing, todo = [], [], []
    feature_files = [f for f in os.listdir(features_abs) if f.endswith('.md')]

    for fname in feature_files:
        f_path = os.path.join(features_rel, fname)

        complete_ts_str = run_command(
            f"git log -1 --grep='\\[Complete {f_path}\\]' --format=%ct"
        )
        test_ts_str = run_command(
            f"git log -1 --grep='\\[Ready for HIL Test {f_path}\\]' --format=%ct"
        )
        file_mod_ts_str = run_command(
            f"git log -1 --format=%ct -- '{f_path}'"
        )

        complete_ts = int(complete_ts_str) if complete_ts_str else 0
        test_ts = int(test_ts_str) if test_ts_str else 0
        file_mod_ts = int(file_mod_ts_str) if file_mod_ts_str else 0

        status = "TODO"
        if complete_ts > test_ts:
            if file_mod_ts <= complete_ts:
                status = "DONE"
        elif test_ts > 0:
            if file_mod_ts <= test_ts:
                status = "TESTING"

        if status == "DONE":
            done.append((fname, complete_ts))
        elif status == "TESTING":
            testing.append(fname)
        else:
            todo.append(fname)

    done.sort(key=lambda x: x[1], reverse=True)
    return done, sorted(testing), sorted(todo)


def get_test_status(summary_path):
    """Gets the test status from a summary JSON file."""
    if not os.path.exists(summary_path):
        return "UNKNOWN", "No Test History"

    with open(summary_path, 'r') as f:
        content = f.read()
        if '"error_nums": 0' in content and '"failure_nums": 0' in content:
            return "PASS", "Systems Nominal"
        return "FAIL", "Logic Broken"


def get_git_status():
    """Gets the current git status."""
    return run_command("git status --porcelain | grep -v '.DS_Store' | grep -v '.cache/'")


def get_last_commit():
    """Gets the last commit message."""
    return run_command("git log -1 --format='%h %s (%cr)'")


def _feature_list_html(features, css_class):
    """Renders a <ul> of feature names with status squares."""
    if not features:
        return ""
    items = ''.join(
        f'<li><span class="sq {css_class}"></span>{name}</li>'
        for name in features
    )
    return f'<ul class="fl">{items}</ul>'


def _domain_column_html(domain):
    """Builds the HTML for one domain column."""
    done_tuples, testing, todo = get_feature_status(
        domain["features_rel"], domain["features_abs"]
    )

    # DONE capping
    total_done = len(done_tuples)
    visible_done = [name for name, _ in done_tuples[:DONE_CAP]]
    overflow = total_done - DONE_CAP

    todo_html = _feature_list_html(todo, "todo")
    testing_html = _feature_list_html(testing, "testing")
    done_html = _feature_list_html(visible_done, "done")
    overflow_html = (
        f'<p class="dim">and {overflow} more&hellip;</p>' if overflow > 0 else ""
    )

    test_status, test_msg = get_test_status(domain["test_summary"])

    return f"""
    <div class="domain-col">
        <h2>{domain["label"]}</h2>
        <h3>TODO</h3>
        {todo_html or '<p class="dim">None pending.</p>'}
        <h3>TESTING</h3>
        {testing_html or '<p class="dim">None in testing.</p>'}
        <h3>DONE</h3>
        {done_html or '<p class="dim">None complete.</p>'}
        {overflow_html}
        <div class="test-bar">
            <span class="st-{test_status.lower()}">{test_status}</span>
            <span class="dim">{domain["test_label"]}: {test_msg}</span>
        </div>
    </div>"""


def generate_html():
    """Generates the full dashboard HTML."""
    git_status = get_git_status()
    last_commit = get_last_commit()

    if not git_status:
        git_html = '<p class="clean">Clean State <span class="dim">(Ready for next task)</span></p>'
    else:
        git_html = '<p class="wip">Work in Progress:</p><pre>' + git_status + '</pre>'

    domain_columns = ''.join(_domain_column_html(d) for d in DOMAINS)

    return f"""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>CDD Monitor</title>
<meta http-equiv="refresh" content="5">
<style>
*{{box-sizing:border-box;margin:0;padding:0}}
body{{
  background:#14191F;color:#B0B0B0;
  font-family:'Menlo','Monaco','Consolas',monospace;
  font-size:12px;padding:8px 12px;
}}
.hdr{{display:flex;justify-content:space-between;align-items:baseline;margin-bottom:6px}}
.hdr h1{{font-size:14px;color:#FFF;font-weight:600}}
.dim{{color:#666;font-size:0.9em}}
h2{{font-size:13px;color:#FFF;margin-bottom:6px;border-bottom:1px solid #2A2F36;padding-bottom:4px}}
h3{{font-size:11px;color:#888;margin:8px 0 2px;text-transform:uppercase;letter-spacing:.5px}}
.domains{{display:flex;gap:16px;margin-bottom:10px}}
.domain-col{{flex:1;min-width:0;background:#1A2028;border-radius:4px;padding:8px 10px}}
.fl{{list-style:none}}
.fl li{{display:flex;align-items:center;margin-bottom:1px;line-height:1.5}}
.sq{{width:7px;height:7px;margin-right:6px;flex-shrink:0;border-radius:1px}}
.sq.done{{background:#32CD32}}
.sq.testing{{background:#4A90E2}}
.sq.todo{{background:#FFD700}}
.test-bar{{margin-top:8px;padding-top:6px;border-top:1px solid #2A2F36}}
.st-pass{{color:#32CD32;font-weight:bold}}
.st-fail{{color:#FF4500;font-weight:bold}}
.st-unknown{{color:#666;font-weight:bold}}
.ctx{{background:#1A2028;border-radius:4px;padding:8px 10px}}
.clean{{color:#32CD32}}
.wip{{color:#FFD700;margin-bottom:2px}}
pre{{background:#14191F;padding:6px;border-radius:3px;white-space:pre-wrap;word-wrap:break-word;max-height:100px;overflow-y:auto;margin-top:2px}}
</style>
</head>
<body>
<div class="hdr">
  <h1>CDD Monitor</h1>
  <span class="dim">{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}</span>
</div>
<div class="domains">
  {domain_columns}
</div>
<div class="ctx">
  <h2>Workspace</h2>
  {git_html}
  <p class="dim" style="margin-top:4px">{last_commit}</p>
</div>
</body>
</html>"""


class Handler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.send_header("Content-type", "text/html")
        self.end_headers()
        self.wfile.write(generate_html().encode('utf-8'))

    def log_message(self, format, *args):
        pass  # Suppress request logging noise


if __name__ == "__main__":
    with socketserver.TCPServer(("", PORT), Handler) as httpd:
        print(f"CDD Monitor serving at http://localhost:{PORT}")
        httpd.serve_forever()
