#!/usr/bin/env python
#
# Generate a reversible amalgamation of several C source files
# along with their required internal headers.
#
# This script assumes that there are a bunch of C files, a bunch
# of private header files and one public header file.
#
# The script takes a list of C file names, parses `#include` directives
# found in them and recursively resolves dependencies in such a way
# that a header referenced from an included header will be emitted before the
# header that depends on it. All headers will always be emitted before the
# source files.
#
# The embedded include files usually contain private internals.
# However sometimes it's necessary for some other tools or for advanced users
# to have access to internal definitions. One such example is the generated
# C source containing frozen heap. The amalgamation script will allow users
# to include the amalgamated C file and cause extract only the internal headers:
#
#     #define NS_EXPORT_INTERNAL_HEADERS
#     #include "v7.c"
#
# Where `NS` can be overridden via the --prefix flag.
# This feature can be enabled with the --exportable-headers, and basically
# all it does is to wrap the C body in a preprocessor guard.
#
# TODO(mkm): make it work also for mongoose where we also generate
# the public header from a bunch of unamalgamated header files.
# Currently this script can handle mongoose amalgamation because it doesn't
# flip the --autoinc flag.
#

import argparse
import re
import sys
import os
from StringIO import StringIO

parser = argparse.ArgumentParser(description='Produce an amalgamated source')
parser.add_argument('--prefix', default="NS",
                    help='prefix for MODULE_LINES guard')
parser.add_argument('--srcdir', default=".", help='source dir')
parser.add_argument('--ignore', default="",
                    help='comma separated list of files to not amalgam')
# hack, teach amalgam to render the LICENSE file instead
parser.add_argument('--first', type=str, help='put this file in first position.'
                    ' Usually contains licensing info')
parser.add_argument('--public-header', dest="public",
                    help='name of the public header file that will be'
                    ' included at the beginning of the file')
parser.add_argument('--autoinc', action='store_true',
                    help='automatically embed include files')
parser.add_argument('--strict', action='store_true',
                    help='fail loudly if an include file cannot be resolved')
parser.add_argument('--norel', action='store_true',
                    help="do not try to compute a friendly relative path")
parser.add_argument('--exportable-headers', dest="export", action='store_true',
                    help='allow exporting internal headers')
parser.add_argument('-I', default=".", dest='include_path', help='include path')
parser.add_argument('sources', nargs='*', help='sources')

class File(object):
    def __init__(self, name, parent_name):
        self.name = name
        self.parent_name = parent_name
        self.buf = StringIO()
        emit_file(self.buf, self.name, self.parent_name)

    def emit(self):
        print self.buf.getvalue(),


args = parser.parse_args()

sources = []
includes = []

already_included = set()

ignore_files = [i.strip() for i in args.ignore.split(',')]

def should_ignore(name, parent_name):
    return (name in already_included
            or not (args.strict or os.path.exists(resolve(name, parent_name)))
            or name in ignore_files)

def resolve(path, parent_name):
    path_from_parent = None
    if parent_name != None and not os.path.isabs(path):
        # calculate the path relative to the "parent_name" file, i.e. to the file
        # which includes the current one.
        path_from_parent = os.path.join(os.path.dirname(parent_name), path)

    if os.path.isabs(path) or os.path.exists(path):
        p = path
    elif path_from_parent != None and os.path.exists(path_from_parent):
        p = path_from_parent
    else:
        p = os.path.join(args.include_path, path)
    if os.path.exists(p) and not args.norel:
        p = os.path.realpath(p).replace('%s%s' % (os.getcwd(), os.sep), '')
    # print >>sys.stderr, '%s -> %s (cwd %s)' % (path, p, os.getcwd())
    return p.replace(os.sep, '/')

def emit_line_directive(out, name, parent_name):
    print >>out, '''#ifdef %(prefix)s_MODULE_LINES
#line 1 "%(name)s"
#endif''' % dict(
    prefix = args.prefix,
    name = resolve(name, parent_name),
)

def emit_body(out, name, parent_name):
    resolved_name = resolve(name, parent_name)
    if not args.strict and not os.path.exists(resolved_name):
        print >>out, '#include "%s"' % (name,)
        return

    with open(resolved_name) as f:
        for l in f:
            match = re.match('( *#include "(.*)")', l)
            if match:
                all, path_to_include = match.groups()
                if args.autoinc:
                    if not should_ignore(path_to_include, parent_name):
                        already_included.add(path_to_include)
                        includes.append(File(path_to_include, resolved_name))
                print >>out, '/* Amalgamated: %s */' % (all,)
            else:
                print >>out, l,


def emit_file(out, name, parent_name):
    emit_line_directive(out, name, parent_name)
    emit_body(out, name, parent_name)

for i in args.sources:
    sources.append(File(i, None))

if args.first:
    for inc in reversed(args.first.split(',')):
        for i, f in enumerate(includes):
            if f.name == inc:
                del includes[i]
                includes.insert(0, f)
                break

# emitting

if sys.platform == "win32":
    import os, msvcrt
    msvcrt.setmode(sys.stdout.fileno(), os.O_BINARY)

if args.public:
    print '#include "%s"' % (args.public)

for i in includes:
    i.emit()

if args.export:
    print '#ifndef %s_EXPORT_INTERNAL_HEADERS' % (args.prefix,)
for i in sources:
    i.emit()
if args.export:
    print '#endif /* %s_EXPORT_INTERNAL_HEADERS */' % (args.prefix,)
