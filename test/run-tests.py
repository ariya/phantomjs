#!/usr/bin/env python

import optparse
import os
import subprocess
import sys
import threading
import time

TIMEOUT = 35  # Maximum duration of PhantomJS execution (in seconds)

TESTS = [
    'basics/exit.js',
    'basics/global.js',
    'basics/onerror.js',
    'basics/stacktrace.js',
    'basics/version.js',
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

    args = [];
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
    return result



init()
sys.exit(run_tests())
