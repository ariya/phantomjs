#!/usr/bin/env python
# Copyright (C) 2012 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Usage: make-file-arrays.py [--condition=condition-string] --out-h=<header-file-name> --out-cpp=<cpp-file-name> <input-file>...

import os.path
import re
import sys
from optparse import OptionParser


def make_variable_name_and_read(file_name):
    result = re.match(r"([\w\d_]+)\.([\w\d_]+)", os.path.basename(file_name))
    if not result:
        print "Invalid input file name:", os.path.basename(file_name)
        sys.exit(1)
    variable_name = result.group(1)[0].lower() + result.group(1)[1:] + result.group(2).capitalize()
    file = open(file_name, "rb")
    content = file.read()
    file.close()
    return (variable_name, content)


def strip_whitespace_and_comments(file_name, content):
    result = re.match(r".*\.([^.]+)", file_name)
    if not result:
        print "The file name has no extension:", file_name
        sys.exit(1)
    extension = result.group(1).lower()
    multi_line_comment = re.compile(r"/\*.*?\*/", re.MULTILINE | re.DOTALL)
    single_line_comment = re.compile(r"//.*$", re.MULTILINE)
    repeating_space = re.compile(r"[ \t]+", re.MULTILINE)
    leading_space = re.compile(r"^[ \t]+", re.MULTILINE)
    trailing_space = re.compile(r"[ \t]+$", re.MULTILINE)
    empty_line = re.compile(r"\n+")
    if extension == "js":
        content = multi_line_comment.sub("", content)
        content = single_line_comment.sub("", content)
        content = repeating_space.sub(" ", content)
        content = leading_space.sub("", content)
        content = trailing_space.sub("", content)
        content = empty_line.sub("\n", content)
    elif extension == "css":
        content = multi_line_comment.sub("", content)
        content = repeating_space.sub(" ", content)
        content = leading_space.sub("", content)
        content = trailing_space.sub("", content)
        content = empty_line.sub("\n", content)
    return content


def main():
    parser = OptionParser()
    parser.add_option("--out-h", dest="out_header")
    parser.add_option("--out-cpp", dest="out_cpp")
    parser.add_option("--condition", dest="flag")
    (options, args) = parser.parse_args()
    if len(args) < 1:
        parser.error("Need one or more input files")
    if not options.out_header:
        parser.error("Need to specify --out-h=filename")
    if not options.out_cpp:
        parser.error("Need to specify --out-cpp=filename")

    if options.flag:
        options.flag = options.flag.replace(" AND ", " && ")
        options.flag = options.flag.replace(" OR ", " || ")

    header_file = open(options.out_header, "w")
    if options.flag:
        header_file.write("#if " + options.flag + "\n")
    header_file.write("namespace WebCore {\n")

    cpp_file = open(options.out_cpp, "w")
    cpp_file.write("#include \"config.h\"\n")
    cpp_file.write("#include \"" + os.path.basename(options.out_header) + "\"\n")
    if options.flag:
        cpp_file.write("#if " + options.flag + "\n")
    cpp_file.write("namespace WebCore {\n")

    for file_name in args:
        (variable_name, content) = make_variable_name_and_read(file_name)
        content = strip_whitespace_and_comments(file_name, content)
        size = len(content)
        header_file.write("extern const char %s[%d];\n" % (variable_name, size))
        cpp_file.write("const char %s[%d] = {\n" % (variable_name, size))
        for index in range(size):
            char_code = ord(content[index])
            if char_code < 128:
                cpp_file.write("%d" % char_code)
            else:
                cpp_file.write("'\\x%02x'" % char_code)
            cpp_file.write("," if index != len(content) - 1 else "};\n")
            if index % 20 == 19:
                cpp_file.write("\n")
        cpp_file.write("\n")

    header_file.write("}\n")
    if options.flag:
        header_file.write("#endif\n")
    header_file.close()

    cpp_file.write("}\n")
    if options.flag:
        cpp_file.write("#endif\n")
    cpp_file.close()


if __name__ == "__main__":
    main()
