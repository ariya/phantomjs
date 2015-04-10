#!/usr/bin/env python

import os, random, sys, time, urllib

#
# Options
#

dry_run = len(sys.argv) > 1 and "--dry-run" in set(sys.argv[1:])
quiet = len(sys.argv) > 1 and "--quiet" in set(sys.argv[1:])

#
# Functions and constants
#

def download_progress_hook(block_count, block_size, total_blocks):
        if quiet or random.random() > 0.5:
                return
        sys.stdout.write(".")
        sys.stdout.flush()

def download_url_to_file(url, file, message):
        if not quiet:
                print message + " ",
        if not dry_run:
                dir = os.path.dirname(file)
                if len(dir) and not os.path.exists(dir):
                    os.makedirs(dir)
                urllib.urlretrieve(url, file, download_progress_hook)
        if not quiet:
                print
 
# This is mostly just the list of North America http mirrors from http://cygwin.com/mirrors.html,
# but a few have been removed that seemed unresponsive from Cupertino.
mirror_servers = ["http://cygwin.elite-systems.org/",
                  "http://mirror.mcs.anl.gov/cygwin/",
                  "http://cygwin.osuosl.org/",
                  "http://mirrors.kernel.org/sourceware/cygwin/",
                  "http://mirrors.xmission.com/cygwin/",
                  "http://sourceware.mirrors.tds.net/pub/sourceware.org/cygwin/"]

package_mirror_url = mirror_servers[random.choice(range(len(mirror_servers)))]

def download_package(package, message):
        download_url_to_file(package_mirror_url + package["path"], package["path"], message)

required_packages = frozenset(["apache",
                               "bc",
                               "bison",
                               "curl",
                               "diffutils",
                               "e2fsprogs",
                               "emacs",
                               "flex",
                               "gcc",
                               "gperf",
                               "keychain",
                               "make",
                               "minires",
                               "nano",
                               "openssh",
                               "patch",
                               "perl",
                               "perl-libwin32",
                               "python",
                               "rebase",
                               "rsync",
                               "ruby",
                               "subversion",
                               "unzip",
                               "vim",
                               "zip"])

#
# Main
#

print "Using Cygwin mirror server " + package_mirror_url + " to download setup.ini..."

urllib.urlretrieve(package_mirror_url + "setup.ini", "setup.ini.orig")

downloaded_packages_file_path = "setup.ini.orig"
downloaded_packages_file = file(downloaded_packages_file_path, "r")
if not dry_run:
    modified_packages_file = file("setup.ini", "w")

packages = {}
current_package = ''
for line in downloaded_packages_file.readlines():
        if line[0] == "@":
                current_package = line[2:-1]
                packages[current_package] = {"name": current_package, "needs_download": False, "requires": [], "path": ""}
        elif line[:10] == "category: ":
                if current_package in required_packages:
                        line = "category: Base\n"
                if "Base" in set(line[10:-1].split()):
                        packages[current_package]["needs_download"] = True
        elif line[:10] == "requires: ":
                packages[current_package]["requires"] = line[10:].split()
                packages[current_package]["requires"].sort()
        elif line[:9] == "install: " and not len(packages[current_package]["path"]):
                end_of_path = line.find(" ", 9)
                if end_of_path != -1:
                        packages[current_package]["path"] = line[9:end_of_path]
        if not dry_run:
            modified_packages_file.write(line)

downloaded_packages_file.close()
os.remove(downloaded_packages_file_path)
if not dry_run:
    modified_packages_file.close()

names_to_download = set()
package_names = packages.keys()
package_names.sort()

def add_package_and_dependencies(name):
        if name in names_to_download:
                return
        if not name in packages:
                return
        packages[name]["needs_download"] = True
        names_to_download.add(name)
        for dep in packages[name]["requires"]:
                add_package_and_dependencies(dep)

for name in package_names:
        if packages[name]["needs_download"]:
                add_package_and_dependencies(name)

downloaded_so_far = 0
for name in package_names:
        if packages[name]["needs_download"]:
                downloaded_so_far += 1
                download_package(packages[name], "Downloading package %3d of %3d (%s)" % (downloaded_so_far, len(names_to_download), name))

download_url_to_file("http://cygwin.com/setup.exe", "setup.exe", "Downloading setup.exe")

seconds_to_sleep = 10

print """
Finished downloading Cygwin. In %d seconds,
I will run setup.exe. Select the "Install
from Local Directory" option and browse to
"%s"
when asked for the "Local Package Directory".
""" % (seconds_to_sleep, os.getcwd())


while seconds_to_sleep > 0:
        print "%d..." % seconds_to_sleep,
        sys.stdout.flush()
        time.sleep(1)
        seconds_to_sleep -= 1
print

if not dry_run:
        os.execl("setup.exe")
