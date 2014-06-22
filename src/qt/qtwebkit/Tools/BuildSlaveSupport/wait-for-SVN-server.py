#!/usr/bin/env python
#
# Copyright (C) 2006 John Pye
# Copyright (C) 2012 University of Szeged
#
# This script is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
# USA

from optparse import OptionParser
import exceptions
import sys
import time
import xml.dom.minidom
import os
import subprocess


def getLatestSVNRevision(SVNServer):
    try:
        p = subprocess.Popen(["svn", "log", "--non-interactive", "--verbose", "--xml", "--limit=1", SVNServer], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        response = p.communicate()[0]
        doc = xml.dom.minidom.parseString(response)
        el = doc.getElementsByTagName("logentry")[0]
        return el.getAttribute("revision")
    except xml.parsers.expat.ExpatError, e:
        print "FAILED TO PARSE 'svn log' XML:"
        print str(e)
        print "----"
        print "RECEIVED TEXT:"
        print response
        sys.exit(1)


def waitForSVNRevision(SVNServer, revision):
    if not revision or not revision.isdigit():
        latestRevision = int(getLatestSVNRevision(SVNServer))
        print "Latest SVN revision on %s is r%d. Don't wait, because revision argument isn't a valid SVN revision." % (SVNServer, latestRevision)
        return

    revision = int(revision)
    while True:
        latestRevision = int(getLatestSVNRevision(SVNServer))
        if latestRevision < revision:
            print "Latest SVN revision on %s is r%d, but we are waiting for r%d. Sleeping for 5 seconds." % (SVNServer, latestRevision, revision)
            time.sleep(5)
        else:
            print "Latest SVN revision on %s is r%d, which is newer or equal than r%d." % (SVNServer, latestRevision, revision)
            break


if __name__ == '__main__':
    parser = OptionParser()
    parser.add_option("-r", "--revision", dest="revision", help="SVN revision number")
    parser.add_option("-s", "--svn-server", dest="SVNServer", help="SVN server")
    options, args = parser.parse_args()
    waitForSVNRevision(options.SVNServer, options.revision)
