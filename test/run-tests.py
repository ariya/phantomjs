#!/usr/bin/env python

import argparse
import errno
import glob
import imp
import os
import posixpath
import shlex
import SimpleHTTPServer
import socket
import SocketServer
import ssl
import string
import cStringIO as StringIO
import subprocess
import sys
import threading
import time
import traceback
import urllib

# All files matching one of these glob patterns will be run as tests.
TESTS = [
    'basics/*.js',
    'module/*/*.js',
    'standards/*/*.js',
    'regression/*.js',
    'run-tests.js'
]

TIMEOUT    = 20    # Maximum duration of PhantomJS execution (in seconds).
                   # There's currently no way to adjust this on a per-test
                   # basis, so it has to be large.

HTTP_PORT  = 9180  # These are currently hardwired into every test that
HTTPS_PORT = 9181  # uses the test servers.

# create_default_context and SSLContext were only added in 2.7.9,
# which is newer than the python2 that ships with OSX :-(
# The fallback tries to mimic what create_default_context(CLIENT_AUTH)
# does.  Security obviously isn't important in itself for a test
# server, but making sure the PJS client can talk to a server
# configured according to modern TLS best practices _is_ important.
# Unfortunately, there is no way to set things like OP_NO_SSL2 or
# OP_CIPHER_SERVER_PREFERENCE prior to 2.7.9.
CIPHERLIST_2_7_9 = (
    'ECDH+AESGCM:DH+AESGCM:ECDH+AES256:DH+AES256:ECDH+AES128:DH+AES:ECDH+HIGH:'
    'DH+HIGH:ECDH+3DES:DH+3DES:RSA+AESGCM:RSA+AES:RSA+HIGH:RSA+3DES:!aNULL:'
    '!eNULL:!MD5:!DSS:!RC4'
)
def wrap_socket_ssl(sock, base_path):
    crtfile = os.path.join(base_path, 'certs/https-snakeoil.crt')
    keyfile = os.path.join(base_path, 'certs/https-snakeoil.key')

    try:
        ctx = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
        ctx.load_cert_chain(crtfile, keyfile)
        return ctx.wrap_socket(sock, server_side=True)

    except AttributeError:
        return ssl.wrap_socket(sock,
                               keyfile=keyfile,
                               certfile=crtfile,
                               server_side=True,
                               ciphers=CIPHERLIST_2_7_9)

# This should be in the standard library somewhere, but as far as I
# can tell, it isn't.
class ResponseHookImporter(object):
    def __init__(self, www_path):
        # All Python response hooks, no matter how deep below www_path,
        # are treated as direct children of the fake "test_www" package.
        if 'test_www' not in sys.modules:
            imp.load_source('test_www', www_path + '/__init__.py')

        self.tr = string.maketrans('-./%', '____')

    def __call__(self, path):
        modname = 'test_www.' + path.translate(self.tr)
        try:
            return sys.modules[modname]
        except KeyError:
            return imp.load_source(modname, path)

# This should also be in the standard library somewhere, and definitely
# isn't.
# FIXME: Use non-blocking I/O and select on Unix for efficiency.
# FIXME: Can hang forever if the subprocess closes its stdout but doesn't exit.
def record_process_output(proc, verbose, timeout):

    def read_thread(linebuf, fp):
        while True:
            line = fp.readline().rstrip()
            if not line: break # EOF
            line = line.rstrip()
            if line:
                linebuf.append(line)
                if verbose:
                    sys.stdout.write(line + '\n')

    stdout = []
    stderr = []
    sothrd = threading.Thread(target=read_thread, args=(stdout, proc.stdout))
    sethrd = threading.Thread(target=read_thread, args=(stderr, proc.stderr))
    timed_out = False

    sothrd.start()
    sethrd.start()
    sothrd.join(timeout)
    if sothrd.is_alive():
        timed_out = True
        proc.terminate()
        sothrd.join()
    sethrd.join()
    proc.wait()
    if timed_out:
        stderr.append("  TIMEOUT | Process terminated after {} seconds."
                      .format(timeout))
    return proc.returncode, stdout, stderr


class FileHandler(SimpleHTTPServer.SimpleHTTPRequestHandler, object):

    def __init__(self, *args, **kwargs):
        self._cached_untranslated_path = None
        self._cached_translated_path = None
        self.postdata = None
        super(FileHandler, self).__init__(*args, **kwargs)

    # silent, do not pollute stdout nor stderr.
    def log_message(self, format, *args):
        return

    # accept POSTs, read the postdata and stash it in an instance variable,
    # then forward to do_GET; handle_request hooks can vary their behavior
    # based on the presence of postdata and/or the command verb.
    def do_POST(self):
        try:
            ln = int(self.headers.get('content-length'))
        except TypeError, ValueError:
            self.send_response(400, 'Bad Request')
            self.send_header('Content-Type', 'text/plain')
            self.end_headers()
            self.wfile.write("No or invalid Content-Length in POST (%r)"
                             % self.headers.get('content-length'))
            return

        self.postdata = self.rfile.read(ln)
        self.do_GET()

    # allow provision of a .py file that will be interpreted to
    # produce the response.
    def send_head(self):
        path = self.translate_path(self.path)

        # do not allow direct references to .py(c) files,
        # or indirect references to __init__.py
        if (path.endswith('.py') or path.endswith('.pyc') or
            path.endswith('__init__')):
            self.send_error(404, 'File not found')
            return None

        if os.path.exists(path):
            return super(FileHandler, self).send_head()

        py = path + '.py'
        if os.path.exists(py):
            try:
                mod = self.get_response_hook(py)
                return mod.handle_request(self)
            except:
                self.send_error(500, 'Internal Server Error in '+py)
                raise

        self.send_error(404, 'File not found')
        return None

    # modified version of SimpleHTTPRequestHandler's translate_path
    # to resolve the URL relative to the www/ directory
    # (e.g. /foo -> test/www/foo)
    def translate_path(self, path):

        # Cache for efficiency, since our send_head calls this and
        # then, in the normal case, the parent class's send_head
        # immediately calls it again.
        if (self._cached_translated_path is not None and
            self._cached_untranslated_path == path):
            return self._cached_translated_path

        orig_path = path

        # Strip query string and/or fragment, if present.
        x = path.find('?')
        if x != -1: path = path[:x]
        x = path.find('#')
        if x != -1: path = path[:x]

        # Ensure consistent encoding of special characters, then
        # lowercase everything so that the tests behave consistently
        # whether or not the local filesystem is case-sensitive.
        path = urllib.quote(urllib.unquote(path)).lower()

        # Prevent access to files outside www/.
        # At this point we want specifically POSIX-like treatment of 'path'
        # because it is still a URL component and not a filesystem path.
        # SimpleHTTPRequestHandler.send_head() expects us to preserve the
        # distinction between paths with and without a trailing slash, but
        # posixpath.normpath() discards that distinction.
        trailing_slash = path.endswith('/')
        path = posixpath.normpath(path)
        while path.startswith('/'):
            path = path[1:]
        while path.startswith('../'):
            path = path[3:]

        # Now resolve the normalized, clamped path relative to the www/
        # directory, according to local OS conventions.
        path = os.path.normpath(os.path.join(self.www_path, *path.split('/')))
        if trailing_slash:
            # it must be a '/' even on Windows
            path += '/'

        self._cached_untranslated_path = orig_path
        self._cached_translated_path = path
        return path

class TCPServer(SocketServer.ThreadingMixIn, SocketServer.TCPServer):
    # This is how you are officially supposed to set SO_REUSEADDR per
    # https://docs.python.org/2/library/socketserver.html#SocketServer.BaseServer.allow_reuse_address
    allow_reuse_address = True

    def __init__(self, port, use_ssl, handler, base_path, signal_error):
        SocketServer.TCPServer.__init__(self, ('localhost', port), handler)
        if use_ssl:
            self.socket = wrap_socket_ssl(self.socket, base_path)
        self._signal_error = signal_error

    def handle_error(self, request, client_address):
        # Ignore errors which can occur naturally if the client
        # disconnects in the middle of a request.  EPIPE and
        # ECONNRESET *should* be the only such error codes
        # (according to the OSX manpage for send()).
        _, exval, _ = sys.exc_info()
        if getattr(exval, 'errno', None) in (errno.EPIPE, errno.ECONNRESET):
            return

        # Otherwise, report the error to the test runner.
        self._signal_error(sys.exc_info())

class HTTPTestServer(object):
    def __init__(self, base_path, signal_error):
        self.httpd        = None
        self.httpsd       = None
        self.base_path    = base_path
        self.www_path     = os.path.join(base_path, 'www')
        self.signal_error = signal_error

    def __enter__(self):
        handler = FileHandler
        handler.extensions_map.update({
            '.htm': 'text/html',
            '.html': 'text/html',
            '.css': 'text/css',
            '.js': 'application/javascript',
            '.json': 'application/json'
        })
        handler.www_path = self.www_path
        handler.get_response_hook = ResponseHookImporter(self.www_path)

        self.httpd  = TCPServer(HTTP_PORT, False,
                                handler, self.base_path, self.signal_error)
        httpd_thread = threading.Thread(target=self.httpd.serve_forever)
        httpd_thread.daemon = True
        httpd_thread.start()

        self.httpsd = TCPServer(HTTPS_PORT, True,
                                handler, self.base_path, self.signal_error)
        httpsd_thread = threading.Thread(target=self.httpsd.serve_forever)
        httpsd_thread.daemon = True
        httpsd_thread.start()

        return self

    def __exit__(self, *dontcare):
        self.httpd.shutdown()
        self.httpsd.shutdown()

class TestRunner(object):
    def __init__(self, base_path, phantomjs_exe, options):
        self.base_path       = base_path
        self.cert_path       = os.path.join(base_path, 'certs')
        self.phantomjs_exe   = phantomjs_exe
        self.verbose         = options.verbose
        self.debugger        = options.debugger
        self.to_run          = options.to_run
        self.http_server_err = None

    def signal_server_error(self, exc_info):
        self.http_server_err = exc_info

    def get_base_command(self, debugger):
        if debugger is None:
            return [self.phantomjs_exe]
        elif debugger == "gdb":
            return ["gdb", "--args", self.phantomjs_exe]
        elif debugger == "lldb":
            return ["lldb", "--", self.phantomjs_exe]
        else:
            raise RuntimeError("Don't know how to invoke " + self.debugger)

    def run_phantomjs(self, script, script_args=[], pjs_args=[], silent=False):
        verbose  = self.verbose
        debugger = self.debugger
        if silent:
            verbose = False
            debugger = None

        output = []
        command = self.get_base_command(debugger)
        command.extend(pjs_args)
        command.append(script)
        if verbose:
            command.append('--verbose={}'.format(verbose))
        command.extend(script_args)

        if verbose >= 3:
            sys.stdout.write("## running {}\n".format(" ".join(command)))

        if debugger:
            subprocess.call(command)
            return 0, [], []
        else:
            proc = subprocess.Popen(command,
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE)
            return record_process_output(proc, verbose, TIMEOUT)

    def run_test(self, script, name):
        script_args = []
        pjs_args = []
        use_snakeoil = True

        # Parse any directives at the top of the script.
        try:
            with open(script, "rt") as s:
                for line in s:
                    if not line.startswith("//!"):
                        break
                    tokens = shlex.split(line[3:], comments=True)

                    for i in range(len(tokens)):
                        tok = tokens[i]
                        if tok == "no-snakeoil":
                            use_snakeoil = False
                        elif tok == "phantomjs:":
                            if i+1 == len(tokens):
                                raise ValueError("phantomjs: directive requires"
                                                 "at least one argument")
                            pjs_args.extend(tokens[(i+1):])
                            break
                        elif tok == "script:":
                            if i+1 == len(tokens):
                                raise ValueError("script: directive requires"
                                                 "at least one argument")
                            script_args.extend(tokens[(i+1):])
                            break
                        else:
                            raise ValueError("unrecognized directive: " + tok)

        except OSError as e:
            sys.stdout.write('{} ({}): {}\n'
                             .format(name, e.filename, e.strerror))
            return 1
        except Exception as e:
            sys.stdout.write('{} ({}): {}\n'
                             .format(name, script, str(e)))
            return 1

        if use_snakeoil:
            pjs_args.insert(0, '--ssl-certificates-path=' + self.cert_path)

        sys.stdout.write(name + ":\n")
        self.http_server_err = None
        returncode, out, err = self.run_phantomjs(script, script_args, pjs_args)

        if returncode != 0:
            if not self.verbose:
                if out:
                    sys.stdout.write("\n".join(out))
                    sys.stdout.write("\n")
                if err:
                    sys.stdout.write("\n".join(err))
                    sys.stdout.write("\n")

        if self.http_server_err is not None:
            ty, val, tb = self.http_server_err
            sys.stdout.write("    ERROR | httpd: {}\n".format(
                traceback.format_exception_only(ty, val)[-1]))
            if self.verbose:
                for line in traceback.format_tb(tb, 5):
                    sys.stdout.write("## " + line)
            return 1

        return returncode

    def run_tests(self):
        start = time.time()

        result = 0
        any_executed = False
        base = self.base_path
        nlen = len(base) + 1
        for test_group in TESTS:
            test_glob = os.path.join(base, test_group)

            for test_script in sorted(glob.glob(test_glob)):
                tname = os.path.splitext(test_script)[0][nlen:]
                if self.to_run:
                    for to_run in self.to_run:
                        if to_run in tname:
                            break
                    else:
                        continue

                any_executed = True
                ret = self.run_test(test_script, tname)
                if ret != 0:
                    sys.stdout.write('The test {} FAILED\n\n'.format(tname))
                    result = 1

        if not any_executed:
            sys.stdout.write("All tests skipped.\n")
            return 1

        if result == 0:
            sys.stdout.write("\nAll tests successful. "
                             "Total time: {:.3f} seconds.\n"
                             .format(time.time() - start))

        return result

def init():
    base_path = os.path.normpath(os.path.dirname(os.path.abspath(__file__)))
    phantomjs_exe = os.path.normpath(base_path + '/../bin/phantomjs')
    if sys.platform in ('win32', 'cygwin'):
        phantomjs_exe += '.exe'
    if not os.path.isfile(phantomjs_exe):
        sys.stdout.write("{} is unavailable, cannot run tests.\n"
                         .format(phantomjs_exe))
        sys.exit(1)

    parser = argparse.ArgumentParser(description='Run PhantomJS tests.')
    parser.add_argument('-v', '--verbose', action='count',
                        help='Increase verbosity of logs (repeat for more)')
    parser.add_argument('to_run', nargs='*', metavar='test',
                        help='tests to run (default: all of them)')
    parser.add_argument('--debugger', default=None,
                        help="Run PhantomJS under DEBUGGER")

    options = parser.parse_args()
    runner = TestRunner(base_path, phantomjs_exe, options)
    if options.verbose:
        rc, ver, err = runner.run_phantomjs('--version', silent=True)
        if rc != 0 or len(ver) != 1 or len(err) != 0:
            sys.stdout.write("    ERROR | Version check failed\n")
            for l in ver: sys.stdout.write("## {}\n".format(l))
            for l in err: sys.stdout.write("## {}\n".format(l))
            sys.stdout.write("## exit {}".format(rc))
            sys.exit(1)

        sys.stdout.write("     INFO | Testing PhantomJS {}\n".format(ver[0]))

    return runner

def main():
    runner = init()
    try:
        with HTTPTestServer(runner.base_path, runner.signal_server_error):
            sys.exit(runner.run_tests())

    except Exception as e:
        ty, val, tb = sys.exc_info()
        sys.stdout.write("    ERROR | {}\n".format(
            traceback.format_exception_only(ty, val)[-1]))
        if runner.verbose:
            for line in traceback.format_tb(tb, 5):
                sys.stdout.write("## " + line)

        sys.exit(1)

main()
