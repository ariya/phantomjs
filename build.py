#!/usr/bin/env python
# -*- coding: utf-8 -*-

#
#  This file is part of the PhantomJS project from Ofi Labs.
#
#  Copyright (C) 2014 Milian Wolff, KDAB <milian.wolff@kdab.com>
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#
#    * Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in the
#      documentation and/or other materials provided with the distribution.
#    * Neither the name of the <organization> nor the
#      names of its contributors may be used to endorse or promote products
#      derived from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
#  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
#  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
#  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
#  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
#  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

import argparse
import os
import platform
import sys
import subprocess
import re
import multiprocessing

root = os.path.abspath(os.path.dirname(__file__))
third_party_names = ["libicu", "libxml", "openssl", "zlib"]
third_party_path = os.path.join(root, "src", "qt", "3rdparty")

openssl_search_paths = [{
  "name": "Brew",
  "header": "/usr/local/opt/openssl/include/openssl/opensslv.h",
  "flags": [
    "-I/usr/local/opt/openssl/include",
    "-L/usr/local/opt/openssl/lib"
  ]
}, {
  "name": "MacPorts",
  "header": "/opt/local/include/openssl/opensslv.h",
  "flags": [
    "-I/opt/local/include",
    "-L/opt/local/lib"
  ]
}]

# check if path points to an executable
# source: http://stackoverflow.com/a/377028
def isExe(fpath):
    return os.path.isfile(fpath) and os.access(fpath, os.X_OK)

# find path to executable in PATH environment variable, similar to UNIX which command
# source: http://stackoverflow.com/a/377028
def which(program):
    if isExe(program):
        return program
    else:
        for path in os.environ["PATH"].split(os.pathsep):
            path = path.strip("")
            exe_file = os.path.join(path, program)
            if isExe(exe_file):
                return exe_file
    return None

# returns the path to the QMake executable which gets built in our internal QtBase fork
def qmakePath():
    exe = "qmake"
    if platform.system() == "Windows":
        exe += ".exe"
    return os.path.abspath("src/qt/qtbase/bin/" + exe)

# returns paths for 3rd party libraries (Windows only)
def findThirdPartyDeps():
    include_dirs = []
    lib_dirs = []
    for dep in third_party_names:
        include_dirs.append("-I")
        include_dirs.append(os.path.join(third_party_path, dep, "include"))
        lib_dirs.append("-L")
        lib_dirs.append(os.path.join(third_party_path, dep, "lib"))
    return (include_dirs, lib_dirs)

class PhantomJSBuilder(object):
    options = {}
    makeCommand = []

    def __init__(self, options):
        self.options = options

        # setup make command or equivalent with arguments
        if platform.system() == "Windows":
            makeExe = which("jom.exe")
            if not makeExe:
                makeExe = "nmake"
            self.makeCommand = [makeExe, "/NOLOGO"]
        else:
            flags = []
            if self.options.jobs:
                # number of jobs explicitly given
                flags = ["-j", self.options.jobs]
            elif not re.match("-j\s*[0-9]+", os.getenv("MAKEFLAGS", "")):
                # if the MAKEFLAGS env var does not contain any "-j N", set a sane default
                flags = ["-j", multiprocessing.cpu_count()]
            self.makeCommand = ["make"]
            self.makeCommand.extend(flags)

        # if there is no git subdirectory, automatically go into no-git
        # mode
        if not os.path.isdir(".git"):
            self.options.skip_git = True

    # run the given command in the given working directory
    def execute(self, command, workingDirectory):
        # python 2 compatibility: manually convert to strings
        command = [str(c) for c in command]
        workingDirectory = os.path.abspath(workingDirectory)
        print("Executing in %s: %s" % (workingDirectory, " ".join(command)))
        if self.options.dry_run:
            return 0

        stdout = sys.stdout
        if self.options.condense_output:
            stdout = subprocess.PIPE

        process = subprocess.Popen(command, stdout=stdout, stderr=sys.stderr, cwd=workingDirectory)
        if self.options.condense_output:
            while True:
                line = process.stdout.readline()
                if line != '':
                    # Show a single . per line of stdout
                    sys.stdout.write('.')
                    sys.stdout.flush()
                else:
                    print ''
                    break
        process.wait()
        return process.returncode

    # run git clean in the specified path
    def gitClean(self, path):
        if self.options.skip_git: return 0
        return self.execute(["git", "clean", "-xfd"], path)

    # run make, nmake or jom in the specified path
    def make(self, path):
        return self.execute(self.makeCommand, path)

    # run qmake in the specified path
    def qmake(self, path, options):
        qmake = qmakePath()
        # verify that qmake was properly built
        if not isExe(qmake) and not self.options.dry_run:
            raise RuntimeError("Could not find QMake executable: %s" % qmake)
        command = [qmake]
        if self.options.qmake_args:
            command.extend(self.options.qmake_args)
        if options:
            command.extend(options)
        return self.execute(command, path)

    # returns a list of platform specific Qt Base configure options
    def platformQtConfigureOptions(self):
        platformOptions = []
        if platform.system() == "Windows":
            platformOptions = [
                "-mp",
                "-static-runtime",
                "-no-cetest",
                "-no-angle",
                "-icu",
                "-openssl",
                "-openssl-linked",
            ]
            deps = findThirdPartyDeps()
            platformOptions.extend(deps[0])
            platformOptions.extend(deps[1])
        else:
            # Unix platform options
            platformOptions = [
                # use the headless QPA platform
                "-qpa", "phantom",
                # explicitly compile with SSL support, so build will fail if headers are missing
                "-openssl", "-openssl-linked",
                # disable unnecessary Qt features
                "-no-openvg",
                "-no-eglfs",
                "-no-egl",
                "-no-glib",
                "-no-gtkstyle",
                "-no-cups",
                "-no-sm",
                "-no-xinerama",
                "-no-xkb",
                "-no-xcb",
                "-no-kms",
                "-no-linuxfb",
                "-no-directfb",
                "-no-mtdev",
                "-no-libudev",
                "-no-evdev",
                "-no-pulseaudio",
                "-no-alsa",
                "-no-feature-PRINTPREVIEWWIDGET"
            ]

            if self.options.silent:
                platformOptions.append("-silent")

            if platform.system() == "Darwin":
                # Mac OS specific options
                # NOTE: fontconfig is not required on Darwin (we use Core Text for font enumeration)
                platformOptions.extend([
                    "-no-pkg-config",
                    "-no-c++11", # Build fails on mac right now with C++11 TODO: is this still valid?
                ])
                # Dirty hack to find OpenSSL libs
                openssl = os.getenv("OPENSSL", "")
                if openssl == "":
                  # search for OpenSSL
                  openssl_found = False
                  for search_path in openssl_search_paths:
                    if os.path.exists(search_path["header"]):
                      openssl_found = True
                      platformOptions.extend(search_path["flags"])
                      print("Found OpenSSL installed via %s" % search_path["name"])

                  if not openssl_found:
                    raise RuntimeError("Could not find OpenSSL")
                else:
                  # TODO: Implement
                  raise RuntimeError("Not implemented")
            else:
                # options specific to other Unixes, like Linux, BSD, ...
                platformOptions.extend([
                    "-fontconfig", # Fontconfig for better font matching
                    "-icu", # ICU for QtWebKit (which provides the OSX headers) but not QtBase
                ])
        return platformOptions

    # configure Qt Base
    def configureQtBase(self):
        print("configuring Qt Base, please wait...")

        configureExe = os.path.abspath("src/qt/qtbase/configure")
        if platform.system() == "Windows":
            configureExe += ".bat"

        configure = [configureExe,
            "-static",
            "-opensource",
            "-confirm-license",
            # we use an in-source build for now and never want to install
            "-prefix", os.path.abspath("src/qt/qtbase"),
            # use the bundled libraries, vs. system-installed ones
            "-qt-zlib",
            "-qt-libpng",
            "-qt-libjpeg",
            "-qt-pcre",
            # disable unnecessary Qt features
            "-nomake", "examples",
            "-nomake", "tools",
            "-nomake", "tests",
            "-no-qml-debug",
            "-no-dbus",
            "-no-opengl",
            "-no-audio-backend",
            "-D", "QT_NO_GRAPHICSVIEW",
            "-D", "QT_NO_GRAPHICSEFFECT",
            "-D", "QT_NO_STYLESHEET",
            "-D", "QT_NO_STYLE_CDE",
            "-D", "QT_NO_STYLE_CLEANLOOKS",
            "-D", "QT_NO_STYLE_MOTIF",
            "-D", "QT_NO_STYLE_PLASTIQUE",
            "-D", "QT_NO_PRINTPREVIEWDIALOG"
        ]
        configure.extend(self.platformQtConfigureOptions())
        if self.options.qt_config:
            configure.extend(self.options.qt_config)

        if self.options.debug:
            configure.append("-debug")
        elif self.options.release:
            configure.append("-release")
        else:
            # build Release by default
            configure.append("-release")

        if self.execute(configure, "src/qt/qtbase") != 0:
            raise RuntimeError("Configuration of Qt Base failed.")

    # build Qt Base
    def buildQtBase(self):
        if self.options.skip_qtbase:
            print("Skipping build of Qt Base")
            return

        if self.options.git_clean_qtbase:
            self.gitClean("src/qt/qtbase")

        if self.options.git_clean_qtbase or not self.options.skip_configure_qtbase:
            self.configureQtBase()

        print("building Qt Base, please wait...")
        if self.make("src/qt/qtbase") != 0:
            raise RuntimeError("Building Qt Base failed.")

    # build Qt WebKit
    def buildQtWebKit(self):
        if self.options.skip_qtwebkit:
            print("Skipping build of Qt WebKit")
            return

        if self.options.git_clean_qtwebkit:
            self.gitClean("src/qt/qtwebkit")

        os.putenv("SQLITE3SRCDIR", os.path.abspath("src/qt/qtbase/src/3rdparty/sqlite"))

        print("configuring Qt WebKit, please wait...")
        configureOptions = [
            # disable some webkit features we do not need
            "WEBKIT_CONFIG-=build_webkit2",
            "WEBKIT_CONFIG-=netscape_plugin_api",
            "WEBKIT_CONFIG-=use_gstreamer",
            "WEBKIT_CONFIG-=use_gstreamer010",
            "WEBKIT_CONFIG-=use_native_fullscreen_video",
            "WEBKIT_CONFIG-=video",
            "WEBKIT_CONFIG-=web_audio",
        ]
        if self.options.webkit_qmake_args:
            configureOptions.extend(self.options.webkit_qmake_args)
        if self.qmake("src/qt/qtwebkit", configureOptions) != 0:
            raise RuntimeError("Configuration of Qt WebKit failed.")

        print("building Qt WebKit, please wait...")
        if self.make("src/qt/qtwebkit") != 0:
            raise RuntimeError("Building Qt WebKit failed.")

    # build PhantomJS
    def buildPhantomJS(self):
        print("Configuring PhantomJS, please wait...")
        if self.qmake(".", self.options.phantomjs_qmake_args) != 0:
            raise RuntimeError("Configuration of PhantomJS failed.")
        print("Building PhantomJS, please wait...")
        if self.make(".") != 0:
            raise RuntimeError("Building PhantomJS failed.")

    # ensure the git submodules are all available
    def ensureSubmodulesAvailable(self):
        if self.options.skip_git: return
        if self.execute(["git", "submodule", "init"], ".") != 0:
            raise RuntimeError("Initialization of git submodules failed.")
        if self.execute(["git", "submodule", "update", "--remote"], ".") != 0:
            raise RuntimeError("Initial update of git submodules failed.")

    # run all build steps required to get a final PhantomJS binary at the end
    def run(self):
        self.ensureSubmodulesAvailable();
        self.buildQtBase()
        self.buildQtWebKit()
        self.buildPhantomJS()

# parse command line arguments and return the result
def parseArguments():
    parser = argparse.ArgumentParser(description="Build PhantomJS from sources.")
    parser.add_argument("-r", "--release", action="store_true",
                            help="Enable compiler optimizations.")
    parser.add_argument("-d", "--debug", action="store_true",
                            help="Build with debug symbols enabled.")
    parser.add_argument("-j", "--jobs", type=int,
                            help="How many parallel compile jobs to use. Defaults to %d." % multiprocessing.cpu_count())
    parser.add_argument("-c", "--confirm", action="store_true",
                            help="Silently confirm the build.")
    parser.add_argument("-n", "--dry-run", action="store_true",
                            help="Only print what would be done without actually executing anything.")

    # NOTE: silent build does not exist on windows apparently
    if platform.system() != "Windows":
        parser.add_argument("-s", "--silent", action="store_true",
                            help="Reduce output during compilation.")

    advanced = parser.add_argument_group("advanced options")
    advanced.add_argument("--qmake-args", type=str, action="append",
                            help="Additional arguments that will be passed to all QMake calls.")
    advanced.add_argument("--webkit-qmake-args", type=str, action="append",
                            help="Additional arguments that will be passed to the Qt WebKit QMake call.")
    advanced.add_argument("--phantomjs-qmake-args", type=str, action="append",
                            help="Additional arguments that will be passed to the PhantomJS QMake call.")
    advanced.add_argument("--qt-config", type=str, action="append",
                            help="Additional arguments that will be passed to Qt Base configure.")
    advanced.add_argument("--git-clean-qtbase", action="store_true",
                            help="Run git clean in the Qt Base folder.\n"
                                 "ATTENTION: This will remove all untracked files!")
    advanced.add_argument("--git-clean-qtwebkit", action="store_true",
                            help="Run git clean in the Qt WebKit folder.\n"
                                 "ATTENTION: This will remove all untracked files!")
    advanced.add_argument("--skip-qtbase", action="store_true",
                            help="Skip Qt Base completely and do not build it.\n"
                                 "Only enable this option when Qt Base was built "
                                 "previously and no update is required.")
    advanced.add_argument("--skip-configure-qtbase", action="store_true",
                            help="Skip configure step of Qt Base, only build it.\n"
                                 "Only enable this option when the environment has "
                                 "not changed and only an update of Qt Base is required.")
    advanced.add_argument("--skip-qtwebkit", action="store_true",
                            help="Skip Qt WebKit completely and do not build it.\n"
                                 "Only enable this option when Qt WebKit was built "
                                 "previously and no update is required.")
    advanced.add_argument("--skip-configure-qtwebkit", action="store_true",
                            help="Skip configure step of Qt WebKit, only build it.\n"
                                 "Only enable this option when neither the environment nor Qt Base "
                                 "has changed and only an update of Qt WebKit is required.")
    advanced.add_argument("--skip-git", action="store_true",
                            help="Skip all actions that require Git.  For use when building from "
                                 "a tarball release.")
    advanced.add_argument("--condense-output", action="store_true",
                            help="Condense build output by replacing stdout lines with '.' ")
    options = parser.parse_args()
    if options.debug and options.release:
        raise RuntimeError("Cannot build with both debug and release mode enabled.")
    return options

# main entry point which gets executed when this script is run
def main():
    # change working directory to the folder this script lives in
    os.chdir(os.path.dirname(os.path.realpath(__file__)))

    try:
        options = parseArguments()
        if not options.confirm:
            print("""\
----------------------------------------
               WARNING
----------------------------------------

Building PhantomJS from source takes a very long time, anywhere from 30 minutes
to several hours (depending on the machine configuration). It is recommended to
use the premade binary packages on supported operating systems.

For details, please go the the web site: http://phantomjs.org/download.html.
""")
            while True:
                sys.stdout.write("Do you want to continue (Y/n)? ")
                sys.stdout.flush()
                answer = sys.stdin.readline().strip().lower()
                if answer == "n":
                    print("Cancelling PhantomJS build.")
                    return
                elif answer == "y" or answer == "":
                    break
                else:
                    print("Invalid answer, try again.")

        builder = PhantomJSBuilder(options)
        builder.run()
    except RuntimeError as error:
        sys.stderr.write("\nERROR: Failed to build PhantomJS! %s\n" % error)
        sys.stderr.flush()
        sys.exit(1)

if __name__ == "__main__":
    main()
