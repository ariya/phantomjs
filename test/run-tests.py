#!/usr/bin/env python

import inspect
import optparse
import os
import posixpath
import SimpleHTTPServer
import SocketServer
import socket
import subprocess
import sys
import threading
import time
import urllib

TIMEOUT = 35  # Maximum duration of PhantomJS execution (in seconds)

HTTP_PORT = 9180
http_running = False

TESTS = [
    'basics/exit.js',
    'basics/global.js',
    'basics/onerror.js',
    'basics/stacktrace.js',
    'basics/version.js',
    'module/webpage/open.js',
    'module/system/system.js',
    'module/system/args.js',
    'module/system/os.js',
    'module/system/pid.js',
    'module/system/stdout.js',
    'module/system/stdin.js',
    'module/system/stderr.js',
    'standards/javascript/date.js',
    'standards/javascript/function.js',
    'regression/issue12482.js',
    'run-tests.js'
]


class FileHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):

    # silent, do not pollute stdout nor stderr.
    def log_message(self, format, *args):
        return

    # modified version of SimpleHTTPRequestHandler's translate_path
    # to resolve the URL relative to the www/ directory
    # (e.g. /foo -> test/www/foo)
    def translate_path(self, path):
        path = path.split('?', 1)[0]
        path = path.split('#', 1)[0]
        path = posixpath.normpath(urllib.unquote(path))
        words = path.split('/')
        words = filter(None, words)

        path = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
        path += '/www'

        for word in words:
            drive, word = os.path.splitdrive(word)
            head, word = os.path.split(word)
            if word in (os.curdir, os.pardir):
                continue
            path = os.path.join(path, word)
        return path


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
        httpd = SocketServer.TCPServer(('', HTTP_PORT), handler)
        while http_running:
            httpd.handle_request()
    except socket.error:
        print 'Fatal error: unable to launch a test server at port', HTTP_PORT
        print 'Check that the port is not already used!'
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
    global base_path, phantomjs_exe, options

    base_path = os.path.dirname(os.path.abspath(__file__))
    phantomjs_exe = os.path.normpath(base_path + '/../bin/phantomjs')
    if not os.path.isfile(phantomjs_exe):
        print 'Could not locate ' + phantomjs_exe
        sys.exit(1)

    parser = optparse.OptionParser(
        usage='%prog [options]',
        description='Run PhantomJS tests.'
    )
    parser.add_option('--verbose', action='store_true', default=False, help='Show a verbose log')
    (options, args) = parser.parse_args(sys.argv[1:])

    if options.verbose:
        returncode, version = run_phantomjs('--version')
        print 'Checking PhantomJS version %s' % version


def run_phantomjs(script, args=[]):
    output = []
    command = [phantomjs_exe, script]
    command.extend(args)
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


def run_test(filename):
    script = os.path.normpath(base_path + '/' + filename)

    args = []
    if options.verbose:
        args.append('--verbose')

    result = 0
    if not os.path.isfile(script):
        print 'Could not locate %s' % filename
        result = 1
    else:
        print '%s:' % filename
        returncode, output = run_phantomjs(script, args)
        if returncode != 0:
            if not options.verbose:
                print '%s' % output
            result = 1

    return result


def run_tests():
    setup_server()
    if not http_running:
        return

    start = time.time()
    if options.verbose:
        print 'Starting the tests...'
        print

    result = 0
    for test in TESTS:
        ret = run_test(test)
        if ret != 0:
            print 'The test %s FAILED' % test
            print
            result = 1

    if result == 0:
        print
        print 'No failure. Finished in %d seconds.' % (time.time() - start)

    terminate_server()
    return result


init()
sys.exit(run_tests())
