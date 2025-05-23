# coding=utf8
"""
Run webassembly unit tests in a webassembly execution engine.
Usage:
    python runner.py [-h] --engine ENGINE --engine-args ENGINE_ARGS [--port PORT] [--exit] test_executable ...
"""

import argparse
import jinja2
import logging
import shlex
import shutil
import subprocess
import sys
import threading

from functools import partial
from http.server import SimpleHTTPRequestHandler, HTTPServer, HTTPStatus
from pathlib import Path
from urllib.parse import parse_qs, unquote, urlsplit

logger = logging.getLogger("vtkWebAssemblyTestRunner")

# /VTK/Testing/WebAssembly
BASE_DIR = Path(__file__).parent.absolute().resolve()
# /VTK/Testing/WebAssembly/templates
TEMPLATES_DIR = BASE_DIR.joinpath("templates")
# /VTK
VTK_SRC_DIR = BASE_DIR.parent.parent.absolute().resolve()


class vtkTestHTTPHandler(SimpleHTTPRequestHandler):

    # From Python 3.9 onwards SimpleHTTPRequestHandler does not populate
    # extensions_map with web defaults.
    extensions_map = {
        "": "application/octet-stream",
        ".html": "text/html",
        ".js": "text/javascript",
        ".mjs": "text/javascript",
        ".wasm": "application/wasm",
    }

    def __init__(self, *args, runner=None, **kwargs):
        self.runner = runner  # type: vtkWebAssemblyTestRunner
        super().__init__(*args, **kwargs)

    def log_message(self, format, *args):
        logger.debug(format, *args)

    def end_headers(self):
        # Do not allow client to use cache. Ideal for development where .wasm/.js may change.
        self.send_header('Cache-Control', 'no-store')
        super().end_headers()

    def do_GET(self):
        parts = urlsplit(self.path)
        if parts.path == '/':
            # For the root, generate html in memory and send it to the client
            payload = self.runner.generate_index_html().encode(encoding='utf-8')
            self.send_response(HTTPStatus.OK)
            self.send_header("Content-type", "text/html")
            self.send_header("Content-Length", str(len(payload)))
            self.send_header("Last-Modified", self.date_time_string())
            self.end_headers()
            self.wfile.write(payload)
        else:
            super().do_GET()

    def translate_path(self, path: str):
        parts = urlsplit(path)

        # Serve js/wasm from file system
        for filetype in [self.runner.js, self.runner.wasm]:
            if parts.path == '/' + filetype.name:
                filename = str(filetype)
                logger.debug(f"Translated {filetype} to {filename}")
                if not Path(filename).exists():
                    logger.error(f"{filename} does not exist!")
                    exit(1)
                else:
                    return filename
        if parts.path == "/favicon.ico":
            # Serve vtk logo for favicon
            return str(VTK_SRC_DIR.joinpath("vtkLogo.ico"))
        elif parts.path.startswith("/preload"):
            # Respond to preload requests with the file on host file system.
            query = parse_qs(parts.query)
            args = query.get('file')
            if args and len(args):
                return unquote(args[0])
            elif args is None:
                logger.error("Invalid query for preload")
        else:
            return super().translate_path(path)

    def do_POST(self):

        content_length = int(self.headers['Content-Length'])
        if not content_length:
            # useful for testing server with curl, etc.
            return self.send_post_status_response(b'OK')

        data = self.rfile.read(content_length)
        parts = urlsplit(self.path)
        status = b'OK'

        if parts.path.startswith("/dump"):
            # "dump?file=filename" will write binary data into filename
            query = parse_qs(parts.query)
            args = query.get('file')
            if args and len(args):
                outfile = Path(unquote(args[0]))
                outfile.parent.mkdir(parents=True, exist_ok=True)
                with outfile.open(mode='wb') as fh:
                    fh.write(data)
                    logger.info(
                        f"Wrote {len(data)} bytes to {str(outfile)}")
            elif args is None:
                status = b"Invalid query for /dump"
                logger.error(status)
        elif parts.path.startswith("/console_output"):
            logger.info(data.decode(encoding='utf-8'))
        elif parts.path.startswith("/exit"):
            try:
                self.runner.exit_code = int(data)
                logger.info(
                    f"Received exit code {self.runner.exit_code}")
                if self.runner.exit:
                    status = b"close-window"
            except ValueError:
                logger.error(
                    f"Invalid exit code! Expected an integer, got {data.decode()}")
                raise

        self.send_post_status_response(status)
        logger.debug(f"posted response {status.decode()}")

    def send_post_status_response(self, status):
        self.send_response(200)
        self.send_header('Content-type', 'text/plain')
        self.end_headers()
        try:
            self.wfile.write(status)
        except (ConnectionAbortedError, BrokenPipeError) as _:
            # Engine was forcibly shutdown? Ignore it.
            pass


class vtkWebAssemblyTestRunner:

    def __init__(self, args):

        # Keep the program arguments here, so that they're conveniently available through intellisense on `self`
        self.engine = args.engine
        self.engine_args = args.engine_args
        self.exit = args.exit
        self.port = args.port
        self.test_executable = args.test_executable
        self.test_args = args.test_args
        if args.verbose:
            logging.basicConfig(level=logging.DEBUG, format="%(asctime)s %(filename)s:%(lineno)d %(message)s")
        else:
            logging.basicConfig(level=logging.INFO, format="%(asctime)s %(filename)s:%(lineno)d %(message)s")
        # The exit code is set when a unit test POSTs an /exit message
        self.exit_code = None

        self.js = Path(args.test_executable)
        self.wasm = Path(args.test_executable.replace(".js", ".wasm"))

        self.templates = jinja2.Environment(
            loader=jinja2.FileSystemLoader(str(TEMPLATES_DIR)))

        self._httpd = None
        self._httpd_thread = threading.Thread(
            name="httpd", target=self._start_http_server)
        self._httpd_started = threading.Event()
        self._url = None
        self._server_is_shutdown = False

        # Add arguments that are recommended when automating VTK wasm tests with a specific engine
        self.implicit_engine_args = ""
        if self.engine:
            if "chrome" in self.engine or "chromium" in self.engine:
                flags = [
                    "--disable-application-cache",
                    "--disable-restore-session-state",
                    "--new-window",
                    "--incognito",
                    "--no-default-browser-check",
                    "--no-first-run",
                    "--enable-features=WebAssemblyExperimentalJSPI"
                ]
                # on linux, chrome needs more flags to unblock webgpu
                if sys.platform == "linux":
                    flags.extend([
                        "--enable-features=Vulkan",
                        "--enable-unsafe-webgpu",
                        "--use-angle=Vulkan",
                    ])
                self.implicit_engine_args = " ".join(flags)

    def run(self):

        # Run the browser after http server is ready to accept connections.
        self._httpd_thread.start()
        self._wait_for_server_start()
        if self.engine:
            subprocess_args = [self.engine] + shlex.split(self.implicit_engine_args) + shlex.split(self.engine_args) + [self._url]
            logger.info(f"Running subprocess '{' '.join(subprocess_args)}'")
            try:
                subprocess.run(subprocess_args, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            except Exception as e:
                logger.error("An error occurred while launching engine subprocess!")
                logger.error(e)
                self._server_is_shutdown = True
                self.exit_code = 1
            except KeyboardInterrupt:
                self.exit_code = 1
                self._server_is_shutdown = True
        else:
            logger.info(f"An engine was not specified. Script may appear to hang but it is actually running a http server.")
        # If a test did not send an exit code, this `join` will block. It can happen when a test is stuck.
        # In such scenario, the httpd loop will not break and `ctest`` will terminate us after the timeout interval
        # has elapsed.
        try:
            while self._httpd_thread.is_alive():
                self._httpd_thread.join(timeout=1)
        except KeyboardInterrupt:
            # On windows, socket recv and select methods are uninterruptible, making CTRL+C unreliable. For that reason,
            # our `httpd` thread cannot see sigint.
            # See https://github.com/python/cpython/issues/79115 and https://github.com/python/cpython/issues/85609
            # For this reason, handle SIGINT on main thread.
            self._server_is_shutdown = True
            self.exit_code = 1
            # Attempt to safely join the thread and release sockets
            self._httpd_thread.join(timeout=1000)
            if self._httpd_thread.is_alive():
                logger.warning(f"Port {self.port} may not be reusable. Failed to stop HTTP server at {self._url}")
            logger.debug("Caught KeyboardInterrupt, exiting.")

        return self.exit_code if self.exit_code is not None else 1

    def _wait_for_server_start(self):
        while not self._httpd_started.wait(0.001):
            continue

    def generate_index_html(self):
        js_type = ""
        with self.js.open(mode="r", encoding="utf-8") as js_fh:
            if js_fh.read().count('import.meta.url'):
                js_type = "module"
        test_args = "'" + "','".join(self.test_args) + "'"
        template = self.templates.get_template("index.html")
        use_webgpu = "WebGPU" in self.js.name
        content = template.render(
            js_filename=self.js.name, js_type=js_type, test_args=test_args, use_webgpu=use_webgpu)
        return content

    def _start_http_server(self):
        handler = partial(vtkTestHTTPHandler, runner=self)
        self._httpd = HTTPServer(("localhost", self.port), handler)
        self._httpd.timeout = 0.001

        host, port = self._httpd.socket.getsockname()[:2]
        url_host = f'[{host}]' if ':' in host else host
        # SharedArrayBuffer is not available from the default route
        url_host = url_host.replace("0.0.0.0", "localhost")
        self._url = f"http://{url_host}:{port}/"
        logger.info(f"Serving HTTP at {self._url}")

        self._httpd_started.set()

        while not self._server_is_shutdown:
            self._process_one_http_request()

    def _process_one_http_request(self):

        self._server_is_shutdown = False
        try:
            self._httpd.handle_request()
        except:
            logger.error(
                f"An error occurred while handling HTTP request.")
            self.exit_code = 1

        if self.exit and self.exit_code is not None:
            self._server_is_shutdown = True
            return


if __name__ == "__main__":

    parser = argparse.ArgumentParser(
        description="Run unit test in webassembly environments")

    engine_grp = parser.add_argument_group(
        title="WebAssembly Engine",
        description="Arguments that specify the webassembly engine used to run unit tests")
    engine_grp.add_argument("--engine",
                            help="Path to a webassembly execution engine. Technically, this can point to a web browser or a webview runtime like tauri/wry application.")
    engine_grp.add_argument("--engine-args",
                            help="Additional arguments that will be passed to the engine",
                            default="")

    httpd_grp = parser.add_argument_group(
        title="HTTP Server",
        description="Arguments that are relevant to the http server.")
    httpd_grp.add_argument(
        "--port",
        "-p",
        help="Specify alternate port [default: 0]",
        default=0,
        required=False,
        type=int)

    parser.add_argument("test_executable",
                        help="Path to a .js file which gets loaded as a script in the engine.")
    parser.add_argument(
        "test_args",
        help="Arguments to a unit test test.",
        nargs=argparse.REMAINDER)
    parser.add_argument(
        "--exit",
        help="After the test sends an exit code, close the engine process and shutdown HTTP server, if serving [default: False]",
        action='store_true',
        default=False,
        required=False)
    parser.add_argument(
        "-v", "--verbose",
        help="Enable verbose output",
        action='store_true',
        default=False,
        required=False
    )

    cmdline_options = parser.parse_args()
    runner = vtkWebAssemblyTestRunner(cmdline_options)
    exit(runner.run())
