#!/usr/bin/env python

import cStringIO as StringIO
import glob
import imp
import optparse
import os
import posixpath
import shlex
import SimpleHTTPServer
import SocketServer
import socket
import string
import subprocess
import sys
import threading
import time
import urllib

TIMEOUT = 10  # Maximum duration of PhantomJS execution (in seconds)

HTTP_PORT = 9180
http_running = False

TESTS = [
    'basics/*.js',
    'module/system/*.js',
    'module/webpage/*.js',
    'module/cookiejar/*.js',
    'standards/javascript/*.js',
    'standards/console/*.js',
    'regression/*.js',
    'run-tests.js'
]

# This should be in the standard library somewhere, but as far as I
# can tell, isn't.
def import_file_as_module(path):
    # All Python response hooks, no matter how deep below www_path,
    # are treated as direct children of the fake "test_www" package.
    if 'test_www' not in sys.modules:
        imp.load_source('test_www', www_path + '/__init__.py')

    tr = string.maketrans('-./%', '____')
    modname = 'test_www.' + path.translate(tr)
    try:
        return sys.modules[modname]
    except KeyError:
        return imp.load_source(modname, path)

class FileHandler(SimpleHTTPServer.SimpleHTTPRequestHandler, object):

    def __init__(self, *args, **kwargs):
        self._cached_untranslated_path = None
        self._cached_translated_path = None
        super(FileHandler, self).__init__(*args, **kwargs)

    # silent, do not pollute stdout nor stderr.
    def log_message(self, format, *args):
        return

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
                mod = import_file_as_module(py)
                return mod.handle_request(self)
            except:
                import cgitb
                buf = StringIO.StringIO()
                cgitb.Hook(file=buf).handle()
                buf = buf.getvalue()

                self.send_response(500, 'Internal Server Error')
                self.send_header('Content-Type', 'text/html')
                self.send_header('Content-Length', str(len(buf)))
                self.end_headers()
                return StringIO.StringIO(buf)

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
        path = os.path.normpath(os.path.join(www_path, *path.split('/')))
        if trailing_slash:
            # it must be a '/' even on Windows
            path += '/'

        self._cached_untranslated_path = orig_path
        self._cached_translated_path = path
        return path

# This is how you are officially supposed to set SO_REUSEADDR per
# https://docs.python.org/2/library/socketserver.html#SocketServer.BaseServer.allow_reuse_address

class TCPServer(SocketServer.TCPServer):
    allow_reuse_address = True

def run_httpd():
    global http_running
    handler = FileHandler
    handler.extensions_map.update({
        '.htm': 'text/html',
        '.html': 'text/html',
        '.css': 'text/css',
        '.js': 'application/javascript',
        '.json': 'application/json'
    })
    try:
        httpd = TCPServer(('', HTTP_PORT), handler)
        while http_running:
            httpd.handle_request()
    except socket.error as e:
        print 'Fatal error: unable to launch a test server at port', HTTP_PORT
        print str(e)
        http_running = False
        sys.exit(1)
    return


def setup_server():
    global http_running

    http_running = True
    httpd_thread = threading.Thread(target=run_httpd)
    httpd_thread.start()

    time.sleep(2)
    if http_running:
        if options.verbose:
            print 'serving at port', HTTP_PORT


def terminate_server():
    global http_running
    http_running = False

    # ping the server once to trigger the check for http_running (after every request)
    urllib.urlopen('http://localhost:{0}'.format(HTTP_PORT))


def init():
    global base_path, www_path, phantomjs_exe, options

    base_path = os.path.dirname(os.path.abspath(__file__))
    phantomjs_exe = os.path.normpath(base_path + '/../bin/phantomjs')
    if not os.path.isfile(phantomjs_exe):
        print 'Could not locate ' + phantomjs_exe
        sys.exit(1)

    www_path = os.path.join(base_path, 'www')

    parser = optparse.OptionParser(
        usage='%prog [options] [tests to run...]',
        description='Run PhantomJS tests.'
    )
    parser.add_option('--verbose', action='store_true', default=False, help='Show a verbose log')
    (options, args) = parser.parse_args(sys.argv[1:])

    options.to_run = args

    if options.verbose:
        returncode, version = run_phantomjs('--version')
        print 'Checking PhantomJS version %s' % version


def run_phantomjs(script, script_args=[], pjs_args=[]):
    output = []
    command = [phantomjs_exe]
    command.extend(pjs_args)
    command.append(script)
    command.extend(script_args)
    process = subprocess.Popen(command, stderr=subprocess.STDOUT, stdout=subprocess.PIPE)

    def runner():
        while True:
            line = process.stdout.readline()
            if line != '':
                output.append(line.rstrip())
                if options.verbose:
                    print '%s' % line.rstrip()
            else:
                break

    thread = threading.Thread(target=runner)
    thread.start()
    thread.join(TIMEOUT)
    if thread.is_alive():
        print 'Process is running more than ' + str(TIMEOUT) + ' seconds. Terminating...'
        process.terminate()
        thread.join()
    process.wait()
    return process.returncode, '\n'.join(output)


def run_test(script, name):
    script_args = []
    pjs_args = []
    if options.verbose:
        script_args.append('--verbose')

    try:
        with open(script, "rt") as s:
            p_prefix = "// phantomjs: "
            s_prefix = "// script: "
            for line in s:
                if line.startswith(p_prefix):
                    pjs_args.extend(shlex.split(line[len(p_prefix):]))
                if line.startswith(s_prefix):
                    script_args.extend(shlex.split(line[len(s_prefix):]))
                if not line.startswith("//"):
                    break
    except OSError as e:
        print '%s: %s: %s' % (name, e.filename, e.strerror)
        return 1

    print '%s:' % name
    returncode, output = run_phantomjs(script, script_args, pjs_args)
    if returncode != 0:
        if not options.verbose:
            print '%s' % output
        return 1

    return 0


def run_tests():
    setup_server()
    if not http_running:
        return 1

    start = time.time()
    if options.verbose:
        print 'Starting the tests...'

    result = 0
    any_executed = False
    for test_group in TESTS:
        test_group_name = os.path.dirname(test_group)
        test_glob = os.path.normpath(base_path + '/' + test_group)

        if options.verbose:
            print
            print 'Test group: %s...' % test_group_name

        for test_script in glob.glob(test_glob):
            tname = test_group_name + '/' + os.path.basename(test_script)
            if options.to_run:
                for to_run in options.to_run:
                    if to_run in tname:
                        break
                else:
                    if options.verbose:
                        print "%s: skipped" % tname
                    continue

            any_executed = True
            ret = run_test(test_script, tname)
            if ret != 0:
                print 'The test %s FAILED' % tname
                print
                result = 1

    if not any_executed:
        result = 1
        print
        print 'ALL TESTS SKIPPED.'

    if result == 0:
        print
        print 'No failure. Finished in %d seconds.' % (time.time() - start)

    terminate_server()
    return result


init()
sys.exit(run_tests())
