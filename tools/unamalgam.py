#!/usr/bin/env python
import sys
import re
import os

cur_src = None
in_mod = False
ofile = None

strip_re = re.compile(r'/\* Amalgamated: (.*) \*/')
def clean(l):
    return strip_re.sub(r'\1', l)

manifest = []
fname = sys.argv[1]
with open(fname) as f:
    for l in f:
        if re.match('#ifdef .*_MODULE_LINES', l):
            l = next(f)
            g = re.match(r'#line [01] "(.*)"', l)
            cur_src = g.group(1)

            # if there is currently opened file, close it
            if ofile:
                ofile.close()

            cur_src = re.sub(r'\.\./', '', cur_src)

            # create directory for the next file if needed
            cur_src_dir = os.path.dirname(cur_src)
            if cur_src_dir != '' and not os.path.exists(cur_src_dir):
                os.makedirs(cur_src_dir)

            # open next file for writing
            ofile = open(cur_src, "w")
            print >>sys.stderr, '=> %s' % cur_src
            manifest.append(cur_src)
            next(f)
        elif ofile:
            ofile.write(clean(l))

m = open('%s.manifest' % os.path.basename(fname), 'w')
print >>m, '\n'.join(manifest)
